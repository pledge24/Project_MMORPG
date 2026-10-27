"""몬스터 표의 기본 보상과 드롭 그룹, 드롭 항목 표의 규칙."""

from __future__ import annotations

import unittest

from tests.helpers import DesignDataCase, sheet

ITEM = (["Id", "Type", "MaxStack"], ["key:int32", "enum:ItemType", "int32"], ["both", "both", "both"])
MONSTER = (
    ["Id", "MinExp", "MaxExp", "MinGold", "MaxGold", "DropGroups"],
    ["key:int32", "int64", "int64", "int64", "int64", "ref[]:DropGroup"],
    ["both", "server", "server", "server", "server", "server"],
)
DROP_GROUP = (
    ["Id", "Method", "Rate", "PickCount", "Memo"],
    ["key:int32", "enum:DropMethod", "float", "int32?", "string"],
    ["server", "server", "server", "server", "none"],
)
DROP_ENTRY = (
    ["GroupId", "ItemId", "Rate", "Weight", "MinCount", "MaxCount"],
    ["key:parent:DropGroup", "key:ref:Item", "float?", "int32?", "int32", "int32"],
    ["server", "server", "server", "server", "server", "server"],
)


class DropRuleCase(DesignDataCase):
    def setUp(self) -> None:
        super().setUp()
        self.enums["DropMethod"] = {"INDEPENDENT": 1, "WEIGHTED": 2}
        self.add_table("Item", sheet(*ITEM, [3000, "MISCELLANEOUS", 99], [3001, "MISCELLANEOUS", 99], [3002, "MISCELLANEOUS", 99]))

    def add_monsters(self, *rows: list) -> None:
        self.add_table("Monster", sheet(*MONSTER, *rows))

    def add_groups(self, *rows: list) -> None:
        self.add_table("DropGroup", sheet(*DROP_GROUP, *rows))

    def add_entries(self, *rows: list) -> None:
        self.add_table("DropEntry", sheet(*DROP_ENTRY, *rows), nest="Entries")

    def add_valid_drops(self) -> None:
        """독립 판정 그룹 1과 가중치 그룹 2. 몬스터 5000이 둘 다 쓴다."""
        self.add_monsters([5000, 5, 10, 10, 20, "1;2"])
        self.add_groups([1, "INDEPENDENT", 1.0, None, "물약"], [2, "WEIGHTED", 0.5, 2, None])
        self.add_entries(
            [1, 3000, 0.1, None, 1, 3],
            [2, 3000, None, 10, 1, 1],
            [2, 3001, None, 30, 1, 1],
            [2, 3002, None, 60, 2, 2],
        )


class MonsterBaseRewardTest(DropRuleCase):
    def setUp(self) -> None:
        super().setUp()
        self.add_groups([1, "INDEPENDENT", 1.0, None, None])
        self.add_entries([1, 3000, 0.1, None, 1, 1])

    def test_equal_min_and_max_pass(self):
        self.add_monsters([5000, 5, 5, 0, 0, "1"])
        self.build_ok()

    def test_min_exp_above_max_exp_is_rejected(self):
        self.add_monsters([5000, 11, 10, 10, 20, "1"])
        self.assert_error("Monster.xlsx", "5행", "MinExp", "11", "MaxExp", "10")

    def test_min_gold_above_max_gold_is_rejected(self):
        self.add_monsters([5000, 5, 10, 21, 20, "1"])
        self.assert_error("Monster.xlsx", "5행", "MinGold", "21", "MaxGold", "20")

    def test_negative_reward_is_rejected(self):
        self.add_monsters([5000, -1, 10, 10, 20, "1"])
        self.assert_error("Monster.xlsx", "5행", "MinExp", "-1")


