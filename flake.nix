{
  inputs.nixpkgs.url = "github:NickCao/nixpkgs/nixos-unstable-small";
  outputs = { self, nixpkgs, ... }: with nixpkgs.legacyPackages.x86_64-linux; {
    packages.x86_64-linux.dhack = callPackage ./dhack.nix { inherit (linuxPackages_latest) kernel; };
  };
}
