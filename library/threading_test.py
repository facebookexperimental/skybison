#!/usr/bin/env python3
import unittest
from threading import Condition, Event

from test_support import pyro_only


class ConditionTests(unittest.TestCase):
    def test_dunder_init_creates_lock(self):
        c = Condition()
        self.assertIsNotNone(c._lock)

    @pyro_only
    def test_enter_exit_functionality(self):
        c = Condition()
        lock_count = c._lock._count
        with c:
            self.assertEqual(lock_count + 1, c._lock._count)
        self.assertEqual(lock_count, c._lock._count)


class EventTests(unittest.TestCase):
    def test_dunder_init_clears_flag(self):
        e = Event()
        self.assertFalse(e.is_set())

    def test_clear_clears_flag(self):
        e = Event()
        e.set()
        self.assertTrue(e.is_set())
        e.clear()
        self.assertFalse(e.is_set())

    def test_set_sets_flag(self):
        e = Event()
        self.assertFalse(e.is_set())
        e.set()
        self.assertTrue(e.is_set())

    def test_wait_on_set_flag_returns(self):
        e = Event()
        e.set()
        e.wait()
        val = e.wait(5)
        self.assertTrue(val)


if __name__ == "__main__":
    unittest.main()
