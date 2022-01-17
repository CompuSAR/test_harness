from typing import Callable


class DeferredActions:
    def __init__(self):
        self._current_cycle = 0
        self._actions: dict[ int, list[Callable] ] = {}

    def add_action(self, callback: Callable, when: int) -> None:
        assert when>=0

        self._actions.setdefault( self._current_cycle + when, [] ).append(callback)

    def advance(self) -> None:
        if self._current_cycle in self._actions:
            for action in self._actions[self._current_cycle]:
                action()

            del self._actions[self._current_cycle]

        self._current_cycle += 1
