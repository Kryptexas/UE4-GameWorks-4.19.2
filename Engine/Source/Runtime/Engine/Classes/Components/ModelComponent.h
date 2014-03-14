// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ModelComponent.generated.h"

UCLASS(HeaderGroup=Component, MinimalAPI)
class UModelComponent : public UPrimitiveComponent, public IInterface_CollisionDataProvider
{
	GENERATED_UCLASS_BODY()

private:
	/** The BSP tree. */
	class UModel* Model;

	/** The index of this component in the ULevel's ModelComponents array. */
	int32 ComponentIndex;

public:

	/** Description of collision */
	UPROPERTY()
	class UBodySetup* ModelBodySetup;

private:
	/** The nodes which this component renders. */
	TArray<uint16> Nodes;

	/** The elements used to render the nodes. */
	TIndirectArray<FModelElement> Elements;

public:

#if WITH_EDITOR
	/**
	 * Minimal initialization constructor.
	 */
	UModelComponent( const class FPostConstructInitializeProperties& PCIP,UModel* InModel, uint16 InComponentIndex, uint32 MaskedSurfaceFlags, const TArray<uint16>& InNodes );
#endif // WITH_EDITOR

	/**
	 * Commit the editor's changes to BSP surfaces.  Reconstructs rendering info based on the new surface data.
	 * The model should not be attached when this is called.
	 */
	void CommitSurfaces();

	/**
	 * Rebuild the model's rendering info.
	 */
	void BuildRenderData();

	/**
	 * Free empty elements.
	 */
	void ShrinkElements();


	/** Calculate the lightmap resolution to be used by the given surface. */
	ENGINE_API void GetSurfaceLightMapResolution( int32 SurfaceIndex, int32 QualityScale, int32& Width, int32& Height, FMatrix& WorldToMap, TArray<int32>* GatheredNodes=NULL ) const;

	/** Copy model elements from the other component. This is used when duplicating components for PIE to guarantee correct rendering. */
	ENGINE_API void CopyElementsFrom(UModelComponent* OtherModelComponent);

	// Begin UPrimitiveComponent interface.
	virtual bool GetLightMapResolution( int32& Width, int32& Height ) const OVERRIDE;
	virtual int32 GetStaticLightMapResolution() const OVERRIDE;
	virtual void GetLightAndShadowMapMemoryUsage( int32& LightMapMemoryUsage, int32& ShadowMapMemoryUsage ) const OVERRIDE;
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const OVERRIDE;
	virtual FPrimitiveSceneProxy* CreateSceneProxy() OVERRIDE;
	virtual bool ShouldRecreateProxyOnUpdateTransform() const OVERRIDE;
#if WITH_EDITOR
	virtual void GetStaticLightingInfo(FStaticLightingPrimitiveInfo& OutPrimitiveInfo,const TArray<ULightComponent*>& InRelevantLights,const FLightingBuildOptions& Options) OVERRIDE;
	virtual bool SetupLightmapResolutionViewInfo(FPrimitiveSceneProxy& Proxy) const OVERRIDE;
#endif
	virtual ELightMapInteractionType GetStaticLightingType() const OVERRIDE	{ return LMIT_Texture;	}
	virtual void GetStreamingTextureInfo(TArray<FStreamingTexturePrimitiveInfo>& OutStreamingTextures) const OVERRIDE;
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials) const OVERRIDE;
	virtual class UBodySetup* GetBodySetup() OVERRIDE { return ModelBodySetup; };
	virtual int32 GetNumMaterials() const OVERRIDE;
	virtual UMaterialInterface* GetMaterial(int32 MaterialIndex) const OVERRIDE;
	// End UPrimitiveComponent interface.

	// Begin UActorComponent interface.
	virtual void InvalidateLightingCacheDetailed(bool bInvalidateBuildEnqueuedLighting, bool bTranslationOnly) OVERRIDE;
	// End UActorComponent interface.

	// Begin UObject interface.
	virtual void Serialize(FArchive& Ar) OVERRIDE;
	virtual void PostLoad() OVERRIDE;
#if WITH_EDITOR
	virtual void PostEditUndo() OVERRIDE;
#endif // WITH_EDITOR
	virtual SIZE_T GetResourceSize(EResourceSizeMode::Type Mode) OVERRIDE;
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	// End UObject interface.

	// Begin Interface_CollisionDataProvider Interface
	virtual bool GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData) OVERRIDE;
	virtual bool ContainsPhysicsTriMeshData(bool InUseAllTriData) const OVERRIDE;
	virtual bool WantsNegXTriMesh() OVERRIDE { return false; }
	// End Interface_CollisionDataProvider Interface


	/** Ensure ModelBodySetup is present and correctly configured. */
	void CreateModelBodySetup();

#if WITH_EDITOR
	/** Selects all surfaces that are part of this model component. */
	ENGINE_API void SelectAllSurfaces();

	/** Invalidate current collision data */
	void InvalidateCollisionData();

	/**
	 *	Generate the Elements array.
	 *
	 *	@param	bBuildRenderData	If true, build render data after generating the elements.
	 *
	 *	@return	bool				true if successful, false if not.
	 */
	virtual bool GenerateElements(bool bBuildRenderData);
#endif // WITH_EDITOR

	//
	// Accessors.
	//

	UModel* GetModel() const { return Model; }
	const TIndirectArray<FModelElement>& GetElements() const { return Elements; }
	TIndirectArray<FModelElement>& GetElements() { return Elements; }



	/**
	 * Create a new temp FModelElement element for the component, which will be applied
	 * when lighting is all done.
	 *
	 * @param Component The UModelComponent to make the temp element in
	 */
	static FModelElement* CreateNewTempElement(UModelComponent* Component);

	/**
	 * Apply all the elements that we were putting into the TempBSPElements map to
	 * the Elements arrays in all components
	 *
	 * @param bLightingWasSuccessful If true, the lighting should be applied, otherwise, the temp lighting should be cleaned up
	 */
	ENGINE_API static void ApplyTempElements(bool bLightingWasSuccessful);


private:
	/** The new BSP elements that are made during lighting, and will be applied to the components when all lighting is done */
	static TMap<UModelComponent*, TIndirectArray<FModelElement> > TempBSPElements;

	friend void SetDebugLightmapSample(TArray<UActorComponent*>* Components, UModel* Model, int32 iSurf, FVector ClickLocation);
	friend class UModel;
	friend class FStaticLightingSystem;
};



