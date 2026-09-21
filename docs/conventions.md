# 코딩 컨벤션

코드 안의 규칙을 다룬다. 이름, 주석, 멤버 배치, 타입 사용이 여기 속한다.
**무엇을 어디에 둘지는 `docs/folder-structure.md`가 다룬다.**

규칙은 두 종류다.

- **엔진 제약**: 지키지 않으면 동작이 달라진다. 선택할 수 없다. `[엔진 제약]`으로 표시한다
- **프로젝트 규칙**: 이 프로젝트에서 정한 것이다. 근거를 함께 적는다

---

## 1. 세 티어에 공통으로 적용되는 것

### 1.1 주석과 로그는 한국어로 쓴다

코드를 고칠 때 주변 언어에 맞춘다. 영어 주석이 남아 있는 파일을 건드리면 그 파일의 주석도
한국어로 바꾼다.

이 규칙은 테스트 실행 경로에도 반영되어 있다. `Server/GameServerTests/TestMain.cpp`가
`gtest_main.cc` 대신 직접 `main()`을 두고 `SetConsoleOutputCP(CP_UTF8)`을 호출하는 이유가 이것이다.

### 1.2 TODO는 작성자를 적는다

```cpp
// TODO(Name): 내용
```

작성자가 없는 TODO는 언제 누가 왜 남겼는지 추적할 수 없다.

### 1.3 인코딩

`.proto`와 `.bat`은 cp949, 나머지는 전부 UTF-8이다. `.gitattributes`와 두 개의 `.editorconfig`가
이를 강제한다.

**cp949 파일의 한국어 주석을 UTF-8로 읽으면 깨져 보인다. 정상이다.** UTF-8로 다시 저장해서
"고치면" 이 파일들을 소비하는 도구 쪽이 깨진다.

---

## 2. 언리얼 클라이언트 C++

### 2.1 타입 접두사

`[엔진 제약]` UHT가 접두사로 타입을 판별한다.

| 대상 | 접두사 | 예 |
| --- | --- | --- |
| `UObject` 파생 | `U` | `UP1InventoryWidget` |
| `AActor` 파생 | `A` | `AP1Character` |
| 구조체 | `F` | `FP1ItemData` |
| 열거형 | `E` | `EP1EntityType` |
| 인터페이스 | `I` | `IP1Interactable` |
| bool 변수 | `b` | `bIsDead` |

리플렉션 대상이 아닌 클래스와 구조체에도 `F`를 붙인다. `FRunnable`을 상속한 워커, 세션, 송신
버퍼가 여기 해당한다.

### 2.2 타입 접두사 뒤에 `P1`을 붙인다

`[엔진 제약]` `UCLASS`는 네임스페이스 안에 둘 수 없다. 리플렉션 이름은 전역에서 유일해야 한다.
약어가 없으면 엔진과 플러그인의 클래스 이름과 충돌할 수 있다.

```
O  UP1InventoryWidget
X  UInventoryWidget
```

**새로 만드는 첫 번째 타입부터 약어를 붙인다.**
— 나중에 클래스 이름을 바꾸면 그 클래스를 부모로 삼은 블루프린트가 부모를 잃는다. 복구하려면
`DefaultEngine.ini`의 `[CoreRedirects]`에 항목을 직접 쓰고, 헤더와 `.cpp`의 파일 이름과 `#include`
구문과 클래스 선언을 모두 바꿔야 한다.

```ini
[CoreRedirects]
+ClassRedirects=(OldName="/Script/P1.InventoryWidget", NewName="/Script/P1.P1InventoryWidget")
```

**예외: 생성물과 생성물이 이름으로 부르는 타입에는 약어를 붙이지 않는다.**
— `Protocol/Templates/PacketHandler.h`가 아래 네 이름을 그대로 적는다. 템플릿은 한 벌뿐이고
같은 출력이 DummyClient로도 복사되므로, 클라 쪽만 이름을 바꾸면 생성기를 다시 돌리는 순간
빌드가 깨진다. 생성물 자체에 컨벤션을 적용하지 않는 이유는 `docs/folder-structure.md`에 있다.

| 타입 | 무엇인가 |
| --- | --- |
| `ClientPacketHandler` | 생성물이다. 파일도 생성기가 만든다 |
| `PacketSession` · `SendBuffer` · `PacketHeader` | 손으로 쓴 코드지만 생성물이 이름으로 부른다 |

