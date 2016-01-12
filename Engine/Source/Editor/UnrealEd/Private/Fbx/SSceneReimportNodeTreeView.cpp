// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "SSceneReimportNodeTreeView.h"
#include "ClassIconFinder.h"
#include "SSceneImportStaticMeshListView.h"

#define LOCTEXT_NAMESPACE "SFbxReimportSceneTreeView"

SFbxReimportSceneTreeView::~SFbxReimportSceneTreeView()
{
	FbxRootNodeArray.Empty();
	SceneInfo = nullptr;
	SceneInfoOriginal = nullptr;
	NodeStatusMap = nullptr;
	
	//Clear the internal data it is all SharedPtr
	NodeTreeData.Empty();
}

void SFbxReimportSceneTreeView::Construct(const SFbxReimportSceneTreeView::FArguments& InArgs)
{
	SceneInfo = InArgs._SceneInfo;
	SceneInfoOriginal = InArgs._SceneInfoOriginal;
	NodeStatusMap = InArgs._NodeStatusMap;
	//Build the FbxNodeInfoPtr tree data
	check(SceneInfo.IsValid());
	check(SceneInfoOriginal.IsValid());
	check(NodeStatusMap != nullptr);

	TMap<FString, TSharedPtr<FTreeNodeValue>> NodeTreePath;
	//Build hierarchy path
	for (FbxNodeInfoPtr NodeInfo : SceneInfo->HierarchyInfo)
	{
		TSharedPtr<FTreeNodeValue> NodeValue = MakeShareable(new FTreeNodeValue());
		NodeValue->CurrentNode = NodeInfo;
		NodeTreeData.Add(NodeInfo, NodeValue);
		NodeValue->OriginalNode = nullptr;
		NodeTreePath.Add(NodeInfo->NodeHierarchyPath, NodeValue);
	}
	//Build hierarchy path for original fbx
	for (FbxNodeInfoPtr NodeInfo : SceneInfoOriginal->HierarchyInfo)
	{
		TSharedPtr<FTreeNodeValue> NodeValue = nullptr;
		if (NodeTreePath.Contains(NodeInfo->NodeHierarchyPath))
		{
			NodeValue = *(NodeTreePath.Find(NodeInfo->NodeHierarchyPath));
		}
		else
		{
			NodeValue = MakeShareable(new FTreeNodeValue());
			NodeValue->CurrentNode = nullptr;
			NodeTreePath.Add(NodeInfo->NodeHierarchyPath, NodeValue);
		}
		NodeValue->OriginalNode = NodeInfo;
		NodeTreeData.Add(NodeInfo, NodeValue);
	}

	//build the status array
	TArray<TSharedPtr<FTreeNodeValue>> NodeTreeValues;
	NodeTreePath.GenerateValueArray(NodeTreeValues);
	for (TSharedPtr<FTreeNodeValue> NodeValue : NodeTreeValues)
	{
		EFbxSceneReimportStatusFlags NodeStatus = EFbxSceneReimportStatusFlags::None;
		if (NodeValue->CurrentNode.IsValid())
		{
			if (!NodeValue->CurrentNode->ParentNodeInfo.IsValid())
			{
				//Add root node to the UI tree view
				FbxRootNodeArray.Add(NodeValue->CurrentNode);
			}
			if (NodeValue->OriginalNode.IsValid())
			{
				NodeStatus |= EFbxSceneReimportStatusFlags::Same;
				if (NodeValue->OriginalNode->bImportNode)
				{
					NodeStatus |= EFbxSceneReimportStatusFlags::ReimportAsset;
				}
			}
			else
			{
				//by default reimport new node user can uncheck them
				NodeStatus |= EFbxSceneReimportStatusFlags::Added | EFbxSceneReimportStatusFlags::ReimportAsset;
			}
			NodeStatusMap->Add(NodeValue->CurrentNode->NodeHierarchyPath, NodeStatus);
		}
		else if (NodeValue->OriginalNode.IsValid())
		{
			if (!NodeValue->OriginalNode->ParentNodeInfo.IsValid())
			{
				//Add root node to the UI tree view
				FbxRootNodeArray.Add(NodeValue->OriginalNode);
			}
			NodeStatus |= EFbxSceneReimportStatusFlags::Removed;
			if (NodeValue->OriginalNode->bImportNode)
			{
				NodeStatus |= EFbxSceneReimportStatusFlags::ReimportAsset;
			}
			NodeStatusMap->Add(NodeValue->OriginalNode->NodeHierarchyPath, NodeStatus);
		}
		
	}
	NodeTreePath.Empty();

	STreeView::Construct
	(
		STreeView::FArguments()
		.TreeItemsSource(&FbxRootNodeArray)
		.SelectionMode(ESelectionMode::Multi)
		.OnGenerateRow(this, &SFbxReimportSceneTreeView::OnGenerateRowFbxSceneTreeView)
		.OnGetChildren(this, &SFbxReimportSceneTreeView::OnGetChildrenFbxSceneTreeView)
		.OnContextMenuOpening(this, &SFbxReimportSceneTreeView::OnOpenContextMenu)
		.OnSelectionChanged(this, &SFbxReimportSceneTreeView::OnSelectionChanged)
	);
}

