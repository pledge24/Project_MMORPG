"""저장소 문서를 텍스트로 검사한다.

문서에 걸리는 규칙 중 정규식과 파일 존재 확인만으로 판정할 수 있는 것을 본다. 낡은 내용을
찾거나 문서가 담을 것을 지키는지 판단하는 일은 이 스크립트가 하지 않는다. 그쪽은
`/my-doc-gardening` 스킬이 이 스크립트의 결과 위에 얹는다.

검사 항목과 근거는 아래와 같다.

| 검사 이름            | 무엇을 보는가                                  | 근거                                  |
| -------------------- | ---------------------------------------------- | ------------------------------------- |
| `doc-filename`       | 문서 파일 이름이 소문자와 대시인지             | `CLAUDE.md` 「컨벤션」                |
| `section-ref`        | 「절」로 가리킨 다른 문서의 절이 있는지        | 문서 전반의 상호 참조                 |
| `path-ref`           | 백틱으로 적은 저장소 경로가 있는지             | 문서 전반의 상호 참조                 |
| `backlog-ref`        | backlog 번호 참조가 실제 항목을 가리키는지     | `docs/backlog.md`                     |
| `references-mention` | 규범 문서와 ADR이 `docs/references`를 안 쓰는지 | `docs/references/README.md`           |
| `adr-format`         | ADR 파일 이름, 번호 연속, `status` 값          | `ADR-FORMAT.md`                       |
| `tech-debt-format`   | 개수 줄, 정렬, 항목 양식과 어휘                | `docs/tech-debt.md` 「작성 방법」     |
| `context-map-absent` | `CONTEXT-MAP.md`가 없는지                      | `CLAUDE.md` 「도메인 문서」           |
| `encoding`           | `.md`는 UTF-8, `.proto`·`.bat`은 cp949인지     | `docs/conventions.md` 1.3             |
| `doc-pointer`        | 규범 문서마다 `CLAUDE.md`에 포인터가 있는지    | `CLAUDE.md` 「문서 위치」             |

사용법:

    py -3 Tools/DocLint/check_docs.py            # 전부 검사한다
    py -3 Tools/DocLint/check_docs.py --only path-ref
    py -3 Tools/DocLint/check_docs.py --list
    py -3 Tools/DocLint/check_docs.py --self-test

검사 대상은 git이 추적하는 파일뿐이다. 위반이 하나라도 있으면 종료 코드가 1이다.

**참조 검사(`section-ref`, `path-ref`, `backlog-ref`)는 ADR과 work 파일을 보지 않는다.**
두 문서는 결정 당시의 서술을 보존한다. ADR은 본문을 고치지 않고 날짜를 적은 덧붙임으로만
갱신하므로, 본문에 남은 옛 참조를 위반으로 잡으면 고칠 길이 없어 위반이 0건이 되지 않는다.
work 파일의 「기록」도 그때의 경로와 번호를 그대로 적는다. 두 문서의 낡은 참조는 스킬의
드리프트 축이 판단한다.
"""

from __future__ import annotations

import argparse
import fnmatch
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path, PurePosixPath

REPO_ROOT = Path(__file__).resolve().parents[2]
SELF_PATH = Path(__file__).resolve().relative_to(REPO_ROOT).as_posix()

# 파일 이름 규칙의 예외다. 루트의 관례 파일과 폴더 진입점이다 (`CLAUDE.md` 「컨벤션」).
# `CONTEXT.md`는 `domain-modeling` 스킬이 정한 이름이라 루트 관례 파일에 넣는다.
FILENAME_EXCEPTIONS = frozenset({"CLAUDE.md", "CONTEXT.md", "README.md", "docs/ARCHITECTURE.md"})

# 저장소 경로로 보고 존재를 확인할 백틱 토큰의 시작이다. 최상위 폴더와 루트 문서만 둔다.
# 이 밖의 토큰(`Room.cpp`, `Game/Combat/`)은 기준 폴더를 알 수 없어서 판정하지 않는다.
PATH_PREFIXES = (
    "docs/",
    "P1/",
    "Server/",
    "Tools/",
    "Protocol/",
    "DesignData/",
    ".claude/",
    ".github/",
    ".githooks/",
)
ROOT_FILES = ("CLAUDE.md", "CONTEXT.md", "README.md", ".gitignore", ".gitattributes")

# 빌드 산출물의 루트 폴더다. 하위의 `Debug/`, `Release/`만 무시 규칙에 걸리고 이 폴더 자체는
# 규칙에 이름이 없어서, 「무시하기로 한 경로」로 판정되지 않는다 (`Server/.gitignore`).
BUILD_OUTPUT_DIRS = ("Server/Binary",)

# ADR의 `status` 값이다 (`ADR-FORMAT.md` 「Optional sections」).
ADR_STATUS_RE = re.compile(r"^(proposed|accepted|deprecated|superseded by ADR-\d{4})$")

TECH_DEBT_LEVELS = ("높음", "중간", "낮음")
TECH_DEBT_SCOPES = ("함수", "파일", "모듈", "기능", "프로젝트")
TECH_DEBT_AREAS = ("client", "server", "protocol", "shared", "build", "ops")


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

    파일이 없는 환경에서도 판정이 같도록 `--no-index`로 규칙만 대조한다. 이유는
    `Tools/ConventionLint/check_conventions.py`의 같은 함수에 적혀 있다.
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


