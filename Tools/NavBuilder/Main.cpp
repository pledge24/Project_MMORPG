// 추출 커맨드렛이 쓴 맵마다의 충돌 형상으로 navmesh를 빌드한다. 추출 스크립트(P1/Scripts/Invoke-PlacementExtract.ps1)가 부른다.
//
// NavBuilder.exe --agent <NavAgent.json> --geometry-dir <형상 폴더> --out-dir <서버 추출 산출물 폴더> --polygon-dir <폴리곤 폴더>
//
// 형상 폴더의 NavGeometry_<맵 번호>.json마다 out-dir에 NavMesh_<맵 번호>.navmesh를, polygon-dir에 에디터가 겹쳐 그리는
// NavMeshPolygons_<맵 번호>.json을 쓴다. 형상에 실린 스폰 마커가 걸을 수 있는 면 위에 없으면 그 맵은 실패한다.
// 맵 하나라도 실패하면 아무 파일도 바꾸지 않고 1로 끝난다.
// out-dir에서 이번에 빌드하지 않은 맵의 옛 navmesh는 지운다. 그 폴더에는 사람이 쓴 파일을 두지 않는다.

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <format>
#include <map>
#include <set>
#include <windows.h>
#include "NavMeshBuild.h"

using namespace std;
using Json = nlohmann::json;

namespace
{
    constexpr const char* GEOMETRY_PREFIX = "NavGeometry_";
    constexpr const char* POLYGON_PREFIX = "NavMeshPolygons_";

    void Log(const string& line)
    {
        cout << "[navmesh 빌드] " << line << '\n';
    }

    optional<string> ReadJson(const filesystem::path& path, Json& outDocument)
    {
        ifstream file(path);
        if (file.is_open() == false)
            return format("{}를 열지 못했다", path.string());

        try
        {
            outDocument = Json::parse(file);
        }
        catch (const Json::parse_error& e)
        {
            return format("{}: Json 문법 오류(byte {}): {}", path.string(), e.byte, e.what());
        }
        return nullopt;
    }

    bool WriteBytes(const filesystem::path& path, const vector<uint8_t>& bytes)
    {
        ofstream file(path, ios::binary | ios::trunc);
        file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<streamsize>(bytes.size()));
        return file.good();
    }

    bool WriteText(const filesystem::path& path, const string& text)
    {
        // 줄바꿈은 LF로 쓴다. 텍스트 모드로 쓰면 CRLF가 된다.
        ofstream file(path, ios::binary | ios::trunc);
        file << text;
        return file.good();
    }

    /** --이름 값 쌍을 읽는다. 값이 없는 인자가 있으면 빈 맵. */
    map<string, string> ParseArguments(int argc, char* argv[])
    {
        map<string, string> arguments;
        for (int i = 1; i + 1 < argc; i += 2)
            arguments[argv[i]] = argv[i + 1];
        if (argc % 2 == 0)
            return {};
        return arguments;
    }

    struct BuiltMap
    {
        int32_t mapId = 0;
        vector<uint8_t> navMesh;
        string polygons;
    };
}

