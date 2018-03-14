// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "MergeProxyUtils/Utils.h"

#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Input/SCheckBox.h"
#include "Engine/Selection.h"
#include "Editor.h"
#include "Components/ChildActorComponent.h"
#include "Components/ShapeComponent.h"


#define LOCTEXT_NAMESPACE "MergeProxyDialog"

void FComponentSelectionControl::UpdateSelectedCompnentsAndListBox()
{
	StoreCheckBoxState();
	UpdateSelectedStaticMeshComponents();
	ComponentsListView->ClearSelection();
	ComponentsListView->RequestListRefresh();
}

void FComponentSelectionControl::StoreCheckBoxState() 
{
	StoredCheckBoxStates.Empty();

	// Loop over selected mesh component and store its checkbox state
	for (TSharedPtr<FMergeComponentData> SelectedComponent : SelectedComponents)
	{
		UPrimitiveComponent* PrimComponent = SelectedComponent->PrimComponent.Get();
		const ECheckBoxState State = SelectedComponent->bShouldIncorporate ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		StoredCheckBoxStates.Add(PrimComponent, State);
	}
}

void FComponentSelectionControl::UpdateSelectedStaticMeshComponents()
{
	NumSelectedMeshComponents = 0;

	// Retrieve selected actors
	USelection* SelectedActors = GEditor->GetSelectedActors();
	TArray<AActor*> Actors;
	TArray<ULevel*> UniqueLevels;
	for (FSelectionIterator Iter(*SelectedActors); Iter; ++Iter)
	{
		AActor* Actor = Cast<AActor>(*Iter);
		if (Actor)
		{
			Actors.Add(Actor);
			UniqueLevels.AddUnique(Actor->GetLevel());
		}
	}

	// Retrieve static mesh components from selected actors
	SelectedComponents.Empty();
	for (int32 ActorIndex = 0; ActorIndex < Actors.Num(); ++ActorIndex)
	{
		AActor* Actor = Actors[ActorIndex];
		check(Actor != nullptr);

		TArray<UChildActorComponent*> ChildActorComponents;
		Actor->GetComponents<UChildActorComponent>(ChildActorComponents);
		for (UChildActorComponent* ChildComponent : ChildActorComponents)
		{
			// Push actor at the back of array so we will process it
			AActor* ChildActor = ChildComponent->GetChildActor();
			if (ChildActor)
			{
				Actors.Add(ChildActor);
			}
		}

		TArray<UPrimitiveComponent*> PrimComponents;
		Actor->GetComponents<UPrimitiveComponent>(PrimComponents);
		for (UPrimitiveComponent* PrimComponent : PrimComponents)
		{
			bool bInclude = false; // Should put into UI list
			bool bShouldIncorporate = false; // Should default to part of merged mesh
			bool bIsMesh = false;
			if (UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(PrimComponent))
			{
				bShouldIncorporate = (StaticMeshComponent->GetStaticMesh() != nullptr);
				bInclude = true;
				bIsMesh = true;
			}
			else if (UShapeComponent* ShapeComponent = Cast<UShapeComponent>(PrimComponent))
			{
				bShouldIncorporate = true;
				bInclude = true;
			}

			if (bInclude)
			{
				SelectedComponents.Add(TSharedPtr<FMergeComponentData>(new FMergeComponentData(PrimComponent)));
				TSharedPtr<FMergeComponentData>& ComponentData = SelectedComponents.Last();

				ComponentData->bShouldIncorporate = bShouldIncorporate;

				// See if we stored a checkbox state for this mesh component, and set accordingly
				ECheckBoxState* StoredState = StoredCheckBoxStates.Find(PrimComponent);
				if (StoredState != nullptr)
				{
					ComponentData->bShouldIncorporate = (*StoredState == ECheckBoxState::Checked);
				}

				// Keep count of selected meshes
				if (ComponentData->bShouldIncorporate && bIsMesh)
				{
					NumSelectedMeshComponents++;
				}
			}

		}
	}
}


TSharedRef<ITableRow> FComponentSelectionControl::MakeComponentListItemWidget(TSharedPtr<FMergeComponentData> ComponentData, const TSharedRef<STableViewBase>& OwnerTable)
{
	check(ComponentData->PrimComponent != nullptr);

	// Retrieve information about the mesh component
	const FString OwningActorName = ComponentData->PrimComponent->GetOwner()->GetName();

	// If box should be enabled
	bool bEnabled = true;
	bool bIsMesh = false;

	FString ComponentInfo;
	if (UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(ComponentData->PrimComponent.Get()))
	{
		ComponentInfo = (StaticMeshComponent->GetStaticMesh() != nullptr) ? StaticMeshComponent->GetStaticMesh()->GetName() : TEXT("No Static Mesh Available");
		bEnabled = (StaticMeshComponent->GetStaticMesh() != nullptr);
		bIsMesh = true;
	}
	else if (UShapeComponent* ShapeComponent = Cast<UShapeComponent>(ComponentData->PrimComponent.Get()))
	{
		ComponentInfo = ShapeComponent->GetClass()->GetName();
	}

	const FString ComponentName = ComponentData->PrimComponent->GetName();

	// See if we stored a checkbox state for this mesh component, and set accordingly
	ECheckBoxState State = bEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	ECheckBoxState* StoredState = StoredCheckBoxStates.Find(ComponentData->PrimComponent.Get());
	if (StoredState)
	{
		State = *StoredState;
	}


	return SNew(STableRow<TSharedPtr<FMergeComponentData>>, OwnerTable)
		[
			SNew(SBox)
			[
				// Disable UI element if this static mesh component has invalid static mesh data
				SNew(SHorizontalBox)
				.IsEnabled(bEnabled)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SCheckBox)
					.IsChecked(State)
					.ToolTipText(LOCTEXT("IncorporateCheckBoxToolTip", "When ticked the Component will be incorporated into the merge"))

					.OnCheckStateChanged_Lambda([=](ECheckBoxState NewState)
					{
						ComponentData->bShouldIncorporate = (NewState == ECheckBoxState::Checked);

						if (bIsMesh)
						{
							this->NumSelectedMeshComponents += (NewState == ECheckBoxState::Checked) ? 1 : -1;
						}
					})
				]

				+ SHorizontalBox::Slot()
				.Padding(5.0, 0, 0, 0)
				.AutoWidth()
				[
					SNew(STextBlock)
					.Text(FText::FromString(OwningActorName + " - " + ComponentInfo + " - " + ComponentName))
				]
			]
		];

}

#undef LOCTEXT_NAMESPACE