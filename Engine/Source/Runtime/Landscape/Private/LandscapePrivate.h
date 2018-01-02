// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Stats/Stats.h"


LANDSCAPE_API DECLARE_LOG_CATEGORY_EXTERN(LogLandscape, Warning, All);

/**
 * Landscape stats
 */
DECLARE_CYCLE_STAT_EXTERN(TEXT("Landscape Dynamic Draw Time"), STAT_LandscapeDynamicDrawTime, STATGROUP_Landscape, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Landscape Static Draw LOD Time"), STAT_LandscapeStaticDrawLODTime, STATGROUP_Landscape, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("LandscapeVF Draw Time VS"), STAT_LandscapeVFDrawTimeVS, STATGROUP_Landscape, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("LandscapeVF Draw Time PS"), STAT_LandscapeVFDrawTimePS, STATGROUP_Landscape, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Landscape Init View Custom Data"), STAT_LandscapeInitViewCustomData, STATGROUP_Landscape, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Landscape Update View Custom Data"), STAT_LandscapeUpdateViewCustomData, STATGROUP_Landscape, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Landscape Compute Custom Mesh Batch LOD"), STAT_LandscapeComputeCustomMeshBatchLOD, STATGROUP_Landscape, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Landscape Compute Custom Shadow Mesh Batch LOD"), STAT_LandscapeComputeCustomShadowMeshBatchLOD, STATGROUP_Landscape, );

DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Landscape Component Using SubSection DrawCall"), STAT_LandscapeComponentUsingSubSectionDrawCalls, STATGROUP_Landscape, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Landscape Tessellated Shadow Cascade"), STAT_LandscapeTessellatedShadowCascade, STATGROUP_Landscape, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Landscape Tessellated Components"), STAT_LandscapeTessellatedComponents, STATGROUP_Landscape, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Landscape Processed Triangles"), STAT_LandscapeTriangles, STATGROUP_Landscape, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Landscape Render Passes"), STAT_LandscapeComponentRenderPasses, STATGROUP_Landscape, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Landscape DrawCalls"), STAT_LandscapeDrawCalls, STATGROUP_Landscape, );

DECLARE_MEMORY_STAT_EXTERN(TEXT("Landscape Vertex Mem"), STAT_LandscapeVertexMem, STATGROUP_Landscape, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Component Mem"), STAT_LandscapeComponentMem, STATGROUP_Landscape, );
