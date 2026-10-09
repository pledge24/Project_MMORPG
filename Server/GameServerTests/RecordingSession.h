#pragma once

/**
 * 보낸 패킷을 소켓 대신 목록에 쌓는 세션. 테스트만 쓴다.
 * 룸과 핸들러가 이 세션에 보낸 패킷을 꺼내 종류와 내용을 대조한다.
 */
class RecordingSession : public GameSession
{
public:
    virtual void Send(SendBufferRef sendBuffer) override
    {
        _sent.push_back(sendBuffer);
    }

    /** 보낸 패킷 중 id가 packetId인 것을 PacketType으로 풀어 돌려준다. */
    template<typename PacketType>
    vector<PacketType> SentPackets(uint16 packetId) const
    {
        vector<PacketType> packets;
        for (const SendBufferRef& buffer : _sent)
        {
            const PacketHeader* header = reinterpret_cast<const PacketHeader*>(buffer->Buffer());
            if (header->id != packetId)
                continue;

            PacketType& packet = packets.emplace_back();
            packet.ParseFromArray(buffer->Buffer() + sizeof(PacketHeader), header->size - static_cast<int32>(sizeof(PacketHeader)));
        }
        return packets;
    }

private:
    vector<SendBufferRef> _sent;
};
