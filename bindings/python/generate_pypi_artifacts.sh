#!/usr/bin/env sh
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2024 Vincent Fazio <vfazio@gmail.com>
#
# This is a script to generate an sdist and wheels for publishing to PyPI.
#
# This script requires:
#   * Python3 + venv or virtualenv + pip
#   * Docker or Podman (https://cibuildwheel.pypa.io/en/stable/options/#container-engine)
#   * binfmt support and qemu-user-static for AArch64 registered as Fixed
#     (https://docs.kernel.org/admin-guide/binfmt-misc.html)
#
# On Debian based systems, AArch64 binfmt support can be checked via:
#   cat /proc/sys/fs/binfmt_misc/qemu-aarch64
#
# The entry should be enabled and "F" should be in the list of flags.
#

usage()
{
	printf "\n"
	printf "Usage: %s -v <libgpiod_source_version> [-o <output_directory>] [-s <source_directory>] [-cfh]\n" "$0"
	printf "\t-v Version of libgpiod sources to bundle in sdist. Overrides LIBGPIOD_VERSION\n"
	printf "\t-o Directory to store outputs\n"
	printf "\t-s Directory with python binding sources\n"
	printf "\t-c Calculate checksums for generated outputs\n"
	printf "\t-f Forcibly remove old files from output directory\n"
	printf "\t-h Show this help output\n"
	exit 1
}

src_version=${LIBGPIOD_VERSION:-} # Default to environment specified library version
output_dir=$(pwd) # Default to putting outputs in the current directory
source_dir=$(pwd) # Assume the current directory has the python binding sources
calc_hash=0 # Do not calculate hashes by default
force=0 # Do not forcibly remove files by default

while getopts :hv:o:s:cf value; do
	case ${value} in
		c)
			calc_hash=1
			;;
		f)
			force=1
			;;
		o)
			output_dir=${OPTARG}
			;;
		s)
			source_dir=${OPTARG}
			;;
		v)
			src_version=${OPTARG}
			;;
		h | *)
			usage
			;;
	esac
done

if [ -z "${source_dir}" ] || [ ! -d "${output_dir}" ]; then
	printf "Invalid source directory %s.\n" "${source_dir}"
	exit 1
fi

if [ -z "${output_dir}" ] || [ ! -w "${output_dir}" ]; then
	printf "Output directory %s is not writable.\n" "${output_dir}"
	exit 1
fi

if [ -z "${src_version}" ]; then
	printf "The libgpiod source version must be specified.\n"
	exit 1
fi

shift $((OPTIND-1))

# We require Python3 for building artifacts
if ! command -v python3 >/dev/null 2>&1; then
	printf "Python3 is required to generate PyPI artifacts.\n"
	exit 1
fi

# Pip is necessary for installing build dependencies
if ! python3 -m pip -h >/dev/null 2>&1; then
	printf "The pip module is required to generate wheels.\n"
	exit 1
fi

# Check for a virtual environment tool to not pollute user installed packages
has_venv=$(python3 -m venv -h >/dev/null 2>&1 && echo 1 || echo 0)
has_virtualenv=$(python3 -m virtualenv -h >/dev/null 2>&1 && echo 1 || echo 0)

if ! { [ "${has_venv}" -eq 1 ] || [ "${has_virtualenv}" -eq 1 ]; }; then
	printf "A virtual environment module is required to generate wheels.\n"
	exit 1
fi

venv_module=$([ "${has_virtualenv}" -eq 1 ] && echo "virtualenv" || echo "venv" )

# Stage the build in a temp directory.
cur_dir=$(pwd)
temp_dir=$(mktemp -d)
cd "${temp_dir}" || { printf "Failed to enter temp directory.\n"; exit 1; }

# Setup a virtual environment
python3 -m "${venv_module}" .venv
venv_python="${temp_dir}/.venv/bin/python"

# Install build dependencies
# cibuildwheel 2.18.1 pins the toolchain containers to 2024-05-13-0983f6f
${venv_python} -m pip install build==1.2.1 cibuildwheel==2.18.1

LIBGPIOD_VERSION=${src_version} ${venv_python} -m build --sdist --outdir ./dist "${source_dir}"
sdist=$(find ./dist -name '*.tar.gz')

# Target only CPython and X86_64 + AArch64 Linux wheels unless specified otherwise via environment variables
CIBW_BUILD=${CIBW_BUILD:-"cp*"} CIBW_ARCHS=${CIBW_ARCHS:-"x86_64,aarch64"} \
	${venv_python} -m cibuildwheel --platform linux "${sdist}" --output-dir dist/

if [ "${force}" -eq 1 ]; then
	printf "\nRemoving files from %s/dist/\n" "${output_dir}"
	rm -rf "${output_dir}/dist/"
fi

cp -fa dist/ "${output_dir}/"

if [ "${calc_hash}" -eq 1 ]; then
	printf "\nHashes for generated outputs:\n"
	sha256sum "${output_dir}/dist/"*
fi

cd "${cur_dir}" || { printf "Failed to return to previous working directory.\n"; exit 1; }
rm -rf "${temp_dir}"
