#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2024 Paulo Ferreira de Castro <pefcastro@gmail.com>
#
# This script makes armv7l gpiod Python wheels for combinations of libc (glibc
# and musl) and CPython versions, complementing the wheels made by cibuildwheel
# (through the 'generate_pypi_artifacts.sh' script).
#
# Modify the TARGET_PYTHON_VERSIONS global variable below in order to change
# the targeted CPython versions.
#
# Dependencies: Docker and binfmt_misc for ARM emulation.
#
# Usage:
# ./generate_armv7l_wheels.sh
#
# The wheels will be placed in the ./dist directory.
#

TARGET_PYTHON_VERSIONS=(3.9 3.10 3.11 3.12)
TARGET_ARCH='armv7l'

# Docker image used to run Python commands locally (whichever host CPU).
NATIVE_PYTHON_IMG="python:3.12-alpine"

quit() {
	echo -e "\n${1}"
	exit 1
}

# Print the gpiod Python binding version from gpiod/version.py.
print_gpiod_version() {
	set -x
	docker run --rm -iv "${PWD}/gpiod:/gpiod" "${NATIVE_PYTHON_IMG}" python <<-EOF
		import sys
		sys.path.insert(0, "/gpiod")
		from version import __version__
		print(__version__)
	EOF
	{ local status="$?"; set +x; } 2>/dev/null
	return "${status}"
}

# Make the sdist if it does not already exist in the './dist' directory.
ensure_sdist() {
	local gpiod_version
	gpiod_version="$(print_gpiod_version)" || \
		quit "Failed to determine the gpiod Python binding version"

	SDIST_TARBALL="gpiod-${gpiod_version}.tar.gz"
	if [ -f "dist/${SDIST_TARBALL}" ]; then
		return
	fi
	if [ -z "${LIBGPIOD_VERSION}" ]; then
		quit "Please set the LIBGPIOD_VERSION env var to enable making the sdist."
	fi
	set -x
	docker run --rm -v "${PWD}:/py_bindings" -w /py_bindings -e LIBGPIOD_VERSION \
		"${NATIVE_PYTHON_IMG}" python setup.py sdist
	{ local status="$?"; set +x; } 2>/dev/null
	return "${status}"
}

# Set the BASE_IMG array with details of the Docker image used to build wheels.
set_base_img_array() {
	local libc="$1" # The string 'glibc' or 'musl'
	declare -Ag BASE_IMG
	case "${libc}" in
		glibc)	BASE_IMG[name]='python'
				BASE_IMG[distro]='bullseye'
				BASE_IMG[platform]='linux/arm/v7'
				BASE_IMG[wheel_plat]="manylinux_2_17_${TARGET_ARCH}"
				BASE_IMG[deps]="
RUN apt-get update && apt-get install -y autoconf-archive
# auditwheel requires patchelf v0.14+, but Debian Bullseye comes with v0.12.
RUN curl -Ls 'https://github.com/NixOS/patchelf/releases/download/0.18.0/patchelf-0.18.0-${TARGET_ARCH}.tar.gz' \
	| tar -xzC /usr ./bin/patchelf
";;
		musl)	BASE_IMG[name]='python'
				BASE_IMG[distro]='alpine3.20'
				BASE_IMG[platform]='linux/arm/v7'
				BASE_IMG[wheel_plat]="musllinux_1_2_${TARGET_ARCH}"
				BASE_IMG[deps]="
RUN apk add autoconf autoconf-archive automake bash binutils curl \
	g++ git libtool linux-headers make patchelf pkgconfig
";;
	esac
}

# Make a "wheel builder" docker image used to build gpiod wheels.
# The base image is the official Python image on Docker Hub, in either the
# Debian variant (glibc) or the Alpine variant (musl). Currently using
# Debian 11 Bullseye because it supports building wheels targeting glibc
# v2.17. The newer Debian 12 Bookworm supports higher glibc versions only.
make_wheel_builder_image() {
	local builder_img_tag="$1"
	local python_version="$2"
	set -x
	docker build --platform "${BASE_IMG[platform]}" --pull --progress plain \
		--tag "${builder_img_tag}" --file - . <<-EOF
			FROM '${BASE_IMG[name]}:${python_version}-${BASE_IMG[distro]}'
			${BASE_IMG[deps]}
			RUN pip install auditwheel
		EOF
	{ local status="$?"; set +x; } 2>/dev/null
	return "${status}"
}

# Build a gpiod wheel in a container of the given builder image tag.
build_wheel() {
	local builder_img_tag="$1"
	set -x
	docker run --rm --platform "${BASE_IMG[platform]}" -iv "${PWD}/dist:/dist" \
		-w /tmp "${builder_img_tag}" bash <<-EOF
			set -ex
			pip wheel --no-deps "/dist/${SDIST_TARBALL}"
			auditwheel repair --plat "${BASE_IMG[wheel_plat]}" --wheel-dir /dist ./*.whl
		EOF
	{ local status="$?"; set +x; } 2>/dev/null
	return "${status}"
}

# Build gpiod wheels for combinations of libc and Python versions.
build_combinations() {
	local builder_img_tag=

	for libc in glibc musl; do
		set_base_img_array "${libc}"

		for ver in "${TARGET_PYTHON_VERSIONS[@]}"; do
			builder_img_tag="gpiod-wheel-builder-cp${ver/./}-${libc}-${TARGET_ARCH}"

			make_wheel_builder_image "${builder_img_tag}" "${ver}" || \
				quit "Failed to build Docker image '${builder_img_tag}'"

			build_wheel "${builder_img_tag}" || \
				quit "Failed to build gpiod wheel with image '${builder_img_tag}'"

			set -x
			docker image rm "${builder_img_tag}"
			{ set +x; } 2>/dev/null
		done
	done
}

main() {
	set -e
	local script_dir # Directory where this script is located
	script_dir="$(cd -- "$(dirname "$0")" >/dev/null 2>&1 || true; pwd -P)"
	cd "${script_dir}" || quit "'cd ${script_dir}' failed"
	ensure_sdist || quit "Failed to find or make the sdist tarball."
	build_combinations
	echo
	ls -l dist/*"${TARGET_ARCH}"*
	echo -e "\nAll done! Hint: Use 'docker system prune' to free disk space."
}

main
