// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "GameplayTagsEditorModulePrivatePCH.h"
#include "SGameplayTagWidget.h"
#include "GameplayTagsModule.h"
#include "GameplayTags.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "GameplayTagWidget"

const FString SGameplayTagWidget::SettingsIniSection = TEXT("GameplayTagWidget");

void SGameplayTagWidget::Construct(const FArguments& InArgs, const TArray<FEditableGameplayTagContainerDatum>& EditableTagContainers)
{
	ensure(EditableTagContainers.Num() > 0);
	TagContainers = EditableTagContainers;

	OnTagChanged = InArgs._OnTagChanged;
	bReadOnly = InArgs._ReadOnly;
	TagContainerName = InArgs._TagContainerName;

	IGameplayTagsModule::Get().GetGameplayTagsManager().GetFilteredGameplayRootTags(InArgs._Filter, TagItems);

	// Tag the assets as transactional so they can support undo/redo
	for (int32 AssetIdx = 0; AssetIdx < TagContainers.Num(); ++AssetIdx)
	{
		if (TagContainers[AssetIdx].TagContainerOwner)
		{
			TagContainers[AssetIdx].TagContainerOwner->SetFlags(RF_Transactional);
		}
	}

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			.VAlign(VAlign_Top)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.OnClicked(this, &SGameplayTagWidget::OnExpandAllClicked)
					.Text(LOCTEXT("GameplayTagWidget_ExpandAll", "Expand All").ToString())
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.OnClicked(this, &SGameplayTagWidget::OnCollapseAllClicked)
					.Text(LOCTEXT("GameplayTagWidget_CollapseAll", "Collapse All").ToString())
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.IsEnabled(!bReadOnly)
					.OnClicked(this, &SGameplayTagWidget::OnClearAllClicked)
					.Text(LOCTEXT("GameplayTagWidget_ClearAll", "Clear All").ToString())
				]
				+SHorizontalBox::Slot()
				.VAlign( VAlign_Center )
				.FillWidth(1.f)
				.Padding(5,1,5,1)
				[
					SNew(SSearchBox)
					.HintText(LOCTEXT("GameplayTagWidget_SearchBoxHint", "Search Gameplay Tags"))
					.OnTextChanged( this, &SGameplayTagWidget::OnFilterTextChanged )
				]
			]
			+SVerticalBox::Slot()
			[
				SNew(SBorder)
				.Padding(FMargin(4.f))
				[
					SAssignNew(TagTreeWidget, STreeView< TSharedPtr<FGameplayTagNode> >)
					.TreeItemsSource(&TagItems)
					.OnGenerateRow(this, &SGameplayTagWidget::OnGenerateRow)
					.OnGetChildren(this, &SGameplayTagWidget::OnGetChildren)
					.OnExpansionChanged( this, &SGameplayTagWidget::OnExpansionChanged)
					.SelectionMode(ESelectionMode::Multi)
				]
			]
		]
	];

	// Force the entire tree collapsed to start
	SetTagTreeItemExpansion(false);
	 
	LoadSettings();

	// Strip any invalid tags from the assets being edited
	VerifyAssetTagValidity();
}

void SGameplayTagWidget::OnFilterTextChanged( const FText& InFilterText )
{
	FilterString = InFilterText.ToString();	

	if( FilterString.IsEmpty() )
	{
		TagTreeWidget->SetTreeItemsSource( &TagItems );
	}
	else
	{
		FilteredTagItems.Empty();

		for( int32 iItem = 0; iItem < TagItems.Num(); ++iItem )
		{
			if( FilterChildrenCheck( TagItems[iItem] ) )
			{
				FilteredTagItems.Add( TagItems[iItem] );
			}
		}

		TagTreeWidget->SetTreeItemsSource( &FilteredTagItems );	
	}
		
	TagTreeWidget->RequestTreeRefresh();	
}

