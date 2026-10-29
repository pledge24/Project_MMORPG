"""맵, 지역, 포털 표의 규칙."""

from __future__ import annotations

import json
import unittest

from tests.helpers import DesignDataCase, sheet

MAP = (["Id", "Name"], ["key:int32", "string"], ["both", "client"])
# 공간 열(중심, 반경, 리스폰 지점, 포털 좌표)은 없다. 레벨의 마커에서 추출한다.
# 몬스터 구성과 수, 리젠 시간은 스폰 그룹 표에 있다.
AREA = (
    ["Id", "MapId", "Name", "HasRespawnPoint"],
    ["key:int32", "ref:Map", "string", "bool"],
    ["both", "both", "client", "both"],
)
PORTAL = (
    ["Id", "AreaId", "DestAreaId"],
    ["key:int32", "parent:Area", "ref:Area"],
    ["both", "both", "both"],
)


def town(area_id: int = 10, map_id: int = 1111) -> list:
    return [area_id, map_id, "마을", True]


def field(area_id: int = 20, map_id: int = 1111) -> list:
    return [area_id, map_id, "초급 사냥터", False]


def portal(portal_id: int = 11, area_id: int = 10, dest_area_id: int = 20) -> list:
    return [portal_id, area_id, dest_area_id]


class MapRuleCase(DesignDataCase):
    def setUp(self) -> None:
        super().setUp()
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


if __name__ == "__main__":
    unittest.main()
