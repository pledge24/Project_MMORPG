#include "Core/P1GameInstance.h"

#include "Network/P1ConnectionSubsystem.h"
#include "Sync/P1StatefulEntityManager.h"
#include "Protocol.pb.h"
#include "Game/Entities/P1MyPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Game/Progress/P1MyPlayerData.h"
#include "Utils/LogCategory.h"

namespace
{
    /** 사유를 모르는 끊김에 보여 줄 문구다. 서버가 먼저 내려갔거나 사유 패킷을 잃은 경우다. */
    const TCHAR* const CONNECTION_LOST_NOTICE = TEXT("게임 서버와 연결이 끊겼습니다.");
}

UP1GameInstance::UP1GameInstance()
{
}

void UP1GameInstance::Init()
{
    Super::Init();

    UP1MyPlayerData* MyPlayerData = GetSubsystem<UP1MyPlayerData>();
    if (IsValid(MyPlayerData) == false)
    {
        UE_LOG(LogP1System, Warning, TEXT("내 플레이어 데이터 서브시스템을 찾지 못했다"));
    }
    else
    {
        MyPlayerData->OnMyPlayerSpawned.AddUObject(this, &UP1GameInstance::HandleMyPlayerSpawned);
    }

    _Connection = GetSubsystem<UP1ConnectionSubsystem>();
    if (IsValid(_Connection) == false)
    {
        UE_LOG(LogP1System, Warning, TEXT("연결 서브시스템을 찾지 못했다"));
    }
    else
    {
        _Connection->OnConnectionLost.AddUObject(this, &UP1GameInstance::HandleConnectionLost);
    }
}

void UP1GameInstance::BeginDestroy()
{
    Super::BeginDestroy();
}

//~ Network Method
#pragma region Network Method

bool UP1GameInstance::IsConnected() const
{
	return _Connection && _Connection->IsConnected();
}

void UP1GameInstance::HandleConnectionLost()
{
	ReturnToLogin(CONNECTION_LOST_NOTICE);
}

void UP1GameInstance::HandleLeaveGame(const Protocol::S_LEAVE_GAME& LeaveGamePkt)
{
	switch (LeaveGamePkt.reason())
	{
	case Protocol::LEAVE_REASON_DUPLICATE_LOGIN:
		ReturnToLogin(TEXT("다른 곳에서 같은 계정으로 로그인해 연결이 끊겼습니다."));
		break;
	case Protocol::LEAVE_REASON_INVALID_TOKEN:
		ReturnToLogin(TEXT("로그인 정보가 만료되었습니다. 다시 로그인하세요."));
		break;
	default:
		ReturnToLogin(CONNECTION_LOST_NOTICE);
		break;
	}
}

FString UP1GameInstance::ConsumeLoginNotice()
{
	return MoveTemp(PendingLoginNotice);
}

void UP1GameInstance::ReturnToLogin(const FString& Notice)
{
	UE_LOG(LogP1Network, Warning, TEXT("게임 서버 연결이 끊겨 로그인 화면으로 돌아간다: %s"), *Notice);

	// 토큰은 게임 서버가 한 번 쓰고 지웠다. 다시 들어가려면 인증 서버에 다시 로그인해야 한다.
	// 그래서 연결만 닫고 다시 잇지 않는다.
	if (_Connection)
		_Connection->Close();

	_MyPlayer = nullptr;

	PendingLoginNotice = Notice;
	UGameplayStatics::OpenLevel(GetWorld(), FName("L_LoginMap"));
}

#pragma endregion Network Method

//~ Handle Packet Method

void UP1GameInstance::HandleEnterGame(const Protocol::S_ENTER_GAME& EnterGamePkt)
{
    if (EnterGamePkt.success() == false)
        return;

    UP1MyPlayerData* MyPlayerData = GetSubsystem<UP1MyPlayerData>();

    // 게임 서버에 입장한 시점에 가져온 캐릭터의 모든 정보를 저장한다.
    MyPlayerData->InitMyPlayerData(EnterGamePkt);

    // 임시
    UGameplayStatics::OpenLevel(GetWorld(), FName("L_InGameMap"));
}

void UP1GameInstance::HandleEnterMap(const Protocol::S_ENTER_MAP& EnterMapPkt)
{
    if (EnterMapPkt.success() == false)
    {
        UE_LOG(LogP1Network, Warning, TEXT("맵 입장에 실패했습니다. map_id: %d"), EnterMapPkt.map_id());
        return;
    }

    // 입장한 map + room 정보 저장
    if(UP1MyPlayerData* MyPlayerData = GetSubsystem<UP1MyPlayerData>())
    {
        MyPlayerData->SetRoomId(EnterMapPkt.room_id());
        MyPlayerData->SetMapId(EnterMapPkt.map_id());
    }

    // 임시
    UGameplayStatics::OpenLevel(GetWorld(), FName("L_InGameMap"));
}

void UP1GameInstance::HandleEnterRoom(const Protocol::S_ENTER_ROOM& EnterRoomPkt)
{
    if (EnterRoomPkt.success() == false)
    {
        UE_LOG(LogP1Network, Warning, TEXT("Room 입장에 실패했습니다. room_id: %d"), EnterRoomPkt.room_id());
        return;
    }

    if (UP1MyPlayerData* MyPlayerData = GetSubsystem<UP1MyPlayerData>())
    {
        MyPlayerData->SetRoomId(EnterRoomPkt.room_id());

        // 같은 맵 안의 방 이동이라면 나를 제외한 모든 엔티티를 Despawn + 텔레포트.
        // 다른 룸으로 리스폰하는 경우도 룸이 한 맵 안의 논리 분할이라 같은 처리다.
        if (EnterRoomPkt.enter_type() == Protocol::ENTER_TYPE_SAME_MAP_TRANSFER
            || EnterRoomPkt.enter_type() == Protocol::ENTER_TYPE_RESPAWN)
        {
            if (UP1StatefulEntityManager* EntityManager = GetEntityManager())
                EntityManager->DespawnAllEntities();
            if (EnterRoomPkt.has_enter_pos() && IsValid(_MyPlayer))
            {
                _MyPlayer->SetClientPos(EnterRoomPkt.enter_pos());
                _MyPlayer->SetServerPos(EnterRoomPkt.enter_pos());
            }
        }
        
    }

}

UP1StatefulEntityManager* UP1GameInstance::GetEntityManager() const
{
    if (UWorld* World = GetWorld())
        return World->GetSubsystem<UP1StatefulEntityManager>();

    return nullptr;
}

void UP1GameInstance::HandleRewardResult(const Protocol::S_REWARD_RESULT& RewardResultPkt)
{
    if (IsConnected() == false)
        return;

    auto* World = GetWorld();
    if (World == nullptr)
        return;

    

}

void UP1GameInstance::HandleMyPlayerSpawned(AP1MyPlayer* MyPlayer)
{
    _MyPlayer = MyPlayer;
}
