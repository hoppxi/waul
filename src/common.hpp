#pragma once
#include <string>
#include <vector>

namespace waul {

enum LogLevel { DEBUG, INFO, SUCCESS, WARN, ERROR };

void log_init();
void log_msg(LogLevel level, const char* fmt, ...);

std::string get_config_dir();
std::string get_cache_dir();
std::string get_runtime_dir();
std::string get_socket_path();
void ensure_dir(const std::string& path);

}