bool SGameplayTagWidget::FilterChildrenCheck( TSharedPtr<FGameplayTagNode> InItem )
{
	if( !InItem.IsValid() )
	{
		return false;
	}

	if( InItem->GetCompleteTag().ToString().Contains( FilterString ) || FilterString.IsEmpty() )
	{
		return true;
	}

	TArray< TSharedPtr<FGameplayTagNode> > Children = InItem->GetChildTagNodes();

	for( int32 iChild = 0; iChild < Children.Num(); ++iChild )
	{
		if( FilterChildrenCheck( Children[iChild] ) )
		{
			return true;
		}
	}

	return false;
}

TSharedRef<ITableRow> SGameplayTagWidget::OnGenerateRow(TSharedPtr<FGameplayTagNode> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	FString TooltipText;
	if (InItem.IsValid())
	{
		TooltipText = InItem.Get()->GetCompleteTag().ToString();
	}

	return SNew(STableRow< TSharedPtr<FGameplayTagNode> >, OwnerTable)
		.Style(FEditorStyle::Get(), "GameplayTagTreeView")
		[
			SNew(SCheckBox)
			.OnCheckStateChanged(this, &SGameplayTagWidget::OnTagCheckStatusChanged, InItem)
			.IsChecked(this, &SGameplayTagWidget::IsTagChecked, InItem)
			.ToolTipText(TooltipText)
			.ReadOnly( bReadOnly )
			[
				SNew(STextBlock)				
				.Text(InItem->GetSimpleTag().ToString())
			]	
		];
}

void SGameplayTagWidget::OnGetChildren(TSharedPtr<FGameplayTagNode> InItem, TArray< TSharedPtr<FGameplayTagNode> >& OutChildren)
{
	TArray< TSharedPtr<FGameplayTagNode> > FilteredChildren;
	TArray< TSharedPtr<FGameplayTagNode> > Children = InItem->GetChildTagNodes();

	for( int32 iChild = 0; iChild < Children.Num(); ++iChild )
	{
		if( FilterChildrenCheck( Children[iChild] ) )
		{
			FilteredChildren.Add( Children[iChild] );
		}
	}
	OutChildren += FilteredChildren;
}

void SGameplayTagWidget::OnTagCheckStatusChanged(ESlateCheckBoxState::Type NewCheckState, TSharedPtr<FGameplayTagNode> NodeChanged)
{
	if (NewCheckState == ESlateCheckBoxState::Checked)
	{
		OnTagChecked(NodeChanged);
	}
	else if (NewCheckState == ESlateCheckBoxState::Unchecked)
	{
		OnTagUnchecked(NodeChanged, true);
	}

	OnTagChanged.ExecuteIfBound();
}

void SGameplayTagWidget::OnTagChecked(TSharedPtr<FGameplayTagNode> NodeChecked)
{
	FScopedTransaction Transaction( LOCTEXT("GameplayTagWidget_AddTags", "Add Gameplay Tags") );
	TWeakPtr<FGameplayTagNode> CurNode(NodeChecked);

	while (CurNode.IsValid())
	{
		for (int32 ContainerIdx = 0; ContainerIdx < TagContainers.Num(); ++ContainerIdx)
		{
			if (TagContainers[ContainerIdx].TagContainerOwner)
			{
				UObject* OwnerObj = TagContainers[ContainerIdx].TagContainerOwner;
				FGameplayTagContainer* Container = TagContainers[ContainerIdx].TagContainer;
				if (OwnerObj && Container)
				{
					OwnerObj->PreEditChange(NULL);
					Container->AddTag(CurNode.Pin()->GetCompleteTag());
					OwnerObj->PostEditChange();
				}
			}
		}
		CurNode = CurNode.Pin()->GetParentTagNode();
	}
}

