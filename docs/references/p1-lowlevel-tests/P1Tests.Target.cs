using UnrealBuildTool;

/*
 * P1 클라이언트의 L1(Low-Level Tests) 타깃.
 *
 * LLT는 에디터가 아니라 **전용 타깃의 별도 exe**다. Rider의 Unit Tests 창은
 * L2(Simple Automation Test)용이라 이걸 인식하지 못한다 — 그쪽으로 시도하지 말 것.
 *
 * 첫 파이프라인 증명이므로 의존성을 Core로만 묶는다. CoreUObject·Engine을 끌어들이면
 * Installed Build(런처 설치본)에서 필요한 프리빌트 산출물 범위가 커진다.
 */
[SupportedPlatforms(UnrealPlatformClass.Desktop)]
public class P1TestsTarget : TestTargetRules
{
	public P1TestsTarget(TargetInfo Target) : base(Target)
	{
		bNeverCompileAgainstEngine = true;
		bNeverCompileAgainstEditor = true;
	}
}
