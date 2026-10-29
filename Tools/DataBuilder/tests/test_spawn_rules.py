"""스폰 그룹과 스폰 항목 표의 규칙."""

from __future__ import annotations

import json
import unittest

from tests.helpers import DesignDataCase, sheet

MONSTER = (
    ["Id", "MinExp", "MaxExp", "MinGold", "MaxGold"],
    ["key:int32", "int64", "int64", "int64", "int64"],
    ["both", "server", "server", "server", "server"],
)
SPAWN_GROUP = (
    ["Id", "MaxCount", "RespawnSec", "Memo"],
    ["key:int32", "int32", "int32", "string"],
    ["server", "server", "server", "none"],
)
SPAWN_ENTRY = (
    ["GroupId", "MonsterId", "Weight"],
    ["key:parent:SpawnGroup", "key:ref:Monster", "int32"],
    ["server", "server", "server"],
)


class SpawnRuleCase(DesignDataCase):
    def setUp(self) -> None:
        super().setUp()
        self.add_table("Monster", sheet(*MONSTER, [5000, 1, 2, 1, 2], [5001, 1, 2, 1, 2]))

    def add_groups(self, *rows: list) -> None:
        self.add_table("SpawnGroup", sheet(*SPAWN_GROUP, *rows))

    def add_entries(self, *rows: list) -> None:
        self.add_table("SpawnEntry", sheet(*SPAWN_ENTRY, *rows), nest="Entries")


class SpawnGroupTest(SpawnRuleCase):
    def test_valid_group_passes(self):
        self.add_groups([1, 5, 10, "초급 사냥터"])
        self.add_entries([1, 5000, 1], [1, 5001, 2])
        self.build_ok()

    def test_entries_nest_under_their_group(self):
        self.add_groups([1, 5, 10, None], [2, 3, 0, None])
        self.add_entries([1, 5000, 1], [2, 5001, 1])
        groups = json.loads(self.output("SpawnGroup.json"))
        self.assertEqual([(g["Id"], [e["MonsterId"] for e in g["Entries"]]) for g in groups], [(1, [5000]), (2, [5001])])
        self.assertNotIn("Memo", groups[0])

    def test_zero_max_count_and_respawn_time_pass(self):
        self.add_groups([1, 0, 0, None])
        self.add_entries([1, 5000, 1])
        self.build_ok()

    def test_negative_max_count_is_rejected(self):
        self.add_groups([1, -1, 10, None])
        self.add_entries([1, 5000, 1])
        self.assert_error("SpawnGroup.xlsx", "5행", "MaxCount", "-1")

    def test_negative_respawn_time_is_rejected(self):
        self.add_groups([1, 5, -1, None])
        self.add_entries([1, 5000, 1])
        self.assert_error("SpawnGroup.xlsx", "5행", "RespawnSec", "-1")

    def test_group_without_entry_is_rejected(self):
        self.add_groups([1, 5, 10, None], [2, 5, 10, None])
        self.add_entries([1, 5000, 1])
        self.assert_error("SpawnGroup.xlsx", "6행", "그룹 2", "SpawnEntry")


class SpawnEntryTest(SpawnRuleCase):
    def setUp(self) -> None:
        super().setUp()
        self.add_groups([1, 5, 10, None])

    def test_unknown_monster_is_rejected(self):
        self.add_entries([1, 5000, 1], [1, 5999, 1])
        self.assert_error("SpawnEntry.xlsx", "6행", "MonsterId", "5999")

    def test_zero_weight_is_rejected(self):
        self.add_entries([1, 5000, 0])
        self.assert_error("SpawnEntry.xlsx", "5행", "Weight", "0")


if __name__ == "__main__":
    unittest.main()