같은 폴더의 `FP1PacketHeader`와 `FP1RecvWorker`와 `FP1SendWorker`는 템플릿이 부르지 않으므로
약어를 붙인다. **`PacketHeader`와 `FP1PacketHeader`는 서로 다른 구조체다.** 앞은 송신 버퍼를
바이트로 읽는 뷰이고, 뒤는 수신 경로가 `FArchive`로 직렬화하는 헤더다.

### 2.3 `UUW` 형태의 이중 접두사를 쓰지 않는다

`[엔진 제약]` 리플렉션 이름은 첫 글자 하나만 떼어낸 것이다. `AMyActor`의 리플렉션 이름은
`MyActor`다. `UUWInventory`로 선언하면 리플렉션 이름이 `UWInventory`가 되어 블루프린트 클래스
선택창과 에셋 검색에 그대로 노출된다.

역할은 접두사가 아니라 접미사로 나타낸다.

```
O  UP1InventoryWidget
X  UUWP1Inventory
```

### 2.4 파일 이름

파일 이름에는 타입 접두사를 붙이지 않는다. 프로젝트 약어는 남긴다.

```
클래스  UP1InventoryWidget
파일    P1InventoryWidget.h / P1InventoryWidget.cpp
```

**파일 하나에 타입 하나를 둔다.** 파일 이름과 그 안의 주 타입 이름이 일치해야 한다.
— 파일 이름으로 타입을 찾을 수 없으면 검색이 파일 열기로 바뀐다.

예외: 그 타입 안에서만 쓰이고 밖에서 이름으로 부르지 않는 작은 열거형은 같은 파일에 둔다.

### 2.5 헤더 포함 순서

`[엔진 제약]` `.generated.h`는 항상 마지막에 둔다.

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "P1InventoryWidget.generated.h"
```

### 2.6 `#include`는 경로를 한정한다

```cpp
O  #include "Combat/P1AttackSystemComponent.h"
X  #include "P1AttackSystemComponent.h"
```

— `#include` 줄에 도메인 이름이 드러나야 경계를 넘는 참조가 검색으로 잡힌다. 근거는
`docs/adr/0005-drop-include-path-flattening.md`에 있다.

예외: 생성물끼리 부르는 `#include`는 생성기가 만든다. 손으로 고치지 않는다.

### 2.7 로그는 선언한 카테고리로 남긴다

`Log/` 아래에 선언한 카테고리만 쓴다. **`LogTemp`을 쓰지 않는다.**
— `LogTemp`은 필터로 걸러낼 수 없다. 한 곳에서 쓰기 시작하면 로그 창에서 그 줄을 다시 찾을
방법이 없어진다.

카테고리 이름은 `LogP1`으로 시작한다. 엔진과 플러그인의 카테고리 이름과 섞이지 않게 한다.

선언된 카테고리 중 어디에도 맞지 않는 로그가 생기면 기능 단위로 카테고리를 하나 만든다. 파일마다
만들지 않는다.

### 2.8 헤더와 구현의 주석은 역할이 다르다

**`.h`에는 호출하는 쪽이 알아야 할 것을 적는다.** 적을 것은 아래 다섯 가지다.

- 단위와 값의 범위 (초인지 밀리초인지, 0에서 1 사이인지)
- 실행 권한 (게임 스레드 전용인지, 로컬 플레이어 전용인지)
- 호출 시점의 제약 (`BeginPlay` 이후에만 유효한지)
- 반환값이 `nullptr`일 수 있는지
- 이름에 드러나지 않는 부작용

```cpp
/** 초 단위 쿨다운이다. 0이면 쿨다운이 없다. */
UPROPERTY(EditDefaultsOnly, Category = "Combat")
float CooldownSeconds = 1.0f;

/** 맨손 상태에서는 nullptr을 반환한다. */
AP1Weapon* GetEquippedWeapon() const;
```

함수 이름으로 알 수 있는 내용을 반복하지 않는다.

```cpp
X  /** 데미지를 적용한다. */
X  void ApplyDamage(float Amount, AActor* Instigator);
```

**`.cpp`에는 고치는 쪽이 알아야 할 것을 적는다.** 헤더의 설명을 반복하지 않는다.

- 자명하지 않은 순서 의존성
- 엔진 동작을 우회하려고 넣은 코드
- 실수처럼 보이지만 의도한 코드
- 서버와 클라이언트의 동기화 타이밍 전제

```cpp
void AP1Character::BeginPlay()
{
    Super::BeginPlay();

    // 서버가 보낸 첫 상태 패킷보다 먼저 끝나야 한다.
    // PossessedBy에서 초기화하면 순서가 뒤집힌다.
    InitializeVisuals();
}
```

