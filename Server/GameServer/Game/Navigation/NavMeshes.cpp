#include "Core/pch.h"
#include "Game/Navigation/NavMeshes.h"
#include <charconv>
#include <filesystem>
#include <fstream>

namespace
{
    /** 추출 산출물 폴더. 작업 디렉터리 기준이고, Gamedata가 배치 데이터를 읽는 폴더와 같다. */
    constexpr string_view EXTRACTED_DIR = "Game/Data/Extracted/";

    /** NavMesh_<맵 번호>.navmesh에서 맵 번호를 꺼낸다. 형식이 아니면 nullopt. */
    optional<int32> ParseMapId(string_view fileName)
    {
        const string_view prefix = NavMeshFile::FILE_PREFIX;
        const string_view extension = NavMeshFile::FILE_EXTENSION;
        if (fileName.starts_with(prefix) == false || fileName.ends_with(extension) == false || fileName.size() <= prefix.size() + extension.size())
            return nullopt;

        const string_view digits = fileName.substr(prefix.size(), fileName.size() - prefix.size() - extension.size());
        int32 mapId = 0;
        const auto [end, error] = from_chars(digits.data(), digits.data() + digits.size(), mapId);
        if (error != errc() || end != digits.data() + digits.size())
            return nullopt;

        return mapId;
    }
}

map<int32, shared_ptr<const NavMesh>> NavMeshes::s_navMeshes;

bool NavMeshes::LoadAll()
{
    NavMeshDocuments documents;

    const string agentPath = string(EXTRACTED_DIR) + NavMeshFile::AGENT_FILE;
    ifstream agentFile(agentPath);
    if (agentFile.is_open() == false)
    {
        GLogger->Error("에이전트 설정 {}를 열지 못했다. 추출을 돌린다", agentPath);
        return false;
    }
    try
    {
        documents.agent = Json::parse(agentFile);
    }
    catch (const Json::parse_error& e)
    {
        GLogger->Error("{}: Json 문법 오류(byte {}): {}", NavMeshFile::AGENT_FILE, e.byte, e.what());
        return false;
    }

    error_code directoryError;
    for (const filesystem::directory_entry& entry : filesystem::directory_iterator(EXTRACTED_DIR, directoryError))
    {
        const string fileName = entry.path().filename().string();
        if (fileName.starts_with(NavMeshFile::FILE_PREFIX) == false)
            continue;

        ifstream file(entry.path(), ios::binary);
        if (file.is_open() == false)
        {
            GLogger->Error("navmesh {}를 열지 못했다", fileName);
            return false;
        }
        documents.files[fileName].assign(istreambuf_iterator<char>(file), istreambuf_iterator<char>());
    }
    if (directoryError)
    {
        GLogger->Error("추출 산출물 폴더 {}를 읽지 못했다: {}", EXTRACTED_DIR, directoryError.message());
        return false;
    }

    if (optional<string> error = Load(documents))
    {
        GLogger->Error("navmesh를 불러오지 못했다. {}", error.value());
        return false;
    }

    GLogger->Info("맵 {}개의 navmesh를 불러왔다", s_navMeshes.size());
    return true;
}

optional<string> NavMeshes::Load(const NavMeshDocuments& documents)
{
    NavMeshFile::AgentSettings agent;
    if (optional<string> error = NavMeshFile::ParseAgent(documents.agent, OUT agent))
        return error;

    map<int32, shared_ptr<const NavMesh>> navMeshes;
    for (const auto& [fileName, file] : documents.files)
    {
        const optional<int32> mapId = ParseMapId(fileName);
        if (mapId.has_value() == false)
            return format("{}: 파일 이름이 {}<맵 번호>{} 형식이 아니다", fileName, NavMeshFile::FILE_PREFIX, NavMeshFile::FILE_EXTENSION);
        if (Gamedata::GetMaps().contains(mapId.value()) == false)
            return format("{}: 맵 표에 없는 맵 {}의 navmesh다", fileName, mapId.value());

        shared_ptr<const NavMesh> navMesh;
        if (optional<string> error = NavMesh::Load(file, agent, OUT navMesh))
            return format("{}: {}", fileName, error.value());

        navMeshes.emplace(mapId.value(), std::move(navMesh));
    }

    for (const auto& [mapId, mapRow] : Gamedata::GetMaps())
    {
        if (navMeshes.contains(mapId) == false)
            return format("맵 {}의 navmesh({}{}{})가 없다. 추출을 돌린다", mapId, NavMeshFile::FILE_PREFIX, mapId, NavMeshFile::FILE_EXTENSION);
    }

    Install(std::move(navMeshes));
    return nullopt;
}

void NavMeshes::Install(map<int32, shared_ptr<const NavMesh>> navMeshes)
{
    s_navMeshes = std::move(navMeshes);
}

shared_ptr<const NavMesh> NavMeshes::Find(int32 mapId)
{
    auto it = s_navMeshes.find(mapId);
    return it == s_navMeshes.end() ? nullptr : it->second;
}
