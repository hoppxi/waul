#pragma once
#include <string>

namespace waul {

class Wayland {
public:
  static int init();
  static void run();
  static void stop();
  static void set_wallpaper(const std::string &path);
  static std::string get_current_wallpaper();

private:
  static bool running;
  static std::string current_wall;
};

} // namespace waul