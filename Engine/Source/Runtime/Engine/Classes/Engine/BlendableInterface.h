// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BlendableInterface.generated.h"

/** Where to place a material node in the post processing graph */
UENUM()
enum EBlendableLocation
{
	BL_AfterTonemapping UMETA(DisplayName="After Tonemapping"),
	BL_BeforeTonemapping UMETA(DisplayName="Before Tonemapping"),
	BL_BeforeTranslucency UMETA(DisplayName="Before Translucency"),
//	BL_AfterOpaque,
//	BL_AfterFog,
//	BL_AfterTranslucency,
//	BL_AfterPostProcessAA,
	BL_MAX,
};

/** Dummy class needed to support InterfaceCast<IBlendableInterface>(Object) */
UINTERFACE()
class UBlendableInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

/**
 * derive from this class if you want to be blended by the PostProcess blending e.g. PostproceessVolume
 */
class IBlendableInterface
{
	GENERATED_IINTERFACE_BODY()
		
	// @param Weight 0..1, 1=fully take the values from this object, crash if outside the valid range
	virtual void OverrideBlendableSettings(class FSceneView& View, float Weight) const = 0;
};

struct FPostProcessMaterialNode
{
	FPostProcessMaterialNode(UMaterialInstanceDynamic* InMID, EBlendableLocation InLocation, int32 InPriority)
		:MID(InMID), Location(InLocation), Priority(InPriority)
	{
	}

	UMaterialInstanceDynamic* MID;
	EBlendableLocation Location;
	int32 Priority;

	// for type safety in FBlendableManager
	static const FName& GetFName()
	{
		static const FName Name = FName(TEXT("FPostProcessMaterialNode"));

		return Name;
	}

	struct FCompare
	{
		FORCEINLINE bool operator()(const FPostProcessMaterialNode& P1, const FPostProcessMaterialNode& P2) const
		{
			if(P1.Location < P2.Location) return true;
			if(P1.Location > P2.Location) return false;

			if(P1.Priority < P2.Priority) return true;
			if(P1.Priority > P2.Priority) return false;

			return false;
		}
	};
};


