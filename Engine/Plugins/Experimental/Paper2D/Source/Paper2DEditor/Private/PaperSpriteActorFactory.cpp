// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "AssetData.h"

//////////////////////////////////////////////////////////////////////////
// UPaperSpriteActorFactory

UPaperSpriteActorFactory::UPaperSpriteActorFactory(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	DisplayName = NSLOCTEXT("Paper2D", "PaperSpriteFactoryDisplayName", "Add Sprite");
	NewActorClass = APaperSpriteActor::StaticClass();
}

void UPaperSpriteActorFactory::PostSpawnActor(UObject* Asset, AActor* NewActor)
{
	if (UPaperSprite* Sprite = Cast<UPaperSprite>(Asset))
	{
		GEditor->SetActorLabelUnique(NewActor, Sprite->GetName());

		APaperSpriteActor* TypedActor = CastChecked<APaperSpriteActor>(NewActor);
		UPaperSpriteComponent* RenderComponent = TypedActor->RenderComponent;
		check(RenderComponent);

		RenderComponent->UnregisterComponent();
		RenderComponent->SetSprite(Sprite);
		RenderComponent->RegisterComponent();
	}
}

void UPaperSpriteActorFactory::PostCreateBlueprint(UObject* Asset, AActor* CDO)
{
	checkf(false, TEXT("APaperSpriteActor isn't blueprintable; how did you get here?"));
}

bool UPaperSpriteActorFactory::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg)
{
	if (AssetData.IsValid() && AssetData.GetClass()->IsChildOf(UPaperSprite::StaticClass()))
	{
		return true;
	}
	else
	{
		OutErrorMsg = NSLOCTEXT("Paper2D", "CanCreateActorFrom_NoSprite", "No sprite was specified.");
		return false;
	}
}
