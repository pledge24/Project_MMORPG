"""서버 모듈. 모듈마다 산출물 폴더와 생성되는 C++ 네임스페이스가 따로다."""

from __future__ import annotations

import unittest

from databuilder.config import ConfigError
from tests.helpers import DesignDataCase, setting_sheet


class ModuleTest(DesignDataCase):
    def test_header_namespace_defaults_to_design_data(self):
        self.add_setting("Progress", setting_sheet(["StartGold", "int64", "server", 1, None]))

        self.assertIn("\nnamespace DesignData\n{\n", self.output("DesignData.h"))

    def test_setting_goes_only_to_its_module_with_that_namespace(self):
        lobby_dir = self.add_module("lobby", "Server/Lobby/Generated", "LobbyDesignData")
        self.add_setting("Progress", setting_sheet(["StartGold", "int64", "server", 1, None]))
        self.add_setting("Account", setting_sheet(["SlotCount", "int32", "server", 3, None]), modules=["lobby"])

        outputs = self.build_ok().outputs
        self.assertIn("Progress.json", outputs[self.out_dir])
        self.assertNotIn("Account.json", outputs[self.out_dir])
        self.assertIn("Account.json", outputs[lobby_dir])
        self.assertNotIn("Progress.json", outputs[lobby_dir])

        lobby_header = outputs[lobby_dir]["DesignData.h"]
        self.assertIn("\nnamespace LobbyDesignData\n{\n", lobby_header)
        self.assertIn("struct AccountConfig", lobby_header)
        self.assertNotIn("ProgressConfig", lobby_header, "다른 모듈의 설정 구조체를 싣지 않는다")

    def test_two_modules_with_one_namespace_are_rejected(self):
        # 한 번역 단위가 두 모듈의 헤더를 함께 include하면 같은 이름이 두 번 정의된다.
        self.add_module("lobby", "Server/Lobby/Generated")
        with self.assertRaisesRegex(ConfigError, "namespace 'DesignData'"):
            self.build()

    def test_namespace_must_be_identifier(self):
        self.add_module("lobby", "Server/Lobby/Generated", "lobby_data")
        with self.assertRaisesRegex(ConfigError, "modules.lobby의 'namespace'"):
            self.build()


if __name__ == "__main__":
    unittest.main()
