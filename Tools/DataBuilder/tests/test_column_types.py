"""열 타입마다 셀 값을 읽는 규칙. 정수, 실수, 문자열, 불, 열거형과 열거형 목록."""

from __future__ import annotations

import json
import unittest

from databuilder.column_types import CellError, ColumnType, parse_type
from tests.helpers import DesignDataCase, sheet


class ParseTypeTest(unittest.TestCase):
    def test_valid_types(self):
        self.assertEqual(parse_type("int32"), ColumnType("int32"))
        self.assertEqual(parse_type("key:string"), ColumnType("string", is_key=True))
        self.assertEqual(parse_type("enum[]:ItemType"), ColumnType("enum", "ItemType", is_list=True))
        self.assertEqual(parse_type("ref?:Thing"), ColumnType("ref", "Thing", is_optional=True))
        self.assertEqual(parse_type("ref[]:Thing"), ColumnType("ref", "Thing", is_list=True))
        self.assertEqual(parse_type("key:parent:Thing"), ColumnType("parent", "Thing", is_key=True))
        self.assertEqual(parse_type("float?"), ColumnType("float", is_optional=True))
        self.assertEqual(parse_type("int32?"), ColumnType("int32", is_optional=True))

    def test_invalid_types(self):
        for text in ["", None, "int", "key:float", "key:bool", "key:ref?:Thing", "key:enum[]:ItemType", "enum?:ItemType", "parent[]:Thing", "ref:", "key:int32?", "bool?", "string?"]:
            with self.subTest(text=text), self.assertRaises(CellError):
                parse_type(text)


def one_column(type_text: str, *values) -> list[list]:
    return sheet(["Id", "Value"], ["key:int32", type_text], ["both", "both"], *[[i + 1, v] for i, v in enumerate(values)])


class CellValueTest(DesignDataCase):
    def values(self) -> list:
        return [row["Value"] for row in json.loads(self.output("Thing.json"))]

    def test_integers(self):
        self.add_table("Thing", one_column("int32", 7, 7.0, "8", -(2**31)))
        self.assertEqual(self.values(), [7, 7, 8, -(2**31)])

    def test_integer_out_of_range_or_not_integer_is_rejected(self):
        self.add_table("Thing", one_column("int32", 2**31, 1.5, True))
        errors = self.assert_error("5행", "범위")
        self.assertTrue(any("6행" in error and "정수가 아니다" in error for error in errors))
        self.assertTrue(any("7행" in error and "정수가 아니다" in error for error in errors))

    def test_int64_holds_large_values(self):
        self.add_table("Thing", one_column("int64", 2**40))
        self.assertEqual(self.values(), [2**40])

    def test_float(self):
        self.add_table("Thing", one_column("float", 1.5, 2))
        self.assertEqual(self.values(), [1.5, 2.0])

    def test_bool_accepts_excel_bool_and_text(self):
        self.add_table("Thing", one_column("bool", True, "false", "TRUE"))
        self.assertEqual(self.values(), [True, False, True])

    def test_bool_rejects_other_values(self):
        self.add_table("Thing", one_column("bool", 1))
        self.assert_error("5행", "불이 아니다")

    def test_empty_required_cell_is_rejected(self):
        self.add_table("Thing", one_column("string", "검", None))
        self.assert_error("6행", "B열(Value)", "비어 있다")

    def test_optional_number_writes_blank_cell_as_null(self):
        self.add_table("Thing", one_column("float?", 0.5, None))
        self.assertEqual(self.values(), [0.5, None])

    def test_optional_number_still_checks_type(self):
        self.add_table("Thing", one_column("int32?", 1.5))
        self.assert_error("5행", "정수가 아니다")

    def test_proto_enum_is_written_without_prefix(self):
        self.add_table("Thing", one_column("enum:ItemType", "GEAR", "CONSUMABLE"))
        self.assertEqual(self.values(), ["GEAR", "CONSUMABLE"])

    def test_proto_enum_zero_value_and_full_name_are_rejected(self):
        self.add_table("Thing", one_column("enum:ItemType", "NONE", "ITEM_TYPE_GEAR"))
        errors = self.assert_error("5행", "NONE", "ItemType")
        self.assertTrue(any("6행" in error and "ITEM_TYPE_GEAR" in error for error in errors))

    def test_unknown_enum_value_is_rejected(self):
        self.add_table("Thing", one_column("enum:CurrencyType", "GOLD", "RUBY"))
        self.assert_error("6행", "RUBY", "CurrencyType", "GOLD, HONOR")

    def test_unknown_enum_is_rejected(self):
        self.add_table("Thing", one_column("enum:Color", "RED"))
        self.assert_error("2행", "Color")

    def test_enum_list_is_split_by_semicolon(self):
        self.add_table("Thing", one_column("enum[]:CharacterClass", "WARRIOR; MAGE", "WARRIOR", None))
        self.assertEqual(self.values(), [["WARRIOR", "MAGE"], ["WARRIOR"], []])

    def test_enum_list_with_unknown_or_empty_item_is_rejected(self):
        self.add_table("Thing", one_column("enum[]:CharacterClass", "WARRIOR;ARCHER", "WARRIOR;;MAGE"))
        errors = self.assert_error("5행", "ARCHER")
        self.assertTrue(any("6행" in error and "빈 항목" in error for error in errors))

    def test_enum_defined_in_proto_and_config_is_rejected(self):
        self.enums["ItemType"] = {"GEAR": 1}
        self.add_table("Thing", one_column("int32", 1))
        self.assert_error("ItemType", "Common.proto")


if __name__ == "__main__":
    unittest.main()
