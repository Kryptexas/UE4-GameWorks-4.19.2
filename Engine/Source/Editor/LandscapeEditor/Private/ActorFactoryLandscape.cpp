// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "LandscapeEditorPrivatePCH.h"
#include "ActorFactoryLandscape.h"
#include "LandscapePlaceholder.h"

#define LOCTEXT_NAMESPACE "Landscape"

UActorFactoryLandscape::UActorFactoryLandscape(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	DisplayName = LOCTEXT("Landscape", "Landscape");
	NewActorClass = ALandscapeProxy::StaticClass();
}

AActor* UActorFactoryLandscape::SpawnActor(UObject* Asset, ULevel* InLevel, const FVector& Location, const FRotator& Rotation, EObjectFlags ObjectFlags, const FName& Name)
{
	GEditorModeTools().ActivateMode(FBuiltinEditorModes::EM_Landscape);

	FEdModeLandscape* EdMode = (FEdModeLandscape*)GEditorModeTools().GetActiveMode(FBuiltinEditorModes::EM_Landscape);

	EdMode->UISettings->NewLandscape_Location = Location;
	EdMode->UISettings->NewLandscape_Rotation = Rotation;

	EdMode->SetCurrentTool("ToolSet_NewLandscape");

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.OverrideLevel = InLevel;
	SpawnInfo.ObjectFlags = ObjectFlags;
	SpawnInfo.Name = Name;
	return InLevel->OwningWorld->SpawnActor(ALandscapePlaceholder::StaticClass(), &Location, &Rotation, SpawnInfo);
}

ALandscapePlaceholder::ALandscapePlaceholder(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UTexture2D> TerrainTexture;
		FConstructorStatics()
			: TerrainTexture(TEXT("/Engine/EditorResources/S_Terrain"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	TSubobjectPtr<USceneComponent> SceneComponent = PCIP.CreateDefaultSubobject<USceneComponent>(this, TEXT("RootComponent0"));
	RootComponent = SceneComponent.Get();
	RootComponent->Mobility = EComponentMobility::Static;

#if WITH_EDITORONLY_DATA
	TSubobjectPtr<UBillboardComponent> SpriteComponent = PCIP.CreateEditorOnlyDefaultSubobject<UBillboardComponent>(this, TEXT("Sprite"));
	if (SpriteComponent.IsValid())
	{
		SpriteComponent->Sprite = ConstructorStatics.TerrainTexture.Get();
		SpriteComponent->AttachParent = RootComponent;
		SpriteComponent->RelativeLocation = FVector(0, 0, 100);
		SpriteComponent->bAbsoluteScale = true;
	}
#endif
}

bool ALandscapePlaceholder::TeleportTo(const FVector& DestLocation, const FRotator& DestRotation, bool bIsATest /*= false*/, bool bNoCheck /*= false*/)
{
	bool bResult = Super::TeleportTo(DestLocation, DestRotation, bIsATest, bNoCheck);

	GEditorModeTools().ActivateMode(FBuiltinEditorModes::EM_Landscape);

	FEdModeLandscape* EdMode = (FEdModeLandscape*)GEditorModeTools().GetActiveMode(FBuiltinEditorModes::EM_Landscape);

	EdMode->UISettings->NewLandscape_Location = GetActorLocation();
	EdMode->UISettings->NewLandscape_Rotation = GetActorRotation();

	EdMode->SetCurrentTool("ToolSet_NewLandscape");

	return bResult;
}

void ALandscapePlaceholder::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (!HasAnyFlags(RF_Transient))
	{
		Destroy();
	}
}

#undef LOCTEXT_NAMESPACE
