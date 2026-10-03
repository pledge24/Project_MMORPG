#include "Core/P1InGamePlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Game/Entities/P1MyPlayer.h"
#include "Game/Progress/P1MyPlayerData.h"
#include "Network/P1PacketSender.h"
#include "UI/P1ScreenSubsystem.h"

void AP1InGamePlayerController::BeginPlay()
{
    Super::BeginPlay();

    Protocol::C_ENTER_ROOM EnterRoomPkt;
    {
        EnterRoomPkt.set_enter_type(Protocol::ENTER_TYPE_CROSS_MAP_TRANSFER);

        if (UP1MyPlayerData* MyPlayerData = GetGameInstance()->GetSubsystem<UP1MyPlayerData>())
        {
            int32 RoomId = MyPlayerData->GetRoomId();
            EnterRoomPkt.set_room_id(RoomId);
        }

        FP1PacketSender::Send(this, EnterRoomPkt);
    }

    if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        if (InGameUIMappingContext)
            InputSubsystem->AddMappingContext(InGameUIMappingContext, 0);
    }

    if (UP1ScreenSubsystem* Screens = GetScreens())
        Screens->CreateScreens(this);
}

void AP1InGamePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 로컬 플레이어는 맵보다 오래 산다. 이 맵의 위젯을 서브시스템이 쥐고 남지 않게 놓는다.
    if (UP1ScreenSubsystem* Screens = GetScreens())
        Screens->ReleaseScreens();

    Super::EndPlay(EndPlayReason);
}

void AP1InGamePlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
    {
        if (ToggleStatusWindowAction)
            EnhancedInputComponent->BindAction(ToggleStatusWindowAction, ETriggerEvent::Started, this, &AP1InGamePlayerController::OnToggleStatusWindowWidget);

        if (ToggleInventoryAction)
            EnhancedInputComponent->BindAction(ToggleInventoryAction, ETriggerEvent::Started, this, &AP1InGamePlayerController::OnToggleInventoryWidget);
    }
}

void AP1InGamePlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    // 전투 모드는 내 플레이어가 갖고, 화면 서브시스템이 알림을 받아 화면에 보여 준다.
    AP1MyPlayer* MyPlayer = Cast<AP1MyPlayer>(InPawn);
    UP1ScreenSubsystem* Screens = GetScreens();
    if (MyPlayer && Screens)
    {
        MyPlayer->OnBattleModeChanged.AddUObject(Screens, &UP1ScreenSubsystem::ShowBattleMode);
    }
}

void AP1InGamePlayerController::OnToggleStatusWindowWidget()
{
    if (UP1ScreenSubsystem* Screens = GetScreens())
        Screens->ToggleWindow(EP1WidgetType::WIDGET_STATUS_WINDOW);
}

void AP1InGamePlayerController::OnToggleInventoryWidget()
{
    if (UP1ScreenSubsystem* Screens = GetScreens())
        Screens->ToggleWindow(EP1WidgetType::WIDGET_INVENTORY);
}

UP1ScreenSubsystem* AP1InGamePlayerController::GetScreens() const
{
    return ULocalPlayer::GetSubsystem<UP1ScreenSubsystem>(GetLocalPlayer());
}

void AP1InGamePlayerController::TurnOnWidget(EP1WidgetType Type)
{
    if (UP1ScreenSubsystem* Screens = GetScreens())
        Screens->OpenWindow(Type);
}

void AP1InGamePlayerController::TurnOffWidget(EP1WidgetType Type)
{
    if (UP1ScreenSubsystem* Screens = GetScreens())
        Screens->CloseWindow(Type);
}

void AP1InGamePlayerController::DisplayWarningText(const FText& Message)
{
    if (UP1ScreenSubsystem* Screens = GetScreens())
        Screens->DisplayWarningText(Message);
}
