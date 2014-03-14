// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

AWindDirectionalSource::AWindDirectionalSource(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UTexture2D> SpriteTexture;
		FName ID_Wind;
		FText NAME_Wind;
		FConstructorStatics()
			: SpriteTexture(TEXT("/Engine/EditorResources/S_WindDirectional"))
			, ID_Wind(TEXT("Wind"))
			, NAME_Wind(NSLOCTEXT( "SpriteCategory", "Wind", "Wind" ))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;


	Component = PCIP.CreateDefaultSubobject<UWindDirectionalSourceComponent>(this, TEXT("WindDirectionalSourceComponent0"));
	RootComponent = Component;

#if WITH_EDITORONLY_DATA
	ArrowComponent = PCIP.CreateEditorOnlyDefaultSubobject<UArrowComponent>(this, TEXT("ArrowComponent0"));
	if (ArrowComponent)
	{
		ArrowComponent->ArrowColor = FColor(150, 200, 255);
		ArrowComponent->bTreatAsASprite = true;
		ArrowComponent->SpriteInfo.Category = ConstructorStatics.ID_Wind;
		ArrowComponent->SpriteInfo.DisplayName = ConstructorStatics.NAME_Wind;
		ArrowComponent->AttachParent = Component;
	}

	if (SpriteComponent)
	{
		SpriteComponent->Sprite = ConstructorStatics.SpriteTexture.Get();
		SpriteComponent->SpriteInfo.Category = ConstructorStatics.ID_Wind;
		SpriteComponent->SpriteInfo.DisplayName = ConstructorStatics.NAME_Wind;
		SpriteComponent->AttachParent = Component;
	}
#endif // WITH_EDITORONLY_DATA
}


bool FWindSourceSceneProxy::GetWindParameters(const FVector& EvaluatePosition, FVector4& WindDirectionAndSpeed, float& Weight) const 
{ 
	if (bIsPointSource)
	{
		const float Distance = (Position - EvaluatePosition).Size();
		if (Distance <= Radius)
		{
			// Mimic Engine point light attenuation with a FalloffExponent of 1
			const float RadialFalloff = FMath::Max(1.0f - ((EvaluatePosition - Position) / Radius).SizeSquared(), 0.0f);
			WindDirectionAndSpeed = FVector4((EvaluatePosition - Position) / Distance * Strength * RadialFalloff, Speed); 
			Weight = Distance / Radius * Strength;
			return true;
		}
		Weight = 0.f;
		WindDirectionAndSpeed = FVector4(0,0,0,0);
		return false;
	}

	Weight = Strength;
	WindDirectionAndSpeed = FVector4(Direction * Strength, Speed); 
	return true;
}

bool FWindSourceSceneProxy::GetDirectionalWindParameters(FVector4& WindDirectionAndSpeed, float& Weight) const 
{ 
	if (bIsPointSource)
	{
		Weight = 0.f;
		WindDirectionAndSpeed = FVector4(0,0,0,0);
		return false;
	}

	Weight = Strength;
	WindDirectionAndSpeed = FVector4(Direction * Strength, Speed); 
	return true;
}

void FWindSourceSceneProxy::ApplyWorldOffset(FVector InOffset)
{
	if (bIsPointSource)
	{
		Position+= InOffset;
	}
}

UWindDirectionalSourceComponent::UWindDirectionalSourceComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	Strength = 1.0f;
	Speed = 1.0f;
	// wind will be activated automatically by default
	bAutoActivate = true;
}

void UWindDirectionalSourceComponent::CreateRenderState_Concurrent()
{
	Super::CreateRenderState_Concurrent();
	World->Scene->AddWindSource(this);
}

void UWindDirectionalSourceComponent::SendRenderTransform_Concurrent()
{
	Super::SendRenderTransform_Concurrent();
	World->Scene->RemoveWindSource(this);
	World->Scene->AddWindSource(this);
}

void UWindDirectionalSourceComponent::DestroyRenderState_Concurrent()
{
	Super::DestroyRenderState_Concurrent();
	World->Scene->RemoveWindSource(this);
}

FWindSourceSceneProxy* UWindDirectionalSourceComponent::CreateSceneProxy() const
{
	return new FWindSourceSceneProxy(
		ComponentToWorld.GetUnitAxis( EAxis::X ),
		Strength,
		Speed
		);
}

