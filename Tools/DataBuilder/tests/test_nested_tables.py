"""자식 표를 부모 표의 행 아래에 중첩하는 산출물. 설정 파일에서 자식 표에 `nest`를 적는다."""

from __future__ import annotations

import json
import unittest

from databuilder.build import generate
from databuilder.config import load_config
from tests.helpers import DesignDataCase, sheet

GROUP = (["Id", "Rate"], ["key:int32", "float"], ["both", "both"])
ENTRY = (["GroupId", "ItemId", "Count"], ["key:parent:Group", "key:int32", "int32"], ["both", "both", "both"])


class NestedTableTest(DesignDataCase):
    def add_groups(self, *rows: list) -> None:
        self.add_table("Group", sheet(*GROUP, *rows))

    def add_entries(self, *rows: list, nest: str | None = "Entries") -> None:
        self.add_table("Entry", sheet(*ENTRY, *rows), nest=nest)

    def test_children_are_written_under_their_parent_one_per_line(self):
        self.add_groups([2, 1.0], [1, 0.5], [3, 0.25])
        self.add_entries([1, 20, 2], [2, 10, 3], [1, 10, 1])

        self.assertEqual(
            self.output("Group.json"),
            '[\n'
            '{"Id":1,"Rate":0.5,"Entries":[\n'
            '{"ItemId":10,"Count":1},\n'
            '{"ItemId":20,"Count":2}\n'
            ']},\n'
            '{"Id":2,"Rate":1.0,"Entries":[\n'
            '{"ItemId":10,"Count":3}\n'
            ']},\n'
            '{"Id":3,"Rate":0.25,"Entries":[]}\n'
            ']\n',
        )

    def test_nested_table_has_no_file_or_table_type_of_its_own(self):
        self.add_groups([1, 0.5])
        self.add_entries([1, 10, 1])

        outputs = self.build_ok().outputs[self.out_dir]
        self.assertNotIn("Entry.json", outputs)
        header = outputs["DesignData.h"]
        self.assertNotIn("EntryTable", header)
        self.assertNotIn("ENTRY_FILE", header)
        self.assertIn("vector<EntryRow> Entries;", header)
        self.assertLess(header.index("struct EntryRow"), header.index("struct GroupRow"), "자식 행 구조체가 먼저 선언되어야 한다")

    def test_nested_output_is_valid_json(self):
        self.add_groups([1, 0.5])
        self.add_entries([1, 10, 1])

        self.assertEqual(json.loads(self.output("Group.json")), [{"Id": 1, "Rate": 0.5, "Entries": [{"ItemId": 10, "Count": 1}]}])

    def test_nest_without_parent_column_is_rejected(self):
        self.add_groups([1, 0.5])
        self.add_table("Entry", sheet(["Id", "Count"], ["key:int32", "int32"], ["both", "both"], [1, 1]), nest="Entries")
        self.assert_error("Entry.xlsx", "부모 표 참조 열")

    def test_nest_name_that_clashes_with_parent_column_is_rejected(self):
        self.add_groups([1, 0.5])
        self.add_entries([1, 10, 1], nest="Rate")
        self.assert_error("Entry.xlsx", "Rate", "Group")

    def test_nested_table_must_go_to_the_same_modules_as_its_parent(self):
        # 부모 행 구조체가 자식 행 구조체를 필드로 가지므로, 어느 한쪽만 가는 모듈의 헤더는 컴파일되지 않는다.
        for group_modules, entry_modules in ((["game"], ["game", "lobby"]), (["game", "lobby"], ["game"])):
            with self.subTest(group=group_modules, entry=entry_modules):
                self.add_table("Group", sheet(*GROUP, [1, 0.5]), modules=group_modules)
                self.add_table("Entry", sheet(*ENTRY, [1, 10, 1]), modules=entry_modules, nest="Entries")
                path = self.config_path()
                path.write_text(path.read_text(encoding="utf-8") + '\n[modules.lobby]\nserver_dir = "Server/Lobby/Generated"\n', encoding="utf-8")

                errors = generate(load_config(path, self.root)).errors
                self.assertTrue(any("Entry" in error and "lobby" in error for error in errors), errors)


if __name__ == "__main__":
    unittest.main()
