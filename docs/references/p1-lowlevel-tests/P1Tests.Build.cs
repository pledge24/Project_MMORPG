using UnrealBuildTool;

/*
 * P1 클라이언트 L1 테스트 모듈 (Catch2 기반 Low-Level Tests).
 * 엔진 예제 Engine/Source/Programs/LowLevelTests/MathCoreTests 의 구조를 따랐다.
 */
public class P1Tests : TestModuleRules
{
	static P1Tests()
	{
		if (InTestMode)
		{
			TestMetadata = new Metadata();
			TestMetadata.TestName = "P1";
			TestMetadata.TestShortName = "P1";
			TestMetadata.ReportType = "xml";
		}
	}

	public P1Tests(ReadOnlyTargetRules Target) : base(Target, InUsesCatch2: true)
	{
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			});
	}
}
