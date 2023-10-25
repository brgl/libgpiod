# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

from os import getenv, path, unlink
from shutil import copytree, rmtree

from setuptools import Extension, find_packages, setup
from setuptools.command.build_ext import build_ext as orig_build_ext
from setuptools.command.sdist import log
from setuptools.command.sdist import sdist as orig_sdist
from setuptools.errors import BaseError

LINK_SYSTEM_LIBGPIOD = getenv("LINK_SYSTEM_LIBGPIOD") == "1"
LIBGPIOD_MINIMUM_VERSION = "2.1.0"
LIBGPIOD_VERSION = getenv("LIBGPIOD_VERSION")
GPIOD_WITH_TESTS = getenv("GPIOD_WITH_TESTS") == "1"
SRC_BASE_URL = "https://mirrors.edge.kernel.org/pub/software/libs/libgpiod/"
TAR_FILENAME = "libgpiod-{version}.tar.gz"
ASC_FILENAME = "sha256sums.asc"
SHA256_CHUNK_SIZE = 2048

# __version__
with open("gpiod/version.py", "r") as fd:
    exec(fd.read())


def sha256(filename):
    """
    Return a sha256sum for a specific filename, loading the file in chunks
    to avoid potentially excessive memory use.
    """
    from hashlib import sha256

    sha256sum = sha256()
    with open(filename, "rb") as f:
        for chunk in iter(lambda: f.read(SHA256_CHUNK_SIZE), b""):
            sha256sum.update(chunk)

    return sha256sum.hexdigest()


def find_sha256sum(asc_file, tar_filename):
    """
    Search through a local copy of sha256sums.asc for a specific filename
    and return the associated sha256 sum.
    """
    with open(asc_file, "r") as f:
        for line in f:
            line = line.strip().split("  ")
            if len(line) == 2 and line[1] == tar_filename:
                return line[0]

    raise BaseError(f"no signature found for {tar_filename}")


def fetch_tarball(command):
    """
    Verify the requested LIBGPIOD_VERSION tarball exists in sha256sums.asc,
    fetch it from https://mirrors.edge.kernel.org/pub/software/libs/libgpiod/
    and verify its sha256sum.

    If the check passes, extract the tarball and copy the lib and include
    dirs into our source tree.
    """

    # If no LIBGPIOD_VERSION is specified in env, just run the command
    if LIBGPIOD_VERSION is None:
        return command

    # If LIBGPIOD_VERSION is specified, apply the tarball wrapper
    def wrapper(self):
        # Just-in-time import of tarfile and urllib.request so these are
        # not required for Yocto to build a vendored or linked package
        import tarfile
        from tempfile import TemporaryDirectory
        from urllib.request import urlretrieve

        from packaging.version import Version

        # The "build" frontend will run setup.py twice within the same
        # temporary output directory. First for "sdist" and then for "wheel"
        # This would cause the build to fail with dirty "lib" and "include"
        # directories.
        # If the version in "libgpiod-version.txt" already matches our
        # requested tarball, then skip the fetch altogether.
        try:
            if open("libgpiod-version.txt", "r").read() == LIBGPIOD_VERSION:
                log.info(f"skipping tarball fetch")
                command(self)
                return
        except OSError:
            pass

        # Early exit for build tree with dirty lib/include dirs
        for check_dir in "lib", "include":
            if path.isdir(f"./{check_dir}"):
                raise BaseError(f"refusing to overwrite ./{check_dir}")

        with TemporaryDirectory(prefix="libgpiod-") as temp_dir:
            tarball_filename = TAR_FILENAME.format(version=LIBGPIOD_VERSION)
            tarball_url = f"{SRC_BASE_URL}{tarball_filename}"
            asc_url = f"{SRC_BASE_URL}{ASC_FILENAME}"

            log.info(f"fetching: {asc_url}")

            asc_filename, _ = urlretrieve(asc_url, path.join(temp_dir, ASC_FILENAME))

            tarball_sha256 = find_sha256sum(asc_filename, tarball_filename)

            if Version(LIBGPIOD_VERSION) < Version(LIBGPIOD_MINIMUM_VERSION):
                raise BaseError(f"requires libgpiod>={LIBGPIOD_MINIMUM_VERSION}")

            log.info(f"fetching: {tarball_url}")

            downloaded_tarball, _ = urlretrieve(
                tarball_url, path.join(temp_dir, tarball_filename)
            )

            log.info(f"verifying: {tarball_filename}")
            if sha256(downloaded_tarball) != tarball_sha256:
                raise BaseError(f"signature mismatch for {tarball_filename}")

            # Unpack the downloaded tarball
            log.info(f"unpacking: {tarball_filename}")
            with tarfile.open(downloaded_tarball) as f:
                f.extractall(temp_dir)

            # Copy the include and lib directories we need to build libgpiod
            base_dir = path.join(temp_dir, f"libgpiod-{LIBGPIOD_VERSION}")
            copytree(path.join(base_dir, "include"), "./include")
            copytree(path.join(base_dir, "lib"), "./lib")

        # Save the libgpiod version for sdist
        open("libgpiod-version.txt", "w").write(LIBGPIOD_VERSION)

        # Run the command
        command(self)

        # Clean up the build directory
        rmtree("./lib", ignore_errors=True)
        rmtree("./include", ignore_errors=True)
        unlink("libgpiod-version.txt")

    return wrapper


