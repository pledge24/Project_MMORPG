"""NPC 표와 상점 표의 규칙."""

from __future__ import annotations

import json
import unittest

from tests.helpers import DesignDataCase, setting_sheet, sheet

ITEM = (["Id", "Type", "MaxStack"], ["key:int32", "enum:ItemType", "int32"], ["both", "both", "both"])
SHOP = (["Id", "Items"], ["key:int32", "ref[]:Item"], ["both", "both"])
NPC = (["Id", "Name", "ShopId"], ["key:int32", "string", "ref:Shop"], ["both", "client", "both"])


class NpcRuleCase(DesignDataCase):
    def setUp(self) -> None:
        super().setUp()
        self.add_table("Item", sheet(*ITEM, [3001, "MISCELLANEOUS", 99], [3002, "MISCELLANEOUS", 99]))

    def add_shops(self, *rows: list) -> None:
        self.add_table("Shop", sheet(*SHOP, *rows))

    def add_npcs(self, *rows: list) -> None:
        self.add_table("Npc", sheet(*NPC, *rows))


class NpcTableTest(NpcRuleCase):
    def test_npc_with_shop_selling_known_items_passes(self):
        self.add_shops([1, "3001;3002"])
        self.add_npcs([100, "상인", 1])

        npcs = json.loads(self.output("Npc.json"))
        self.assertEqual(npcs, [{"Id": 100, "ShopId": 1}])

    def test_npc_referring_unknown_shop_is_rejected(self):
        self.add_shops([1, "3001"])
        self.add_npcs([100, "상인", 2])
        self.assert_error("Npc.xlsx", "5행", "ShopId", "2")


class ShopTableTest(NpcRuleCase):
    def test_shop_selling_unknown_item_is_rejected(self):
        self.add_shops([1, "3001;9999"])
        self.add_npcs([100, "상인", 1])
        self.assert_error("Shop.xlsx", "5행", "Items", "9999")

    def test_shop_without_items_is_rejected(self):
        self.add_shops([1, None])
        self.add_npcs([100, "상인", 1])
        self.assert_error("Shop.xlsx", "5행", "Items", "비어")

    def test_shop_listing_same_item_twice_is_rejected(self):
        self.add_shops([1, "3001;3001"])
        self.add_npcs([100, "상인", 1])
        self.assert_error("Shop.xlsx", "5행", "Items", "3001")


class InteractionSettingTest(NpcRuleCase):
    def test_interaction_range_must_be_positive(self):
        self.add_setting("Interaction", setting_sheet(["InteractionRange", "float", "both", 0, None]))
        self.assert_error("Config.xlsx", "InteractionRange", "0보다")


if __name__ == "__main__":
    unittest.main()
