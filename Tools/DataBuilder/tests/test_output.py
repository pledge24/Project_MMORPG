"""산출물: 한 행 한 줄, 키 순 정렬, 열 순서 고정. 같은 원본이면 같은 바이트. 실패하면 산출물을 바꾸지 않는다."""

from __future__ import annotations

import difflib
import io
import json
import unittest
from contextlib import redirect_stderr, redirect_stdout

from databuilder.build import run
from tests.helpers import DesignDataCase, sheet

NAMES = ["Name", "Id", "Price", "Icon", "Memo"]
TYPES = ["string", "key:int32", "int32", "string", "string"]
TARGETS = ["both", "both", "server", "client", "none"]


class OutputTest(DesignDataCase):
    def add_items(self, *rows) -> None:
        self.add_table("Thing", sheet(NAMES, TYPES, TARGETS, *rows))

    def run_builder(self) -> int:
        with redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
            return run(self.config_path(), self.root)

    def test_one_row_per_line_sorted_by_key_in_column_order(self):
        self.add_items(["방패", 2, 50, "I_Shield", "메모"], ["검", 1, 100, "I_Sword", None])

        self.assertEqual(self.output("Thing.json"), '[\n{"Name":"검","Id":1,"Price":100},\n{"Name":"방패","Id":2,"Price":50}\n]\n')

    def test_server_output_drops_client_and_none_columns(self):
        self.add_items(["검", 1, 100, "I_Sword", "메모"])

        row = json.loads(self.output("Thing.json"))[0]
        self.assertEqual(list(row), ["Name", "Id", "Price"])

    def test_same_source_gives_same_bytes(self):
        self.add_items(["검", 1, 100, "I", None], ["방패", 2, 50, "I", None])

        self.assertEqual(self.run_builder(), 0)
        first = {path.name: path.read_bytes() for path in self.out_dir.iterdir()}
        self.assertEqual(self.run_builder(), 0)
        second = {path.name: path.read_bytes() for path in self.out_dir.iterdir()}
        self.assertEqual(first, second)
        self.assertNotIn(b"\r\n", first["Thing.json"], "줄바꿈은 LF로 고정한다")

    def test_changing_one_value_changes_one_line(self):
        rows = [["검", 1, 100, "I", None], ["방패", 2, 50, "I", None], ["물약", 3, 10, "I", None]]
        self.add_items(*rows)
        before = self.output("Thing.json")
        rows[1][2] = 60
        self.add_items(*rows)
        after = self.output("Thing.json")

        changed = [line for line in difflib.ndiff(before.splitlines(), after.splitlines()) if line[:1] in "+-"]
        self.assertEqual(changed, ['- {"Name":"방패","Id":2,"Price":50},', '+ {"Name":"방패","Id":2,"Price":60},'])

    def test_failure_leaves_outputs_unchanged(self):
        self.add_items(["검", 1, 100, "I", None])
        self.assertEqual(self.run_builder(), 0)
        before = {path.name: path.read_bytes() for path in self.out_dir.iterdir()}

        self.add_items(["검", 1, 100, "I", None], ["방패", 1, 50, "I", None])
        self.assertEqual(self.run_builder(), 1)

        self.assertEqual({path.name: path.read_bytes() for path in self.out_dir.iterdir()}, before)

    def test_output_folder_is_emptied_on_each_run(self):
        self.add_items(["검", 1, 100, "I", None])
        self.out_dir.mkdir(parents=True)
        (self.out_dir / "Stale.json").write_text("[]", encoding="utf-8")

        self.assertEqual(self.run_builder(), 0)

        self.assertEqual(sorted(path.name for path in self.out_dir.iterdir()), ["DataVersion.json", "DesignData.h", "Thing.json"])

    def test_output_folder_must_be_named_generated(self):
        self.add_items(["검", 1, 100, "I", None])
        path = self.config_path()
        path.write_text(path.read_text(encoding="utf-8").replace("Server/Game/Generated", "Server/Game"), encoding="utf-8")

        with redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()) as errors:
            self.assertEqual(run(path, self.root), 1)
        self.assertIn("Generated", errors.getvalue())

    def test_data_version_follows_values_not_descriptions(self):
        self.add_items(["검", 1, 100, "I", None])
        version = self.build_ok().version
        self.assertRegex(version, r"^[0-9a-f]{16}$")
        self.assertEqual(json.loads(self.output("DataVersion.json")), {"Version": version})

        self.add_table("Thing", sheet(NAMES, TYPES, TARGETS, ["검", 1, 100, "I", None], descriptions=["이름", "번호", "가격", "아이콘", "메모"]))
        self.assertEqual(self.build_ok().version, version, "설명은 산출물을 바꾸지 않는다")

        self.add_items(["검", 1, 100, "I", "메모"])
        self.assertEqual(self.build_ok().version, version, "어디에도 싣지 않는 열은 산출물을 바꾸지 않는다")

        self.add_items(["검", 1, 101, "I", None])
        self.assertNotEqual(self.build_ok().version, version)


if __name__ == "__main__":
    unittest.main()
