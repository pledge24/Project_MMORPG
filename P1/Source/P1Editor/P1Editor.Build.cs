using UnrealBuildTool;

public class P1Editor : ModuleRules
{
	public P1Editor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "DataValidation", "UnrealEd", "Json", "P1" });

		// 게임 모듈과 같이 도메인 폴더는 등록하지 않고 `#include`를 경로 한정으로 쓴다.
		// 게임 모듈의 헤더는 그 모듈 루트 기준 경로(`Game/Data/...`)로 부른다.
		PrivateIncludePaths.AddRange(new string[]
		{
		    "P1Editor/",
		    "P1/",
		});
	}
}
