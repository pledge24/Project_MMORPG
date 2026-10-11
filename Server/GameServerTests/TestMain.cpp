#include "Core/pch.h"
#include <gtest/gtest.h>
#include "Core/ServerContext.h"

/*--------------------------------------------------------------
    gtest_main.cc 대신 직접 main을 둔다.

    이유는 콘솔 코드페이지 하나다. gtest는 진단 메시지를 UTF-8 바이트 그대로
    stdout에 쓰는데 이 환경의 기본 콘솔 코드페이지는 cp949라, 실패 메시지의
    한국어가 깨져서 읽히지 않는다("湲고? ?щ’ ??젣媛..."). 이 프로젝트는
    주석과 로그를 한국어로 쓰기로 했으므로, 읽히지 않는 실패 진단은
    테스트 하네스로서 제 역할을 못 한다.
---------------------------------------------------------------*/

int main(int argc, char** argv)
{
    SetConsoleOutputCP(CP_UTF8);

    // 운영 코드와 같은 전역 객체를 만든다. 테스트는 이 객체가 만든 로거와 룸 관리자를 쓴다.
    ServerContext context;

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
