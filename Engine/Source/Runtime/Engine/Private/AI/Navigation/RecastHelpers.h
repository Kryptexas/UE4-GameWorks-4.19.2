// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Coord system utilities
 *
 * Translates between Unreal and Recast coords.
 * Unreal: x, y, z
 * Recast: -x, z, -y
 */

#pragma once

FVector Unreal2RecastPoint(const float* UnrealPoint);
FVector Unreal2RecastPoint(const FVector& UnrealPoint);
FBox Unreal2RecastBox(const FBox& UnrealBox);
FMatrix Unreal2RecastMatrix();

FVector Recast2UnrealPoint(const float* RecastPoint);
FVector Recast2UnrealPoint(const FVector& RecastPoint);
FBox Recast2UnrealBox(const float* RecastMin, const float* RecastMax);
FBox Recast2UnrealBox(const FBox& RecastBox);
