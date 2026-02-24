#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

build_type="Debug"
build_dir="build-debug"
example_target=""

print_help() {
  cat <<'USAGE'
Usage: ./scripts/compile.sh [--release] [--debug] [--example <name>] [--all]

Builds PrimeStudio example binaries.

Options:
  --release          Build with Release configuration (default: Debug)
  --debug            Build with Debug configuration
  --example <name>   Example to build: render_demo | widgets_demo | <cmake-target>
  --all              Build all examples (default)
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --release)
      build_type="Release"
      build_dir="build-release"
      shift
      ;;
    --debug)
      build_type="Debug"
      build_dir="build-debug"
      shift
      ;;
    --example)
      if [[ $# -lt 2 ]]; then
        echo "--example requires a value" >&2
        exit 1
      fi
      case "$2" in
        render_demo)
          example_target="primestudio_render_demo"
          ;;
        widgets_demo)
          example_target="primestudio_widgets_demo"
          ;;
        *)
          example_target="$2"
          ;;
      esac
      shift 2
      ;;
    --all)
      example_target=""
      shift
      ;;
    -h|--help)
      print_help
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      print_help
      exit 1
      ;;
  esac
done

cxx_compiler=""
if [[ "$(uname -s)" == "Darwin" ]]; then
  cxx_compiler="clang++"
fi

primestage_repo=""
primeframe_repo=""
primemanifest_repo=""
primestage_source_dir=""
primeframe_source_dir=""
primemanifest_source_dir=""
if [[ -d "$root_dir/../PrimeStage/.git" ]]; then
  primestage_repo="$root_dir/../PrimeStage"
  primestage_source_dir="$root_dir/../PrimeStage"
fi
if [[ -d "$root_dir/../PrimeFrame/.git" ]]; then
  primeframe_repo="$root_dir/../PrimeFrame"
  primeframe_source_dir="$root_dir/../PrimeFrame"
fi
if [[ -d "$root_dir/../PrimeManifest/.git" ]]; then
  primemanifest_repo="$root_dir/../PrimeManifest"
  primemanifest_source_dir="$root_dir/../PrimeManifest"
fi

cmake -S "$root_dir" -B "$root_dir/$build_dir" \
  -DCMAKE_BUILD_TYPE="$build_type" \
  -DPRIMESTUDIO_BUILD_EXAMPLES=ON \
  -DPRIMESTUDIO_BUILD_TESTS=OFF \
  ${primestage_repo:+-DPRIMESTUDIO_PRIMESTAGE_GIT_REPOSITORY="$primestage_repo"} \
  ${primeframe_repo:+-DPRIMEFRAME_GIT_REPOSITORY="$primeframe_repo"} \
  ${primemanifest_repo:+-DPRIMESTUDIO_PRIMEMANIFEST_GIT_REPOSITORY="$primemanifest_repo"} \
  ${primestage_source_dir:+-DFETCHCONTENT_SOURCE_DIR_PRIMESTAGE="$primestage_source_dir"} \
  ${primeframe_source_dir:+-DFETCHCONTENT_SOURCE_DIR_PRIMEFRAME="$primeframe_source_dir"} \
  ${primemanifest_source_dir:+-DFETCHCONTENT_SOURCE_DIR_PRIMEMANIFEST="$primemanifest_source_dir"} \
  ${cxx_compiler:+-DCMAKE_CXX_COMPILER="$cxx_compiler"}

if [[ -n "$example_target" ]]; then
  cmake --build "$root_dir/$build_dir" --target "$example_target"
else
  cmake --build "$root_dir/$build_dir"
fi