def read_bytes(rel_path: str) -> bytes:
    return (REPO_ROOT / rel_path).read_bytes()


def read_text(rel_path: str) -> str:
    """저장소의 텍스트 파일을 읽는다. 인코딩이 섞여 있어도 검사가 멈추지 않게 한다."""
    data = read_bytes(rel_path)
    try:
        return data.decode("utf-8")
    except UnicodeDecodeError:
        return data.decode("cp949", errors="replace")


# ----------------------------------------------------------------------------------
# 문서 묶음
# ----------------------------------------------------------------------------------


def tracked_markdown() -> list[str]:
    return sorted(path for path in run_git_ls_files(["*.md"]) if path.endswith(".md"))


def normative_docs() -> list[str]:
    """규범 문서다. 루트의 `CLAUDE.md`·`CONTEXT.md`, `docs/` 바로 아래와 `docs/agents/`의 문서."""
    docs = []
    for path in tracked_markdown():
        parts = PurePosixPath(path).parts
        if path in ("CLAUDE.md", "CONTEXT.md"):
            docs.append(path)
        elif len(parts) == 2 and parts[0] == "docs":
            docs.append(path)
        elif len(parts) == 3 and parts[:2] == ("docs", "agents"):
            docs.append(path)
    return docs


def adr_docs() -> list[str]:
    return [path for path in tracked_markdown() if PurePosixPath(path).parent.as_posix() == "docs/adr"]


def code_lines(text: str) -> list[tuple[int, str]]:
    """코드 펜스 밖의 줄을 줄 번호와 함께 돌려준다.

    펜스 안은 예시이거나 양식이다. `docs/tech-debt.md`의 새 항목 양식이 실제 항목처럼 보이고,
    예시 경로는 실재하지 않는다.
    """
    lines: list[tuple[int, str]] = []
    in_fence = False
    for number, line in enumerate(text.splitlines(), 1):
        if line.lstrip().startswith("```"):
            in_fence = not in_fence
            continue
        if not in_fence:
            lines.append((number, line))
    return lines


HEADING_RE = re.compile(r"^(#{1,6})\s+(.*?)\s*#*\s*$")
SECTION_NUMBER_RE = re.compile(r"^\d+(?:\.\d+)*\.?\s+")


def headings(text: str) -> list[tuple[int, int, str]]:
    """코드 펜스 밖의 제목을 (줄 번호, 수준, 제목) 목록으로 돌려준다."""
    found = []
    for number, line in code_lines(text):
        match = HEADING_RE.match(line)
        if match:
            found.append((number, len(match.group(1)), match.group(2)))
    return found


def normalize_title(title: str) -> str:
    """제목을 비교할 수 있게 다듬는다. 절 번호, 백틱, 겹친 공백을 지운다."""
    title = SECTION_NUMBER_RE.sub("", title.strip())
    title = title.replace("`", "")
    return re.sub(r"\s+", " ", title).strip()


# ----------------------------------------------------------------------------------
# 검사 1: 문서 파일 이름 (`CLAUDE.md` 「컨벤션」)
# ----------------------------------------------------------------------------------

DOC_FILENAME_RE = re.compile(r"^[a-z0-9]+(?:[-.][a-z0-9]+)*\.md$")


def check_doc_filename(_: argparse.Namespace) -> list[Violation]:
    """루트와 `docs/`의 문서 파일 이름이 소문자와 대시인지 본다.

    `.claude/skills/`는 보지 않는다. 스킬 폴더는 상류 관례(`SKILL.md`, `ADR-FORMAT.md`)를 따른다.
    """
    violations = []
    for path in tracked_markdown():
        in_root = "/" not in path
        if not (in_root or path.startswith("docs/")):
            continue
        if path in FILENAME_EXCEPTIONS or PurePosixPath(path).name == "README.md":
            continue
        if DOC_FILENAME_RE.match(PurePosixPath(path).name):
            continue
        violations.append(
            Violation(path, 0, "문서 파일 이름은 소문자와 대시로 쓴다 (CLAUDE.md 「컨벤션」)")
        )
    return violations


# ----------------------------------------------------------------------------------
# 검사 2: 다른 문서의 절 참조
# ----------------------------------------------------------------------------------

# 문서 이름 바로 뒤에 「절」이 붙은 참조다. `CLAUDE.md` 「컨벤션」, `docs/build.md`의 「빌드 명령」.
# 문서 이름 없이 쓴 「」는 상태 값(「머지」)이나 인용일 수 있어서 판정하지 않는다.
SECTION_REF_RE = re.compile(
    r"`?((?:[\w.-]+/)*[\w.-]+\.md)`?\s*(?:의\s*)?((?:「[^」\n]+」\s*(?:[·,와과]\s*)?)+)"
)


def resolve_doc(name: str, markdown: list[str]) -> str | None:
    """문서 이름을 추적 중인 문서 경로로 푼다. `build.md`처럼 폴더를 뺀 이름도 받는다."""
    if name in markdown:
        return name
    candidates = [path for path in markdown if path.endswith("/" + name)]
    return candidates[0] if len(candidates) == 1 else None