/** The item used for visualizing the class in the tree. */
class SFbxReimportSceneTreeViewItem : public STableRow< FbxNodeInfoPtr >
{
public:

	SLATE_BEGIN_ARGS(SFbxReimportSceneTreeViewItem)
		: _FbxNodeInfo(nullptr)
		, _NodeStatusMap(nullptr)
	{}

	/** The item content. */
	SLATE_ARGUMENT(FbxNodeInfoPtr, FbxNodeInfo)
	SLATE_ARGUMENT(FbxSceneReimportStatusMapPtr, NodeStatusMap)
	SLATE_END_ARGS()

	/**
	* Construct the widget
	*
	* @param InArgs   A declaration from which to construct the widget
	*/
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
	{
		FbxNodeInfo = InArgs._FbxNodeInfo;
		NodeStatusMap = InArgs._NodeStatusMap;
		
		//This is suppose to always be valid
		check(FbxNodeInfo.IsValid());
		check(NodeStatusMap != nullptr);

		UClass *IconClass = AActor::StaticClass();
		if (FbxNodeInfo->AttributeInfo.IsValid())
		{
			IconClass = FbxNodeInfo->AttributeInfo->GetType();
		}
		const FSlateBrush* ClassIcon = FClassIconFinder::FindIconForClass(IconClass);

		this->ChildSlot
		[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SCheckBox)
				.OnCheckStateChanged(this, &SFbxReimportSceneTreeViewItem::OnItemCheckChanged)
				.IsChecked(this, &SFbxReimportSceneTreeViewItem::IsItemChecked)
			]
		+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SExpanderArrow, SharedThis(this))
			]

		+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.0f, 2.0f, 6.0f, 2.0f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				[
					SNew(SImage)
					.Image(ClassIcon)
					.Visibility(ClassIcon != FEditorStyle::GetDefaultBrush() ? EVisibility::Visible : EVisibility::Collapsed)
				]
				+ SOverlay::Slot()
				.HAlign(HAlign_Left)
				[
					SNew(SImage)
					.Image(this, &SFbxReimportSceneTreeViewItem::GetIconOverlay)
				]
			]
		+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(0.0f, 3.0f, 6.0f, 3.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FbxNodeInfo->NodeName))
				.ToolTipText(this, &SFbxReimportSceneTreeViewItem::GetTooltip)
			]

		];

		STableRow< FbxNodeInfoPtr >::ConstructInternal(
			STableRow::FArguments()
			.ShowSelection(true),
			InOwnerTableView
		);
	}

