"""클라이언트 산출물: 데이터 테이블에 채울 JSON과 그 행 구조체(USTRUCT) 헤더.

키 말고 클라이언트로 가는 열(both, client)이 하나라도 있는 표만 클라이언트 산출물을 만든다.
JSON의 각 행은 첫 필드 `RowName`이 데이터 테이블의 행 이름(기획 데이터의 키)이다.
"""

from __future__ import annotations

import io
import json
import unittest
from contextlib import redirect_stderr, redirect_stdout

from databuilder.build import run
from tests.helpers import DesignDataCase, setting_sheet, sheet

THING = (
    ["Id", "Name", "Type", "Classes", "Price", "Speed", "Sellable", "Secret", "Memo"],
    ["key:int32", "string", "enum:ItemType", "enum[]:CharacterClass", "int64", "float", "bool", "int32", "string"],
    ["both", "client", "both", "both", "both", "client", "both", "server", "none"],
)


class ClientOutputCase(DesignDataCase):
    def setUp(self) -> None:
        super().setUp()
        self.with_client = True


class ClientJsonTest(ClientOutputCase):
    def test_rows_start_with_row_name_and_carry_client_columns_in_key_order(self):
        self.add_table("Thing", sheet(*THING,
            [2, "방패", "GEAR", "", 50, 1.5, False, 7, "메모"],
            [1, "검", "GEAR", "WARRIOR;MAGE", 100, 2.0, True, 9, None],
        ))

        self.assertEqual(
            self.client_json()["Thing.json"],
            '[\n'
            '{"RowName":"1","Id":1,"Name":"검","Type":"GEAR","Classes":["WARRIOR","MAGE"],"Price":100,"Speed":2.0,"Sellable":true},\n'
            '{"RowName":"2","Id":2,"Name":"방패","Type":"GEAR","Classes":[],"Price":50,"Speed":1.5,"Sellable":false}\n'
            ']\n',
        )

    def test_table_without_client_columns_besides_the_key_has_no_client_output(self):
        self.add_table("Thing", sheet(*THING, [1, "검", "GEAR", "", 100, 2.0, True, 9, None]))
        self.add_table("Rank", sheet(["Rank", "Exp"], ["key:int32", "int64"], ["both", "server"], [1, 10]))
        self.add_table("Hidden", sheet(["Id", "Value"], ["key:int32", "int32"], ["server", "server"], [1, 10]))

        files = self.client_json()
        self.assertIn("Thing.json", files)
        self.assertNotIn("Rank.json", files, "키만 클라이언트로 가는 표는 클라이언트 산출물이 없다")
        self.assertNotIn("Hidden.json", files)
        self.assertNotIn("P1RankRow.h", self.client_source())

    def test_composite_key_row_name_joins_key_values(self):
        self.add_table("Stat", sheet(
            ["Class", "Level", "MaxHp"],
            ["key:enum:CharacterClass", "key:int32", "int32"],
            ["both", "both", "both"],
            ["MAGE", 1, 80], ["WARRIOR", 2, 120], ["WARRIOR", 1, 100],
        ))

        rows = json.loads(self.client_json()["Stat.json"])
        self.assertEqual([row["RowName"] for row in rows], ["WARRIOR_1", "WARRIOR_2", "MAGE_1"], "행은 키 순서다(열거형은 값 번호 순서)")

    def test_nested_children_are_written_under_their_parent_without_parent_column(self):
        self.add_table("Group", sheet(["Id", "Name"], ["key:int32", "string"], ["both", "client"], [1, "그룹"]))
        self.add_table("Entry", sheet(
            ["GroupId", "ItemId", "Count", "Rate"],
            ["key:parent:Group", "key:int32", "int32", "float"],
            ["both", "both", "both", "server"],
            [1, 20, 2, 0.5], [1, 10, 1, 0.5],
        ), nest="Entries")

        files = self.client_json()
        self.assertEqual(
            files["Group.json"],
            '[\n{"RowName":"1","Id":1,"Name":"그룹","Entries":[\n{"ItemId":10,"Count":1},\n{"ItemId":20,"Count":2}\n]}\n]\n',
        )
        self.assertNotIn("Entry.json", files)

    def test_parent_without_client_output_drops_nested_children(self):
        self.add_table("Group", sheet(["Id", "Rate"], ["key:int32", "float"], ["both", "server"], [1, 0.5]))
        self.add_table("Entry", sheet(["GroupId", "ItemId", "Count"], ["key:parent:Group", "key:int32", "int32"], ["both"] * 3, [1, 10, 1]), nest="Entries")

        self.assert_error("Entry.xlsx", "Group", "클라이언트")

    def test_client_folders_are_emptied_on_each_run(self):
        self.add_table("Thing", sheet(*THING, [1, "검", "GEAR", "", 100, 2.0, True, 9, None]))
        for folder in (self.client_json_dir, self.client_source_dir):
            folder.mkdir(parents=True)
            (folder / "Stale.txt").write_text("x", encoding="utf-8")

        with redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
            self.assertEqual(run(self.config_path(), self.root), 0)

        self.assertEqual(sorted(path.name for path in self.client_json_dir.iterdir()), ["Thing.json"])
        self.assertNotIn("Stale.txt", [path.name for path in self.client_source_dir.iterdir()])

    def test_client_folders_must_be_named_generated(self):
        self.add_table("Thing", sheet(*THING, [1, "검", "GEAR", "", 100, 2.0, True, 9, None]))
        path = self.config_path()
        path.write_text(path.read_text(encoding="utf-8").replace("Client/DesignData/Generated", "Client/DesignData"), encoding="utf-8")

        with redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()) as errors:
            self.assertEqual(run(path, self.root), 1)
        self.assertIn("Generated", errors.getvalue())


