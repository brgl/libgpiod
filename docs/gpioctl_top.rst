..
   SPDX-License-Identifier: CC-BY-SA-4.0
   SPDX-FileCopyrightText: 2026 Bartosz Golaszewski <bartosz.golaszewski@oss.qualcomm.com>

..
   This file is part of libgpiod.

   gpioctl command-line tool documentation

gpioctl - Persistent GPIO control tool
======================================

``gpioctl`` provides GPIO control with **persistence**: a per-user background
daemon holds requested lines even after the client process exits, unlike
``gpioget`` or ``gpioset`` which release their lines on exit.

The client auto-spawns the daemon on first use and communicates with it over
a per-user, abstract Unix-domain socket. The daemon stays alive as long as it
holds any active GPIO requests or client connections. When all requests are
released and client connections closed, the server starts an idle timer and
exists after 60 seconds of inactivity.

Examples
--------

Request a line as output and set it active:

.. code-block:: none

   $ gpioctl -c gpiochip0 request --output 5=active
   request0

The client prints the name of the new request and exits immediately; the daemon
holds the line. Confirm with
``gpioinfo``:

.. code-block:: none

   $ gpioinfo --chip gpiochip0 5
   gpiochip0 5  unnamed  output consumer="gpioctl"

List all requests currently held by the daemon:

.. code-block:: none

   $ gpioctl requests
   request0 (gpiochip0) consumer="gpioctl" offsets: [5]

List requests without quoting the consumer name:

.. code-block:: none

   $ gpioctl --unquoted requests
   request0 (gpiochip0) consumer=gpioctl offsets: [5]

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

Read the same lines without quoting the names:

.. code-block:: none

   $ gpioctl --unquoted get power_button power_led
   power_button=inactive power_led=active

Read a request's values as bare numbers instead of names:

.. code-block:: none

   $ gpioctl --numeric get request0
   1

Change the output value by request:

.. code-block:: none

   $ gpioctl set request0 inactive

Change an output line by its name:

.. code-block:: none

   $ gpioctl set power_led=active

Change multiple output lines by name in a single call:

.. code-block:: none

   $ gpioctl set power_led=active status_led=inactive

Reconfigure a held request - switch to input and enable edge detection:

.. code-block:: none

   $ gpioctl reconfigure request0 --both-edges

Reconfigure a request of several lines to output. Values must be given for all
of its lines, either in the order shown by ``gpioctl requests`` or as
``line=value`` pairs in any order, where ``line`` is the name or the offset of
a line of the request:

.. code-block:: none

   $ gpioctl reconfigure request1 --output active inactive active
   $ gpioctl reconfigure request1 --output 7=inactive power_led=active 3=active

Monitor edge events on a held edge-detection request:

.. code-block:: none

   $ gpioctl monitor request0
   1718000123.000456789    rising  gpiochip0 5
   1718000124.001234567    falling gpiochip0 5

Monitor a request, printing its current line value before the first event:

.. code-block:: none

   $ gpioctl monitor --initial-state request0
   active
   1718000123.000456789    rising  gpiochip0 5
   1718000124.001234567    falling gpiochip0 5

Monitor a single line of a held request by its name; only events for that line
are printed (the line name is quoted unless ``--unquoted`` is given):

.. code-block:: none

   $ gpioctl monitor power_button
   1718000125.002345678    falling gpiochip0 2 "power_button"

Monitor a request with a custom output format:

.. code-block:: none

   $ gpioctl monitor --format="%c %o %l %E %S" request0
   gpiochip0 5 unnamed rising 1718000123.000456789
   gpiochip0 5 unnamed falling 1718000124.001234567

Release a single request:

.. code-block:: none

   $ gpioctl release request0

Stop the daemon (releases all held lines):

.. code-block:: none

   $ gpioctl stop

Caveats
-------

By default the daemon listens on an abstract Unix-domain socket named after
the user id, ``gpioctl-<uid>``. Abstract socket names are not protected by
file permissions: any local user can bind a name before the daemon of the user
it belongs to does. The clients and the daemon check the credentials of their
peer and refuse to talk to a server of another user, so nothing is
disclosed, but the squatted name keeps the victim's own daemon from starting
and ``gpioctl`` then fails for them.

As a workaround use a socket in a private directory, for example one under
``$XDG_RUNTIME_DIR``, by passing ``--socket-path`` to every invocation:

.. code-block:: none

   $ gpioctl --socket-path "$XDG_RUNTIME_DIR/gpioctl.sock" request --output 5=active

A socket created this way is a regular file and is protected by the
permissions of its directory. A lock file with the same name and a ``.lock``
suffix is created next to it to serialize starting the daemon. It is empty and
safe to remove when the daemon is not running.

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