private:
	FText GetTooltip() const
	{
		FString TooltipText = FbxNodeInfo->NodeName;
		if (NodeStatusMap->Contains(FbxNodeInfo->NodeHierarchyPath))
		{
			EFbxSceneReimportStatusFlags ReimportFlags = *NodeStatusMap->Find(FbxNodeInfo->NodeHierarchyPath);
			//The remove only should not be possible this is why there is no remove only case

			if ((ReimportFlags & EFbxSceneReimportStatusFlags::Added) != EFbxSceneReimportStatusFlags::None)
			{
				TooltipText += LOCTEXT("SFbxReimportSceneTreeViewItem_AddedTooltip", " Will be add to the blueprint hierarchy.").ToString();
			}
			else if ((ReimportFlags & EFbxSceneReimportStatusFlags::Same) != EFbxSceneReimportStatusFlags::None)
			{
				//No tooltip because the item will not change
			}
			else if ((ReimportFlags & EFbxSceneReimportStatusFlags::Removed) != EFbxSceneReimportStatusFlags::None)
			{
				TooltipText += LOCTEXT("SFbxReimportSceneTreeViewItem_RemovedTooltip", " Will be remove from the blueprint hierarchy.").ToString();
			}
		}
		return FText::FromString(TooltipText);
	}
	const FSlateBrush *GetIconOverlay() const
	{
		const FSlateBrush *SlateBrush = FEditorStyle::GetBrush("FBXIcon.ReimportError");
		if (NodeStatusMap->Contains(FbxNodeInfo->NodeHierarchyPath))
		{
			EFbxSceneReimportStatusFlags ReimportFlags = *NodeStatusMap->Find(FbxNodeInfo->NodeHierarchyPath);
			//The remove only should not be possible this is why there is no remove only case

			if ((ReimportFlags & EFbxSceneReimportStatusFlags::Added) != EFbxSceneReimportStatusFlags::None)
			{
				SlateBrush = FEditorStyle::GetBrush("FBXIcon.ReimportAdded");
			}
			else if ((ReimportFlags & EFbxSceneReimportStatusFlags::Same) != EFbxSceneReimportStatusFlags::None)
			{
				SlateBrush = FEditorStyle::GetBrush("FBXIcon.ReimportSameContent");
			}
			else if ((ReimportFlags & EFbxSceneReimportStatusFlags::Removed) != EFbxSceneReimportStatusFlags::None)
			{
				SlateBrush = FEditorStyle::GetBrush("FBXIcon.ReimportRemovedContent");
			}
		}
		return SlateBrush;
	}

	void OnItemCheckChanged(ECheckBoxState CheckType)
	{
		if (!FbxNodeInfo.IsValid() || !NodeStatusMap->Contains(FbxNodeInfo->NodeHierarchyPath))
			return;
		EFbxSceneReimportStatusFlags *StatusFlag = NodeStatusMap->Find(FbxNodeInfo->NodeHierarchyPath);
		if (CheckType == ECheckBoxState::Checked)
		{
			*StatusFlag = *StatusFlag | EFbxSceneReimportStatusFlags::ReimportAsset;
		}
		else
		{
			*StatusFlag = *StatusFlag & ~EFbxSceneReimportStatusFlags::ReimportAsset;
		}
	}

	ECheckBoxState IsItemChecked() const
	{
		if (NodeStatusMap->Contains(FbxNodeInfo->NodeHierarchyPath))
		{
			return (*(NodeStatusMap->Find(FbxNodeInfo->NodeHierarchyPath)) & EFbxSceneReimportStatusFlags::ReimportAsset) != EFbxSceneReimportStatusFlags::None ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		}
		return ECheckBoxState::Unchecked;
	}

	/** The node info to build the tree view row from. */
	FbxNodeInfoPtr FbxNodeInfo;

	FbxSceneReimportStatusMapPtr NodeStatusMap;
};

