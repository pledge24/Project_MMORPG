"""DataBuilder의 클라이언트 JSON으로 기획 데이터 테이블을 채우는 에디터 메뉴.

에디터 메인 메뉴의 「Tools > P1 > 기획 데이터 테이블 채우기」가 `fill_all()`을 부른다.
`Config/DefaultEngine.ini`의 Python 시작 스크립트가 에디터를 띄울 때 `register_menu()`를 부른다.

`DesignData/Generated/<표>.json` 하나가 데이터 테이블 `/Game/P1/Data/DesignData/DT_<표>` 하나다. 행 구조체는 `FP1<표>Row`다.
테이블이 없으면 그 구조체로 만들고, 있으면 비우고 다시 채운다. 원본에서 지운 행은 테이블에도 남지 않는다.
행 이름은 JSON 각 행의 `RowName`(기획 데이터의 키)이다. 채운 테이블은 저장한다.
"""

from __future__ import annotations

import os

import unreal

JSON_DIR = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "DesignData", "Generated")
ASSET_DIR = "/Game/P1/Data/DesignData"
# DataBuilder가 클라이언트 JSON의 각 행 첫 필드로 싣는 행 이름.
ROW_NAME_FIELD = "RowName"

MENU_NAME = "LevelEditor.MainMenu.Tools"
SECTION_NAME = "P1"


def row_struct(table_name: str):
    """행 구조체 FP1<표>Row. 클라이언트를 빌드하지 않아 없으면 None."""
    return unreal.load_object(None, f"/Script/P1.P1{table_name}Row")


def find_or_create_table(table_name: str, struct) -> unreal.DataTable | None:
    assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    asset_path = f"{ASSET_DIR}/DT_{table_name}"
    if assets.does_asset_exist(asset_path):
        table = assets.load_asset(asset_path)
        if not isinstance(table, unreal.DataTable):
            unreal.log_error(f"[기획 데이터] {asset_path}가 데이터 테이블이 아니다")
            return None
        return table

    factory = unreal.DataTableFactory()
    factory.set_editor_property("struct", struct)
    table = unreal.AssetToolsHelpers.get_asset_tools().create_asset(f"DT_{table_name}", ASSET_DIR, unreal.DataTable, factory)
    if table is None:
        unreal.log_error(f"[기획 데이터] {asset_path}를 만들지 못했다")
    else:
        unreal.log(f"[기획 데이터] {asset_path}를 만들었다")
    return table


def fill_table(json_path: str) -> bool:
    """JSON 하나로 그 표의 데이터 테이블을 비우고 다시 채운 뒤 저장한다. 실패하면 False."""
    table_name = os.path.splitext(os.path.basename(json_path))[0]
    struct = row_struct(table_name)
    if struct is None:
        unreal.log_error(f"[기획 데이터] 행 구조체 FP1{table_name}Row가 없다. 클라이언트를 빌드한 뒤 다시 실행한다")
        return False

    table = find_or_create_table(table_name, struct)
    if table is None:
        return False

    # 행 이름을 열 이름 대신 RowName에서 읽는다. 아이템 표처럼 Name 열이 있는 표도 키로 행 이름을 정한다.
    table.set_editor_property("import_key_field", ROW_NAME_FIELD)
    # 이 값이 참이면 임포터가 지난 행을 남긴다. 원본에서 지운 행이 테이블에서도 사라지게 끈다.
    table.set_editor_property("preserve_existing_values", False)
    # 엔진이 테이블을 비운 뒤 채우므로 원본에서 지운 행이 남지 않는다. 실패하면 이유를 출력 로그에 남긴다.
    if not unreal.DataTableFunctionLibrary.fill_data_table_from_json_file(table, json_path, struct):
        unreal.log_error(f"[기획 데이터] {json_path}로 {table.get_path_name()}를 채우지 못했다. 위의 로그에 이유가 있다")
        return False

    if not unreal.get_editor_subsystem(unreal.EditorAssetSubsystem).save_loaded_asset(table, only_if_is_dirty=False):
        unreal.log_error(f"[기획 데이터] {table.get_path_name()}를 저장하지 못했다")
        return False

    row_count = len(unreal.DataTableFunctionLibrary.get_data_table_row_names(table))
    unreal.log(f"[기획 데이터] {table.get_path_name()} ← {os.path.basename(json_path)} ({row_count}행)")
    return True


def fill_all() -> bool:
    """클라이언트 JSON마다 데이터 테이블을 채운다. 하나라도 실패하면 False."""
    if not os.path.isdir(JSON_DIR):
        unreal.log_error(f"[기획 데이터] {JSON_DIR}가 없다. pwsh Tools/DataBuilder/Invoke-DataBuilder.ps1을 먼저 돌린다")
        return False

    paths = [os.path.join(JSON_DIR, name) for name in sorted(os.listdir(JSON_DIR)) if name.endswith(".json")]
    failures = [path for path in paths if not fill_table(path)]
    if failures:
        unreal.log_error(f"[기획 데이터] 테이블 {len(paths)}개 중 {len(failures)}개를 채우지 못했다")
        return False

    unreal.log(f"[기획 데이터] 테이블 {len(paths)}개를 채웠다")
    return True


def register_menu() -> None:
    """에디터 메인 메뉴의 Tools에 「기획 데이터 테이블 채우기」를 더한다. 두 번 불러도 항목은 하나다."""
    menus = unreal.ToolMenus.get()
    menu = menus.extend_menu(MENU_NAME)
    if menu is None:
        unreal.log_warning(f"[기획 데이터] 메뉴 {MENU_NAME}가 없어 메뉴를 더하지 않았다")
        return

    entry = unreal.ToolMenuEntry(name="P1FillDesignDataTables", type=unreal.MultiBlockType.MENU_ENTRY)
    entry.set_label("기획 데이터 테이블 채우기")
    entry.set_tool_tip("P1/DesignData/Generated의 JSON으로 /Game/P1/Data/DesignData의 데이터 테이블을 만들거나 비우고 다시 채운다")
    entry.set_string_command(unreal.ToolMenuStringCommandType.PYTHON, "", "import p1_design_data_tables; p1_design_data_tables.fill_all()")
    menu.add_section(SECTION_NAME, "P1")
    menu.add_menu_entry(SECTION_NAME, entry)
    menus.refresh_all_widgets()
