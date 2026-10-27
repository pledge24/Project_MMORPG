"""생성되는 C++ 헤더. 필드 이름은 열 이름 그대로이고, .proto의 열거형은 프로토콜 헤더의 것을 쓴다."""

from __future__ import annotations

import unittest

from tests.helpers import DesignDataCase, setting_sheet, sheet


class EmitCppTest(DesignDataCase):
    def header(self) -> str:
        return self.output("DesignData.h")

    def test_row_struct_uses_column_names_and_server_columns_only(self):
        self.add_table("Thing", sheet(
            ["Id", "Name", "Type", "Icon", "Price"],
            ["key:int32", "string", "enum:ItemType", "string", "int64"],
            ["both", "both", "server", "client", "server"],
            [1, "검", "GEAR", "I_Sword", 100],
            descriptions=["번호", "이름", "종류", "아이콘", "가격"],
        ))

        header = self.header()
        self.assertIn("    struct ThingRow\n    {\n        /** 번호 */\n        int32 Id = 0;\n", header)
        self.assertIn("        string Name;\n", header)
        self.assertIn("        Protocol::ItemType Type = {};\n", header)
        self.assertIn("        int64 Price = 0;\n", header)
        self.assertNotIn("Icon", header)
        self.assertIn("using ThingTable = map<int32, ThingRow>;", header)
        self.assertIn('constexpr string_view THING_FILE = "Thing.json";', header)
        self.assertIn('#include "Protocol/Common.pb.h"', header)
        self.assertIn('{ "GEAR", Protocol::ITEM_TYPE_GEAR },', header)
        self.assertNotIn("enum class ItemType", header, ".proto에 있는 열거형은 생성하지 않는다")

    def test_enum_not_in_proto_is_generated(self):
        self.add_table("Currency", sheet(["Id", "Type"], ["key:int32", "enum:CurrencyType"], ["server"] * 2, [1, "GOLD"]))

        header = self.header()
        self.assertIn("    enum class CurrencyType : int32\n    {\n        GOLD = 1,\n        HONOR = 2,\n    };\n", header)
        self.assertNotIn("#include", header, "프로토콜 열거형을 쓰지 않으면 프로토콜 헤더도 include하지 않는다")

    def test_reference_list_optional_and_composite_key_types(self):
        self.add_table("Thing", sheet(["Id"], ["key:int32"], ["both"], [10]))
        self.add_table("Stat", sheet(
            ["Class", "Level", "Rewards", "Bonus", "Classes"],
            ["key:enum:CharacterClass", "key:int32", "ref[]:Thing", "ref?:Thing", "enum[]:CharacterClass"],
            ["both"] * 5,
            ["WARRIOR", 1, "10", None, "WARRIOR"],
        ))

        header = self.header()
        self.assertIn("using StatTable = map<tuple<Protocol::CharacterClass, int32>, StatRow>;", header)
        self.assertIn("        vector<int32> Rewards;\n", header)
        self.assertIn("        optional<int32> Bonus;\n", header)
        self.assertIn("        vector<Protocol::CharacterClass> Classes;\n", header)
        self.assertIn("auto key = tuple{ row.Class, row.Level };", header)

    def test_setting_struct(self):
        self.add_setting("Progress", setting_sheet(["StartGold", "int64", "server", 1, "시작 골드"], ["Note", "string", "client", "x", None]))

        header = self.header()
        self.assertIn("    struct ProgressConfig\n    {\n        /** 시작 골드 */\n        int64 StartGold = 0;\n    };\n", header)
        self.assertIn("inline optional<string> ParseProgress(const Json& document, OUT ProgressConfig& config)", header)


if __name__ == "__main__":
    unittest.main()
