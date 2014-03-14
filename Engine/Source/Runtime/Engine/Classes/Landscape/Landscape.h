// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "LandscapeInfo.h"
#include "Landscape.generated.h"

UENUM()
enum ELandscapeSetupErrors
{
	LSE_None,
	// No Landscape Info available
	LSE_NoLandscapeInfo,
	// There was already component with same X,Y
	LSE_CollsionXY,
	// No Layer Info, need to add proper layers
	LSE_NoLayerInfo,
	LSE_MAX,
};

UCLASS(dependson=ULightComponent, HeaderGroup=Terrain, Placeable, hidecategories=LandscapeProxy, showcategories=(Display, Movement, Collision, Lighting, LOD, Input), MinimalAPI)
class ALandscape : public ALandscapeProxy, public INavRelevantActorInterface
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	class ULandscapeSplinesComponent* SplineComponent;

	// Make a key for XYtoComponentMap
	static FIntPoint MakeKey( int32 X, int32 Y ) { return FIntPoint(X, Y); }
	static void UnpackKey( FIntPoint Key, int32& OutX, int32& OutY ) { OutX = Key.X; OutY = Key.Y; }

	// Begin AActor Interface
#if WITH_EDITOR
	virtual void CheckForErrors() OVERRIDE;
	virtual void Destroyed() OVERRIDE;
#endif
	// End AActor Interface

	// Begin INavRelevantActorInterface Interface
	virtual bool DoesSupplyPerComponentNavigationCollision() const OVERRIDE { return true; }
	// End INavRelevantActorInterface Interface

	// Begin ALandscapeProxy Interface
	virtual ALandscape* GetLandscapeActor() OVERRIDE;
#if WITH_EDITOR
	virtual UMaterialInterface* GetLandscapeMaterial() const OVERRIDE;
	virtual UMaterialInterface* GetLandscapeHoleMaterial() const OVERRIDE;
	// End ALandscapeProxy Interface

	ENGINE_API bool HasAllComponent(); // determine all component is in this actor


	// Include Components with overlapped vertices
	ENGINE_API static void CalcComponentIndicesOverlap(const int32 X1, const int32 Y1, const int32 X2, const int32 Y2, const int32 ComponentSizeQuads, 
		int32& ComponentIndexX1, int32& ComponentIndexY1, int32& ComponentIndexX2, int32& ComponentIndexY2);

	// Exclude Components with overlapped vertices
	static void CalcComponentIndices(const int32 X1, const int32 Y1, const int32 X2, const int32 Y2, const int32 ComponentSizeQuads, 
		int32& ComponentIndexX1, int32& ComponentIndexY1, int32& ComponentIndexX2, int32& ComponentIndexY2);

	static void SplitHeightmap(ULandscapeComponent* Comp, bool bMoveToCurrentLevel = false);
	

	// Begin UObject interface.
	virtual void PreSave() OVERRIDE;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	virtual void PostEditMove(bool bFinished) OVERRIDE;
#endif
	virtual void PostLoad() OVERRIDE;
	// End UObject Interface



};


