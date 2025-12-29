#include "wayland_backend.hpp"
#include "ipc.hpp"
#include "common.hpp"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <filesystem>
#include <vector>

using namespace waul;

void print_help(bool is_error = false) {
    std::ostream& out = is_error ? std::cerr : std::cout;
    if (is_error) out << "Error: Invalid arguments\n\n";
    
    out << "waul - Minimalist Wayland Wallpaper Daemon\n\n"
        << "Usage: waul [OPTIONS]\n\n"
        << "Options:\n"
        << "  --set <path>    Set wallpaper (starts daemon if needed)\n"
        << "  --query         Print current wallpaper path\n"
        << "  --reload        Restart daemon\n"
        << "  --quit          Stop daemon\n"
        << "  --ping          Check if daemon is running\n"
        << "  --version       Print version\n"
        << "  --help          Show this help message\n";
}

void print_version() {
    std::cout << "waul v0.1.0\n";
}

int main(int argc, char** argv) {
    log_init();

    if (argc > 1) {
        std::string action = argv[1];

        if (action == "--help" || action == "-h") {
            print_help();
            return 0;
        }
        else if (action == "--version" || action == "-v") {
            print_version();
            return 0;
        }
        else if (action == "--set") {
            if (argc < 3) {
                print_help(true);
                return 1;
            }
            std::string path = std::filesystem::absolute(argv[2]);
            
            if (ipc_send_command("set|" + path) != 0) {
                std::cout << "Daemon not running, starting...\n";
                if (fork() == 0) {
                    setsid();
                    std::string cache = get_cache_dir() + "/last_wall";
                    ensure_dir(get_cache_dir());
                    FILE* f = fopen(cache.c_str(), "w");
                    if(f) { fputs(path.c_str(), f); fclose(f); }
                    goto run_daemon;
                }
                return 0;
            }
            return 0;
        }
        else if (action == "--reload") {
            ipc_send_command("quit", false);
            usleep(200000); 
            if (fork() == 0) {
                setsid();
                goto run_daemon;
            }
            return 0;
        }
        else if (action == "--quit") return ipc_send_command("quit");
        else if (action == "--query") return ipc_send_command("query");
        else if (action == "--ping") {
             if (ipc_send_command("ping") != 0) {
                 std::cout << "Daemon not running.\n";
                 return 1;
             }
             return 0;
        }
        else {
            print_help(true);
            return 1;
        }
    }

    if (ipc_send_command("ping", false) == 0) {
        std::cerr << "Daemon is already running.\n";
        return 0;
    }

    if (fork() != 0) return 0;
    setsid();

run_daemon:
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    if (Wayland::init()) {
        Wayland::run();
    }
    return 0;
}