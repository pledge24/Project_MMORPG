// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "P1/P1GameModeBase.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeP1GameModeBase() {}
// Cross Module References
	ENGINE_API UClass* Z_Construct_UClass_AGameModeBase();
	P1_API UClass* Z_Construct_UClass_AP1GameModeBase();
	P1_API UClass* Z_Construct_UClass_AP1GameModeBase_NoRegister();
	UPackage* Z_Construct_UPackage__Script_P1();
// End Cross Module References
	void AP1GameModeBase::StaticRegisterNativesAP1GameModeBase()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AP1GameModeBase);
	UClass* Z_Construct_UClass_AP1GameModeBase_NoRegister()
	{
		return AP1GameModeBase::StaticClass();
	}
	struct Z_Construct_UClass_AP1GameModeBase_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AP1GameModeBase_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AGameModeBase,
		(UObject* (*)())Z_Construct_UPackage__Script_P1,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AP1GameModeBase_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "HideCategories", "Info Rendering MovementReplication Replication Actor Input Movement Collision Rendering HLOD WorldPartition DataLayers Transformation" },
		{ "IncludePath", "P1GameModeBase.h" },
		{ "ModuleRelativePath", "P1GameModeBase.h" },
		{ "ShowCategories", "Input|MouseInput Input|TouchInput" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_AP1GameModeBase_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AP1GameModeBase>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AP1GameModeBase_Statics::ClassParams = {
		&AP1GameModeBase::StaticClass,
		"Game",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		nullptr,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		0,
		0,
		0x009002ACu,
		METADATA_PARAMS(Z_Construct_UClass_AP1GameModeBase_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AP1GameModeBase_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AP1GameModeBase()
	{
		if (!Z_Registration_Info_UClass_AP1GameModeBase.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AP1GameModeBase.OuterSingleton, Z_Construct_UClass_AP1GameModeBase_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AP1GameModeBase.OuterSingleton;
	}
	template<> P1_API UClass* StaticClass<AP1GameModeBase>()
	{
		return AP1GameModeBase::StaticClass();
	}
	AP1GameModeBase::AP1GameModeBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AP1GameModeBase);
	AP1GameModeBase::~AP1GameModeBase() {}
	struct Z_CompiledInDeferFile_FID_Users_zmwps_Desktop_Project_MMORPG_P1_Source_P1_P1GameModeBase_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_zmwps_Desktop_Project_MMORPG_P1_Source_P1_P1GameModeBase_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AP1GameModeBase, AP1GameModeBase::StaticClass, TEXT("AP1GameModeBase"), &Z_Registration_Info_UClass_AP1GameModeBase, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AP1GameModeBase), 2406498958U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_zmwps_Desktop_Project_MMORPG_P1_Source_P1_P1GameModeBase_h_2774012030(TEXT("/Script/P1"),
		Z_CompiledInDeferFile_FID_Users_zmwps_Desktop_Project_MMORPG_P1_Source_P1_P1GameModeBase_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_zmwps_Desktop_Project_MMORPG_P1_Source_P1_P1GameModeBase_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
