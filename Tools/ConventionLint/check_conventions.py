"""저장소 규범을 텍스트로 검사한다.

`docs/conventions.md`와 `docs/folder-structure.md`가 정한 규칙 중 빌드 없이 판정할 수 있는
여덟 가지를 본다. 엔진도 v145 툴셋도 필요하지 않으므로 호스티드 러너에서 그대로 돈다.

검사 항목과 근거는 아래와 같다.

| 검사 이름              | 무엇을 보는가                          | 근거                        |
| ---------------------- | -------------------------------------- | --------------------------- |
| `file-type`            | 파일 이름과 그 안의 주 타입 이름       | `conventions.md` 2.4        |
| `p1-prefix`            | 리플렉션 타입의 `P1` 약어              | `conventions.md` 2.2        |
| `include-path`         | `#include`가 도메인 경로를 쓰는지      | `conventions.md` 2.6        |
| `server-member`        | 서버 멤버 변수의 `_camelCase`          | `conventions.md` 3.2        |
| `asset-prefix`         | 추적 중인 에셋의 접두사                | `folder-structure.md` 4.3   |
| `folder-symmetry`      | `Game/` 하위 폴더가 ADR-0007의 표와 같은지 | ADR-0007                |
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

# 게임 서버 쪽에서 이름 규칙을 적용하는 범위다. `Server/ServerCore/`는 제외한다
# (`conventions.md` 3절). 완성된 네트워크 코어라서 고쳐서 얻는 것보다 잃는 것이 크다.
SERVER_SOURCE_ROOTS = ("Server/GameServer", "Server/DummyClient")

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

# 운영체제가 정한 이름을 그대로 담는 포인터다. 이름을 바꾸면 Winsock 문서에서 같은 이름으로
# 찾을 수 없다 (`conventions.md` 3.2의 예외).
WINSOCK_MEMBER_NAMES = frozenset({"ConnectEx", "DisconnectEx", "AcceptEx"})

# 규범을 아직 지키지 않는 자리다. **예외가 아니라 갚아야 할 부채다.**
#
# 규칙을 고쳐서 통과시키는 것과 미준수를 여기 적는 것은 다르다. 앞은 규범이 바뀌는 것이고
# 뒤는 규범은 그대로 둔 채 갚을 자리를 이름으로 남기는 것이다. 여기 적은 것은 전부
# `docs/tech-debt.md`에 대응 항목이 있어야 하고, 갚으면 양쪽에서 함께 지운다.
PENDING_SERVER_MEMBER_DEBT = frozenset(
    {
        # `vector2D`와 `vector3D`의 성분 `x`·`y`·`z`다. 수학 벡터의 성분 이름이고 바깥에서
        # `lhs.x * scale`처럼 직접 읽는다. 동작이 있으므로 3.2의 순수 데이터 구조체 예외에는
        # 들지 않는다. 고치면 사용처가 함께 움직이므로 이 티켓의 범위 밖이다.
        ("Server/GameServer/Utils/Utils.h", "vector2D"),
        ("Server/GameServer/Utils/Utils.h", "vector3D"),
    }
)

# 에셋 접두사 표다 (`folder-structure.md` 4.3).
ASSET_PREFIXES = frozenset(
    {
        "BP", "WBP", "BPC", "ABP",
        "SK", "SM", "SKEL", "PHYS",
        "AM", "BS", "A", "ALI",
        "M", "MI", "MF", "T",
        "NS", "MS", "SC",
        "DT", "DA", "CT", "ST",
        "IA", "IMC", "L",
        "F", "FF",
    }
)

# 레벨은 확장자가 따로 있으므로 접두사를 하나로 못박는다.
LEVEL_PREFIX = "L"

# 팩에서 복사해 온 에셋이 사는 폴더다. 접두사 규칙은 자작 에셋에만 적용하고, 팩에서 가져온
# 것은 이름과 하위 구조를 바꾸지 않는다 (`folder-structure.md` 4.1과 4.3).
#
# 폴더로 적는 이유는 파일 이름만으로는 자작과 복사본을 가릴 수 없기 때문이다. 이 목록에 폴더를
# 더할 때는 그 폴더 전체가 팩에서 온 것인지 확인한다. 자작 에셋을 여기 두면 검사에서 빠진다.
PACK_COPY_DIRS = (
    "P1/Content/P1/Characters/Player/Mannequins/",
    "P1/Content/P1/Characters/Monsters/Down_Minions/",
    "P1/Content/P1/Characters/Monsters/Dusk_Minions/",
    "P1/Content/P1/Items/Equipment/Armors/Armor0R/SkeletalMeshes/PhysicsAssets05/",
)

# 게임 도메인 폴더가 사는 자리다. 이 아래 **한 단계**만 대칭 판정의 대상이다.
CLIENT_GAME_ROOT = f"{CLIENT_SOURCE}/Game"
SERVER_GAME_ROOT = "Server/GameServer/Game"

# ADR-0007의 표를 그대로 옮긴 것이다
# (`docs/adr/0007-limit-folder-symmetry-to-shared-domains.md` 31~39줄).
#
# **이 표를 고칠 때는 ADR도 함께 고친다.** 한쪽만 고치면 이 검사가 빨강이 되므로 어긋난 채로
# 남지는 않지만, 어느 쪽이 의도인지는 사람이 정해야 한다.
#
# 표를 문서에서 파싱하지 않는 이유가 있다. `docs/folder-structure.md`의 클라이언트 표는
# `Game/` 하위를 여덟 개 적는데 `Game/Items/`와 `Game/Interaction/`은 디스크에 없고 「아직
# 없다」 표시도 없다. 같은 문서 3.3의 의존 방향 화살표도 같다. ADR-0007의 표만이 디스크와
# 일치한다. 문서 쪽 어긋남은 `docs/tech-debt.md`에 있다.
#
# 짝이 `None`인 줄은 「한쪽에만 둔다」가 결정이라는 뜻이다. 빈 폴더를 만들어 맞추지 않는다.
GAME_DOMAIN_FOLDERS = (
    # (도메인, 게임 서버, 클라이언트)
    ("엔티티", "Entities", "Entities"),
    ("인벤토리", "Inventory", "Inventory"),
    ("장비", "Equipment", "Equipment"),
    ("게임 데이터", "Data", "Data"),
    ("룸", "Room", None),
    ("전투", None, "Combat"),
    ("월드 액터", None, "World"),
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


def run_git_ls_files(patterns: list[str], extra_flags: tuple[str, ...] = ()) -> list[str]:
    """git이 추적하는 파일 중 패턴에 맞는 것을 저장소 기준 경로로 돌려준다.

    `extra_flags`에 `--others --ignored --exclude-standard`를 주면 추적 파일 대신
    `.gitignore`가 무시하는 파일을 돌려준다. `project-item-exists`가 쓴다.
    """
    result = subprocess.run(
        ["git", "ls-files", "-z", *extra_flags, "--"] + patterns,
        cwd=REPO_ROOT,
        capture_output=True,
        check=True,
    )
    raw = result.stdout.decode("utf-8")
    return [entry for entry in raw.split("\0") if entry]


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
# 검사 4: 서버 멤버 변수의 `_camelCase` (`conventions.md` 3.2)
# ----------------------------------------------------------------------------------

# 멤버 변수 선언의 마지막 식별자를 잡는다. 초기화식과 배열 크기는 앞에서 잘라낸다.
MEMBER_NAME_RE = re.compile(r"([A-Za-z_][A-Za-z0-9_]*)[ \t]*$")

# 선언이 아닌 줄을 걸러낸다.
NON_MEMBER_KEYWORDS = (
    "using ", "typedef ", "friend ", "return ", "public:", "protected:", "private:",
    "template", "enum ", "namespace ", "extern ", "#", "static_assert",
)


def _member_candidates(body: str) -> list[tuple[int, str]]:
    """클래스 본문에서 멤버 변수로 보이는 선언의 (줄 오프셋, 이름)을 돌려준다."""
    found: list[tuple[int, str]] = []
    depth = 0
    for offset, raw_line in enumerate(body.split("\n")):
        line = raw_line.strip()
        opened = raw_line.count("{")
        closed = raw_line.count("}")

        # 중첩 블록(함수 본문, 내부 타입) 안은 보지 않는다. 멤버 선언은 깊이 0에만 있다.
        if depth > 0:
            depth += opened - closed
            continue
        depth += opened - closed

        if not line or line.startswith("//"):
            continue
        if any(line.startswith(keyword) for keyword in NON_MEMBER_KEYWORDS):
            continue
        if not line.endswith(";"):
            continue
        if "(" in line or ")" in line:
            # 함수 선언과 생성자다. 함수 포인터 멤버는 이 프로젝트에 없다.
            continue
        if line.startswith("class ") or line.startswith("struct "):
            # 전방 선언이다.
            continue

        declaration = line[:-1]
        # 초기화식을 잘라낸다. `int32 _roomId = 0;`에서 이름만 남긴다.
        declaration = declaration.split("=", 1)[0]
        # 배열 크기를 잘라낸다.
        declaration = declaration.split("[", 1)[0]
        declaration = declaration.rstrip()

        # 상수는 SCREAMING_SNAKE_CASE 규칙(3.4)을 따르므로 이 검사의 대상이 아니다.
        tokens = declaration.replace("*", " ").replace("&", " ").split()
        if "const" in tokens or "constexpr" in tokens:
            continue
        if len(tokens) < 2:
            # 타입 없이 이름만 있는 줄이다. 매크로이거나 접근 지정자다.
            continue

        match = MEMBER_NAME_RE.search(declaration)
        if match is None:
            continue

        found.append((offset, match.group(1)))
    return found


def _record_bodies(text: str) -> list[tuple[str, str, int, str]]:
    """(종류, 타입 이름, 본문 첫 줄 번호, 본문)을 돌려준다.

    중첩 타입은 바깥 타입의 본문에 포함되므로 따로 세지 않는다. 멤버 후보를 뽑는 쪽이 중첩
    블록을 건너뛴다.
    """
    records: list[tuple[str, str, int, str]] = []
    cursor = 0
    while True:
        match = CLASS_DEF_RE.search(text, cursor)
        if match is None:
            return records

        kind, name = match.group(1), match.group(2)
        # 매칭의 마지막 문자가 여는 중괄호다. 그 다음부터 본문이다.
        body_start = match.end()
        depth = 1
        index = body_start
        while index < len(text) and depth > 0:
            if text[index] == "{":
                depth += 1
            elif text[index] == "}":
                depth -= 1
            index += 1

        body = text[body_start : index - 1]
        first_body_line = text.count("\n", 0, body_start) + 1
        records.append((kind, name, first_body_line, body))
        cursor = index


def _is_plain_data_struct(kind: str, body: str) -> bool:
    """필드만 담고 동작이 없는 순수 데이터 구조체인지 본다 (3.2의 예외)."""
    if kind != "struct":
        return False
    return "(" not in body


def check_server_member_names(_: argparse.Namespace) -> list[Violation]:
    """게임 서버와 더미 클라이언트의 멤버 변수 이름을 본다."""
    patterns = []
    for root in SERVER_SOURCE_ROOTS:
        patterns.extend([f"{root}/**/*.h", f"{root}/**/*.cpp"])

    violations: list[Violation] = []
    for rel_path in run_git_ls_files(patterns):
        if is_generated(rel_path):
            continue

        text = strip_comments_and_strings(read_text(rel_path))
        for kind, type_name, start_line, body in _record_bodies(text):
            if _is_plain_data_struct(kind, body):
                continue
            if (rel_path, type_name) in PENDING_SERVER_MEMBER_DEBT:
                continue

            for offset, member in _member_candidates(body):
                if member in WINSOCK_MEMBER_NAMES:
                    continue
                if re.fullmatch(r"s_[a-z][A-Za-z0-9]*", member):
                    continue
                if re.fullmatch(r"_[a-z][A-Za-z0-9]*", member):
                    continue

                violations.append(
                    Violation(
                        rel_path,
                        start_line + offset,
                        f"{type_name}의 멤버 '{member}'이 '_camelCase'가 아니다. "
                        f"static 멤버는 's_'를 붙인다 (conventions.md 3.2)",
                    )
                )
    return violations


# ----------------------------------------------------------------------------------
# 검사 5: 추적 중인 에셋의 접두사 (`folder-structure.md` 4.3)
# ----------------------------------------------------------------------------------


def check_asset_prefix(_: argparse.Namespace) -> list[Violation]:
    """git이 추적하는 에셋의 이름이 접두사 표를 따르는지 본다."""
    violations: list[Violation] = []
    for rel_path in run_git_ls_files(["P1/Content/**/*.uasset", "P1/Content/**/*.umap"]):
        if any(rel_path.startswith(pack_dir) for pack_dir in PACK_COPY_DIRS):
            continue

        path = Path(rel_path)
        stem = path.stem
        prefix = stem.split("_", 1)[0] if "_" in stem else ""

        if path.suffix == ".umap":
            if prefix == LEVEL_PREFIX:
                continue
            violations.append(
                Violation(
                    rel_path,
                    0,
                    f"레벨 '{stem}'에 '{LEVEL_PREFIX}_' 접두사가 없다 "
                    f"(folder-structure.md 4.3)",
                )
            )
            continue

        if prefix in ASSET_PREFIXES:
            continue

        violations.append(
            Violation(
                rel_path,
                0,
                f"에셋 '{stem}'의 접두사가 표에 없다. 팩에서 복사해 온 것이면 "
                f"check_conventions.py의 PACK_COPY_DIRS에 그 폴더를 적는다 "
                f"(folder-structure.md 4.3)",
            )
        )
    return violations


# ----------------------------------------------------------------------------------
# 검사 6: `Game/` 하위 폴더의 클라이언트와 서버 대칭 (ADR-0007)
# ----------------------------------------------------------------------------------


def _game_subfolders(root: str) -> set[str]:
    """`root` 바로 아래의 폴더 이름을 git이 추적하는 파일 경로에서 모은다.

    디스크를 직접 훑지 않는다. self-test가 파일 목록만 바꿔 끼우는 구조라서, 디렉터리를
    직접 보면 픽스처로 덮을 수 없다.

    추적하는 파일이 하나도 없는 폴더는 여기 잡히지 않는다. 빈 폴더이거나 전부 무시되는
    폴더인데 어느 쪽이든 저장소의 내용이 아니다.
    """
    prefix = f"{root}/"
    folders: set[str] = set()
    for rel_path in run_git_ls_files([f"{root}/*", f"{root}/**/*"]):
        # 경로를 여기서 한 번 더 확인한다. self-test의 가짜 파일 목록은 패턴으로 거르지
        # 않고 픽스처를 통째로 주기 때문에, 이 확인이 없으면 반대편 티어의 파일이 섞인다.
        if not rel_path.startswith(prefix):
            continue
        parts = PurePosixPath(rel_path[len(prefix) :]).parts
        # 폴더 하나와 그 안의 파일 하나가 있어야 한 단계 아래의 폴더로 센다.
        if len(parts) >= 2:
            folders.add(parts[0])
    return folders


def check_folder_symmetry(_: argparse.Namespace) -> list[Violation]:
    """`Game/` 바로 아래 폴더 집합이 ADR-0007의 표와 같은지 본다.

    **양쪽의 이름을 서로 대조하지 않는다.** 그렇게 하면 판정이 순환한다. 이름이 같아야
    「양쪽에 다 있다」로 분류되므로, 이름이 같은 것만 남기고 이름을 비교하면 결과가 언제나
    참이다. 2026년 9월에 실제로 일어난 어긋남(서버 `Game/Object/` 대 클라이언트
    `Characters/`)이 그 방식으로는 「한쪽에만 있는 폴더 둘」이 되어 조용히 통과한다.

    그래서 양쪽을 각각 표와 대조한다. 표에 없는 이름이 생기는 것과 표가 요구하는 폴더가
    사라지는 것을 양방향으로 잡는다.
    """
    tiers = (
        (SERVER_GAME_ROOT, {row[1] for row in GAME_DOMAIN_FOLDERS if row[1]}),
        (CLIENT_GAME_ROOT, {row[2] for row in GAME_DOMAIN_FOLDERS if row[2]}),
    )

    violations: list[Violation] = []
    for root, expected in tiers:
        actual = _game_subfolders(root)

        for name in sorted(actual - expected):
            violations.append(
                Violation(
                    f"{root}/{name}",
                    0,
                    "ADR-0007의 표에 없는 폴더다. 반대편 티어와 이름을 맞추거나, "
                    "한쪽에만 두기로 정했다면 ADR-0007의 표와 이 스크립트의 "
                    "`GAME_DOMAIN_FOLDERS`에 줄을 더한다",
                )
            )

        for name in sorted(expected - actual):
            violations.append(
                Violation(
                    f"{root}/{name}",
                    0,
                    "ADR-0007의 표가 요구하는 폴더인데 추적하는 파일이 없다. "
                    "폴더를 옮기거나 지웠다면 ADR-0007의 표와 이 스크립트의 "
                    "`GAME_DOMAIN_FOLDERS`를 함께 고친다",
                )
            )
    return violations


# ----------------------------------------------------------------------------------
# 검사 7·8: 프로젝트 파일의 등록 항목 (#74)
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

    그래서 추적 중인 파일과 **`.gitignore`가 무시하는 파일**을 둘 다 실재로 친다. 무시되는
    파일은 저장소가 관리하지 않으므로 이 검사가 막으려는 사고(파일을 옮기고 프로젝트 파일을
    안 고치는 것)의 대상이 아니다.
    """
    tracked = {path.lower() for path in run_git_ls_files([])}
    ignored = {
        path.lower()
        for path in run_git_ls_files([], ("--others", "--ignored", "--exclude-standard"))
    }

    violations: list[Violation] = []
    for project_path in _project_files():
        text = read_text(project_path)
        for match in PROJECT_ITEM_RE.finditer(text):
            kind, include = match.group(1), match.group(2)
            target = _resolve_project_item(project_path, include)
            if target is None:
                continue
            if target.lower() in tracked or target.lower() in ignored:
                continue

            line = text.count("\n", 0, match.start()) + 1
            violations.append(
                Violation(
                    project_path,
                    line,
                    f'<{kind} Include="{include}">가 가리키는 "{target}"이 저장소에 없다',
                )
            )
    return violations


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
    "server-member": ("서버 멤버 변수의 _camelCase", check_server_member_names),
    "asset-prefix": ("추적 중인 에셋의 접두사", check_asset_prefix),
    "folder-symmetry": ("Game/ 하위 폴더의 클라·서버 대칭", check_folder_symmetry),
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
    "server-member": {
        "Server/GameServer/Game/Room/Room.h": (
            "#pragma once\n"
            "class Room : public JobQueue\n"
            "{\n"
            "public:\n"
            "    void Init();\n"
            "private:\n"
            "    int32 roomId = 0;\n"
            "};\n"
        ),
    },
    "asset-prefix": {
        "P1/Content/P1/UI/Screens/Inventory.uasset": None,
    },
    # 2026년 9월에 실제로 있던 어긋남이다. 서버가 엔티티 폴더를 `Object/`로 부르고 있고
    # 클라이언트는 `Entities/`로 부른다. 잡혀야 할 위반은 둘이다. 표에 없는 `Object/`가
    # 생긴 것과, 표가 요구하는 `Entities/`가 서버에 없는 것이다.
    "folder-symmetry": {
        f"{SERVER_GAME_ROOT}/Object/Object.cpp": None,
        f"{SERVER_GAME_ROOT}/Inventory/Inventory.cpp": None,
        f"{SERVER_GAME_ROOT}/Equipment/EquippedGear.cpp": None,
        f"{SERVER_GAME_ROOT}/Data/Gamedata.cpp": None,
        f"{SERVER_GAME_ROOT}/Room/Room.cpp": None,
        f"{CLIENT_GAME_ROOT}/Entities/P1Player.cpp": None,
        f"{CLIENT_GAME_ROOT}/Inventory/P1InventoryComponent.cpp": None,
        f"{CLIENT_GAME_ROOT}/Equipment/P1EquipmentComponent.cpp": None,
        f"{CLIENT_GAME_ROOT}/Data/P1ItemData.h": None,
        f"{CLIENT_GAME_ROOT}/Combat/P1AttackSystemComponent.cpp": None,
        f"{CLIENT_GAME_ROOT}/World/P1Portal.cpp": None,
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
    "server-member": {
        "Server/GameServer/Game/Room/Room.h": (
            "#pragma once\n"
            "class Room : public JobQueue\n"
            "{\n"
            "public:\n"
            "    void Init();\n"
            "private:\n"
            "    int32 _roomId = 0;\n"
            "    static atomic<int64> s_idGenerator;\n"
            "};\n"
            "struct RoomEnterData\n"
            "{\n"
            "    int32 nextRoomId = 0;\n"
            "};\n"
        ),
    },
    "asset-prefix": {
        "P1/Content/P1/UI/Screens/WBP_Inventory.uasset": None,
        "P1/Content/P1/Maps/L_InGameMap.umap": None,
    },
    # 위 픽스처에서 서버의 `Object/`만 `Entities/`로 고친 것이다. 서버에만 있는 `Room/`과
    # 클라이언트에만 있는 `Combat/`·`World/`는 표가 허용하므로 위반이 아니다.
    # `Data/Json/`은 두 단계 아래라 판정 대상이 아니다.
    "folder-symmetry": {
        f"{SERVER_GAME_ROOT}/Entities/Entity.cpp": None,
        f"{SERVER_GAME_ROOT}/Inventory/Inventory.cpp": None,
        f"{SERVER_GAME_ROOT}/Equipment/EquippedGear.cpp": None,
        f"{SERVER_GAME_ROOT}/Data/Gamedata.cpp": None,
        f"{SERVER_GAME_ROOT}/Data/Json/S_Item.json": None,
        f"{SERVER_GAME_ROOT}/Room/Room.cpp": None,
        f"{CLIENT_GAME_ROOT}/Entities/P1Player.cpp": None,
        f"{CLIENT_GAME_ROOT}/Inventory/P1InventoryComponent.cpp": None,
        f"{CLIENT_GAME_ROOT}/Equipment/P1EquipmentComponent.cpp": None,
        f"{CLIENT_GAME_ROOT}/Data/P1ItemData.h": None,
        f"{CLIENT_GAME_ROOT}/Combat/P1AttackSystemComponent.cpp": None,
        f"{CLIENT_GAME_ROOT}/World/P1Portal.cpp": None,
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
    """파일 목록과 내용을 픽스처로 바꿔 끼우고 검사 하나를 돌린다."""
    global run_git_ls_files, read_text

    original_ls, original_read = run_git_ls_files, read_text

    def fake_ls(patterns: list[str], extra_flags: tuple[str, ...] = ()) -> list[str]:
        # 픽스처는 작으므로 패턴별로 거르지 않고 전부 준다. 검사 본체가 확장자와 경로로
        # 다시 거르기 때문에 이것으로 충분하다.
        #
        # 무시되는 파일을 묻는 호출에는 빈 목록을 준다. 픽스처에 `.gitignore`가 없으므로
        # 무시되는 파일도 없다.
        if "--ignored" in extra_flags:
            return []
        return sorted(fixture)

    def fake_read(rel_path: str) -> str:
        content = fixture.get(rel_path)
        if content is None:
            raise AssertionError(f"픽스처에 내용이 없는 파일을 읽으려 했다: {rel_path}")
        return content

    run_git_ls_files, read_text = fake_ls, fake_read
    try:
        return CHECKS[name][1](argparse.Namespace())
    finally:
        run_git_ls_files, read_text = original_ls, original_read


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
