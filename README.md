# waul

A hypr-minimalist Wayland wallpaper daemon. (Only wayland, wlroots based compositors e.g. hyprland, sway.)

The goal of `waul` is to stay under 2MB of RAM, and to provid the exact styling options needed to create "floating wallpaper" looks (like Quickshell or specialized bars) without the bloat of animations or heavy dependencies. By adjusting margins and borders, you can create a framed wallpaper that sits below your status bar or floats centered on your screen.

## Included

- Ultra-lightweight: Minimal C++ footprint (~2MB RAM).
- Custom Margins: Drop the top margin to accommodate your bar.
- Borders & Radius: Native software-rendered rounded corners and borders.
- Instant Config: Changes to `config.ini` are applied whenever you set a new wallpaper.

## Configuration

uses `~/.config/waul/config.ini`:

```ini
# Margins: Top Left Bottom Right
margin = 30 0 0 0
# lets you to set 30px margin from the top letting your bar backgroud black

# Border Width: Top Left Bottom Right
border_width = 0 0 0 0
# lets you add border width to the specified position, 0 in this case

# Border Radius: TL TR BR BL
border_radius = 20 20 0 0
# lets you to set rounded corner to the wallaper letting you have the same wallpaper style like quickshell setups

# Border Color: R G B A
border_color = 0 0 0 255 ; lets you set broder color

# Background Color (shows in margins and border radius): R G B A
background_color = 0 0 0 255
# lets you set color to show behind the cropped section to match the color with your bar
```

## Installtion

### NixOS / Home Manager

Add `waul` to your flake inputs and use the provided module:

```nix

inputs.waul = {
  url = "github:hoppxi/waul";
  inputs.nixpkgs.follows = "nixpkgs";
};

# In your NixOS or Home Manager config:
imports = [
  inputs.waul.homeModules.default # or nixosModules.default
];

services.waul = {
  enable = true;
  margin = "40 20 20 20";  # Top Left Bottom Right
  border-radius = "15";    # TL TR BR BL
  borer-width = "0 0 0 0"; # TL TR BR BL
  border-color = "255 255 255 150"; # RGBA
  background-color = "0 0 0 255";   # RGBA
};

```

### Building on other distro

#### 1. Dependencies

Make sure you have the following installed:

- **Build Tools:** `cmake`, `pkg-config`, `gcc` or `clang`
- **Libraries:** `wayland`, `wayland-protocols`, `wlr-protocols`, `stb`

**On Arch:**

```bash
sudo pacman -S cmake pkgconf wayland wayland-protocols wlr-protocols stb

```

**On Ubuntu/Debian:**

```bash
sudo apt install cmake pkg-config libwayland-dev wayland-protocols
# wlr-protocols and stb may need to be cloned manually if not in your repos

```

#### 2. Build

```bash
git clone git@github.com:hoppxi/waul.git
cd waul
cmake -B build
cmake --build build
sudo cp build/waul /usr/local/bin/

```

#### 3. Setup Config

Create the config file manually:

```bash
mkdir -p ~/.config/waul
touch ~/.config/waul/config.ini
# copy the example config above

```

## License

MIT.
