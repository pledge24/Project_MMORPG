"""표 하나에만 걸리는 기획 데이터 규칙과, 저장소의 실제 원본이 통과하는지."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from databuilder.build import generate
from databuilder.config import load_config
from tests.helpers import DesignDataCase, sheet

REPO_ROOT = Path(__file__).resolve().parents[3]

CURRENCY = (["Id", "Name", "Type"], ["key:int32", "string", "enum:CurrencyType"], ["server"] * 3)


class CurrencyRuleTest(DesignDataCase):
    def test_exactly_one_gold_passes(self):
        self.add_table("Currency", sheet(*CURRENCY, [1, "골드", "GOLD"], [2, "명예", "HONOR"]))
        self.build_ok()

    def test_no_gold_is_rejected(self):
        self.add_table("Currency", sheet(*CURRENCY, [2, "명예", "HONOR"]))
        self.assert_error("Currency.xlsx", "GOLD", "0개")

    def test_two_golds_are_rejected(self):
        self.add_table("Currency", sheet(*CURRENCY, [1, "골드", "GOLD"], [2, "골드2", "GOLD"]))
        self.assert_error("Currency.xlsx", "GOLD", "2개", "5행", "6행")


class RepositoryDesignDataTest(unittest.TestCase):
    """저장소의 설정과 원본이 오류 없이 생성되고, 커밋된 산출물과 같은지 본다."""

    def test_repository_design_data_builds_and_matches_committed_outputs(self):
        config = load_config(REPO_ROOT / "Tools" / "DataBuilder" / "databuilder.toml", REPO_ROOT)
        result = generate(config)
        self.assertEqual(result.errors, [])

        for out_dir, files in result.outputs.items():
            for name, text in files.items():
                committed = out_dir / name
                with self.subTest(file=committed.relative_to(REPO_ROOT).as_posix()):
                    self.assertTrue(committed.is_file(), "생성기를 돌리지 않았다")
                    self.assertEqual(committed.read_bytes().replace(b"\r\n", b"\n"), text.encode("utf-8"),
                                     "커밋된 산출물이 원본과 다르다. Invoke-DataBuilder.ps1을 다시 돌린다")
            self.assertEqual(sorted(path.name for path in out_dir.iterdir()), sorted(files), "산출물 폴더에 생성하지 않는 파일이 있다")

    def test_currency_values_are_kept_from_old_table(self):
        config = load_config(REPO_ROOT / "Tools" / "DataBuilder" / "databuilder.toml", REPO_ROOT)
        files = generate(config).outputs[config.modules["game"].server_dir]

        self.assertEqual(json.loads(files["Currency.json"]), [{"Id": 1, "Name": "골드", "Type": "GOLD"}])


if __name__ == "__main__":
    unittest.main()
