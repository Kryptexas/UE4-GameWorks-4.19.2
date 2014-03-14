// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	HeightFogComponent.cpp: Height fog implementation.
=============================================================================*/

#include "EnginePrivate.h"
#include "Net/UnrealNetwork.h"


UExponentialHeightFogComponent::UExponentialHeightFogComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bEnabled_DEPRECATED = true;

	FogInscatteringColor = FLinearColor(0.447f, 0.638f, 1.0f);

	DirectionalInscatteringExponent = 4.0f;
	DirectionalInscatteringStartDistance = 10000.0f;
	DirectionalInscatteringColor = FLinearColor(0.25f, 0.25f, 0.125f);

	FogDensity = 0.02f;
	FogHeightFalloff = 0.2f;
	FogMaxOpacity = 1.0f;
	StartDistance = 0.0f;
}

void UExponentialHeightFogComponent::CreateRenderState_Concurrent()
{
	Super::CreateRenderState_Concurrent();

	if(bVisible && FogDensity * 1000 > DELTA && FogMaxOpacity > DELTA)
	{
		World->Scene->AddExponentialHeightFog(this);
	}
}

void UExponentialHeightFogComponent::SendRenderTransform_Concurrent()
{
	World->Scene->RemoveExponentialHeightFog(this);
	if(bVisible && FogDensity * 1000 > DELTA && FogMaxOpacity > DELTA)
	{
		World->Scene->AddExponentialHeightFog(this);
	}

	Super::SendRenderTransform_Concurrent();
}

void UExponentialHeightFogComponent::DestroyRenderState_Concurrent()
{
	Super::DestroyRenderState_Concurrent();
	World->Scene->RemoveExponentialHeightFog(this);
}

void UExponentialHeightFogComponent::PostLoad()
{
	Super::PostLoad();

	if (GetLinkerUE4Version() < VER_UE4_REMOVE_COMPONENT_ENABLED_FLAG)
	{
		bVisible = bEnabled_DEPRECATED;	
	}
}

#if WITH_EDITOR
void UExponentialHeightFogComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	FogDensity = FMath::Clamp(FogDensity, 0.0f, 10.0f);
	FogHeightFalloff = FMath::Clamp(FogHeightFalloff, 0.0f, 2.0f);
	FogMaxOpacity = FMath::Clamp(FogMaxOpacity, 0.0f, 1.0f);
	StartDistance = FMath::Clamp(StartDistance, 0.0f, (float)WORLD_MAX);

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif // WITH_EDITOR

void UExponentialHeightFogComponent::PostInterpChange(UProperty* PropertyThatChanged)
{
	Super::PostInterpChange(PropertyThatChanged);

	MarkRenderStateDirty();
}

void UExponentialHeightFogComponent::SetFogDensity(float Value)
{
	if(FogDensity != Value)
	{
		FogDensity = Value;
		MarkRenderStateDirty();
	}
}

void UExponentialHeightFogComponent::SetFogInscatteringColor(FLinearColor Value)
{
	if(FogInscatteringColor != Value)
	{
		FogInscatteringColor = Value;
		MarkRenderStateDirty();
	}
}

void UExponentialHeightFogComponent::SetDirectionalInscatteringExponent(float Value)
{
	if(DirectionalInscatteringExponent != Value)
	{
		DirectionalInscatteringExponent = Value;
		MarkRenderStateDirty();
	}
}

void UExponentialHeightFogComponent::SetDirectionalInscatteringStartDistance(float Value)
{
	if(DirectionalInscatteringStartDistance != Value)
	{
		DirectionalInscatteringStartDistance = Value;
		MarkRenderStateDirty();
	}
}

void UExponentialHeightFogComponent::SetDirectionalInscatteringColor(FLinearColor Value)
{
	if(DirectionalInscatteringColor != Value)
	{
		DirectionalInscatteringColor = Value;
		MarkRenderStateDirty();
	}
}

void UExponentialHeightFogComponent::SetFogHeightFalloff(float Value)
{
	if(FogHeightFalloff != Value)
	{
		FogHeightFalloff = Value;
		MarkRenderStateDirty();
	}
}

void UExponentialHeightFogComponent::SetFogMaxOpacity(float Value)
{
	if(FogMaxOpacity != Value)
	{
		FogMaxOpacity = Value;
		MarkRenderStateDirty();
	}
}

void UExponentialHeightFogComponent::SetStartDistance(float Value)
{
	if(StartDistance != Value)
	{
		StartDistance = Value;
		MarkRenderStateDirty();
	}
}

//////////////////////////////////////////////////////////////////////////
// AExponentialHeightFog

AExponentialHeightFog::AExponentialHeightFog(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UTexture2D> FogTextureObject;
		FName ID_Fog;
		FText NAME_Fog;
		FConstructorStatics()
			: FogTextureObject(TEXT("/Engine/EditorResources/S_ExpoHeightFog"))
			, ID_Fog(TEXT("Fog"))
			, NAME_Fog(NSLOCTEXT( "SpriteCategory", "Fog", "Fog" ))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	Component = PCIP.CreateDefaultSubobject<UExponentialHeightFogComponent>(this, TEXT("HeightFogComponent0"));
	RootComponent = Component;

#if WITH_EDITORONLY_DATA
	if (SpriteComponent)
	{
		SpriteComponent->Sprite = ConstructorStatics.FogTextureObject.Get();
		SpriteComponent->SpriteInfo.Category = ConstructorStatics.ID_Fog;
		SpriteComponent->SpriteInfo.DisplayName = ConstructorStatics.NAME_Fog;
		SpriteComponent->AttachParent = Component;
	}
#endif // WITH_EDITORONLY_DATA
}

void AExponentialHeightFog::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	bEnabled = Component->bVisible;
}

void AExponentialHeightFog::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );
	
	DOREPLIFETIME( AExponentialHeightFog, bEnabled );
}

void AExponentialHeightFog::OnRep_bEnabled()
{
	Component->SetVisibility(bEnabled);
}
