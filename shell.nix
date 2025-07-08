{ pkgs ? import <nixpkgs> {} }:
let
in
  pkgs.mkShell {
    buildInputs = [
      pkgs.platformio
      pkgs.esptool
      pkgs.python312Packages.pip
      pkgs.python312Packages.zopfli
      pkgs.python312Packages.wheel
      pkgs.python312Packages.pip
      pkgs.python312Packages.intelhex
      pkgs.python312Packages.wheel
      pkgs.python312Packages.pyyaml
      pkgs.python312Packages.pygments
      pkgs.python312Packages.mdurl
      pkgs.python312Packages.markdown-it-py
      pkgs.python312Packages.rich
      pkgs.python312Packages.rich-click
      # optional: needed as a programmer i.e. for esp32
      # pkgs.avrdude
    ];
    shellHook = ''
export PLATFORMIO_CORE_DIR=$PWD/.platformio
'';
}