### 2.9 리플렉션 멤버의 주석은 `/** */`로 쓴다

`[엔진 제약]` UHT는 리플렉션 대상 멤버 바로 위의 주석을 ToolTip 메타데이터로 수집한다. 수집된
주석은 에디터 디테일 패널과 블루프린트에서 툴팁으로 보인다.

이 동작 때문에 `UPROPERTY`와 `UFUNCTION`의 주석 형식을 `/** */`로 통일한다. 주석을 쓰는 동시에
에디터 문서를 얻는다.

계약이 둘 이상이면 줄을 나눈다.

```cpp
/**
 * 서버에서 받은 상태를 적용한다.
 * 이미 적용한 Sequence보다 오래된 데이터는 버린다.
 */
void ApplyState(const FP1PlayerStateData& Data);
```

### 2.10 `@param`과 `@return`을 쓰지 않는다

Doxygen 문서를 생성하지 않는 동안에는 쓰지 않는다. 인자의 단위나 허용 범위를 적어야 하면 서술
문장에 넣는다.

Doxygen을 도입하면 이 규칙을 다시 정한다.

### 2.11 섹션 표시는 `//~`로 쓴다

`[엔진 제약]` UHT가 리플렉션 대상 멤버 바로 위의 주석을 툴팁으로 수집하기 때문에, `// Combat`
같은 표시를 `UPROPERTY` 위에 두면 그 `UPROPERTY`의 툴팁이 "Combat"이 된다. `//~`로 시작하는 줄은
수집에서 빠진다.

형태는 두 가지만 쓴다.

- `//~ Begin <TypeName> Interface` / `//~ End <TypeName> Interface` — 엔진 인터페이스 오버라이드
- `//~ <FeatureName>` — 기능 섹션

```cpp
//~ Begin AActor Interface
virtual void BeginPlay() override;
virtual void Tick(float DeltaSeconds) override;
//~ End AActor Interface

//~ Combat
UPROPERTY(EditDefaultsOnly, Category = "Combat")
float AttackPower = 10.0f;
```

`/*---- 이름 ----*/` 형태의 배너 주석을 쓰지 않는다.

### 2.12 클래스 멤버는 기능 단위로 묶는다

에픽 공식 표준은 클래스 전체를 public에서 private 순서로 배치하라고 안내한다. 이 프로젝트는 기능
섹션 **안에** 그 순서를 적용한다.

— 게임플레이 클래스 하나가 이동, 전투, 인벤토리를 함께 다룬다. 접근 지정자로만 정렬하면 전투
함수와 전투 변수가 파일의 양 끝으로 갈라져서, 기능 하나를 고칠 때마다 파일 전체를 오간다.

순서는 아래와 같다.

1. 생성자
2. 엔진 인터페이스 오버라이드
3. 기능 섹션. 각 섹션 안에서 public → protected → private, 함수 먼저 변수 나중

```cpp
UCLASS()
class P1_API AP1Character : public ACharacter
{
    GENERATED_BODY()

public:
    AP1Character();

    //~ Begin AActor Interface
    virtual void BeginPlay() override;
    //~ End AActor Interface

    //~ Combat
public:
    void ApplyDamage(float Amount, AActor* Instigator);

protected:
    void HandleDeath();

private:
    UPROPERTY()
    float Health = 100.0f;

    //~ Inventory
public:
    UP1InventoryComponent* GetInventory() const { return Inventory; }

private:
    UPROPERTY(VisibleAnywhere, Category = "Inventory")
    TObjectPtr<UP1InventoryComponent> Inventory;
};
```

**멤버 선언 순서를 바꿀 때 초기화 순서를 확인한다.**
— C++은 멤버를 선언 순서대로 초기화한다. 생성자 초기화 리스트나 클래스 내 초기화가 다른 멤버를
읽으면, 순서를 바꾸는 순간 읽는 값이 달라진다. 컴파일러가 경고하지 않는 경우가 있다.

### 2.13 public에는 외부에서 호출하는 것만 둔다

기능마다 public 블록이 새로 열리기 때문에 구조상 public이 늘어나기 쉽다.

한 기능 섹션의 public 함수가 5개를 넘으면 그 기능을 컴포넌트로 뺄 시점인지 판단한다
(→ 「2.16 분리 기준」).

### 2.14 UObject 포인터는 `TObjectPtr`로 선언한다

```cpp
UPROPERTY(VisibleAnywhere)
TObjectPtr<UP1InventoryComponent> Inventory;
```

