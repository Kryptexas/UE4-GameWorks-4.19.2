// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "LevelEditor.h"
#include "SActorDetails.h"
#include "SSCSEditor.h"
#include "ComponentEditorUtils.h"
#include "Editor/PropertyEditor/Public/PropertyEditing.h"
#include "LevelEditorGenericDetails.h"
#include "ScopedTransaction.h"

void SActorDetails::Construct(const FArguments& InArgs, const FName TabIdentifier)
{
	struct Local
	{
		static bool IsPropertyVisible(const FPropertyAndParent& PropertyAndParent)
		{
			// For details views in the level editor all properties are the instanced versions
			if(PropertyAndParent.Property.HasAllPropertyFlags(CPF_DisableEditOnInstance))
			{
				return false;
			}

			return true;
		}
	};

	bSelectionGuard = false;
	bShowingRootActorNodeSelected = false;

	// Event subscriptions
	USelection::SelectionChangedEvent.AddRaw(this, &SActorDetails::OnEditorSelectionChanged);

	FPropertyEditorModule& PropPlugin = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bUpdatesFromSelection = true;
	DetailsViewArgs.bLockable = true;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::ComponentsAndActorsUseNameArea;
	DetailsViewArgs.NotifyHook = GUnrealEd;
	DetailsViewArgs.ViewIdentifier = TabIdentifier;
	DetailsViewArgs.bCustomNameAreaLocation = true;
	DetailsViewArgs.bCustomFilterAreaLocation = true;

	DetailsView = PropPlugin.CreateDetailView(DetailsViewArgs);

	DetailsView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateStatic(&Local::IsPropertyVisible));

	auto IsPropertyEditingEnabled = [&]()
	{
		bool bIsEditable = true;
		const TArray<TWeakObjectPtr<UObject> >& Objects = DetailsView->GetSelectedObjects();
		for (auto Object : Objects)
		{
			UActorComponent* ActorComp = Cast<UActorComponent>( Object.Get() );
			if (ActorComp)
			{
				bIsEditable = ActorComp->CreationMethod != EComponentCreationMethod::ConstructionScript;
				if( !bIsEditable )
				{
					break;
				}
			}
		}
		return bIsEditable;
	};

	DetailsView->SetIsPropertyEditingEnabledDelegate(FIsPropertyEditingEnabled::CreateLambda(IsPropertyEditingEnabled));

	// Set up a delegate to call to add generic details to the view
	DetailsView->SetGenericLayoutDetailsDelegate(FOnGetDetailCustomizationInstance::CreateStatic(&FLevelEditorGenericDetails::MakeInstance));

	GEditor->RegisterForUndo(this);

	SAssignNew(ComponentsBox, SBox)
		.Visibility(EVisibility::Collapsed);

	ComponentsBox->SetContent
	(
		SAssignNew(SCSEditor, SSCSEditor)
		.EditorMode(EComponentEditorMode::ActorInstance)
		.ActorContext(this, &SActorDetails::GetSelectedActor)												// Get the instance of the actor in the world
		.OnSelectionUpdated(this, &SActorDetails::OnSCSEditorTreeViewSelectionChanged)						// A selection has been made in the tree view, so inform the level editor
		//.OnHighlightPropertyInDetailsView(this, &SLevelEditor::OnSCSEditorHighlightPropertyInDetailsView)	// Also unsure and don't think it's needed
	);

	ChildSlot
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.Padding(0.0f, 0.0f, 0.0f, 2.0f)
		.AutoHeight()
		[
			DetailsView->GetNameAreaWidget().ToSharedRef()
		]
		+ SVerticalBox::Slot()
		.Padding(0.0f, 0.0f, 0.0f, 2.0f)
		.AutoHeight()
		[
			DetailsView->GetFilterAreaWidget().ToSharedRef()
		]
		+SVerticalBox::Slot()
		[
			SAssignNew(DetailsSplitter, SSplitter)
			.Orientation(Orient_Vertical)
			+ SSplitter::Slot()
			[
				DetailsView.ToSharedRef()
			]
		]
	];

	DetailsSplitter->AddSlot(0)
	.Value(.2f)
	[
		ComponentsBox.ToSharedRef()
	];
}

SActorDetails::~SActorDetails()
{
	USelection::SelectionChangedEvent.RemoveAll(this);
}

