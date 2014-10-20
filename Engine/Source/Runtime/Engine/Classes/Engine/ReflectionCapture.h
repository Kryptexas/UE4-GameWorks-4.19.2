// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "ReflectionCapture.generated.h"

UCLASS(abstract, hidecategories=(Collision, Attachment, Actor), MinimalAPI)
class AReflectionCapture : public AActor
{
	GENERATED_UCLASS_BODY()

private:
	/** Reflection capture component. */
	UPROPERTY(Category = DecalActor, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UReflectionCaptureComponent* CaptureComponent;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	UBillboardComponent* SpriteComponent;
#endif // WITH_EDITORONLY_DATA

public:	

	virtual bool IsLevelBoundsRelevant() const override { return false; }

#if WITH_EDITOR
	virtual void PostEditMove(bool bFinished) override;
#endif // WITH_EDITOR

	/** Returns CaptureComponent subobject **/
	FORCEINLINE class UReflectionCaptureComponent* GetCaptureComponent() const { return CaptureComponent; }
#if WITH_EDITORONLY_DATA
	/** Returns SpriteComponent subobject **/
	FORCEINLINE UBillboardComponent* GetSpriteComponent() const { return SpriteComponent; }
#endif
};



