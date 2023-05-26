# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2022 Linaro Ltd.
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

# We do not want to build against the system libs when building with make. So we
# specify the paths to the build directory of the C lib.
command = SYSTEM_DEPS_LIBGPIOD_NO_PKG_CONFIG=1 \
		SYSTEM_DEPS_LIBGPIOD_SEARCH_NATIVE="${PWD}/../../../lib/.libs/" \
		SYSTEM_DEPS_LIBGPIOD_LIB=gpiod \
		SYSTEM_DEPS_LIBGPIOD_INCLUDE="${PWD}/../../../include/"  \
		cargo build --release --lib

if WITH_TESTS
command += --tests
endif

if WITH_EXAMPLES
command += --examples
endif

all:
	$(command)

clean:
	cargo clean

EXTRA_DIST = Cargo.toml
SUBDIRS = examples src tests
