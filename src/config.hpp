#pragma once
#include <string>

namespace waul {

struct ConfigState {
  int m[4] = {0, 0, 0, 0};    // Margins
  int bw[4] = {0, 0, 0, 0};   // Border Width
  int br[4] = {0, 0, 0, 0};   // Border Radius
  int bc[4] = {0, 0, 0, 255}; // Border Color
  int bg[4] = {0, 0, 0, 255}; // Background Color
};

class Config {
public:
  static ConfigState &get();
  static void load();
  static std::string get_path();

private:
  static ConfigState state;
};

} // namespace waul