class ClientRuleTest(ClientOutputCase):
    def test_optional_value_cannot_go_to_the_client(self):
        self.add_table("Thing", sheet(["Id", "Bonus"], ["key:int32", "int32?"], ["both", "client"], [1, None]))
        self.assert_error("Thing.xlsx", "Bonus", "선택")

    def test_setting_field_other_than_number_or_bool_cannot_go_to_the_client(self):
        self.add_setting("Progress", setting_sheet(["StartGold", "int64", "server", 1, None], ["Note", "string", "client", "x", None]))
        self.assert_error("Config.xlsx", "Note", "클라이언트")

    def test_generated_enum_value_none_cannot_go_to_the_client(self):
        self.enums["Mood"] = {"NONE": 1, "HAPPY": 2}
        self.add_table("Thing", sheet(["Id", "Mood"], ["key:int32", "enum:Mood"], ["both", "client"], [1, "HAPPY"]))
        self.assert_error("Mood", "NONE")

    def test_without_client_section_no_client_rules_apply(self):
        self.with_client = False
        self.add_table("Thing", sheet(["Id", "Bonus"], ["key:int32", "int32?"], ["both", "client"], [1, None]))
        self.build_ok()


class ClientHeaderTest(ClientOutputCase):
    def test_row_struct_has_one_property_per_client_column(self):
        self.add_table("Thing", sheet(*THING, [1, "검", "GEAR", "", 100, 2.0, True, 9, None],
            descriptions=["번호", "이름", "종류", "직업", "가격", "속도", "판매", "비밀", "메모"]))

        header = self.client_source()["P1ThingRow.h"]
        self.assertTrue(header.startswith("// DataBuilder가 생성한 파일이다."))
        self.assertIn('#include "P1ThingRow.generated.h"', header)
        self.assertIn("struct FP1ThingRow : public FTableRowBase\n{\n    GENERATED_BODY()\n", header)
        self.assertIn("    /** 번호 */\n    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)\n    int32 Id = 0;\n", header)
        self.assertIn("    FString Name;\n", header)
        self.assertIn("    int64 Price = 0;\n", header)
        self.assertIn("    float Speed = 0.f;\n", header)
        self.assertIn("    bool Sellable = false;\n", header)
        self.assertNotIn("Secret", header)
        self.assertNotIn("Memo", header)

    def test_proto_enum_is_a_value_name_with_a_protocol_getter(self):
        self.add_table("Thing", sheet(*THING, [1, "검", "GEAR", "", 100, 2.0, True, 9, None]))

        files = self.client_source()
        header = files["P1ThingRow.h"]
        self.assertIn('#include "Network/Common.pb.h"', header)
        self.assertIn('#include "Game/Data/Generated/P1DesignDataEnumNames.h"', header)
        self.assertIn("    FString Type;\n", header)
        self.assertIn("    TArray<FString> Classes;\n", header)
        self.assertIn("    Protocol::ItemType GetType() const\n", header)
        self.assertIn("    TArray<Protocol::CharacterClass> GetClasses() const\n", header)
        self.assertNotIn("EP1ItemType", "".join(files.values()), ".proto의 열거형은 클라이언트에 다시 정의하지 않는다")

        names = files["P1DesignDataEnumNames.h"]
        self.assertIn("inline bool ParseEnumName(const FString& Name, Protocol::ItemType& OutValue)", names)
        self.assertIn('if (Name.Equals(TEXT("GEAR"), ESearchCase::CaseSensitive)) { OutValue = Protocol::ITEM_TYPE_GEAR; return true; }', names)
        self.assertIn("inline bool ParseEnumName(const FString& Name, Protocol::CharacterClass& OutValue)", names)

    def test_generated_enum_becomes_a_uenum(self):
        self.add_table("Wallet", sheet(["Id", "Type", "Types"], ["key:int32", "enum:CurrencyType", "enum[]:CurrencyType"], ["both"] * 3, [1, "GOLD", "GOLD;HONOR"]))

        files = self.client_source()
        self.assertIn(
            "UENUM(BlueprintType)\nenum class EP1CurrencyType : uint8\n{\n    None = 0 UMETA(Hidden),\n    GOLD = 1,\n    HONOR = 2,\n};\n",
            files["P1CurrencyType.h"],
        )
        header = files["P1WalletRow.h"]
        self.assertIn('#include "Game/Data/Generated/P1CurrencyType.h"', header)
        self.assertIn("    EP1CurrencyType Type = EP1CurrencyType::None;\n", header)
        self.assertIn("    TArray<EP1CurrencyType> Types;\n", header)
        self.assertNotIn("P1DesignDataEnumNames.h", files, ".proto 열거형을 쓰지 않으면 값 이름 헤더도 없다")

    def test_reference_column_uses_the_key_type(self):
        self.add_table("Thing", sheet(["Id", "Name"], ["key:int32", "string"], ["both", "client"], [10, "검"]))
        self.add_table("Shop", sheet(["Id", "Items", "Main"], ["key:int32", "ref[]:Thing", "ref:Thing"], ["both"] * 3, [1, "10", 10]))

        header = self.client_source()["P1ShopRow.h"]
        self.assertIn("    TArray<int32> Items;\n", header)
        self.assertIn("    int32 Main = 0;\n", header)

    def test_nested_child_is_a_plain_struct_in_its_own_header(self):
        self.add_table("Group", sheet(["Id", "Name"], ["key:int32", "string"], ["both", "client"], [1, "그룹"]))
        self.add_table("Entry", sheet(["GroupId", "ItemId", "Count"], ["key:parent:Group", "key:int32", "int32"], ["both"] * 3, [1, 10, 2]), nest="Entries")

        files = self.client_source()
        self.assertIn("struct FP1EntryRow\n{", files["P1EntryRow.h"])
        self.assertNotIn("GroupId", files["P1EntryRow.h"])
        group = files["P1GroupRow.h"]
        self.assertIn('#include "Game/Data/Generated/P1EntryRow.h"', group)
        self.assertIn("    TArray<FP1EntryRow> Entries;\n", group)

    def test_setting_numbers_and_bools_for_the_client_become_constants(self):
        self.add_setting("Interaction", setting_sheet(
            ["Range", "float", "both", 300, "사거리(cm)"],
            ["MaxTargets", "int32", "client", 2, None],
            ["Enabled", "bool", "client", True, None],
            ["Secret", "int64", "server", 7, None],
        ))

        header = self.client_source()["P1InteractionConfig.h"]
        self.assertIn("namespace P1DesignData::Interaction\n{", header)
        self.assertIn("    /** 사거리(cm) */\n    inline constexpr float Range = 300.0f;\n", header)
        self.assertIn("    inline constexpr int32 MaxTargets = 2;\n", header)
        self.assertIn("    inline constexpr bool Enabled = true;\n", header)
        self.assertNotIn("Secret", header)

    def test_setting_without_client_fields_has_no_client_header(self):
        self.add_setting("Progress", setting_sheet(["StartGold", "int64", "server", 1, None]))
        self.add_table("Thing", sheet(*THING, [1, "검", "GEAR", "", 100, 2.0, True, 9, None]))

        self.assertNotIn("P1ProgressConfig.h", self.client_source())

    def test_version_header_carries_the_data_version(self):
        self.add_table("Thing", sheet(*THING, [1, "검", "GEAR", "", 100, 2.0, True, 9, None]))

        result = self.build_ok()
        self.assertIn(f'inline constexpr const TCHAR* Version = TEXT("{result.version}");', result.outputs[self.client_source_dir]["P1DesignDataVersion.h"])


if __name__ == "__main__":
    unittest.main()