void SActorDetails::SetObjects(const TArray<UObject*>& InObjects)
{
	if(!DetailsView->IsLocked())
	{
		DetailsView->SetObjects(InObjects);

		bool bShowingComponents = false;

		if(InObjects.Num() == 1 && FKismetEditorUtilities::CanCreateBlueprintOfClass(InObjects[0]->GetClass()) && GetDefault<UEditorExperimentalSettings>()->bInWorldBPEditing)
		{
			auto Actor = GetSelectedActor();
			if(Actor)
			{
				bShowingComponents = true;

				// Update the tree if a new actor is selected
				if(GEditor->GetSelectedComponentCount() == 0)
				{
					SCSEditor->UpdateTree();
				}
			}
		}

		ComponentsBox->SetVisibility(bShowingComponents ? EVisibility::Visible : EVisibility::Collapsed);
	}
}

void SActorDetails::PostUndo(bool bSuccess)
{
	// Enable the selection guard to prevent OnTreeSelectionChanged() from altering the editor's component selection
	TGuardValue<bool> SelectionGuard(bSelectionGuard, true);

	// Refresh the tree and update the selection to match the world
	SCSEditor->UpdateTree();
	UpdateComponentTreeFromEditorSelection();
}

void SActorDetails::PostRedo(bool bSuccess)
{
	PostUndo(bSuccess);
}

void SActorDetails::OnEditorSelectionChanged(UObject* Object)
{
	if(!bSelectionGuard && SCSEditor.IsValid())
	{
		// Make sure the selection set that changed is relevant to us
		auto Selection = Cast<USelection>(Object);
		if(Selection == GEditor->GetSelectedComponents() || Selection == GEditor->GetSelectedActors())
		{
			UpdateComponentTreeFromEditorSelection();

			if(GEditor->GetSelectedComponentCount() == 0) // An actor was selected
			{
				// Ensure the selection flags are up to date for the components in the selected actor
				for(FSelectionIterator It(GEditor->GetSelectedActorIterator()); It; ++It)
				{
					auto Actor = CastChecked<AActor>(*It);
					GUnrealEd->SetActorSelectionFlags(Actor);
				}
			}
		}
	}
}

AActor* SActorDetails::GetSelectedActor() const
{
	//@todo this won't work w/ multi-select
	return Cast<AActor>(*GEditor->GetSelectedActorIterator());
}

void SActorDetails::OnSCSEditorRootSelected(AActor* Actor)
{
	if(!bSelectionGuard)
	{
		GEditor->SelectNone(true, true, false);
		GEditor->SelectActor(Actor, true, true, true);
	}
}

