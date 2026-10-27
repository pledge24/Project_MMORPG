"""표 하나에만 걸리는 기획 데이터 규칙. 시트 규칙과 타입으로 나타낼 수 없는 의미를 검사한다.

코드가 정한 제약(DB 열 크기, 로비와 게임의 대조)은 여기 두지 않는다. 그 제약은 서버가 기동할 때 검사한다.
규칙은 표 이름으로 찾는다. 표가 설정 파일에 없으면 규칙도 돌지 않는다.
"""

from __future__ import annotations

from collections.abc import Callable

from databuilder.workbooks import DesignData, Table, locate, table_keys


def exactly_one_gold(table: Table, _: DesignData) -> list[str]:
    """서버는 재화 종류로 골드를 찾으므로 골드인 재화가 하나로 정해져야 한다."""
    gold_rows = [row for row in table.rows if row.values.get("Type") == "GOLD"]
    if len(gold_rows) == 1:
        return []
    places = ", ".join(f"시트 '{row.sheet}' {row.number}행" for row in gold_rows)
    return [f"{table.config.workbook}: Type이 GOLD인 행이 하나여야 하는데 {len(gold_rows)}개다" + (f" ({places})" if places else "")]


# 아이템 번호의 범위 관례. 번호만 보고도 종류를 알 수 있게 한다.
ITEM_KEY_RANGES = {"GEAR": (1000, 1999), "CONSUMABLE": (2000, 2999), "MISCELLANEOUS": (3000, 3999)}

# 종류별 표 → 그 표가 참조할 수 있는 아이템 종류. 그 종류의 아이템은 이 표에 행이 있어야 한다.
ITEM_KIND_TABLES = {"ItemEquipStat": "GEAR", "ItemConsumeEffect": "CONSUMABLE"}


def item_key_in_kind_range(table: Table, _: DesignData) -> list[str]:
    errors = []
    for row in table.rows:
        item_id, item_type = row.values["Id"], row.values["Type"]
        low, high = ITEM_KEY_RANGES[item_type]
        if not low <= item_id <= high:
            errors.append(f"{locate(table.config.workbook, row.sheet, row.number)}: 아이템 {item_id}은 {item_type}라서 번호가 {low}~{high}여야 한다")
    return errors


def item_max_stack_at_least_one(table: Table, _: DesignData) -> list[str]:
    return [
        f"{locate(table.config.workbook, row.sheet, row.number)}: MaxStack이 1 이상이어야 하는데 {row.values['MaxStack']}이다"
        for row in table.rows
        if row.values["MaxStack"] < 1
    ]


def item_has_kind_row(table: Table, data: DesignData) -> list[str]:
    """장비는 착용 부위를, 소모품은 사용 효과를 종류별 표에서 얻으므로 행이 빠지면 쓸 수 없는 아이템이 된다."""
    errors = []
    for kind_table_name, item_type in ITEM_KIND_TABLES.items():
        kind_table = data.tables.get(kind_table_name)
        known = table_keys(kind_table) if kind_table is not None else set()
        for row in table.rows:
            if row.values["Type"] == item_type and row.values["Id"] not in known:
                errors.append(f"{locate(table.config.workbook, row.sheet, row.number)}: {item_type} 아이템 {row.values['Id']}의 행이 표 '{kind_table_name}'에 없다")
    return errors


def cooldown_not_negative(table: Table, _: DesignData) -> list[str]:
    """서버는 마지막 사용 시각에 대기를 더해 비교하므로 음수 대기는 끝나지 않는 대기가 된다. 대기가 없으면 0을 적는다."""
    return [
        f"{locate(table.config.workbook, row.sheet, row.number)}: CooldownMs가 0 이상이어야 하는데 {row.values['CooldownMs']}이다"
        for row in table.rows
        if row.values["CooldownMs"] < 0
    ]


def references_item_of_kind(item_type: str) -> Callable[[Table, DesignData], list[str]]:
    """종류별 표가 그 종류의 아이템만 참조하는지 본다."""

    def rule(table: Table, data: DesignData) -> list[str]:
        items = data.tables.get("Item")
        if items is None:
            return [f"{table.config.workbook}: 아이템 종류를 확인할 표 'Item'이 설정 파일에 없다"]
        types = {row.values["Id"]: row.values["Type"] for row in items.rows}
        errors = []
        for row in table.rows:
            item_id = row.values["ItemId"]
            if types[item_id] != item_type:
                errors.append(f"{locate(table.config.workbook, row.sheet, row.number)}: 아이템 {item_id}은 {types[item_id]}라서 {item_type} 아이템의 표에 둘 수 없다")
        return errors

    return rule


def levels_from_one_ending_at_max(table: Table, _: DesignData) -> list[str]:
    """서버는 마지막 행을 최대 레벨로 보고, 레벨마다 필요한 경험치를 앞에서부터 빼 레벨을 정한다.
    레벨이 빠지거나 마지막이 아닌 레벨의 필요 경험치가 0이면 경험치 없이 레벨이 오른다."""
    if not table.rows:
        return [f"{table.config.workbook}: 행이 없다. 레벨 1 행은 있어야 한다"]
    errors = []
    last = len(table.rows) - 1
    for index, row in enumerate(table.rows):
        where = locate(table.config.workbook, row.sheet, row.number)
        level, requirement = row.values["Level"], row.values["ExpRequirement"]
        if level != index + 1:
            errors.append(f"{where}: 레벨이 {index + 1}이어야 하는데 {level}이다. 레벨은 1부터 빠짐없이 이어져야 한다")
        if index == last and requirement != 0:
            errors.append(f"{where}: 최대 레벨 {level}의 ExpRequirement가 0이어야 하는데 {requirement}이다")
        elif index != last and requirement < 1:
            errors.append(f"{where}: ExpRequirement가 1 이상이어야 하는데 {requirement}이다. 0은 최대 레벨에만 적는다")
    return errors


def every_level_per_class(table: Table, data: DesignData) -> list[str]:
    """표에 나오는 직업은 레벨 표의 모든 레벨에 능력치 행이 있어야 한다. 빠진 레벨에 오른 캐릭터는 능력치를 정하지 못한다."""
    levels = data.tables.get("Level")
    if levels is None:
        return [f"{table.config.workbook}: 레벨을 확인할 표 'Level'이 설정 파일에 없다"]
    all_levels = table_keys(levels)
    by_class: dict[str, set[int]] = {}
    for row in table.rows:
        by_class.setdefault(row.values["Class"], set()).add(row.values["Level"])
    errors = []
    for class_name, class_levels in by_class.items():
        missing = sorted(all_levels - class_levels)
        if missing:
            shown = ", ".join(str(level) for level in missing)
            errors.append(f"{table.config.workbook}: 직업 {class_name}에 레벨 {shown}의 행이 없다")
    return errors


TABLE_RULES: dict[str, list[Callable[[Table, DesignData], list[str]]]] = {
    "Currency": [exactly_one_gold],
    "Item": [item_key_in_kind_range, item_max_stack_at_least_one, item_has_kind_row],
    "ItemEquipStat": [references_item_of_kind("GEAR")],
    "ItemConsumeEffect": [references_item_of_kind("CONSUMABLE"), cooldown_not_negative],
    "Level": [levels_from_one_ending_at_max],
    "ClassLevelStat": [every_level_per_class],
}


def check_rules(data: DesignData, errors: list[str]) -> None:
    for name, rules in TABLE_RULES.items():
        table = data.tables.get(name)
        if table is None:
            continue
        for rule in rules:
            errors.extend(rule(table, data))
