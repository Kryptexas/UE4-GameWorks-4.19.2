// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SkyLightComponent.cpp: SkyLightComponent implementation.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineLightClasses.h"
#if WITH_EDITOR
#include "ObjectEditorUtils.h"
#endif
#include "MessageLog.h"
#include "UObjectToken.h"
#include "Net/UnrealNetwork.h"
#include "MapErrors.h"

#define LOCTEXT_NAMESPACE "SkyLightComponent"

void FSkyTextureCubeResource::InitRHI()
{
	if (GRHIFeatureLevel >= ERHIFeatureLevel::SM3)
	{
		TextureCubeRHI = RHICreateTextureCube(Size, Format, NumMips, 0, NULL);
		TextureRHI = TextureCubeRHI;

		// Create the sampler state RHI resource.
		FSamplerStateInitializerRHI SamplerStateInitializer
		(
			SF_Trilinear,
			AM_Clamp,
			AM_Clamp,
			AM_Clamp
		);
		SamplerStateRHI = RHICreateSamplerState(SamplerStateInitializer);
	}
}

void FSkyTextureCubeResource::Release()
{
	check( IsInGameThread() );
	checkSlow(NumRefs > 0);
	if(--NumRefs == 0)
	{
		BeginReleaseResource(this);
		// Have to defer actual deletion until above rendering command has been processed, we will use the deferred cleanup interface for that
		BeginCleanup(this);
	}
}

void UWorld::UpdateAllSkyCaptures()
{
	TArray<USkyLightComponent*> UpdatedComponents;

	for (TObjectIterator<USkyLightComponent> It; It; ++It)
	{
		USkyLightComponent* CaptureComponent = *It;

		if (ContainsActor(CaptureComponent->GetOwner()) && !CaptureComponent->IsPendingKill())
		{
			// Purge cached derived data and force an update
			CaptureComponent->SetCaptureIsDirty();
			UpdatedComponents.Add(CaptureComponent);
		}
	}

	USkyLightComponent::UpdateSkyCaptureContents(this);
}

FSkyLightSceneProxy::FSkyLightSceneProxy(const USkyLightComponent* InLightComponent)
	: LightComponent(InLightComponent)
	, ProcessedTexture(InLightComponent->ProcessedSkyTexture)
	, SkyDistanceThreshold(InLightComponent->SkyDistanceThreshold)
	, bCastShadows(InLightComponent->CastShadows)
	, bPrecomputedLightingIsValid(InLightComponent->bPrecomputedLightingIsValid)
	, LightColor(FLinearColor(InLightComponent->LightColor) * InLightComponent->Intensity)
	, IrradianceEnvironmentMap(InLightComponent->IrradianceEnvironmentMap)
{
}

USkyLightComponent::USkyLightComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
#if WITH_EDITORONLY_DATA
	if (!IsRunningCommandlet())
	{
		static ConstructorHelpers::FObjectFinder<UTexture2D> StaticTexture(TEXT("/Engine/EditorResources/LightIcons/SkyLight"));
		StaticEditorTexture = StaticTexture.Object;
		DynamicEditorTexture = StaticTexture.Object;
	}
#endif

	Brightness_DEPRECATED = 1;
	Intensity = 1;
	SkyDistanceThreshold = 150000;
	Mobility = EComponentMobility::Stationary;
	bCaptureDirty = false;
	bLowerHemisphereIsBlack = true;
	bSavedConstructionScriptValuesValid = true;
}

FSkyLightSceneProxy* USkyLightComponent::CreateSceneProxy() const
{
	if (ProcessedSkyTexture)
	{
		return new FSkyLightSceneProxy(this);
	}
	
	return NULL;
}

void USkyLightComponent::SetCaptureIsDirty() 
{ 
	if (bVisible && bAffectsWorld)
	{
		SkyCapturesToUpdate.AddUnique(this);
		bCaptureDirty = true; 

		// Mark saved values as invalid, in case a sky recapture is requested in a construction script between a save / restore of sky capture state
		bSavedConstructionScriptValuesValid = false;
	}
}

TArray<USkyLightComponent*> USkyLightComponent::SkyCapturesToUpdate;

void USkyLightComponent::CreateRenderState_Concurrent()
{
	Super::CreateRenderState_Concurrent();

	bool bHidden = false;
#if WITH_EDITORONLY_DATA
	bHidden = GetOwner() ? GetOwner()->bHiddenEdLevel : false;
#endif // WITH_EDITORONLY_DATA

	if(!ShouldComponentAddToScene())
	{
		bHidden = true;
	}

	const bool bIsValid = SourceType != SLS_SpecifiedCubemap || Cubemap != NULL;

	if (bAffectsWorld && bVisible && !bHidden && bIsValid)
	{
		// Create the light's scene proxy.
		SceneProxy = CreateSceneProxy();

		// Add the light to the scene.
		World->Scene->SetSkyLight(SceneProxy);
	}
}

