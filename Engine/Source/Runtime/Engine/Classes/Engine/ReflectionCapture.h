// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "ReflectionCapture.generated.h"

UCLASS(abstract, hidecategories=(Collision, Attachment, Actor), HeaderGroup=Decal, MinimalAPI)
class AReflectionCapture : public AActor
{
	GENERATED_UCLASS_BODY()

	/** Reflection capture component. */
	UPROPERTY(Category=DecalActor, VisibleAnywhere, BlueprintReadOnly)
	TSubobjectPtr<class UReflectionCaptureComponent> CaptureComponent;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TSubobjectPtr<UBillboardComponent> SpriteComponent;
#endif // WITH_EDITORONLY_DATA
	
	virtual bool IsLevelBoundsRelevant() const OVERRIDE { return false; }
public:
#if WITH_EDITOR
	virtual void PostEditMove(bool bFinished) OVERRIDE;
#endif // WITH_EDITOR

};



