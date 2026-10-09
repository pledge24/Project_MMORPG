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

    // 저장할지 모르는 채로 대기부터 건다. 표시를 바꾼 뒤에 걸면, 룸 이동 중에 끊긴 플레이어를 들어갈 룸이 먼저
    // 저장하고 대기를 푼 다음에 이 대기가 걸려 아무도 풀지 않는다.
    _saveGate.Hold(userId);

    // 표시를 먼저 바꾸고 룸을 읽는다. 순서는 Room::EnterPlayer의 주석을 본다.
    if (player->MarkDisconnected() == false)
    {
        // 룸에 들어간 적이 없으면 이 세션에서 바뀐 것이 없으므로 저장하지 않는다. 큐에 남은 첫 룸 입장도 저장하지 않고
        // 플레이어를 빼기만 하므로 대기를 여기서 푼다. 그사이 맡겨 둔 불러오기는 DB 큐로 넘긴다.
        ReleaseHold(userId);
        return;
    }

    // 룸에 들어간 적이 있으면 소속 룸이 정해져 있다. 룸 이동 중이면 떠난 룸이고, 퇴장은 들어갈 룸이 이어 받는다.
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

void ProgressCoordinator::OnDuplicateLogin(int64 userId, const PlayerRef& replacedPlayer)
{
    // 기존 세션의 접속 종료는 송신을 마친 뒤에야 오지만, 새 세션은 곧 목록을 받고 입장할 수 있다.
    // 그 사이의 입장이 저장 전의 진행을 불러오지 않도록 대기를 여기서 먼저 건다.
    // 아직 룸에 없는 플레이어도 끊기기 전에 룸에 들어갈 수 있으므로 건다. 저장할 것이 없으면 접속 종료가 푼다.
    // 입장 전의 세션은 접속 종료가 아무것도 하지 않으므로 걸지 않는다. 걸면 풀어 줄 곳이 없다.
    if (replacedPlayer != nullptr)
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

    // 저장 대기는 OnDisconnected가 진행 표시를 바꾸기 전에 이미 걸었다.
    const int64 userId = saveData->userId;

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

void ProgressCoordinator::ReleaseHold(int64 userId)
{
    // 저장 잡 밖에서 풀 때만 쓴다. 맡겨 둔 불러오기는 DB를 쓰므로 이 스레드에서 돌리지 않고 같은 계정의 큐로 넘긴다.
    if (optional<SaveGate::ParkedLoad> parked = _saveGate.Release(userId))
        _pushDBJob(userId, std::move(parked->run));
}