def check_section_ref(_: argparse.Namespace) -> list[Violation]:
    """「절」로 가리킨 다른 문서의 절 제목이 실제로 있는지 본다.

    절 번호와 백틱은 비교에서 뺀다. 「저장소 최상위」는 `## 2. 저장소 최상위`와 맞는다.
    """
    markdown = tracked_markdown()
    titles_cache: dict[str, set[str]] = {}
    violations = []

    for doc in normative_docs():
        for number, line in code_lines(read_text(doc)):
            for match in SECTION_REF_RE.finditer(line):
                target = resolve_doc(match.group(1), markdown)
                if target is None:
                    # 문서가 없는 것은 `path-ref`가 잡는다. 백틱 없이 쓴 이름은 거기서도 안 본다.
                    continue
                if target not in titles_cache:
                    # 줄표 뒤에 부연을 단 제목(`엔진 제약 — 무언가를 계획하기 전에`)은 앞부분으로도
                    # 가리킨다.
                    titles = set()
                    for _, _, title in headings(read_text(target)):
                        titles.add(normalize_title(title))
                        titles.add(normalize_title(title.split(" — ")[0]))
                    titles_cache[target] = titles
                titles = titles_cache[target]
                for section in re.findall(r"「([^」\n]+)」", match.group(2)):
                    if normalize_title(section) in titles:
                        continue
                    violations.append(
                        Violation(doc, number, f"{target}에 「{section}」 절이 없다")
                    )
    return violations


# ----------------------------------------------------------------------------------
# 검사 3: 저장소 경로 참조
# ----------------------------------------------------------------------------------

BACKTICK_RE = re.compile(r"`([^`\n]+)`")
LINE_SUFFIX_RE = re.compile(r":\d+(?:-\d+)?$")
BRACE_RE = re.compile(r"\{([^{}]*)\}")


def expand_braces(token: str) -> list[str]:
    """`Server/Binary/{Debug,Release}/`를 두 경로로 펼친다."""
    match = BRACE_RE.search(token)
    if match is None:
        return [token]
    head, tail = token[: match.start()], token[match.end() :]
    expanded = []
    for option in match.group(1).split(","):
        expanded.extend(expand_braces(head + option + tail))
    return expanded


def path_candidate(token: str) -> str | None:
    """백틱 토큰이 판정할 저장소 경로면 다듬어서 돌려준다. 아니면 `None`.

    경로로 보는 것은 최상위 폴더로 시작하고, 파일 확장자가 있거나 `/`로 끝나는 토큰이다.
    `P1/Network`처럼 둘 다 없는 토큰은 모듈 기준 상대 경로일 수 있어서 보지 않는다.
    `<이름>`, `NNNN` 같은 자리표시자가 든 토큰도 보지 않는다.
    """
    token = token.strip()
    if " " in token or "<" in token or "NNNN" in token:
        return None
    if not (token.startswith(PATH_PREFIXES) or token in ROOT_FILES):
        return None
    token = LINE_SUFFIX_RE.sub("", token)
    last = token.rstrip("/").rsplit("/", 1)[-1]
    if not (token.endswith("/") or "." in last or token in ROOT_FILES):
        return None
    return token


def check_path_ref(_: argparse.Namespace) -> list[Violation]:
    """규범 문서가 백틱으로 적은 저장소 경로가 실재하는지 본다.

    실재의 기준은 git이다. 추적 중인 파일과 폴더, 그리고 `.gitignore`가 무시하기로 한 경로를
    실재로 친다. 빌드 산출물과 로그(`P1/Saved/`, `docs/reports/`)는 저장소가 관리하지 않지만
    문서가 가리킬 이유가 있다.

    `docs/backlog.md`는 보지 않는다. 착수 전 작업 후보라서 아직 없는 경로를 적는다.
    """
    tracked = set(run_git_ls_files([]))
    folders = {
        "/".join(PurePosixPath(path).parts[:depth])
        for path in tracked
        for depth in range(1, len(PurePosixPath(path).parts))
    }

    unresolved: list[tuple[str, int, str, str]] = []
    for doc in normative_docs():
        if doc == "docs/backlog.md":
            continue
        for number, line in code_lines(read_text(doc)):
            for match in BACKTICK_RE.finditer(line):
                token = path_candidate(match.group(1))
                if token is None:
                    continue
                for path in expand_braces(token):
                    bare = path.rstrip("/")
                    if any(ch in bare for ch in "*?["):
                        if not fnmatch.filter(tracked, bare):
                            unresolved.append((doc, number, match.group(1), bare))
                        continue
                    if bare in tracked or bare in folders:
                        continue
                    if any(bare == d or bare.startswith(d + "/") for d in BUILD_OUTPUT_DIRS):
                        continue
                    unresolved.append((doc, number, match.group(1), bare))

    # 폴더는 그 아래 아무 경로나 무시되는지로 판정한다. `P1/Saved/`는 이름 자체가 아니라
    # `Saved/` 규칙에 걸린다.
    probes = sorted({row[3] for row in unresolved} | {row[3] + "/x" for row in unresolved})
    ignored = run_git_check_ignore(probes)

    return [
        Violation(doc, number, f"`{token}`이 가리키는 \"{path}\"이 저장소에 없다")
        for doc, number, token, path in unresolved
        if path not in ignored and path + "/x" not in ignored
    ]


