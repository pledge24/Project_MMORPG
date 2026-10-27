"""맵, 지역, 포털 표의 규칙."""

from __future__ import annotations

import json
import unittest

from tests.helpers import DesignDataCase, sheet

MONSTER = (
    ["Id", "MinExp", "MaxExp", "MinGold", "MaxGold"],
    ["key:int32", "int64", "int64", "int64", "int64"],
    ["both", "server", "server", "server", "server"],
)
MAP = (["Id", "Name"], ["key:int32", "string"], ["both", "client"])
AREA = (
    ["Id", "MapId", "Name", "HasRespawnPoint", "Monsters", "MaxMonsterCount", "MonsterRespawnSec",
     "CenterX", "CenterY", "CenterZ", "HalfExtentX", "HalfExtentY", "RespawnX", "RespawnY", "RespawnZ"],
    ["key:int32", "ref:Map", "string", "bool", "ref[]:Monster", "int32", "int32",
     "float", "float", "float", "float", "float", "float?", "float?", "float?"],
    ["both", "both", "client", "both", "server", "server", "server",
     "server", "server", "server", "server", "server", "server", "server", "server"],
)
PORTAL = (
    ["Id", "AreaId", "SrcX", "SrcY", "SrcZ", "Radius", "DestAreaId", "DestX", "DestY", "DestZ", "DestYaw"],
    ["key:int32", "parent:Area", "float", "float", "float", "float", "ref:Area", "float", "float", "float", "float"],
    ["both", "both", "server", "server", "server", "server", "server", "server", "server", "server", "server"],
)


def town(area_id: int = 10, map_id: int = 1111) -> list:
    return [area_id, map_id, "마을", True, None, 0, 0, 0, 0, 0, 1500, 1500, 0, 0, 0]


def field(area_id: int = 20, map_id: int = 1111, monsters: str | None = "5000", half_extent: float = 2500) -> list:
    return [area_id, map_id, "초급 사냥터", False, monsters, 10, 10, -10000, 10000, 0, half_extent, half_extent, None, None, None]


def portal(portal_id: int = 11, area_id: int = 10, dest_area_id: int = 20, radius: float = 500) -> list:
    return [portal_id, area_id, -1200, 1000, 0, radius, dest_area_id, -8000, 10000, 0, 180]


class MapRuleCase(DesignDataCase):
    def setUp(self) -> None:
        super().setUp()
        self.add_table("Monster", sheet(*MONSTER, [5000, 1, 2, 1, 2]))
        self.add_table("Map", sheet(*MAP, [1111, "시작의 섬"]))

    def add_areas(self, *rows: list) -> None:
        self.add_table("Area", sheet(*AREA, *rows))

    def add_portals(self, *rows: list) -> None:
        self.add_table("Portal", sheet(*PORTAL, *rows), nest="Portals")


class AreaTableTest(MapRuleCase):
    def test_town_and_field_with_portals_pass(self):
        self.add_areas(town(), field())
        self.add_portals(portal(11, 10, 20), portal(21, 20, 10))
        self.build_ok()

    def test_portals_nest_under_their_area(self):
        self.add_areas(town(), field())
        self.add_portals(portal(11, 10, 20), portal(21, 20, 10))
        areas = json.loads(self.output("Area.json"))
        self.assertEqual([p["Id"] for p in areas[0]["Portals"]], [11])
        self.assertEqual([p["Id"] for p in areas[1]["Portals"]], [21])
        self.assertNotIn("Portal.json", self.build_ok().outputs[self.out_dir])

    def test_unknown_map_is_rejected(self):
        self.add_areas(town(), field(map_id=2222))
        self.add_portals()
        self.assert_error("Area.xlsx", "6행", "MapId", "2222")

    def test_unknown_monster_is_rejected(self):
        self.add_areas(town(), field(monsters="5000;5999"))
        self.add_portals()
        self.assert_error("Area.xlsx", "6행", "Monsters", "5999")

    def test_zero_half_extent_is_rejected(self):
        self.add_areas(town(), field(half_extent=0))
        self.add_portals()
        self.assert_error("Area.xlsx", "6행", "HalfExtentX", "0보다 커야")

    def test_negative_max_monster_count_is_rejected(self):
        row = field()
        row[5] = -1
        self.add_areas(town(), row)
        self.add_portals()
        self.assert_error("Area.xlsx", "6행", "MaxMonsterCount", "-1")

    def test_negative_monster_respawn_time_is_rejected(self):
        row = field()
        row[6] = -1
        self.add_areas(town(), row)
        self.add_portals()
        self.assert_error("Area.xlsx", "6행", "MonsterRespawnSec", "-1")


class TownRuleTest(MapRuleCase):
    def setUp(self) -> None:
        super().setUp()
        self.add_portals()

    def test_no_area_with_respawn_point_is_rejected(self):
        self.add_areas(field())
        self.assert_error("Area.xlsx", "HasRespawnPoint", "하나여야", "0개")

    def test_two_areas_with_respawn_point_are_rejected(self):
        self.add_areas(town(10), town(30))
        self.assert_error("Area.xlsx", "HasRespawnPoint", "하나여야", "2개", "5행", "6행")

    def test_respawn_point_without_coordinates_is_rejected(self):
        row = town()
        row[12] = None
        self.add_areas(row, field())
        self.assert_error("Area.xlsx", "5행", "RespawnX")

    def test_coordinates_without_respawn_point_are_rejected(self):
        row = field()
        row[12:15] = [0, 0, 0]
        self.add_areas(town(), row)
        self.assert_error("Area.xlsx", "6행", "리스폰 지점이 없는", "RespawnX")


class PortalTableTest(MapRuleCase):
    def setUp(self) -> None:
        super().setUp()
        self.add_areas(town(), field())

    def test_unknown_destination_area_is_rejected(self):
        self.add_portals(portal(11, 10, 99))
        self.assert_error("Portal.xlsx", "5행", "DestAreaId", "99")

    def test_unknown_source_area_is_rejected(self):
        self.add_portals(portal(11, 99, 20))
        self.assert_error("Portal.xlsx", "5행", "AreaId", "99")

    def test_zero_radius_is_rejected(self):
        self.add_portals(portal(radius=0))
        self.assert_error("Portal.xlsx", "5행", "Radius", "0보다 커야")

    def test_negative_radius_is_rejected(self):
        self.add_portals(portal(radius=-5))
        self.assert_error("Portal.xlsx", "5행", "Radius", "-5")


if __name__ == "__main__":
    unittest.main()
