{
  pkgs ? import <nixpkgs> { },
}:

pkgs.mkShell {
  nativeBuildInputs = with pkgs; [
    cmake
    pkg-config
    wayland-scanner
  ];

  buildInputs = with pkgs; [
    wayland
    wayland-protocols
    wlr-protocols
    stb
  ];

  shellHook = ''
    echo "waul development shell"
  '';
}
