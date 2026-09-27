"""저장소 규범을 텍스트로 검사한다.

`docs/conventions.md`와 `docs/folder-structure.md`가 정한 규칙 중 빌드 없이 판정할 수 있는
다섯 가지를 본다. 엔진도 v145 툴셋도 필요하지 않으므로 호스티드 러너에서 그대로 돈다.

검사 항목과 근거는 아래와 같다.

| 검사 이름              | 무엇을 보는가                          | 근거                        |
| ---------------------- | -------------------------------------- | --------------------------- |
| `file-type`            | 파일 이름과 그 안의 주 타입 이름       | `conventions.md` 2.4        |
| `p1-prefix`            | 리플렉션 타입의 `P1` 약어              | `conventions.md` 2.2        |
| `include-path`         | `#include`가 도메인 경로를 쓰는지      | `conventions.md` 2.6        |
| `project-item-exists`  | 프로젝트 파일의 등록 항목이 실재하는지 | #74                         |
| `project-filter-path`  | `.filters`의 논리 폴더가 디스크와 같은지 | #74                       |

사용법:

    py -3 Tools/ConventionLint/check_conventions.py            # 전부 검사한다
    py -3 Tools/ConventionLint/check_conventions.py --only p1-prefix
    py -3 Tools/ConventionLint/check_conventions.py --list

검사 대상은 git이 추적하는 파일뿐이다. 추적하지 않는 파일은 저장소의 내용이 아니므로 보지
않는다. `node_modules`와 빌드 산출물이 자연히 빠진다.

위반이 하나라도 있으면 종료 코드가 1이다.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path, PurePosixPath

REPO_ROOT = Path(__file__).resolve().parents[2]

# 클라이언트 런타임 모듈의 소스 루트다.
CLIENT_SOURCE = "P1/Source/P1"

# 생성기가 만드는 파일이다. 손으로 고치지 않으므로 어떤 검사도 적용하지 않는다.
# `GenPackets.bat`이 `Protocol/`에서 만들어 각 티어로 복사한다.
GENERATED_FILE_NAMES = frozenset(
    {
        "ClientPacketHandler.h",
        "ClientPacketHandler.cpp",
        "ServerPacketHandler.h",
        "ServerPacketHandler.cpp",
        "Enum.pb.h",
        "Protocol.pb.h",
        "Struct.pb.h",
    }
)

# 생성물이 이름으로 부르기 때문에 프로젝트 약어를 붙이지 않는 타입이다.
# `Protocol/Templates/PacketHandler.h`가 이 이름들을 그대로 적는다 (`conventions.md` 2.2).
GENERATED_NAME_DEPENDENTS = frozenset(
    {"ClientPacketHandler", "PacketSession", "SendBuffer", "PacketHeader"}
)

# 항목을 하나씩 명시 등록하는 프로젝트 파일이다.
#
# `Server/AuthServer/AuthServer.esproj`는 뺀다. SDK 스타일이라 항목을 나열하지 않고 폴더째
# 포함하므로 「등록한 것이 실재하는가」라는 명제가 성립하지 않는다. `P1`의 프로젝트 파일도
# 뺀다. UBT가 생성하고 git이 추적하지 않는다.
PROJECT_FILE_PATTERNS = ("*.vcxproj", "*.pyproj")

# 프로젝트 파일이 파일 하나를 가리킬 때 쓰는 항목 유형이다. 빌드 대상과 그저 목록에 보이게
# 하는 것이 섞여 있는데, 어느 쪽이든 경로가 실재해야 하는 것은 같다.
PROJECT_ITEM_KINDS = (
    "ClCompile",
    "ClInclude",
    "None",
    "Content",
    "UpToDateCheckInput",
    "Text",
    "Image",
    "Natvis",
    "Compile",
)


@dataclass(frozen=True)
class Violation:
    """위반 한 건이다. `path:line`과 사람이 읽을 설명을 담는다."""

    path: str
    line: int
    message: str

    def format(self) -> str:
        if self.line > 0:
            return f"{self.path}:{self.line}: {self.message}"
        return f"{self.path}: {self.message}"


def run_git_ls_files(patterns: list[str]) -> list[str]:
    """git이 추적하는 파일 중 패턴에 맞는 것을 저장소 기준 경로로 돌려준다."""
    result = subprocess.run(
        ["git", "ls-files", "-z", "--"] + patterns,
        cwd=REPO_ROOT,
        capture_output=True,
        check=True,
    )
    raw = result.stdout.decode("utf-8")
    return [entry for entry in raw.split("\0") if entry]


def run_git_check_ignore(paths: list[str]) -> set[str]:
    """준 경로 중 `.gitignore`가 무시하는 것만 추려서 돌려준다.

    **`git ls-files --others --ignored`를 쓰지 않는다.** 그쪽은 디스크를 훑어 실재하는
    파일만 나열하므로, 파일 자체가 없는 환경에서는 무시 대상을 하나도 돌려주지 않는다.
    러너가 그런 환경이다. 실제로 이 검사를 처음 올렸을 때 로컬에서 통과한 것이 CI에서
    `DB/config.h` 1건으로 실패했다.

    `check-ignore`는 경로 문자열을 규칙에 대조하므로 파일이 없어도 판정이 같다.
    `--no-index`는 추적 여부를 보지 않고 규칙만 보게 한다.
    """
    if not paths:
        return set()

    result = subprocess.run(
        ["git", "check-ignore", "--no-index", "--stdin"],
        cwd=REPO_ROOT,
        input="\n".join(paths).encode("utf-8"),
        capture_output=True,
    )
    # 하나도 무시되지 않으면 종료 코드가 1이다. 오류가 아니다. 진짜 오류는 128이다.
    if result.returncode not in (0, 1):
        raise RuntimeError(
            f"git check-ignore가 {result.returncode}으로 끝났다: "
            f"{result.stderr.decode('utf-8', errors='replace')}"
        )
    return {line for line in result.stdout.decode("utf-8").splitlines() if line}


def read_text(rel_path: str) -> str:
    """저장소의 텍스트 파일을 읽는다. 인코딩이 섞여 있어도 검사가 멈추지 않게 한다."""
    data = (REPO_ROOT / rel_path).read_bytes()
    try:
        return data.decode("utf-8")
    except UnicodeDecodeError:
        # `.proto`와 `.bat`은 cp949다 (`conventions.md` 1.3). 이 검사가 보는 파일은 전부
        # UTF-8이지만, 새 파일이 섞여 들어와도 예외로 죽지 않게 한다.
        return data.decode("cp949", errors="replace")


def strip_comments_and_strings(source: str, preserve_string_contents: bool = False) -> str:
    """주석과 문자열 리터럴의 내용을 공백으로 바꾼다. 줄 번호와 길이는 그대로 둔다.

    주석 안의 예시 코드가 위반으로 잡히는 것을 막는다. 규범 문서를 인용한 주석이 실제로 있다.

    `preserve_string_contents`를 켜면 문자열의 내용을 남긴다. `#include "경로"`처럼 문자열
    자체가 검사 대상인 곳에서 쓴다. 이때도 문자열은 파싱하므로 문자열 안의 `//`가 주석으로
    오인되지 않는다.
    """
    out = []
    i = 0
    length = len(source)
    while i < length:
        char = source[i]
        nxt = source[i + 1] if i + 1 < length else ""

        if char == "/" and nxt == "/":
            while i < length and source[i] != "\n":
                out.append(" ")
                i += 1
            continue

        if char == "/" and nxt == "*":
            out.append("  ")
            i += 2
            while i < length and not (source[i] == "*" and i + 1 < length and source[i + 1] == "/"):
                out.append("\n" if source[i] == "\n" else " ")
                i += 1
            out.append("  ")
            i += 2
            continue

        if char in ('"', "'"):
            quote = char
            out.append(quote)
            i += 1
            while i < length and source[i] != quote:
                if source[i] == "\\" and i + 1 < length:
                    out.append(source[i : i + 2] if preserve_string_contents else "  ")
                    i += 2
                    continue
                if preserve_string_contents:
                    out.append(source[i])
                else:
                    out.append("\n" if source[i] == "\n" else " ")
                i += 1
            if i < length:
                out.append(quote)
                i += 1
            continue

        out.append(char)
        i += 1

    return "".join(out)


def is_generated(rel_path: str) -> bool:
    return Path(rel_path).name in GENERATED_FILE_NAMES


# ----------------------------------------------------------------------------------
# 검사 1: 파일 이름과 타입 이름의 일치 (`conventions.md` 2.4)
# ----------------------------------------------------------------------------------

# 전방 선언과 실제 정의를 가른다. 정의는 여는 중괄호가 뒤따르고 전방 선언은 `;`로 끝난다.
#
# **여는 중괄호가 다음 줄에 오는 것을 반드시 허용해야 한다.** 이 저장소의 C++은 중괄호를
# 다음 줄에 둔다. 같은 줄만 보는 정규식은 타입을 하나도 찾지 못하고, 그러면 이 검사가 아무것도
# 검사하지 않으면서 통과한다.
CLASS_DEF_RE = re.compile(
    r"^[ \t]*(class|struct)[ \t]+(?:[A-Z][A-Z0-9_]*_API[ \t]+)?"
    r"([A-Za-z_][A-Za-z0-9_]*)[ \t]*(?::[^;{]*)?\s*\{",
    re.MULTILINE,
)
ENUM_DEF_RE = re.compile(
    r"^[ \t]*enum[ \t]+(?:class[ \t]+)?([A-Za-z_][A-Za-z0-9_]*)[ \t]*(?::[^;{]*)?\s*\{",
    re.MULTILINE,
)

# 타입 접두사 한 글자다 (`conventions.md` 2.1). 파일 이름에는 붙이지 않는다.
TYPE_PREFIX_RE = re.compile(r"^[UAFEI](?=[A-Z])")


def declared_types(text: str) -> list[tuple[str, int]]:
    """헤더가 정의하는 타입의 이름과 줄 번호를 돌려준다. 전방 선언은 세지 않는다."""
    found: list[tuple[str, int]] = []
    for match in CLASS_DEF_RE.finditer(text):
        line = text.count("\n", 0, match.start()) + 1
        found.append((match.group(2), line))
    for match in ENUM_DEF_RE.finditer(text):
        line = text.count("\n", 0, match.start()) + 1
        found.append((match.group(1), line))
    return sorted(found, key=lambda item: item[1])


def check_file_type_match(_: argparse.Namespace) -> list[Violation]:
    """헤더 파일 이름과 그 안의 주 타입 이름이 맞는지 본다."""
    violations: list[Violation] = []
    for rel_path in run_git_ls_files([f"{CLIENT_SOURCE}/**/*.h", f"{CLIENT_SOURCE}/*.h"]):
        if is_generated(rel_path):
            continue

        text = strip_comments_and_strings(read_text(rel_path))
        types = declared_types(text)
        if not types:
            # 타입을 정의하지 않는 헤더다. 다른 헤더를 모아 주기만 하는 파일이 여기 해당한다.
            continue

        stem = Path(rel_path).stem
        expected = {TYPE_PREFIX_RE.sub("", name) for name, _ in types}
        if stem in expected:
            continue

        # 그 타입 안에서만 쓰는 작은 열거형은 같은 파일에 둘 수 있다 (2.4의 예외). 파일 이름과
        # 맞는 타입이 하나라도 있으면 통과이므로, 여기까지 왔다는 것은 하나도 없다는 뜻이다.
        names = ", ".join(f"{name}({line}줄)" for name, line in types)
        violations.append(
            Violation(
                rel_path,
                types[0][1],
                f"파일 이름 '{stem}'과 맞는 타입이 없다. 정의된 타입: {names}. "
                f"파일 이름과 주 타입 이름을 맞춘다 (conventions.md 2.4)",
            )
        )
    return violations


# ----------------------------------------------------------------------------------
# 검사 2: 리플렉션 타입의 `P1` 약어 (`conventions.md` 2.2)
# ----------------------------------------------------------------------------------

REFLECTION_MACRO_RE = re.compile(r"^[ \t]*(UCLASS|USTRUCT|UENUM|UINTERFACE)[ \t]*\(", re.MULTILINE)


def check_p1_prefix(_: argparse.Namespace) -> list[Violation]:
    """리플렉션 대상 타입에 프로젝트 약어가 붙었는지 본다.

    리플렉션 이름은 전역에서 유일해야 한다. 약어가 없으면 엔진과 플러그인의 클래스 이름과
    충돌할 자리가 열린다.
    """
    violations: list[Violation] = []
    for rel_path in run_git_ls_files([f"{CLIENT_SOURCE}/**/*.h", f"{CLIENT_SOURCE}/*.h"]):
        if is_generated(rel_path):
            continue

        text = strip_comments_and_strings(read_text(rel_path))
        types = declared_types(text)
        if not types:
            continue

        for macro in REFLECTION_MACRO_RE.finditer(text):
            macro_line = text.count("\n", 0, macro.start()) + 1
            # 매크로 바로 다음에 오는 타입 정의가 그 매크로의 대상이다.
            target = next((item for item in types if item[1] > macro_line), None)
            if target is None:
                continue

            name, line = target
            if name in GENERATED_NAME_DEPENDENTS:
                continue

            body = TYPE_PREFIX_RE.sub("", name)
            if body.startswith("P1"):
                continue

            violations.append(
                Violation(
                    rel_path,
                    line,
                    f"리플렉션 타입 '{name}'에 프로젝트 약어가 없다. "
                    f"타입 접두사 뒤에 'P1'을 붙인다 (conventions.md 2.2)",
                )
            )
    return violations


# ----------------------------------------------------------------------------------
# 검사 3: `#include`가 도메인 경로를 쓰는지 (`conventions.md` 2.6)
# ----------------------------------------------------------------------------------

INCLUDE_RE = re.compile(r'^[ \t]*#[ \t]*include[ \t]+"([^"]+)"', re.MULTILINE)


def check_include_path(_: argparse.Namespace) -> list[Violation]:
    """프로젝트 헤더를 파일 이름만으로 include하는 자리를 잡는다.

    `#include` 줄에 도메인 이름이 드러나야 경계를 넘는 참조가 검색으로 잡힌다 (ADR-0005).
    """
    headers = run_git_ls_files([f"{CLIENT_SOURCE}/**/*.h", f"{CLIENT_SOURCE}/*.h"])

    # 도메인 폴더 안에 있는 헤더의 이름과 그 실제 경로를 모은다. 모듈 루트에 있는 헤더는
    # 한정할 경로가 없으므로 대상이 아니다.
    domain_headers: dict[str, str] = {}
    for rel_path in headers:
        if is_generated(rel_path):
            continue
        relative_to_module = Path(rel_path).relative_to(CLIENT_SOURCE)
        if len(relative_to_module.parts) == 1:
            continue
        domain_headers[relative_to_module.name] = relative_to_module.as_posix()

    violations: list[Violation] = []
    sources = run_git_ls_files(
        [
            f"{CLIENT_SOURCE}/**/*.h",
            f"{CLIENT_SOURCE}/**/*.cpp",
            f"{CLIENT_SOURCE}/*.h",
            f"{CLIENT_SOURCE}/*.cpp",
        ]
    )
    for rel_path in sources:
        if is_generated(rel_path):
            continue

        # include 경로는 문자열 리터럴 안에 있다. 내용을 지우면 검사할 것이 남지 않는다.
        text = strip_comments_and_strings(read_text(rel_path), preserve_string_contents=True)
        for match in INCLUDE_RE.finditer(text):
            included = match.group(1)
            if "/" in included:
                continue

            target = domain_headers.get(included)
            if target is None:
                # 엔진 헤더이거나 모듈 루트의 헤더다. 둘 다 한정할 도메인 경로가 없다.
                continue

            line = text.count("\n", 0, match.start()) + 1
            violations.append(
                Violation(
                    rel_path,
                    line,
                    f'#include "{included}"가 경로를 한정하지 않는다. '
                    f'"{target}"로 적는다 (conventions.md 2.6)',
                )
            )
    return violations


# ----------------------------------------------------------------------------------
# 검사 4·5: 프로젝트 파일의 등록 항목 (#74)
# ----------------------------------------------------------------------------------

PROJECT_ITEM_RE = re.compile(
    r"<(" + "|".join(PROJECT_ITEM_KINDS) + r")\s+Include=\"([^\"]+)\"",
    re.IGNORECASE,
)

# 논리 폴더는 여는 태그와 닫는 태그 사이의 `<Filter>`에 있다. 자기 닫는 태그
# (`<ClCompile Include="..." />`)는 논리 폴더를 적지 않으므로 이 정규식에 걸리지 않는다.
PROJECT_FILTER_RE = re.compile(
    r"<(" + "|".join(PROJECT_ITEM_KINDS) + r")\s+Include=\"([^\"]+)\"\s*>(.*?)</\1>",
    re.IGNORECASE | re.DOTALL,
)

FILTER_TAG_RE = re.compile(r"<Filter>(.*?)</Filter>", re.IGNORECASE | re.DOTALL)


def _resolve_project_item(project_path: str, include: str) -> str | None:
    """항목의 `Include` 경로를 저장소 기준 경로로 푼다.

    MSBuild 변수가 든 경로는 값을 모르므로 `None`을 돌려주고 건너뛴다.
    """
    if "$(" in include or "%(" in include:
        return None

    base = PurePosixPath(project_path).parent
    parts: list[str] = []
    for part in (base / include.replace("\\", "/")).parts:
        if part == "..":
            if parts:
                parts.pop()
        elif part != ".":
            parts.append(part)
    return "/".join(parts)


def _item_disk_folder(include: str) -> str:
    """항목 경로에서 논리 폴더와 대조할 디스크 폴더를 뽑는다.

    **`..`를 벗긴다.** Visual Studio의 논리 트리에는 상대 경로라는 개념이 없어서 `..`를
    논리 폴더 이름에 쓸 수 없다. 벗기지 않으면 솔루션 폴더 밖을 가리키는 항목이 전부
    어긋난 것으로 잡힌다. 2026년 9월 22일에 재 보니 불일치 31건 중 27건이 그것이었다.
    """
    parts = [p for p in PurePosixPath(include.replace("\\", "/")).parts[:-1] if p != ".."]
    return "\\".join(parts)


def _project_files(suffix: str = "") -> list[str]:
    """추적 중인 프로젝트 파일을 돌려준다. `suffix`로 `.filters`를 고른다.

    확장자를 여기서 한 번 더 확인한다. self-test의 가짜 파일 목록은 패턴으로 거르지 않고
    픽스처를 통째로 주기 때문에, 이 확인이 없으면 픽스처의 다른 파일까지 열게 된다.
    """
    patterns = [pattern + suffix for pattern in PROJECT_FILE_PATTERNS]
    endings = tuple(pattern.lstrip("*") for pattern in patterns)
    return sorted(path for path in run_git_ls_files(patterns) if path.endswith(endings))


def check_project_item_exists(_: argparse.Namespace) -> list[Violation]:
    """프로젝트 파일이 등록한 항목이 저장소에 실재하는지 본다.

    **실재의 기준은 디스크가 아니라 git이다.** 디스크를 보면 로컬에서 통과한 것이 CI에서
    실패한다. `GameServer.vcxproj`가 등록한 `DB/config.h`가 그런 파일인데, 디스크에 있고
    `Server/.gitignore`가 의도적으로 무시하며 예시 파일이 없어서 러너에는 존재하지 않는다.

    그래서 추적 중인 파일과 **`.gitignore`가 무시하기로 한 경로**를 둘 다 실재로 친다.
    무시되는 경로는 저장소가 관리하지 않으므로 이 검사가 막으려는 사고(파일을 옮기고 프로젝트
    파일을 안 고치는 것)의 대상이 아니다. 「무시하는 파일」이 아니라 「무시하기로 한 경로」인
    것이 중요하다. 판정이 파일의 존재 여부를 타면 환경에 따라 결과가 갈린다.
    """
    tracked = {path.lower() for path in run_git_ls_files([])}

    # 먼저 추적 목록으로 거르고, 남은 것만 무시 규칙에 대조한다. `check-ignore`를 한 번만
    # 부르려고 경로를 모아 두었다가 한꺼번에 넘긴다.
    unresolved: list[tuple[str, int, str, str, str]] = []
    for project_path in _project_files():
        text = read_text(project_path)
        for match in PROJECT_ITEM_RE.finditer(text):
            kind, include = match.group(1), match.group(2)
            target = _resolve_project_item(project_path, include)
            if target is None or target.lower() in tracked:
                continue
            line = text.count("\n", 0, match.start()) + 1
            unresolved.append((project_path, line, kind, include, target))

    ignored = run_git_check_ignore(sorted({row[4] for row in unresolved}))

    return [
        Violation(
            project_path,
            line,
            f'<{kind} Include="{include}">가 가리키는 "{target}"이 저장소에 없다',
        )
        for project_path, line, kind, include, target in unresolved
        if target not in ignored
    ]


def check_project_filter_path(_: argparse.Namespace) -> list[Violation]:
    """`.filters`의 논리 폴더가 항목의 디스크 폴더와 같은지 본다.

    논리 폴더를 적지 않은 항목은 프로젝트 루트에 놓인다는 뜻이므로 판정하지 않는다.
    """
    violations: list[Violation] = []
    for filters_path in _project_files(".filters"):
        text = read_text(filters_path)
        for match in PROJECT_FILTER_RE.finditer(text):
            kind, include, body = match.group(1), match.group(2), match.group(3)
            if "$(" in include or "%(" in include:
                continue

            tag = FILTER_TAG_RE.search(body)
            if tag is None:
                continue

            logical = tag.group(1).strip()
            disk = _item_disk_folder(include)
            if logical == disk:
                continue

            line = text.count("\n", 0, match.start()) + 1
            violations.append(
                Violation(
                    filters_path,
                    line,
                    f'<{kind} Include="{include}">의 논리 폴더가 "{logical}"인데 '
                    f'디스크 폴더는 "{disk}"다',
                )
            )
    return violations


# ----------------------------------------------------------------------------------
# 진입점
# ----------------------------------------------------------------------------------

CHECKS = {
    "file-type": ("파일 이름과 타입 이름의 일치", check_file_type_match),
    "p1-prefix": ("리플렉션 타입의 P1 약어", check_p1_prefix),
    "include-path": ("#include의 도메인 경로 한정", check_include_path),
    "project-item-exists": ("프로젝트 파일 등록 항목의 실재", check_project_item_exists),
    "project-filter-path": ("filters의 논리 폴더와 디스크 폴더", check_project_filter_path),
}


# ----------------------------------------------------------------------------------
# 자기 검증
# ----------------------------------------------------------------------------------
#
# **검사가 아무것도 검사하지 않으면서 통과하는 상태를 잡는다.**
#
# 이 스크립트를 처음 돌렸을 때 다섯 검사가 모두 통과했는데, 그중 셋은 정규식이 타입을 하나도
# 찾지 못해서 통과한 것이었다. 이 저장소의 C++은 여는 중괄호를 다음 줄에 두는데 정규식이 같은
# 줄만 보고 있었다. 위반 0건과 대상 0건은 출력이 같다.
#
# 그래서 일부러 어긋낸 입력을 검사에 먹여 위반이 실제로 잡히는지 본다. 파일 목록과 내용을
# 주는 두 함수만 바꿔 끼우므로 검사 본체는 실제로 도는 코드 그대로다.

# 검사 이름마다 (위반이 박힌 픽스처, 잡혀야 할 위반 수)를 둔다.
SELF_TEST_FIXTURES: dict[str, dict[str, str | None]] = {
    "file-type": {
        f"{CLIENT_SOURCE}/Combat/P1AttackSystemComponent.h": (
            "#pragma once\n"
            "UCLASS()\n"
            "class P1_API UP1DifferentName : public UActorComponent\n"
            "{\n"
            "    GENERATED_BODY()\n"
            "};\n"
        ),
    },
    "p1-prefix": {
        f"{CLIENT_SOURCE}/UI/P1InventoryWidget.h": (
            "#pragma once\n"
            "UCLASS()\n"
            "class P1_API UInventoryWidget : public UUserWidget\n"
            "{\n"
            "    GENERATED_BODY()\n"
            "};\n"
        ),
    },
    "include-path": {
        f"{CLIENT_SOURCE}/Characters/P1MyPlayer.h": "#pragma once\n",
        f"{CLIENT_SOURCE}/UI/P1HUDWidget.cpp": '#include "P1MyPlayer.h"\n',
    },
    "project-item-exists": {
        "Server/GameServer/GameServer.vcxproj": (
            "<Project>\n"
            "  <ItemGroup>\n"
            '    <ClCompile Include="Game\\Entities\\Gone.cpp" />\n'
            "  </ItemGroup>\n"
            "</Project>\n"
        ),
    },
    "project-filter-path": {
        "Server/GameServer/GameServer.vcxproj.filters": (
            "<Project>\n"
            "  <ItemGroup>\n"
            '    <None Include="..\\..\\Protocol\\Schema\\Enum.proto">\n'
            "      <Filter>Protocol\\Proto</Filter>\n"
            "    </None>\n"
            "  </ItemGroup>\n"
            "</Project>\n"
        ),
    },
}

# 규범을 지키는 입력이다. 여기서 위반이 나오면 검사가 과하게 잡는 것이다.
SELF_TEST_CLEAN: dict[str, dict[str, str | None]] = {
    "file-type": {
        f"{CLIENT_SOURCE}/Combat/P1AttackSystemComponent.h": (
            "#pragma once\n"
            "UCLASS()\n"
            "class P1_API UP1AttackSystemComponent : public UActorComponent\n"
            "{\n"
            "    GENERATED_BODY()\n"
            "};\n"
        ),
    },
    "p1-prefix": {
        f"{CLIENT_SOURCE}/UI/P1InventoryWidget.h": (
            "#pragma once\n"
            "UCLASS()\n"
            "class P1_API UP1InventoryWidget : public UP1UserWidget\n"
            "{\n"
            "    GENERATED_BODY()\n"
            "};\n"
        ),
    },
    "include-path": {
        f"{CLIENT_SOURCE}/Characters/P1MyPlayer.h": "#pragma once\n",
        f"{CLIENT_SOURCE}/UI/P1HUDWidget.cpp": '#include "Characters/P1MyPlayer.h"\n',
    },
    "project-item-exists": {
        "Server/GameServer/GameServer.vcxproj": (
            "<Project>\n"
            "  <ItemGroup>\n"
            '    <ClCompile Include="Game\\Entities\\Player.cpp" />\n'
            "    <!-- MSBuild 변수가 든 경로는 값을 모르므로 건너뛴다 -->\n"
            '    <ClCompile Include="$(IntDir)\\Generated.cpp" />\n'
            "  </ItemGroup>\n"
            "</Project>\n"
        ),
        "Server/GameServer/Game/Entities/Player.cpp": None,
    },
    "project-filter-path": {
        "Server/GameServer/GameServer.vcxproj.filters": (
            "<Project>\n"
            "  <ItemGroup>\n"
            '    <None Include="..\\..\\Protocol\\Schema\\Enum.proto">\n'
            "      <Filter>Protocol\\Schema</Filter>\n"
            "    </None>\n"
            "    <!-- `..`를 벗기고 대조하는지 본다. 벗기지 않으면 여기가 잡힌다 -->\n"
            '    <ClCompile Include="..\\GameServer\\Main\\pch.cpp">\n'
            "      <Filter>GameServer\\Main</Filter>\n"
            "    </ClCompile>\n"
            "    <!-- 논리 폴더를 적지 않은 항목은 프로젝트 루트에 놓인다 -->\n"
            '    <ClCompile Include="Main\\Global.cpp" />\n'
            "  </ItemGroup>\n"
            "</Project>\n"
        ),
    },
}


def _run_against_fixture(name: str, fixture: dict[str, str | None]) -> list[Violation]:
    """파일 목록과 내용을 픽스처로 바꿔 끼우고 검사 하나를 돌린다.

    저장소를 보는 함수는 이 셋뿐이다. 검사 본체가 이 셋만 거쳐서 바깥을 보게 두면 픽스처로
    전부 덮을 수 있다. 디스크를 직접 훑는 코드를 검사에 넣으면 여기서 덮이지 않는다.
    """
    global run_git_ls_files, read_text, run_git_check_ignore

    original_ls = run_git_ls_files
    original_read = read_text
    original_check_ignore = run_git_check_ignore

    def fake_ls(patterns: list[str]) -> list[str]:
        # 픽스처는 작으므로 패턴별로 거르지 않고 전부 준다. 검사 본체가 확장자와 경로로
        # 다시 거르기 때문에 이것으로 충분하다.
        return sorted(fixture)

    def fake_read(rel_path: str) -> str:
        content = fixture.get(rel_path)
        if content is None:
            raise AssertionError(f"픽스처에 내용이 없는 파일을 읽으려 했다: {rel_path}")
        return content

    def fake_check_ignore(paths: list[str]) -> set[str]:
        # 픽스처에 `.gitignore`가 없으므로 무시되는 경로도 없다.
        return set()

    run_git_ls_files, read_text = fake_ls, fake_read
    run_git_check_ignore = fake_check_ignore
    try:
        return CHECKS[name][1](argparse.Namespace())
    finally:
        run_git_ls_files, read_text = original_ls, original_read
        run_git_check_ignore = original_check_ignore


def run_self_test() -> int:
    """일부러 어긋낸 입력을 검사가 잡는지, 올바른 입력을 그냥 두는지 본다."""
    failures: list[str] = []

    for name in sorted(CHECKS):
        description = CHECKS[name][0]

        dirty = _run_against_fixture(name, SELF_TEST_FIXTURES[name])
        clean = _run_against_fixture(name, SELF_TEST_CLEAN[name])

        if not dirty:
            failures.append(
                f"{name}: 일부러 어긋낸 입력에서 위반을 잡지 못했다. "
                f"이 검사는 아무것도 검사하지 않고 있다"
            )
        elif clean:
            detail = "; ".join(violation.format() for violation in clean)
            failures.append(f"{name}: 규범을 지키는 입력을 위반으로 잡았다 — {detail}")
        else:
            print(f"[통과] {name} — {description}: 위반 {len(dirty)}건을 잡았고 정상 입력은 통과했다")

    print()
    if failures:
        print(f"자기 검증 실패 {len(failures)}건:")
        for failure in failures:
            print(f"  {failure}")
        return 1

    print("자기 검증을 모두 통과했다.")
    return 0


def main() -> int:
    # 윈도우 콘솔의 기본 코드 페이지가 cp949라서 한국어 출력이 예외로 죽는다. CI 러너에서도
    # 같다. 출력 스트림을 UTF-8로 고정한다.
    for stream in (sys.stdout, sys.stderr):
        stream.reconfigure(encoding="utf-8", errors="replace")

    parser = argparse.ArgumentParser(
        description="저장소 규범을 텍스트로 검사한다.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "--only",
        action="append",
        choices=sorted(CHECKS),
        help="지정한 검사만 돌린다. 여러 번 줄 수 있다",
    )
    parser.add_argument("--list", action="store_true", help="검사 목록을 보여주고 끝낸다")
    parser.add_argument(
        "--self-test",
        action="store_true",
        help="검사가 실제로 위반을 잡는지 확인하고 끝낸다",
    )
    args = parser.parse_args()

    if args.list:
        for name, (description, _) in sorted(CHECKS.items()):
            print(f"{name:<15} {description}")
        return 0

    if args.self_test:
        return run_self_test()

    selected = args.only or sorted(CHECKS)
    total = 0

    for name in selected:
        description, check = CHECKS[name]
        violations = check(args)
        total += len(violations)

        if violations:
            print(f"[실패] {name} — {description}: {len(violations)}건")
            for violation in violations:
                print(f"  {violation.format()}")
        else:
            print(f"[통과] {name} — {description}")

    print()
    if total:
        print(f"위반 {total}건을 찾았다.")
        return 1

    print("위반이 없다.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
