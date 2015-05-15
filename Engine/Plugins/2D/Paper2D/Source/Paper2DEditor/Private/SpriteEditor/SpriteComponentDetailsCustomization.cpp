// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "PaperSpriteComponent.h"
#include "PaperGroupedSpriteActor.h"
#include "SpriteComponentDetailsCustomization.h"
#include "ScopedTransaction.h"
#include "ILayers.h"
#include "GroupedSpriteDetailsCustomization.h"
#include "ComponentReregisterContext.h"

#define LOCTEXT_NAMESPACE "SpriteEditor"

//////////////////////////////////////////////////////////////////////////
// FSpriteComponentDetailsCustomization

TSharedRef<IDetailCustomization> FSpriteComponentDetailsCustomization::MakeInstance()
{
	return MakeShareable(new FSpriteComponentDetailsCustomization);
}

void FSpriteComponentDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	// Create a category so this is displayed early in the properties
	IDetailCategoryBuilder& SpriteCategory = DetailBuilder.EditCategory("Sprite", FText::GetEmpty(), ECategoryPriority::Important);

	ObjectsBeingCustomized.Empty();
	DetailBuilder.GetObjectsBeingCustomized(/*out*/ ObjectsBeingCustomized);

	if (ObjectsBeingCustomized.Num() > 1)
	{
		// Expose merge buttons
		FDetailWidgetRow& MergeRow = SpriteCategory.AddCustomRow(LOCTEXT("MergeSearchText", "Merge"))
			.WholeRowContent()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f, 0.0f)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Left)
				[
					SNew(SButton)
					.Text(LOCTEXT("MergeSprites", "Merge Sprites"))
					.ToolTipText(LOCTEXT("MergeSprites_Tooltip", "Merges all selected sprites into entries on a single InstancedSpriteComponent"))
					.OnClicked(this, &FSpriteComponentDetailsCustomization::MergeSprites)
				]
			];
	}
}


FBox FSpriteComponentDetailsCustomization::ComputeBoundsForComponents(const TArray<UActorComponent*>& ComponentList)
{
	FBox BoundsOfList(ForceInit);

	for (const UActorComponent* Component : ComponentList)
	{
		if (const USceneComponent* SceneComponent = Cast<const USceneComponent>(Component))
		{
			BoundsOfList += SceneComponent->Bounds.GetBox();
		}
	}

	return BoundsOfList;
}

FReply FSpriteComponentDetailsCustomization::MergeSprites()
{
	TArray<UActorComponent*> ComponentsToHarvest;
	TSubclassOf<UActorComponent> HarvestClassType = UPaperSpriteComponent::StaticClass();
	TArray<AActor*> ActorsToDelete;

	FGroupedSpriteComponentDetailsCustomization::BuildHarvestList(ObjectsBeingCustomized, UPaperSpriteComponent::StaticClass(), /*out*/ ComponentsToHarvest, /*out*/ ActorsToDelete);

	if (ComponentsToHarvest.Num() > 0)
	{
		if (UWorld* World = ComponentsToHarvest[0]->GetWorld())
		{
			const FBox ComponentBounds = ComputeBoundsForComponents(ComponentsToHarvest);
			const FTransform MergedWorldTM(ComponentBounds.GetCenter());

			FActorSpawnParameters SpawnParams;
			SpawnParams.bDeferConstruction = true;
			if (APaperGroupedSpriteActor* SpawnedActor = World->SpawnActor<APaperGroupedSpriteActor>(SpawnParams))
			{
				const FScopedTransaction Transaction(LOCTEXT("MergeSprites", "Merge sprite instances"));

				// Create a merged sprite component
				{
					UPaperGroupedSpriteComponent* MergedSpriteComponent = SpawnedActor->GetRenderComponent();
					FComponentReregisterContext ReregisterContext(MergedSpriteComponent);

					// Create an instance from each sprite component that we're harvesting
					for (UActorComponent* SourceComponent : ComponentsToHarvest)
					{
						UPaperSpriteComponent* SourceSpriteComponent = CastChecked<UPaperSpriteComponent>(SourceComponent);

						UPaperSprite* Sprite = SourceSpriteComponent->GetSprite();
						const FLinearColor SpriteColor = SourceSpriteComponent->GetSpriteColor();
						const FTransform RelativeSpriteTransform = SourceSpriteComponent->GetComponentTransform().GetRelativeTransform(MergedWorldTM);

						MergedSpriteComponent->AddInstance(RelativeSpriteTransform, Sprite, /*bWorldSpace=*/ false, SpriteColor);
					}
				}

				// Finalize the new component
				UGameplayStatics::FinishSpawningActor(SpawnedActor, MergedWorldTM);

				// Delete the existing actor instances
				for (AActor* ActorToDelete : ActorsToDelete)
				{
					// Remove from active selection in editor
					GEditor->SelectActor(ActorToDelete, /*bSelected=*/ false, /*bNotify=*/ false);
					GEditor->Layers->DisassociateActorFromLayers(ActorToDelete);
					World->EditorDestroyActor(ActorToDelete, /*bShouldModifyLevel=*/ true);
				}

				// Select the new replacement instance
				GEditor->SelectActor(SpawnedActor, /*bSelected=*/ true, /*bNotify=*/ true);
			}
			else
			{
				// We're in the BP editor and don't currently support merging here!
				UE_LOG(LogPaper2DEditor, Warning, TEXT("Merging sprites in the Blueprint editor is not currently supported"));
			}
		}
	}

	return FReply::Handled();
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
