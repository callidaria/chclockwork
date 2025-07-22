{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
    buildInputs = [
        pkgs.g++
        pkgs.make
        pkgs.glew
        pkgs.SDL2
        pkgs.glm
        pkgs.assimp
        pkgs.freetype
    ];
}
