"""서버의 navmesh를 지금 연 레벨 위에 겹쳐 그리는 에디터 메뉴.

에디터 메인 메뉴의 「Tools > P1 > 서버 navmesh 겹쳐 그리기」가 `draw()`를, 「서버 navmesh 지우기」가 `clear()`를 부른다.
`Config/DefaultEngine.ini`의 Python 시작 스크립트가 에디터를 띄울 때 `register_menu()`를 부른다.

그리는 것은 navmesh 빌드 도구(Tools/NavBuilder)가 서버의 navmesh 파일에서 꺼낸 폴리곤이다(`Saved/NavExtract/NavMeshPolygons_<맵 번호>.json`).
추출 스크립트(`Scripts/Invoke-PlacementExtract.ps1`)를 돌려야 생긴다. 파일마다 실린 레벨 경로로 지금 연 레벨의 파일을 고른다.
선은 지울 때까지 남는다.
"""

from __future__ import annotations

import json
import os

import unreal

POLYGON_DIR = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "NavExtract")
POLYGON_PREFIX = "NavMeshPolygons_"

MENU_NAME = "LevelEditor.MainMenu.Tools"
SECTION_NAME = "P1"

# 바닥과 겹쳐 깜박이지 않도록 조금 띄운다(cm). navmesh는 바닥보다 복셀 한두 칸(5~10cm) 위에 있다.
DRAW_LIFT = 5.0
EDGE_COLOR = unreal.LinearColor(0.0, 1.0, 0.3, 1.0)
EDGE_THICKNESS = 2.0
# 지울 때까지 남긴다(초).
DRAW_DURATION = 1.0e7


def editor_world() -> unreal.World | None:
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()


def find_polygon_file(world: unreal.World) -> str | None:
    """지금 레벨의 폴리곤 파일. 빌드 도구가 파일마다 레벨의 패키지 경로를 싣는다. 없으면 None."""
    level_path = world.get_path_name().split(".")[0]
    if not os.path.isdir(POLYGON_DIR):
        return None

    for name in sorted(os.listdir(POLYGON_DIR)):
        if not name.startswith(POLYGON_PREFIX):
            continue
        path = os.path.join(POLYGON_DIR, name)
        with open(path, encoding="utf-8") as file:
            if json.load(file).get("Level") == level_path:
                return path
    return None


def draw() -> bool:
    """지금 레벨의 서버 navmesh 폴리곤 테두리를 그린다. 그리기 전에 지난 선을 지운다."""
    world = editor_world()
    if world is None:
        unreal.log_error("[서버 navmesh] 에디터 월드가 없다")
        return False

    path = find_polygon_file(world)
    if path is None:
        unreal.log_error(f"[서버 navmesh] {POLYGON_DIR}에 {world.get_name()}의 폴리곤 파일이 없다. 추출 스크립트(Scripts/Invoke-PlacementExtract.ps1)를 돌린다")
        return False

    with open(path, encoding="utf-8") as file:
        document = json.load(file)
    map_id = document["MapId"]
    polygons = document["Polygons"]

    unreal.SystemLibrary.flush_persistent_debug_lines(world)
    edges = 0
    for polygon in polygons:
        points = [unreal.Vector(x, y, z + DRAW_LIFT) for x, y, z in polygon]
        for i, start in enumerate(points):
            end = points[(i + 1) % len(points)]
            unreal.SystemLibrary.draw_debug_line(world, start, end, EDGE_COLOR, DRAW_DURATION, EDGE_THICKNESS)
            edges += 1

    unreal.log(f"[서버 navmesh] 맵 {map_id}: 폴리곤 {len(polygons)}개, 변 {edges}개를 그렸다 ({path})")
    return True


def clear() -> None:
    """겹쳐 그린 선을 지운다."""
    world = editor_world()
    if world is not None:
        unreal.SystemLibrary.flush_persistent_debug_lines(world)
        unreal.log("[서버 navmesh] 겹쳐 그린 선을 지웠다")


def register_menu() -> None:
    """에디터 메인 메뉴의 Tools에 겹쳐 그리기와 지우기를 더한다. 두 번 불러도 항목은 하나씩이다."""
    menus = unreal.ToolMenus.get()
    menu = menus.extend_menu(MENU_NAME)
    if menu is None:
        unreal.log_warning(f"[서버 navmesh] 메뉴 {MENU_NAME}가 없어 메뉴를 더하지 않았다")
        return

    menu.add_section(SECTION_NAME, "P1")
    entries = [
        ("P1DrawServerNavMesh", "서버 navmesh 겹쳐 그리기", "서버가 읽는 navmesh의 폴리곤을 지금 연 레벨 위에 그린다", "draw()"),
        ("P1ClearServerNavMesh", "서버 navmesh 지우기", "겹쳐 그린 navmesh 선을 지운다", "clear()"),
    ]
    for name, label, tool_tip, call in entries:
        entry = unreal.ToolMenuEntry(name=name, type=unreal.MultiBlockType.MENU_ENTRY)
        entry.set_label(label)
        entry.set_tool_tip(tool_tip)
        entry.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", f"import p1_server_navmesh; p1_server_navmesh.{call}")
        menu.add_menu_entry(SECTION_NAME, entry)
    menus.refresh_all_widgets()
