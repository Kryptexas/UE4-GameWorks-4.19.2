// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "MeshComponent.generated.h"

UCLASS(HeaderGroup=Component, abstract)
class ENGINE_API UMeshComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

	/** Per-Component material overrides.  These must NOT be set directly or a race condition can occur between GC and the rendering thread. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Rendering)
	TArray<class UMaterialInterface*> Materials;


	// Begin UObject Interface
	virtual void BeginDestroy() OVERRIDE;
	// End UObject Interface

	// Begin UPrimitiveComponent Interface
	virtual int32 GetNumMaterials() const OVERRIDE;
	virtual UMaterialInterface* GetMaterial(int32 ElementIndex) const OVERRIDE;
	virtual void SetMaterial(int32 ElementIndex, UMaterialInterface* Material) OVERRIDE;
	// End UPrimitiveComponent Interface

	/** Accesses the scene relevance information for the materials applied to the mesh. */
	FMaterialRelevance GetMaterialRelevance() const;

	/**
	 *	Tell the streaming system whether or not all mip levels of all textures used by this component should be loaded and remain loaded.
	 *	@param bForceMiplevelsToBeResident		Whether textures should be forced to be resident or not.
	 */
	void SetTextureForceResidentFlag( bool bForceMiplevelsToBeResident );

	/**
	 *	Tell the streaming system to start loading all textures with all mip-levels.
	 *	@param Seconds							Number of seconds to force all mip-levels to be resident
	 *	@param bPrioritizeCharacterTextures		Whether character textures should be prioritized for a while by the streaming system
	 *	@param CinematicTextureGroups			Bitfield indicating which texture groups that use extra high-resolution mips
	 */
	void PrestreamTextures( float Seconds, bool bPrioritizeCharacterTextures, int32 CinematicTextureGroups = 0 );


};



