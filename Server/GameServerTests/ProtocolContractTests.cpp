#include "pch.h"
#include <gtest/gtest.h>

/*--------------------------------------------------------------
    프로토콜 계약 테스트 — GenPackets.bat 재실행 회귀 그물

    .proto 3개는 원본이 하나(Server/Common/Protobuf/bin/)인데 생성물이
    5개 트리로 XCOPY된다. 그래서 "생성기를 다시 돌렸더니 무언가 조용히
    어긋났다"가 이 프로젝트의 고유 위험이다.

    잡고 싶은 것은 셋이다.
      1) 메시지 집합·선언 순서가 나도 모르게 바뀌는 것
      2) 패킷 ID가 겹치거나 규칙(1000부터 연속)을 벗어나는 것
      3) 생성된 메시지가 등록되지 않거나 직렬화가 깨지는 것

    1·2는 아래 PROTOCOL_MESSAGES 목록 하나로 함께 검사한다. 목록은 중복이
    아니라 **계약 선언**이다 — 여기가 실패하면 ".proto를 바꿨으니 목록도
    의식적으로 갱신하라"는 뜻이다.
    3은 리플렉션으로 전수 검사하므로 메시지가 늘어도 이 파일을 고칠 필요가 없다.
---------------------------------------------------------------*/

// Protocol.proto의 메시지 — 선언 순서 그대로. 생성기가 이 순서대로 1000부터 ID를 매긴다.
#define PROTOCOL_MESSAGES(X)                                                 \
    X(C_PING) X(S_PONG)                                                      \
    X(C_LOGIN) X(S_LOGIN)                                                    \
    X(C_CREATE_CHARACTER) X(S_CREATE_CHARACTER)                              \
    X(C_DELETE_CHARACTER) X(S_DELETE_CHARACTER)                              \
    X(C_ENTER_GAME) X(S_ENTER_GAME)                                          \
    X(C_LEAVE_GAME) X(S_LEAVE_GAME)                                          \
    X(C_MAP_LOAD_COMPLETE)                                                   \
    X(C_ENTER_MAP) X(S_ENTER_MAP)                                            \
    X(C_ENTER_ROOM) X(S_ENTER_ROOM)                                          \
    X(S_SPAWN) X(S_DESPAWN)                                                  \
    X(C_MOVE) X(S_MOVE)                                                      \
    X(C_NORMAL_ATTACK) X(S_NORMAL_ATTACK)                                    \
    X(S_HIT)                                                                 \
    X(C_BUY_ITEM) X(S_BUY_ITEM)                                              \
    X(C_SELL_ITEM) X(S_SELL_ITEM)                                            \
    X(C_EQUIP_GEAR) X(S_EQUIP_GEAR)                                          \
    X(C_UNEQUIP_GEAR) X(S_UNEQUIP_GEAR)                                      \
    X(C_USE_ITEM) X(S_USE_ITEM)                                              \
    X(S_DIE) X(S_REWARD_RESULT)                                              \
    X(C_RESPAWN) X(S_RESPAWN)                                                \
    X(C_CHAT) X(S_CHAT)

namespace
{
    constexpr uint16 FIRST_PACKET_ID = 1000;

    const char* const EXPECTED_MESSAGE_NAMES[] = {
#define AS_STRING(name) #name,
        PROTOCOL_MESSAGES(AS_STRING)
#undef AS_STRING
    };

    const uint16 DECLARED_PACKET_IDS[] = {
#define AS_PACKET_ID(name) PKT_##name,
        PROTOCOL_MESSAGES(AS_PACKET_ID)
#undef AS_PACKET_ID
    };

    constexpr size_t EXPECTED_MESSAGE_COUNT = sizeof(EXPECTED_MESSAGE_NAMES) / sizeof(EXPECTED_MESSAGE_NAMES[0]);

    const google::protobuf::FileDescriptor* ProtocolFile()
    {
        // 파일명("Protocol.proto")으로 찾지 않는다 — protoc에 넘긴 경로에 따라 달라진다.
        // 생성된 타입에서 역으로 얻으면 그런 가정이 필요 없다.
        return Protocol::C_PING::descriptor()->file();
    }

