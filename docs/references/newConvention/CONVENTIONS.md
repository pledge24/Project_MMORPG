# 코딩 컨벤션

UE 5.8 런처 빌드 기반 MMORPG 클라이언트의 C++ 컨벤션이다. 게임 서버는 언리얼 외부의 C++ 서버를 쓴다.

이 문서의 규칙은 두 종류다.

- **엔진 제약**: 지키지 않으면 동작이 달라진다. 선택할 수 없다.
- **프로젝트 규칙**: 이 프로젝트에서 정한 것이다. 근거와 함께 적는다.

각 절에서 엔진 제약에 해당하는 항목은 `[엔진 제약]`으로 표시한다.

이 문서에서 `Arc`는 프로젝트 약어의 자리표시자다. 실제 약어를 정한 뒤 문서 전체를 한 번에 바꾼다.

---

## 1. 주석

### 1.1 헤더와 구현의 역할을 나눈다

**`.h`에는 호출하는 쪽이 알아야 할 것을 적는다.**
— 함수 이름으로 알 수 있는 내용을 반복하면 유지 비용만 늘어난다.

헤더에 적을 것은 아래 다섯 가지다.

- 단위와 값의 범위 (초인지 밀리초인지, 0에서 1 사이인지)
- 실행 권한 (서버 전용인지, 로컬 클라이언트 전용인지)
- 호출 시점의 제약 (`BeginPlay` 이후에만 유효한지)
- 반환값이 `nullptr`일 수 있는지
- 이름에 드러나지 않는 부작용

```cpp
/** 서버 전용이다. 데미지를 적용하고 사망 판정까지 한다. */
void ApplyDamage(float Amount, AActor* Instigator);

/** 초 단위 쿨다운이다. 0이면 쿨다운이 없다. */
UPROPERTY(EditDefaultsOnly, Category = "Combat")
float CooldownSeconds = 1.0f;

/** 맨손 상태에서는 nullptr을 반환한다. */
AArcWeapon* GetEquippedWeapon() const;
```

아래 형태는 쓰지 않는다.

```cpp
/** 데미지를 적용한다. */
void ApplyDamage(float Amount, AActor* Instigator);
```

**`.cpp`에는 고치는 쪽이 알아야 할 것을 적는다.**
— 헤더의 설명을 반복하지 않는다.

- 자명하지 않은 순서 의존성
- 엔진 동작을 우회하려고 넣은 코드
- 실수처럼 보이지만 의도한 코드
- 서버와 클라이언트의 동기화 타이밍 전제

```cpp
void AArcCharacter::BeginPlay()
{
    Super::BeginPlay();

    // 어빌리티 부여가 리플리케이션보다 먼저 끝나야 한다.
    // PossessedBy에서 부여하면 순서가 뒤집힌다.
    GrantStartupAbilities();
}
```

### 1.2 한 줄을 기본으로 하되 계약을 한 줄에 압축하지 않는다

호출하는 쪽이 알아야 할 계약이 둘 이상이면 줄을 나눈다.

```cpp
/**
 * 서버에서 받은 상태를 적용한다.
 * 이미 적용한 Sequence보다 오래된 데이터는 버린다.
 */
void ApplyState(const FArcPlayerStateData& Data);
```

### 1.3 `/** */`를 UPROPERTY와 UFUNCTION 문서화에 쓴다

`[엔진 제약]` UHT는 리플렉션 대상 멤버 바로 위의 주석을 ToolTip 메타데이터로 수집한다. 수집된 주석은 에디터 디테일 패널과 블루프린트에서 툴팁으로 보인다.

이 동작 때문에 주석 형식을 `/** */`로 통일한다. 주석을 쓰는 동시에 에디터 문서를 얻는다.

### 1.4 `@param`과 `@return`을 쓰지 않는다

Doxygen 문서를 생성하지 않는 동안에는 쓰지 않는다. 인자 설명이 필요하면 서술 문장에 넣는다.

Doxygen을 도입하면 이 규칙을 다시 정한다.

### 1.5 TODO 형식

```cpp
// TODO(Name): 내용
```

---

## 2. 섹션 구분

