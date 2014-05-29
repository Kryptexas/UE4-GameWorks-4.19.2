
#include "PropertyEditorPrivatePCH.h"
#include "SDetailsViewBase.h"
#include "AssetSelection.h"
#include "PropertyNode.h"
#include "ItemPropertyNode.h"
#include "CategoryPropertyNode.h"
#include "ObjectPropertyNode.h"
#include "ScopedTransaction.h"
#include "AssetThumbnail.h"
#include "SDetailNameArea.h"
#include "IPropertyUtilities.h"
#include "PropertyEditorHelpers.h"
#include "PropertyEditor.h"
#include "PropertyDetailsUtilities.h"
#include "SPropertyEditorEditInline.h"
#include "ObjectEditorUtils.h"

SDetailsViewBase::~SDetailsViewBase()
{
	if (ThumbnailPool.IsValid())
	{
		ThumbnailPool->ReleaseResources();
	}
}


void SDetailsViewBase::OnGetChildrenForDetailTree(TSharedRef<IDetailTreeNode> InTreeNode, TArray< TSharedRef<IDetailTreeNode> >& OutChildren)
{
	InTreeNode->GetChildren(OutChildren);
}

TSharedRef<ITableRow> SDetailsViewBase::OnGenerateRowForDetailTree(TSharedRef<IDetailTreeNode> InTreeNode, const TSharedRef<STableViewBase>& OwnerTable)
{
	return InTreeNode->GenerateNodeWidget(OwnerTable, ColumnSizeData, PropertyUtilities.ToSharedRef());
}

void SDetailsViewBase::SetRootExpansionStates(const bool bExpand, const bool bRecurse)
{
	for (auto Iter = RootTreeNodes.CreateIterator(); Iter; ++Iter)
	{
		SetNodeExpansionState(*Iter, bExpand, bRecurse);
	}
}

void SDetailsViewBase::SetNodeExpansionState(TSharedRef<IDetailTreeNode> InTreeNode, bool bIsItemExpanded, bool bRecursive)
{
	TArray< TSharedRef<IDetailTreeNode> > Children;
	InTreeNode->GetChildren(Children);

	if (Children.Num())
	{
		RequestItemExpanded(InTreeNode, bIsItemExpanded);
		InTreeNode->OnItemExpansionChanged(bIsItemExpanded);

		if (bRecursive)
		{
			for (int32 ChildIndex = 0; ChildIndex < Children.Num(); ++ChildIndex)
			{
				TSharedRef<IDetailTreeNode> Child = Children[ChildIndex];

				SetNodeExpansionState(Child, bIsItemExpanded, bRecursive);
			}
		}
	}
}

void SDetailsViewBase::SetNodeExpansionStateRecursive(TSharedRef<IDetailTreeNode> InTreeNode, bool bIsItemExpanded)
{
	SetNodeExpansionState(InTreeNode, bIsItemExpanded, true);
}

void SDetailsViewBase::OnItemExpansionChanged(TSharedRef<IDetailTreeNode> InTreeNode, bool bIsItemExpanded)
{
	SetNodeExpansionState(InTreeNode, bIsItemExpanded, false);
}

FReply SDetailsViewBase::OnLockButtonClicked()
{
	bIsLocked = !bIsLocked;
	return FReply::Handled();
}

void SDetailsViewBase::HideFilterArea(bool bHide)
{
	DetailsViewArgs.bAllowSearch = !bHide;
}

EVisibility SDetailsViewBase::GetTreeVisibility() const
{
	return DetailLayout.IsValid() && DetailLayout->HasDetails() ? EVisibility::Visible : EVisibility::Collapsed;
}

/** Returns the image used for the icon on the filter button */
const FSlateBrush* SDetailsViewBase::OnGetFilterButtonImageResource() const
{
	if (bHasActiveFilter)
	{
		return FEditorStyle::GetBrush(TEXT("PropertyWindow.FilterCancel"));
	}
	else
	{
		return FEditorStyle::GetBrush(TEXT("PropertyWindow.FilterSearch"));
	}
}

void SDetailsViewBase::EnqueueDeferredAction(FSimpleDelegate& DeferredAction)
{
	DeferredActions.AddUnique(DeferredAction);
}

void SDetailsViewBase::OnColorPickerWindowClosed(const TSharedRef<SWindow>& Window)
{
	// A color picker window is no longer open
	bHasOpenColorPicker = false;
	ColorPropertyNode.Reset();
}


void SDetailsViewBase::SetIsPropertyVisibleDelegate(FIsPropertyVisible InIsPropertyVisible)
{
	IsPropertyVisibleDelegate = InIsPropertyVisible;
}

void SDetailsViewBase::SetIsPropertyEditingEnabledDelegate(FIsPropertyEditingEnabled IsPropertyEditingEnabled)
{
	IsPropertyEditingEnabledDelegate = IsPropertyEditingEnabled;
}

bool SDetailsViewBase::IsPropertyEditingEnabled() const
{
	// If the delegate is not bound assume property editing is enabled, otherwise ask the delegate
	return !IsPropertyEditingEnabledDelegate.IsBound() || IsPropertyEditingEnabledDelegate.Execute();
}

void SDetailsViewBase::SetGenericLayoutDetailsDelegate(FOnGetDetailCustomizationInstance OnGetGenericDetails)
{
	GenericLayoutDelegate = OnGetGenericDetails;
}

TSharedPtr<FAssetThumbnailPool> SDetailsViewBase::GetThumbnailPool() const
{
	if (!ThumbnailPool.IsValid())
	{
		// Create a thumbnail pool for the view if it doesnt exist.  This does not use resources of no thumbnails are used
		ThumbnailPool = MakeShareable(new FAssetThumbnailPool(50, TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &SDetailsView::IsHovered))));
	}

	return ThumbnailPool;
}

void SDetailsViewBase::NotifyFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
{
	OnFinishedChangingPropertiesDelegate.Broadcast(PropertyChangedEvent);
}

void SDetailsViewBase::RequestItemExpanded(TSharedRef<IDetailTreeNode> TreeNode, bool bExpand)
{
	// Don't change expansion state if its already in that state
	if (DetailTree->IsItemExpanded(TreeNode) != bExpand)
	{
		FilteredNodesRequestingExpansionState.Add(TreeNode, bExpand);
	}
}

void SDetailsViewBase::RefreshTree()
{
	DetailTree->RequestTreeRefresh();
}

void SDetailsViewBase::SaveCustomExpansionState(const FString& NodePath, bool bIsExpanded)
{
	if (bIsExpanded)
	{
		ExpandedDetailNodes.Add(NodePath);
	}
	else
	{
		ExpandedDetailNodes.Remove(NodePath);
	}
}

bool SDetailsViewBase::GetCustomSavedExpansionState(const FString& NodePath) const
{
	return ExpandedDetailNodes.Contains(NodePath);
}

bool SDetailsViewBase::IsPropertyVisible(const UProperty* Property) const
{
	return IsPropertyVisibleDelegate.IsBound() ? IsPropertyVisibleDelegate.Execute(Property) : true;
}

TSharedPtr<IPropertyUtilities> SDetailsViewBase::GetPropertyUtilities()
{
	return PropertyUtilities;
}
