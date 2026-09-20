# references

**현행 제약이 아닌 자료를 모아 둔다.** 여기 있는 문서를 판정 근거로 인용하지 않는다.

걷어낸 하네스 v1의 이력이 둘이다.

- `work/` — 세션별 작업 기록 2개
- `decisions/` — ADR 3개와 색인. `status: accepted`로 적혀 있어도 지금 코드를 구속하지 않는다

진행 중인 작업의 입력 자료가 하나다.

- `newConvention/` — 폴더 구조와 컨벤션 이행의 입력이 된 참고 문서 2개. 새 프로젝트 시작을
  전제로 쓰여 있고 이 저장소가 택하지 않은 분기를 절반쯤 담고 있다. **판정 기준은
  `docs/conventions.md`와 `docs/folder-structure.md`다.** 이행이 끝나면 이 폴더를 지운다

ADR-0002가 담고 있던 테스트 인프라 근거는 `docs/testing.md`에, 나머지 현행 제약도 각각
`docs/ARCHITECTURE.md`·`docs/build.md`·`docs/tech-debt.md`의 본문에 옮겼다. 여기 있는 문서와
본문이 어긋나면 **본문이 맞다.**

하네스 기계 자체(스킬, 훅, 템플릿, 전 과정 해설)는 이 폴더에 없다. 저장소 밖으로 뺐다.
