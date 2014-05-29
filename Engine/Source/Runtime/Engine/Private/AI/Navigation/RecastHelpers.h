// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Coord system utilities
 *
 * Translates between Unreal and Recast coords.
 * Unreal: x, y, z
 * Recast: -x, z, -y
 */

#pragma once

ENGINE_API FVector Unreal2RecastPoint(const float* UnrealPoint);
ENGINE_API FVector Unreal2RecastPoint(const FVector& UnrealPoint);
ENGINE_API FBox Unreal2RecastBox(const FBox& UnrealBox);
ENGINE_API FMatrix Unreal2RecastMatrix();

ENGINE_API FVector Recast2UnrealPoint(const float* RecastPoint);
ENGINE_API FVector Recast2UnrealPoint(const FVector& RecastPoint);
ENGINE_API FBox Recast2UnrealBox(const float* RecastMin, const float* RecastMax);
ENGINE_API FBox Recast2UnrealBox(const FBox& RecastBox);
