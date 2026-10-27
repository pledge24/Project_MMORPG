"""표 하나에만 걸리는 기획 데이터 규칙. 시트 규칙과 타입으로 나타낼 수 없는 의미를 검사한다.

코드가 정한 제약(DB 열 크기, 로비와 게임의 대조)은 여기 두지 않는다. 그 제약은 서버가 기동할 때 검사한다.
규칙은 표 이름으로 찾는다. 표가 설정 파일에 없으면 규칙도 돌지 않는다.
"""

from __future__ import annotations

from collections.abc import Callable

from databuilder.workbooks import DesignData, Table


def exactly_one_gold(table: Table, _: DesignData) -> list[str]:
    """서버는 재화 종류로 골드를 찾으므로 골드인 재화가 하나로 정해져야 한다."""
    gold_rows = [row for row in table.rows if row.values.get("Type") == "GOLD"]
    if len(gold_rows) == 1:
        return []
    places = ", ".join(f"시트 '{row.sheet}' {row.number}행" for row in gold_rows)
    return [f"{table.config.workbook}: Type이 GOLD인 행이 하나여야 하는데 {len(gold_rows)}개다" + (f" ({places})" if places else "")]


TABLE_RULES: dict[str, list[Callable[[Table, DesignData], list[str]]]] = {
    "Currency": [exactly_one_gold],
}


def check_rules(data: DesignData, errors: list[str]) -> None:
    for name, rules in TABLE_RULES.items():
        table = data.tables.get(name)
        if table is None:
            continue
        for rule in rules:
            errors.extend(rule(table, data))