int main(int argc, char* argv[])
{
    // 추출 스크립트가 출력을 UTF-8로 읽는다.
    ::SetConsoleOutputCP(CP_UTF8);

    const map<string, string> arguments = ParseArguments(argc, argv);
    for (const char* required : { "--agent", "--geometry-dir", "--out-dir", "--polygon-dir" })
    {
        if (arguments.contains(required) == false)
        {
            Log(format("인자 {}가 없다. 사용법: NavBuilder.exe --agent <NavAgent.json> --geometry-dir <폴더> --out-dir <폴더> --polygon-dir <폴더>", required));
            return 1;
        }
    }
    const filesystem::path geometryDir = arguments.at("--geometry-dir");
    const filesystem::path outDir = arguments.at("--out-dir");
    const filesystem::path polygonDir = arguments.at("--polygon-dir");

    Json agentDocument;
    NavMeshFile::AgentSettings agent;
    optional<string> agentError = ReadJson(arguments.at("--agent"), agentDocument);
    if (agentError.has_value() == false)
        agentError = NavMeshFile::ParseAgent(agentDocument, agent);
    if (agentError.has_value())
    {
        Log(agentError.value());
        return 1;
    }

    // 모든 맵을 메모리에서 빌드한 뒤에 쓴다. 한 맵이라도 실패하면 아무것도 바꾸지 않는다.
    vector<BuiltMap> builtMaps;
    vector<string> errors;
    error_code directoryError;
    vector<filesystem::path> geometryFiles;
    for (const filesystem::directory_entry& entry : filesystem::directory_iterator(geometryDir, directoryError))
    {
        if (entry.path().filename().string().starts_with(GEOMETRY_PREFIX))
            geometryFiles.push_back(entry.path());
    }
    if (directoryError)
    {
        Log(format("형상 폴더 {}를 읽지 못했다: {}", geometryDir.string(), directoryError.message()));
        return 1;
    }
    sort(geometryFiles.begin(), geometryFiles.end());
    if (geometryFiles.empty())
    {
        Log(format("형상 폴더 {}에 {}*.json이 없다. 추출 커맨드렛을 먼저 돌린다", geometryDir.string(), GEOMETRY_PREFIX));
        return 1;
    }

    for (const filesystem::path& path : geometryFiles)
    {
        Json document;
        NavMeshBuild::Geometry geometry;
        optional<string> error = ReadJson(path, document);
        if (error.has_value() == false)
            error = NavMeshBuild::ParseGeometry(document, geometry);

        BuiltMap built;
        if (error.has_value() == false)
        {
            built.mapId = geometry.mapId;
            error = NavMeshBuild::Build(geometry, agent, built.navMesh);
        }

        vector<string> offMesh;
        if (error.has_value() == false)
            error = NavMeshBuild::FindSpawnPointsOffMesh(built.navMesh, agent, geometry.spawnPoints, offMesh);
        if (error.has_value() == false && offMesh.empty() == false)
        {
            for (const string& name : offMesh)
                errors.push_back(format("{}: 스폰 마커 {}가 걸을 수 있는 바닥 위에 없다. 벽과 장애물에서 떨어진 바닥 위로 옮긴다", path.filename().string(), name));
            continue;
        }

        Json polygons;
        if (error.has_value() == false)
            error = NavMeshBuild::DumpPolygons(built.navMesh, built.mapId, agent, polygons);

        if (error.has_value())
        {
            errors.push_back(format("{}: {}", path.filename().string(), error.value()));
            continue;
        }

        // 에디터 메뉴가 지금 연 레벨과 대조해 겹쳐 그릴 파일을 고른다.
        polygons["Level"] = document.value("Level", "");
        built.polygons = polygons.dump() + "\n";
        Log(format("맵 {}: 삼각형 {}개 → 폴리곤 {}개, {}바이트", built.mapId, geometry.triangles.size() / 3, polygons["Polygons"].size(), built.navMesh.size()));
        builtMaps.push_back(std::move(built));
    }

    if (errors.empty() == false)
    {
        for (const string& error : errors)
            Log(error);
        Log(format("오류 {}건. 산출물을 바꾸지 않았다", errors.size()));
        return 1;
    }

    filesystem::create_directories(polygonDir);
    set<string> written;
    for (const BuiltMap& built : builtMaps)
    {
        const string navMeshName = format("{}{}{}", NavMeshFile::FILE_PREFIX, built.mapId, NavMeshFile::FILE_EXTENSION);
        const filesystem::path polygonPath = polygonDir / format("{}{}.json", POLYGON_PREFIX, built.mapId);
        if (WriteBytes(outDir / navMeshName, built.navMesh) == false || WriteText(polygonPath, built.polygons) == false)
        {
            Log(format("맵 {}의 산출물을 쓰지 못했다", built.mapId));
            return 1;
        }
        written.insert(navMeshName);
        Log((outDir / navMeshName).string());
    }

    for (const filesystem::directory_entry& entry : filesystem::directory_iterator(outDir))
    {
        const string fileName = entry.path().filename().string();
        if (fileName.starts_with(NavMeshFile::FILE_PREFIX) && written.contains(fileName) == false)
        {
            filesystem::remove(entry.path());
            Log(format("형상이 없는 맵의 옛 navmesh {}를 지웠다", fileName));
        }
    }

    Log(format("맵 {}개의 navmesh를 썼다", builtMaps.size()));
    return 0;
}
