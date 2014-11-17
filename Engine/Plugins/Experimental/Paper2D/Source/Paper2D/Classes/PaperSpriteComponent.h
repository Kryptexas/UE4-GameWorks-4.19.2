// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperSprite.h"

#include "PaperSpriteComponent.generated.h"

UCLASS(ShowCategories=(Mobility), EarlyAccessPreview, meta=(BlueprintSpawnableComponent))
class PAPER2D_API UPaperSpriteComponent : public UMeshComponent
{
	GENERATED_UCLASS_BODY()

protected:
	// The sprite asset used by this component
	UPROPERTY(Category=Sprite, EditAnywhere, BlueprintReadOnly, meta=(DisplayThumbnail = "true"))
	UPaperSprite* SourceSprite;

	// DEPRECATED in 4.4: The material override for this sprite component (if any); replaced by the Materials array inherited from UMeshComponent
	UPROPERTY()
	UMaterialInterface* MaterialOverride_DEPRECATED;

	// The color of the sprite (passed to the sprite material as a vertex color)
	UPROPERTY(BlueprintReadOnly, Interp, Category=Sprite)
	FLinearColor SpriteColor;

public:
	/** Change the PaperSprite used by this instance. */
	UFUNCTION(BlueprintCallable, Category="Sprite")
	virtual bool SetSprite(class UPaperSprite* NewSprite);

	/** Gets the PaperSprite used by this instance. */
	UFUNCTION(BlueprintPure, Category="Sprite")
	virtual UPaperSprite* GetSprite();

	/** Set color of the sprite */
	UFUNCTION(BlueprintCallable, Category="Sprite")
	void SetSpriteColor(FLinearColor NewColor);

	// Returns the wireframe color to use for this component.
	FLinearColor GetWireframeColor() const;

public:
	// UObject interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
#if WITH_EDITORONLY_DATA
	virtual void Serialize(FArchive& Ar) override;
	virtual void PostLoad() override;
#endif
	// End of UObject interface

	// UActorComponent interface
	virtual void SendRenderDynamicData_Concurrent() override;
	virtual const UObject* AdditionalStatObject() const override;
	// End of UActorComponent interface

	// USceneComponent interface
	virtual bool HasAnySockets() const override;
	virtual FTransform GetSocketTransform(FName InSocketName, ERelativeTransformSpace TransformSpace = RTS_World) const override;
	virtual void QuerySupportedSockets(TArray<FComponentSocketDescription>& OutSockets) const override;
	// End of USceneComponent interface

	// UPrimitiveComponent interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const override;
	virtual class UBodySetup* GetBodySetup() override;
	virtual void GetUsedTextures(TArray<UTexture*>& OutTextures, EMaterialQualityLevel::Type QualityLevel) override;
	virtual UMaterialInterface* GetMaterial(int32 MaterialIndex) const override;
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials) const override;
	virtual void GetStreamingTextureInfo(TArray<FStreamingTexturePrimitiveInfo>& OutStreamingTextures) const override;
	virtual int32 GetNumMaterials() const override;
	// End of UPrimitiveComponent interface

protected:
	friend class FPaperSpriteSceneProxy;
};

// Allow the old name to continue to work for one release
DEPRECATED(4.3, "UPaperRenderComponent has been renamed to UPaperSpriteComponent")
typedef UPaperSpriteComponent UPaperRenderComponent;
