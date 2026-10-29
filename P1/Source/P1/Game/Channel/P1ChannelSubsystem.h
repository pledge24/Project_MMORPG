#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Game/Channel/P1ChannelEntry.h"
#include "Utils/Types.h"
#include "P1ChannelSubsystem.generated.h"

namespace Protocol
{
    class S_ENTER_ROOM;
    class S_CHANNEL_LIST;
    class S_CHANGE_CHANNEL;
    enum EntryResult : int;
}

DECLARE_MULTICAST_DELEGATE_OneParam(FOnChannelChanged, int32 /*Channel*/);
DECLARE_MULTICAST_DELEGATE(FOnChannelListChanged);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnChannelChangeFailed, const FText& /*Message*/);

/**
 * 채널의 송신 창구이자 지금 채널과 채널 목록의 사본.
 *
 * 같은 맵의 다른 채널로 옮기는 요청과 채널 목록 요청을 보내고, 서버가 알린 지금 채널과 채널마다의 인원을 보관해 알린다.
 * 레벨과 무관하게 살아 있어 화면이 언제든 지금 채널을 읽는다. 모든 멤버 함수는 게임 스레드 전용이다.
 */
UCLASS()
class P1_API UP1ChannelSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

    //~ Current Channel
public:
    /** 룸에 들어간 적이 없으면 0. */
    int32 GetChannel() const { return Channel; }

    /** 성공한 룸 입장 응답의 채널을 지금 채널로 삼고, 채널 변경의 대기를 푼다. 채널이 바뀌었을 때만 알린다. */
    void HandleEnterRoom(const Protocol::S_ENTER_ROOM& EnterRoomPkt);

    FOnChannelChanged OnChannelChanged;

private:
    int32 Channel = 0;

    //~ Channel List
public:
    void RequestChannelList();
    void HandleChannelList(const Protocol::S_CHANNEL_LIST& ChannelListPkt);

    /** 채널 번호 순서다. 목록을 받은 적이 없으면 비어 있다. */
    const TArray<FP1ChannelEntry>& GetChannels() const { return Channels; }

    FOnChannelListChanged OnChannelListChanged;

private:
    TArray<FP1ChannelEntry> Channels;

    //~ Channel Change
public:
    /** 지금 채널이거나 앞선 요청의 응답을 기다리는 중이면 보내지 않고 false. 거절되면 OnChannelChangeFailed로 알린다. */
    bool RequestChangeChannel(int32 TargetChannel);

    /** 채널 변경을 보내고 룸 입장 응답이나 거절을 기다리는 중이면 true. */
    bool IsChangingChannel() const { return bChangingChannel; }

    /** 거절되면 대기를 풀고 결과의 문구를 알린다. 성공하면 뒤이은 룸 입장 응답이 대기를 푼다. */
    void HandleChangeChannel(const Protocol::S_CHANGE_CHANNEL& ChangeChannelPkt);

    /** 서버가 보낸 채널 변경 결과 값을 화면 문구로 바꾼다. 모르는 값이면 값을 담은 일반 오류 문구. */
    static FText GetResultText(Protocol::EntryResult Result);

    FOnChannelChangeFailed OnChannelChangeFailed;

private:
    bool bChangingChannel = false;

    //~ 공통
public:
    /** 요청을 보내는 함수를 바꾼다. 테스트가 송신을 기록하려고 쓴다. 비우면 연결로 보낸다. */
    using FPacketSender = TFunction<void(SendBufferRef)>;
    void SetPacketSender(FPacketSender InSender) { PacketSender = MoveTemp(InSender); }

    /** 이 서브시스템의 델리게이트에서 Listener가 붙인 것을 모두 뗀다. 레벨과 함께 사라지는 구독자가 사라지기 전에 부른다. */
    void RemoveListener(const UObject* Listener);

private:
    template <typename T>
    void SendRequest(T& Pkt);

    FPacketSender PacketSender;
};
