#include "Core/pch.h"
#include "Network/GameEntry.h"
#include "DB/ProgressStorage.h"
#include "Game/Entities/Player.h"
#include "Game/Entities/EntityFactory.h"
#include "Game/Entities/PlayerProgress.h"
#include "Game/QuickSlots/QuickSlotComponent.h"
#include "Game/Room/RoomTransfer.h"
#include "Network/ItemPackets.h"
#include "Network/ProgressCoordinator.h"
#include "DB/DAOCommon.h"

namespace
{
    // DB는 맵과 좌표만 저장한다. 들어갈 룸은 그 맵과 좌표로 고르고, 범위 밖이면 마을의 룸과 리스폰 지점을 돌려준다.
    // 들어갈 룸이 없으면 nullopt.
    optional<RoomAssignment> AssignEnterRoom(const PlayerProgress& progress)
    {
        Protocol::PosInfo pos;
        pos.mutable_pos()->set_x(progress.pos.x);
        pos.mutable_pos()->set_y(progress.pos.y);
        pos.mutable_pos()->set_z(progress.pos.z);
        pos.set_yaw(progress.yaw);

        const optional<RoomAssignment> assignment = RoomTransfer::AssignRoom(progress.mapId, pos);
        if (assignment.has_value() == false)
        {
            GLogger->Error("캐릭터 {}가 맵 {}에서 들어갈 룸이 없어 입장을 거절합니다(지역 표에 마을 리스폰 지점이 없다)",
                progress.characterId, progress.mapId);
            return nullopt;
        }

        return assignment;
    }
}

void GameEntry::RequestEnter(const GameSessionRef& session, int64 userId, int64 characterId)
{
    // 플레이어는 DB 잡 안에서 만든다. 들어갈 룸은 진행을 DB에서 불러와야 알 수 있으므로 이 시점에는 넘길 룸 큐가 없다.
    // 언제 어느 DB 큐에서 돌지는 조율자가 정한다.
    auto load = [session, userId, characterId]()
        {
            // 저장을 기다리는 사이에 끊겼으면 불러올 이유가 없다. 불러오면 끊긴 것을 알아챈 뒤 또 저장한다.
            if (session->IsConnected() == false)
                return;

            // 불러오기가 모두 성공하고 검증을 통과해야 세션에 플레이어가 생긴다. 룸에는 그 뒤 EnterPlayer로 들어간다.
            Protocol::S_ENTER_GAME enterGamePkt;
            try
            {
                DBConnectionGuard conn;
                enterGamePkt = Enter(*conn, session, userId, characterId);
            }
            catch (const exception& error)
            {
                // 실패해도 응답을 보내야 클라이언트가 캐릭터 선택 화면에서 기다리지 않는다.
                GLogger->Error("캐릭터 {} 입장 실패: {}", characterId, error.what());
                enterGamePkt.Clear();
                enterGamePkt.set_result(Protocol::ENTRY_RESULT_INTERNAL_ERROR);
            }
            FinishEnter(session, enterGamePkt);
        };

    auto reject = [session]()
        {
            Protocol::S_ENTER_GAME enterGameFailPkt;
            enterGameFailPkt.set_result(Protocol::ENTRY_RESULT_PREVIOUS_SAVE_PENDING);
            FinishEnter(session, enterGameFailPkt);
        };

    GProgressCoordinator->RequestEnter(userId, { std::move(load), std::move(reject) });
}

void GameEntry::FinishEnter(const GameSessionRef& session, const Protocol::S_ENTER_GAME& enterGamePkt)
{
    // 응답을 보낸 뒤에 바꾸면, 응답을 받은 클라이언트의 다음 요청(맵 로드 완료, 다시 고른 캐릭터의 입장)이 앞 단계에서 버려진다.
    if (enterGamePkt.result() == Protocol::ENTRY_RESULT_SUCCESS)
        session->CompleteGameEntry();
    else
        session->CancelGameEntry();

    SendPacket(session, enterGamePkt);
}

