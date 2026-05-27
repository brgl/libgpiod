# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2026 Vincent Fazio <vfazio@gmail.com>

import errno
import fcntl
import os
import threading
from concurrent.futures import ThreadPoolExecutor, as_completed
from contextlib import nullcontext
from select import EPOLLIN, epoll
from typing import TYPE_CHECKING, ClassVar
from unittest import TestCase

import gpiod
from gpiod.line import Direction, Edge, Value

from . import gpiosim
from .helpers import is_free_threaded

if TYPE_CHECKING:
    from contextlib import AbstractContextManager


# Threading & the CPython bindings as they relate to the C extension:
#
# Python is sometimes mistakenly considered thread-safe but this is not the
# case even with GIL enabled builds as there can still be data races between
# threads on pure Python objects.
#
# What is guaranteed is ref counts, memory management, etc being handled safely.
# Mutations on objects like dicts/lists are _not_ guaranteed to be safe.
#
# Of the objects exposed by the bindings, the following are effectively "frozen":
#   * ChipInfo
#   * LineInfo
#   * InfoEvent
#   * EdgeEvent
#   * gpiod.line Enums
#
# The *Info and *Event objects are return values from the C extension, are not
# inputs, and are immutable. There should be no thread-safety concerns for them.
#
# The remaining objects are:
#   * Chip
#   * LineRequest
#   * LineSettings
#
# LineSettings is a pure Python class, is an argument to functions, and is not
# passed to the C extension directly. There should be no major concerns about
# the thread-safety of this object within the C extension.
#
# Chip and LineRequest objects are pure Python classes _but_ wrap classes that
# are exposed by the C extension. Example: LineRequest wraps Request which wraps
# request_object from the C extension that has buffers allocated at creation.
# Calling get_values on the Python class will fill the buffer for the underlying
# C object which could race with another thread writing/reading at the same time.
#
# As such, these classes are at risk for conflicts between threads.
#
# For GIL enabled CPython builds, calling into the extension maintains the GIL
# until a call such as Py_BEGIN_ALLOW_THREADS releases it. Until that call, the
# GIL provides implicit safety for the aforementioned buffers.
#
# For no-GIL builds, the GIL is no longer in place to provide that safety.
# Without the GIL acting as a mutex, either the C extension or the caller are
# responsible for providing thread safety.
#
# The libgpiod C API itself is not advertised as being thread-safe and the
# bindings do not add any explicit thread-safety mechanisms (there is no internal
# synchronization). Users of the bindings must provide external synchronization
# if sharing Chip or LineRequest objects across threads.


def get_lock() -> "AbstractContextManager[None | bool]":
    """
    Helper function to return a lock or a nullcontext so that no lock is used.
    Can be used for a quick sanity check that things are not thread-safe.
    """

    lock: AbstractContextManager[None | bool]
    if os.getenv("TESTS_NO_LOCKING"):
        lock = nullcontext()
    else:
        lock = threading.Lock()
    return lock


# It should be noted that the values used for the tests below are not "smart"...
# They do not auto-balance so any tweaks may require significant rework. For
# example, there are generally 4 lines used for testing which matches the number
# of threads spun up, with the thread identifier acting as an index to the line
# it controls/queries.


class ThreadedTestCase(TestCase):
    NUM_THREADS: ClassVar[int]
    ITERATIONS: ClassVar[int]
    TIMEOUT: ClassVar[int]

    # This is stubbed out to prevent docstrings from being used as descriptions
    # when unit tests are run in verbose mode.
    # See: https://docs.python.org/3/library/unittest.html#unittest.TestCase.shortDescription
    def shortDescription(self) -> None:
        return None

    @classmethod
    def setUpClass(cls) -> None:
        cls.NUM_THREADS = 4
        # we want to stress test free threaded builds a bit more
        cls.ITERATIONS = 200 if is_free_threaded() else 20
        cls.TIMEOUT = 2


