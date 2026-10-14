---
paths:
  - "P1/Source/**"
---

# UE 클라이언트 코드

`[엔진 제약]`은 어기면 동작이 달라지는 규칙이고, 나머지는 이 프로젝트가 정한 규칙이다.
「이름과 파일」의 일부는 `Tools/ConventionLint`가 CI에서 검사한다.

- 호출자를 검색할 때는 `_Implementation` 접미사를 붙인 이름과 뗀 이름을 둘 다 검색한다. UE RPC와
  `BlueprintNativeEvent`는 선언과 구현의 이름이 달라서 한쪽만 찾으면 호출 사슬이 끊긴다.

## 이름과 파일

- `[엔진 제약]` 리플렉션 타입은 타입 접두사 뒤에 `P1`을 붙인다(`UP1InventoryWidget`). 처음 만들 때부터 붙인다
  — 리플렉션 이름은 전역에서 유일해야 한다. 나중에 이름을 바꾸면 그 클래스를 부모로 둔 블루프린트가 부모를 잃는다
- 이름을 바꾸며 `[CoreRedirects]`를 넣었으면 같은 작업 안에서 지운다. 참조 에셋을 모두 다시 저장하고, `Content`와 `Config`에 옛 이름이 남지 않은 것을 확인한 뒤 지운다. 그다음 에디터를 다시 띄워 경고가 없는지 본다
  — 남은 리다이렉트는 쌓이기만 하고, 옛 이름을 새 클래스에 쓸 수 없게 만든다. MCP로 다시 저장할 때의 함정은 `ue-mcp` 스킬에 있다
- 예외: 델리게이트 타입(`FOn<사건>`)과, 생성 템플릿이 이름을 그대로 부르는 `ClientPacketHandler`·`PacketSession`·`SendBuffer`·`PacketHeader`에는 `P1`을 붙이지 않는다. `PacketHeader`와 `FP1PacketHeader`는 서로 다른 구조체다
- 파일 하나에 타입 하나를 두고, 파일 이름을 주 타입 이름과 맞춘다. 그 타입 안에서만 쓰는 작은 열거형만 같은 파일에 둔다
- `#include`는 `"Game/Combat/P1AttackSystemComponent.h"`처럼 경로를 한정한다. 근거는 ADR 0005에 있다
- 로그는 `Utils/LogCategory.h`에 선언한 `LogP1*` 카테고리로 남긴다. 맞는 카테고리가 없으면 기능 단위로 하나 만든다. `LogTemp`은 필터로 걸러낼 수 없으므로 쓰지 않는다

## 클라이언트 주석

주석의 양과 내용은 `cpp-common.md`의 「주석」을 따른다. 여기에는 UE에서만 생기는 형식 제약을 적는다.

- `[엔진 제약]` `UPROPERTY`·`UFUNCTION`의 주석은 `/** */`로 쓴다. UHT가 바로 위 주석을 에디터 툴팁으로 수집한다
- `[엔진 제약]` 같은 이유로 섹션 표시는 반드시 `//~`로 시작한다. `// Combat`을 `UPROPERTY` 위에 두면 그 툴팁이 "Combat"이 된다

## 클래스 구성

- 기능을 컴포넌트나 서브시스템으로 빼는 조건은 둘이다. (1) 그 기능 섹션의 `UPROPERTY`가 3개를 넘고 다른 섹션의 코드가 그 값을 읽지 않는다. (2) 둘 이상의 액터 클래스가 같은 기능을 쓴다. 한 기능 섹션의 public 함수가 5개를 넘으면 이 조건을 확인한다
- 위젯은 `UP1UserWidget`을 상속한다. `UP1UserWidget`에서는 Tick을 끄고, 값 갱신은 델리게이트 구독으로 한다. 시각 구성은 WBP에, 로직과 `BindWidget` 선언은 C++에 둔다
- CommonUI와 GAS는 쓰지 않는다. CommonUI를 도입하면 `UP1UserWidget`의 부모를 `UCommonUserWidget`으로 바꾸고, `UP1ActivatableWidget`을 별도 계층으로 만든다
- `[엔진 제약]` 서브시스템 사이의 의존은 `Initialize`에서 `Collection.InitializeDependency<T>()`로 선언한다. 초기화 순서가 보장되지 않는다