class DropGroupTest(DropRuleCase):
    def test_valid_drops_pass_without_warning(self):
        self.add_valid_drops()
        self.assertEqual(self.build_ok().warnings, [])

    def test_group_rate_must_be_above_zero_and_at_most_one(self):
        for rate in (0.0, -0.1, 1.5):
            with self.subTest(rate=rate):
                self.add_monsters([5000, 5, 10, 10, 20, "1"])
                self.add_groups([1, "INDEPENDENT", rate, None, None])
                self.add_entries([1, 3000, 0.1, None, 1, 1])
                self.assert_error("DropGroup.xlsx", "5행", "Rate", str(rate))

    def test_group_without_entries_is_rejected(self):
        self.add_monsters([5000, 5, 10, 10, 20, "1;2"])
        self.add_groups([1, "INDEPENDENT", 1.0, None, None], [2, "INDEPENDENT", 1.0, None, None])
        self.add_entries([1, 3000, 0.1, None, 1, 1])
        self.assert_error("DropGroup.xlsx", "6행", "2", "항목")

    def test_pick_count_above_entry_count_is_rejected(self):
        self.add_monsters([5000, 5, 10, 10, 20, "1"])
        self.add_groups([1, "WEIGHTED", 1.0, 3, None])
        self.add_entries([1, 3000, None, 1, 1, 1], [1, 3001, None, 1, 1, 1])
        self.assert_error("DropGroup.xlsx", "5행", "PickCount", "3", "2")

    def test_pick_count_equal_to_entry_count_passes(self):
        self.add_monsters([5000, 5, 10, 10, 20, "1"])
        self.add_groups([1, "WEIGHTED", 1.0, 2, None])
        self.add_entries([1, 3000, None, 1, 1, 1], [1, 3001, None, 1, 1, 1])
        self.build_ok()

    def test_weighted_group_needs_pick_count_of_at_least_one(self):
        for pick_count in (None, 0):
            with self.subTest(pick_count=pick_count):
                self.add_monsters([5000, 5, 10, 10, 20, "1"])
                self.add_groups([1, "WEIGHTED", 1.0, pick_count, None])
                self.add_entries([1, 3000, None, 1, 1, 1])
                self.assert_error("DropGroup.xlsx", "5행", "PickCount")

    def test_independent_group_with_pick_count_is_rejected(self):
        self.add_monsters([5000, 5, 10, 10, 20, "1"])
        self.add_groups([1, "INDEPENDENT", 1.0, 1, None])
        self.add_entries([1, 3000, 0.1, None, 1, 1])
        self.assert_error("DropGroup.xlsx", "5행", "PickCount", "INDEPENDENT")

    def test_group_no_monster_uses_is_a_warning(self):
        self.add_monsters([5000, 5, 10, 10, 20, "1"])
        self.add_groups([1, "INDEPENDENT", 1.0, None, None], [2, "INDEPENDENT", 1.0, None, None])
        self.add_entries([1, 3000, 0.1, None, 1, 1], [2, 3000, 0.1, None, 1, 1])
        self.assert_warning("DropGroup.xlsx", "6행", "2", "몬스터")


class DropEntryTest(DropRuleCase):
    def setUp(self) -> None:
        super().setUp()
        self.add_monsters([5000, 5, 10, 10, 20, "1;2"])
        self.add_groups([1, "INDEPENDENT", 1.0, None, None], [2, "WEIGHTED", 1.0, 1, None])

    def test_independent_entry_with_weight_is_rejected(self):
        self.add_entries([1, 3000, 0.1, 5, 1, 1], [2, 3000, None, 1, 1, 1])
        self.assert_error("DropEntry.xlsx", "5행", "Weight", "INDEPENDENT")

    def test_independent_entry_without_rate_is_rejected(self):
        self.add_entries([1, 3000, None, None, 1, 1], [2, 3000, None, 1, 1, 1])
        self.assert_error("DropEntry.xlsx", "5행", "Rate", "INDEPENDENT")

    def test_weighted_entry_with_rate_is_rejected(self):
        self.add_entries([1, 3000, 0.1, None, 1, 1], [2, 3000, 0.5, 1, 1, 1])
        self.assert_error("DropEntry.xlsx", "6행", "Rate", "WEIGHTED")

    def test_weighted_entry_without_weight_is_rejected(self):
        self.add_entries([1, 3000, 0.1, None, 1, 1], [2, 3000, None, None, 1, 1])
        self.assert_error("DropEntry.xlsx", "6행", "Weight", "WEIGHTED")

    def test_entry_rate_must_be_above_zero_and_at_most_one(self):
        self.add_entries([1, 3000, 1.5, None, 1, 1], [2, 3000, None, 1, 1, 1])
        self.assert_error("DropEntry.xlsx", "5행", "Rate", "1.5")

    def test_entry_weight_must_be_at_least_one(self):
        self.add_entries([1, 3000, 0.1, None, 1, 1], [2, 3000, None, 0, 1, 1])
        self.assert_error("DropEntry.xlsx", "6행", "Weight", "0")

    def test_min_count_above_max_count_is_rejected(self):
        self.add_entries([1, 3000, 0.1, None, 3, 2], [2, 3000, None, 1, 1, 1])
        self.assert_error("DropEntry.xlsx", "5행", "MinCount", "3", "MaxCount", "2")

    def test_min_count_below_one_is_rejected(self):
        self.add_entries([1, 3000, 0.1, None, 0, 1], [2, 3000, None, 1, 1, 1])
        self.assert_error("DropEntry.xlsx", "5행", "MinCount", "0")


if __name__ == "__main__":
    unittest.main()
