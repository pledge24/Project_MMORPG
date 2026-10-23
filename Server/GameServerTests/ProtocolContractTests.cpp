#include "Core/pch.h"
#include <gtest/gtest.h>
#include <optional>

/*--------------------------------------------------------------
    프로토콜 계약 테스트 — 생성 스크립트 재실행 회귀 그물

    스키마 원본은 Protocol/Schema/ 한 곳인데 생성물이 서버, 클라이언트,
    더미 클라이언트로 복사된다. 그래서 "생성기를 다시 돌렸더니 무언가 조용히
    어긋났다"가 이 프로젝트의 고유 위험이다.

    잡고 싶은 것은 여섯이다.
      0) 스키마 파일이 생기거나 범위가 바뀌었는데 아래 표가 모르는 것
      1) 스키마 파일이 빌드에서 빠지거나, 파일의 패킷 집합·선언 순서가 나도 모르게 바뀌는 것
      2) 패킷 번호가 그 파일의 범위를 벗어나거나, 범위 안에서 선언 순서를 따르지 않는 것
      3) 두 파일의 범위나 두 패킷의 번호가 겹치는 것
      4) 생성된 메시지가 등록되지 않거나 직렬화가 깨지는 것
      5) 스키마 파일끼리의 import가 정한 방향을 벗어나는 것

    0~3과 5는 아래 SCHEMA_FILES 표 하나로 검사한다. 표는 중복이 아니라
    **계약 선언**이다 — 여기가 실패하면 ".proto를 바꿨으니 표도 의식적으로
    갱신하라"는 뜻이다. 0은 생성기가 핸들러 헤더에 적은 PACKET_SCHEMA_FILES
    (생성기가 읽은 스키마 파일과 각 파일의 `// @packet_id_range`)와 표를 대조한다.
    4는 리플렉션으로 전수 검사하므로 메시지가 늘어도 이 검사는 고칠 필요가 없다.
---------------------------------------------------------------*/

// 파일마다의 패킷 — 선언 순서 그대로. 생성기가 이 순서대로 그 파일 범위의 첫 번호부터 매긴다.
#define COMMON_PACKETS(X)                                                    \
    X(S_DISCONNECT)

#define LOBBY_PACKETS(X)                                                     \
    X(C_CONNECT_LOBBY) X(S_CONNECT_LOBBY)                                    \
    X(S_CHARACTER_LIST)                                                      \
    X(C_CREATE_CHARACTER) X(S_CREATE_CHARACTER)                              \
    X(C_DELETE_CHARACTER) X(S_DELETE_CHARACTER)

#define ENTRY_PACKETS(X)                                                     \
    X(C_ENTER_GAME) X(S_ENTER_GAME)                                          \
    X(C_MAP_LOAD_COMPLETE)                                                   \
    X(S_ENTER_ROOM)                                                          \
    X(C_USE_PORTAL) X(S_USE_PORTAL)                                          \
    X(C_RESPAWN) X(S_RESPAWN)                                                \
    X(C_RETURN_TO_LOBBY) X(S_RETURN_TO_LOBBY)

#define ENTITY_PACKETS(X)                                                    \
    X(S_SPAWN) X(S_DESPAWN)                                                  \
    X(C_MOVE) X(S_MOVE)                                                      \
    X(S_GEAR_APPEARANCE)

#define COMBAT_PACKETS(X)                                                    \
    X(C_NORMAL_ATTACK) X(S_NORMAL_ATTACK)                                    \
    X(S_ATTACK)                                                              \
    X(S_HIT)                                                                 \
    X(S_DIE)                                                                 \
    X(S_REWARD)

#define ITEM_PACKETS(X)                                                      \
    X(C_BUY_ITEM) X(S_BUY_ITEM)                                              \
    X(C_SELL_ITEM) X(S_SELL_ITEM)                                            \
    X(C_EQUIP_GEAR) X(S_EQUIP_GEAR)                                          \
    X(C_UNEQUIP_GEAR) X(S_UNEQUIP_GEAR)                                      \
    X(C_USE_ITEM) X(S_USE_ITEM)                                              \
    X(C_SET_QUICK_SLOT) X(S_SET_QUICK_SLOT)                                  \
    X(C_USE_QUICK_SLOT)

#define CHAT_PACKETS(X)                                                      \
    X(C_CHAT) X(S_CHAT)                                                      \
    X(S_CHAT_MESSAGE)

namespace
{
    struct DeclaredPacket
    {
        const char* name;
        uint16 id;          // 생성된 PKT_ 값
    };

    struct PacketIdRange
    {
        uint16 first;
        uint16 last;
    };

