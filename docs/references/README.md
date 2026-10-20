# references

**앞으로의 설계에 참고할 자료를 모아 둔다.** 현행 제약이 아니다. 새 기능이나 구조를 설계하기 시작할 때
읽고, 그 설계에 무엇을 가져갈지는 설계하는 쪽이 정한다.

- 여기 있는 자료를 판정 근거로 인용하지 않는다. 현행 제약은 `docs/ARCHITECTURE.md`와 `docs/adr/`,
  그 밖의 규범 문서 본문에 있다. 여기 있는 자료와 본문이 어긋나면 **본문이 맞다**
- 규범 문서와 ADR은 이 폴더를 언급하지 않는다. 설계에서 가져온 내용은 그 문서 안에 자기 완결적으로
  다시 쓴다. `Tools/DocLint/check_docs.py`의 `references-mention` 검사가 이 규칙을 본다
  — 규범을 읽는 쪽이 판정 근거를 찾아 이 폴더로 들어오면, 아직 정하지 않은 설계를 제약으로 읽게 된다.

## 자료

| 경로 | 내용 |
|---|---|
| `gamedb-extension.example.sql` | GameDB 확장 설계 메모. 지금 스키마에 없는 테이블(`CharacterExpansion`, `CharacterStatAlloc`, `CharacterBuff`, `AccountCurrency`, `CurrencyLog`)까지 담은 전체 그림이다. 실행하는 스크립트가 아니다 |
| `harness-v1/` | 2026-09에 걷어낸 하네스 v1의 이력. 설계 참고 자료가 아니라 기록으로 남겨 둔다. 상세는 `harness-v1/README.md` |