# ----------------------------------------------------------------------------------
# 검사 4: backlog 번호 참조
# ----------------------------------------------------------------------------------

# `backlog.md` 7번, backlog 4번(제목), `docs/backlog.md`의 「5. 제목」
BACKLOG_NUMBER_RE = re.compile(r"backlog(?:\.md)?`?\s*(?:의\s*)?(\d+)번(?:\(([^)\n]+)\))?")
BACKLOG_TITLE_RE = re.compile(r"backlog(?:\.md)?`?\s*(?:의\s*)?「(\d+)\.\s*([^」\n]+)」")
# backlog 안에서 다른 항목을 가리키는 `10번(에디터 에셋 검증기)`. 괄호가 없는 `N번`은 backlog
# 항목이 아닐 수 있어서 보지 않는다.
BACKLOG_SELF_RE = re.compile(r"(?<![\d.])(\d+)번\(([^)\n]+)\)")
BACKLOG_ITEM_RE = re.compile(r"^(\d+)\.\s+(.*)$")

# backlog 번호를 적을 수 있는 코드와 설정 파일이다. 주석이 backlog 항목을 근거로 가리킨다.
BACKLOG_REF_CODE_PATTERNS = (".github/*", "P1/Scripts/*", "Tools/*", ".claude/hooks/*")
BACKLOG_REF_CODE_SUFFIXES = (".yml", ".yaml", ".ps1", ".py", ".bat", ".sh")


def check_backlog_ref(_: argparse.Namespace) -> list[Violation]:
    """backlog 번호 참조가 실제 `## N.` 항목을 가리키는지 본다. 제목을 같이 적었으면 제목도 본다.

    backlog는 항목을 지워도 번호를 다시 매기지 않는다. 그래서 지운 번호를 가리키는 참조는 끊긴
    참조로 남는다.
    """
    items: dict[str, str] = {}
    for _, level, title in headings(read_text("docs/backlog.md")):
        match = BACKLOG_ITEM_RE.match(title)
        if level == 2 and match:
            items[match.group(1)] = normalize_title(title)

    def judge(doc: str, number: int, item: str, title: str | None) -> Violation | None:
        if item not in items:
            return Violation(doc, number, f"backlog에 {item}번 항목이 없다")
        if title and normalize_title(title) not in items[item]:
            return Violation(
                doc, number, f"backlog {item}번은 「{items[item]}」이다. 참조한 제목은 「{title}」"
            )
        return None

    sources = normative_docs()
    sources += [
        path
        for path in run_git_ls_files(list(BACKLOG_REF_CODE_PATTERNS))
        # 이 스크립트는 뺀다. 자기 검증 픽스처가 일부러 끊긴 번호를 적는다.
        if path.endswith(BACKLOG_REF_CODE_SUFFIXES) and path != SELF_PATH
    ]

    violations = []
    for doc in sources:
        text = read_text(doc)
        lines = code_lines(text) if doc.endswith(".md") else list(enumerate(text.splitlines(), 1))
        for number, line in lines:
            found = [(m.group(1), m.group(2)) for m in BACKLOG_NUMBER_RE.finditer(line)]
            found += [(m.group(1), m.group(2)) for m in BACKLOG_TITLE_RE.finditer(line)]
            if doc == "docs/backlog.md":
                found += [(m.group(1), m.group(2)) for m in BACKLOG_SELF_RE.finditer(line)]
            for item, title in dict.fromkeys(found):
                violation = judge(doc, number, item, title)
                if violation:
                    violations.append(violation)
    return violations


# ----------------------------------------------------------------------------------
# 검사 5: `docs/references` 언급 (`docs/references/README.md`)
# ----------------------------------------------------------------------------------

REFERENCES_RE = re.compile(r"docs/references\b")


def check_references_mention(_: argparse.Namespace) -> list[Violation]:
    """규범 문서와 ADR이 하네스 v1 이력을 가리키지 않는지 본다.

    `CLAUDE.md`는 뺀다. 문서 위치의 색인이라 이 폴더가 무엇인지 적을 자리다.
    """
    violations = []
    for doc in normative_docs() + adr_docs():
        if doc == "CLAUDE.md":
            continue
        for number, line in code_lines(read_text(doc)):
            if REFERENCES_RE.search(line):
                violations.append(
                    Violation(
                        doc,
                        number,
                        "규범 문서와 ADR은 docs/references를 가리키지 않는다. "
                        "필요한 내용은 본문에 옮긴다 (docs/references/README.md)",
                    )
                )
    return violations


# ----------------------------------------------------------------------------------
# 검사 6: ADR 형식 (`ADR-FORMAT.md`)
# ----------------------------------------------------------------------------------

ADR_FILENAME_RE = re.compile(r"^(\d{4})-[a-z0-9]+(?:-[a-z0-9]+)*\.md$")
FRONTMATTER_RE = re.compile(r"\A---\n(.*?)\n---\n", re.DOTALL)


