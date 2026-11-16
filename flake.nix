#################
#
# quickshell-bar - A simple desktop bar built with Quickshell that I'm developing mostly for myself :)
# Copyright (C) 2026 The caelestia-shell contributors (see caelestia-shell-contributors.txt), Andrej Wolfenhaut <andrejwolfenhaut@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
# Some code is reused from the caelestia-shell project (https://github.com/caelestia-dots/shell).
# In the following section of every source file, the relevant reused code and/or ideas is/are described.
# Otherwise, this comment ends here.
#
# - From flake.nix
#   - General idea of how to package/develop a Quickshell application with a C++ plugin using a Nix flake
#   - Which build tools to use (CMake, ninja)
#
#################

{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";

    quickshell = {
      url = "git+https://git.outfoxxed.me/outfoxxed/quickshell";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs =
    {
      self,
      nixpkgs,
      quickshell,
      ...
    }:
    let
      system = "x86_64-linux";

      pkgs = nixpkgs.legacyPackages.${system};

      qtModules = with pkgs.qt6; [ ];
      quickshell-with-qt-modules =
        (quickshell.packages.${system}.default.override {
          withX11 = false;
          withI3 = false;
        }).withModules qtModules;
    in
    {
      packages.${system} = rec {
        quickshell-bar = pkgs.callPackage ./nix {
          stdenv = pkgs.clangStdenv;

          inherit qtModules quickshell-with-qt-modules;
        };
        default = quickshell-bar;
      };

      devShells.${system} = {
        default =
          let
            quickshell-bar = self.packages.${system}.quickshell-bar;
          in
          pkgs.mkShell.override { inherit (quickshell-bar) stdenv; } {
            inputsFrom = [
              quickshell-bar
              quickshell-bar.plugin
            ];
            packages = [ self.formatter.${system} ];
          };
      };

      apps.${system} = rec {
        quickshell-bar = {
          type = "app";
          program = "${self.packages.${system}.quickshell-bar}/bin/quickshell-bar";
        };


        # Non-Nix-aware VSCode settings are located as a template at /nix/vsCodeSettingsTemplate.
        # The are used as a template for generating Nix-aware VSCode settings under /.vscode, e.g. the compiler path is inserted.
        # To generate the Nix-aware VSCode settings, run the generate-vscode-settings app from the flake.
        # (nix run .#generate-vscode-settings)
        generate-vscode-settings =
        let
          qmlls-with-import-paths = pkgs.callPackage (
            {
              stdenv,
              makeWrapper,
              kdePackages,
            }:
            stdenv.mkDerivation {
              pname = "qmlls-with-import-paths";
              inherit (kdePackages.qtdeclarative) version;

              buildInputs = [ kdePackages.qtdeclarative ];
              nativeBuildInputs = [ makeWrapper ];

              dontUnpack = true;
              dontWrapQtApps = true;

              installPhase = ''
                mkdir -p "$out/bin"
                makeWrapper ${kdePackages.qtdeclarative}/bin/qmlls "$out/bin/qmlls" \
                  --add-flags "-I ${kdePackages.qtdeclarative}/lib/qt-6/qml" \
                  --add-flags "-I ${quickshell-with-qt-modules}/lib/qt-6/qml"
              '';
            }
          ) { };

          generationScript = ''
            changeFile "settings.json"
            assign '."qt-qml.qmlls.customExePath"' '"${qmlls-with-import-paths}/bin/qmlls"'
          
            changeFile "c_cpp_properties.json"
            assign '.configurations[.configurations|length - 1].compilerPath' '"${pkgs.clang}/bin/clang++"'
            for includeSubpath in ${pkgs.qt6.qtbase}/include/*; do
              append '.configurations[.configurations|length - 1].includePath' '"'"$includeSubpath"'"'
            done
            for includeSubpath in ${pkgs.qt6.qtdeclarative}/include/*; do
              append '.configurations[.configurations|length - 1].includePath' '"'"$includeSubpath"'"'
            done
          '';
        in
        {
          type = "app";
          program = "${pkgs.callPackage ./nix/generateVSCodeSettings.nix { inherit generationScript; }}/bin/generate-vscode-settings";
        };

        default = quickshell-bar;
      };

      formatter.${system} = pkgs.writeShellApplication {
        name = "nixfmt";
        runtimeInputs = [ pkgs.nixfmt ];
        text = ''
          exec nixfmt --strict "$@"
        '';
      };
    };
}