TSharedRef< ITableRow > SFbxReimportSceneTreeView::OnGenerateRowFbxSceneTreeView(FbxNodeInfoPtr Item, const TSharedRef< STableViewBase >& OwnerTable)
{
	TSharedRef< SFbxReimportSceneTreeViewItem > ReturnRow = SNew(SFbxReimportSceneTreeViewItem, OwnerTable)
		.FbxNodeInfo(Item)
		.NodeStatusMap(NodeStatusMap);
	return ReturnRow;
}

void SFbxReimportSceneTreeView::OnGetChildrenFbxSceneTreeView(FbxNodeInfoPtr InParent, TArray< FbxNodeInfoPtr >& OutChildren)
{
	check(NodeTreeData.Contains(InParent));
	TSharedPtr<FTreeNodeValue> NodeValue = *(NodeTreeData.Find(InParent));
	TArray<FString> ChildNames;
	//Current node contain the add and same node
	if (NodeValue->CurrentNode.IsValid())
	{
		for (FbxNodeInfoPtr Child : NodeValue->CurrentNode->Childrens)
		{
			if (Child.IsValid())
			{
				ChildNames.Add(Child->NodeName);
				OutChildren.Add(Child);
			}
		}
	}
	//Original node is use for finding the remove nodes
	if (NodeValue->OriginalNode.IsValid())
	{
		for (FbxNodeInfoPtr Child : NodeValue->OriginalNode->Childrens)
		{
			//We have a delete node
			if (Child.IsValid() && !ChildNames.Contains(Child->NodeName))
			{
				OutChildren.Add(Child);
			}
		}
	}
}

void SFbxReimportSceneTreeView::OnToggleSelectAll(ECheckBoxState CheckType)
{
	//check all actor for import
	TArray<TSharedPtr<FTreeNodeValue>> NodeValues;
	NodeTreeData.GenerateValueArray(NodeValues);
	for (TSharedPtr<FTreeNodeValue> NodeValue: NodeValues)
	{
		FbxNodeInfoPtr NodeInfo;
		if (NodeValue->CurrentNode.IsValid())
		{
			NodeInfo = NodeValue->CurrentNode;
		}
		else
		{
			NodeInfo = NodeValue->OriginalNode;
		}
		
		//We should never see no current and no original node
		check(NodeInfo.IsValid());

		EFbxSceneReimportStatusFlags *ItemStatus = NodeStatusMap->Find(NodeInfo->NodeHierarchyPath);
		if (ItemStatus != nullptr)
		{
			*ItemStatus = (CheckType == ECheckBoxState::Checked) ? *ItemStatus | EFbxSceneReimportStatusFlags::ReimportAsset : *ItemStatus & ~EFbxSceneReimportStatusFlags::ReimportAsset;
		}
	}
}

void RecursiveSetExpand(SFbxReimportSceneTreeView *TreeView, FbxNodeInfoPtr NodeInfoPtr, bool ExpandState)
{
	TreeView->SetItemExpansion(NodeInfoPtr, ExpandState);
	for (auto Child : NodeInfoPtr->Childrens)
	{
		RecursiveSetExpand(TreeView, Child, ExpandState);
	}
}

FReply SFbxReimportSceneTreeView::OnExpandAll()
{
	for (auto NodeInfoIt = SceneInfo->HierarchyInfo.CreateIterator(); NodeInfoIt; ++NodeInfoIt)
	{
		FbxNodeInfoPtr NodeInfo = (*NodeInfoIt);

		if (!NodeInfo->ParentNodeInfo.IsValid())
		{
			RecursiveSetExpand(this, NodeInfo, true);
		}
	}
	return FReply::Handled();
}

FReply SFbxReimportSceneTreeView::OnCollapseAll()
{
	for (auto NodeInfoIt = SceneInfo->HierarchyInfo.CreateIterator(); NodeInfoIt; ++NodeInfoIt)
	{
		FbxNodeInfoPtr NodeInfo = (*NodeInfoIt);

		if (!NodeInfo->ParentNodeInfo.IsValid())
		{
			RecursiveSetExpand(this, NodeInfo, false);
		}
	}
	return FReply::Handled();
}

