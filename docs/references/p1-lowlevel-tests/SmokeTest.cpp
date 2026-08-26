#include "CoreTypes.h"

#include "Containers/UnrealString.h"
#include "TestHarness.h"

/*
 * L1 파이프라인 증명용 스모크. 이 테스트가 하는 일은 "작성 → 빌드 → 실행 →
 * 결과 확인" 루프가 UE 클라 쪽에서도 돈다는 것을 보이는 것뿐이다.
 * 실제 게임 로직 테스트는 이 파이프라인이 선 뒤에 붙인다.
 */
TEST_CASE("P1 L1 파이프라인이 동작한다", "[P1][Smoke]")
{
	const FString Name = TEXT("P1");

	CHECK(Name.Len() == 2);
	CHECK(Name == TEXT("P1"));
}
