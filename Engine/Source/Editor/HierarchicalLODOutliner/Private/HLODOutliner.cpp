// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "HierarchicalLODOutlinerPrivatePCH.h"
#include "HLODOutliner.h"

#include "Engine.h"
#include "Engine/LODActor.h"
#include "Engine/World.h"
#include "HierarchicalLOD.h"
#include "HierarchicalLODVolume.h"

#include "Editor.h"
#include "EditorModeManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "SListView.h"
#include "ScopedTransaction.h"
#include "BSPOps.h"

#include "ITreeItem.h"
#include "LODActorItem.h"
#include "LODLevelItem.h"
#include "StaticMeshActorItem.h"
#include "HLODTreeWidgetItem.h"
#include "HLODSelectionActor.h"
#include "TreeItemID.h"

#include "HierarchicalLODUtils.h"


#define LOCTEXT_NAMESPACE "HLODOutliner"

namespace HLODOutliner
{
	SHLODOutliner::SHLODOutliner()
	{
		bNeedsRefresh = true;
		CurrentWorld = nullptr;
		ForcedLODLevel = -1;
	}

	SHLODOutliner::~SHLODOutliner()
	{
		DeregisterDelegates();	
		DestroySelectionActors();
		CurrentWorld = nullptr;
		HLODTreeRoot.Empty();
		SelectedNodes.Empty();		
		AllNodes.Empty();
		SelectionActors.Empty();
		LODLevelBuildFlags.Empty();
		LODLevelActors.Empty();
		PendingActions.Empty();
	}

	BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
		void SHLODOutliner::Construct(const FArguments& InArgs)
	{
		CreateSettingsView();

		/** Holds all widgets for the profiler window like menu bar, toolbar and tabs. */
		TSharedRef<SVerticalBox> MainContentPanel = SNew(SVerticalBox);
		ChildSlot
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
				[
					SNew(SOverlay)

					// Overlay slot for the main HLOD window area
					+ SOverlay::Slot()
					[
						MainContentPanel
					]
				]
			];

