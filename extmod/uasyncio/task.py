# MicroPython uasyncio module
# MIT license; Copyright (c) 2019-2020 Damien P. George

# This file contains the core TaskQueue based on a pairing heap, and the core Task class.
# They can optionally be replaced by C implementations.

from . import core


# pairing-heap meld of 2 heaps; O(1)
def ph_meld(h1, h2):
    if h1 is None:
        return h2
    if h2 is None:
        return h1
    lt = core.ticks_diff(h1.ph_key, h2.ph_key) < 0
    if lt:
        if h1.ph_child is None:
            h1.ph_child = h2
        else:
            h1.ph_child_last.ph_next = h2
        h1.ph_child_last = h2
        h2.ph_next = None
        h2.ph_rightmost_parent = h1
        return h1
    else:
        h1.ph_next = h2.ph_child
        h2.ph_child = h1
        if h1.ph_next is None:
            h2.ph_child_last = h1
            h1.ph_rightmost_parent = h2
        return h2


# pairing-heap pairing operation; amortised O(log N)
def ph_pairing(child):
    heap = None
    while child is not None:
        n1 = child
        child = child.ph_next
        n1.ph_next = None
        if child is not None:
            n2 = child
            child = child.ph_next
            n2.ph_next = None
            n1 = ph_meld(n1, n2)
        heap = ph_meld(heap, n1)
    return heap


# pairing-heap delete of a node; stable, amortised O(log N)
def ph_delete(heap, node):
    if node is heap:
        child = heap.ph_child
        node.ph_child = None
        return ph_pairing(child)
    # Find parent of node
    parent = node
    while parent.ph_next is not None:
        parent = parent.ph_next
    parent = parent.ph_rightmost_parent
    # Replace node with pairing of its children
    if node is parent.ph_child and node.ph_child is None:
        parent.ph_child = node.ph_next
        node.ph_next = None
        return heap
    elif node is parent.ph_child:
        child = node.ph_child
        next = node.ph_next
        node.ph_child = None
        node.ph_next = None
        node = ph_pairing(child)
        parent.ph_child = node
    else:
        n = parent.ph_child
        while node is not n.ph_next:
            n = n.ph_next
        child = node.ph_child
        next = node.ph_next
        node.ph_child = None
        node.ph_next = None
        node = ph_pairing(child)
        if node is None:
            node = n
        else:
            n.ph_next = node
    node.ph_next = next
    if next is None:
        node.ph_rightmost_parent = parent
        parent.ph_child_last = node
    return heap


# TaskQueue class based on the above pairing-heap functions.
class TaskQueue:
    def __init__(self):
        self.heap = None

    def peek(self):
        return self.heap

    def push_sorted(self, v, key):
        v.data = None
        v.ph_key = key
        v.ph_child = None
        v.ph_next = None
        self.heap = ph_meld(v, self.heap)

    def push_head(self, v):
        self.push_sorted(v, core.ticks())

    def pop_head(self):
        v = self.heap
        self.heap = ph_pairing(self.heap.ph_child)
        return v

    def remove(self, v):
        self.heap = ph_delete(self.heap, v)


# Task class representing a coroutine, can be waited on and cancelled.
class Task:
    def __init__(self, coro, globals=None):
        self.coro = coro  # Coroutine of this Task
        self.data = None  # General data for queue it is waiting on
        self.ph_key = 0  # Pairing heap
        self.ph_child = None  # Paring heap
        self.ph_child_last = None  # Paring heap
        self.ph_next = None  # Paring heap
        self.ph_rightmost_parent = None  # Paring heap

    def __iter__(self):
        if self.coro is self:
            # Signal that the completed-task has been await'ed on.
            self.waiting = None
        elif not hasattr(self, "waiting"):
            # Lazily allocated head of linked list of Tasks waiting on completion of this task.
            self.waiting = TaskQueue()
        return self

    def __next__(self):
        if self.coro is self:
            # Task finished, raise return value to caller so it can continue.
            raise self.data
        else:
            # Put calling task on waiting queue.
            self.waiting.push_head(core.cur_task)
            # Set calling task's data to this task that it waits on, to double-link it.
            core.cur_task.data = self

    def done(self):
        return self.coro is self

    def cancel(self):
        # Check if task is already finished.
        if self.coro is self:
            return False
        # Can't cancel self (not supported yet).
        if self is core.cur_task:
            raise RuntimeError("can't cancel self")
        # If Task waits on another task then forward the cancel to the one it's waiting on.
        while isinstance(self.data, Task):
            self = self.data
        # Reschedule Task as a cancelled task.
        if hasattr(self.data, "remove"):
            # Not on the main running queue, remove the task from the queue it's on.
            self.data.remove(self)
            core._task_queue.push_head(self)
        elif core.ticks_diff(self.ph_key, core.ticks()) > 0:
            # On the main running queue but scheduled in the future, so bring it forward to now.
            core._task_queue.remove(self)
            core._task_queue.push_head(self)
        self.data = core.CancelledError
        return True

    def throw(self, value):
        # This task raised an exception which was uncaught; handle that now.
        # Set the data because it was cleared by the main scheduling loop.
        self.data = value
        if not hasattr(self, "waiting"):
            # Nothing await'ed on the task so call the exception handler.
            core._exc_context["exception"] = value
            core._exc_context["future"] = self
            core.Loop.call_exception_handler(core._exc_context)