class Chip(ThreadedTestCase):
    def setUp(self) -> None:
        self.sim = gpiosim.Chip(
            num_lines=4, label="foobar", line_names={0: "l0", 1: "l1", 2: "l2", 3: "l3"}
        )
        self.chip = gpiod.Chip(self.sim.dev_path)

    def tearDown(self) -> None:
        self.chip.close()
        self.chip = None  # type: ignore[assignment]
        self.sim = None  # type: ignore[assignment]

    def test_per_thread_creation_and_query(self) -> None:
        """
        Test that multiple threads can create and query a chip pointing to the
        same backing device without a mutex

        Synchronization: Not required
        """

        barrier = threading.Barrier(self.NUM_THREADS, timeout=self.TIMEOUT)

        def worker(tid: int) -> None:
            barrier.wait()
            for _ in range(self.ITERATIONS):
                offset = tid % self.NUM_THREADS
                with gpiod.Chip(self.sim.dev_path) as chip:
                    info = chip.get_info()
                    self.assertEqual(
                        (info.name, info.label, info.num_lines),
                        (
                            self.sim.name,
                            "foobar",
                            4,
                        ),
                    )
                    line_info = chip.get_line_info(f"l{offset}")
                    self.assertEqual(
                        (line_info.offset, line_info.name), (offset, f"l{offset}")
                    )

        with ThreadPoolExecutor(max_workers=self.NUM_THREADS) as executor:
            futures = [executor.submit(worker, i) for i in range(self.NUM_THREADS)]
            for future in as_completed(futures, timeout=self.TIMEOUT):
                future.result(timeout=self.TIMEOUT)

    def test_shared_creation_and_query(self) -> None:
        """
        Test querying a single chip shared across multiple threads

        Synchronization: Not required
        """

        barrier = threading.Barrier(self.NUM_THREADS, timeout=self.TIMEOUT)
        lock = get_lock()

        def worker(tid: int) -> None:
            barrier.wait()
            for _ in range(self.ITERATIONS):
                offset = tid % self.NUM_THREADS
                with lock:
                    info = self.chip.get_info()
                self.assertEqual(
                    (info.name, info.label, info.num_lines),
                    (self.sim.name, "foobar", 4),
                )
                with lock:
                    line_info = self.chip.get_line_info(f"l{offset}")
                self.assertEqual(
                    (line_info.offset, line_info.name), (offset, f"l{offset}")
                )

        with ThreadPoolExecutor(max_workers=self.NUM_THREADS) as executor:
            futures = [executor.submit(worker, i) for i in range(self.NUM_THREADS)]
            for future in as_completed(futures, timeout=self.TIMEOUT):
                future.result(timeout=self.TIMEOUT)

    def test_shared_closed(self) -> None:
        """
        Tests that querying a single `Chip` shared across multiple threads after
        closing raises an error

        Synchronization: Required

        Note:
        The underlying `gpiod_chip` struct gets freed on close, leaving a mine
        for other threads to step on
        """

        barrier = threading.Barrier(self.NUM_THREADS, timeout=self.TIMEOUT)
        lock = get_lock()

        def worker() -> None:
            barrier.wait()
            with lock:
                info = self.chip.get_info()
                self.chip.close()
            self.assertEqual(
                (info.name, info.label, info.num_lines),
                (self.sim.name, "foobar", 4),
            )

        with ThreadPoolExecutor(max_workers=self.NUM_THREADS) as executor:
            futures = [executor.submit(worker) for _ in range(self.NUM_THREADS)]
            error_count = 0
            for future in as_completed(futures, timeout=self.TIMEOUT):
                try:
                    future.result(timeout=self.TIMEOUT)
                except gpiod.ChipClosedError:
                    error_count += 1
            self.assertEqual(error_count, self.NUM_THREADS - 1)


