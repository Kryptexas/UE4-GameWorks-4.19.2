// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
ActorFactory.cpp:
=============================================================================*/

#include "UnrealEd.h"
#include "ActorFactoryProceduralFoliage.h"
#include "ProceduralFoliage.h"
#include "ProceduralFoliageActor.h"
#include "ProceduralFoliageComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogActorFactory, Log, All);

#define LOCTEXT_NAMESPACE "ActorFactoryProceduralFoliage"

/*-----------------------------------------------------------------------------
UActorFactoryProceduralFoliage
-----------------------------------------------------------------------------*/
UActorFactoryProceduralFoliage::UActorFactoryProceduralFoliage(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	DisplayName = LOCTEXT("ProceduralFoliageDisplayName", "ProceduralFoliage");
	NewActorClass = AProceduralFoliageActor::StaticClass();
	bUseSurfaceOrientation = true;
}

bool UActorFactoryProceduralFoliage::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg)
{
	if (!AssetData.IsValid() || !AssetData.GetClass()->IsChildOf(UProceduralFoliage::StaticClass()))
	{
		OutErrorMsg = NSLOCTEXT("CanCreateActor", "NoProceduralFoliage", "A valid ProceduralFoliage must be specified.");
		return false;
	}

	return true;
}

void UActorFactoryProceduralFoliage::PostSpawnActor(UObject* Asset, AActor* NewActor)
{
	Super::PostSpawnActor(Asset, NewActor);
	UProceduralFoliage* ProceduralFoliage = CastChecked<UProceduralFoliage>(Asset);

	UE_LOG(LogActorFactory, Log, TEXT("Actor Factory created %s"), *ProceduralFoliage->GetName());

	// Change properties
	AProceduralFoliageActor* PFA = CastChecked<AProceduralFoliageActor>(NewActor);
	UProceduralFoliageComponent* ProceduralComponent = PFA->ProceduralComponent;
	check(ProceduralComponent);

	ProceduralComponent->UnregisterComponent();

	ProceduralComponent->ProceduralFoliage = ProceduralFoliage;

	// Init Component
	ProceduralComponent->RegisterComponent();
}

UObject* UActorFactoryProceduralFoliage::GetAssetFromActorInstance(AActor* Instance)
{
	check(Instance->IsA(NewActorClass));
	AProceduralFoliageActor* ProceduralFoliage = CastChecked<AProceduralFoliageActor>(Instance);
	UProceduralFoliageComponent* ProceduralComponent = ProceduralFoliage->ProceduralComponent;
	check(ProceduralComponent);
	return ProceduralComponent->ProceduralFoliage;
}

void UActorFactoryProceduralFoliage::PostCreateBlueprint(UObject* Asset, AActor* CDO)
{
	if (Asset != NULL && CDO != NULL)
	{
		UProceduralFoliage* ProceduralFoliage = CastChecked<UProceduralFoliage>(Asset);
		AProceduralFoliageActor* PFA = CastChecked<AProceduralFoliageActor>(CDO);
		UProceduralFoliageComponent* ProceduralComponent = PFA->ProceduralComponent;
		ProceduralComponent->ProceduralFoliage = ProceduralFoliage;
	}
}
#undef LOCTEXT_NAMESPACE