def check_adr_format(_: argparse.Namespace) -> list[Violation]:
    """ADR 파일 이름, 번호 연속, `status` 값을 본다. `status`는 선택이라 없어도 된다."""
    violations = []
    numbers: dict[int, str] = {}
    for doc in adr_docs():
        name = PurePosixPath(doc).name
        match = ADR_FILENAME_RE.match(name)
        if match is None:
            violations.append(Violation(doc, 0, "ADR 파일 이름은 NNNN-slug.md로 쓴다"))
            continue
        number = int(match.group(1))
        if number in numbers:
            violations.append(Violation(doc, 0, f"ADR 번호 {number:04d}이 {numbers[number]}와 겹친다"))
        numbers.setdefault(number, doc)

        text = read_text(doc).replace("\r\n", "\n")
        front = FRONTMATTER_RE.match(text)
        if front is None:
            continue
        for offset, line in enumerate(front.group(1).splitlines(), 2):
            key, _, value = line.partition(":")
            if key.strip() == "status" and not ADR_STATUS_RE.match(value.strip()):
                violations.append(
                    Violation(
                        doc,
                        offset,
                        f"status 값 '{value.strip()}'이 어휘에 없다. "
                        "proposed, accepted, deprecated, superseded by ADR-NNNN 중 하나다",
                    )
                )

    expected = 1
    for number in sorted(numbers):
        if number != expected:
            violations.append(
                Violation(numbers[number], 0, f"ADR 번호가 {expected:04d}을 건너뛰었다")
            )
        expected = number + 1
    return violations


# ----------------------------------------------------------------------------------
# 검사 7: tech-debt 양식 (`docs/tech-debt.md` 「작성 방법」)
# ----------------------------------------------------------------------------------

COUNT_LINE_RE = re.compile(r"^항목 (\d+)개 \(높음 (\d+) · 중간 (\d+) · 낮음 (\d+)\)$")
META_RE = re.compile(
    r"^> \*\*심각도:\*\* (\S+) · \*\*난이도:\*\* (\S+) · \*\*범위:\*\* (\S+) · (\S+)\s*$"
)
REGISTERED_RE = re.compile(r"^> 등록일: \d{4}년 \d{1,2}월 \d{1,2}일")


@dataclass
class DebtItem:
    title: str
    line: int
    meta: tuple[str, str, str, str] | None
    body: list[str]


def parse_debt_items(text: str) -> tuple[tuple[int, str] | None, list[DebtItem]]:
    """개수 줄과 항목을 읽는다. 항목은 `> **심각도:**` 줄이 바로 뒤따르는 `##` 제목이다."""
    lines = code_lines(text)
    count_line = next(((n, l) for n, l in lines if COUNT_LINE_RE.match(l)), None)
    items: list[DebtItem] = []
    current: DebtItem | None = None
    for index, (number, line) in enumerate(lines):
        if line.startswith("## "):
            # 항목이 아닌 `##`(`## 작성 방법`)가 나오면 앞 항목의 본문이 끝난다.
            current = None
            nxt = lines[index + 1][1] if index + 1 < len(lines) else ""
            if nxt.startswith("> **심각도:**"):
                match = META_RE.match(nxt)
                current = DebtItem(line[3:].strip(), number, match.groups() if match else None, [])
                items.append(current)
            continue
        if line.strip() == "---":
            current = None
            continue
        if current is not None:
            current.body.append(line)
    return count_line, items


def check_tech_debt_format(_: argparse.Namespace) -> list[Violation]:
    """개수 줄과 실제 항목, 정렬, 항목마다 양식과 어휘를 본다."""
    doc = "docs/tech-debt.md"
    count_line, items = parse_debt_items(read_text(doc))
    violations = []

    for item in items:
        if item.meta is None:
            violations.append(
                Violation(doc, item.line + 1, f"「{item.title}」의 심각도 줄이 양식과 다르다")
            )
            continue
        severity, difficulty, scope, area = item.meta
        for label, value, vocabulary in (
            ("심각도", severity, TECH_DEBT_LEVELS),
            ("난이도", difficulty, TECH_DEBT_LEVELS),
            ("범위", scope, TECH_DEBT_SCOPES),
            ("영역", area, TECH_DEBT_AREAS),
        ):
            if value not in vocabulary:
                violations.append(
                    Violation(doc, item.line + 1, f"「{item.title}」의 {label} '{value}'이 어휘에 없다")
                )
        if not any(line.startswith("> 위치:") for line in item.body):
            violations.append(Violation(doc, item.line, f"「{item.title}」에 위치 줄이 없다"))
        if not any(REGISTERED_RE.match(line) for line in item.body):
            violations.append(
                Violation(doc, item.line, f"「{item.title}」에 「등록일: YYYY년 M월 D일」 줄이 없다")
            )
        if not any(line.strip() == "### 영향" for line in item.body):
            violations.append(Violation(doc, item.line, f"「{item.title}」에 「영향」 절이 없다"))

    rank = {level: index for index, level in enumerate(TECH_DEBT_LEVELS)}
    ordered = [item for item in items if item.meta and item.meta[0] in rank and item.meta[1] in rank]
    for previous, current in zip(ordered, ordered[1:]):
        key_previous = (rank[previous.meta[0]], -rank[previous.meta[1]])
        key_current = (rank[current.meta[0]], -rank[current.meta[1]])
        if key_current < key_previous:
            violations.append(
                Violation(
                    doc,
                    current.line,
                    f"「{current.title}」이 「{previous.title}」보다 위에 와야 한다. "
                    "심각도가 높은 항목, 같으면 난이도가 낮은 항목을 위에 둔다",
                )
            )

    if count_line is None:
        violations.append(Violation(doc, 0, "맨 위에 「항목 N개 (높음 N · 중간 N · 낮음 N)」 줄이 없다"))
    else:
        number, line = count_line
        written = tuple(int(value) for value in COUNT_LINE_RE.match(line).groups())
        severities = [item.meta[0] for item in items if item.meta]
        actual = (
            len(items),
            severities.count("높음"),
            severities.count("중간"),
            severities.count("낮음"),
        )
        if written != actual:
            violations.append(
                Violation(
                    doc,
                    number,
                    f"개수 줄이 실제와 다르다. 실제는 항목 {actual[0]}개 "
                    f"(높음 {actual[1]} · 중간 {actual[2]} · 낮음 {actual[3]})",
                )
            )
    return violations