### 2.1 모든 섹션 표시에 `//~`를 쓴다

`[엔진 제약]` UHT가 리플렉션 대상 멤버 바로 위의 주석을 툴팁으로 수집하기 때문에, `// Combat` 같은 표시를 UPROPERTY 위에 두면 그 UPROPERTY의 툴팁이 "Combat"이 된다. `//~`로 시작하는 줄은 수집에서 빠진다.

```cpp
//~ Begin AActor Interface
virtual void BeginPlay() override;
virtual void Tick(float DeltaSeconds) override;
//~ End AActor Interface

//~ Combat
UPROPERTY(EditDefaultsOnly, Category = "Combat")
float AttackPower = 10.0f;
```

`//~ Begin/End` 형식 자체는 엔진 소스의 관례이고 에픽 공식 코딩 표준 문서에는 없다. 형식은 프로젝트 규칙이고, 툴팁에서 빠지는 동작은 엔진 제약이다.

### 2.2 두 가지 형태만 쓴다

- `//~ Begin <TypeName> Interface` / `//~ End <TypeName> Interface`: 엔진 인터페이스 오버라이드
- `//~ <FeatureName>`: 기능 섹션

---

## 3. 클래스 멤버 정렬

### 3.1 기능 단위로 묶고 각 기능 안에서 접근 지정자를 반복한다

에픽 공식 표준은 클래스 전체를 public에서 private 순서로 배치하라고 안내한다. 이 프로젝트는 기능 섹션 **안에** 그 순서를 적용한다.

— 게임플레이 클래스 하나가 이동, 전투, 인벤토리, 퀘스트를 모두 다룬다. 접근 지정자로만 정렬하면 전투 함수와 전투 변수가 파일의 양 끝으로 갈라져서, 기능 하나를 고칠 때마다 파일 전체를 오간다.

순서는 아래와 같다.

1. 생성자
2. 엔진 인터페이스 오버라이드
3. 기능 섹션 (각 섹션 안에서 public → protected → private, 함수 먼저 변수 나중)

```cpp
UCLASS()
class ARC_API AArcCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AArcCharacter();

    //~ Begin AActor Interface
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutProps) const override;
    //~ End AActor Interface

    //~ Combat
public:
    /** 서버 전용이다. */
    void ApplyDamage(float Amount, AActor* Instigator);

protected:
    void HandleDeath();

private:
    void PlayDeathMontage();

    UPROPERTY(ReplicatedUsing = OnRep_Health)
    float Health = 100.0f;

    //~ Inventory
public:
    UArcInventoryComponent* GetInventory() const { return Inventory; }

private:
    UPROPERTY(VisibleAnywhere, Category = "Inventory")
    TObjectPtr<UArcInventoryComponent> Inventory;
};
```

### 3.2 public에는 외부에서 호출하는 것만 둔다

— 기능마다 public 블록이 새로 열리기 때문에 구조상 public이 늘어나기 쉽다.

한 기능 섹션의 public 함수가 5개를 넘으면 그 기능을 컴포넌트로 뺄 시점인지 판단한다 (→ 「7. 분리 기준」).

---

## 4. 이름 규칙

### 4.1 타입 접두사

`[엔진 제약]` UHT가 접두사로 타입을 판별한다.

| 대상 | 접두사 | 예 |
| --- | --- | --- |
| UObject 파생 | `U` | `UArcInventoryWidget` |
| AActor 파생 | `A` | `AArcCharacter` |
| 구조체 | `F` | `FArcItemSlot` |
| 열거형 | `E` | `EArcItemRarity` |
| 인터페이스 | `I` | `IArcInteractable` |
| bool 변수 | `b` | `bIsDead` |

### 4.2 타입 접두사 뒤에 프로젝트 약어를 붙인다

`[엔진 제약]` UCLASS는 네임스페이스 안에 둘 수 없다. 리플렉션 이름은 전역에서 유일해야 한다.

프로젝트 약어는 2글자에서 4글자로 고정한다. 프로젝트 이름 전체를 넣지 않는다.

```
O  UArcInventoryWidget
X  UInventoryWidget
X  UMyAwesomeMMORPGInventoryWidget
```

