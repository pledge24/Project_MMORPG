#pragma once
#include "Game/Navigation/NavMesh.h"

/** 추출 산출물 폴더에서 읽은 내비게이션 파일. */
struct NavMeshDocuments
{
    /** 에이전트 설정(NavAgent.json). */
    Json agent;
    /** 파일 이름 → navmesh 파일 내용. 오류 메시지가 파일 이름을 쓴다. */
    map<string, vector<uint8>> files;
};

/**
 * 맵마다의 navmesh 보관소.
 *
 * 서버가 기동할 때 맵 표의 맵마다 navmesh를 한 번 읽어 설치하고, 룸이 맵 번호로 찾아 함께 읽는다.
 * 설치한 뒤로는 바뀌지 않으므로 여러 스레드가 락 없이 함께 읽는다.
 */
class NavMeshes
{
    //~ 설치
public:
    /** 추출 산출물 폴더의 navmesh를 읽어 검증하고 설치한다. Gamedata를 설치한 뒤에 부른다. 실패하면 사유를 로그에 남기고 false. */
    static bool LoadAll();

    /**
     * 문서를 맵 표(Gamedata::GetMaps)와 대조해 읽고 통과하면 설치한다. 맵마다 navmesh가 하나여야 하고,
     * 형식 버전이나 에이전트 설정 해시가 어긋나면 사유를 돌려주고 이전 navmesh를 그대로 둔다.
     */
    static optional<string> Load(const NavMeshDocuments& documents);

    /** navmesh 전체를 바꿔 끼운다. 부팅과 테스트에서만 부르고, 룸이 읽는 중에 부르면 안 된다. */
    static void Install(map<int32, shared_ptr<const NavMesh>> navMeshes);

    //~ 조회
public:
    /** 그 맵의 navmesh. 없으면 nullptr. */
    static shared_ptr<const NavMesh> Find(int32 mapId);

    //~ 공통
private:
    static map<int32, shared_ptr<const NavMesh>> s_navMeshes;
};
