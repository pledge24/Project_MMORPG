"""액션, 타격, 직업 표의 규칙."""

from __future__ import annotations

import json
import unittest

from tests.helpers import DesignDataCase, sheet

ACTION = (
    ["Id", "Name", "DamageType", "DamageCoefficient", "NextAction", "TargetRule"],
    ["key:int32", "string", "enum:DamageType", "float", "ref?:Action", "enum:ActionTarget"],
    ["both", "client", "server", "server", "server", "server"],
)
ACTION_HIT = (
    ["ActionId", "HitIndex", "Shape", "Radius", "Angle", "Length", "Width", "OffsetX", "OffsetY", "ZMin", "ZMax"],
    ["key:parent:Action", "key:int32", "enum:AttackAreaShape", "float", "float", "float", "float", "float", "float", "float", "float"],
    ["both", "both", "both", "both", "both", "both", "both", "both", "both", "both", "both"],
)
CLASS = (
    ["Class", "NormalAttackAction"],
    ["key:enum:CharacterClass", "ref:Action"],
    ["server", "server"],
)


def hit(action_id: int, hit_index: int, shape: str, **values: float) -> list:
    """타격 행 하나. 적지 않은 형상 값과 기준점은 0, 높이 범위는 -100~200이다."""
    row = {"Radius": 0, "Angle": 0, "Length": 0, "Width": 0, "OffsetX": 0, "OffsetY": 0, "ZMin": -100, "ZMax": 200, **values}
    return [action_id, hit_index, shape, *(row[name] for name in ACTION_HIT[0][3:])]


def circle(action_id: int, hit_index: int = 0, radius: float = 150.0, **values: float) -> list:
    return hit(action_id, hit_index, "CIRCLE", Radius=radius, **values)


def sector(action_id: int, hit_index: int = 0, radius: float = 150.0, angle: float = 120.0) -> list:
    return hit(action_id, hit_index, "SECTOR", Radius=radius, Angle=angle)


def rectangle(action_id: int, hit_index: int = 0, length: float = 600.0, width: float = 80.0) -> list:
    return hit(action_id, hit_index, "RECTANGLE", Length=length, Width=width)


class ActionRuleCase(DesignDataCase):
    def setUp(self) -> None:
        super().setUp()
        self.enums["DamageType"] = {"PHYSICAL": 1, "MAGICAL": 2}
        self.enums["ActionTarget"] = {"ENEMY": 1, "ALLY": 2}
        self.enums["AttackAreaShape"] = {"CIRCLE": 1, "SECTOR": 2, "RECTANGLE": 3}
        self.add_actions([1, "베기", "PHYSICAL", 1.0, 2, "ENEMY"], [2, "올려베기", "PHYSICAL", 1.5, None, "ENEMY"])

    def add_actions(self, *rows: list) -> None:
        self.add_table("Action", sheet(*ACTION, *rows))

    def add_hits(self, *rows: list) -> None:
        self.add_table("ActionHit", sheet(*ACTION_HIT, *rows), nest="Hits")


class ActionTest(ActionRuleCase):
    def test_every_shape_passes(self):
        self.add_hits(circle(1), sector(1, 1), rectangle(2))
        self.build_ok()

    def test_hits_nest_under_their_action_in_hit_order(self):
        self.add_hits(sector(1, 1, radius=200), circle(1, 0), rectangle(2))
        actions = json.loads(self.output("Action.json"))
        self.assertEqual([(a["Id"], [h["HitIndex"] for h in a["Hits"]]) for a in actions], [(1, [0, 1]), (2, [0])])
        self.assertEqual(actions[0]["NextAction"], 2)
        self.assertIsNone(actions[1]["NextAction"])
        self.assertNotIn("Name", actions[0])

    def test_unknown_next_action_is_rejected(self):
        self.add_actions([1, "베기", "PHYSICAL", 1.0, 9, "ENEMY"])
        self.add_hits(circle(1))
        self.assert_error("Action.xlsx", "5행", "NextAction", "9")

    def test_action_without_hit_is_rejected(self):
        self.add_hits(circle(1))
        self.assert_error("Action.xlsx", "6행", "액션 2", "ActionHit")

    def test_negative_damage_coefficient_is_rejected(self):
        self.add_actions([1, "베기", "PHYSICAL", -0.5, None, "ENEMY"])
        self.add_hits(circle(1))
        self.assert_error("Action.xlsx", "5행", "DamageCoefficient", "-0.5")

    def test_zero_damage_coefficient_passes(self):
        self.add_actions([1, "베기", "PHYSICAL", 0.0, None, "ENEMY"])
        self.add_hits(circle(1))
        self.build_ok()


class ActionHitTest(ActionRuleCase):
    def test_hit_index_not_starting_at_zero_is_rejected(self):
        self.add_hits(circle(1, 1), circle(2))
        self.assert_error("ActionHit.xlsx", "5행", "액션 1", "HitIndex", "0")

    def test_gap_in_hit_index_is_rejected(self):
        self.add_hits(circle(1, 0), circle(1, 2), circle(2))
        self.assert_error("ActionHit.xlsx", "6행", "액션 1", "HitIndex", "1", "2")

    def test_zero_circle_radius_is_rejected(self):
        self.add_hits(circle(1, radius=0), circle(2))
        self.assert_error("ActionHit.xlsx", "5행", "CIRCLE", "Radius", "0")

    def test_sector_needs_positive_angle_up_to_full_turn(self):
        self.add_hits(sector(1, angle=0), sector(2, angle=361))
        errors = self.assert_error("ActionHit.xlsx", "5행", "SECTOR", "Angle", "0")
        self.assertTrue(any("6행" in error and "361" in error for error in errors), "\n".join(errors))

    def test_full_turn_sector_passes(self):
        self.add_hits(sector(1, angle=360), circle(2))
        self.build_ok()

    def test_negative_rectangle_width_is_rejected(self):
        self.add_hits(rectangle(1, width=-10), circle(2))
        self.assert_error("ActionHit.xlsx", "5행", "RECTANGLE", "Width", "-10")

    def test_value_not_used_by_shape_must_be_zero(self):
        self.add_hits(circle(1, Length=300), circle(2))
        self.assert_error("ActionHit.xlsx", "5행", "CIRCLE", "Length", "0")

    def test_height_range_must_not_be_empty(self):
        self.add_hits(circle(1, ZMin=200, ZMax=200), circle(2))
        self.assert_error("ActionHit.xlsx", "5행", "ZMin", "ZMax")


class ClassTest(ActionRuleCase):
    def setUp(self) -> None:
        super().setUp()
        self.add_hits(circle(1), circle(2))

    def test_class_points_at_first_combo_action(self):
        self.add_table("Class", sheet(*CLASS, ["WARRIOR", 1]))
        classes = json.loads(self.output("Class.json"))
        self.assertEqual(classes, [{"Class": "WARRIOR", "NormalAttackAction": 1}])

    def test_unknown_normal_attack_action_is_rejected(self):
        self.add_table("Class", sheet(*CLASS, ["WARRIOR", 7]))
        self.assert_error("Class.xlsx", "5행", "NormalAttackAction", "7")


if __name__ == "__main__":
    unittest.main()
