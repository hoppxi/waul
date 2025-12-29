#pragma once
#include <string>
#include <wayland-client.h>

namespace waul {

struct Buffer {
    wl_buffer* wlbuf = nullptr;
    int fd = -1;
    int w = 0, h = 0;
    size_t size = 0;
};

class Renderer {
public:
    static void init(wl_shm* shm);
    static void resize_buffer(int w, int h);
    static void draw(const std::string& image_path, wl_surface* surf);
    static void cleanup();
    static const Buffer& get_buffer() { return buf; }

private:
    static Buffer buf;
    static wl_shm* shm_ref;
};

}