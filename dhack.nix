{ stdenv, kernel }:

stdenv.mkDerivation {
  name = "dhack";
  src = ./.;
  hardeningDisable = [ "pic" ];
  nativeBuildInputs = kernel.moduleBuildDependencies;
  makeFlags = kernel.makeFlags ++ [
    "-C ${kernel.dev}/lib/modules/${kernel.modDirVersion}/build"
    "INSTALL_MOD_PATH=$(out)"
    "M=$(PWD)"
  ];
  installTargets = "modules_install";
}
