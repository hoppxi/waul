#include "common.hpp"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <sys/stat.h>
#include <unistd.h>

namespace fs = std::filesystem;

namespace waul {

static FILE *log_file = nullptr;

void ensure_dir(const std::string &path) {
  if (!fs::exists(path)) {
    fs::create_directories(path);
  }
}

std::string get_config_dir() {
  const char *cf = getenv("XDG_CONFIG_HOME");
  std::string path = cf ? std::string(cf) + "/waul"
                        : std::string(getenv("HOME")) + "/.config/waul";
  ensure_dir(path);
  return path;
}

std::string get_cache_dir() {
  const char *ch = getenv("XDG_CACHE_HOME");
  std::string path = ch ? std::string(ch) + "/waul"
                        : std::string(getenv("HOME")) + "/.cache/waul";
  ensure_dir(path);
  return path;
}

std::string get_data_home_dir() {
  const char *dt = getenv("XGD_DATA_HOME");
  std::string path = dt ? std::string(dt) + "/waul"
                        : std::string(getenv("HOME")) + "/.local/share/waul";
  ensure_dir(path);
  return path;
}

std::string get_runtime_dir() {
  const char *rd = getenv("XDG_RUNTIME_DIR");
  if (!rd)
    return "/tmp";
  std::string path = std::string(rd) + "/waul";
  ensure_dir(path);
  return path;
}

std::string get_socket_path() { return get_runtime_dir() + "/waul.sock"; }

void log_init() {
  std::string path = get_data_home_dir() + "/waul.log";
  log_file = fopen(path.c_str(), "a");
}

void log_msg(LogLevel level, const char *fmt, ...) {
  if (!log_file)
    return;

  time_t now = time(nullptr);
  char tbuf[20];
  strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", localtime(&now));

  const char *tag = "[UNK]";
  switch (level) {
  case DEBUG:
    tag = "[DBG]";
    break;
  case INFO:
    tag = "[INF]";
    break;
  case SUCCESS:
    tag = "[OK ]";
    break;
  case WARN:
    tag = "[WRN]";
    break;
  case ERROR:
    tag = "[ERR]";
    break;
  }

  fprintf(log_file, "%s %s ", tbuf, tag);

  va_list args;
  va_start(args, fmt);
  vfprintf(log_file, fmt, args);
  va_end(args);

  fprintf(log_file, "\n");
  fflush(log_file);
}

} // namespace waul