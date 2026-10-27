"""레벨 표(직업 공통)와 직업별 능력치 표의 규칙."""

from __future__ import annotations

import unittest

from tests.helpers import DesignDataCase, sheet

LEVEL = (["Level", "ExpRequirement"], ["key:int32", "int64"], ["server", "server"])
CLASS_LEVEL_STAT = (
    ["Class", "Level", "MaxHp", "MaxMp", "PhysicalAttack", "MagicalAttack"],
    ["key:enum:CharacterClass", "key:ref:Level", "int32", "int32", "int32", "int32"],
    ["server"] * 6,
)


class LevelRuleCase(DesignDataCase):
    def add_levels(self, *requirements: int, first_level: int = 1) -> None:
        """레벨 first_level부터 차례로 다음 레벨까지 필요한 경험치를 적는다."""
        rows = [[first_level + i, requirement] for i, requirement in enumerate(requirements)]
        self.add_table("Level", sheet(*LEVEL, *rows))

    def add_class_stats(self, *sheets: tuple[str, list[int]]) -> None:
        """직업마다 시트 하나. 각 시트에는 그 직업의 레벨 목록을 적는다. 직업이 없으면 머리글만 있는 시트 하나다."""
        self.add_table("ClassLevelStat", *[
            sheet(*CLASS_LEVEL_STAT, *[[class_name, level, 100 * level, 50 * level, 10 * level, 2 * level] for level in levels])
            for class_name, levels in sheets
        ] or [sheet(*CLASS_LEVEL_STAT)])


class LevelTableTest(LevelRuleCase):
    def test_levels_from_one_ending_with_zero_requirement_pass(self):
        self.add_levels(100, 200, 0)
        self.add_class_stats(("WARRIOR", [1, 2, 3]), ("MAGE", [1, 2, 3]))
        self.build_ok()

    def test_levels_not_starting_from_one_are_rejected(self):
        self.add_levels(100, 0, first_level=2)
        self.add_class_stats()
        self.assert_error("Level.xlsx", "5행", "레벨이 1이어야", "2")

    def test_gap_in_levels_is_rejected(self):
        self.add_table("Level", sheet(*LEVEL, [1, 100], [3, 0]))
        self.add_class_stats()
        self.assert_error("Level.xlsx", "6행", "레벨이 2이어야", "3")

    def test_max_level_with_nonzero_requirement_is_rejected(self):
        self.add_levels(100, 200)
        self.add_class_stats()
        self.assert_error("Level.xlsx", "6행", "최대 레벨", "ExpRequirement", "200")

    def test_zero_requirement_below_max_level_is_rejected(self):
        self.add_levels(100, 0, 0)
        self.add_class_stats()
        self.assert_error("Level.xlsx", "6행", "ExpRequirement", "1 이상")

    def test_empty_level_table_is_rejected(self):
        self.add_levels()
        self.add_class_stats()
        self.assert_error("Level.xlsx", "행이 없다")


class ClassLevelStatTableTest(LevelRuleCase):
    def test_class_missing_a_level_is_rejected(self):
        self.add_levels(100, 200, 0)
        self.add_class_stats(("WARRIOR", [1, 2, 3]), ("MAGE", [1, 3]))
        self.assert_error("ClassLevelStat.xlsx", "MAGE", "레벨 2")

    def test_same_class_and_level_twice_is_rejected(self):
        self.add_levels(100, 0)
        self.add_class_stats(("WARRIOR", [1, 2, 2]))
        self.assert_error("ClassLevelStat.xlsx", "7행", "WARRIOR", "2")

    def test_level_missing_from_level_table_is_rejected(self):
        self.add_levels(100, 0)
        self.add_class_stats(("WARRIOR", [1, 2, 3]))
        self.assert_error("ClassLevelStat.xlsx", "7행", "Level", "3")

    def test_class_absent_from_table_needs_no_rows(self):
        self.add_levels(100, 0)
        self.add_class_stats(("WARRIOR", [1, 2]))
        self.build_ok()


if __name__ == "__main__":
    unittest.main()