    // 리플렉션으로 모든 필드에 기본값이 아닌 값을 채운다.
    // 빈 메시지를 왕복시키면 빈 바이트열이 나와서 아무것도 검증하지 못한다.
    void FillEveryField(google::protobuf::Message& message, int depth)
    {
        using google::protobuf::FieldDescriptor;

        const google::protobuf::Descriptor* descriptor = message.GetDescriptor();
        const google::protobuf::Reflection* reflection = message.GetReflection();

        for (int i = 0; i < descriptor->field_count(); i++)
        {
            const FieldDescriptor* field = descriptor->field(i);

            // oneof는 첫 멤버만 채운다. 뒤 멤버를 채우면 앞 멤버를 지워서
            // "채운 것"과 "왕복한 것"의 비교가 흔들린다.
            if (field->containing_oneof() != nullptr && field->index_in_oneof() != 0)
                continue;

            const bool repeated = field->is_repeated();

            switch (field->cpp_type())
            {
            case FieldDescriptor::CPPTYPE_INT32:
                repeated ? reflection->AddInt32(&message, field, 7 + i)
                         : reflection->SetInt32(&message, field, 7 + i);
                break;
            case FieldDescriptor::CPPTYPE_INT64:
                repeated ? reflection->AddInt64(&message, field, 700000 + i)
                         : reflection->SetInt64(&message, field, 700000 + i);
                break;
            case FieldDescriptor::CPPTYPE_UINT32:
                repeated ? reflection->AddUInt32(&message, field, 7 + i)
                         : reflection->SetUInt32(&message, field, 7 + i);
                break;
            case FieldDescriptor::CPPTYPE_UINT64:
                repeated ? reflection->AddUInt64(&message, field, 700000 + i)
                         : reflection->SetUInt64(&message, field, 700000 + i);
                break;
            case FieldDescriptor::CPPTYPE_FLOAT:
                repeated ? reflection->AddFloat(&message, field, 2.5f)
                         : reflection->SetFloat(&message, field, 2.5f);
                break;
            case FieldDescriptor::CPPTYPE_DOUBLE:
                repeated ? reflection->AddDouble(&message, field, 1.25)
                         : reflection->SetDouble(&message, field, 1.25);
                break;
            case FieldDescriptor::CPPTYPE_BOOL:
                repeated ? reflection->AddBool(&message, field, true)
                         : reflection->SetBool(&message, field, true);
                break;
            case FieldDescriptor::CPPTYPE_ENUM:
            {
                // 마지막 값을 쓴다. 0번은 대개 *_NONE이라 기본값과 구분되지 않는다.
                const google::protobuf::EnumDescriptor* enumType = field->enum_type();
                const google::protobuf::EnumValueDescriptor* value =
                    enumType->value(enumType->value_count() - 1);
                repeated ? reflection->AddEnum(&message, field, value)
                         : reflection->SetEnum(&message, field, value);
                break;
            }
            case FieldDescriptor::CPPTYPE_STRING:
                repeated ? reflection->AddString(&message, field, "왕복 검사 문자열")
                         : reflection->SetString(&message, field, "왕복 검사 문자열");
                break;
            case FieldDescriptor::CPPTYPE_MESSAGE:
            {
                if (depth <= 0)
                    break;

                google::protobuf::Message* child = repeated
                    ? reflection->AddMessage(&message, field)
                    : reflection->MutableMessage(&message, field);
                FillEveryField(*child, depth - 1);
                break;
            }
            }
        }
    }
}

// (1) 메시지 집합과 선언 순서가 계약대로인가.
// 실패하면 .proto가 바뀐 것이다 — 위 PROTOCOL_MESSAGES를 의식적으로 갱신하라는 신호다.
TEST(ProtocolContract, MessageSetMatchesDeclaredList)
{
    const google::protobuf::FileDescriptor* file = ProtocolFile();
    ASSERT_NE(file, nullptr);

    ASSERT_EQ(static_cast<size_t>(file->message_type_count()), EXPECTED_MESSAGE_COUNT)
        << ".proto의 메시지 개수가 바뀌었다";

    for (size_t i = 0; i < EXPECTED_MESSAGE_COUNT; i++)
    {
        EXPECT_STREQ(file->message_type(static_cast<int>(i))->name().c_str(), EXPECTED_MESSAGE_NAMES[i])
            << i << "번째 메시지의 이름 또는 선언 순서가 바뀌었다";
    }
}

// (2) 패킷 ID가 1000부터 선언 순서대로 겹치지 않고 이어지는가.
// 생성기가 ID를 붙이는 규칙이 이것이고, 어긋나면 클라·서버가 서로 다른 패킷을 읽는다.
TEST(ProtocolContract, PacketIdsAreContiguousAndUnique)
{
    const size_t count = sizeof(DECLARED_PACKET_IDS) / sizeof(DECLARED_PACKET_IDS[0]);
    ASSERT_EQ(count, EXPECTED_MESSAGE_COUNT);

    set<uint16> seen;
    for (size_t i = 0; i < count; i++)
    {
        EXPECT_EQ(DECLARED_PACKET_IDS[i], FIRST_PACKET_ID + static_cast<uint16>(i))
            << EXPECTED_MESSAGE_NAMES[i] << " 의 패킷 ID가 선언 순서와 어긋난다";
        EXPECT_TRUE(seen.insert(DECLARED_PACKET_IDS[i]).second)
            << EXPECTED_MESSAGE_NAMES[i] << " 의 패킷 ID가 다른 패킷과 겹친다";
    }
}

// (3) 모든 메시지가 등록돼 있고 값을 채운 채로 왕복하는가.
// 리플렉션으로 도므로 메시지가 늘어도 이 테스트는 고치지 않는다.
TEST(ProtocolContract, EveryMessageRoundTrips)
{
    const google::protobuf::FileDescriptor* file = ProtocolFile();
    ASSERT_NE(file, nullptr);

    for (int i = 0; i < file->message_type_count(); i++)
    {
        const google::protobuf::Descriptor* descriptor = file->message_type(i);
        SCOPED_TRACE(descriptor->full_name());

        const google::protobuf::Message* prototype =
            google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor);
        ASSERT_NE(prototype, nullptr) << "생성된 메시지가 등록되지 않았다";

        unique_ptr<google::protobuf::Message> original(prototype->New());
        FillEveryField(*original, 3);

        // 필드가 있는 메시지인데 바이트가 0이면 채우기가 실패한 것이고, 그 왕복은
        // 아무것도 검증하지 못한다. 필드 없는 메시지(C_PING, S_PONG, C_LEAVE_GAME,
        // S_LEAVE_GAME, C_MAP_LOAD_COMPLETE)는 0바이트가 정상이므로 여기서 제외한다.
        if (descriptor->field_count() > 0)
            ASSERT_GT(original->ByteSizeLong(), 0u) << "필드를 하나도 채우지 못해 검증이 무의미하다";

        string wire;
        ASSERT_TRUE(original->SerializeToString(&wire));

        unique_ptr<google::protobuf::Message> parsed(prototype->New());
        ASSERT_TRUE(parsed->ParseFromString(wire)) << "직렬화한 바이트를 되읽지 못했다";
        EXPECT_EQ(parsed->SerializeAsString(), wire);
    }
}
