#include "wayland_backend.hpp"
#include "common.hpp"
#include "config.hpp"
#include "ipc.hpp"
#include "renderer.hpp"

#include <wayland-client.h>

#define namespace _namespace
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#undef namespace

#include <cstdio>
#include <cstring>
#include <poll.h>
#include <unistd.h>

namespace waul {

bool Wayland::running = false;
std::string Wayland::current_wall;

static wl_display *display;
static wl_compositor *compositor;
static wl_shm *shm;
static zwlr_layer_shell_v1 *layer_shell;
static wl_surface *surface;
static zwlr_layer_surface_v1 *layer_surface;

static void registry_add(void *, wl_registry *reg, uint32_t name,
                         const char *iface, uint32_t) {
  if (strcmp(iface, wl_compositor_interface.name) == 0)
    compositor = (wl_compositor *)wl_registry_bind(reg, name,
                                                   &wl_compositor_interface, 4);
  else if (strcmp(iface, wl_shm_interface.name) == 0)
    shm = (wl_shm *)wl_registry_bind(reg, name, &wl_shm_interface, 1);
  else if (strcmp(iface, zwlr_layer_shell_v1_interface.name) == 0)
    layer_shell = (zwlr_layer_shell_v1 *)wl_registry_bind(
        reg, name, &zwlr_layer_shell_v1_interface, 1);
}

static const wl_registry_listener reg_listener = {
    .global = registry_add,
    .global_remove = [](void *, wl_registry *, uint32_t) {}};

static void layer_surface_configure(void *, struct zwlr_layer_surface_v1 *ls,
                                    uint32_t serial, uint32_t w, uint32_t h) {
  zwlr_layer_surface_v1_ack_configure(ls, serial);
  if (w == 0)
    w = 1920;
  if (h == 0)
    h = 1080;

  // Only resize if actual dimensions changed
  const auto &buf = Renderer::get_buffer();
  if (buf.w != (int)w || buf.h != (int)h) {
    Renderer::resize_buffer(w, h);
    Renderer::draw(Wayland::get_current_wallpaper(), surface);
  }
}

static const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
    .configure = layer_surface_configure,
    .closed = [](void *, auto *) { Wayland::stop(); }};

int Wayland::init() {
  display = wl_display_connect(nullptr);
  if (!display) {
    log_msg(ERROR, "Could not connect to Wayland display");
    return 0;
  }

  wl_registry *reg = wl_display_get_registry(display);
  wl_registry_add_listener(reg, &reg_listener, nullptr);
  wl_display_roundtrip(display);

  if (!compositor || !layer_shell || !shm) {
    log_msg(ERROR, "Missing required Wayland interfaces (compositor, shm, or "
                   "layer_shell)");
    return 0;
  }

  Renderer::init(shm);

  surface = wl_compositor_create_surface(compositor);
  layer_surface = zwlr_layer_shell_v1_get_layer_surface(
      layer_shell, surface, nullptr, ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND,
      "waul-wallpaper");

  zwlr_layer_surface_v1_add_listener(layer_surface, &layer_surface_listener,
                                     nullptr);
  zwlr_layer_surface_v1_set_anchor(layer_surface, 15); // All 4 sides
  zwlr_layer_surface_v1_set_exclusive_zone(layer_surface, -1);
  wl_surface_commit(surface);
  wl_display_flush(display);

  // Load last wallpaper
  std::string cache_file = get_cache_dir() + "/last_wall";
  FILE *f = fopen(cache_file.c_str(), "r");
  if (f) {
    char buf[1024];
    if (fgets(buf, sizeof(buf), f))
      current_wall = buf;
    fclose(f);
  }

  running = true;
  log_msg(SUCCESS, "Wayland backend initialized");
  return 1;
}

void Wayland::stop() { running = false; }

void Wayland::set_wallpaper(const std::string &path) {
  current_wall = path;
  // Save to cache
  std::string cache_file = get_cache_dir() + "/last_wall";
  FILE *f = fopen(cache_file.c_str(), "w");
  if (f) {
    fputs(path.c_str(), f);
    fclose(f);
  }

  Renderer::draw(current_wall, surface);
  log_msg(INFO, "Wallpaper set: %s", path.c_str());
}

std::string Wayland::get_current_wallpaper() { return current_wall; }

void Wayland::run() {
  int ipc_sock = ipc_server_init();
  if (ipc_sock < 0)
    return;

  struct pollfd fds[2];
  fds[0].fd = wl_display_get_fd(display);
  fds[0].events = POLLIN;
  fds[1].fd = ipc_sock;
  fds[1].events = POLLIN;

  log_msg(INFO, "Entering main loop");

  while (running) {
    while (wl_display_prepare_read(display) != 0)
      wl_display_dispatch_pending(display);
    wl_display_flush(display);

    if (poll(fds, 2, -1) < 0) {
      wl_display_cancel_read(display);
      break;
    }

    if (fds[0].revents & POLLIN) {
      wl_display_read_events(display);
      wl_display_dispatch_pending(display);
    } else {
      wl_display_cancel_read(display);
    }

    if (fds[1].revents & POLLIN) {
      int client = ipc_server_accept(ipc_sock);
      if (client >= 0)
        ipc_handle_client(client);
    }
  }

  Renderer::cleanup();
  close(ipc_sock);
  unlink(get_socket_path().c_str());
  if (display)
    wl_display_disconnect(display);
  log_msg(INFO, "Daemon exit");
}

} // namespace waul