**첫 번째 UCLASS부터 약어를 쓴다.**
— 나중에 클래스 이름을 바꾸면 그 클래스를 부모로 삼은 블루프린트가 부모를 잃는다. 복구하려면 `DefaultEngine.ini`에 Core Redirects 항목을 직접 쓰고, 헤더 파일명, cpp 파일명, `#include` 구문, 클래스 선언을 모두 바꿔야 한다.

```ini
[CoreRedirects]
+ClassRedirects=(OldName="/Script/Arc.InventoryWidget", NewName="/Script/Arc.ArcInventoryWidget")
```

### 4.3 `UUW` 형태의 이중 접두사를 쓰지 않는다

`[엔진 제약]` 리플렉션 이름은 첫 글자 하나만 떼어낸 것이다. `AMyActor`의 리플렉션 이름은 `MyActor`다.

`UUWInventory`로 선언하면 리플렉션 이름이 `UWInventory`가 된다. 엔진이 `UUW`를 특별하게 해석해서가 아니라 첫 글자 `U`만 떼기 때문이다. 결과적으로 블루프린트 클래스 선택창과 에셋 검색에 `UWInventory`가 노출된다.

역할은 접두사가 아니라 접미사로 나타낸다.

```
O  UArcInventoryWidget
X  UUWArcInventory
```

### 4.4 파일 이름

파일 이름에 타입 접두사를 붙이지 않는다. 프로젝트 약어는 남긴다.

```
클래스  UArcInventoryWidget
파일    ArcInventoryWidget.h / ArcInventoryWidget.cpp
```

### 4.5 헤더 포함 순서

`[엔진 제약]` `.generated.h`는 항상 마지막에 둔다.

```cpp
#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "ArcInventoryWidget.generated.h"
```

### 4.6 블루프린트 에셋

```
WBP_Inventory
WBP_ItemSlot
BP_ArcCharacter
DA_ItemTable
```

---

## 5. 위젯

### 5.1 CommonUI를 기반으로 한다

— MMORPG는 인벤토리 위에 아이템 상세 창이 뜨고 그 위에 확인 다이얼로그가 뜬다. 입력 포커스, ESC 처리, 게임패드 포커스를 직접 만들면 결국 CommonUI와 같은 구조에 도달한다. 그때는 위젯이 이미 수십 개 쌓여 있다.

### 5.2 두 계층을 합치지 않는다

`[엔진 제약]` `UCommonActivatableWidget`은 `UCommonUserWidget`을 상속한다. 하나의 프로젝트 베이스 클래스가 두 계층의 부모가 될 수 없다.

```
UCommonUserWidget
    └ UArcUserWidget
        ├ UArcItemSlotWidget
        ├ UArcPartyMemberWidget
        └ UArcQuestEntryWidget

UCommonActivatableWidget
    └ UArcActivatableWidget
        ├ UArcInventoryWidget
        ├ UArcCharacterWidget
        └ UArcQuestWidget
```

**양쪽에서 같이 쓰는 헬퍼는 상속이 아니라 `UBlueprintFunctionLibrary`로 뺀다.**
— 두 계층이 갈라져 있어서 공통 부모에 헬퍼를 넣을 자리가 없다. 계층을 합치려는 시도는 하지 않는다.

### 5.3 모든 위젯을 ActivatableWidget으로 만들지 않는다

`UArcActivatableWidget`은 화면, 페이지, 모달에만 쓴다. 아이템 슬롯과 리스트 엔트리는 `UArcUserWidget`을 상속한다.

### 5.4 공용 베이스를 얇게 유지한다

`UArcUserWidget`에는 접근자만 둔다. 로컬 플레이어 컨트롤러를 얻는 함수, 프로젝트 공용 열거형을 다루는 헬퍼가 여기에 해당한다.

**`UArcUserWidget`에서 Tick을 켜지 않는다.**
— 모든 위젯의 부모에 Tick이 들어가면 위젯이 100개로 늘어났을 때 원인을 찾기 어려운 프레임 저하로 돌아온다. 값 갱신은 델리게이트 구독으로 한다.

