#!/bin/bash -e

# read arguments
opts=$(getopt \
  --longoptions help,clang,setup,no-cmake,no-unity,build-type: \
  --name "$(basename "$0")" \
  --options "" \
  -- "$@"
)

eval set --$opts

RunCMake=true
BuildType="Dev"
NoUnity=""


while [[ $# -gt 0 ]]; do
  case "$1" in
    --help)
      echo "Usage: $(basename $0) [--setup] [--clang] [--no-cmake] [--build-type Debug|Dev|Shipping] [--no-unity]"
      echo "  --setup       Run first time setup. This installs dependencies and makes sure the git repository is setup correctly."
      echo "  --clang       Use clang instead of gcc"
      echo "  --no-cmake    Do not invoke cmake (usefull when only --setup is needed)"
      echo "  --build-type  Which build type cmake should be invoked with Debug|Dev|Shipping"
      echo "  --no-unity    Disable unity builds. This might help to improve code completion in various editors"
      exit 0
      ;;

    --clang)
      UseClang=true
      shift 1
      ;;

    --setup)
      Setup=true
      shift 1
      ;;

    --no-cmake)
      RunCMake=false
      shift 1
      ;;

    --no-unity)
      NoUnity="-DPL_ENABLE_FOLDER_UNITY_FILES=OFF"
      shift 1
      ;;

    --build-type)
      BuildType=$2
      shift 2
      ;;

	  
    *)
      break
      ;;
  esac
done

if [ "$BuildType" != "Debug" -a "$BuildType" != "Dev" -a "$BuildType" != "Release" ]; then
  >&2 echo "The build-type '${BuildType}' is not supported. Only Debug, Dev and Release are supported values."
  exit 1
fi

if [ ! -f "/etc/issue" ]; then
	>&2 echo "/etc/issue does not exist. Failed distribution detection"
	exit 1
fi

Issue=$(cat /etc/issue)

UbuntuPattern="Ubuntu ([0-9][0-9])"
MintPattern="Linux Mint ([0-9][0-9])"

if [[ $Issue =~ $UbuntuPattern ]]; then
  Distribution="Ubuntu"
  Version=${BASH_REMATCH[1]}
elif [[ $Issue =~ $MintPattern ]]; then
  Distribution="Mint"
  Version=${BASH_REMATCH[1]}
fi

# This requires a 'sort' that supports '-V'
verlte() {
    [  "$1" = "`echo -e "$1\n$2" | sort -V | head -n1`" ]
}

verlt() {
    [ "$1" = "$2" ] && return 1 || verlte $1 $2
}

if [ "$Distribution" = "Ubuntu" -a "$Version" = "22" ] || [ "$Distribution" = "Mint" -a "$Version" = "21" ]; then
  packages=(cmake build-essential ninja-build libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev uuid-dev mold libfreetype-dev libtinfo5)

  if [ "$UseClang" = true ]; then
    packages+=(clang-14 libstdc++-12-dev)
    c_compiler=clang-14
    cxx_compiler=clang++-14
  else
    packages+=(gcc-12 g++-12)
    c_compiler=gcc-12
    cxx_compiler=g++-12
  fi
else
  >&2 echo "Your Distribution or Distribution version is not supported by this script"
  >&2 echo "Currently supported are:"
  >&2 echo "  * Ubuntu 22"
  >&2 echo "  * Linux Mint 21"
  exit 1
fi

if [ "$Setup" = true ]; then
  qtVer=$(apt list qt6-base-dev 2>/dev/null | grep -o "6\.[0-9]*\.[0-9]")
  echo $qtVer

  if verlt $qtVer "6.3.0"; then
    >&2 echo -e "\033[0;33mYour distributions package manager does not provide Qt 6.3.0 or newer. Please install Qt manually."
    >&2 echo -e "See https://plasmaengine.github.io/PlasmaDocs/#Plasma2/build/build-linux/#automatic-setup"
  else
    packages+=(qt6-base-dev libqt6svg6-dev qt6-base-private-dev)
  fi

  git submodule update --init
  echo "Attempting to install the following packages through the package manager:"
  echo ${packages[@]}
  sudo apt install ${packages[@]}
fi

CompilerShort=gcc
if [ "$UseClang" = true ]; then
  CompilerShort=clang
fi

if [ "$RunCMake" = true ]; then
  BuildDir="build-${BuildType}-${CompilerShort}"
  cmake -B $BuildDir -S . -G Ninja -DCMAKE_CXX_COMPILER=$cxx_compiler -DCMAKE_C_COMPILER=$c_compiler -DPL_EXPERIMENTAL_EDITOR_ON_LINUX=ON -DPL_BUILD_EXPERIMENTAL_VULKAN=ON -DCMAKE_BUILD_TYPE=$BuildType -DCMAKE_EXPORT_COMPILE_COMMANDS=ON $NoUnity && \
  echo -e "\nRun 'ninja -C ${BuildDir}' to build"
fi