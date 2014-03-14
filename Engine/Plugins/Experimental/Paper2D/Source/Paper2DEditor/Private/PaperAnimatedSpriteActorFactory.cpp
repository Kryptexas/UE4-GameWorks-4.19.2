// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "AssetData.h"

//////////////////////////////////////////////////////////////////////////
// UPaperFlipbookActorFactory

UPaperFlipbookActorFactory::UPaperFlipbookActorFactory(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	DisplayName = NSLOCTEXT("Paper2D", "PaperFlipbookFactoryDisplayName", "Add Animated Sprite");
	NewActorClass = APaperFlipbookActor::StaticClass();
}

void UPaperFlipbookActorFactory::PostSpawnActor(UObject* Asset, AActor* NewActor)
{
	if (UPaperFlipbook* Flipbook = Cast<UPaperFlipbook>(Asset))
	{
		GEditor->SetActorLabelUnique(NewActor, Flipbook->GetName());

		APaperFlipbookActor* TypedActor = CastChecked<APaperFlipbookActor>(NewActor);
		UPaperAnimatedRenderComponent* RenderComponent = TypedActor->RenderComponent;
		check(RenderComponent);

		RenderComponent->UnregisterComponent();
		RenderComponent->SetFlipbook(Flipbook);
		RenderComponent->RegisterComponent();
	}
}

void UPaperFlipbookActorFactory::PostCreateBlueprint(UObject* Asset, AActor* CDO)
{
	checkf(false, TEXT("APaperFlipbookActor isn't blueprintable; how did you get here?"));
}

bool UPaperFlipbookActorFactory::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg)
{
	if (AssetData.IsValid() && AssetData.GetClass()->IsChildOf(UPaperFlipbook::StaticClass()))
	{
		return true;
	}
	else
	{
		OutErrorMsg = NSLOCTEXT("Paper2D", "CanCreateActorFrom_NoFlipbook", "No flipbook was specified.");
		return false;
	}
}
