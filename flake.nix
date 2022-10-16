{
  inputs.nixpkgs.url = "github:NickCao/nixpkgs/nixos-unstable-small";
  outputs = { self, nixpkgs, ... }: with nixpkgs.legacyPackages.x86_64-linux; {
    packages.x86_64-linux.dhack = linuxPackages_latest.callPackage ./dhack.nix { };
  };
}
