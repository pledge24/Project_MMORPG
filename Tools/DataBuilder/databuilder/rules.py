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


def range_ordered(low_column: str, high_column: str, minimum: int) -> Callable[[Table, DesignData], list[str]]:
    """최솟값 열이 minimum 이상이고 최댓값 열보다 크지 않은지 본다. 서버가 두 값 사이에서 고른다."""

    def rule(table: Table, _: DesignData) -> list[str]:
        errors = []
        for row in table.rows:
            low, high = row.values[low_column], row.values[high_column]
            where = locate(table.config.workbook, row.sheet, row.number)
            if low < minimum:
                errors.append(f"{where}: {low_column}가 {minimum} 이상이어야 하는데 {low}이다")
            if low > high:
                errors.append(f"{where}: {low_column} {low}이 {high_column} {high}보다 크다")
        return errors

    return rule


def is_probability(value: float) -> bool:
    return 0.0 < value <= 1.0


# 판정 방식 → 그 방식의 드롭 항목이 채우는 열. 나머지 하나는 비워 둔다.
DROP_ENTRY_ODDS_COLUMN = {"INDEPENDENT": "Rate", "WEIGHTED": "Weight"}


def drop_entries_by_group(data: DesignData) -> dict[int, list]:
    entries: dict[int, list] = {}
    table = data.tables.get("DropEntry")
    for row in table.rows if table is not None else []:
        entries.setdefault(row.values["GroupId"], []).append(row)
    return entries


def drop_group_matches_method(table: Table, data: DesignData) -> list[str]:
    """그룹 확률은 0 초과 1 이하다. 항목이 하나 이상이고, 가중치 그룹만 뽑는 개수를 갖는다. 개수는 항목 수를 넘지 않는다."""
    entries = drop_entries_by_group(data)
    errors = []
    for row in table.rows:
        where = locate(table.config.workbook, row.sheet, row.number)
        group_id, method, pick_count = row.values["Id"], row.values["Method"], row.values["PickCount"]
        if not is_probability(row.values["Rate"]):
            errors.append(f"{where}: 그룹 {group_id}의 Rate가 0 초과 1 이하여야 하는데 {row.values['Rate']}이다")
        count = len(entries.get(group_id, []))
        if count == 0:
            errors.append(f"{where}: 그룹 {group_id}에 드롭 항목이 없다. 표 'DropEntry'에 항목을 하나 이상 둔다")
        if method == "WEIGHTED":
            if pick_count is None or pick_count < 1:
                errors.append(f"{where}: WEIGHTED 그룹 {group_id}의 PickCount가 1 이상이어야 하는데 {'비어 있다' if pick_count is None else pick_count}")
            elif count and pick_count > count:
                errors.append(f"{where}: 그룹 {group_id}의 PickCount {pick_count}가 항목 수 {count}보다 크다")
        elif pick_count is not None:
            errors.append(f"{where}: {method} 그룹 {group_id}은 PickCount를 비워 둔다. 뽑는 개수는 WEIGHTED 그룹에만 쓴다")
    return errors


def drop_entry_matches_group_method(table: Table, data: DesignData) -> list[str]:
    """독립 판정 그룹의 항목은 확률(Rate)만, 가중치 그룹의 항목은 가중치(Weight)만 채운다."""
    groups = data.tables.get("DropGroup")
    if groups is None:
        return [f"{table.config.workbook}: 판정 방식을 확인할 표 'DropGroup'이 설정 파일에 없다"]
    methods = {row.values["Id"]: row.values["Method"] for row in groups.rows}
    errors = []
    for row in table.rows:
        where = locate(table.config.workbook, row.sheet, row.number)
        method = methods[row.values["GroupId"]]
        rate, weight = row.values["Rate"], row.values["Weight"]
        filled = DROP_ENTRY_ODDS_COLUMN[method]
        for column in DROP_ENTRY_ODDS_COLUMN.values():
            if column == filled and row.values[column] is None:
                errors.append(f"{where}: {method} 그룹의 항목은 {column}를 채운다")
            if column != filled and row.values[column] is not None:
                errors.append(f"{where}: {method} 그룹의 항목은 {column}를 비워 둔다")
        if rate is not None and not is_probability(rate):
            errors.append(f"{where}: Rate가 0 초과 1 이하여야 하는데 {rate}이다")
        if weight is not None and weight < 1:
            errors.append(f"{where}: Weight가 1 이상이어야 하는데 {weight}이다")
    return errors


def drop_group_used_by_monster(table: Table, data: DesignData) -> list[str]:
    """어느 몬스터도 쓰지 않는 그룹은 판정되지 않는다. 작업 중일 수 있으므로 경고만 한다."""
    monsters = data.tables.get("Monster")
    used = {group_id for row in monsters.rows for group_id in row.values["DropGroups"]} if monsters is not None else set()
    return [
        f"{locate(table.config.workbook, row.sheet, row.number)}: 그룹 {row.values['Id']}을 쓰는 몬스터가 없다"
        for row in table.rows
        if row.values["Id"] not in used
    ]


TABLE_RULES: dict[str, list[Callable[[Table, DesignData], list[str]]]] = {
    "Currency": [exactly_one_gold],
    "Item": [item_key_in_kind_range, item_max_stack_at_least_one, item_has_kind_row],
    "ItemEquipStat": [references_item_of_kind("GEAR")],
    "ItemConsumeEffect": [references_item_of_kind("CONSUMABLE"), cooldown_not_negative],
    "Level": [levels_from_one_ending_at_max],
    "ClassLevelStat": [every_level_per_class],
    "Monster": [range_ordered("MinExp", "MaxExp", 0), range_ordered("MinGold", "MaxGold", 0)],
    "DropGroup": [drop_group_matches_method],
    "DropEntry": [drop_entry_matches_group_method, range_ordered("MinCount", "MaxCount", 1)],
}

# 산출물을 막지 않는 규칙. 어기면 경고를 남기고 생성은 계속한다.
TABLE_WARNINGS: dict[str, list[Callable[[Table, DesignData], list[str]]]] = {
    "DropGroup": [drop_group_used_by_monster],
}


def check_rules(data: DesignData, errors: list[str], warnings: list[str]) -> None:
    for found, table_rules in ((errors, TABLE_RULES), (warnings, TABLE_WARNINGS)):
        for name, rules in table_rules.items():
            table = data.tables.get(name)
            if table is None:
                continue
            for rule in rules:
                found.extend(rule(table, data))
