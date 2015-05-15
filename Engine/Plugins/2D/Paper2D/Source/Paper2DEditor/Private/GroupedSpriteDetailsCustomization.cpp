// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "GroupedSpriteDetailsCustomization.h"

#include "PaperSpriteActor.h"
#include "PaperSpriteComponent.h"
#include "PaperGroupedSpriteComponent.h"
#include "ScopedTransaction.h"
#include "ILayers.h"
#include "ComponentReregisterContext.h"

#define LOCTEXT_NAMESPACE "SpriteEditor"

//////////////////////////////////////////////////////////////////////////
// FGroupedSpriteComponentDetailsCustomization

TSharedRef<IDetailCustomization> FGroupedSpriteComponentDetailsCustomization::MakeInstance()
{
	return MakeShareable(new FGroupedSpriteComponentDetailsCustomization());
}

FGroupedSpriteComponentDetailsCustomization::FGroupedSpriteComponentDetailsCustomization()
{
}

void FGroupedSpriteComponentDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	// Create a category so this is displayed early in the properties
	IDetailCategoryBuilder& SpriteCategory = DetailBuilder.EditCategory("Sprite", FText::GetEmpty(), ECategoryPriority::Important);

	ObjectsBeingCustomized.Empty();
	DetailBuilder.GetObjectsBeingCustomized(/*out*/ ObjectsBeingCustomized);

	{
		// Expose split buttons
		FDetailWidgetRow& SplitRow = SpriteCategory.AddCustomRow(LOCTEXT("SplitSearchText", "Split"))
			.WholeRowContent()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f, 0.0f)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Left)
				[
					SNew(SButton)
					.Text(LOCTEXT("SplitSprites", "Split Sprites"))
					.ToolTipText(LOCTEXT("SplitSprites_Tooltip", "Splits all sprite instances into separate sprite actors or components"))
					.OnClicked(this, &FGroupedSpriteComponentDetailsCustomization::SplitSprites)
				]
			];
	}
}

void FGroupedSpriteComponentDetailsCustomization::BuildHarvestList(const TArray<TWeakObjectPtr<UObject>>& ObjectsToConsider, TSubclassOf<UActorComponent> HarvestClassType, TArray<UActorComponent*>& OutComponentsToHarvest, TArray<AActor*>& OutActorsToDelete)
{
	// Determine the components to harvest
	for (TWeakObjectPtr<UObject> WeakObject : ObjectsToConsider)
	{
		UObject* Object = WeakObject.Get();
		if (Object == nullptr)
		{
			continue;
		}

		if (AActor* SelectedActor = Cast<AActor>(Object))
		{
			TArray<UActorComponent*> Components;
			SelectedActor->GetComponents(Components);

			bool bHadHarvestableComponents = false;
			for (UActorComponent* Component : Components)
			{
				if (Component->IsA(HarvestClassType) && !Component->IsEditorOnly())
				{
					bHadHarvestableComponents = true;
					OutComponentsToHarvest.Add(Component);
				}
			}

			if (bHadHarvestableComponents)
			{
				OutActorsToDelete.Add(SelectedActor);
			}
		}
		else if (UActorComponent* SelectedComponent = Cast<UActorComponent>(Object))
		{
			if (SelectedComponent->IsA(HarvestClassType) && !SelectedComponent->IsEditorOnly())
			{
				OutComponentsToHarvest.Add(SelectedComponent);
				if (AActor* SelectedComponentOwner = SelectedComponent->GetOwner())
				{
					OutActorsToDelete.AddUnique(SelectedComponentOwner);
				}
			}
		}
	}
}

FReply FGroupedSpriteComponentDetailsCustomization::SplitSprites()
{
	TArray<UActorComponent*> ComponentsToHarvest;
	TSubclassOf<UActorComponent> HarvestClassType = UPaperSpriteComponent::StaticClass();
	TArray<AActor*> ActorsToDelete;
	TArray<AActor*> ActorsCreated;
	
	FGroupedSpriteComponentDetailsCustomization::BuildHarvestList(ObjectsBeingCustomized, UPaperGroupedSpriteComponent::StaticClass(), /*out*/ ComponentsToHarvest, /*out*/ ActorsToDelete);

	if (ComponentsToHarvest.Num() > 0)
	{
		if (UWorld* World = ComponentsToHarvest[0]->GetWorld())
		{
			const FScopedTransaction Transaction(LOCTEXT("SplitSprites", "Split sprite instances"));

			// Create an instance from each item of each batch component that we're harvesting
			for (UActorComponent* SourceComponent : ComponentsToHarvest)
			{
				UPaperGroupedSpriteComponent* SourceBatchComponent = CastChecked<UPaperGroupedSpriteComponent>(SourceComponent);

				for (FSpriteInstanceData& InstanceData : SourceBatchComponent->PerInstanceSpriteData)
				{
					if (InstanceData.SourceSprite != nullptr)
					{
						const FTransform InstanceTransform(FTransform(InstanceData.Transform) * SourceBatchComponent->ComponentToWorld);

						FActorSpawnParameters SpawnParams;
						SpawnParams.bDeferConstruction = true;
						if (APaperSpriteActor* SpawnedActor = World->SpawnActor<APaperSpriteActor>(SpawnParams))
						{
							UPaperSpriteComponent* SpawnedSpriteComponent = SpawnedActor->GetRenderComponent();

							{
								FComponentReregisterContext ReregisterContext(SpawnedSpriteComponent);

								SpawnedSpriteComponent->Modify();
								SpawnedSpriteComponent->SetSpriteColor(InstanceData.VertexColor);
								SpawnedSpriteComponent->SetSprite(InstanceData.SourceSprite);

								// Apply the material override if there is one
								UMaterialInterface* InstanceMaterial = SourceBatchComponent->GetMaterial(InstanceData.MaterialIndex);
								if (InstanceMaterial != InstanceData.SourceSprite->GetMaterial(0))
								{
									SpawnedSpriteComponent->SetMaterial(0, InstanceMaterial);
								}
							}

							UGameplayStatics::FinishSpawningActor(SpawnedActor, InstanceTransform);
							ActorsCreated.Add(SpawnedActor);
						}
					}
				}
			}

			// Delete the existing actor instances
			for (AActor* ActorToDelete : ActorsToDelete)
			{
				// Remove from active selection in editor
				GEditor->SelectActor(ActorToDelete, /*bSelected=*/ false, /*bNotify=*/ false);
				GEditor->Layers->DisassociateActorFromLayers(ActorToDelete);
				World->EditorDestroyActor(ActorToDelete, /*bShouldModifyLevel=*/ true);
			}

			// Select the new replacements instance
			GEditor->SelectNone(/*bNoteSelectionChange=*/ false, false, false);
			for (AActor* SpawnedActor : ActorsCreated)
			{
				GEditor->SelectActor(SpawnedActor, /*bSelected=*/ true, /*bNotify=*/ true);
			}
			GEditor->NoteSelectionChange();
		}
		else
		{
			// We're in the BP editor and don't currently support splitting here!
			UE_LOG(LogPaper2DEditor, Warning, TEXT("Splitting sprites in the Blueprint editor is not currently supported"));
		}
	}

	return FReply::Handled();
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
