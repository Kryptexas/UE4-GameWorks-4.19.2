// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "SkyLightComponent.generated.h"

/** 
 * A cubemap texture resource that knows how to upload the capture data from a sky capture. 
 * @todo - support compression
 */
class FSkyTextureCubeResource : public FTexture, private FDeferredCleanupInterface
{
public:

	FSkyTextureCubeResource() :
		Size(0),
		NumMips(0),
		Format(PF_Unknown),
		TextureCubeRHI(NULL),
		NumRefs(0)
	{}

	virtual ~FSkyTextureCubeResource() { check(NumRefs == 0); }

	void SetupParameters(int32 InSize, int32 InNumMips, EPixelFormat InFormat)
	{
		Size = InSize;
		NumMips = InNumMips;
		Format = InFormat;
	}

	virtual void InitRHI();

	virtual void ReleaseRHI()
	{
		TextureCubeRHI.SafeRelease();
		FTexture::ReleaseRHI();
	}

	virtual uint32 GetSizeX() const
	{
		return Size;
	}

	virtual uint32 GetSizeY() const
	{
		return Size;
	}

	// Reference counting.
	void AddRef()
	{
		check( IsInGameThread() );
		NumRefs++;
	}

	void Release();

	// FDeferredCleanupInterface
	virtual void FinishCleanup()
	{
		delete this;
	}

private:

	int32 Size;
	int32 NumMips;
	EPixelFormat Format;
	FTextureCubeRHIRef TextureCubeRHI;
	int32 NumRefs;
};

UENUM()
enum ESkyLightSourceType
{
	/** Construct the sky light from the captured scene, anything further than SkyDistanceThreshold from the sky light position will be included. */
	SLS_CapturedScene,
	/** Construct the sky light from the specified cubemap. */
	SLS_SpecifiedCubemap,
	SLS_MAX,
};

UCLASS(HeaderGroup=Component, ClassGroup=Lights, HideCategories=(Trigger,Activation,"Components|Activation",Physics), meta=(BlueprintSpawnableComponent))
class ENGINE_API USkyLightComponent : public ULightComponentBase
{
	GENERATED_UCLASS_BODY()

	/** Indicates where to get the light contribution from. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Light)
	TEnumAsByte<enum ESkyLightSourceType> SourceType;

	/** Cubemap to use for sky lighting if SourceType is set to SLS_SpecifiedCubemap. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Light)
	class UTextureCube* Cubemap;

	/** 
	 * Distance from the sky light at which any geometry should be treated as part of the sky. 
	 * This is also used by reflection captures, so update reflection captures to see the impact.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Light)
	float SkyDistanceThreshold;

	/** 
	 * Whether all lighting from the lower hemisphere should be set to zero.  
	 * This is valid when lighting a scene on a planet, but can be disabled for model showcases, etc.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Light)
	bool bLowerHemisphereIsBlack;

	class FSkyLightSceneProxy* CreateSceneProxy() const;

	// Begin UObject Interface
	virtual void PostInitProperties();	
	virtual void PostLoad() OVERRIDE;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	virtual bool CanEditChange(const UProperty* InProperty) const OVERRIDE;
	virtual void CheckForErrors() OVERRIDE;
#endif // WITH_EDITOR
	virtual void BeginDestroy() OVERRIDE;
	virtual bool IsReadyForFinishDestroy() OVERRIDE;
	// End UObject Interface

	virtual void GetComponentInstanceData(FComponentInstanceDataCache& Cache) const OVERRIDE;
	virtual void ApplyComponentInstanceData(const FComponentInstanceDataCache& Cache) OVERRIDE;

	/** Called each tick to recapture and queued sky captures. */
	static void UpdateSkyCaptureContents(UWorld* WorldToUpdate);

	UFUNCTION(BlueprintCallable, Category="Rendering|Components|SkyLight")
	void SetBrightness(float NewBrightness);

	/** Set color of the light */
	UFUNCTION(BlueprintCallable, Category="Rendering|Components|SkyLight")
	void SetLightColor(FLinearColor NewLightColor);

	/** Indicates that the capture needs to recapture the scene, adds it to the recapture queue. */
	void SetCaptureIsDirty();

	/** 
	 * Recaptures the scene for the skylight. 
	 * This is useful for making sure the sky light is up to date after changing something in the world that it would capture.
	 * Warning: this is very costly and will definitely cause a hitch.
	 */
	UFUNCTION(BlueprintCallable, Category="Rendering|Components|SkyLight")
	void RecaptureSky();

	void SetIrradianceEnvironmentMap(const FSHVectorRGB3& InIrradianceEnvironmentMap)
	{
		IrradianceEnvironmentMap = InIrradianceEnvironmentMap;
	}

protected:

	/** Whether the reflection capture needs to re-capture the scene. */
	bool bCaptureDirty;

	/** Indicates whether the cached data stored in GetComponentInstanceData is valid to be applied in ApplyComponentInstanceData. */
	bool bSavedConstructionScriptValuesValid;

	TRefCountPtr<FSkyTextureCubeResource> ProcessedSkyTexture;

	FSHVectorRGB3 IrradianceEnvironmentMap;

	/** Fence used to track progress of releasing resources on the rendering thread. */
	FRenderCommandFence ReleaseResourcesFence;

	FSkyLightSceneProxy* SceneProxy;

	/** 
	 * List of sky captures that need to be recaptured.
	 * These have to be queued because we can only render the scene to update captures at certain points, after the level has loaded.
	 * This queue should be in the UWorld or the FSceneInterface, but those are not available yet in PostLoad.
	 */
	static TArray<USkyLightComponent*> SkyCapturesToUpdate;

	// Begin UActorComponent Interface
	virtual void CreateRenderState_Concurrent() OVERRIDE;
	virtual void DestroyRenderState_Concurrent() OVERRIDE;
	// Begin UActorComponent Interface

	friend class FSkyLightSceneProxy;
};



