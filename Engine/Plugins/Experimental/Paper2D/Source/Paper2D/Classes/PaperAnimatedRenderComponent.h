// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperFlipbook.h"
#include "PaperAnimatedRenderComponent.generated.h"

UCLASS(MinimalAPI, ShowCategories=(Mobility), meta=(BlueprintSpawnableComponent))
class UPaperAnimatedRenderComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

protected:
	UPROPERTY(Category=Sprite, EditAnywhere, meta=(DisplayThumbnail = "true"))
	UPaperFlipbook* SourceFlipbook;

	UPROPERTY(Category=Sprite, EditAnywhere)
	UMaterialInterface* Material;

	UPROPERTY(Category=Sprite, EditAnywhere, BlueprintReadWrite)
	float PlayRate;
public:
	/** Change the flipbook used by this instance. */
	UFUNCTION(BlueprintCallable, Category="Sprite")
	virtual bool SetFlipbook(class UPaperFlipbook* NewFlipbook);

	/** Gets the flipbook used by this instance. */
	UFUNCTION(BlueprintPure, Category="Sprite")
	virtual UPaperFlipbook* GetFlipbook();

	/** Gets the material used by this instance */
	UFUNCTION(BlueprintCallable, Category="Sprite")
	UMaterialInterface* GetSpriteMaterial() const;

	/** Set color of the sprite */
	UFUNCTION(BlueprintCallable, Category="Sprite")
	void SetSpriteColor(FLinearColor NewColor);

	//@TODO: Hackery for demo
	UFUNCTION(BlueprintCallable, Category="Sprite")
	void SetCurrentTime(float NewTime);

protected:
	float AccumulatedTime;
	int32 CachedFrameIndex;

	UPROPERTY(BlueprintReadOnly, Interp, Category=Sprite)
	FLinearColor SpriteColor;

protected:
	void CalculateCurrentFrame();
public:
	// UActorComponent interface
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) OVERRIDE;
	virtual void SendRenderDynamicData_Concurrent() OVERRIDE;
	// End of UActorComponent interface

	// UPrimitiveComponent interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() OVERRIDE;
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const OVERRIDE;
	// End of UPrimitiveComponent interface
};
