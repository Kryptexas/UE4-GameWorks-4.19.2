// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
// Includes all necessary PhysX headers

#pragma once

#include "CoreMinimal.h"
#include "EngineDefines.h"

#if WITH_PHYSX

#ifdef PHYSX_API
#undef PHYSX_API	//Our build system treats PhysX as a module and automatically defines this. PhysX has its own way of defining this.
#endif

MSVC_PRAGMA(warning(push))
MSVC_PRAGMA(warning(disable : 4946)) // reinterpret_cast used between related classes

PRAGMA_DISABLE_SHADOW_VARIABLE_WARNINGS

#if USING_CODE_ANALYSIS
	MSVC_PRAGMA(warning(push))
	MSVC_PRAGMA(warning(disable : ALL_CODE_ANALYSIS_WARNINGS))
#endif	// USING_CODE_ANALYSIS

#pragma pack(push,8)

THIRD_PARTY_INCLUDES_START
#include "Px.h"
#include "PxPhysicsAPI.h"
#include "PxRenderBuffer.h"
#include "PxExtensionsAPI.h"
#include "PxCollectionExt.h"
#include "PxPvd.h"

// utils
#include "PxGeometryQuery.h"
#include "PxMeshQuery.h"
#include "PxTriangle.h"
THIRD_PARTY_INCLUDES_END

// APEX
#if WITH_APEX

#ifdef APEX_API
#undef APEX_API	//Our build system treats PhysX as a module and automatically defines this. PhysX has its own way of defining this.
#endif

// Framework
THIRD_PARTY_INCLUDES_START
#include "Apex.h"
THIRD_PARTY_INCLUDES_END

// Modules

#include "ModuleDestructible.h"
#include "DestructibleAsset.h"
#include "destructible/DestructibleActor.h"

THIRD_PARTY_INCLUDES_START

#if WITH_APEX_CLOTHING
#include "ModuleClothing.h"
#include "ClothingAsset.h"
#include "ClothingActor.h"
#include "ClothingCollision.h"
#endif

#if WITH_APEX_LEGACY
#include "ModuleLegacy.h"
#endif

// Utilities
#include "NvParamUtils.h"

THIRD_PARTY_INCLUDES_END

#endif // #if WITH_APEX

#pragma pack(pop)

#if USING_CODE_ANALYSIS
	MSVC_PRAGMA(warning(pop))
#endif	// USING_CODE_ANALYSIS

MSVC_PRAGMA(warning(pop))

PRAGMA_ENABLE_SHADOW_VARIABLE_WARNINGS

using namespace physx;

#endif // WITH_PHYSX
