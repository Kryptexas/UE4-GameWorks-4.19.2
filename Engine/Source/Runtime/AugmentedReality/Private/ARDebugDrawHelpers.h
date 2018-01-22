// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

class UWorld;
struct FVector;
class FString;
struct FColor;

namespace ARDebugHelpers
{
	void DrawDebugString(const UWorld* InWorld, FVector const& TextLocation, const FString& Text, float Scale, FColor const& TextColor, float Duration, bool bDrawShadow);
}