# ----------------------------------------------------------------------------------
# 검사 8: `CONTEXT-MAP.md`가 없는지 (`CLAUDE.md` 「도메인 문서」)
# ----------------------------------------------------------------------------------


def check_context_map_absent(_: argparse.Namespace) -> list[Violation]:
    """이 저장소는 single-context다. `CONTEXT-MAP.md`가 생기면 스킬이 multi-context로 읽는다."""
    return [
        Violation(path, 0, "single-context 저장소에 CONTEXT-MAP.md가 있다 (CLAUDE.md 「도메인 문서」)")
        for path in tracked_markdown()
        if PurePosixPath(path).name == "CONTEXT-MAP.md"
    ]


# ----------------------------------------------------------------------------------
# 검사 9: 인코딩 (`docs/conventions.md` 1.3)
# ----------------------------------------------------------------------------------

UTF8_BOM = b"\xef\xbb\xbf"


def check_encoding(_: argparse.Namespace) -> list[Violation]:
    """`.md`는 BOM 없는 UTF-8, `.proto`와 `.bat`은 cp949인지 본다.

    cp949 파일을 UTF-8로 다시 저장하면 그 파일을 읽는 생성기와 `cmd`가 깨진다. ASCII만 든
    파일은 두 인코딩이 같으므로 판정하지 않는다. 벤더링한 라이브러리의 파일이 여기 해당한다.
    """
    violations = []
    for path in run_git_ls_files(["*.md", "*.proto", "*.bat"]):
        data = read_bytes(path)
        if path.endswith(".md"):
            if data.startswith(UTF8_BOM):
                violations.append(Violation(path, 0, "UTF-8 BOM이 있다. BOM 없이 저장한다"))
                continue
            try:
                data.decode("utf-8")
            except UnicodeDecodeError as error:
                violations.append(Violation(path, 0, f"UTF-8이 아니다 ({error.reason})"))
            continue

        if all(byte < 0x80 for byte in data):
            continue
        try:
            data.decode("utf-8")
            violations.append(
                Violation(path, 0, "UTF-8로 저장돼 있다. .proto와 .bat은 cp949다 (conventions.md 1.3)")
            )
            continue
        except UnicodeDecodeError:
            pass
        try:
            data.decode("cp949")
        except UnicodeDecodeError:
            violations.append(Violation(path, 0, "cp949로 읽히지 않는다 (conventions.md 1.3)"))
    return violations


# ----------------------------------------------------------------------------------
# 검사 10: `CLAUDE.md`의 문서 포인터 (`CLAUDE.md` 「문서 위치」)
# ----------------------------------------------------------------------------------


def check_doc_pointer(_: argparse.Namespace) -> list[Violation]:
    """`docs/`의 규범 문서마다 `CLAUDE.md`가 경로를 적었는지 본다.

    포인터가 없는 문서는 에이전트가 언제 읽어야 하는지 모른다. 새 규범 문서를 만들고
    `CLAUDE.md`에 줄을 더하지 않는 일을 막는다.
    """
    index = read_text("CLAUDE.md")
    return [
        Violation(doc, 0, f"CLAUDE.md에 `{doc}` 포인터가 없다. 언제 읽는지와 함께 적는다")
        for doc in normative_docs()
        if doc.startswith("docs/") and doc not in index
    ]


# ----------------------------------------------------------------------------------
# 진입점
# ----------------------------------------------------------------------------------

CHECKS = {
    "doc-filename": ("문서 파일 이름", check_doc_filename),
    "section-ref": ("다른 문서의 절 참조", check_section_ref),
    "path-ref": ("저장소 경로 참조", check_path_ref),
    "backlog-ref": ("backlog 번호 참조", check_backlog_ref),
    "references-mention": ("docs/references 언급", check_references_mention),
    "adr-format": ("ADR 파일 이름과 status", check_adr_format),
    "tech-debt-format": ("tech-debt 개수 줄과 양식", check_tech_debt_format),
    "context-map-absent": ("CONTEXT-MAP.md 부재", check_context_map_absent),
    "encoding": ("문서와 cp949 파일의 인코딩", check_encoding),
    "doc-pointer": ("CLAUDE.md의 문서 포인터", check_doc_pointer),
}


