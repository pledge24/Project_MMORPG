#pragma once
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

/**
 * navmesh 파일(.navmesh)의 형식.
 *
 * 빌드 도구(Tools/NavBuilder)가 쓰고 게임 서버가 읽는다. 두 쪽이 이 헤더 하나를 함께 컴파일하므로
 * 서버 pch에 기대지 않고 표준 헤더만 쓴다. 파일은 머리(매직, 형식 버전, 에이전트 설정 해시, 본문 크기) 뒤에 Detour 타일 데이터가 온다.
 */
namespace NavMeshFile
{
    /** 파일 이름은 이 접두사 뒤에 맵 번호를 붙인다(NavMesh_1111.navmesh). */
    constexpr const char* FILE_PREFIX = "NavMesh_";
    constexpr const char* FILE_EXTENSION = ".navmesh";
    /** 추출이 쓰는 에이전트 설정 파일. 서버는 이 파일로 기대하는 해시를 계산한다. */
    constexpr const char* AGENT_FILE = "NavAgent.json";

    /** 머리나 빌드 매개변수, recastnavigation 버전을 바꾸면 올린다. 서버는 다른 버전의 파일로 뜨지 않는다. */
    constexpr uint32_t FORMAT_VERSION = 1;
    constexpr char MAGIC[4] = { 'P', '1', 'N', 'M' };
    /** 매직 4 + 버전 4 + 해시 8 + 본문 크기 4 바이트. */
    constexpr size_t HEADER_SIZE = 20;

    /** 플레이어 캡슐과 이동 컴포넌트에서 추출한 에이전트 설정. 길이는 cm, 경사는 도다. */
    struct AgentSettings
    {
        float radius = 0.f;
        float height = 0.f;
        float maxSlope = 0.f;
        float maxStep = 0.f;
    };

    /** 에이전트 설정의 FNV-1a 64비트 해시. 같은 값이면 어느 프로세스에서도 같다. */
    inline uint64_t HashAgent(const AgentSettings& agent)
    {
        uint64_t hash = 14695981039346656037ull;
        const float values[] = { agent.radius, agent.height, agent.maxSlope, agent.maxStep };
        for (float value : values)
        {
            uint8_t bytes[sizeof(float)];
            std::memcpy(bytes, &value, sizeof(float));
            for (uint8_t byte : bytes)
            {
                hash ^= byte;
                hash *= 1099511628211ull;
            }
        }
        return hash;
    }

    /** 추출이 쓴 에이전트 설정 문서를 읽는다. 필드가 없거나 0 이하면 사유. */
    inline std::optional<std::string> ParseAgent(const nlohmann::json& document, AgentSettings& outAgent)
    {
        const std::pair<const char*, float*> fields[] = {
            { "Radius", &outAgent.radius },
            { "Height", &outAgent.height },
            { "MaxSlope", &outAgent.maxSlope },
            { "MaxStep", &outAgent.maxStep },
        };
        for (const auto& [name, value] : fields)
        {
            if (document.is_object() == false || document.contains(name) == false || document[name].is_number() == false)
                return std::string(AGENT_FILE) + ": '" + name + "'가 없거나 수가 아니다";

            *value = document[name].get<float>();
            if (*value <= 0.f)
                return std::string(AGENT_FILE) + ": '" + name + "'가 0 이하다";
        }
        return std::nullopt;
    }

    namespace Detail
    {
        inline void PutU32(std::vector<uint8_t>& out, uint32_t value)
        {
            for (int i = 0; i < 4; ++i)
                out.push_back(static_cast<uint8_t>(value >> (i * 8)));
        }

        inline uint64_t GetLittleEndian(const uint8_t* bytes, int count)
        {
            uint64_t value = 0;
            for (int i = 0; i < count; ++i)
                value |= static_cast<uint64_t>(bytes[i]) << (i * 8);
            return value;
        }
    }

    /** 머리와 Detour 타일 데이터를 이어 파일 내용을 만든다. 머리의 정수는 리틀 엔디언이다. */
    inline std::vector<uint8_t> Write(const AgentSettings& agent, const uint8_t* tileData, uint32_t tileDataSize)
    {
        std::vector<uint8_t> file;
        file.reserve(HEADER_SIZE + tileDataSize);
        file.insert(file.end(), MAGIC, MAGIC + 4);
        Detail::PutU32(file, FORMAT_VERSION);
        const uint64_t hash = HashAgent(agent);
        Detail::PutU32(file, static_cast<uint32_t>(hash));
        Detail::PutU32(file, static_cast<uint32_t>(hash >> 32));
        Detail::PutU32(file, tileDataSize);
        file.insert(file.end(), tileData, tileData + tileDataSize);
        return file;
    }

    /** 파일 안의 Detour 타일 데이터. 파일 버퍼를 가리키므로 버퍼보다 오래 쓰지 않는다. */
    struct TileData
    {
        const uint8_t* data = nullptr;
        uint32_t size = 0;
    };

    /** 파일의 머리를 확인하고 outTile에 Detour 타일 데이터의 위치를 쓴다. 형식, 버전, 해시가 어긋나면 사유. */
    inline std::optional<std::string> Read(const std::vector<uint8_t>& file, uint64_t expectedAgentHash, TileData& outTile)
    {
        if (file.size() < HEADER_SIZE || std::memcmp(file.data(), MAGIC, 4) != 0)
            return std::string("navmesh 파일이 아니다");

        const uint32_t version = static_cast<uint32_t>(Detail::GetLittleEndian(file.data() + 4, 4));
        if (version != FORMAT_VERSION)
            return "형식 버전 " + std::to_string(version) + "은 서버의 형식 버전 " + std::to_string(FORMAT_VERSION) + "과 다르다. 추출을 다시 돌린다";

        const uint64_t hash = Detail::GetLittleEndian(file.data() + 8, 8);
        if (hash != expectedAgentHash)
            return "에이전트 설정 해시가 " + std::string(AGENT_FILE) + "와 다르다. 에이전트 설정이 바뀐 뒤 navmesh를 다시 빌드하지 않았다";

        const uint32_t size = static_cast<uint32_t>(Detail::GetLittleEndian(file.data() + 16, 4));
        if (size == 0 || file.size() != HEADER_SIZE + size)
            return "본문 크기 " + std::to_string(size) + "가 파일 크기와 맞지 않는다";

        outTile.data = file.data() + HEADER_SIZE;
        outTile.size = size;
        return std::nullopt;
    }
}
