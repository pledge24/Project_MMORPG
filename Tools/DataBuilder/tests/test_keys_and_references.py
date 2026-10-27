"""키와 참조. 키 중복, 복합 키, 다른 표의 키 참조(필수, 선택, 목록), 부모 표 참조."""

from __future__ import annotations

import json
import unittest

from tests.helpers import DesignDataCase, sheet


class KeyTest(DesignDataCase):
    def test_duplicate_key_is_rejected(self):
        self.add_table("Thing", sheet(["Id", "Name"], ["key:int32", "string"], ["both", "both"], [1, "검"], [1, "방패"]))

        self.assert_error("Thing.xlsx 시트 'Sheet1' 6행", "키(Id) 1", "5행과 겹친다")

    def test_duplicate_key_across_sheets_is_rejected(self):
        columns = (["Id", "Name"], ["key:int32", "string"], ["both", "both"])
        self.add_table("Thing", sheet(*columns, [1, "검"]), sheet(*columns, [1, "방패"]))

        self.assert_error("시트 'Sheet2' 5행", "시트 'Sheet1' 5행과 겹친다")

    def test_composite_key_allows_repeated_parts(self):
        self.add_table(
            "Stat",
            sheet(["Class", "Level", "Hp"], ["key:enum:CharacterClass", "key:int32", "int32"], ["both"] * 3,
                  ["MAGE", 1, 80], ["WARRIOR", 2, 120], ["WARRIOR", 1, 100]),
        )

        rows = json.loads(self.output("Stat.json"))
        self.assertEqual([(r["Class"], r["Level"]) for r in rows], [("WARRIOR", 1), ("WARRIOR", 2), ("MAGE", 1)],
                         "열거형 키는 값 번호 순서다")

    def test_duplicate_composite_key_is_rejected(self):
        self.add_table(
            "Stat",
            sheet(["Class", "Level", "Hp"], ["key:enum:CharacterClass", "key:int32", "int32"], ["both"] * 3,
                  ["WARRIOR", 1, 100], ["WARRIOR", 1, 110]),
        )

        self.assert_error("6행", "키(Class, Level) WARRIOR, 1", "겹친다")


class ReferenceTest(DesignDataCase):
    def setUp(self) -> None:
        super().setUp()
        self.add_table("Thing", sheet(["Id", "Name"], ["key:int32", "string"], ["both", "both"], [10, "검"], [20, "물약"]))

    def add_shop(self, type_text: str, *values) -> None:
        self.add_table("Shop", sheet(["Id", "Goods"], ["key:int32", type_text], ["both", "both"], *[[i + 1, v] for i, v in enumerate(values)]))

    def goods(self) -> list:
        return [row["Goods"] for row in json.loads(self.output("Shop.json"))]

    def test_required_reference(self):
        self.add_shop("ref:Thing", 10, "20")
        self.assertEqual(self.goods(), [10, 20])

    def test_missing_reference_is_rejected(self):
        self.add_shop("ref:Thing", 10, 30)
        self.assert_error("Shop.xlsx 시트 'Sheet1' 6행 B열(Goods)", "30", "'Thing'")

    def test_empty_required_reference_is_rejected(self):
        self.add_shop("ref:Thing", None)
        self.assert_error("5행", "B열(Goods)", "비어 있다")

    def test_optional_reference_allows_empty_cell(self):
        self.add_shop("ref?:Thing", 10, None)
        self.assertEqual(self.goods(), [10, None])

    def test_reference_list(self):
        self.add_shop("ref[]:Thing", "10;20", None)
        self.assertEqual(self.goods(), [[10, 20], []])

    def test_missing_reference_in_list_is_rejected(self):
        self.add_shop("ref[]:Thing", "10;99")
        self.assert_error("5행", "99", "'Thing'")

    def test_reference_to_unknown_table_is_rejected(self):
        self.add_shop("ref:Monster", 1)
        self.assert_error("Shop.xlsx", "2행", "'Monster'", "설정 파일에 없다")

    def test_reference_to_composite_key_table_is_rejected(self):
        self.add_table("Stat", sheet(["Class", "Level"], ["key:enum:CharacterClass", "key:int32"], ["both"] * 2, ["WARRIOR", 1]))
        self.add_shop("ref:Stat", 1)
        self.assert_error("Shop.xlsx", "'Stat'", "키 열이 하나가 아니라서")

    def test_parent_reference_as_key(self):
        self.add_table("ThingDetail", sheet(["ThingId", "Attack"], ["key:parent:Thing", "int32"], ["both"] * 2, [20, 5], [10, 3]))

        rows = json.loads(self.output("ThingDetail.json"))
        self.assertEqual([r["ThingId"] for r in rows], [10, 20])

    def test_missing_parent_is_rejected(self):
        self.add_table("ThingDetail", sheet(["ThingId", "Attack"], ["key:parent:Thing", "int32"], ["both"] * 2, [30, 5]))
        self.assert_error("ThingDetail.xlsx", "5행", "30", "'Thing'")

    def test_two_parent_columns_are_rejected(self):
        self.add_table("Pair", sheet(["Id", "A", "B"], ["key:int32", "parent:Thing", "parent:Thing"], ["both"] * 3, [1, 10, 20]))
        self.assert_error("Pair.xlsx", "부모 표 참조 열은 하나뿐")


if __name__ == "__main__":
    unittest.main()
