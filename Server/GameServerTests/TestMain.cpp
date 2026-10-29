#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Core/ServerContext.h"

/*--------------------------------------------------------------
    gtest_main.cc 대신 직접 main을 둔다.

    테스트를 돌리기 전에 운영 코드와 같은 전역 객체를 만들어야 한다.
    콘솔 출력을 UTF-8로 맞추는 일도 그 전역 객체가 맡으므로, 한국어
    실패 메시지가 깨지지 않으려면 이 객체가 gtest보다 먼저 만들어져야 한다.
---------------------------------------------------------------*/

int main(int argc, char** argv)
{
    // 운영 코드와 같은 전역 객체를 만든다. 테스트는 이 객체가 만든 로거와 룸 관리자를 쓴다.
    // 아이템을 새 칸에 넣으면 UID를 발급하므로 발급기도 만든다. 노드 번호는 Debug 서버의 기본값과 같은 0이다.
    ServerContext context;
    context.CreateIdGenerator(0);

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
