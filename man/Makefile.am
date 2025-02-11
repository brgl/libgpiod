# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>
# SPDX-FileCopyrightText: 2025 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

MAN_ENTRIES =
MAN_DEPS =

if WITH_TOOLS

TOOLS = \
	gpiodetect \
	gpioinfo \
	gpioget \
	gpioset \
	gpiomon \
	gpionotify

MAN_ENTRIES += $(TOOLS)
MAN_DEPS += $(foreach dep,$(TOOLS),$(top_builddir)/tools/$(dep))

endif

if WITH_DBUS

GPIOCLI_CMDS = \
	detect \
	find \
	info \
	get \
	monitor \
	notify \
	reconfigure \
	release \
	request \
	requests \
	set \
	wait

MAN_ENTRIES += \
	gpio-manager \
	gpiocli \
	$(foreach cmd,$(GPIOCLI_CMDS),gpiocli-$(cmd))

MAN_DEPS += \
	$(top_builddir)/dbus/manager/gpio-manager
	$(top_builddir)/dbus/client/gpiocli

endif

dist_man1_MANS := $(foreach entry,$(MAN_ENTRIES),$(entry).man)

%.man: $(MAN_DEPS)
	$(AM_V_GEN)export PATH=$(top_builddir)/dbus/manager/:$(top_builddir)/dbus/client/:$(top_builddir)/tools/:$$PATH; \
	if [ "$(*F)" = "gpio-manager" ]; then \
		EXEC=$(*F); \
		NAME="libgpiod D-Bus daemon"; \
		HELP=--help-option=--help; \
	else \
		EXEC=$(if $(findstring -,$(*F)),$(word 1,$(subst -, ,$(*F))),$(*F)); \
		NAME=$(if $(findstring -,$(*F)),$(word 2,$(subst -, ,$(*F))),"libgpiod command-line utility"); \
		HELP=$(if $(findstring -,$(*F)),--help-option="$(word 2,$(subst -, ,$(*F))) --help",--help-option=--help); \
	fi; \
	help2man $$EXEC \
		--include=$(srcdir)/template --output=$(builddir)/$@ --no-info \
		--name="$$NAME" "$$HELP" --manual=$(*F)

clean-local:
	rm -f $(dist_man1_MANS)

EXTRA_DIST = template
