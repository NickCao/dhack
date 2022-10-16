{
  inputs.nixpkgs.url = "github:NickCao/nixpkgs/nixos-unstable-small";
  outputs = { self, nixpkgs, ... }: with nixpkgs.legacyPackages.x86_64-linux; {
    packages.x86_64-linux.dhack = let kernel = linuxPackages_latest.kernel; in stdenv.mkDerivation {
      name = "dhack";
      src = ./.;
      hardeningDisable = [ "pic" "format" ];
      nativeBuildInputs = kernel.moduleBuildDependencies;
      CROSS_COMPILE = stdenv.cc.targetPrefix;
      ARCH = stdenv.hostPlatform.linuxArch;
      makeFlags = [
        "-C${kernel.dev}/lib/modules/${kernel.modDirVersion}/build"
        "INSTALL_MOD_PATH=$(out)"
        "M=$(PWD)"
      ];
      installTargets = "modules_install";
    };
  };
}
