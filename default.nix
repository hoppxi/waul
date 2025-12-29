{
  lib,
  stdenv,
  cmake,
  pkg-config,
  wayland-scanner,
  wayland,
  wayland-protocols,
  wlr-protocols,
  stb,
}:

stdenv.mkDerivation {
  pname = "waul";
  version = "0.1.0";
  src = ./.;

  nativeBuildInputs = [
    cmake
    pkg-config
    wayland-scanner
  ];
  buildInputs = [
    wayland
    wayland-protocols
    wlr-protocols
    stb
  ];

  cmakeFlags = [ "-DCMAKE_BUILD_TYPE=Release" ];

  installPhase = ''
    runHook preInstall
    mkdir -p $out/bin
    install -Dm755 waul $out/bin/waul
    runHook postInstall
  '';

  meta = with lib; {
    description = "Minimalist Wayland wallpaper daemon";
    license = licenses.mit;
    platforms = platforms.linux;
  };
}
