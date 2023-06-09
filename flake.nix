{
  description = "ExpidusOS File Manager";

  nixConfig = rec {
    trusted-public-keys = [ "cache.nixos.org-1:6NCHdD59X431o0gWypbMrAURkbJ16ZPMQFGspcDShjY=" "cache.garnix.io:CTFPyKSLcx5RMJKfLo5EEPUObbA78b0YQ2DTCJXqr9g=" ];
    substituters = [ "https://cache.nixos.org" "https://cache.garnix.io" ];
    trusted-substituters = substituters;
    fallback = true;
    http2 = false;
  };

  inputs.expidus-sdk = {
    url = github:ExpidusOS/sdk;
    inputs.nixpkgs.follows = "nixpkgs";
  };

  inputs.nixpkgs.url = github:ExpidusOS/nixpkgs;

  outputs = { self, expidus-sdk, nixpkgs }:
    with expidus-sdk.lib;
    flake-utils.eachSystem flake-utils.allSystems (system:
      let
        pkgs = expidus-sdk.legacyPackages.${system};
        pname = "expidus-keebie";
      in {
        packages.default = pkgs.flutter.buildFlutterApplication {
          inherit pname;
          version = "0.1.0-${self.shortRev or "dirty"}";

          src = cleanSource self;

          depsListFile = ./deps.json;
          vendorHash = "sha256-L/meECAUf0Jv5glifPNjylXIzroDsJtsyXZs1BPa23c=";

          nativeBuildInputs = with pkgs; [
            removeReferencesTo
            pkg-config
          ];

          buildInputs = with pkgs; [
            wayland
            gtk-layer-shell
          ];

          disallowedReferences = with pkgs; [
            flutter
            flutter.unwrapped
          ];

          meta = {
            description = "ExpidusOS Virtual Keyboard";
            homepage = "https://expidusos.com";
            license = licenses.gpl3;
            maintainers = with maintainers; [ RossComputerGuy ];
            platforms = [ "x86_64-linux" "aarch64-linux" ];
          };
        };

        devShells.default = pkgs.mkShell {
          name = pname;

          packages = with pkgs; [
            wayland
            libxkbcommon
            gtk-layer-shell
            flutter
            pkg-config
          ];
        };
      });
}