class InfoEvent(ThreadedTestCase):
    def setUp(self) -> None:
        self.sim = gpiosim.Chip(num_lines=4, label="foobar")
        self.chip = gpiod.Chip(self.sim.dev_path)

    def tearDown(self) -> None:
        self.chip.close()
        self.chip = None  # type: ignore[assignment]
        self.sim = None  # type: ignore[assignment]

    def test_watch_unwatch_line_info(self) -> None:
        """
        Tests that threads that share a `Chip` can watch/unwatch line info events

        Synchronization: Not required

        Note:
        Threads may encounter EBUSY if the underlying file descriptor is busy or
        if the offset is already being watched
        """

        barrier = threading.Barrier(self.NUM_THREADS, timeout=self.TIMEOUT)
        num_lines = self.chip.get_info().num_lines

        def worker(tid: int) -> None:
            offset = tid % num_lines
            barrier.wait()
            for _ in range(self.ITERATIONS):
                try:
                    info = self.chip.watch_line_info(offset)
                    self.assertEqual(info.offset, offset)
                except OSError as e:
                    if e.errno == errno.EBUSY:
                        retry_count = 0
                        while retry_count < 2:
                            try:
                                retry_count += 1
                                self.chip.unwatch_line_info(offset)
                                break
                            except OSError as e:
                                pass

                info = self.chip.get_line_info(offset)
                self.assertEqual(info.offset, offset)

        with ThreadPoolExecutor(max_workers=self.NUM_THREADS) as executor:
            futures = [executor.submit(worker, _) for _ in range(self.NUM_THREADS)]
            for future in as_completed(futures, timeout=self.TIMEOUT):
                future.result(timeout=self.TIMEOUT)

    def test_watch_unwatch_line_info_locks(self) -> None:
        """
        Tests that threads that share a `Chip` can watch/unwatch line info events
        with locking

        Same as test_watch_unwatch_line_info but with locks and no EBUSY handling

        Synchronization: Not required
        """

        barrier = threading.Barrier(self.NUM_THREADS, timeout=self.TIMEOUT)
        lock = get_lock()
        watching: set[int] = set()

        def worker(tid: int) -> None:
            barrier.wait()
            for _ in range(self.ITERATIONS):
                offset = tid % self.NUM_THREADS
                with lock:
                    if offset in watching:
                        self.chip.unwatch_line_info(offset)
                        watching.remove(offset)
                        info = self.chip.get_line_info(offset)
                    else:
                        info = self.chip.watch_line_info(offset)
                        watching.add(offset)
                self.assertEqual(info.offset, offset)

        with ThreadPoolExecutor(max_workers=self.NUM_THREADS) as executor:
            futures = [executor.submit(worker, i) for i in range(self.NUM_THREADS)]
            for future in as_completed(futures, timeout=self.TIMEOUT):
                future.result(timeout=self.TIMEOUT)

    def test_read_info_event(self) -> None:
        """
        Test that multiple threads that share a Chip can read info events

        Synchronization: Not required
        """

        num_lines = self.chip.get_info().num_lines
        for offset in range(num_lines):
            self.chip.watch_line_info(offset)
        # If read_edge_events() is blocking, threads will hang forever waiting
        # for events that don't exist when we're looking to shutdown.
        flags = fcntl.fcntl(self.chip.fd, fcntl.F_GETFL)
        fcntl.fcntl(self.chip.fd, fcntl.F_SETFL, flags | os.O_NONBLOCK)

        worker_barrier = threading.Barrier(self.NUM_THREADS, timeout=self.TIMEOUT)
        feeder_barrier = threading.Barrier(2, timeout=self.TIMEOUT)
        done_fd = os.eventfd(0)

        total = 0
        counter_lock = threading.Lock()

        poll = epoll()
        poll.register(self.chip.fd, EPOLLIN)
        poll.register(done_fd, EPOLLIN)

        def reader_worker(tid: int) -> None:
            should_exit = False
            local_count = 0
            nonlocal total

            worker_barrier.wait()
            while not should_exit:
                events = poll.poll(timeout=self.TIMEOUT)

                for fd, _ in events:
                    if fd == done_fd:
                        should_exit = True
                        continue
                    if fd == self.chip.fd:
                        # read_info_event() only reads ONE event at a time (unlike edge events).
                        # We must loop until EAGAIN to fully drain the kernel buffer.
                        try:
                            while True:
                                _event = self.chip.read_info_event()
                                self.assertIsNotNone(_event)
                                local_count += 1
                        except OSError as e:
                            if e.errno == errno.EAGAIN:
                                continue
                            raise

            with counter_lock:
                total += local_count

        def feeder(tid: int) -> None:
            offsets = list(range(tid, num_lines, 2))
            worker_barrier.wait()

            for i in range(int(self.ITERATIONS)):
                offset = offsets[i % len(offsets)]
                with self.chip.request_lines(
                    config={offset: gpiod.LineSettings(direction=Direction.INPUT)}
                ) as req:
                    req.reconfigure_lines(
                        config={offset: gpiod.LineSettings(direction=Direction.OUTPUT)}
                    )

            feeder_barrier.wait()
            # Thread 0 signals done when all events have fired
            if tid == 0:
                os.eventfd_write(done_fd, 1)

        with ThreadPoolExecutor(max_workers=self.NUM_THREADS) as ex:
            futures = [ex.submit(feeder, i) for i in range(2)]
            futures += [ex.submit(reader_worker, i) for i in range(2, self.NUM_THREADS)]

            try:
                for f in as_completed(futures, timeout=self.TIMEOUT):
                    f.result(timeout=self.TIMEOUT)
                self.assertGreater(total, 0)
            finally:
                for fd in [self.chip.fd, done_fd]:
                    poll.unregister(fd)
                poll.close()
                os.close(done_fd)
                for offset in range(num_lines):
                    self.chip.unwatch_line_info(offset)