void SGameplayTagWidget::OnTagUnchecked(TSharedPtr<FGameplayTagNode> NodeUnchecked, bool bTransact)
{
	FScopedTransaction Transaction( LOCTEXT("GameplayTagWidget_RemoveTags", "Remove Gameplay Tags"), bTransact);
	if (NodeUnchecked.IsValid())
	{
		for (int32 ContainerIdx = 0; ContainerIdx < TagContainers.Num(); ++ContainerIdx)
		{
			UObject* OwnerObj = TagContainers[ContainerIdx].TagContainerOwner;
			FGameplayTagContainer* Container = TagContainers[ContainerIdx].TagContainer;

			if (OwnerObj && Container)
			{
				OwnerObj->PreEditChange(NULL);
				Container->RemoveTag(NodeUnchecked->GetCompleteTag());
				OwnerObj->PostEditChange();
			}
		}

		const TArray< TSharedPtr<FGameplayTagNode> >& ChildNodes = NodeUnchecked->GetChildTagNodes();
		for (int32 ChildIdx = 0; ChildIdx < ChildNodes.Num(); ++ChildIdx)
		{
			OnTagUnchecked(ChildNodes[ChildIdx], false);
		}
	}
}

ESlateCheckBoxState::Type SGameplayTagWidget::IsTagChecked(TSharedPtr<FGameplayTagNode> Node) const
{
	int32 NumValidAssets = 0;
	int32 NumAssetsTagIsAppliedTo = 0;

	if (Node.IsValid())
	{
		for (int32 ContainerIdx = 0; ContainerIdx < TagContainers.Num(); ++ContainerIdx)
		{
			FGameplayTagContainer* Container = TagContainers[ContainerIdx].TagContainer;
			if (Container)
			{
				NumValidAssets++;
				if (Container->HasTag(Node->GetCompleteTag()))
				{
					++NumAssetsTagIsAppliedTo;
				}
			}
		}
	}

	if (NumAssetsTagIsAppliedTo == 0)
	{
		return ESlateCheckBoxState::Unchecked;
	}
	else if (NumAssetsTagIsAppliedTo == NumValidAssets)
	{
		return ESlateCheckBoxState::Checked;
	}
	else
	{
		return ESlateCheckBoxState::Undetermined;
	}
}

FReply SGameplayTagWidget::OnClearAllClicked()
{
	FScopedTransaction Transaction( LOCTEXT("GameplayTagWidget_RemoveAllTags", "Remove All Gameplay Tags") );

	for (int32 ContainerIdx = 0; ContainerIdx < TagContainers.Num(); ++ContainerIdx)
	{
		UObject* OwnerObj = TagContainers[ContainerIdx].TagContainerOwner;
		FGameplayTagContainer* Container = TagContainers[ContainerIdx].TagContainer;

		if (OwnerObj && Container)
		{
			OwnerObj->PreEditChange(NULL);
			Container->RemoveAllTags();
			OwnerObj->PostEditChange();
		}
	}

	OnTagChanged.ExecuteIfBound();

	return FReply::Handled();
}

FReply SGameplayTagWidget::OnExpandAllClicked()
{
	SetTagTreeItemExpansion(true);
	return FReply::Handled();
}

FReply SGameplayTagWidget::OnCollapseAllClicked()
{
	SetTagTreeItemExpansion(false);
	return FReply::Handled();
}

void SGameplayTagWidget::SetTagTreeItemExpansion(bool bExpand)
{
	TArray< TSharedPtr<FGameplayTagNode> >& TagArray = IGameplayTagsModule::Get().GetGameplayTagsManager().GameplayRootTags;
	for (int32 TagIdx = 0; TagIdx < TagArray.Num(); ++TagIdx)
	{
		SetTagNodeItemExpansion(TagArray[TagIdx], bExpand);
	}
	
}

void SGameplayTagWidget::SetTagNodeItemExpansion(TSharedPtr<FGameplayTagNode> Node, bool bExpand)
{
	if (Node.IsValid() && TagTreeWidget.IsValid())
	{
		TagTreeWidget->SetItemExpansion(Node, bExpand);

		const TArray< TSharedPtr<FGameplayTagNode> >& ChildTags = Node->GetChildTagNodes();
		for (int32 ChildIdx = 0; ChildIdx < ChildTags.Num(); ++ChildIdx)
		{
			SetTagNodeItemExpansion(ChildTags[ChildIdx], bExpand);
		}
	}
}

