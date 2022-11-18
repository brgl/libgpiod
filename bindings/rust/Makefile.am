# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2022 Linaro Ltd.
# SPDX-FileCopyrightTest: 2022 Viresh Kumar <viresh.kumar@linaro.org>

command = cargo build --release --lib

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
