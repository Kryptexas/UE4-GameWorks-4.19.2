// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "DecalComponent.generated.h"

	// -> will be exported to EngineDecalClasses.h

/** 
 * A material that is rendered onto the surface of a mesh. A kind of 'bumper sticker' for a model.
 */
UCLASS(hidecategories=(Collision, Object, Physics, SceneComponent, Activation, "Components|Activation", Physics, Mobility), ClassGroup=Rendering, HeaderGroup=Decal, MinimalAPI, meta=(BlueprintSpawnableComponent))
class UDecalComponent : public USceneComponent
{
	GENERATED_UCLASS_BODY()

	/** Decal material. */
	//editable(Decal) private{private} const MaterialInterface	DecalMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Decal)
	class UMaterialInterface* DecalMaterial;

	/** 
	 * Controls the order in which decal elements are rendered.  Higher values draw later (on top). 
	 * Setting many different sort orders on many different decals prevents sorting by state and can reduce performance.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Decal)
	int32 SortOrder;

	/** setting decal material on decal component. This will force the decal to reattach */
	UFUNCTION(BlueprintCallable, Category="Rendering|Components|Decal")
	ENGINE_API void SetDecalMaterial(class UMaterialInterface* NewDecalMaterial);

	/** Accessor for decal material */
	UFUNCTION(BlueprintCallable, Category="Rendering|Components|Decal")
	ENGINE_API class UMaterialInterface* GetDecalMaterial() const;

	/** Utility to allocate a new Dynamic Material Instance, set its parent to the currently applied material, and assign it */
	UFUNCTION(BlueprintCallable, Category="Rendering|Components|Decal")
	virtual class UMaterialInstanceDynamic* CreateDynamicMaterialInstance();


public:
	/** The decal proxy. */
	FDeferredDecalProxy* SceneProxy;

	/**
	 * Pushes new selection state to the render thread primitive proxy
	 */
	ENGINE_API void PushSelectionToProxy();


	
	/**
	 * Retrieves the materials used in this component
	 *
	 * @param OutMaterials	The list of used materials.
	 */
	virtual void GetUsedMaterials( TArray<UMaterialInterface*>& OutMaterials ) const;
	
	virtual FDeferredDecalProxy* CreateSceneProxy();
	virtual int32 GetNumMaterials() const
	{
		return 1; // DecalMaterial
	}

	virtual UMaterialInterface* GetMaterial(int32 ElementIndex) const
	{
		return (ElementIndex == 0) ? DecalMaterial : NULL;
	}
	virtual void SetMaterial(int32 ElementIndex, UMaterialInterface* InMaterial)
	{
		if (ElementIndex == 0)
		{
			SetDecalMaterial(InMaterial);
		}
	}
	
	// Begin UActorComponent Interface
	virtual void CreateRenderState_Concurrent() OVERRIDE;
	virtual void DestroyRenderState_Concurrent() OVERRIDE;
	virtual void SendRenderTransform_Concurrent() OVERRIDE;
	// End UActorComponent Interface

	// Begin USceneComponent Interface
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const OVERRIDE;
	// End USceneComponent Interface
};

