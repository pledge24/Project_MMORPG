// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "P1/P1GameInstance.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeP1GameInstance() {}
// Cross Module References
	ENGINE_API UClass* Z_Construct_UClass_UGameInstance();
	P1_API UClass* Z_Construct_UClass_UP1GameInstance();
	P1_API UClass* Z_Construct_UClass_UP1GameInstance_NoRegister();
	UPackage* Z_Construct_UPackage__Script_P1();
// End Cross Module References
	DEFINE_FUNCTION(UP1GameInstance::execHandleRecvPackets)
	{
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->HandleRecvPackets();
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UP1GameInstance::execDisconnectFromGameServer)
	{
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->DisconnectFromGameServer();
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UP1GameInstance::execConnectToGameServer)
	{
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->ConnectToGameServer();
		P_NATIVE_END;
	}
	void UP1GameInstance::StaticRegisterNativesUP1GameInstance()
	{
		UClass* Class = UP1GameInstance::StaticClass();
		static const FNameNativePtrPair Funcs[] = {
			{ "ConnectToGameServer", &UP1GameInstance::execConnectToGameServer },
			{ "DisconnectFromGameServer", &UP1GameInstance::execDisconnectFromGameServer },
			{ "HandleRecvPackets", &UP1GameInstance::execHandleRecvPackets },
		};
		FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
	}
	struct Z_Construct_UFunction_UP1GameInstance_ConnectToGameServer_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UP1GameInstance_ConnectToGameServer_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "P1GameInstance.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UP1GameInstance_ConnectToGameServer_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UP1GameInstance, nullptr, "ConnectToGameServer", nullptr, nullptr, 0, nullptr, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(Z_Construct_UFunction_UP1GameInstance_ConnectToGameServer_Statics::Function_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UFunction_UP1GameInstance_ConnectToGameServer_Statics::Function_MetaDataParams)) };
	UFunction* Z_Construct_UFunction_UP1GameInstance_ConnectToGameServer()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UP1GameInstance_ConnectToGameServer_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UP1GameInstance_DisconnectFromGameServer_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UP1GameInstance_DisconnectFromGameServer_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "P1GameInstance.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UP1GameInstance_DisconnectFromGameServer_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UP1GameInstance, nullptr, "DisconnectFromGameServer", nullptr, nullptr, 0, nullptr, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(Z_Construct_UFunction_UP1GameInstance_DisconnectFromGameServer_Statics::Function_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UFunction_UP1GameInstance_DisconnectFromGameServer_Statics::Function_MetaDataParams)) };
	UFunction* Z_Construct_UFunction_UP1GameInstance_DisconnectFromGameServer()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UP1GameInstance_DisconnectFromGameServer_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UP1GameInstance_HandleRecvPackets_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UP1GameInstance_HandleRecvPackets_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "P1GameInstance.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UP1GameInstance_HandleRecvPackets_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UP1GameInstance, nullptr, "HandleRecvPackets", nullptr, nullptr, 0, nullptr, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(Z_Construct_UFunction_UP1GameInstance_HandleRecvPackets_Statics::Function_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UFunction_UP1GameInstance_HandleRecvPackets_Statics::Function_MetaDataParams)) };
	UFunction* Z_Construct_UFunction_UP1GameInstance_HandleRecvPackets()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UP1GameInstance_HandleRecvPackets_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UP1GameInstance);
	UClass* Z_Construct_UClass_UP1GameInstance_NoRegister()
	{
		return UP1GameInstance::StaticClass();
	}
	struct Z_Construct_UClass_UP1GameInstance_Statics
	{
		static UObject* (*const DependentSingletons[])();
		static const FClassFunctionLinkInfo FuncInfo[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UP1GameInstance_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UGameInstance,
		(UObject* (*)())Z_Construct_UPackage__Script_P1,
	};
	const FClassFunctionLinkInfo Z_Construct_UClass_UP1GameInstance_Statics::FuncInfo[] = {
		{ &Z_Construct_UFunction_UP1GameInstance_ConnectToGameServer, "ConnectToGameServer" }, // 1194523884
		{ &Z_Construct_UFunction_UP1GameInstance_DisconnectFromGameServer, "DisconnectFromGameServer" }, // 1617169496
		{ &Z_Construct_UFunction_UP1GameInstance_HandleRecvPackets, "HandleRecvPackets" }, // 10072833
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UP1GameInstance_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "IncludePath", "P1GameInstance.h" },
		{ "ModuleRelativePath", "P1GameInstance.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_UP1GameInstance_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UP1GameInstance>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UP1GameInstance_Statics::ClassParams = {
		&UP1GameInstance::StaticClass,
		nullptr,
		&StaticCppClassTypeInfo,
		DependentSingletons,
		FuncInfo,
		nullptr,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		UE_ARRAY_COUNT(FuncInfo),
		0,
		0,
		0x009000A8u,
		METADATA_PARAMS(Z_Construct_UClass_UP1GameInstance_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_UP1GameInstance_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_UP1GameInstance()
	{
		if (!Z_Registration_Info_UClass_UP1GameInstance.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UP1GameInstance.OuterSingleton, Z_Construct_UClass_UP1GameInstance_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UP1GameInstance.OuterSingleton;
	}
	template<> P1_API UClass* StaticClass<UP1GameInstance>()
	{
		return UP1GameInstance::StaticClass();
	}
	UP1GameInstance::UP1GameInstance(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UP1GameInstance);
	UP1GameInstance::~UP1GameInstance() {}
	struct Z_CompiledInDeferFile_FID_Users_zmwps_Desktop_Project_MMORPG_P1_Source_P1_P1GameInstance_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_zmwps_Desktop_Project_MMORPG_P1_Source_P1_P1GameInstance_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UP1GameInstance, UP1GameInstance::StaticClass, TEXT("UP1GameInstance"), &Z_Registration_Info_UClass_UP1GameInstance, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UP1GameInstance), 2884794559U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_zmwps_Desktop_Project_MMORPG_P1_Source_P1_P1GameInstance_h_964038264(TEXT("/Script/P1"),
		Z_CompiledInDeferFile_FID_Users_zmwps_Desktop_Project_MMORPG_P1_Source_P1_P1GameInstance_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_zmwps_Desktop_Project_MMORPG_P1_Source_P1_P1GameInstance_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