# ----------------------------------------------------------------------------------
# 자기 검증
# ----------------------------------------------------------------------------------
#
# **검사가 아무것도 검사하지 않으면서 통과하는 상태를 잡는다.** 위반 0건과 대상 0건은 출력이
# 같다. 그래서 일부러 어긋낸 입력과 규칙을 지키는 입력을 검사에 먹여, 앞에서는 위반이 나오고
# 뒤에서는 나오지 않는지 본다. 저장소를 보는 함수 넷만 바꿔 끼우므로 검사 본체는 실제로 도는
# 코드 그대로다.

_BACKLOG = "# Backlog\n\n## 1. 첫 항목\n\n## 3. 셋째 항목\n"
_TECH_DEBT_HEAD = "# Tech Debt\n\n{count}\n\n## 작성 방법\n\n```markdown\n## 양식\n> **심각도:** 높음 · **난이도:** 중간 · **범위:** 기능 · client\n```\n\n---\n\n"


def _debt(title: str, severity: str, difficulty: str, area: str = "server") -> str:
    return (
        f"## {title}\n"
        f"> **심각도:** {severity} · **난이도:** {difficulty} · **범위:** 함수 · {area}\n"
        "> 위치: `Server/GameServer/Main/Global.cpp`\n"
        "> 등록일: 2026년 10월 3일\n\n"
        "설명.\n\n### 영향\n\n**변경 비용 증가** — 설명.\n\n"
    )


SELF_TEST_FIXTURES: dict[str, dict[str, str | bytes | None]] = {
    "doc-filename": {"docs/Tech_Debt.md": "# x\n"},
    "section-ref": {
        "docs/build.md": "# 빌드\n\n상세는 `CLAUDE.md` 「도구 라우팅」에 있다.\n",
        "CLAUDE.md": "# CLAUDE\n\n## 빌드와 도구\n",
    },
    "path-ref": {
        "docs/build.md": "스크립트는 `P1/Scripts/Gone.ps1`이다.\n",
        "CLAUDE.md": "# CLAUDE\n",
    },
    "backlog-ref": {
        "docs/backlog.md": _BACKLOG + "\n앞의 3번(첫 항목)과 겹친다.\n",
        "docs/build.md": "근거는 `docs/backlog.md` 5번에 있다.\n",
        "P1/Scripts/Backup.ps1": "# 근거: docs/backlog.md 7번\n",
        "CLAUDE.md": "# CLAUDE\n",
    },
    "references-mention": {
        "docs/ARCHITECTURE.md": "`docs/references/decisions/index.md` 참조.\n",
        "CLAUDE.md": "# CLAUDE\n",
    },
    "adr-format": {
        "docs/adr/0001-first.md": "---\nstatus: done\n---\n\n# 첫 결정\n",
        "docs/adr/0003-third.md": "# 셋째 결정\n",
    },
    "tech-debt-format": {
        "docs/tech-debt.md": _TECH_DEBT_HEAD.format(count="항목 3개 (높음 1 · 중간 0 · 낮음 1)")
        + _debt("낮은 것", "낮음", "낮음")
        + _debt("높은 것", "높음", "중간", area="backend"),
    },
    "context-map-absent": {"CONTEXT-MAP.md": "# map\n"},
    "encoding": {
        "Protocol/Schema/Enum.proto": "// 한국어 주석\n".encode("utf-8"),
        "docs/build.md": UTF8_BOM + "# 빌드\n".encode("utf-8"),
    },
    "doc-pointer": {
        "docs/testing.md": "# 테스트\n",
        "CLAUDE.md": "# CLAUDE\n\n- `docs/build.md`\n",
    },
}