컨테이너에 담을 때도 마찬가지다.

```cpp
UPROPERTY()
TMap<uint64, TObjectPtr<AP1Player>> Players;
```

### 2.15 위젯

**모든 위젯은 `UP1UserWidget`을 상속한다.** `UUserWidget`을 직접 상속하지 않는다.
— 로컬 플레이어 컨트롤러를 얻는 함수처럼 모든 위젯이 쓰는 접근자를 놓을 자리가 필요하다.

**`UP1UserWidget`에서 Tick을 켜지 않는다.**
— 모든 위젯의 부모에 Tick이 들어가면 위젯이 수십 개로 늘었을 때 원인을 찾기 어려운 프레임
저하로 돌아온다. 값 갱신은 델리게이트 구독으로 한다.

**시각 구성은 WBP에 두고 C++에는 로직과 `BindWidget` 선언만 둔다.**

```cpp
UPROPERTY(meta = (BindWidget))
TObjectPtr<UTextBlock> ItemNameText;
```

`[엔진 제약]` `BindWidget`은 WBP에 같은 이름의 위젯이 없으면 컴파일 단계에서 오류를 낸다.

**CommonUI는 지금 쓰지 않는다.** 도입하면 `UP1UserWidget`의 부모를 `UCommonUserWidget`으로 바꾸고,
화면과 모달을 담을 `UP1ActivatableWidget`을 그 옆에 만든다. 두 계층을 하나로 합치려는 시도는 하지
않는다.
— `UCommonActivatableWidget`이 `UCommonUserWidget`을 상속하므로 하나의 베이스가 두 계층의 부모가
될 수 없다.

### 2.16 분리 기준

기능 섹션으로 정리한 것만으로는 클래스가 커지는 것을 막지 못한다. 아래 두 조건 중 하나를
만족하면 그 기능을 컴포넌트나 서브시스템으로 뺀다.

**조건 1: 그 기능만 쓰는 상태를 가진다.** 그 기능 섹션의 `UPROPERTY`가 3개를 넘고, 다른 기능
섹션의 코드가 그 값을 읽지 않으면 분리한다.

예외: 두 기능이 같은 값을 공유하면 줄 수와 무관하게 분리하지 않는다.

**조건 2: 둘 이상의 액터 클래스가 같은 기능을 쓴다.** 몬스터와 플레이어가 같은 로직을 쓰면
컴포넌트로 뺀다. 한 클래스만 쓰면 그 클래스 안에 둔다.

외부 C++ 서버가 판정을 맡으므로 언리얼 리플리케이션을 쓰지 않는다. 컴포넌트는 사실상 클라이언트
상태 컨테이너이고 분리 비용이 낮다.

### 2.17 서브시스템 의존성을 `Initialize`에서 선언한다

`[엔진 제약]` `UGameInstanceSubsystem`의 초기화 순서는 보장되지 않는다.

```cpp
void UP1InventorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Collection.InitializeDependency<UP1NetworkSubsystem>();
    Super::Initialize(Collection);

    // 여기서부터 UP1NetworkSubsystem이 초기화되어 있다.
}
```

---

## 3. 게임 서버 C++

언리얼 밖에서 도는 C++이다. UHT가 없으므로 클라이언트의 이름 규칙을 그대로 옮겨 쓰지 않는다.

**적용 대상은 `Server/GameServer/`와 `Server/DummyClient/`다. `Server/ServerCore/`는 제외한다.**
— ServerCore는 완성된 네트워크 코어다. 게임 로직이 바뀌어도 이 계층은 바뀌지 않으므로, 이름을
고쳐서 얻는 것보다 이미 돌아가는 코드를 건드려서 잃는 것이 크다.

2026년 9월 20일 기준으로 ServerCore에 남아 있는 미준수 항목은 아래 아홉 개다. 고치지 않는다.

| 파일 | 멤버 |
| --- | --- |
| `DB/DBQueue.h` | `jobs` · `mtx` · `cv` · `stopFlag` |
| `Network/NetworkEvent.h` | `eventType` · `owner` · `session` · `sendBuffers` |
| `Network/SocketUtil.h` | `alreadyInit` |

### 3.1 타입 접두사를 붙이지 않는다

클래스와 구조체 이름은 접두사 없는 파스칼 케이스로 짓는다.

```cpp
class Room;
class RoomManager;
struct RoomEnterData;
```

