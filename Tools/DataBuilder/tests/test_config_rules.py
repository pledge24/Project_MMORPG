"""설정 워크북의 초기 진행(`Progress`)과 로비(`Lobby`) 시트."""

from __future__ import annotations

import unittest
from typing import Any

from tests.helpers import DesignDataCase, setting_sheet, sheet


def progress(start_map: Any = 1111, start_gold: Any = 10000) -> list[list[Any]]:
    return setting_sheet(
        ["StartMapId", "ref:Map", "server", start_map, "시작 맵"],
        ["StartX", "float", "server", 1.5, None],
        ["StartY", "float", "server", -2, None],
        ["StartZ", "float", "server", 0, None],
        ["StartYaw", "float", "server", 90, None],
        ["StartGold", "int64", "server", start_gold, "시작 골드"],
    )


def lobby(slot_count: Any = 4, classes: Any = "WARRIOR") -> list[list[Any]]:
    return setting_sheet(
        ["DefaultCharacterSlotCount", "int32", "server", slot_count, None],
        ["CreatableClasses", "enum[]:CharacterClass", "server", classes, None],
    )


class ConfigSheetTest(DesignDataCase):
    def setUp(self) -> None:
        super().setUp()
        self.add_table("Map", sheet(["Id"], ["key:int32"], ["both"], [1111], [2222]))

    def add_config(self, progress_rows: list[list[Any]] | None = None, lobby_rows: list[list[Any]] | None = None) -> None:
        self.add_setting("Progress", progress_rows or progress())
        self.add_setting("Lobby", lobby_rows or lobby())

    def test_config_sheets_pass(self):
        self.add_config(lobby_rows=lobby(classes="WARRIOR;MAGE"))

        outputs = self.build_ok().outputs[self.out_dir]
        self.assertIn('"StartMapId":1111', outputs["Progress.json"])
        self.assertIn('"CreatableClasses":["WARRIOR","MAGE"]', outputs["Lobby.json"])

    def test_duplicate_key_is_rejected(self):
        rows = progress()
        rows.append(["StartGold", "int64", "server", 5, None])
        self.add_config(progress_rows=rows)
        self.assert_error("Config.xlsx 시트 'Progress' 8행 A열(StartGold)", "7행과 겹친다")

    def test_missing_start_map_is_rejected(self):
        self.add_config(progress_rows=progress(start_map=9999))
        self.assert_error("시트 'Progress' 2행 D열(StartMapId)", "9999", "'Map'")

    def test_negative_start_gold_is_rejected(self):
        self.add_config(progress_rows=progress(start_gold=-1))
        self.assert_error("시트 'Progress' 7행 D열(StartGold)", "0 이상")

    def test_zero_start_gold_passes(self):
        self.add_config(progress_rows=progress(start_gold=0))
        self.build_ok()

    def test_empty_class_list_is_rejected(self):
        self.add_config(lobby_rows=lobby(classes=None))
        self.assert_error("시트 'Lobby' 3행 D열(CreatableClasses)", "비어 있다")

    def test_duplicate_class_is_rejected(self):
        self.add_config(lobby_rows=lobby(classes="WARRIOR;MAGE;WARRIOR"))
        self.assert_error("시트 'Lobby' 3행 D열(CreatableClasses)", "WARRIOR", "두 번")

    def test_slot_count_below_one_is_rejected(self):
        self.add_config(lobby_rows=lobby(slot_count=0))
        self.assert_error("시트 'Lobby' 2행 D열(DefaultCharacterSlotCount)", "1 이상")


if __name__ == "__main__":
    unittest.main()
