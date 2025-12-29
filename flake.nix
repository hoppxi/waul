{
  description = "waul - hypr-minimalist Wayland wallpaper daemon";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs =
    { self, nixpkgs }:
    let
      supportedSystems = [
        "x86_64-linux"
        "aarch64-linux"
      ];
      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;
      pkgsFor = system: import nixpkgs { inherit system; };
    in
    {
      packages = forAllSystems (system: {
        default =
          let
            pkgs = pkgsFor system;
          in
          pkgs.stdenv.mkDerivation {
            pname = "waul";
            version = "0.1.0";
            src = ./.;

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

            cmakeFlags = [ "-DCMAKE_BUILD_TYPE=Release" ];

            installPhase = ''
              mkdir -p $out/bin
              install -Dm755 waul $out/bin/waul
            '';
          };
      });

      devShells = forAllSystems (system: {
        default =
          let
            pkgs = pkgsFor system;
          in
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
          };
      });

      moduleLogic =
        {
          config,
          lib,
          pkgs,
          ...
        }:
        let
          cfg = config.services.waul;
          waulPkg = self.packages.${pkgs.system}.default;
          confFile = pkgs.writeText "waul-config.ini" ''
            margin = ${cfg.margin}
            border_width = ${cfg.border-width}
            border_radius = ${cfg.border-radius}
            border_color = ${cfg.border-color}
            background_color = ${cfg.background-color}
          '';
        in
        {
          options.services.waul = {
            enable = lib.mkEnableOption "waul daemon";
            margin = lib.mkOption {
              type = lib.types.str;
              default = "0 0 0 0";
            };
            border-width = lib.mkOption {
              type = lib.types.str;
              default = "0 0 0 0";
            };
            border-radius = lib.mkOption {
              type = lib.types.str;
              default = "0 0 0 0";
            };
            border-color = lib.mkOption {
              type = lib.types.str;
              default = "255 255 255 255";
            };
            background-color = lib.mkOption {
              type = lib.types.str;
              default = "0 0 0 255";
            };
          };

          config = lib.mkIf cfg.enable {
            xdg.configFile."waul/config.ini".source = confFile;

            systemd.user.services.waul = {
              Unit = {
                Description = "waul wallpaper daemon";
                After = [ "graphical-session-pre.target" ];
                PartOf = [ "graphical-session.target" ];
              };
              Service = {
                ExecStart = "${waulPkg}/bin/waul";
                Restart = "on-failure";
              };
              Install.WantedBy = [ "graphical-session.target" ];
            };
          };
        };

      nixosModules.default = self.moduleLogic;
      homeModules.default = self.moduleLogic;
    };
}
