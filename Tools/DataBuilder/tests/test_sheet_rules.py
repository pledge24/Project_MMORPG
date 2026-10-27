"""시트 규칙: 1행 이름, 2행 타입, 3행 대상, 4행 설명, 5행부터 데이터. 시트를 이어 붙이고 시트 이름은 데이터가 아니다."""

from __future__ import annotations

import json
import unittest

from tests.helpers import DesignDataCase, sheet

NAMES = ["Id", "Name", "Memo"]
TYPES = ["key:int32", "string", "string"]
TARGETS = ["both", "server", "none"]


class SheetRulesTest(DesignDataCase):
    def test_rows_start_at_fifth_row_and_description_is_not_data(self):
        self.add_table("Item", sheet(NAMES, TYPES, TARGETS, [1, "검", "메모"], descriptions=["번호", "이름", "메모"]))

        rows = json.loads(self.output("Item.json"))
        self.assertEqual(rows, [{"Id": 1, "Name": "검"}])

    def test_sheets_are_concatenated_and_sheet_name_is_not_data(self):
        self.add_table(
            "Item",
            sheet(NAMES, TYPES, TARGETS, [2, "방패", None]),
            sheet(NAMES, TYPES, TARGETS, [1, "검", None]),
        )

        text = self.output("Item.json")
        self.assertEqual(json.loads(text), [{"Id": 1, "Name": "검"}, {"Id": 2, "Name": "방패"}])
        self.assertNotIn("Sheet", text)

    def test_sheets_with_different_columns_are_rejected(self):
        self.add_table(
            "Item",
            sheet(NAMES, TYPES, TARGETS, [1, "검", None]),
            sheet(NAMES, ["key:int32", "string", "int32"], TARGETS, [2, "방패", None]),
        )

        self.assert_error("Item.xlsx", "시트 'Sheet2'", "열 구성", "Sheet1")

    def test_unknown_target_is_rejected(self):
        self.add_table("Item", sheet(NAMES, TYPES, ["both", "SERVER", "none"], [1, "검", None]))

        self.assert_error("Item.xlsx", "시트 'Sheet1'", "3행", "B열(Name)", "SERVER")

    def test_unknown_type_is_rejected(self):
        self.add_table("Item", sheet(NAMES, ["key:int32", "text", "string"], TARGETS, [1, "검", None]))

        self.assert_error("Item.xlsx", "2행", "B열(Name)", "text")

    def test_column_name_must_be_pascal_case_and_unique(self):
        self.add_table("Item", sheet(["Id", "item_name", "Id"], TYPES, TARGETS, [1, "검", None]))

        errors = self.assert_error("1행", "B열(item_name)", "PascalCase")
        self.assertTrue(any("C열(Id)" in error and "겹친다" in error for error in errors))

    def test_value_in_unnamed_column_is_rejected(self):
        self.add_table("Item", sheet(NAMES, TYPES, TARGETS, [1, "검", None, "남은 값"]))

        self.assert_error("Item.xlsx", "5행", "D열", "이름 없는 열")

    def test_empty_rows_are_skipped(self):
        self.add_table("Item", sheet(NAMES, TYPES, TARGETS, [1, "검", None], [None, None, None], [2, "방패", None]))

        self.assertEqual(len(json.loads(self.output("Item.json"))), 2)

    def test_table_without_key_column_is_rejected(self):
        self.add_table("Item", sheet(NAMES, ["int32", "string", "string"], TARGETS, [1, "검", None]))

        self.assert_error("Item.xlsx", "키 열이 없다")

    def test_key_must_reach_every_side_that_gets_a_column(self):
        self.add_table("Item", sheet(NAMES, TYPES, ["server", "both", "none"], [1, "검", None]))

        self.assert_error("Item.xlsx", "A열(Id)", "client")

    def test_json_cell_is_rejected(self):
        self.add_table("Item", sheet(NAMES, TYPES, TARGETS, [1, '{"x": 1}', None], [2, "[1, 2]", None]))

        errors = self.assert_error("Item.xlsx", "5행", "B열(Name)", "JSON 셀")
        self.assertTrue(any("6행" in error and "JSON 셀" in error for error in errors))

    def test_errors_name_workbook_sheet_row_and_column(self):
        self.add_table("Item", sheet(NAMES, TYPES, TARGETS, [1, "검", None], ["하나", "방패", None]))

        self.assert_error("Item.xlsx 시트 'Sheet1' 6행 A열(Id)", "정수가 아니다")

    def test_missing_workbook_is_rejected(self):
        self.tables["Item"] = ("Item.xlsx", ["game"])

        self.assert_error("Item.xlsx", "워크북이 없다")


if __name__ == "__main__":
    unittest.main()