void SActorDetails::OnSCSEditorTreeViewSelectionChanged(const TArray<FSCSEditorTreeNodePtrType>& SelectedNodes)
{
	if (!bSelectionGuard && SelectedNodes.Num() > 0)
	{
		auto Actor = GetSelectedActor();
		if (Actor)
		{
			USelection* SelectedComponents = GEditor->GetSelectedComponents();
			TArray<UObject*> DetailsObjects;

			// Determine if the root actor node is among the selected nodes
			bool bActorNodeSelected = false;
			for (auto& SelectedNode : SelectedNodes)
			{
				if (SelectedNode.IsValid() && SelectedNode->GetNodeType() == FSCSEditorTreeNode::RootActorNode)
				{
					bActorNodeSelected = true;
					break;
				}
			}

			// Determine if the selected non-root actor nodes differ from the editor component selection
			bool bComponentSelectionChanged = GEditor->GetSelectedComponentCount() != SelectedNodes.Num() - ( bActorNodeSelected ? 1 : 0 );
			if (!bComponentSelectionChanged)
			{
				// Check to see if any of the selected nodes aren't already selected in the world
				for (auto& SelectedNode : SelectedNodes)
				{
					if (SelectedNode.IsValid() && SelectedNode->GetNodeType() != FSCSEditorTreeNode::RootActorNode)
					{
						UActorComponent* ComponentInstance = SelectedNode->FindComponentInstanceInActor(Actor);
						if (ComponentInstance && !SelectedComponents->IsSelected(ComponentInstance))
						{
							bComponentSelectionChanged = true;
							break;
						}
					}
				}
			}

			// Does the actor selection differ from our previous state?
			const bool bActorSelectionChanged = bShowingRootActorNodeSelected != bActorNodeSelected;

			// If necessary, update the editor component selection
			if (bActorSelectionChanged || ( bComponentSelectionChanged && !bActorNodeSelected ))
			{
				// Store whether we're now showing the actor root as selected
				bShowingRootActorNodeSelected = bActorNodeSelected;

				// Enable the selection guard to prevent OnEditorSelectionChanged() from altering the contents of the SCSTreeWidget
				TGuardValue<bool> SelectionGuard(bSelectionGuard, true);

				// Update the editor's component selection to match the node selection

				// Note: this transaction should not take place if we are in the middle of executing an undo or redo because it would clear the top of the transaction stack. 
				const bool bShouldActuallyTransact = !GIsTransacting;
				const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "ClickingOnComponentInTree", "Clicking on Component (tree view)"), bShouldActuallyTransact );

				SelectedComponents->Modify();
				SelectedComponents->BeginBatchSelectOperation();
				SelectedComponents->DeselectAll();

				// If the actor root is selected, then the editor component selection should remain empty and we only show the Actor's details
				if (bShowingRootActorNodeSelected)
				{
					DetailsObjects.Add(Actor);
				}
				else
				{
					for (auto& SelectedNode : SelectedNodes)
					{
						if (SelectedNode.IsValid())
						{
							UActorComponent* ComponentInstance = SelectedNode->FindComponentInstanceInActor(Actor);
							if (ComponentInstance)
							{
								DetailsObjects.Add(ComponentInstance);
								SelectedComponents->Select(ComponentInstance);

								// Ensure the selection override is bound for this component (including any attached editor-only children)
								auto PrimComponent = Cast<UPrimitiveComponent>(ComponentInstance);
								if (PrimComponent && !PrimComponent->SelectionOverrideDelegate.IsBound())
								{
									PrimComponent->Modify();
									PrimComponent->SelectionOverrideDelegate = UPrimitiveComponent::FSelectionOverride::CreateUObject(GUnrealEd, &UUnrealEdEngine::IsComponentSelected);
								}
								else
								{
									//@todo move the selection override binding check to FComponentEditorUtils
									auto SceneComponent = Cast<USceneComponent>(ComponentInstance);
									if (SceneComponent)
									{
										for (auto Component : SceneComponent->AttachChildren)
										{
											PrimComponent = Cast<UPrimitiveComponent>(Component);
											if (PrimComponent && !PrimComponent->SelectionOverrideDelegate.IsBound())
											{
												PrimComponent->Modify();
												PrimComponent->SelectionOverrideDelegate = UPrimitiveComponent::FSelectionOverride::CreateUObject(GUnrealEd, &UUnrealEdEngine::IsComponentSelected);
											}
										}
									}
								}
							}
						}
					}
				}
				
				SelectedComponents->EndBatchSelectOperation();

				DetailsView->SetObjects(DetailsObjects);

				GUnrealEd->SetActorSelectionFlags(Actor);
				GUnrealEd->UpdatePivotLocationForSelection(true);
				GEditor->RedrawLevelEditingViewports();
			}
		}
	}
}

void SActorDetails::UpdateComponentTreeFromEditorSelection()
{
	// Enable the selection guard to prevent OnTreeSelectionChanged() from altering the editor's component selection
	TGuardValue<bool> SelectionGuard(bSelectionGuard, true);

	auto& SCSTreeWidget = SCSEditor->SCSTreeWidget;
	TArray<UObject*> DetailsObjects;

	// Update the tree selection to match the level editor component selection
	SCSTreeWidget->ClearSelection();
	for(FSelectionIterator It(GEditor->GetSelectedComponentIterator()); It; ++It)
	{
		UActorComponent* Component = CastChecked<UActorComponent>(*It);

		auto SCSTreeNode = SCSEditor->GetNodeFromActorComponent(Component, false);
		if(SCSTreeNode.IsValid() && SCSTreeNode->GetComponentTemplate())
		{
			SCSTreeWidget->RequestScrollIntoView(SCSTreeNode);
			SCSTreeWidget->SetItemSelection(SCSTreeNode, true);

			auto ComponentTemplate = SCSTreeNode->GetComponentTemplate();
			check(Component == ComponentTemplate);
			DetailsObjects.Add(Component);
		}
	}

	if(DetailsObjects.Num() > 0)
	{
		DetailsView->SetObjects(DetailsObjects);
	}
	else
	{
		SCSEditor->SelectRoot();
	}
}
