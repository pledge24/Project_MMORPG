"""세로형 설정 시트. `Key | Type | Target | Value | Description` 시트 하나가 구조체 하나가 된다."""

from __future__ import annotations

import json
import unittest

from tests.helpers import DesignDataCase, setting_sheet, sheet


class SettingTest(DesignDataCase):
    def setUp(self) -> None:
        super().setUp()
        self.add_table("Map", sheet(["Id"], ["key:int32"], ["both"], [1111], [2222]))

    def test_setting_sheet_becomes_one_object_with_one_field_per_line(self):
        self.add_setting("Progress", setting_sheet(
            ["StartMapId", "ref:Map", "server", 1111, "시작 맵"],
            ["StartGold", "int64", "server", 10000, "시작 골드"],
            ["Classes", "enum[]:CharacterClass", "both", "WARRIOR;MAGE", None],
            ["Note", "string", "none", "메모", None],
        ))

        text = self.output("Progress.json")
        self.assertEqual(text, '{\n"StartMapId":1111,\n"StartGold":10000,\n"Classes":["WARRIOR","MAGE"]\n}\n')
        self.assertEqual(json.loads(text)["StartGold"], 10000)

    def test_two_sheets_of_one_workbook_are_two_settings(self):
        self.add_setting("Progress", setting_sheet(["StartGold", "int64", "server", 1, None]))
        self.add_setting("Lobby", setting_sheet(["SlotCount", "int32", "server", 3, None]))

        result = self.build_ok()
        self.assertIn("Progress.json", result.outputs[self.out_dir])
        self.assertIn("Lobby.json", result.outputs[self.out_dir])

    def test_wrong_header_is_rejected(self):
        self.add_setting("Progress", [["Name", "Type", "Target", "Value", "Description"], ["StartGold", "int64", "server", 1, None]])
        self.assert_error("Config.xlsx 시트 'Progress' 1행", "Key | Type | Target | Value | Description")

    def test_missing_sheet_is_rejected(self):
        self.add_setting("Progress", setting_sheet(["StartGold", "int64", "server", 1, None]))
        self.settings["Lobby"] = ("Config.xlsx", "Lobby", ["game"])
        self.assert_error("Config.xlsx", "시트 'Lobby'가 없다")

    def test_field_errors_name_the_row(self):
        self.add_setting("Progress", setting_sheet(
            ["StartMapId", "ref:Map", "server", 9999, None],
            ["StartGold", "int64", "server", "많이", None],
            ["StartGold", "int64", "server", 1, None],
            ["Bad", "key:int32", "server", 1, None],
        ))

        errors = self.assert_error("시트 'Progress' 2행 D열(StartMapId)", "9999", "'Map'")
        self.assertTrue(any("3행 D열(StartGold)" in e and "정수가 아니다" in e for e in errors))
        self.assertTrue(any("4행 A열(StartGold)" in e and "겹친다" in e for e in errors))
        self.assertTrue(any("5행 B열(Bad)" in e for e in errors))

    def test_json_cell_is_rejected(self):
        self.add_setting("Progress", setting_sheet(["StartPos", "string", "server", '{"x":1,"y":2}', None]))
        self.assert_error("2행 D열(StartPos)", "JSON 셀")


if __name__ == "__main__":
    unittest.main()
