#include "Core/pch.h"
#include "Network/ProgressCoordinator.h"
#include "DB/ProgressStorage.h"
#include "DB/DAOCommon.h"
#include "Game/Entities/Player.h"
#include "Game/Room/Room.h"

ProgressCoordinator::ProgressCoordinator(SaveGate& saveGate, PushDBJob pushDBJob, SaveProgress saveProgress, ScheduleTimer scheduleTimer)
    : _saveGate(saveGate), _pushDBJob(std::move(pushDBJob)), _saveProgress(std::move(saveProgress)), _scheduleTimer(std::move(scheduleTimer))
{
}

unique_ptr<ProgressCoordinator> ProgressCoordinator::CreateForServer(SaveGate& saveGate)
{
    // 람다는 부를 때에야 전역 객체를 읽는다. 만들 때는 DB 큐가 아직 없어도 된다.
    return make_unique<ProgressCoordinator>(
        saveGate,
        [](int64 userId, CallbackType job)
        {
            GDBManager->GetDBQueueFromId(userId)->Push(make_shared<Job>(std::move(job)));
        },
        [](const PlayerSaveData& data)
        {
            DBConnectionGuard conn;
            ProgressStorage::Save(*conn, data);
        },
        [](uint64 delayMs, CallbackType job)
        {
            GSessionJobQueue->DoTimer(delayMs, std::move(job));
        });
}

void ProgressCoordinator::OnDisconnected(int64 userId, const PlayerRef& player)
{
    if (player == nullptr)
        return;

    LeaveGameAndSave(userId, player);
}

void ProgressCoordinator::OnReturnToLobby(int64 userId, const PlayerRef& player, CallbackType afterSave)
{
    LeaveGameAndSave(userId, player);

    // 목록은 저장이 끝난 뒤에 읽는다. 입장 불러오기와 같이 저장 대기에 맡기면 저장 잡이 대기를 풀며 그 자리에서 돌린다.
    // 저장할 것이 없어 대기가 이미 풀렸으면 맡지 않으므로 같은 계정의 큐에 바로 넣는다.
    // 만료 타이머는 걸지 않는다. 기다리는 저장이 이 세션의 것이고, 저장 잡은 실패해도 대기를 푼다.
    SaveGate::ParkTicket ticket = _saveGate.Park(userId, { afterSave, []() {} });
    switch (ticket.result)
    {
    case SaveGate::ParkResult::NOT_HELD:
        _pushDBJob(userId, std::move(afterSave));
        break;

    case SaveGate::ParkResult::PARKED:
        break;

    case SaveGate::ParkResult::BUSY:
        // 같은 계정의 새 로비 접속이 이 세션을 밀어내고 먼저 입장을 맡겼다. 이 세션은 곧 끊기므로 로비로 받지 않는다.
        GLogger->Warning("계정 {}: 다른 연결의 입장이 저장을 기다리고 있어 로비 복귀를 마치지 않습니다", userId);
        break;
    }
}

void ProgressCoordinator::OnDuplicateConnection(int64 userId, const PlayerRef& replacedPlayer)
{
    // 새 세션의 입장이 기존 세션의 접속 종료보다 빠를 수 있으므로 대기를 여기서 먼저 건다.
    // 입장 전의 세션은 접속 종료가 대기를 풀지 않으므로 걸지 않는다.
    if (replacedPlayer == nullptr)
        return;

    _saveGate.Hold(userId);

    // 밀려난 세션의 접속 종료가 이미 대기를 풀었으면 이 대기를 풀 곳이 없으므로 여기서 푼다.
    // 대기를 건 뒤에 표시를 읽으므로, 끝나지 않은 접속 종료로 보이면 그 해제는 이 대기보다 나중에 온다.
    const Player::Presence presence = replacedPlayer->GetPresence();
    if (presence == Player::Presence::LEFT_BEFORE_ROOM || presence == Player::Presence::SAVED)
        ReleaseHold(userId);
}

void ProgressCoordinator::RequestEnter(int64 userId, SaveGate::ParkedLoad load)
{
    // 이 계정의 접속 종료 저장이 남아 있으면 저장 잡이 같은 userId 큐에서 이 불러오기를 실행한다.
    SaveGate::ParkTicket ticket = _saveGate.Park(userId, load);
    switch (ticket.result)
    {
    case SaveGate::ParkResult::NOT_HELD:
        _pushDBJob(userId, std::move(load.run));
        break;

    case SaveGate::ParkResult::PARKED:
        _scheduleTimer(PARKED_LOAD_TIMEOUT_MS, [this, userId, token = ticket.token]()
            {
                if (optional<SaveGate::ParkedLoad> expired = _saveGate.Expire(userId, token))
                {
                    GLogger->Warning("접속 종료 저장을 기다리다 입장을 거절합니다. userId: {}", userId);
                    expired->reject();
                }
            });
        break;

    case SaveGate::ParkResult::BUSY:
        // 저장을 기다리는 입장 요청이 이미 있다. 쌓으면 요청마다 플레이어가 만들어진다.
        load.reject();
        break;
    }
}

