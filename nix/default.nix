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
# - From nix/default.nix
#   - Structure of Nix derivation for Quickshell application with C++ plugin
#
#################

{
  lib,
  stdenv,
  makeWrapper,
  bash,
  cmake,
  ninja,
  qt6,
  quickshell-with-modules,
  debug ? false,
  extraRuntimeDependencies ? [ ],
  ...
}:
let
  version = "0.0.1";

  cmakeBuildType = if debug then "Debug" else "RelWithDebInfo";

  quickshell-bar-plugin = stdenv.mkDerivation {
    pname = "quickshell-bar-plugin${lib.optionalString debug "-debug"}";
    inherit version;

    src = lib.fileset.toSource {
      root = ./..;
      fileset = lib.fileset.intersection (lib.fileset.fromSource (lib.sources.cleanSource ./..)) (
        lib.fileset.unions [
          ../CMakeLists.txt
          ../src/CMakeLists.txt
          ../src/plugin
        ]
      );
    };

    nativeBuildInputs = [
      cmake
      ninja
    ];
    buildInputs = [
      qt6.qtbase
      qt6.qtdeclarative
    ];

    dontWrapQtApps = true;

    inherit cmakeBuildType;
    cmakeFlags = [
      (lib.cmakeFeature "ENABLE_MODULE" "plugin")
      (lib.cmakeFeature "INSTALL_QMLDIR" qt6.qtbase.qtQmlPrefix)
    ];
  };

  runtimeDependencies = [ bash ] ++ extraRuntimeDependencies;
in
stdenv.mkDerivation {
  pname = "quickshell-bar${lib.optionalString debug "-debug"}";
  inherit version;

  src = lib.fileset.toSource {
    root = ./..;
    fileset = lib.fileset.intersection (lib.fileset.fromSource (lib.sources.cleanSource ./..)) (
      lib.fileset.unions [
        ../CMakeLists.txt
        ../src/CMakeLists.txt
        ../src/shell
      ]
    );
  };

  nativeBuildInputs = [
    cmake
    ninja
    makeWrapper
    qt6.wrapQtAppsHook
  ];
  buildInputs = [
    quickshell-with-modules
    quickshell-bar-plugin
    qt6.qtbase
  ];
  propagatedBuildInputs = runtimeDependencies;

  inherit cmakeBuildType;
  cmakeFlags = [
    (lib.cmakeFeature "ENABLE_MODULE" "shell")
    (lib.cmakeFeature "INSTALL_QSCONFDIR" "${placeholder "out"}/share/quickshell-bar")
  ];

  dontStrip = debug;

  postInstall = ''
    mkdir -p "$out/bin"
    makeWrapper "${quickshell-with-modules}/bin/qs" "$out/bin/quickshell-bar" \
      --prefix PATH : "${lib.makeBinPath runtimeDependencies}" \
      --add-flags "--path $out/share/quickshell-bar"
  '';

  passthru = {
    plugin = quickshell-bar-plugin;
  };
}
