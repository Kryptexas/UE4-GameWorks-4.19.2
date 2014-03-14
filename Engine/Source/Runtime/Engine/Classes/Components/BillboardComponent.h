// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "BillboardComponent.generated.h"

/** 
 * A 2d texture that will be rendered always facing the camera.
 */
UCLASS(HeaderGroup=Component, ClassGroup=Rendering, collapsecategories, hidecategories=(Object,Activation,"Components|Activation",Physics,Collision,Lighting,LOD,Mesh), editinlinenew, meta=(BlueprintSpawnableComponent),MinimalAPI)
class UBillboardComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Sprite)
	class UTexture2D* Sprite;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Sprite)
	uint32 bIsScreenSizeScaled:1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Sprite)
	float ScreenSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Sprite)
	float U;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Sprite)
	float UL;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Sprite)
	float V;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Sprite)
	float VL;

#if WITH_EDITORONLY_DATA
	/** Sprite category that the component belongs to. Value serves as a key into the localization file. */
	UPROPERTY()
	FName SpriteCategoryName_DEPRECATED;

	/** Sprite category information regarding the component */
	UPROPERTY()
	struct FSpriteCategoryInfo SpriteInfo;

#endif // WITH_EDITORONLY_DATA
	/** Change the sprite texture used by this component */
	UFUNCTION(BlueprintCallable, Category="Rendering|Components|Sprite")
	virtual void SetSprite(class UTexture2D* NewSprite);

	/** Change the sprite's UVs */
	UFUNCTION(BlueprintCallable, Category="Rendering|Components|Sprite")
	virtual void SetUV(int32 NewU, int32 NewUL, int32 NewV, int32 NewVL);

	/** Change the sprite texture and the UV's used by this component */
	UFUNCTION(BlueprintCallable, Category="Rendering|Components|Sprite")
	virtual void SetSpriteAndUV(class UTexture2D* NewSprite, int32 NewU, int32 NewUL, int32 NewV, int32 NewVL);


	// Begin UPrimitiveComponent Interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() OVERRIDE;
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const OVERRIDE;
	// End UPrimitiveComponent Interface
};



