..
   SPDX-License-Identifier: CC-BY-SA-4.0
   SPDX-FileCopyrightText: 2025 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

..
   This file is part of libgpiod.

   GPIO D-Bus command-line client documentation

Command-line client
===================

With the manager running the user can run ``gpiocli`` to control GPIOs by
asking the ``gpio-manager`` to act on their behalf.

Examples
--------

Detect chips in the system:

.. code-block:: none

   $ gpiocli detect
   gpiochip0 [INT34C6:00] (463 lines)

Request a set of lines:

.. note::
   **gpiocli** exits immediately but the state of the lines is retained
   because it's the **gpio-manager** that requested them.

.. code-block:: none

   $ gpiocli request --output foo=active
   request0

Previous invocation printed out the name of the request by which the caller
can refer to it later. All active requests can also be inspected at any time:

.. code-block:: none

   $ gpiocli requests
   request0 (gpiochip1) Offsets: [5]


Print the information about the requested line using the information above:

.. code-block:: none

   $ gpiocli info --chip=gpiochip1 5
   gpiochip1   5:      "foo"           [used,consumer="gpiocli request",managed="request0",output,push-pull]

Change the value of the line:

.. code-block:: none

   $ gpiocli set foo=inactive

Read it back:

.. code-block:: none

   $ gpiocli get foo
   "foo"=inactive

We can even reconfigure it to input and enable edge-detection:

.. code-block:: none

   $ gpiocli reconfigure --input --both-edges request0

And wait for edge events:

.. code-block:: none

   $ gpiocli monitor cos
   21763952894920 rising  "foo"

And finally release the request:

.. code-block:: none

   $ gpiocli release request0

.. note::
   For more information please refer to the output of ``gpiocli --help`` as
   well as ``gpiocli <command> --help`` which prints detailed info on every
   available command.

.. note::
   Users can talk to **gpio-manager** using any **D-Bus** library available
   and are not limited to the provided client.

.. toctree::
   :maxdepth: 1
   :caption: Manual entries

   gpiocli<gpiocli>
   gpiocli-detect<gpiocli-detect>
   gpiocli-find<gpiocli-find>
   gpiocli-info<gpiocli-info>
   gpiocli-get<gpiocli-get>
   gpiocli-monitor<gpiocli-monitor>
   gpiocli-notify<gpiocli-notify>
   gpiocli-reconfigure<gpiocli-reconfigure>
   gpiocli-release<gpiocli-release>
   gpiocli-request<gpiocli-request>
   gpiocli-requests<gpiocli-requests>
   gpiocli-set<gpiocli-set>
   gpiocli-wait<gpiocli-wait>