void SGameplayTagWidget::VerifyAssetTagValidity()
{
	TSet<FName> LibraryTags;

	// Create a set that is the library of all valid tags
	TArray< TSharedPtr<FGameplayTagNode> > NodeStack;
	NodeStack.Append(IGameplayTagsModule::Get().GetGameplayTagsManager().GameplayRootTags);

	while (NodeStack.Num() > 0)
	{
		TSharedPtr<FGameplayTagNode> CurNode = NodeStack.Pop();
		if (CurNode.IsValid())
		{
			LibraryTags.Add(CurNode->GetCompleteTag());
			NodeStack.Append(CurNode->GetChildTagNodes());
		}
	}

	// Find and remove any tags on the asset that are no longer in the library
	for (int32 ContainerIdx = 0; ContainerIdx < TagContainers.Num(); ++ContainerIdx)
	{
		UObject* OwnerObj = TagContainers[ContainerIdx].TagContainerOwner;
		FGameplayTagContainer* Container = TagContainers[ContainerIdx].TagContainer;

		if (OwnerObj && Container)
		{
			TSet<FName> AssetTags;
			Container->GetTags(AssetTags);
			
			TSet<FName> InvalidTags = AssetTags.Difference(LibraryTags);
			if (InvalidTags.Num() > 0)
			{
				FString InvalidTagNames;

				OwnerObj->PreEditChange(NULL);
				for (TSet<FName>::TConstIterator InvalidIter(InvalidTags); InvalidIter; ++InvalidIter)
				{
					Container->RemoveTag(*InvalidIter);
					InvalidTagNames += InvalidIter->ToString() + TEXT("\n");					
				}
				OwnerObj->PostEditChange();

				FFormatNamedArguments Arguments;
				Arguments.Add(TEXT("Objects"), FText::FromString( InvalidTagNames ));				
				FText DialogText = FText::Format( LOCTEXT("GameplayTagWidget_InvalidTags", "Invalid Tags that have been removed: \n\n{Objects}"), Arguments );
				OpenMsgDlgInt( EAppMsgType::Ok, DialogText, LOCTEXT("GameplayTagWidget_Warning", "Warning") );
			}
		}
	}
}

void SGameplayTagWidget::LoadSettings()
{
	TArray< TSharedPtr<FGameplayTagNode> >& TagArray = IGameplayTagsModule::Get().GetGameplayTagsManager().GameplayRootTags;
	for (int32 TagIdx = 0; TagIdx < TagArray.Num(); ++TagIdx)
	{
		LoadTagNodeItemExpansion(TagArray[TagIdx] );
	}
}

void SGameplayTagWidget::LoadTagNodeItemExpansion( TSharedPtr<FGameplayTagNode> Node )
{
	if (Node.IsValid() && TagTreeWidget.IsValid())
	{
		bool bExpanded = false;

		if( GConfig->GetBool(*SettingsIniSection, *(TagContainerName + Node->GetCompleteTag().ToString() + TEXT(".Expanded")), bExpanded, GEditorUserSettingsIni) )
		{
			TagTreeWidget->SetItemExpansion( Node, bExpanded );
		}
		else if( IsTagChecked( Node ) ) // If we have no save data but its ticked then we probably lost our settings so we shall expand it
		{
			TagTreeWidget->SetItemExpansion( Node, true );
		}

		const TArray< TSharedPtr<FGameplayTagNode> >& ChildTags = Node->GetChildTagNodes();
		for (int32 ChildIdx = 0; ChildIdx < ChildTags.Num(); ++ChildIdx)
		{
			LoadTagNodeItemExpansion(ChildTags[ChildIdx]);
		}
	}
}

void SGameplayTagWidget::OnExpansionChanged( TSharedPtr<FGameplayTagNode> InItem, bool bIsExpanded )
{
	// Save the new expansion setting to ini file
	GConfig->SetBool(*SettingsIniSection, *(TagContainerName + InItem->GetCompleteTag().ToString() + TEXT(".Expanded")), bIsExpanded, GEditorUserSettingsIni);
}

#undef LOCTEXT_NAMESPACE
