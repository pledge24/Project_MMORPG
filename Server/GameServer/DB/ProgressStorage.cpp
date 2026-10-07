#include "Core/pch.h"
#include "DB/ProgressStorage.h"
#include "DB/CharacterStateDAO.h"
#include "DB/ItemDAO.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/PlayerSaveData.h"

/*-------------------------
    ProgressStorage
--------------------------*/

void ProgressStorage::Load(SessionRef session, int64 characterId)
{
    PlayerRef player = static_pointer_cast<GameSession>(session)->_player;

    // 실패해도 클라에 알려야 로그인 화면에서 기다리지 않는다.
    auto sendEnterGameFail = [&session]()
        {
            Protocol::S_ENTER_GAME enterGameFailPkt;
            enterGameFailPkt.set_success(false);
            SEND_PACKET(enterGameFailPkt)
        };

    // 1. 캐릭터 기본 정보 다시 가져오기(이름, 레벨 등 필요)
    if (CharacterStateDAO::LoadCharacter(session, characterId) == false)
    {
        wcout << L"캐릭터 기본 정보 불러오기 실패" << endl;
        sendEnterGameFail();
        return;
    }

    // 2. 캐릭터 마지막 상태(LastState)가져오기
    if (CharacterStateDAO::LoadLastState(session, characterId) == false)
    {
        wcout << L"캐릭터 마지막 상태(LastState) 불러오기 실패" << endl;
        sendEnterGameFail();
        return;
    }

    // 3. 캐릭터 소유 아이템 정보 가져오기
    if (ItemDAO::LoadItems(session, characterId) == false)
    {
        wcout << L"캐릭터 소유 아이템 정보 불러오기 실패" << endl;
        sendEnterGameFail();
        return;
    }

    // DB에서 가져온 스펙을 기반으로 최종 스텟 계산
    if (player->Start() == false)
    {
        sendEnterGameFail();
        return;
    }
    
    // 패킷으로 만들어서 클라이언트에게 보낸다.
    Protocol::S_ENTER_GAME enterGamePkt;
    {
        enterGamePkt.set_success(true);
        enterGamePkt.mutable_player()->CopyFrom(*player->_entityInfo);

        enterGamePkt.mutable_stat_info()->CopyFrom(*player->_statInfo);
        enterGamePkt.mutable_possession()->CopyFrom(*player->_possession);
    }

    SEND_PACKET(enterGamePkt)
}

void ProgressStorage::Save(const PlayerSaveData& data)
{
    // 1. 캐릭터 기본 정보 업데이트(이름, 레벨 등 필요)
    if (CharacterStateDAO::SaveCharacter(data) == false)
    {
        wcout << L"캐릭터 기본 정보 업데이트 실패" << endl;
        return;
    }

    // 2. 캐릭터 마지막 상태(LastState) 업데이트
    if (CharacterStateDAO::SaveLastState(data) == false)
    {
        wcout << L"캐릭터 마지막 상태(LastState) 업데이트 실패" << endl;
        return;
    }

    // 3. 캐릭터 소유 아이템 정보 업데이트
    if (ItemDAO::SaveItems(data) == false)
    {
        wcout << L"캐릭터 소유 아이템 정보 업데이트 실패" << endl;
        return;
    }
}