extern ENGINE_API int32 GReflectionCaptureSize;

void USkyLightComponent::PostInitProperties()
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		// Enqueue an update by default, so that newly placed components will get an update
		// PostLoad will undo this for components loaded from disk
		bCaptureDirty = true;
		SkyCapturesToUpdate.AddUnique(this);
	}

	Super::PostInitProperties();
}

void USkyLightComponent::PostLoad()
{
	Super::PostLoad();

	// All components are queued for update on creation by default, remove if needed
	if (!bVisible || HasAnyFlags(RF_ClassDefaultObject))
	{
		SkyCapturesToUpdate.Remove(this);
		bCaptureDirty = false;
	}
}

void USkyLightComponent::DestroyRenderState_Concurrent()
{
	Super::DestroyRenderState_Concurrent();
	World->Scene->SetSkyLight(NULL);

	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
		FDestroySkyLightCommand,
		FSkyLightSceneProxy*,LightSceneProxy,SceneProxy,
	{
		delete LightSceneProxy;
	});

	SceneProxy = NULL;
}

#if WITH_EDITOR
void USkyLightComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName CategoryName = FObjectEditorUtils::GetCategoryFName(PropertyChangedEvent.Property);

	// Other options not supported yet
	Mobility = EComponentMobility::Stationary;

	Super::PostEditChangeProperty(PropertyChangedEvent);

	SetCaptureIsDirty();
}

bool USkyLightComponent::CanEditChange(const UProperty* InProperty) const
{
	if (InProperty)
	{
		FString PropertyName = InProperty->GetName();

		if (FCString::Strcmp(*PropertyName, TEXT("Cubemap")) == 0)
		{
			return SourceType == SLS_SpecifiedCubemap;
		}
	}

	return Super::CanEditChange(InProperty);
}

void USkyLightComponent::CheckForErrors()
{
	AActor* Owner = GetOwner();

	if (Owner && bVisible && bAffectsWorld)
	{
		UWorld* ThisWorld = Owner->GetWorld();
		bool bMultipleFound = false;

		if (ThisWorld)
		{
			for (TObjectIterator<USkyLightComponent> ComponentIt; ComponentIt; ++ComponentIt)
			{
				USkyLightComponent* Component = *ComponentIt;

				if (Component != this 
					&& !Component->HasAnyFlags(RF_PendingKill)
					&& Component->bVisible
					&& Component->bAffectsWorld
					&& Component->GetOwner() 
					&& ThisWorld->ContainsActor(Component->GetOwner()))
				{
					bMultipleFound = true;
					break;
				}
			}
		}

		if (bMultipleFound)
		{
			FMessageLog("MapCheck").Error()
				->AddToken(FUObjectToken::Create(Owner))
				->AddToken(FTextToken::Create(LOCTEXT( "MapCheck_Message_MultipleSkyLights", "Multiple sky lights are active, only one can be enabled per world." )))
				->AddToken(FMapErrorToken::Create(FMapErrors::MultipleSkyLights));
		}
	}
}

#endif // WITH_EDITOR

void USkyLightComponent::BeginDestroy()
{
	// Deregister the component from the update queue
	if (bCaptureDirty)
	{
		SkyCapturesToUpdate.Remove(this);
	}

	// Release reference
	ProcessedSkyTexture = NULL;

	// Begin a fence to track the progress of the above BeginReleaseResource being completed on the RT
	ReleaseResourcesFence.BeginFence();

	Super::BeginDestroy();
}

bool USkyLightComponent::IsReadyForFinishDestroy()
{
	// Wait until the fence is complete before allowing destruction
	return Super::IsReadyForFinishDestroy() && ReleaseResourcesFence.IsFenceComplete();
}

/** Used to store lightmap data during RerunConstructionScripts */
class FPrecomputedSkyLightInstanceData : public FComponentInstanceDataBase
{
public:
	static const FName PrecomputedSkyLightInstanceDataTypeName;

	virtual ~FPrecomputedSkyLightInstanceData()
	{}

	// Begin FComponentInstanceDataBase interface
	virtual FName GetDataTypeName() const OVERRIDE
	{
		return PrecomputedSkyLightInstanceDataTypeName;
	}
	// End FComponentInstanceDataBase interface

	FGuid LightGuid;
	bool bPrecomputedLightingIsValid;
	// This has to be refcounted to keep it alive during the handoff without doing a deep copy
	TRefCountPtr<FSkyTextureCubeResource> ProcessedSkyTexture;
	FSHVectorRGB3 IrradianceEnvironmentMap;
};

// Init type name static
const FName FPrecomputedSkyLightInstanceData::PrecomputedSkyLightInstanceDataTypeName(TEXT("PrecomputedSkyLightInstanceData"));

