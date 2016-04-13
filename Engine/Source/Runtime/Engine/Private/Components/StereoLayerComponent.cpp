// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "IHeadMountedDisplay.h"
#include "Components/StereoLayerComponent.h"

UStereoLayerComponent::UStereoLayerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Texture(nullptr)
	, QuadSize(FVector2D(100.0f, 100.0f))
	, UVRect(FBox2D(FVector2D(0.0f, 0.0f), FVector2D(1.0f, 1.0f)))
	, StereoLayerType(SLT_FaceLocked)
	, Priority(0)
	, bIsDirty(true)
	, LayerId(0)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UStereoLayerComponent::BeginDestroy()
{
	Super::BeginDestroy();

	IStereoLayers* StereoLayers;
	if (LayerId && GEngine->HMDDevice.IsValid() && (StereoLayers = GEngine->HMDDevice->GetStereoLayers()) != nullptr)
	{
		StereoLayers->DestroyLayer(LayerId);
		LayerId = 0;
	}
}

void UStereoLayerComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );

	IStereoLayers* StereoLayers;
	if (!GEngine->HMDDevice.IsValid() || (StereoLayers = GEngine->HMDDevice->GetStereoLayers()) == nullptr || !Texture)
	{
		return;
	}

	FTransform RelativeTransform = GetRelativeTransform();
	
	// If the transform changed dirty the layer and push the new transform
	if (!bIsDirty && FMemory::Memcmp(&LastTransform, &RelativeTransform, sizeof(RelativeTransform)) != 0)
	{
		bIsDirty = true;
	}

	if (bIsDirty)
	{
		IStereoLayers::FLayerDesc LayerDsec;
		LayerDsec.Priority = Priority;
		LayerDsec.QuadSize = QuadSize;
		LayerDsec.Texture = Texture;
		LayerDsec.UVRect = UVRect;
		LayerDsec.Transform = RelativeTransform;

		switch (StereoLayerType)
		{
		case SLT_WorldLocked:
			LayerDsec.Type = IStereoLayers::WorldLocked;;
			break;
		case SLT_TorseLocked:
			LayerDsec.Type = IStereoLayers::TorsoLocked;
			break;
		case SLT_FaceLocked:
			LayerDsec.Type = IStereoLayers::FaceLocked;
			break;
		}

		if (LayerId)
		{
			StereoLayers->SetLayerDesc(LayerId, LayerDsec);
		}
		else
		{
			LayerId = StereoLayers->CreateLayer(LayerDsec);
		}

		LastTransform = RelativeTransform;
		bIsDirty = false;
	}
}

void UStereoLayerComponent::SetTexture(UTexture2D* InTexture)
{
	if (Texture == InTexture)
	{
		return;
	}

	Texture = InTexture;
	bIsDirty = true;
}

void UStereoLayerComponent::SetQuadSize(FVector2D InQuadSize)
{
	if (QuadSize == InQuadSize)
	{
		return;
	}

	QuadSize = InQuadSize;
	bIsDirty = true;
}

void UStereoLayerComponent::SetUVRect(FBox2D InUVRect)
{
	if (UVRect == InUVRect)
	{
		return;
	}

	UVRect = InUVRect;
	bIsDirty = true;
}

void UStereoLayerComponent::SetPriority(int32 InPriority)
{
	if (Priority == InPriority)
	{
		return;
	}

	Priority = InPriority;
	bIsDirty = true;
}