TSharedPtr<SWidget> SFbxReimportSceneTreeView::OnOpenContextMenu()
{
	// Build up the menu for a selection
	const bool bCloseAfterSelection = true;
	FMenuBuilder MenuBuilder(bCloseAfterSelection, TSharedPtr<FUICommandList>());

	//Get the different type of the multi selection
	TSet<TSharedPtr<FFbxAttributeInfo>> SelectAssets;
	TArray<FbxNodeInfoPtr> SelectedItems;
	const auto NumSelectedItems = GetSelectedItems(SelectedItems);
	for (auto Item : SelectedItems)
	{
		FbxNodeInfoPtr ItemPtr = Item;
		if (ItemPtr->AttributeInfo.IsValid())
		{
			SelectAssets.Add(ItemPtr->AttributeInfo);
		}
	}

	// We always create a section here, even if there is no parent so that clients can still extend the menu
	MenuBuilder.BeginSection("FbxSceneTreeViewContextMenuImportSection");
	{
		const FSlateIcon PlusIcon(FEditorStyle::GetStyleSetName(), "Plus");
		MenuBuilder.AddMenuEntry(LOCTEXT("CheckForImport", "Add Selection To Import"), FText(), PlusIcon, FUIAction(FExecuteAction::CreateSP(this, &SFbxReimportSceneTreeView::AddSelectionToImport)));
		const FSlateIcon MinusIcon(FEditorStyle::GetStyleSetName(), "PropertyWindow.Button_RemoveFromArray");
		MenuBuilder.AddMenuEntry(LOCTEXT("UncheckForImport", "Remove Selection From Import"), FText(), MinusIcon, FUIAction(FExecuteAction::CreateSP(this, &SFbxReimportSceneTreeView::RemoveSelectionFromImport)));
	}
	MenuBuilder.EndSection();
/*
	if (SelectAssets.Num() > 0)
	{
		MenuBuilder.AddMenuSeparator();
		MenuBuilder.BeginSection("FbxSceneTreeViewContextMenuGotoAssetSection", LOCTEXT("Gotoasset","Go to asset:"));
		{
			for (auto AssetItem : SelectAssets)
			{
				//const FSlateBrush* ClassIcon = FClassIconFinder::FindIconForClass(AssetItem->GetType());
				MenuBuilder.AddMenuEntry(FText::FromString(AssetItem->Name), FText(), FSlateIcon(), FUIAction(FExecuteAction::CreateSP(this, &SFbxReimportSceneTreeView::GotoAsset, AssetItem)));
			}
		}
		MenuBuilder.EndSection();
	}
*/
	

	return MenuBuilder.MakeWidget();
}

void SFbxReimportSceneTreeView::AddSelectionToImport()
{
	SetSelectionImportState(true);
}

void SFbxReimportSceneTreeView::RemoveSelectionFromImport()
{
	SetSelectionImportState(false);
}

void SFbxReimportSceneTreeView::SetSelectionImportState(bool MarkForImport)
{
	TArray<FbxNodeInfoPtr> SelectedItems;
	GetSelectedItems(SelectedItems);
	for (FbxNodeInfoPtr Item : SelectedItems)
	{
		EFbxSceneReimportStatusFlags *ItemStatus = NodeStatusMap->Find(Item->NodeHierarchyPath);
		if (ItemStatus != nullptr)
		{
			*ItemStatus = MarkForImport ? *ItemStatus | EFbxSceneReimportStatusFlags::ReimportAsset : *ItemStatus & ~EFbxSceneReimportStatusFlags::ReimportAsset;
		}
	}
}

void SFbxReimportSceneTreeView::OnSelectionChanged(FbxNodeInfoPtr Item, ESelectInfo::Type SelectionType)
{
}

void SFbxReimportSceneTreeView::GotoAsset(TSharedPtr<FFbxAttributeInfo> AssetAttribute)
{
	//Switch to the asset tab and select the asset
}

#undef LOCTEXT_NAMESPACE
