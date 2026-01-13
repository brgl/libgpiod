..
   SPDX-License-Identifier: CC-BY-SA-4.0
   SPDX-FileCopyrightText: 2026 Bartosz Golaszewski <bartosz.golaszewski@oss.qualcomm.com>

..
   This file is part of libgpiod.

   gpioctl command-line tool documentation

Persistent GPIO control tool
=============================

``gpioctl`` provides GPIO control with **persistence**: a per-user background
daemon holds requested lines even after the client process exits, unlike
``gpioget`` or ``gpioset`` which release their lines on exit.

The client auto-spawns the daemon on first use and communicates with it over
a per-user abstract Unix-domain socket. The daemon exits automatically after
60 seconds of inactivity.

Examples
--------

Request a line as output and set it active:

.. code-block:: none

   $ gpioctl -c gpiochip0 request --output 5=active

The client exits immediately; the daemon holds the line. Confirm with
``gpioinfo``:

.. code-block:: none

   $ gpioinfo --chip gpiochip0 5
   gpiochip0 5  unnamed  output consumer="gpioctl"

List all requests currently held by the daemon:

.. code-block:: none

   $ gpioctl requests
   request0 (gpiochip0) consumer="gpioctl" offsets: [5]

Read the current value of a held request:

.. code-block:: none

   $ gpioctl get request0
   active

Read a line value by its name (daemon finds the request holding that line):

.. code-block:: none

   $ gpioctl get power_button
   "power_button"=inactive

Read multiple lines by name in a single call:

.. code-block:: none

   $ gpioctl get power_button power_led
   "power_button"=inactive "power_led"=active

Change the output value by request:

.. code-block:: none

   $ gpioctl set request0 inactive

Change an output line by its name:

.. code-block:: none

   $ gpioctl set power_led=active

Change multiple output lines by name in a single call:

.. code-block:: none

   $ gpioctl set power_led=active status_led=inactive

Reconfigure a held request — switch to input and enable edge detection:

.. code-block:: none

   $ gpioctl reconfigure request0 --both-edges

Monitor edge events on a held edge-detection request:

.. code-block:: none

   $ gpioctl monitor request0
   1718000123.000456789 rising 5
   1718000124.001234567 falling 5

Release a single request:

.. code-block:: none

   $ gpioctl release request0

Stop the daemon (releases all held lines):

.. code-block:: none

   $ gpioctl stop

.. note::
   For more information please refer to the output of ``gpioctl --help`` as
   well as ``gpioctl <command> --help`` which prints detailed info on every
   available command.

.. toctree::
   :maxdepth: 1
   :caption: Manual entries

   gpioctl<gpioctl>
   gpioctl-request<gpioctl-request>
   gpioctl-release<gpioctl-release>
   gpioctl-requests<gpioctl-requests>
   gpioctl-get<gpioctl-get>
   gpioctl-set<gpioctl-set>
   gpioctl-monitor<gpioctl-monitor>
   gpioctl-reconfigure<gpioctl-reconfigure>