class LineRequest(ThreadedTestCase):
    def setUp(self) -> None:
        self.sim = gpiosim.Chip(
            num_lines=4, label="foobar", line_names={0: "l0", 1: "l1", 2: "l2", 3: "l3"}
        )
        self.chip = gpiod.Chip(self.sim.dev_path)

    def tearDown(self) -> None:
        self.chip.close()
        self.chip = None  # type: ignore[assignment]
        self.sim = None  # type: ignore[assignment]

    def test_per_thread_creation_and_query(self) -> None:
        """
        Test that multiple threads can create and query their own LineRequest
        without a mutex

        Synchronization: Not required

        Note: without a lock, EPERM may get raised due to the direction of the
        offset having been changed from output to input
        """

        barrier = threading.Barrier(self.NUM_THREADS, timeout=self.TIMEOUT)
        lock = get_lock()

        def worker(tid: int) -> None:
            # distribute threads across number of lines
            offset = 2 + (tid % 2)
            with lock:
                request = self.chip.request_lines(
                    config={offset: gpiod.LineSettings(direction=Direction.OUTPUT)}
                )
            counter = 0
            barrier.wait()
            for _ in range(self.ITERATIONS):
                try:
                    with lock:
                        direction = self.chip.get_line_info(offset).direction
                    if direction == Direction.INPUT:
                        continue
                    if request.get_value(offset) == Value.ACTIVE:
                        request.set_value(offset, Value.INACTIVE)
                        self.assertEqual(request.get_value(offset), Value.INACTIVE)
                        counter += 1
                    else:
                        request.set_value(offset, Value.ACTIVE)
                        self.assertEqual(request.get_value(offset), Value.ACTIVE)
                        counter += 1
                # set_value may raise a permission error when the pin is INPUT
                except OSError:
                    pass
            self.assertGreater(counter, 0)

        def feeder(tid: int) -> None:
            offset = tid % 2
            with lock:
                request = self.chip.request_lines(
                    config={offset: gpiod.LineSettings(direction=Direction.OUTPUT)}
                )
            barrier.wait()
            for iteration in range(self.ITERATIONS):
                new_dir = Direction.INPUT if iteration % 2 == 0 else Direction.OUTPUT
                with lock:
                    request.reconfigure_lines(
                        config={offset: gpiod.LineSettings(direction=new_dir)}
                    )

        with ThreadPoolExecutor(max_workers=self.NUM_THREADS) as executor:
            futures = [executor.submit(feeder, i) for i in range(2)]
            futures += [executor.submit(worker, i) for i in range(2, self.NUM_THREADS)]
            for future in as_completed(futures, timeout=self.TIMEOUT):
                future.result(timeout=self.TIMEOUT)

    def test_shared_creation_and_query(self) -> None:
        """
        Test multiple threads can reconfigure, set values and get values on a
        shared line request

        Synchronization: Required

        Note:
        This won't actually blow up, but based on the extension implementation
        the request has a shared buffer for offets/values that are reused for
        getting/setting line values

        Without synchronization, a thread may think it's setting one set of values
        but the buffer values may have been overwritten by another thread

        Implementation Note:
        We use a dual set of events to make sure the feeder/worker pair alternate
        otherwise a thread may monopolize the lock and finish before triggering
        a set_value call. We pair this with a lock to prevent issues with the
        aforementioned buffer contention.
        """

        barrier = threading.Barrier(self.NUM_THREADS, timeout=self.TIMEOUT)
        lock = get_lock()
        num_lines = self.chip.get_info().num_lines
        request = self.chip.request_lines(
            config={range(num_lines): gpiod.LineSettings(direction=Direction.OUTPUT)}
        )

        ready_events = {0: threading.Event(), 1: threading.Event()}
        set_events = {0: threading.Event(), 1: threading.Event()}

        def worker(tid: int) -> None:
            # we're using 2 feeder threads, each with a dedicated offset
            offset = tid % 2
            counter = 0
            ready_event = ready_events[offset]
            set_event = set_events[offset]
            set_event.set()
            barrier.wait()
            for _ in range(self.ITERATIONS):
                ready_event.wait(self.TIMEOUT)
                ready_event.clear()
                with lock:
                    if self.chip.get_line_info(offset).direction == Direction.OUTPUT:
                        if request.get_value(offset) == Value.ACTIVE:
                            request.set_value(offset, Value.INACTIVE)
                            self.assertEqual(request.get_value(offset), Value.INACTIVE)
                            counter += 1
                        else:
                            request.set_value(offset, Value.ACTIVE)
                            self.assertEqual(request.get_value(offset), Value.ACTIVE)
                            counter += 1
                set_event.set()
            self.assertGreater(counter, 0)

        def feeder(tid: int) -> None:
            offset = tid % 2
            ready_event = ready_events[offset]
            set_event = set_events[offset]
            barrier.wait()
            for iteration in range(self.ITERATIONS):
                new_dir = Direction.INPUT if iteration % 2 == 0 else Direction.OUTPUT
                set_event.wait(self.TIMEOUT)
                set_event.clear()
                with lock:
                    request.reconfigure_lines(
                        config={offset: gpiod.LineSettings(direction=new_dir)}
                    )
                ready_event.set()

        with ThreadPoolExecutor(max_workers=self.NUM_THREADS) as executor:
            futures = [executor.submit(feeder, i) for i in range(2)]
            futures += [executor.submit(worker, i) for i in range(2, self.NUM_THREADS)]
            try:
                for future in as_completed(futures, timeout=self.TIMEOUT):
                    future.result(timeout=self.TIMEOUT)
            finally:
                request.release()

    def test_shared_set_get_values(self) -> None:
        """
        Test setting and getting values from a single line request shared across
        multiple threads

        Synchronization: Required

        Note:
        This won't actually blow up, but based on the extension implementation
        the request has a shared buffer for offets/values that are reused for
        getting/setting line values

        Without synchronization, a thread may think it's setting one set of values
        but the buffer values may have been overwritten by another thread
        """

        barrier = threading.Barrier(self.NUM_THREADS, timeout=self.TIMEOUT)
        lock = get_lock()
        offset = 0
        request = self.chip.request_lines(
            config={0: gpiod.LineSettings(direction=Direction.OUTPUT)}
        )

        def worker() -> None:
            counter = 0
            barrier.wait()
            for _ in range(self.ITERATIONS):
                with lock:
                    if request.get_value(offset) == Value.ACTIVE:
                        request.set_value(offset, Value.INACTIVE)
                        self.assertEqual(request.get_value(offset), Value.INACTIVE)
                        counter += 1
                    else:
                        request.set_value(offset, Value.ACTIVE)
                        self.assertEqual(request.get_value(offset), Value.ACTIVE)
                        counter += 1
            self.assertGreater(counter, 0)

        with ThreadPoolExecutor(max_workers=self.NUM_THREADS) as executor:
            futures = [executor.submit(worker) for _ in range(self.NUM_THREADS)]
            try:
                for future in as_completed(futures, timeout=self.TIMEOUT):
                    future.result(timeout=self.TIMEOUT)
            finally:
                request.release()

    def test_shared_close(self) -> None:
        """
        Test that querying a single line request shared across multiple threads
        after releasing raises an error

        Synchronization: Required

        Note:
        The underlying `gpiod_line_request` struct gets freed on release, leaving
        a mine for other threads to step on
        """
        barrier = threading.Barrier(self.NUM_THREADS, timeout=self.TIMEOUT)
        lock = get_lock()

        num_lines = self.chip.get_info().num_lines
        request = self.chip.request_lines(
            config={
                range(num_lines): gpiod.LineSettings(
                    direction=Direction.OUTPUT, output_value=Value.INACTIVE
                )
            }
        )

        def worker() -> None:
            barrier.wait()
            with lock:
                info = request.get_values(range(num_lines))
                request.release()
            for line in info:
                self.assertEqual(line, Value.INACTIVE)

        with ThreadPoolExecutor(max_workers=self.NUM_THREADS) as executor:
            futures = [executor.submit(worker) for _ in range(self.NUM_THREADS)]
            error_count = 0
            for future in as_completed(futures, timeout=self.TIMEOUT):
                try:
                    future.result(timeout=self.TIMEOUT)
                except gpiod.RequestReleasedError:
                    error_count += 1
            self.assertEqual(error_count, self.NUM_THREADS - 1)


