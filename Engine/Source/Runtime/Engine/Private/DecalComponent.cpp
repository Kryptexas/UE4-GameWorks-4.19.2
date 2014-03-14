// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	DecalComponent.cpp: Decal component implementation.
=============================================================================*/

#include "EnginePrivate.h"
#include "LevelUtils.h"
#include "EngineDecalClasses.h"


ADecalActor::ADecalActor(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UTexture2D> DecalTexture;
		FName ID_Decals;
		FText NAME_Decals;
		FConstructorStatics()
			: DecalTexture(TEXT("/Engine/EditorResources/S_DecalActorIcon"))
			, ID_Decals(TEXT("Decals"))
			, NAME_Decals(NSLOCTEXT( "SpriteCategory", "Decals", "Decals" ))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	Decal = PCIP.CreateDefaultSubobject<UDecalComponent>(this, TEXT("NewDecalComponent"));
	Decal->RelativeScale3D = FVector(128.0f, 256.0f, 256.0f);

	Decal->RelativeRotation = FRotator(-90, 0, 0);

	RootComponent = Decal;

#if WITH_EDITORONLY_DATA
	ArrowComponent = PCIP.CreateEditorOnlyDefaultSubobject<UArrowComponent>(this, TEXT("ArrowComponent0"));
	if (ArrowComponent)
	{
		ArrowComponent->bTreatAsASprite = true;
		ArrowComponent->ArrowSize = 1.0f;
		ArrowComponent->ArrowColor = FColor(80, 80, 200, 255);
		ArrowComponent->SpriteInfo.Category = ConstructorStatics.ID_Decals;
		ArrowComponent->SpriteInfo.DisplayName = ConstructorStatics.NAME_Decals;
		ArrowComponent->AttachParent = Decal;
		ArrowComponent->bAbsoluteScale = true;
	}
#endif // WITH_EDITORONLY_DATA

	BoxComponent = PCIP.CreateDefaultSubobject<UBoxComponent>(this, TEXT("DrawBox0"));
	BoxComponent->BodyInstance.bEnableCollision_DEPRECATED = false;
	BoxComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	BoxComponent->ShapeColor = FColor(80, 80, 200, 255);
	BoxComponent->bDrawOnlyIfSelected = true;
	BoxComponent->InitBoxExtent(FVector(1.0f, 1.0f, 1.0f));

	BoxComponent->AttachParent = Decal;

	SpriteComponent = PCIP.CreateDefaultSubobject<UBillboardComponent>(this, TEXT("Sprite"));
	if (SpriteComponent)
	{
		SpriteComponent->Sprite = ConstructorStatics.DecalTexture.Get();
		SpriteComponent->AttachParent = Decal;
		SpriteComponent->bIsScreenSizeScaled = true;
		SpriteComponent->ScreenSize = 0.0025f;
		SpriteComponent->bAbsoluteScale = true;
		SpriteComponent->bReceivesDecals = false;
	}

	bCanBeDamaged = false;
}

#if WITH_EDITOR
void ADecalActor::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

	if (Decal.IsValid())
	{
		Decal->RecreateRenderState_Concurrent();
	}
}

void ADecalActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	// AActor::PostEditChange will ForceUpdateComponents()
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (Decal.IsValid())
	{
		Decal->RecreateRenderState_Concurrent();
	}
}
#endif // WITH_EDITOR

FDeferredDecalProxy::FDeferredDecalProxy(const UDecalComponent* InComponent)
{
	UMaterialInterface* EffectiveMaterial = UMaterial::GetDefaultMaterial(MD_DeferredDecal);

	if(InComponent->DecalMaterial)
	{
		UMaterial* BaseMaterial = InComponent->DecalMaterial->GetMaterial();

		if(BaseMaterial->MaterialDomain == MD_DeferredDecal)
		{
			EffectiveMaterial = InComponent->DecalMaterial;
		}
	}

	Component = InComponent;
	DecalMaterial = EffectiveMaterial;
	ComponentTrans = InComponent->GetComponentToWorld();
	DrawInGame = InComponent->ShouldRender();
	bOwnerSelected = InComponent->IsOwnerSelected();
	SortOrder = InComponent->SortOrder;
}

void FDeferredDecalProxy::SetTransform(const FTransform& InComponentToWorld)
{
	ComponentTrans = InComponentToWorld;
}

UDecalComponent::UDecalComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}


void UDecalComponent::SetDecalMaterial(class UMaterialInterface* NewDecalMaterial)
{
	DecalMaterial = NewDecalMaterial;

	MarkRenderStateDirty();	
}

void UDecalComponent::PushSelectionToProxy()
{
	MarkRenderStateDirty();	
}

class UMaterialInterface* UDecalComponent::GetDecalMaterial() const
{
	return DecalMaterial;
}

class UMaterialInstanceDynamic* UDecalComponent::CreateDynamicMaterialInstance()
{
	// Create the MID
	UMaterialInstanceDynamic* Instance = UMaterialInstanceDynamic::Create(DecalMaterial, this);

	// Assign it, once parent is set
	SetDecalMaterial(Instance);

	return Instance;
}

void UDecalComponent::GetUsedMaterials( TArray<UMaterialInterface*>& OutMaterials ) const
{
	OutMaterials.Add( GetDecalMaterial() );
}


FDeferredDecalProxy* UDecalComponent::CreateSceneProxy()
{
	return new FDeferredDecalProxy(this);
}

FBoxSphereBounds UDecalComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	return FBoxSphereBounds();
}

void UDecalComponent::CreateRenderState_Concurrent()
{
	Super::CreateRenderState_Concurrent();

	// Mimics UPrimitiveComponent's visibility logic, although without the UPrimitiveCompoent visibility flags
	if ( ShouldComponentAddToScene() && ShouldRender() )
	{
		World->Scene->AddDecal(this);
	}
}

void UDecalComponent::SendRenderTransform_Concurrent()
{	
	//If Decal isn't hidden update its transform.
	if ( ShouldComponentAddToScene() && ShouldRender() )
	{
		World->Scene->UpdateDecalTransform(this);
	}

	Super::SendRenderTransform_Concurrent();
}

void UDecalComponent::DestroyRenderState_Concurrent()
{
	Super::DestroyRenderState_Concurrent();
	World->Scene->RemoveDecal(this);
}

void ADecalActor::SetDecalMaterial(class UMaterialInterface* NewDecalMaterial)
{
	if(Decal.IsValid())
	{
		Decal->SetDecalMaterial(NewDecalMaterial);
	}
}

class UMaterialInterface* ADecalActor::GetDecalMaterial() const
{
	return Decal.IsValid() ? Decal->GetDecalMaterial() : NULL;
}

class UMaterialInstanceDynamic* ADecalActor::CreateDynamicMaterialInstance()
{
	return Decal.IsValid() ? Decal->CreateDynamicMaterialInstance() : NULL;
}