— 접두사는 UHT가 타입을 판별하려고 요구하는 것이다. 여기에는 UHT가 없고, 네임스페이스를 쓸 수
있으므로 이름 충돌을 막을 다른 수단이 있다.

예외: 생성된 패킷 타입의 `C_`와 `S_`는 접두사가 아니라 방향 표시다(→ 「3.5」).

### 3.2 멤버 변수는 `_camelCase`로 쓴다

```cpp
private:
    int32 _roomId = 0;
    bool _isValid = false;
```

접근 지정자와 무관하게 적용한다. public 멤버에도 밑줄을 붙인다.
— 밑줄이 뜻하는 것은 접근 범위가 아니라 "이것은 멤버다"이다. 지역 변수와 매개변수를 한눈에
가려낼 수 있어야 한다.

예외: 필드만 담고 동작이 없는 순수 데이터 구조체는 붙이지 않는다.

```cpp
struct PacketHeader
{
    uint16 size;
    uint16 id;
};
```

**static 멤버 변수에는 `_` 대신 `s_`를 붙인다.**

```cpp
class ObjectUtils
{
private:
    static atomic<int64> s_idGenerator;
};
```

— static 멤버는 객체 없이도 살아 있다. 수명이 다르므로 이름도 갈라 둔다.

예외: 운영체제나 외부 라이브러리가 정한 이름을 그대로 담는 포인터는 그 이름을 지킨다.
`SocketUtil`의 `ConnectEx`·`DisconnectEx`·`AcceptEx`가 여기 해당한다.
— 이름을 바꾸면 Winsock 문서에서 같은 이름으로 찾을 수 없다.

### 3.3 함수는 파스칼 케이스로 쓴다

```cpp
bool AddItem(...);
void ClearDirtyFlags();
```

지역 변수와 매개변수는 카멜 케이스로 쓴다.

```cpp
void Room::EnterPlayer(PlayerRef player, int32 targetRoomId)
```

### 3.4 상수는 SCREAMING_SNAKE_CASE로 쓴다

```cpp
const int32 MAX_LEVEL = 50;
static constexpr int32 CELL_SIZE = 200;
```

### 3.5 패킷 핸들러 이름은 진입 경로를 드러낸다

- `C_Handle*` — 클라이언트 패킷으로 진입하는 핸들러
- `Handle*` — 서버 내부에서 부르는 핸들러

이 구분은 장식이 아니다. 신뢰 경계의 위치를 이름이 표시한다. `C_Handle*`에 들어온 값은 전부
검증 대상이다.

### 3.6 룸 소유 상태를 인라인으로 고치지 않는다

`Room`이 `JobQueue`를 상속한다. 패킷 핸들러는 인라인으로 일하지 않고 `room->DoAsync(...)`로 잡만
밀어넣고 리턴한다. 룸 소유 상태는 큐 위에서 직렬화되므로 락이 없다.

**다른 룸의 오브젝트에 직접 손대지 않는다.** DB 작업도 `DBQueue`에 push한다.

상세는 `docs/ARCHITECTURE.md`에 있다.

---

## 4. 인증 서버 (Node)

### 4.1 ESM을 쓴다

`package.json`에 `"type": "module"`이 있다. `import`와 `export`를 쓰고 `require`를 쓰지 않는다.

### 4.2 폴더 이름은 소문자로 쓴다

`src/` 아래의 폴더 이름을 소문자로 통일한다.
— 대소문자를 구분하지 않는 파일 시스템에서 개발하고 구분하는 곳에 배포하면 import 경로가 조용히
깨진다.

### 4.3 에러를 삼키지 않는다

`catch`에서 응답만 돌려주고 원인을 남기지 않는 코드를 쓰지 않는다. 최소한 `console.error`로
원문을 남긴다.
— 로그인은 가장 자주 도는 경로이고 의존성이 셋(bcrypt, MSSQL, Redis)이다. 원인을 남기지 않으면
셋 중 무엇이 죽었는지 알려면 서버에 붙어 재현해야 한다.

---

## 5. 아직 정하지 않은 것

아래 항목은 필요해지는 시점에 정한다. 미리 정하지 않는다.

- 게임플레이 어빌리티 시스템(GAS) 도입 여부. 서버가 판정을 맡으므로 지금은 도입하지 않는다
- 로그 레벨 사용 기준. 카테고리 이름 규칙만 「2.7」에 정해 두었다
- 데이터 에셋과 데이터 테이블 중 어느 쪽을 기본으로 삼을지
- Doxygen 도입 여부. 도입하면 「2.10」을 다시 정한다
