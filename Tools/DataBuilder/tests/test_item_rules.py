"""아이템 표 세 개(공통, 장비 능력치, 소모 효과)의 규칙."""

from __future__ import annotations

import unittest

from tests.helpers import DesignDataCase, sheet

ITEM = (
    ["Id", "Name", "Type", "MaxStack"],
    ["key:int32", "string", "enum:ItemType", "int32"],
    ["both", "client", "both", "both"],
)
EQUIP_STAT = (["ItemId", "Hp"], ["key:parent:Item", "int32"], ["both", "both"])
CONSUME_EFFECT = (["ItemId", "HpRestoreRatio", "CooldownMs"], ["key:parent:Item", "float", "int32"], ["both", "both", "both"])


class ItemRuleCase(DesignDataCase):
    def add_items(self, *rows: list) -> None:
        self.add_table("Item", sheet(*ITEM, *rows))

    def add_equip_stats(self, *item_ids: int) -> None:
        self.add_table("ItemEquipStat", sheet(*EQUIP_STAT, *[[item_id, 10] for item_id in item_ids]))

    def add_consume_effects(self, *item_ids: int) -> None:
        self.add_table("ItemConsumeEffect", sheet(*CONSUME_EFFECT, *[[item_id, 0.3, 10000] for item_id in item_ids]))


class ItemKindTableTest(ItemRuleCase):
    def test_kind_tables_that_reference_their_own_kind_pass(self):
        self.add_items([1000, "검", "GEAR", 1], [2000, "물약", "CONSUMABLE", 10])
        self.add_equip_stats(1000)
        self.add_consume_effects(2000)
        self.build_ok()

    def test_equip_stat_for_consumable_is_rejected(self):
        self.add_items([1000, "검", "GEAR", 1], [2000, "물약", "CONSUMABLE", 10])
        self.add_equip_stats(1000, 2000)
        self.add_consume_effects(2000)
        self.assert_error("ItemEquipStat.xlsx", "6행", "2000", "GEAR", "CONSUMABLE")

    def test_consume_effect_for_gear_is_rejected(self):
        self.add_items([1000, "검", "GEAR", 1], [2000, "물약", "CONSUMABLE", 10])
        self.add_equip_stats(1000)
        self.add_consume_effects(1000, 2000)
        self.assert_error("ItemConsumeEffect.xlsx", "5행", "1000", "CONSUMABLE", "GEAR")

    def test_gear_without_equip_stat_is_rejected(self):
        self.add_items([1000, "검", "GEAR", 1], [1001, "투구", "GEAR", 1])
        self.add_equip_stats(1000)
        self.add_consume_effects()
        self.assert_error("Item.xlsx", "6행", "1001", "ItemEquipStat")

    def test_consumable_without_consume_effect_is_rejected(self):
        self.add_items([2000, "물약", "CONSUMABLE", 10])
        self.add_equip_stats()
        self.add_consume_effects()
        self.assert_error("Item.xlsx", "5행", "2000", "ItemConsumeEffect")

    def test_miscellaneous_needs_no_kind_table(self):
        self.add_items([3000, "돌멩이", "MISCELLANEOUS", 99])
        self.add_equip_stats()
        self.add_consume_effects()
        self.build_ok()


class ItemKeyRangeTest(ItemRuleCase):
    def test_keys_at_both_ends_of_each_range_pass(self):
        self.add_items(
            [1000, "검", "GEAR", 1], [1999, "검2", "GEAR", 1],
            [2000, "물약", "CONSUMABLE", 10], [2999, "물약2", "CONSUMABLE", 10],
            [3000, "돌", "MISCELLANEOUS", 1], [3999, "돌2", "MISCELLANEOUS", 1],
        )
        self.add_equip_stats(1000, 1999)
        self.add_consume_effects(2000, 2999)
        self.build_ok()

    def test_key_outside_its_kind_range_is_rejected(self):
        self.add_items([1000, "검", "GEAR", 1], [2000, "잘못 놓인 검", "GEAR", 1])
        self.add_equip_stats(1000, 2000)
        self.add_consume_effects()
        self.assert_error("Item.xlsx", "6행", "2000", "GEAR", "1000~1999")

    def test_key_below_every_range_is_rejected(self):
        self.add_items([999, "돌", "MISCELLANEOUS", 1])
        self.add_equip_stats()
        self.add_consume_effects()
        self.assert_error("Item.xlsx", "5행", "999", "3000~3999")


class ItemMaxStackTest(ItemRuleCase):
    def test_max_stack_below_one_is_rejected(self):
        self.add_items([3000, "돌", "MISCELLANEOUS", 0])
        self.add_equip_stats()
        self.add_consume_effects()
        self.assert_error("Item.xlsx", "5행", "MaxStack", "0")


class ItemCooldownTest(ItemRuleCase):
    def add_potion_with_cooldown(self, cooldown_ms: int) -> None:
        self.add_items([2000, "물약", "CONSUMABLE", 10])
        self.add_equip_stats()
        self.add_table("ItemConsumeEffect", sheet(*CONSUME_EFFECT, [2000, 0.3, cooldown_ms]))

    def test_zero_cooldown_passes(self):
        self.add_potion_with_cooldown(0)
        self.build_ok()

    def test_negative_cooldown_is_rejected(self):
        self.add_potion_with_cooldown(-1)
        self.assert_error("ItemConsumeEffect.xlsx", "5행", "CooldownMs", "-1")


if __name__ == "__main__":
    unittest.main()