### 5.5 시각 구성은 WBP에 둔다

C++ 클래스에는 로직과 `BindWidget` 선언만 둔다.

```cpp
UPROPERTY(meta = (BindWidget))
TObjectPtr<UTextBlock> ItemNameText;
```

`[엔진 제약]` `BindWidget`은 WBP에 같은 이름의 위젯이 없으면 컴파일 단계에서 오류를 낸다.

---

## 6. 아키텍처

### 6.1 위젯이 서버 패킷 구조체를 직접 쓰지 않는다

```
외부 C++ 서버
    ↓  네트워크 계층: FArcInventoryResponse
UArcInventorySubsystem
    ↓  게임플레이 상태: FArcInventoryState
UArcInventoryWidget
```

— 패킷 구조체가 위젯까지 올라오면 서버 프로토콜이 바뀔 때마다 위젯을 같이 고친다.

위젯의 책임을 아래 형태로 제한한다.

```cpp
void UArcInventoryWidget::Refresh(const FArcInventoryState& State);
```

### 6.2 상태는 수명이 같은 단위로 나눈다

`UGameInstanceSubsystem` 하나에 모든 상태를 넣지 않는다.

```
UGameInstance
    ├ UArcNetworkSubsystem
    ├ UArcInventorySubsystem
    ├ UArcQuestSubsystem
    └ UArcPartySubsystem
```

지금 필요한 서브시스템만 만든다. 미리 다 만들지 않는다.

### 6.3 서브시스템 의존성을 `Initialize`에서 선언한다

`[엔진 제약]` `UGameInstanceSubsystem`의 초기화 순서는 보장되지 않는다.

```cpp
void UArcInventorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Collection.InitializeDependency<UArcNetworkSubsystem>();
    Super::Initialize(Collection);

    // 여기서부터 UArcNetworkSubsystem이 초기화되어 있다.
}
```

### 6.4 UObject 포인터를 `TObjectPtr`로 선언한다

```cpp
UPROPERTY(VisibleAnywhere)
TObjectPtr<UArcInventoryComponent> Inventory;
```

---

## 7. 분리 기준

기능 섹션으로 정리한 것만으로는 클래스가 계속 커지는 것을 막지 못한다. 아래 두 조건 중 하나를 만족하면 그 기능을 컴포넌트나 서브시스템으로 뺀다.

**조건 1: 그 기능만 쓰는 상태를 가진다.**
— 그 기능 섹션의 UPROPERTY가 3개를 넘고, 다른 기능 섹션의 코드가 그 값을 읽지 않으면 분리한다.

예외: 두 기능이 같은 값을 공유하면 줄 수와 무관하게 분리하지 않는다. `bIsStunned`를 전투와 이동이 같이 읽는 경우가 이에 해당한다.

**조건 2: 다른 액터 클래스에도 같은 기능이 필요하다.**
— NPC와 플레이어가 같은 전투 로직을 쓰면 컴포넌트로 뺀다. 플레이어에게만 필요하면 캐릭터 안에 둔다.

외부 C++ 서버를 쓰기 때문에 언리얼 리플리케이션을 거의 쓰지 않는다. 컴포넌트는 사실상 클라이언트 상태 컨테이너이므로 분리 비용이 낮다.

---

## 8. 폴더 구조

`Source/Arc/` 아래를 기능별로 나누지 않고 시작한다.

```
Source/Arc/
    UI/
    Character/
    Network/
    System/
```

한 폴더의 파일이 15개를 넘으면 그 폴더만 나눈다. 미리 깊게 파지 않는다.

---

## 9. 아직 정하지 않은 것

- 게임플레이 어빌리티 시스템(GAS) 도입 여부와, 도입할 경우의 어트리뷰트 이름 규칙
- 로그 카테고리 이름 규칙과 로그 레벨 사용 기준
- 서버 패킷 구조체의 이름 규칙 (`FArcInventoryResponse` 형태를 임시로 쓴다)
- 데이터 에셋과 데이터 테이블 중 어느 쪽을 기본으로 할지

클래스를 10개쯤 만든 뒤 이 4개 항목에 답한다.