    struct SchemaFile
    {
        // protoc가 붙인 파일 이름. 스키마끼리의 import 경로와 같으므로 생성 스크립트가 바꿀 수 없다.
        const char* name;
        PacketIdRange range;
        vector<DeclaredPacket> packets;
        // 이 파일이 import하는 스키마 파일. Common은 아무것도 import하지 않고, Common이 아닌 도메인 파일을
        // import하는 것은 Entry 하나다. 도메인 파일끼리 얽히면 한 도메인의 계약을 고칠 때 다른 도메인이 함께 흔들린다.
        set<string> imports;
    };

#define AS_DECLARED_PACKET(name) DeclaredPacket{ #name, PKT_##name },

    const SchemaFile SCHEMA_FILES[] = {
        { "Common.proto", PacketIdRange{ 100, 199 }, { COMMON_PACKETS(AS_DECLARED_PACKET) }, {} },
        { "Lobby.proto", PacketIdRange{ 200, 299 }, { LOBBY_PACKETS(AS_DECLARED_PACKET) }, { "Common.proto" } },
        { "Entry.proto", PacketIdRange{ 300, 399 }, { ENTRY_PACKETS(AS_DECLARED_PACKET) }, { "Common.proto", "Entity.proto", "Item.proto" } },
        { "Entity.proto", PacketIdRange{ 400, 499 }, { ENTITY_PACKETS(AS_DECLARED_PACKET) }, { "Common.proto" } },
        { "Combat.proto", PacketIdRange{ 500, 599 }, { COMBAT_PACKETS(AS_DECLARED_PACKET) }, { "Common.proto" } },
        { "Item.proto", PacketIdRange{ 600, 699 }, { ITEM_PACKETS(AS_DECLARED_PACKET) }, { "Common.proto" } },
        { "Chat.proto", PacketIdRange{ 700, 799 }, { CHAT_PACKETS(AS_DECLARED_PACKET) }, {} },
    };

#undef AS_DECLARED_PACKET

    bool IsPacketName(const string& messageName)
    {
        return messageName.starts_with("C_") || messageName.starts_with("S_");
    }