		MainContentPanel->AddSlot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 4.0f)
			[
				CreateButtonWidgets()
			];

		MainContentPanel->AddSlot()
			.FillHeight(1.0f)
			[
				SNew(SSplitter)
				.Orientation(Orient_Vertical)

				+ SSplitter::Slot()
				.Value(0.5)
				[
					CreateTreeviewWidget()
				]
				+ SSplitter::Slot()
				.Value(0.5)
				[
					SettingsView.ToSharedRef()
				]		
			];

		RegisterDelegates();

		// Register to update when an undo/redo operation has been called to update our list of actors
		GEditor->RegisterForUndo(this);	
	}

	TSharedRef<SWidget> SHLODOutliner::CreateButtonWidgets()
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.Padding(FMargin(0.0f, 5.0f))
			[

				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(FMargin(5.0f, 0.0f))
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.Text(LOCTEXT("GenerateClusters", "Generate Clusters"))
					.OnClicked(this, &SHLODOutliner::HandlePreviewHLODs)
				]

				+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(FMargin(5.0f, 0.0f))
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.Text(LOCTEXT("DeleteClusters", "Delete Clusters"))
						.OnClicked(this, &SHLODOutliner::HandleDeleteHLODs)
					]
				+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(FMargin(5.0f, 0.0f))
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.Text(LOCTEXT("BuildMeshes", "Build LOD Meshes for Clusters"))
						.OnClicked(this, &SHLODOutliner::HandleBuildLODActors)
					]	

			];
		
	}

	TSharedRef<SWidget> SHLODOutliner::CreateTreeviewWidget()
	{
		return SAssignNew(TreeView, SHLODTree)
			.ItemHeight(24.0f)
			.TreeItemsSource(&HLODTreeRoot)
			.OnGenerateRow(this, &SHLODOutliner::OnOutlinerGenerateRow)
			.OnGetChildren(this, &SHLODOutliner::OnOutlinerGetChildren)
			.OnSelectionChanged(this, &SHLODOutliner::OnOutlinerSelectionChanged)
			.OnMouseButtonDoubleClick(this, &SHLODOutliner::OnOutlinerDoubleClick)
			.OnContextMenuOpening(this, &SHLODOutliner::OnOpenContextMenu)
			.OnExpansionChanged(this, &SHLODOutliner::OnItemExpansionChanged)
			.HeaderRow
			(
				SNew(SHeaderRow)
				+ SHeaderRow::Column("SceneActorName")
				.DefaultLabel(LOCTEXT("SceneActorName", "Scene Actor Name"))
				.FillWidth(0.5f)
				+ SHeaderRow::Column("ForceCheckbox")
				.DefaultLabel(LOCTEXT("ForceCheckbox", "Force HLOD"))
				.DefaultTooltip(LOCTEXT("ForceCheckboxTooltip", "Force Hierarchical LOD level visibility"))
				.FillWidth(0.25f)
				+ SHeaderRow::Column("TriangleCount")
				.DefaultLabel(LOCTEXT("TriangleCount", "Number of Triangles"))
				.DefaultTooltip(LOCTEXT("TriangleCountToolTip", "Number of Triangles in a LOD Mesh"))
				.FillWidth(0.25f)				
				);
	}
	void SHLODOutliner::CreateSettingsView()
	{
		// Create a property view
		FPropertyEditorModule& EditModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

		FNotifyHook* NotifyHook = this;
		FDetailsViewArgs DetailsViewArgs(
			/*bUpdateFromSelection=*/ false,
			/*bLockable=*/ false,
			/*bAllowSearch=*/ false,
			FDetailsViewArgs::HideNameArea,
			/*bHideSelectionTip=*/ true,
			/*InNotifyHook=*/ NotifyHook,
			/*InSearchInitialKeyFocus=*/ false,
			/*InViewIdentifier=*/ NAME_None);
		DetailsViewArgs.DefaultsOnlyVisibility = FDetailsViewArgs::EEditDefaultsOnlyNodeVisibility::Automatic;
		DetailsViewArgs.bShowOptions = false;

		SettingsView = EditModule.CreateDetailView(DetailsViewArgs);

		struct Local
		{
			/** Delegate to show all properties */
			static bool IsPropertyVisible(const FPropertyAndParent& PropertyAndParent, bool bInShouldShowNonEditable)
			{
				if (PropertyAndParent.Property.GetName() == "HierarchicalLODSetup" || (PropertyAndParent.ParentProperty && PropertyAndParent.ParentProperty->GetName() == "MergeSetting"))
				{
					return true;
				}
				return false;
			}

		};

		SettingsView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateStatic(&Local::IsPropertyVisible, true));
		SettingsView->SetDisableCustomDetailLayouts(true);
	}

	void SHLODOutliner::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
	{
		// Get a collection of items and folders which were formerly collapsed
		const FParentsExpansionState ExpansionStateInfo = GetParentsExpansionState();

		if (bNeedsRefresh)
		{
			Populate();
		}

		bool bChangeMade = false;

		// Only deal with 256 at a time
		const int32 End = FMath::Min(PendingActions.Num(), 256);
		for (int32 Index = 0; Index < End; ++Index)
		{
			auto& PendingAction = PendingActions[Index];
			switch (PendingAction.Type)
			{
			case FOutlinerAction::AddItem:
				bChangeMade |= AddItemToTree(PendingAction.Item, PendingAction.ParentItem);
				break;

			case FOutlinerAction::MoveItem:
				MoveItemInTree(PendingAction.Item, PendingAction.ParentItem);
				bChangeMade = true;
				break;

			case FOutlinerAction::RemoveItem:
				RemoveItemFromTree(PendingAction.Item);
				bChangeMade = true;
				break;
			default:
				check(false);
				break;
			}
		}
		PendingActions.RemoveAt(0, End);

		// Restore expansion states
		SetParentsExpansionState(ExpansionStateInfo);

		if (bChangeMade)
		{
			TreeView->RequestTreeRefresh();
		}

	}

	void SHLODOutliner::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		SCompoundWidget::OnMouseEnter(MyGeometry, MouseEvent);
	}

	void SHLODOutliner::OnMouseLeave(const FPointerEvent& MouseEvent)
	{
		SCompoundWidget::OnMouseLeave(MouseEvent);
	}

	FReply SHLODOutliner::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
	{
		return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
	}

	FReply SHLODOutliner::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
	{
		return SCompoundWidget::OnDrop(MyGeometry, DragDropEvent);
	}

	FReply SHLODOutliner::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
	{
		return SCompoundWidget::OnDragOver(MyGeometry, DragDropEvent);
	}

	void SHLODOutliner::PostUndo(bool bSuccess)
	{
		FullRefresh();
	}

	FReply SHLODOutliner::HandleBuildHLODs()
	{
		if (CurrentWorld)
		{
			CurrentWorld->HierarchicalLODBuilder->Build();
		}

		FullRefresh();
		return FReply::Handled();
	}

	FReply SHLODOutliner::HandleDeleteHLODs()
	{
		if (CurrentWorld)
		{
			CurrentWorld->HierarchicalLODBuilder->ClearHLODs();
		}
		FullRefresh();
		return FReply::Handled();
	}

	FReply SHLODOutliner::HandlePreviewHLODs()
	{
		if (CurrentWorld)
		{
			CurrentWorld->HierarchicalLODBuilder->PreviewBuild();
		}
		FullRefresh();
		return FReply::Handled();
	}

	FReply SHLODOutliner::HandleDeletePreviewHLODs()
	{
		if (CurrentWorld)
		{
			CurrentWorld->HierarchicalLODBuilder->ClearPreviewBuild();
		}
		FullRefresh();
		return FReply::Handled();
	}

	FReply SHLODOutliner::HandleBuildLODActors()
	{
		if (CurrentWorld)
		{
			DestroySelectionActors();
			CurrentWorld->HierarchicalLODBuilder->BuildMeshesForLODActors();
		}

		FullRefresh();
		return FReply::Handled();
	}

	FReply SHLODOutliner::HandleForceRefresh()
	{
		FullRefresh();

		return FReply::Handled();
	}

	FReply SHLODOutliner::HandleTestFunction()
	{
		if (SelectedNodes.Num() > 0)
		{
			if (SelectedNodes[0]->GetTreeItemType() == ITreeItem::HierarchicalLODActor)
			{
				FLODActorItem* Item = static_cast<FLODActorItem*>(SelectedNodes[0].Get());
				ALODActor* LODActor = Item->LODActor.Get();
				
				FBox BoundingBox = LODActor->GetComponentsBoundingBox(true);

				AHierarchicalLODVolume* Volume = CurrentWorld->SpawnActor<AHierarchicalLODVolume>(AHierarchicalLODVolume::StaticClass(), FTransform(BoundingBox.GetCenter()));
				Volume->BrushBuilder = NewObject<UCubeBuilder>();			

				// this code builds a brush for the new actor
				Volume->PreEditChange(NULL);

				Volume->PolyFlags = 0;
				Volume->Brush = NewObject<UModel>(Volume, NAME_None, RF_Transactional);
				Volume->Brush->Initialize(nullptr, true);
				Volume->Brush->Polys = NewObject<UPolys>(Volume->Brush, NAME_None, RF_Transactional);
				Volume->GetBrushComponent()->Brush = Volume->Brush;
				Volume->BrushBuilder = NewObject<UCubeBuilder>();

				UCubeBuilder* CubeBuilder = static_cast<UCubeBuilder*>(Volume->BrushBuilder);	

				CubeBuilder->X = BoundingBox.GetSize().X * 1.5f;
				CubeBuilder->Y = BoundingBox.GetSize().Y * 1.5f;
				CubeBuilder->Z = BoundingBox.GetSize().Z * 1.5f;

				Volume->BrushBuilder->Build(CurrentWorld, Volume);

				FBSPOps::csgPrepMovingBrush(Volume);

				// Set the texture on all polys to NULL.  This stops invisible textures
				// dependencies from being formed on volumes.
				if (Volume->Brush)
				{
					for (int32 poly = 0; poly < Volume->Brush->Polys->Element.Num(); ++poly)
					{
						FPoly* Poly = &(Volume->Brush->Polys->Element[poly]);
						Poly->Material = NULL;
					}
				}

				Volume->PostEditChange();

				bool check = true;
			}
		}
		return FReply::Handled();
	}

	END_SLATE_FUNCTION_BUILD_OPTIMIZATION

		void SHLODOutliner::RegisterDelegates()
	{
		FEditorDelegates::MapChange.AddSP(this, &SHLODOutliner::OnMapChange);
		FEditorDelegates::NewCurrentLevel.AddSP(this, &SHLODOutliner::OnNewCurrentLevel);
		FWorldDelegates::LevelAddedToWorld.AddSP(this, &SHLODOutliner::OnLevelAdded);
		FWorldDelegates::LevelRemovedFromWorld.AddSP(this, &SHLODOutliner::OnLevelRemoved);
		GEngine->OnLevelActorListChanged().AddSP(this, &SHLODOutliner::FullRefresh);
		GEngine->OnLevelActorAdded().AddSP(this, &SHLODOutliner::OnLevelActorsAdded);
		GEngine->OnLevelActorDeleted().AddSP(this, &SHLODOutliner::OnLevelActorsRemoved);
		GEngine->OnActorMoved().AddSP(this, &SHLODOutliner::OnActorMovedEvent);

		// Register to be notified when properties are edited
		FCoreDelegates::OnActorLabelChanged.AddRaw(this, &SHLODOutliner::OnActorLabelChanged);
		
		// Selection change
		USelection::SelectionChangedEvent.AddRaw(this, &SHLODOutliner::OnLevelSelectionChanged);
		USelection::SelectObjectEvent.AddRaw(this, &SHLODOutliner::OnLevelSelectionChanged);
				
		// HLOD related events
		GEngine->OnHLODActorMoved().AddSP(this, &SHLODOutliner::OnHLODActorMovedEvent);
		GEngine->OnHLODActorAdded().AddSP(this, &SHLODOutliner::OnHLODActorAddedEvent);
		GEngine->OnHLODActorMarkedDirty().AddSP(this, &SHLODOutliner::OnHLODActorMarkedDirtyEvent);
		GEngine->OnHLODDrawDistanceChanged().AddSP(this, &SHLODOutliner::OnHLODDrawDistanceChangedEvent);
		GEngine->OnHLODLevelsArrayChanged().AddSP(this, &SHLODOutliner::OnHLODLevelsArrayChangedEvent);
		
	}

	void SHLODOutliner::DeregisterDelegates()
	{
		FEditorDelegates::MapChange.RemoveAll(this);
		FEditorDelegates::NewCurrentLevel.RemoveAll(this);
		FWorldDelegates::LevelAddedToWorld.RemoveAll(this);
		FWorldDelegates::LevelRemovedFromWorld.RemoveAll(this);
		GEngine->OnLevelActorListChanged().RemoveAll(this);
		GEngine->OnLevelActorAdded().RemoveAll(this);
		GEngine->OnLevelActorDeleted().RemoveAll(this);
		GEngine->OnActorMoved().RemoveAll(this);

		FCoreDelegates::OnActorLabelChanged.RemoveAll(this);

		USelection::SelectionChangedEvent.RemoveAll(this);
		USelection::SelectObjectEvent.RemoveAll(this);

		GEngine->OnHLODActorMoved().RemoveAll(this);
		GEngine->OnHLODActorAdded().RemoveAll(this);
		GEngine->OnHLODActorMarkedDirty().RemoveAll(this);
	}

	void SHLODOutliner::ForceViewLODActor(TSharedRef<ITreeItem> Item)
	{
		if (CurrentWorld)
		{
			const FScopedTransaction Transaction(LOCTEXT("UndoAction_LODLevelForcedView", "LOD Level Forced View"));
			FLODActorItem* ActorItem = (FLODActorItem*)(&Item.Get());

			if (ActorItem->LODActor.IsValid())
			{
				ActorItem->LODActor->Modify();
				ActorItem->LODActor->ToggleForceView();				
			}
		}
	}

	ECheckBoxState SHLODOutliner::IsHLODLevelChecked(TSharedRef<ITreeItem> Item) const
	{
		check(Item->GetTreeItemType() == ITreeItem::HierarchicalLODLevel);

		FLODLevelItem* LevelItem = static_cast<FLODLevelItem*>(&Item.Get());

		return (LevelItem->LODLevelIndex == ForcedLODLevel) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}

	void SHLODOutliner::HandleCheckBoxCheckedStateChanged(ECheckBoxState NewState, TSharedRef<ITreeItem> Item)
	{
		check(Item->GetTreeItemType() == ITreeItem::HierarchicalLODLevel);

		FLODLevelItem* LevelItem = static_cast<FLODLevelItem*>(&Item.Get());
		if (LevelItem->LODLevelIndex == ForcedLODLevel && NewState == ECheckBoxState::Unchecked)
		{
			// Reset all
			RestoreForcedLODLevel(LevelItem->LODLevelIndex);
			ForcedLODLevel = -1;
		}
		else if ( LevelItem->LODLevelIndex != ForcedLODLevel && NewState == ECheckBoxState::Checked)
		{
			RestoreForcedLODLevel(ForcedLODLevel);
			SetForcedLODLevel(LevelItem->LODLevelIndex);			
		}
	}

	bool SHLODOutliner::CanHLODLevelBeForced(const uint32 LODLevel) const
	{
		return LODLevelBuildFlags[LODLevel];
	}

	void SHLODOutliner::RestoreForcedLODLevel(const uint32 LODLevel)
	{
		if (LODLevel == -1)
		{
			return;
		}

		if (CurrentWorld)
		{
			auto Level = CurrentWorld->GetCurrentLevel();
			for (auto Actor : Level->Actors)
			{
				ALODActor* LODActor = Cast<ALODActor>(Actor);
				if (LODActor)
				{
					if (LODActor->LODLevel == LODLevel + 1)
					{
						LODActor->SetForcedView(false);
					}
					else
					{
						LODActor->SetHiddenFromEditorView(false, LODLevel + 1);
					}
				}
			}
		}
	}

	void SHLODOutliner::SetForcedLODLevel(const uint32 LODLevel)
	{
		if (CurrentWorld)
		{
			auto Level = CurrentWorld->GetCurrentLevel();
			for (auto Actor : Level->Actors)
			{
				ALODActor* LODActor = Cast<ALODActor>(Actor);
				if (LODActor)
				{
					if (LODActor->LODLevel == LODLevel + 1)
					{
						LODActor->SetForcedView(true);
					}
					else
					{
						LODActor->SetHiddenFromEditorView(true, LODLevel + 1);
					}
				}
			}
		}
		ForcedLODLevel = LODLevel;
	}

	void SHLODOutliner::BuildLODActor(TSharedRef<ITreeItem> Item)
	{
		if (CurrentWorld)
		{			
			FLODActorItem* ActorItem = (FLODActorItem*)(&Item.Get());

			auto Parent = ActorItem->GetParent();

			ITreeItem::TreeItemType Type = Parent->GetTreeItemType();			
			if (Type == ITreeItem::HierarchicalLODLevel)
			{
				FLODLevelItem* LevelItem = (FLODLevelItem*)(Parent.Get());
				CurrentWorld->HierarchicalLODBuilder->BuildMeshForLODActor(ActorItem->LODActor.Get(), LevelItem->LODLevelIndex);
				FullRefresh();
			}			
		}
	}

	void SHLODOutliner::SelectLODActor(TSharedRef<ITreeItem> Item)
	{
		if (CurrentWorld)
		{
			FLODActorItem* ActorItem = (FLODActorItem*)(&Item.Get());

			if (ActorItem->LODActor.IsValid())
			{
				EmptySelection();
				StartSelection();
				SelectActorInViewport(ActorItem->LODActor.Get(), 0);
				EndSelection();
			}			
		}
	}

	void SHLODOutliner::DeleteCluster(TSharedRef<ITreeItem> Item)
	{
		const FScopedTransaction Transaction(LOCTEXT("UndoAction_DestroyLODActor", "Delete LOD Actor"));
		FLODActorItem* ActorItem = (FLODActorItem*)(&Item.Get());
		
		ALODActor* LODActor = ActorItem->LODActor.Get();
		ALODActor* ParentActor = HierarchicalLODUtils::GetParentLODActor(LODActor);

		LODActor->Modify();

		if (ParentActor)
		{
			ParentActor->Modify();
		}

		HierarchicalLODUtils::DeleteLODActor(LODActor);
		CurrentWorld->DestroyActor(LODActor);

		if (ParentActor && !ParentActor->HasValidSubActors())
		{			
			DestroyLODActor(ParentActor);
		}
	}

	void SHLODOutliner::RemoveStaticMeshActorFromCluster(TSharedRef<ITreeItem> Item)
	{
		if (CurrentWorld)
		{
			const FScopedTransaction Transaction(LOCTEXT("UndoAction_RemoveStaticMeshActorFromCluster", "Removed Static Mesh Actor From Cluster"));
			FStaticMeshActorItem* ActorItem = (FStaticMeshActorItem*)(&Item.Get());
			auto Parent = ActorItem->GetParent();
			
			ITreeItem::TreeItemType Type = Parent->GetTreeItemType();			
			if (Type == ITreeItem::HierarchicalLODActor)
			{
				FLODActorItem* ParentLODActorItem = (FLODActorItem*)(Parent.Get());
				ParentLODActorItem->LODActor->Modify();
				ActorItem->StaticMeshActor->Modify();
				ParentLODActorItem->LODActor->RemoveSubActor(ActorItem->StaticMeshActor.Get());

				PendingActions.Emplace(FOutlinerAction::RemoveItem, Item);

				if (!ParentLODActorItem->LODActor->HasValidSubActors())
				{
					DestroyLODActor(ParentLODActorItem->LODActor.Get());
					PendingActions.Emplace(FOutlinerAction::RemoveItem, Parent);
				}
			}
		}
	}

	void SHLODOutliner::RemoveLODActorFromCluster(TSharedRef<ITreeItem> Item)
	{
		if (CurrentWorld)
		{
			const FScopedTransaction Transaction(LOCTEXT("UndoAction_RemoveLODActorFromCluster", "Removed LOD Actor From Cluster"));
			FLODActorItem* ActorItem = (FLODActorItem*)(&Item.Get());
			auto Parent = ActorItem->GetParent();

			ITreeItem::TreeItemType Type = Parent->GetTreeItemType();
			if (Type == ITreeItem::HierarchicalLODActor)
			{
				FLODActorItem* ParentLODActorItem = (FLODActorItem*)(Parent.Get());
				ParentLODActorItem->LODActor->Modify();
				ActorItem->LODActor->Modify();
				ParentLODActorItem->LODActor->RemoveSubActor(ActorItem->LODActor.Get());

				PendingActions.Emplace(FOutlinerAction::RemoveItem, Item);

				if (!ParentLODActorItem->LODActor->HasValidSubActors())
				{
					DestroyLODActor(ParentLODActorItem->LODActor.Get());
					PendingActions.Emplace(FOutlinerAction::RemoveItem, Parent);
				}
			}
		}
	}

	void SHLODOutliner::SelectContainedActors(TSharedRef<ITreeItem> Item)
	{
		FLODActorItem* ActorItem = (FLODActorItem*)(&Item.Get());

		ALODActor* LODActor = ActorItem->LODActor.Get();
		SelectLODActorAndContainedActorsInViewport(LODActor, 0);
	}

	void SHLODOutliner::DestroyLODActor(ALODActor* InActor)
	{		
		ALODActor* ParentActor = HierarchicalLODUtils::GetParentLODActor(InActor);

		HierarchicalLODUtils::DeleteLODActor(InActor);
		CurrentWorld->DestroyActor(InActor);

		if (ParentActor && !ParentActor->HasValidSubActors())
		{
			DestroyLODActor(ParentActor);
		}
	}

	void SHLODOutliner::UpdateDrawDistancesForLODLevel(const uint32 LODLevelIndex)
	{
		if (CurrentWorld)
		{
			auto Level = CurrentWorld->GetCurrentLevel();
			for (auto Actor : Level->Actors)
			{
				ALODActor* LODActor = Cast<ALODActor>(Actor);
				if (LODActor)
				{
					if (LODActor->LODLevel == LODLevelIndex + 1)
					{
						LODActor->LODDrawDistance = LODLevelDrawDistances[LODLevelIndex];
						LODActor->UpdateSubActorLODParents();
					}
				}
			}
		}
	}

	void SHLODOutliner::RemoveLODLevelActors(const int32 HLODLevelIndex)
	{
		if (CurrentWorld)
		{
			HierarchicalLODUtils::DeleteLODActorsInHLODLevel(CurrentWorld, HLODLevelIndex);
		}
	}

	TSharedRef<ITableRow> SHLODOutliner::OnOutlinerGenerateRow(FTreeItemPtr InTreeItem, const TSharedRef<STableViewBase>& OwnerTable)
	{
		TSharedRef<ITableRow> Widget = SNew(SHLODWidgetItem, OwnerTable)
			.TreeItemToVisualize(InTreeItem)
			.Outliner(this)
			.World(CurrentWorld);

		//ItemMap.Add(InTreeItem->GetHash(), InTreeItem);
		return Widget;
	}

	void SHLODOutliner::OnOutlinerGetChildren(FTreeItemPtr InParent, TArray<FTreeItemPtr>& OutChildren)
	{		
		for (auto& WeakChild : InParent->GetChildren())
		{
			auto Child = WeakChild.Pin();
			// Should never have bogus entries in this list
			check(Child.IsValid());
			OutChildren.Add(Child);
		}
	}

	void SHLODOutliner::OnOutlinerSelectionChanged(FTreeItemPtr TreeItem, ESelectInfo::Type SelectInfo)
	{
		if (SelectInfo == ESelectInfo::Direct)
		{
			return;
		}

		EmptySelection();

		SelectedNodes = TreeView->GetSelectedItems();

		if (TreeItem.IsValid())
		{
			
			StartSelection();

			ITreeItem::TreeItemType Type = TreeItem->GetTreeItemType();

			switch (Type)
			{
				case ITreeItem::HierarchicalLODLevel:
				{
					FLODLevelItem* LevelItem = (FLODLevelItem*)(TreeItem.Get());
					const TArray<TWeakPtr<ITreeItem>>& Children = LevelItem->GetChildren();
					for (auto& WeakChild : Children)
					{
						auto Child = WeakChild.Pin();
						check(Child.IsValid());
						FLODActorItem* ActorItem = (FLODActorItem*)(Child.Get());
						SelectLODActorAndContainedActorsInViewport(ActorItem->LODActor.Get(), 0);
					}

					break;
				}

				case ITreeItem::HierarchicalLODActor:
				{
					FLODActorItem* ActorItem = (FLODActorItem*)(TreeItem.Get());
					SelectActorInViewport(ActorItem->LODActor.Get(), 0);
					break;
				}

				case ITreeItem::StaticMeshActor:
				{
					FStaticMeshActorItem* StaticMeshActorItem = (FStaticMeshActorItem*)(TreeItem.Get());
					SelectActorInViewport(StaticMeshActorItem->StaticMeshActor.Get(), 0);
					break;
				}

			
			}
			

			EndSelection();
		}
	}

	void SHLODOutliner::OnOutlinerDoubleClick(FTreeItemPtr TreeItem)
	{
		
	}

	TSharedPtr<SWidget> SHLODOutliner::OnOpenContextMenu()
	{
		if (!CurrentWorld)
		{
			return nullptr;
		}

		// Build up the menu for a selection
		const bool bCloseAfterSelection = true;
		TSharedPtr<FExtender> Extender = MakeShareable(new FExtender);

		FMenuBuilder MenuBuilder(bCloseAfterSelection, TSharedPtr<FUICommandList>(), Extender);

		const auto NumSelectedItems = TreeView->GetNumItemsSelected();
		if (NumSelectedItems == 1)
		{
			TreeView->GetSelectedItems()[0]->GenerateContextMenu(MenuBuilder, *this);

			return MenuBuilder.MakeWidget();
		}

		return TSharedPtr<SWidget>();
	}

	void SHLODOutliner::OnItemExpansionChanged(FTreeItemPtr TreeItem, bool bIsExpanded)
	{
		TreeItem->bIsExpanded = bIsExpanded;

		// Expand any children that are also expanded
		for (auto WeakChild : TreeItem->GetChildren())
		{
			auto Child = WeakChild.Pin();
			if (Child->bIsExpanded)
			{
				TreeView->SetItemExpansion(Child, true);
			}
		}
	}

	void SHLODOutliner::StartSelection()
	{
		GEditor->GetSelectedActors()->BeginBatchSelectOperation();
	}

	void SHLODOutliner::EmptySelection()
	{
		GEditor->SelectNone(false, true, true);
		DestroySelectionActors();
	}

	void SHLODOutliner::DestroySelectionActors()
	{
		if (CurrentWorld)
		{
			for (AHLODSelectionActor* BoundsActor : SelectionActors)
			{
				if (BoundsActor)
				{
					CurrentWorld->DestroyActor(BoundsActor);
				}
			}
		}
		SelectionActors.Empty();
	}

	void SHLODOutliner::SelectActorInViewport(AActor* Actor, const uint32 SelectionDepth)
	{
		GEditor->SelectActor(Actor, true, false);

		if (Actor->IsA<ALODActor>() && SelectionDepth == 0)
		{
			CreateBoundingSphereForActor(Actor);
		}
	}

	void SHLODOutliner::SelectLODActorAndContainedActorsInViewport(ALODActor* LODActor, const uint32 SelectionDepth)
	{
		TArray<AActor*> SubActors;
		ExtractStaticMeshActorsFromLODActor(LODActor, SubActors);
		for (AActor* SubActor : SubActors)
		{
			SelectActorInViewport(SubActor, SelectionDepth + 1);
		}

		SelectActorInViewport(LODActor, SelectionDepth);
	}

	UDrawSphereComponent* SHLODOutliner::CreateBoundingSphereForActor(AActor* Actor)
	{
		if (CurrentWorld)
		{
			AHLODSelectionActor* SelectionActor = CurrentWorld->SpawnActorDeferred<AHLODSelectionActor>(AHLODSelectionActor::StaticClass(), FTransform());
			SelectionActor->ClearFlags(RF_Public | RF_Standalone);
			SelectionActor->SetFlags(RF_Transient);			

			UDrawSphereComponent* BoundSphereSpawned = SelectionActor->GetDrawSphereComponent();
			BoundSphereSpawned->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
			BoundSphereSpawned->RegisterComponent();

			FVector Origin, Extent;
			FBox BoundingBox = Actor->GetComponentsBoundingBox(true);
			BoundSphereSpawned->SetWorldLocation(BoundingBox.GetCenter());
			BoundSphereSpawned->SetSphereRadius(BoundingBox.GetExtent().Size());
			BoundSphereSpawned->ShapeColor = FColor::Red;		

			SelectionActors.Add(SelectionActor);

			return BoundSphereSpawned;
		}

		return nullptr;
	}

	void SHLODOutliner::ExtractStaticMeshActorsFromLODActor(ALODActor* LODActor, TArray<AActor*> &InOutActors)
	{
		for (auto ChildActor : LODActor->SubActors)
		{
			if (ChildActor)
			{
				TArray<AActor*> ChildActors;
				if (ChildActor->IsA<ALODActor>())
				{
					ExtractStaticMeshActorsFromLODActor(Cast<ALODActor>(ChildActor), ChildActors);
				}

				ChildActors.Push(ChildActor);
				InOutActors.Append(ChildActors);
			}
		}
	}

	void SHLODOutliner::EndSelection()
	{
		// Commit selection changes
		GEditor->GetSelectedActors()->EndBatchSelectOperation();

		// Fire selection changed event
		GEditor->NoteSelectionChange();
	}

	void SHLODOutliner::OnLevelSelectionChanged(UObject* Obj)
	{		
		AActor* Actor = Cast<AActor>(Obj);
		if (Actor)
		{
			auto Item = TreeItemsMap.Find(Actor);
			if (Item)
			{
				SelectItemInTree(*Item);
			}	
			else
			{
				DestroySelectionActors();
			}
		}
	}

	void SHLODOutliner::OnLevelAdded(ULevel* InLevel, UWorld* InWorld)
	{
		//FullRefresh();
	}

	void SHLODOutliner::OnLevelRemoved(ULevel* InLevel, UWorld* InWorld)
	{
		//FullRefresh();
	}

	void SHLODOutliner::OnLevelActorsAdded(AActor* InActor)
	{
		if (!InActor->IsA<AHLODSelectionActor>() && !InActor->IsA<AWorldSettings>())
			FullRefresh();
	}

	void SHLODOutliner::OnLevelActorsRemoved(AActor* InActor)
	{
		if (!InActor->IsA<AHLODSelectionActor>() && !InActor->IsA<AWorldSettings>())
			FullRefresh();
	}
	
	void SHLODOutliner::OnActorLabelChanged(AActor* ChangedActor)
	{
		if (!ChangedActor->IsA<AHLODSelectionActor>())
			FullRefresh();
	}

	void SHLODOutliner::OnMapChange(uint32 MapFlags)
	{
		FullRefresh();
	}

	void SHLODOutliner::OnNewCurrentLevel()
	{
		FullRefresh();
	}

	void SHLODOutliner::OnHLODActorMovedEvent(const AActor* InActor, const AActor* ParentActor)
	{
		FTreeItemPtr* TreeItem = TreeItemsMap.Find(InActor);
		FTreeItemPtr* ParentItem = TreeItemsMap.Find(ParentActor);
		if (TreeItem && ParentItem)
		{			
			PendingActions.Emplace(FOutlinerAction::MoveItem, *TreeItem, *ParentItem);

			auto CurrentParent = (*TreeItem)->GetParent(); 

			if (CurrentParent.IsValid())
			{
				if (CurrentParent->GetTreeItemType() == ITreeItem::HierarchicalLODActor)
				{
					FLODActorItem* ParentLODActorItem = (FLODActorItem*)CurrentParent.Get();
					if (!ParentLODActorItem->LODActor->HasValidSubActors())
					{
						DestroyLODActor(ParentLODActorItem->LODActor.Get());
						PendingActions.Emplace(FOutlinerAction::RemoveItem, CurrentParent);
					}
				}
			}
		}
	}

	void SHLODOutliner::OnActorMovedEvent(AActor* InActor)
	{
		if (InActor->IsA<ALODActor>())
		{
			return;
		}

		ALODActor* ParentActor = HierarchicalLODUtils::GetParentLODActor(InActor);
		if (ParentActor)
		{
			ParentActor->Modify();
			ParentActor->SetIsDirty(true);
		}		
	}

	void SHLODOutliner::OnHLODActorAddedEvent(const AActor* InActor, const AActor* ParentActor)
	{
		FullRefresh();
	}

	void SHLODOutliner::OnHLODActorMarkedDirtyEvent(ALODActor* InActor)
	{		
		if (InActor->GetStaticMeshComponent()->StaticMesh)
		{
			HierarchicalLODUtils::DeleteLODActorAssets(InActor);		
		}		
		FullRefresh();
	}

	void SHLODOutliner::OnHLODDrawDistanceChangedEvent()
	{
		if (CurrentWorld)
		{
			auto WorldSettings = CurrentWorld->GetWorldSettings();

			int32 MaxLODLevel = FMath::Min(WorldSettings->HierarchicalLODSetup.Num(), LODLevelDrawDistances.Num());
			for (int32 LODLevelIndex = 0; LODLevelIndex < MaxLODLevel; ++LODLevelIndex)
			{
				if (LODLevelDrawDistances[LODLevelIndex] != WorldSettings->HierarchicalLODSetup[LODLevelIndex].DrawDistance)
				{
					LODLevelDrawDistances[LODLevelIndex] = WorldSettings->HierarchicalLODSetup[LODLevelIndex].DrawDistance;
					UpdateDrawDistancesForLODLevel(LODLevelIndex);
				}
			}
		}
	}

	void SHLODOutliner::OnHLODLevelsArrayChangedEvent()
	{
		if (CurrentWorld)
		{
			auto WorldSettings = CurrentWorld->GetWorldSettings();			
			if (WorldSettings->HierarchicalLODSetup.Num() > 1 && WorldSettings->HierarchicalLODSetup.Num() > LODLevelDrawDistances.Num())
			{
				// HLOD level was added, use previous level settings to "guess" new settings
				auto& NewLevelSetup = WorldSettings->HierarchicalLODSetup.Last();
				auto& OldLastLevelSetup = WorldSettings->HierarchicalLODSetup[WorldSettings->HierarchicalLODSetup.Num() - 2];

				NewLevelSetup.bSimplifyMesh = OldLastLevelSetup.bSimplifyMesh;
				NewLevelSetup.MergeSetting = OldLastLevelSetup.MergeSetting;
				NewLevelSetup.ProxySetting = OldLastLevelSetup.ProxySetting;

				NewLevelSetup.DesiredBoundRadius = OldLastLevelSetup.DesiredBoundRadius * 2.5f;
				NewLevelSetup.DesiredFillingPercentage = FMath::Max( OldLastLevelSetup.DesiredFillingPercentage * 0.75f, 1.0f );
				NewLevelSetup.DrawDistance = OldLastLevelSetup.DrawDistance * 2.5f;
				NewLevelSetup.MinNumberOfActorsToBuild = OldLastLevelSetup.MinNumberOfActorsToBuild;			
			}
			else if (WorldSettings->HierarchicalLODSetup.Num() < LODLevelDrawDistances.Num())
			{
				// HLOD Level was removed, now remove all LODActors for this level
				RemoveLODLevelActors(LODLevelDrawDistances.Num() - 1);
			}

			FullRefresh();
		}
	}

	void SHLODOutliner::FullRefresh()
	{
		bNeedsRefresh = true;
	}

	void SHLODOutliner::Populate()
	{
		HLODTreeRoot.Empty();
		TreeItemsMap.Empty();

		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::PIE)
			{
				CurrentWorld = Context.World();
				break;
			}
			else if (Context.WorldType == EWorldType::Editor)
			{
				CurrentWorld = Context.World();
			}
		}

		if (CurrentWorld)
		{
			// Update settings view
			SettingsView->SetObject(CurrentWorld->GetWorldSettings());

			TArray<FTreeItemRef> LevelNodes;
			AWorldSettings* WorldSettings = CurrentWorld->GetWorldSettings();
			if (WorldSettings)
			{
				LODLevelBuildFlags.Empty();
				LODLevelActors.Empty();
				LODLevelDrawDistances.Empty();

				const uint32 LODLevels = WorldSettings->HierarchicalLODSetup.Num();
				for (uint32 LODLevelIndex = 0; LODLevelIndex < LODLevels; ++LODLevelIndex)
				{
					FTreeItemRef LevelItem = MakeShareable(new FLODLevelItem(LODLevelIndex));

					PendingActions.Emplace(FOutlinerAction::AddItem, LevelItem);

					LevelNodes.Add(LevelItem->AsShared());
					HLODTreeRoot.Add(LevelItem->AsShared());
					AllNodes.Add(LevelItem->AsShared());

					LODLevelBuildFlags.Add(true);
					LODLevelActors.AddDefaulted();
					LODLevelDrawDistances.Add(WorldSettings->HierarchicalLODSetup[LODLevelIndex].DrawDistance);

					TreeItemsMap.Add(LevelItem->GetID(), LevelItem);
				}

				for (AActor* Actor : CurrentWorld->GetCurrentLevel()->Actors)
				{
					if (Actor && Actor->IsA<ALODActor>())
					{
						ALODActor* LODActor = CastChecked<ALODActor>(Actor);

						if (LODActor)
						{
							FTreeItemRef Item = MakeShareable(new FLODActorItem(LODActor));
							AllNodes.Add(Item->AsShared());

							PendingActions.Emplace(FOutlinerAction::AddItem, Item, LevelNodes[LODActor->LODLevel - 1]);

							for (AActor* ChildActor : LODActor->SubActors)
							{
								if (ChildActor->IsA<ALODActor>())
								{
									FTreeItemRef ChildItem = MakeShareable(new FLODActorItem(CastChecked<ALODActor>(ChildActor)));
									AllNodes.Add(ChildItem->AsShared());
									Item->AddChild(ChildItem);
								}
								else
								{
									FTreeItemRef ChildItem = MakeShareable(new FStaticMeshActorItem(ChildActor));
									AllNodes.Add(ChildItem->AsShared());

									PendingActions.Emplace(FOutlinerAction::AddItem, ChildItem, Item);
								}
							}

							LODLevelBuildFlags[LODActor->LODLevel - 1] &= !LODActor->IsDirty();
							LODLevelActors[LODActor->LODLevel - 1].Add(LODActor);
						}
					}
				}

				for (uint32 LODLevelIndex = 0; LODLevelIndex < LODLevels; ++LODLevelIndex)
				{
					if (LODLevelActors[LODLevelIndex].Num() == 0)
					{
						LODLevelBuildFlags[LODLevelIndex] = false;
					}
				}
			}

			TreeView->RequestTreeRefresh();
		}

		bNeedsRefresh = false;
	}

	TMap<FTreeItemID, bool> SHLODOutliner::GetParentsExpansionState() const
	{
		FParentsExpansionState States;
		for (const auto& Pair : TreeItemsMap)
		{
			if (Pair.Value->GetChildren().Num())
			{
				States.Add(Pair.Key, Pair.Value->bIsExpanded);
			}
		}

		return States;
	}

	void SHLODOutliner::SetParentsExpansionState(const FParentsExpansionState& ExpansionStateInfo) const
	{
		for (const auto& Pair : TreeItemsMap)
		{
			auto& Item = Pair.Value;
			if (Item->GetChildren().Num())
			{
				const bool* bIsExpanded = ExpansionStateInfo.Find(Pair.Key);
				if (bIsExpanded)
				{
					TreeView->SetItemExpansion(Item, *bIsExpanded);
				}
				else
				{
					TreeView->SetItemExpansion(Item, Item->bIsExpanded);
				}
			}
		}
	}

	const bool SHLODOutliner::AddItemToTree(FTreeItemPtr InItem, FTreeItemPtr InParentItem)
	{
		const auto ItemID = InItem->GetID();

		if (TreeItemsMap.Find(ItemID))
		{
			return false;
		}

		TreeItemsMap.Add(ItemID, InItem);

		if (InParentItem.Get())
		{
			InParentItem->AddChild(InItem->AsShared());
		}		

		return true;
	}

	void SHLODOutliner::MoveItemInTree(FTreeItemPtr InItem, FTreeItemPtr InParentItem)
	{
		auto CurrentParent = InItem->Parent;
		if (CurrentParent.IsValid())
		{
			CurrentParent.Pin()->RemoveChild(InItem->AsShared());
		}

		if (InParentItem.Get())
		{
			InParentItem->AddChild(InItem->AsShared());
		}
	}

	void SHLODOutliner::RemoveItemFromTree(FTreeItemPtr InItem)
	{
		const int32 NumRemoved = TreeItemsMap.Remove(InItem->GetID());

		if (!NumRemoved)
		{
			return;
		}

		auto ParentItem = InItem->GetParent();
		if (ParentItem.IsValid())
		{
			ParentItem->RemoveChild(InItem->AsShared());
		}
	}

	void SHLODOutliner::SelectItemInTree(FTreeItemPtr InItem)
	{
		auto Parent = InItem->GetParent();
		while (Parent.IsValid() && !Parent->bIsExpanded)
		{
			Parent->bIsExpanded = true;
			Parent = InItem->GetParent();
		}
		TreeView->SetSelection(InItem);

		TreeView->RequestTreeRefresh();
	}

	FReply SHLODOutliner::RetrieveActors()
	{
		bNeedsRefresh = true;
		return FReply::Handled();
	}

};

#undef LOCTEXT_NAMESPACE // LOCTEXT_NAMESPACE "HLODOutliner"