class build_ext(orig_build_ext):
    """
    Wrap build_ext to amend the module sources and settings to build
    the bindings and gpiod into a combined module when a version is
    specified and LINK_SYSTEM_LIBGPIOD=1 is not present in env.

    run is wrapped with @fetch_tarball in order to fetch the sources
    needed to build binary wheels when LIBGPIOD_VERSION is specified, eg:

    LIBGPIOD_VERSION="2.0.2" python3 -m build .
    """

    @fetch_tarball
    def run(self):
        # Try to get the gpiod version from the .txt file included in sdist
        try:
            libgpiod_version = open("libgpiod-version.txt", "r").read()
        except OSError:
            libgpiod_version = LIBGPIOD_VERSION

        if libgpiod_version and not LINK_SYSTEM_LIBGPIOD:
            # When building the extension from an sdist with a vendored
            # amend gpiod._ext sources and settings accordingly.
            gpiod_ext = self.ext_map["gpiod._ext"]
            gpiod_ext.sources += [
                "lib/chip.c",
                "lib/chip-info.c",
                "lib/edge-event.c",
                "lib/info-event.c",
                "lib/internal.c",
                "lib/line-config.c",
                "lib/line-info.c",
                "lib/line-request.c",
                "lib/line-settings.c",
                "lib/misc.c",
                "lib/request-config.c",
            ]
            gpiod_ext.libraries = []
            gpiod_ext.include_dirs = ["include", "lib", "gpiod/ext"]
            gpiod_ext.extra_compile_args.append(
                f'-DGPIOD_VERSION_STR="{libgpiod_version}"',
            )

        super().run()

        # We don't ever want the module tests directory in our package
        # since this might include gpiosim._ext or procname._ext from a
        # previous dirty build tree.
        rmtree(path.join(self.build_lib, "tests"), ignore_errors=True)


class sdist(orig_sdist):
    """
    Wrap sdist in order to fetch the libgpiod source files for vendoring
    into a source distribution.

    run is wrapped with @fetch_tarball in order to fetch the sources
    needed to build binary wheels when LIBGPIOD_VERSION is specified, eg:

    LIBGPIOD_VERSION="2.0.2" python3 -m build . --sdist
    """

    @fetch_tarball
    def run(self):
        super().run()


gpiod_ext = Extension(
    "gpiod._ext",
    sources=[
        "gpiod/ext/chip.c",
        "gpiod/ext/common.c",
        "gpiod/ext/line-config.c",
        "gpiod/ext/line-settings.c",
        "gpiod/ext/module.c",
        "gpiod/ext/request.c",
    ],
    define_macros=[("_GNU_SOURCE", "1")],
    libraries=["gpiod"],
    extra_compile_args=["-Wall", "-Wextra"],
)

gpiosim_ext = Extension(
    "tests.gpiosim._ext",
    sources=["tests/gpiosim/ext.c"],
    define_macros=[("_GNU_SOURCE", "1")],
    libraries=["gpiosim"],
    extra_compile_args=["-Wall", "-Wextra"],
)

procname_ext = Extension(
    "tests.procname._ext",
    sources=["tests/procname/ext.c"],
    define_macros=[("_GNU_SOURCE", "1")],
    extra_compile_args=["-Wall", "-Wextra"],
)

extensions = [gpiod_ext]
if GPIOD_WITH_TESTS:
    extensions.append(gpiosim_ext)
    extensions.append(procname_ext)

setup(
    name="gpiod",
    url="https://git.kernel.org/pub/scm/libs/libgpiod/libgpiod.git",
    packages=find_packages(exclude=["tests", "tests.*"]),
    python_requires=">=3.9.0",
    ext_modules=extensions,
    cmdclass={"build_ext": build_ext, "sdist": sdist},
    version=__version__,
    author="Bartosz Golaszewski",
    author_email="brgl@bgdev.pl",
    description="Python bindings for libgpiod",
    long_description=open("README.md", "r").read(),
    long_description_content_type="text/markdown",
    platforms=["linux"],
    license="LGPLv2.1",
)