    const google::protobuf::FileDescriptor* FindSchemaFile(const char* name)
    {
        return google::protobuf::DescriptorPool::generated_pool()->FindFileByName(name);
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

// (0) 표가 생성기가 읽은 스키마 파일을 빠짐없이 담고, 파일마다 범위가 .proto의 선언과 같은가.
// 모든 스키마 파일이 범위를 선언한다. 범위가 없는 파일을 생성기는 0~0으로 적으므로 여기서 걸린다.
TEST(ProtocolContract, SchemaTableMatchesGenerator)
{
    map<string, const PacketSchemaFile*> generated;
    for (const PacketSchemaFile& file : PACKET_SCHEMA_FILES)
        generated.emplace(file.name, &file);

    set<string> declared;
    for (const SchemaFile& schema : SCHEMA_FILES)
    {
        SCOPED_TRACE(schema.name);
        declared.insert(schema.name);

        const auto it = generated.find(schema.name);
        ASSERT_NE(it, generated.end()) << "생성기가 읽지 않은 스키마 파일이 표에 있다";

        const PacketSchemaFile& file = *it->second;
        EXPECT_EQ(schema.range.first, file.firstPacketId) << "첫 번호가 .proto와 다르다";
        EXPECT_EQ(schema.range.last, file.lastPacketId) << "끝 번호가 .proto와 다르다";
    }

    for (const auto& [name, file] : generated)
        EXPECT_TRUE(declared.contains(name)) << name << " 이 표에 없다. 스키마 파일을 더했으면 표에도 더한다";
}

// (1) 스키마 파일이 모두 빌드에 들어 있고, 파일마다 패킷 집합과 선언 순서가 계약대로인가.
// 실패하면 .proto가 바뀌었거나 새 .pb.cc를 프로젝트에 등록하지 않은 것이다.
TEST(ProtocolContract, EverySchemaFileDeclaresItsPackets)
{
    for (const SchemaFile& schema : SCHEMA_FILES)
    {
        SCOPED_TRACE(schema.name);

        const google::protobuf::FileDescriptor* file = FindSchemaFile(schema.name);
        ASSERT_NE(file, nullptr) << "스키마 파일이 빌드에 없다. 생성된 .pb.cc를 프로젝트에 등록했는지 본다";

        vector<string> actualPackets;
        for (int i = 0; i < file->message_type_count(); i++)
        {
            const string& messageName = file->message_type(i)->name();
            if (IsPacketName(messageName))
                actualPackets.push_back(messageName);
        }

        ASSERT_EQ(actualPackets.size(), schema.packets.size()) << "파일의 패킷 개수가 바뀌었다";
        for (size_t i = 0; i < actualPackets.size(); i++)
        {
            EXPECT_EQ(actualPackets[i], schema.packets[i].name)
                << i << "번째 패킷의 이름 또는 선언 순서가 바뀌었다";
        }
    }
}

// (2) 패킷 번호가 그 파일 범위의 첫 번호부터 선언 순서대로 이어지고, 범위를 넘지 않는가.
// 생성기가 번호를 붙이는 규칙이 이것이고, 어긋나면 클라·서버가 서로 다른 패킷을 읽는다.
TEST(ProtocolContract, PacketIdsFollowDeclarationOrderWithinFileRange)
{
    for (const SchemaFile& schema : SCHEMA_FILES)
    {
        SCOPED_TRACE(schema.name);

        for (size_t i = 0; i < schema.packets.size(); i++)
        {
            const DeclaredPacket& packet = schema.packets[i];
            EXPECT_EQ(packet.id, schema.range.first + i)
                << packet.name << " 의 패킷 번호가 파일 안의 선언 순서와 어긋난다";
            EXPECT_LE(packet.id, schema.range.last)
                << packet.name << " 의 패킷 번호가 파일의 범위를 넘는다";
        }
    }
}

// (3) 파일의 번호 범위끼리, 패킷 번호끼리 겹치지 않는가.
// 범위가 겹치면 한 파일에 패킷을 더할 때 다른 파일의 번호와 부딪힌다.
TEST(ProtocolContract, PacketIdRangesAndIdsDoNotOverlap)
{
    vector<pair<const char*, PacketIdRange>> ranges;
    for (const SchemaFile& schema : SCHEMA_FILES)
    {
        // 핸들러 표의 칸 수가 UINT16_MAX라 마지막 번호 65535는 쓸 수 없다.
        EXPECT_LE(schema.range.first, schema.range.last) << schema.name;
        EXPECT_LT(schema.range.last, UINT16_MAX) << schema.name;

        for (const auto& [otherName, other] : ranges)
        {
            const bool overlaps = schema.range.first <= other.last && other.first <= schema.range.last;
            EXPECT_FALSE(overlaps) << schema.name << " 의 번호 범위가 " << otherName << " 의 범위와 겹친다";
        }
        ranges.emplace_back(schema.name, schema.range);
    }

    map<uint16, const char*> seen;
    for (const SchemaFile& schema : SCHEMA_FILES)
    {
        for (const DeclaredPacket& packet : schema.packets)
        {
            const auto [it, inserted] = seen.emplace(packet.id, packet.name);
            EXPECT_TRUE(inserted) << packet.name << " 의 패킷 번호가 " << it->second << " 와 겹친다";
        }
    }
}

// (4) 모든 스키마 파일의 모든 메시지가 등록돼 있고 값을 채운 채로 왕복하는가.
// 리플렉션으로 도므로 메시지가 늘어도 이 테스트는 고치지 않는다.
TEST(ProtocolContract, EveryMessageRoundTrips)
{
    for (const SchemaFile& schema : SCHEMA_FILES)
    {
        const google::protobuf::FileDescriptor* file = FindSchemaFile(schema.name);
        ASSERT_NE(file, nullptr) << schema.name;

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
            // 아무것도 검증하지 못한다. 필드 없는 메시지(C_MAP_LOAD_COMPLETE, C_RETURN_TO_LOBBY)는
            // 0바이트가 정상이므로 여기서 제외한다.
            if (descriptor->field_count() > 0)
                ASSERT_GT(original->ByteSizeLong(), 0u) << "필드를 하나도 채우지 못해 검증이 무의미하다";

            string wire;
            ASSERT_TRUE(original->SerializeToString(&wire));

            unique_ptr<google::protobuf::Message> parsed(prototype->New());
            ASSERT_TRUE(parsed->ParseFromString(wire)) << "직렬화한 바이트를 되읽지 못했다";
            EXPECT_EQ(parsed->SerializeAsString(), wire);
        }
    }
}

// (5) 스키마 파일마다 import가 표와 같은가.
// 표에 없는 import가 생기면 자료형이 쓰는 도메인이 아닌 파일에 있거나, 도메인 파일끼리 얽힌 것이다.
TEST(ProtocolContract, SchemaImportsFollowDomainDirection)
{
    for (const SchemaFile& schema : SCHEMA_FILES)
    {
        SCOPED_TRACE(schema.name);

        const google::protobuf::FileDescriptor* file = FindSchemaFile(schema.name);
        ASSERT_NE(file, nullptr);

        set<string> actualImports;
        for (int i = 0; i < file->dependency_count(); i++)
            actualImports.insert(string(file->dependency(i)->name()));

        EXPECT_EQ(actualImports, schema.imports) << "import가 바뀌었다";
    }
}
