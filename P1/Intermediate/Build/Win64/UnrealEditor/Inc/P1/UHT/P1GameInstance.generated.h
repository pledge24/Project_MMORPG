// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "P1GameInstance.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef P1_P1GameInstance_generated_h
#error "P1GameInstance.generated.h already included, missing '#pragma once' in P1GameInstance.h"
#endif
#define P1_P1GameInstance_generated_h

#define FID_Users_zmwps_Desktop_Project_MMORPG_P1_Source_P1_P1GameInstance_h_16_SPARSE_DATA
#define FID_Users_zmwps_Desktop_Project_MMORPG_P1_Source_P1_P1GameInstance_h_16_RPC_WRAPPERS \
 \
	DECLARE_FUNCTION(execHandleRecvPackets); \
	DECLARE_FUNCTION(execDisconnectFromGameServer); \
	DECLARE_FUNCTION(execConnectToGameServer);


#define FID_Users_zmwps_Desktop_Project_MMORPG_P1_Source_P1_P1GameInstance_h_16_RPC_WRAPPERS_NO_PURE_DECLS \
 \
	DECLARE_FUNCTION(execHandleRecvPackets); \
	DECLARE_FUNCTION(execDisconnectFromGameServer); \
	DECLARE_FUNCTION(execConnectToGameServer);


#define FID_Users_zmwps_Desktop_Project_MMORPG_P1_Source_P1_P1GameInstance_h_16_ACCESSORS
#define FID_Users_zmwps_Desktop_Project_MMORPG_P1_Source_P1_P1GameInstance_h_16_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUP1GameInstance(); \
	friend struct Z_Construct_UClass_UP1GameInstance_Statics; \
public: \
	DECLARE_CLASS(UP1GameInstance, UGameInstance, COMPILED_IN_FLAGS(0 | CLASS_Transient), CASTCLASS_None, TEXT("/Script/P1"), NO_API) \
	DECLARE_SERIALIZER(UP1GameInstance)


#define FID_Users_zmwps_Desktop_Project_MMORPG_P1_Source_P1_P1GameInstance_h_16_INCLASS \
private: \
	static void StaticRegisterNativesUP1GameInstance(); \
	friend struct Z_Construct_UClass_UP1GameInstance_Statics; \
public: \
	DECLARE_CLASS(UP1GameInstance, UGameInstance, COMPILED_IN_FLAGS(0 | CLASS_Transient), CASTCLASS_None, TEXT("/Script/P1"), NO_API) \
	DECLARE_SERIALIZER(UP1GameInstance)


#define FID_Users_zmwps_Desktop_Project_MMORPG_P1_Source_P1_P1GameInstance_h_16_STANDARD_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UP1GameInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UP1GameInstance) \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UP1GameInstance); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UP1GameInstance); \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	NO_API UP1GameInstance(UP1GameInstance&&); \
	NO_API UP1GameInstance(const UP1GameInstance&); \
public: \
	NO_API virtual ~UP1GameInstance();


#define FID_Users_zmwps_Desktop_Project_MMORPG_P1_Source_P1_P1GameInstance_h_16_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UP1GameInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	NO_API UP1GameInstance(UP1GameInstance&&); \
	NO_API UP1GameInstance(const UP1GameInstance&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UP1GameInstance); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UP1GameInstance); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UP1GameInstance) \
	NO_API virtual ~UP1GameInstance();


#define FID_Users_zmwps_Desktop_Project_MMORPG_P1_Source_P1_P1GameInstance_h_13_PROLOG
#define FID_Users_zmwps_Desktop_Project_MMORPG_P1_Source_P1_P1GameInstance_h_16_GENERATED_BODY_LEGACY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Users_zmwps_Desktop_Project_MMORPG_P1_Source_P1_P1GameInstance_h_16_SPARSE_DATA \
	FID_Users_zmwps_Desktop_Project_MMORPG_P1_Source_P1_P1GameInstance_h_16_RPC_WRAPPERS \
	FID_Users_zmwps_Desktop_Project_MMORPG_P1_Source_P1_P1GameInstance_h_16_ACCESSORS \
	FID_Users_zmwps_Desktop_Project_MMORPG_P1_Source_P1_P1GameInstance_h_16_INCLASS \
	FID_Users_zmwps_Desktop_Project_MMORPG_P1_Source_P1_P1GameInstance_h_16_STANDARD_CONSTRUCTORS \
public: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


#define FID_Users_zmwps_Desktop_Project_MMORPG_P1_Source_P1_P1GameInstance_h_16_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Users_zmwps_Desktop_Project_MMORPG_P1_Source_P1_P1GameInstance_h_16_SPARSE_DATA \
	FID_Users_zmwps_Desktop_Project_MMORPG_P1_Source_P1_P1GameInstance_h_16_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_Users_zmwps_Desktop_Project_MMORPG_P1_Source_P1_P1GameInstance_h_16_ACCESSORS \
	FID_Users_zmwps_Desktop_Project_MMORPG_P1_Source_P1_P1GameInstance_h_16_INCLASS_NO_PURE_DECLS \
	FID_Users_zmwps_Desktop_Project_MMORPG_P1_Source_P1_P1GameInstance_h_16_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> P1_API UClass* StaticClass<class UP1GameInstance>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Users_zmwps_Desktop_Project_MMORPG_P1_Source_P1_P1GameInstance_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