void USkyLightComponent::GetComponentInstanceData(FComponentInstanceDataCache& Cache) const
{
	// Allocate new struct for holding light map data
	TSharedRef<FPrecomputedSkyLightInstanceData> LightMapData = MakeShareable(new FPrecomputedSkyLightInstanceData());

	// Fill in info
	LightMapData->LightGuid = LightGuid;
	LightMapData->bPrecomputedLightingIsValid = bPrecomputedLightingIsValid;
	LightMapData->ProcessedSkyTexture = ProcessedSkyTexture;
	LightMapData->IrradianceEnvironmentMap = IrradianceEnvironmentMap;

	// Add to cache
	Cache.AddInstanceData(LightMapData);
}

void USkyLightComponent::ApplyComponentInstanceData(const FComponentInstanceDataCache& Cache)
{
	TArray< TSharedPtr<FComponentInstanceDataBase> > CachedData;
	Cache.GetInstanceDataOfType(FPrecomputedSkyLightInstanceData::PrecomputedSkyLightInstanceDataTypeName, CachedData);

	for (int32 DataIdx = 0; DataIdx<CachedData.Num(); DataIdx++)
	{
		check(CachedData[DataIdx].IsValid());
		check(CachedData[DataIdx]->GetDataTypeName() == FPrecomputedSkyLightInstanceData::PrecomputedSkyLightInstanceDataTypeName);
		TSharedPtr<FPrecomputedSkyLightInstanceData> LightMapData = StaticCastSharedPtr<FPrecomputedSkyLightInstanceData>(CachedData[DataIdx]);

		LightGuid = LightMapData->LightGuid;
		bPrecomputedLightingIsValid = LightMapData->bPrecomputedLightingIsValid;
		ProcessedSkyTexture = LightMapData->ProcessedSkyTexture;
		IrradianceEnvironmentMap = LightMapData->IrradianceEnvironmentMap;

		if (ProcessedSkyTexture && bSavedConstructionScriptValuesValid)
		{
			// We have valid capture state, remove the queued update
			bCaptureDirty = false;
			SkyCapturesToUpdate.Remove(this);
		}

		MarkRenderStateDirty();
	}
}

void USkyLightComponent::UpdateSkyCaptureContents(UWorld* WorldToUpdate)
{
	if (WorldToUpdate->Scene)
	{
		// Iterate backwards so we can remove elements without changing the index
		for (int32 CaptureIndex = SkyCapturesToUpdate.Num() - 1; CaptureIndex >= 0; CaptureIndex--)
		{
			USkyLightComponent* CaptureComponent = SkyCapturesToUpdate[CaptureIndex];

			if (!CaptureComponent->GetOwner() || WorldToUpdate->ContainsActor(CaptureComponent->GetOwner()))
			{
				// Only capture valid sky light components
				if (CaptureComponent->SourceType != SLS_SpecifiedCubemap || CaptureComponent->Cubemap)
				{
					// Allocate the needed texture on first capture
					if (!CaptureComponent->ProcessedSkyTexture)
					{
						CaptureComponent->ProcessedSkyTexture = new FSkyTextureCubeResource();
						CaptureComponent->ProcessedSkyTexture->SetupParameters(GReflectionCaptureSize, FMath::CeilLogTwo(GReflectionCaptureSize) + 1, PF_FloatRGBA);
						BeginInitResource(CaptureComponent->ProcessedSkyTexture);
						CaptureComponent->MarkRenderStateDirty();
					}

					WorldToUpdate->Scene->UpdateSkyCaptureContents(CaptureComponent);
				}

				// Only remove queued update requests if we processed it for the right world
				SkyCapturesToUpdate.RemoveAt(CaptureIndex);
			}
		}
	}
}

/** Set brightness of the light */
void USkyLightComponent::SetBrightness(float NewBrightness)
{
	// Can't set brightness on a static light
	if (!(IsRegistered() && Mobility == EComponentMobility::Static)
		&& Intensity != NewBrightness)
	{
		Intensity = NewBrightness;
		MarkRenderStateDirty();
	}
}

/** Set color of the light */
void USkyLightComponent::SetLightColor(FLinearColor NewLightColor)
{
	FColor NewColor(NewLightColor);

	// Can't set color on a static light
	if (!(IsRegistered() && Mobility == EComponentMobility::Static)
		&& LightColor != NewColor)
	{
		LightColor	= NewColor;
		MarkRenderStateDirty();
	}
}

void USkyLightComponent::RecaptureSky()
{
	SetCaptureIsDirty();
}

ASkyLight::ASkyLight(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		FName ID_Sky;
		FText NAME_Sky;

		FConstructorStatics()
			: ID_Sky(TEXT("Sky"))
			, NAME_Sky(NSLOCTEXT( "SpriteCategory", "Sky", "Sky" ))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	LightComponent = PCIP.CreateDefaultSubobject<USkyLightComponent>(this, TEXT("SkyLightComponent0"));
	RootComponent = LightComponent;

}

void ASkyLight::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME( ASkyLight, bEnabled );
}

void ASkyLight::OnRep_bEnabled()
{
	LightComponent->SetVisibility(bEnabled);
}

#undef LOCTEXT_NAMESPACE