# 규칙을 지키는 입력이다. 여기서 위반이 나오면 검사가 과하게 잡는 것이다.
SELF_TEST_CLEAN: dict[str, dict[str, str | bytes | None]] = {
    "doc-filename": {
        "docs/tech-debt.md": "# x\n",
        "docs/ARCHITECTURE.md": "# x\n",
        "docs/adr/0001-unify-build-path.md": "# x\n",
        "docs/work/2026-10-02-client-structure.md": "# x\n",
        "CLAUDE.md": "# x\n",
        # 스킬 폴더는 상류 관례를 따르므로 보지 않는다.
        ".claude/skills/domain-modeling/ADR-FORMAT.md": "# x\n",
    },
    "section-ref": {
        "docs/build.md": (
            "# 빌드\n\n상세는 `CLAUDE.md` 「빌드와 도구」에 있다.\n"
            "절 번호를 빼고 맞춘다: `docs/folder-structure.md`의 「저장소 최상위」.\n"
            "문서 이름이 없는 「머지」는 상태 값이라 보지 않는다.\n"
            "줄표 앞부분으로 가리킨다: `CLAUDE.md` 「엔진 제약」.\n"
        ),
        "CLAUDE.md": "# CLAUDE\n\n## 빌드와 도구\n\n### 엔진 제약 — 계획하기 전에 본다\n",
        "docs/folder-structure.md": "# 폴더\n\n## 2. 저장소 최상위\n",
    },
    "path-ref": {
        "docs/build.md": (
            "스크립트는 `P1/Scripts/Invoke-UeBuild.ps1`:12이다.\n"
            "산출물은 `Server/Binary/{Debug,Release}/`, 로그는 `P1/Saved/Logs/P1.log`다.\n"
            "원본은 `DesignData/Original_*.xlsx`, 모듈 상대 경로 `P1/Network`는 보지 않는다.\n"
            "```\nP1/Scripts/Example.ps1 처럼 펜스 안은 예시다 `P1/Gone.ps1`\n```\n"
        ),
        "docs/backlog.md": "새로 만들 `Protocol/Generate.ps1`.\n",
        "CLAUDE.md": "# CLAUDE\n",
        "P1/Scripts/Invoke-UeBuild.ps1": None,
        "DesignData/Original_Item.xlsx": None,
    },
    "backlog-ref": {
        "docs/backlog.md": _BACKLOG + "\n앞의 1번(첫 항목)과 겹친다.\n",
        "docs/build.md": "근거는 `docs/backlog.md`의 「3. 셋째 항목」과 backlog 1번에 있다.\n",
        "P1/Scripts/Backup.ps1": "# 근거: docs/backlog.md 3번\n",
        "CLAUDE.md": "# CLAUDE\n",
    },
    "references-mention": {
        "docs/codegen.md": "근거는 `docs/adr/0009-keep-asset-references-out-of-design-data.md`에 있다.\n",
        "CLAUDE.md": "- `docs/references/` — 하네스 v1의 이력\n",
    },
    "adr-format": {
        "docs/adr/0001-first.md": "---\nstatus: accepted\n---\n\n# 첫 결정\n",
        "docs/adr/0002-second.md": "---\nstatus: superseded by ADR-0003\n---\n\n# 둘째\n",
        "docs/adr/0003-third.md": "# 셋째 결정\n",
    },
    "tech-debt-format": {
        "docs/tech-debt.md": _TECH_DEBT_HEAD.format(count="항목 3개 (높음 1 · 중간 0 · 낮음 2)")
        + _debt("높은 것", "높음", "중간")
        + _debt("낮고 쉬운 것", "낮음", "낮음")
        + _debt("낮고 어려운 것", "낮음", "중간"),
    },
    "context-map-absent": {"CONTEXT.md": "# 용어집\n"},
    "encoding": {
        "Protocol/Schema/Enum.proto": "// 한국어 주석\n".encode("cp949"),
        "P1/Source/ProtobufCore/any.proto": b"syntax = \"proto3\";\n",
        "docs/build.md": "# 빌드\n".encode("utf-8"),
    },
    "doc-pointer": {
        "docs/testing.md": "# 테스트\n",
        "CLAUDE.md": "# CLAUDE\n\n- `docs/testing.md`\n",
    },
}


def _run_against_fixture(name: str, fixture: dict[str, str | bytes | None]) -> list[Violation]:
    """파일 목록과 내용을 픽스처로 바꿔 끼우고 검사 하나를 돌린다.

    값이 `None`인 파일은 목록에만 있고 내용은 읽지 않는다. 경로 존재만 보는 검사용이다.
    """
    global run_git_ls_files, read_bytes, run_git_check_ignore

    original = (run_git_ls_files, read_bytes, run_git_check_ignore)

    def fake_ls(patterns: list[str]) -> list[str]:
        # 패턴을 흉내 낸다. 검사 본체는 결과를 다시 거르지만, `*.md`처럼 확장자로 고른 목록에
        # 다른 파일이 섞이면 읽을 수 없는 파일을 읽게 된다.
        if not patterns:
            return sorted(fixture)
        return sorted(
            path
            for path in fixture
            if any(fnmatch.fnmatch(path, p) or fnmatch.fnmatch(PurePosixPath(path).name, p) for p in patterns)
        )

    def fake_read_bytes(rel_path: str) -> bytes:
        content = fixture.get(rel_path)
        if content is None:
            raise AssertionError(f"픽스처에 내용이 없는 파일을 읽으려 했다: {rel_path}")
        return content if isinstance(content, bytes) else content.encode("utf-8")

    def fake_check_ignore(paths: list[str]) -> set[str]:
        # 실제 `.gitignore`의 `Saved/` 규칙만 흉내 낸다.
        return {path for path in paths if "/Saved/" in path + "/"}

    run_git_ls_files, read_bytes, run_git_check_ignore = fake_ls, fake_read_bytes, fake_check_ignore
    try:
        return CHECKS[name][1](argparse.Namespace())
    finally:
        run_git_ls_files, read_bytes, run_git_check_ignore = original


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
            failures.append(f"{name}: 규칙을 지키는 입력을 위반으로 잡았다. {detail}")
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
    # 윈도우 콘솔의 기본 코드 페이지가 cp949라서 한국어 출력이 예외로 죽는다. 출력을 UTF-8로 고정한다.
    for stream in (sys.stdout, sys.stderr):
        stream.reconfigure(encoding="utf-8", errors="replace")

    parser = argparse.ArgumentParser(
        description="저장소 문서를 텍스트로 검사한다.",
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
            print(f"{name:<20} {description}")
        return 0

    if args.self_test:
        return run_self_test()

    selected = args.only or list(CHECKS)
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