void ProgressCoordinator::LeaveRoomAndSave(const RoomRef& room, const PlayerRef& player)
{
    optional<PlayerSaveData> saveData = room->HandleDisconnect(player);
    if (saveData.has_value() == false)
    {
        // 접속 종료가 소속 룸을 읽은 뒤에 플레이어가 그 룸을 떠났다. 다른 맵으로 가 맵 로드를 기다리면 소속 룸이 목적지로 바뀌어
        // 있으므로 그 룸 큐가 저장한다. 같은 맵 안에서 옮기는 중이면 소속 룸이 아직 이 룸이고, 목적지 룸의 입장이 저장을 이어 받는다.
        RoomRef current = player->GetRoom();
        if (current != nullptr && current != room)
        {
            current->DoAsync([this, current, player]()
                {
                    LeaveRoomAndSave(current, player);
                });
        }
        return;
    }

    SaveLeftPlayer(player, std::move(saveData.value()));
}

void ProgressCoordinator::SaveLeftPlayer(const PlayerRef& player, PlayerSaveData saveData)
{
    // 저장 대기는 OnDisconnected가 진행 표시를 바꾸기 전에 이미 걸었다.
    const int64 userId = saveData.userId;

    // 입장 불러오기와 같은 userId 큐에 넣는다. 그사이 맡겨 둔 불러오기는 저장이 끝난 뒤 이 잡에서 실행한다.
    _pushDBJob(userId, [this, player, data = std::move(saveData)]()
        {
            // 연결을 빌리지 못해도 아래에서 대기를 풀어야 한다. 풀지 않으면 다음 입장이 상한까지 막힌다.
            try
            {
                _saveProgress(data);
            }
            catch (const exception& error)
            {
                GLogger->Error("계정 {} 접속 종료 저장 실패: {}", data.userId, error.what());
            }

            // 저장이 실패해도 대기를 푼다. 실패한 저장은 다시 시도하지 않으므로 기다려도 결과가 같다.
            // 표시를 먼저 바꾼다. 그 뒤에 온 밀어내기가 이 해제를 놓쳤는지 표시로 안다(OnDuplicateConnection).
            player->MarkSaved();
            if (optional<SaveGate::ParkedLoad> parked = _saveGate.Release(data.userId))
                parked->run();
        });
}

void ProgressCoordinator::LeaveGameAndSave(int64 userId, const PlayerRef& player)
{
    // 로비 복귀 중에 끊기면 접속 종료가 같은 플레이어로 다시 온다. 진행 표시가 이미 바뀌었으면 아래 판정이 저장 없이 끝나거나,
    // 이미 룸에서 빠진 플레이어를 룸 큐가 찾지 못해 저장을 넣지 않는다. 그래서 저장은 한 번만 돈다.

    // 접속 종료가 계정 등록을 지우기 전에 대기를 건다. 반대로 하면 그사이 새 로비 접속이 저장 전의 진행을 불러온다.
    // 표시를 바꾼 뒤에 걸면, 들어갈 룸이 먼저 저장하고 대기를 푼 다음에 이 대기가 걸려 아무도 풀지 않는다.
    _saveGate.Hold(userId);

    // 표시를 먼저 바꾸고 룸을 읽는다. 순서는 Room::EnterPlayer의 주석을 본다.
    if (player->MarkDisconnected() == false)
    {
        // 룸에 들어간 적이 없으면 바뀐 것이 없어 저장하지 않는다. 큐에 남은 첫 입장도 플레이어를 빼기만 하므로 대기를 여기서 푼다.
        ReleaseHold(userId);
        return;
    }

    // 룸 이동 중이면 떠난 룸이 나온다. 그때 퇴장은 들어갈 룸이 이어 받는다.
    RoomRef room = player->GetRoom();
    if (room == nullptr)
    {
        // 룸에 들어갔다는 표시는 소속 룸을 정한 뒤에만 세운다. 여기로 오면 그 순서가 깨진 것이다.
        GLogger->Error("계정 {}: 룸에 들어간 플레이어의 소속 룸이 없어 저장하지 못합니다", userId);
        ReleaseHold(userId);
        return;
    }

    room->DoAsync([this, room, player]()
        {
            LeaveRoomAndSave(room, player);
        });
}

void ProgressCoordinator::ReleaseHold(int64 userId)
{
    // 저장 잡 밖에서 풀 때만 쓴다. 맡겨 둔 불러오기는 DB를 쓰므로 이 스레드에서 돌리지 않고 같은 계정의 큐로 넘긴다.
    if (optional<SaveGate::ParkedLoad> parked = _saveGate.Release(userId))
        _pushDBJob(userId, std::move(parked->run));
}
