#include "Game/Channel/P1ChannelSubsystem.h"
#include "Network/P1PacketSender.h"
#include "Entry.pb.h"
#include "Utils/LogCategory.h"

void UP1ChannelSubsystem::HandleEnterRoom(const Protocol::S_ENTER_ROOM& EnterRoomPkt)
{
    if (EnterRoomPkt.result() != Protocol::ENTRY_RESULT_SUCCESS)
        return;

    // 채널 변경의 성공 응답 뒤에는 들어갈 룸의 입장 응답이 온다. 그 전에 다시 보내면 옮기는 중의 플레이어라 서버가 버린다.
    bChangingChannel = false;

    // 같은 룸 안의 순간이동(같은 맵의 포털, 리스폰)도 입장 응답을 받는다. 채널이 그대로면 알리지 않는다.
    if (Channel == EnterRoomPkt.channel())
        return;

    Channel = EnterRoomPkt.channel();
    OnChannelChanged.Broadcast(Channel);
}

void UP1ChannelSubsystem::RequestChannelList()
{
    Protocol::C_CHANNEL_LIST Pkt;
    SendRequest(Pkt);
}

void UP1ChannelSubsystem::HandleChannelList(const Protocol::S_CHANNEL_LIST& ChannelListPkt)
{
    Channels.Reset();
    for (const Protocol::ChannelInfo& Info : ChannelListPkt.channels())
    {
        FP1ChannelEntry Entry;
        Entry.Channel = Info.channel();
        Entry.PlayerCount = Info.player_count();
        Channels.Add(Entry);
    }

    OnChannelListChanged.Broadcast();
}

bool UP1ChannelSubsystem::RequestChangeChannel(int32 TargetChannel)
{
    // 서버는 지금 채널로 옮기는 요청을 거절하므로 보내지 않는다. 화면은 지금 채널의 이동 버튼을 막는다.
    if (bChangingChannel || TargetChannel == Channel)
        return false;

    Protocol::C_CHANGE_CHANNEL Pkt;
    Pkt.set_channel(TargetChannel);
    SendRequest(Pkt);

    bChangingChannel = true;
    return true;
}

void UP1ChannelSubsystem::HandleChangeChannel(const Protocol::S_CHANGE_CHANNEL& ChangeChannelPkt)
{
    if (ChangeChannelPkt.result() == Protocol::ENTRY_RESULT_SUCCESS)
        return;

    UE_LOG(LogP1Network, Warning, TEXT("채널 변경이 거절됐습니다: %s"), UTF8_TO_TCHAR(Protocol::EntryResult_Name(ChangeChannelPkt.result()).c_str()));

    bChangingChannel = false;
    OnChannelChangeFailed.Broadcast(GetResultText(ChangeChannelPkt.result()));
}

FText UP1ChannelSubsystem::GetResultText(Protocol::EntryResult Result)
{
    // 서버는 문구를 보내지 않는다. 채널 변경의 결과 값을 더하면 여기에도 문구를 더한다. 빠뜨리면 UE 테스트가 알린다.
    switch (Result)
    {
    case Protocol::ENTRY_RESULT_SUCCESS:
        return FText::FromString(TEXT("채널을 옮겼습니다."));
    case Protocol::ENTRY_RESULT_DEAD:
        return FText::FromString(TEXT("사망한 상태에서는 채널을 옮길 수 없습니다."));
    case Protocol::ENTRY_RESULT_MAP_LOADING:
        return FText::FromString(TEXT("맵을 불러오는 중에는 채널을 옮길 수 없습니다."));
    case Protocol::ENTRY_RESULT_SAME_CHANNEL:
        return FText::FromString(TEXT("이미 그 채널에 있습니다."));
    case Protocol::ENTRY_RESULT_INVALID_CHANNEL:
        return FText::FromString(TEXT("없는 채널입니다."));
    case Protocol::ENTRY_RESULT_INTERNAL_ERROR:
        return FText::FromString(TEXT("서버 오류로 채널을 옮기지 못했습니다."));
    default:
        break;
    }

    // 클라이언트보다 새 서버가 보낸 값이다.
    return FText::FromString(FString::Printf(TEXT("알 수 없는 오류입니다. (%d)"), static_cast<int32>(Result)));
}

void UP1ChannelSubsystem::RemoveListener(const UObject* Listener)
{
    OnChannelChanged.RemoveAll(Listener);
    OnChannelListChanged.RemoveAll(Listener);
    OnChannelChangeFailed.RemoveAll(Listener);
}

template <typename T>
void UP1ChannelSubsystem::SendRequest(T& Pkt)
{
    // 테스트가 바꾼 송신 함수가 있으면 그것으로, 없으면 게임 인스턴스의 연결로 보낸다.
    if (PacketSender)
    {
        PacketSender(ClientPacketHandler::MakeSerializedPacket(Pkt));
        return;
    }

    FP1PacketSender::Send(GetGameInstance(), Pkt);
}
