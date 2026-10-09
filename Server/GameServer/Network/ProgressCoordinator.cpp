#include "Core/pch.h"
#include "Network/ProgressCoordinator.h"
#include "DB/ProgressStorage.h"
#include "DB/DAOCommon.h"
#include "Game/Entities/Player.h"
#include "Game/Room/Room.h"

namespace
{
    // 운영 코드의 의존성. 람다는 부를 때에야 전역 객체를 읽으므로 정적 초기화 순서에 기대지 않는다.
    ProgressCoordinator DefaultCoordinator(
        GSaveGate,
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

ProgressCoordinator* GProgressCoordinator = &DefaultCoordinator;

ProgressCoordinator::ProgressCoordinator(SaveGate& saveGate, PushDBJob pushDBJob, SaveProgress saveProgress, ScheduleTimer scheduleTimer)
    : _saveGate(saveGate), _pushDBJob(std::move(pushDBJob)), _saveProgress(std::move(saveProgress)), _scheduleTimer(std::move(scheduleTimer))
{
}

void ProgressCoordinator::OnDisconnected(int64 userId, const PlayerRef& player)
{
    if (player == nullptr)
        return;

    // 룸이 없으면 저장하지 않으므로 걸지 않는다. 걸면 풀어 줄 저장이 없어 다음 입장이 만료까지 막힌다.
    // 룸 입장 잡이 큐에 남아 있다가 저장하는 경우는 LeaveRoomAndSave가 건다.
    if (player->GetRoom() != nullptr)
        _saveGate.Hold(userId);

    // 표시를 먼저 쓰고 룸을 읽는다. 순서는 Room::EnterPlayer의 주석을 본다.
    player->MarkDisconnected();

    // 룸에 들어간 적이 없으면 이 세션에서 바뀐 것이 없으므로 저장하지 않는다.
    RoomRef room = player->GetRoom();
    if (room == nullptr)
        return;

    room->DoAsync([this, room, player]()
        {
            LeaveRoomAndSave(room, player);
        });
}

void ProgressCoordinator::OnDuplicateLogin(int64 userId, const PlayerRef& replacedPlayer)
{
    // 기존 세션의 접속 종료는 송신을 마친 뒤에야 오지만, 새 세션은 곧 목록을 받고 입장할 수 있다.
    // 그 사이의 입장이 저장 전의 진행을 불러오지 않도록 대기를 여기서 먼저 건다.
    // 룸에 없는 세션은 저장하지 않으므로 걸지 않는다.
    if (replacedPlayer != nullptr && replacedPlayer->GetRoom() != nullptr)
        _saveGate.Hold(userId);
}

void ProgressCoordinator::RequestEnter(int64 userId, CallbackType load, CallbackType reject)
{
    // 이 계정의 접속 종료 저장이 남아 있으면 저장 잡이 같은 userId 큐에서 이 불러오기를 실행한다.
    SaveGate::ParkTicket ticket = _saveGate.Park(userId, { load, reject });
    switch (ticket.result)
    {
    case SaveGate::ParkResult::NOT_HELD:
        _pushDBJob(userId, std::move(load));
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
        reject();
        break;
    }
}

void ProgressCoordinator::LeaveRoomAndSave(const RoomRef& room, const PlayerRef& player)
{
    optional<PlayerSaveData> saveData = room->HandleDisconnect(player);
    if (saveData.has_value() == false)
        return;

    // 끊길 때 룸이 없어 OnDisconnected가 대기를 걸지 않은 경우(룸 입장 잡이 큐에 남아 있던 경우)를 여기서 덮는다.
    // 이미 걸려 있으면 아무것도 하지 않는다.
    const int64 userId = saveData->userId;
    _saveGate.Hold(userId);

    // 입장 불러오기와 같은 userId 큐에 넣는다. 그사이 맡겨 둔 불러오기는 저장이 끝난 뒤 이 잡에서 실행한다.
    _pushDBJob(userId, [this, data = std::move(saveData.value())]()
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
            if (optional<SaveGate::ParkedLoad> parked = _saveGate.Release(data.userId))
                parked->run();
        });
}
