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
    // DB는 맵과 좌표만 저장한다. 들어갈 룸은 그 맵과 좌표로 고르고, 마을로 보낼 때는 맵과 위치도 마을의 것으로 바꾼다.
    // 들어갈 룸이 없으면 nullopt.
    optional<int32> AssignEnterRoom(PlayerProgress& progress)
    {
        Protocol::PosInfo pos;
        pos.mutable_pos()->set_x(progress.pos.x);
        pos.mutable_pos()->set_y(progress.pos.y);
        pos.mutable_pos()->set_z(progress.pos.z);
        pos.set_yaw(progress.yaw);

        const optional<RoomAssignment> assignment = RoomTransfer::AssignRoom(progress.mapId, pos);
        if (assignment.has_value() == false)
        {
            GLogger->Error("캐릭터 {}가 맵 {}에서 들어갈 룸이 없어 입장을 거절합니다(맵 표에 마을 리스폰 지점이 없다)",
                progress.characterId, progress.mapId);
            return nullopt;
        }

        progress.mapId = assignment->mapId;
        progress.pos = vector3D(assignment->pos.pos().x(), assignment->pos.pos().y(), assignment->pos.pos().z());
        progress.yaw = assignment->pos.yaw();
        return assignment->roomId;
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
                enterGamePkt.set_success(false);
            }
            FinishEnter(session, enterGamePkt);
        };

    auto reject = [session]()
        {
            Protocol::S_ENTER_GAME enterGameFailPkt;
            enterGameFailPkt.set_success(false);
            FinishEnter(session, enterGameFailPkt);
        };

    GProgressCoordinator->RequestEnter(userId, { std::move(load), std::move(reject) });
}

void GameEntry::FinishEnter(const GameSessionRef& session, const Protocol::S_ENTER_GAME& enterGamePkt)
{
    // 응답을 보낸 뒤에 바꾸면, 응답을 받은 클라이언트의 다음 요청(룸 입장, 다시 고른 캐릭터의 입장)이 앞 단계에서 버려진다.
    if (enterGamePkt.success())
        session->CompleteGameEntry();
    else
        session->CancelGameEntry();

    SendPacket(session, enterGamePkt);
}

Protocol::S_ENTER_GAME GameEntry::Enter(DBConnection& conn, const GameSessionRef& session, int64 userId, int64 characterId)
{
    Protocol::S_ENTER_GAME enterGamePkt;
    enterGamePkt.set_success(false);

    // 진행을 불러오고 들어갈 룸을 고른다.
    PlayerProgress progress;
    if (ProgressStorage::Load(conn, userId, characterId, OUT progress) == false)
        return enterGamePkt;

    const optional<int32> enterRoomId = AssignEnterRoom(progress);
    if (enterRoomId.has_value() == false)
        return enterGamePkt;

    // 검증을 통과한 플레이어만 세션에 등록한다.
    PlayerRef player = SpawnPlayer(session, userId, progress, enterRoomId.value());
    if (player == nullptr)
        return enterGamePkt;

    enterGamePkt.set_success(true);
    enterGamePkt.mutable_player()->CopyFrom(player->GetEntityInfo());
    // 맵과 룸 번호는 플레이어의 서버 필드라 엔티티 정보에 없다. 클라이언트는 입장 응답의 두 값으로 첫 룸 입장을 요청한다.
    enterGamePkt.mutable_player()->mutable_player_info()->set_map_id(player->GetMapId());
    enterGamePkt.mutable_player()->mutable_player_info()->set_room_id(enterRoomId.value());
    enterGamePkt.mutable_stat_info()->CopyFrom(player->GetStatInfo());
    *enterGamePkt.mutable_possession() = ItemPackets::MakePossession(*player);
    // 클라이언트는 받은 퀵슬롯 수로 퀵슬롯을 만들므로 빈 퀵슬롯도 싣는다.
    ItemPackets::AddQuickSlots(player->GetQuickSlots().GetAll(), enterGamePkt.mutable_quick_slots());

    return enterGamePkt;
}

PlayerRef GameEntry::SpawnPlayer(const GameSessionRef& session, int64 userId, const PlayerProgress& progress, int32 enterRoomId)
{
    PlayerSpawnParams spawnParams;
    spawnParams.session = session;
    spawnParams.userId = userId;
    spawnParams.progress = &progress;
    spawnParams.enterRoomId = enterRoomId;

    // 팩토리는 진행을 채우고 검증까지 한다. 검증에 실패하면 nullptr이고, 그때 세션은 아직 바뀌지 않았다.
    PlayerRef player = EntityFactory::Create<Player>(spawnParams);
    if (player == nullptr)
    {
        GLogger->Warning("캐릭터 {}의 진행이 검증을 통과하지 못해 입장을 거절합니다", progress.characterId);
        return nullptr;
    }

    // 비어 있을 때만 등록한다. 이미 있는 플레이어를 덮어쓰면 그 플레이어는 룸에 남고, 끊길 때 퇴장하지 않는다.
    if (session->TryRegisterPlayer(player) == false)
    {
        GLogger->Warning("계정 {}의 세션에 이미 플레이어가 있어 입장을 거절합니다", userId);
        return nullptr;
    }

    return player;
}
