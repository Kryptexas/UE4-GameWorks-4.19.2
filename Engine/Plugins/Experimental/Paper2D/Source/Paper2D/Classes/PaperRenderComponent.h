// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperRenderComponent.generated.h"

UCLASS(DependsOn=UPaperSprite, MinimalAPI, ShowCategories=(Mobility), meta=(BlueprintSpawnableComponent))
class UPaperRenderComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

protected:
	// The sprite asset used by this component
	UPROPERTY(Category=Sprite, EditAnywhere, BlueprintReadOnly, meta=(DisplayThumbnail = "true"))
	UPaperSprite* SourceSprite;

	// The material override for this sprite component (if any)
	UPROPERTY(Category=Sprite, EditAnywhere, BlueprintReadOnly)
	UMaterialInterface* MaterialOverride;

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
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif
	// End of UObject interface

	// UActorComponent interface
	virtual void SendRenderDynamicData_Concurrent() OVERRIDE;
	virtual const UObject* AdditionalStatObject() const OVERRIDE;
	// End of UActorComponent interface

	// USceneComponent interface
	virtual bool HasAnySockets() const OVERRIDE;
	virtual FTransform GetSocketTransform(FName InSocketName, ERelativeTransformSpace TransformSpace = RTS_World) const OVERRIDE;
	virtual void QuerySupportedSockets(TArray<FComponentSocketDescription>& OutSockets) const OVERRIDE;
	// End of USceneComponent interface

	// UPrimitiveComponent interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() OVERRIDE;
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const OVERRIDE;
	virtual class UBodySetup* GetBodySetup() OVERRIDE;
	// End of UPrimitiveComponent interface

protected:
	friend class FPaperRenderSceneProxy;
};