Protocol::S_ENTER_GAME GameEntry::Enter(DBConnection& conn, const GameSessionRef& session, int64 userId, int64 characterId)
{
    Protocol::S_ENTER_GAME enterGamePkt;

    // 진행을 불러오고 들어갈 룸을 고른다.
    PlayerProgress progress;
    if (ProgressStorage::Load(conn, userId, characterId, OUT progress) == false)
    {
        enterGamePkt.set_result(Protocol::ENTRY_RESULT_INTERNAL_ERROR);
        return enterGamePkt;
    }

    const optional<RoomAssignment> assignment = AssignEnterRoom(progress);
    RoomRef enterRoom = assignment.has_value() ? GRoomManager->FindRoom(assignment->roomId) : nullptr;
    if (enterRoom == nullptr)
    {
        enterGamePkt.set_result(Protocol::ENTRY_RESULT_INTERNAL_ERROR);
        return enterGamePkt;
    }

    // 검증을 통과한 플레이어만 세션에 등록한다.
    PlayerRef player = SpawnPlayer(session, userId, progress, enterRoom, assignment->pos);
    if (player == nullptr)
    {
        enterGamePkt.set_result(Protocol::ENTRY_RESULT_INTERNAL_ERROR);
        return enterGamePkt;
    }

    enterGamePkt.set_result(Protocol::ENTRY_RESULT_SUCCESS);
    enterGamePkt.set_map_id(player->GetMapId());
    // 맵을 여는 동안 만들어지는 화면이 이름과 레벨을 읽는다. 내 엔티티는 룸 입장 응답이 싣는다.
    const Protocol::PlayerInfo& playerInfo = player->GetPlayerInfo();
    enterGamePkt.set_name(playerInfo.name());
    enterGamePkt.set_character_class(player->GetCharacterClass());
    enterGamePkt.set_level(playerInfo.level());
    enterGamePkt.mutable_stats()->CopyFrom(player->GetStatInfo());
    *enterGamePkt.mutable_possession() = ItemPackets::MakePossession(*player);
    // 클라이언트는 받은 퀵슬롯 수로 퀵슬롯을 만들므로 빈 퀵슬롯도 싣는다.
    ItemPackets::AddQuickSlots(player->GetQuickSlots().GetAll(), enterGamePkt.mutable_quick_slots());

    return enterGamePkt;
}

PlayerRef GameEntry::SpawnPlayer(const GameSessionRef& session, int64 userId, const PlayerProgress& progress, const RoomRef& enterRoom, const Protocol::PosInfo& enterPos)
{
    PlayerSpawnParams spawnParams;
    spawnParams.session = session;
    spawnParams.userId = userId;
    spawnParams.progress = &progress;

    // 팩토리는 진행을 채우고 검증까지 한다. 검증에 실패하면 nullptr이고, 그때 세션은 아직 바뀌지 않았다.
    PlayerRef player = EntityFactory::Create<Player>(spawnParams);
    if (player == nullptr)
    {
        GLogger->Warning("캐릭터 {}의 진행이 검증을 통과하지 못해 입장을 거절합니다", progress.characterId);
        return nullptr;
    }

    // 세션에 등록하기 전에 정한다. 등록한 뒤에 정하면 그사이 온 맵 로드 완료가 소속 룸 없이 버려진다.
    player->BeginMapLoad(enterRoom, enterPos);

    // 비어 있을 때만 등록한다. 이미 있는 플레이어를 덮어쓰면 그 플레이어는 룸에 남고, 끊길 때 퇴장하지 않는다.
    if (session->TryRegisterPlayer(player) == false)
    {
        GLogger->Warning("계정 {}의 세션에 이미 플레이어가 있어 입장을 거절합니다", userId);
        return nullptr;
    }

    return player;
}