class EdgeEvent(ThreadedTestCase):
    def setUp(self) -> None:
        self.sim = gpiosim.Chip(num_lines=4, label="foobar")
        self.chip = gpiod.Chip(self.sim.dev_path)

    def tearDown(self) -> None:
        self.chip.close()
        self.sim = None  # type: ignore[assignment]
        self.chip = None  # type: ignore[assignment]

    def test_read_edge_events(self) -> None:
        """
        Test that multiple threads can read edge events on a shared LineRequest

        Synchronization: Required

        Note:
        The request object has a gpiod_edge_event_buffer for events to be read into.
        Without synchronization, that buffer will be overwritten by another thread
        when attempting to create event objects
        """
        num_lines = self.chip.get_info().num_lines
        req = self.chip.request_lines(
            config={
                range(num_lines): gpiod.LineSettings(
                    direction=Direction.INPUT, edge_detection=Edge.BOTH
                )
            }
        )

        # If read_edge_events() is blocking, threads will hang forever waiting
        # for events that don't exist during shutdown.
        flags = fcntl.fcntl(req.fd, fcntl.F_GETFL)
        fcntl.fcntl(req.fd, fcntl.F_SETFL, flags | os.O_NONBLOCK)

        worker_barrier = threading.Barrier(self.NUM_THREADS, timeout=self.TIMEOUT)
        feeder_barrier = threading.Barrier(2, timeout=self.TIMEOUT)
        done_fd = os.eventfd(0)

        total = 0
        counter_lock = threading.Lock()
        req_lock = get_lock()

        poll = epoll()
        poll.register(req.fd, EPOLLIN)
        poll.register(done_fd, EPOLLIN)

        def reader_worker(tid: int) -> None:
            nonlocal total
            should_exit = False
            local_count = 0
            worker_barrier.wait()

            while not should_exit:
                events = poll.poll(self.TIMEOUT)

                for fd, _ in events:
                    if fd == done_fd:
                        should_exit = True
                        continue

                    if fd == req.fd:
                        try:
                            with req_lock:
                                # O_NONBLOCK prevents hanging
                                evs = req.read_edge_events()
                            if evs:
                                local_count += len(evs)
                        except OSError as e:
                            if e.errno == errno.EAGAIN:
                                continue
                            raise

            with counter_lock:
                total += local_count

        def feeder(tid: int) -> None:
            offsets = list(range(tid, num_lines, 2))
            worker_barrier.wait()

            for i in range(int(self.ITERATIONS)):
                offset = offsets[i % len(offsets)]
                for pull in [gpiosim.Chip.Pull.UP, gpiosim.Chip.Pull.DOWN]:
                    self.sim.set_pull(offset, pull)

            feeder_barrier.wait()
            # Thread 0 signals done when all pulses have fired
            if tid == 0:
                os.eventfd_write(done_fd, 1)

        with ThreadPoolExecutor(max_workers=self.NUM_THREADS) as ex:
            futures = [ex.submit(feeder, i) for i in range(2)]
            futures += [ex.submit(reader_worker, i) for i in range(2, self.NUM_THREADS)]

            try:
                for f in as_completed(futures, timeout=self.TIMEOUT):
                    f.result(timeout=self.TIMEOUT)
                self.assertGreater(total, 0)
            finally:
                for fd in [req.fd, done_fd]:
                    poll.unregister(fd)
                poll.close()
                os.close(done_fd)
                req.release()
