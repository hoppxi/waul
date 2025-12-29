#include "ipc.hpp"
#include "common.hpp"
#include "wayland_backend.hpp"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

namespace waul {

static void send_str(int fd, const char* s) {
    if (write(fd, s, strlen(s)) < 0) {} 
}

static void send_data(int fd, const char* d, size_t len) {
    if (write(fd, d, len) < 0) {}
}

int ipc_send_command(const std::string& cmd, bool wait_response) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) return 1;

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, get_socket_path().c_str(), sizeof(addr.sun_path)-1);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        close(sock);
        if (access(get_socket_path().c_str(), F_OK) == 0) {
             unlink(get_socket_path().c_str());
        }
        return 1;
    }

    send_data(sock, cmd.c_str(), cmd.size());

    if (wait_response) {
        char buf[1024] = {0};
        ssize_t n = read(sock, buf, sizeof(buf)-1);
        if (n > 0) std::cout << buf << std::endl;
    }
    
    close(sock);
    return 0;
}

int ipc_server_init() {
    std::string path = get_socket_path();
    unlink(path.c_str());

    int sock = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (sock < 0) {
        log_msg(ERROR, "Failed to create IPC socket");
        return -1;
    }

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path)-1);

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        log_msg(ERROR, "Failed to bind IPC socket");
        close(sock);
        return -1;
    }

    if (listen(sock, 5) < 0) {
        close(sock);
        return -1;
    }

    log_msg(SUCCESS, "IPC Server listening on %s", path.c_str());
    return sock;
}

int ipc_server_accept(int server_fd) {
    return accept(server_fd, nullptr, nullptr);
}

void ipc_handle_client(int fd) {
    char buf[1024] = {0};
    if (read(fd, buf, sizeof(buf)-1) > 0) {
        std::string cmd(buf);
        log_msg(DEBUG, "IPC Recv: %s", buf);

        if (cmd == "ping") {
            send_str(fd, "pong");
        }
        else if (cmd == "quit") {
            Wayland::stop();
            send_str(fd, "bye");
        }
        else if (cmd == "query") {
            std::string p = Wayland::get_current_wallpaper();
            send_data(fd, p.c_str(), p.size());
        }
        else if (cmd.find("set|") == 0) {
            std::string p = cmd.substr(4);
            if (access(p.c_str(), F_OK) == 0) {
                Wayland::set_wallpaper(p);
                send_str(fd, "ok");
            } else {
                send_str(fd, "err: not found");
                log_msg(WARN, "IPC Request file not found: %s", p.c_str());
            }
        }
    }
    close(fd);
}

}