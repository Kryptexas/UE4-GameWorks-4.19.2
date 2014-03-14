// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "SceneOutlinerPrivatePCH.h"
#include "SceneOutlinerTreeItems.h"

#define LOCTEXT_NAMESPACE "SceneOutlinerGutter"

class FVisibilityDragDropOp : public FDragDropOperation, public TSharedFromThis<FVisibilityDragDropOp>
{
public:
	
	/** Flag which defines whether to hide destination actors or not */
	bool bHidden;

	/** Undo transaction stolen from the gutter which is kept alive for the duration of the drag */
	TUniquePtr<FScopedTransaction> UndoTransaction;

	/** Get the type ID for this drag/drop operation */
	static FString GetTypeId() { static FString Type = TEXT("FVisibilityDragDropOp"); return Type; }

	/** The widget decorator to use */
	virtual TSharedPtr<SWidget> GetDefaultDecorator() const OVERRIDE
	{
		return SNullWidget::NullWidget;
	}

	/** Create a new drag and drop operation out of the specified flag */
	static TSharedRef<FVisibilityDragDropOp> New(const bool _bHidden, TUniquePtr<FScopedTransaction>& ScopedTransaction)
	{
		TSharedRef<FVisibilityDragDropOp> Operation = MakeShareable(new FVisibilityDragDropOp);
		FSlateApplication::GetDragDropReflector().RegisterOperation<FVisibilityDragDropOp>(Operation);

		Operation->bHidden = _bHidden;
		Operation->UndoTransaction = MoveTemp(ScopedTransaction);

		Operation->Construct();
		return Operation;
	}
};

/** Widget responsible for managing the visibility for a single actor */
class SVisibilityWidget : public SImage
{
public:
	SLATE_BEGIN_ARGS(SVisibilityWidget){}
		SLATE_ARGUMENT(TWeakPtr<SceneOutliner::TOutlinerTreeItem>, TreeItem);
	SLATE_END_ARGS()

	/** Construct this widget */
	void Construct(const FArguments& InArgs)
	{
		WeakTreeItem = InArgs._TreeItem;

		SImage::Construct(
			SImage::FArguments()
				.Image(this, &SVisibilityWidget::GetBrush)
		);
	}

	/** Check if the specified actor is visible */
	static bool IsActorVisible(const AActor* Actor)
	{
		return Actor && !Actor->IsTemporarilyHiddenInEditor();
	}

private:

	/** Start a new drag/drop operation for this widget */
	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) OVERRIDE
	{
		if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
		{
			return FReply::Handled().BeginDragDrop(FVisibilityDragDropOp::New(!IsVisible(), UndoTransaction));
		}
		else
		{
			return FReply::Unhandled();
		}
	}

	/** If a visibility drag drop operation has entered this widget, set its actor to the new visibility state */
	virtual void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) OVERRIDE
	{
		if (DragDrop::IsTypeMatch<FVisibilityDragDropOp>(DragDropEvent.GetOperation()))
		{
			TSharedPtr<FVisibilityDragDropOp> DragOp = StaticCastSharedPtr<FVisibilityDragDropOp>(DragDropEvent.GetOperation());
			SetIsVisible(!DragOp->bHidden);
		}
	}

	/** Called when the mouse button is pressed down on this widget */
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) OVERRIDE
	{
		if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
		{
			return FReply::Unhandled();
		}

		// Open an undo transaction
		UndoTransaction.Reset(new FScopedTransaction(LOCTEXT("SetActorVisibility", "Set Actor Visibility")));

		SetIsVisible(!IsVisible());

		return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
	}

	/** Process a mouse up message */
	virtual FReply OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE
	{
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			UndoTransaction.Reset();
			return FReply::Handled();
		}

		return FReply::Unhandled();
	}

	/** Called when this widget had captured the mouse, but that capture has been revoked for some reason. */
	virtual void OnMouseCaptureLost() OVERRIDE
	{
		UndoTransaction.Reset();
	}

	/** Get the brush for this widget */
	const FSlateBrush* GetBrush() const
	{
		if (IsVisible())
		{
			return IsHovered() ? FEditorStyle::GetBrush( "Level.VisibleHighlightIcon16x" ) :
				FEditorStyle::GetBrush( "Level.VisibleIcon16x" );
		}
		else
		{
			return IsHovered() ? FEditorStyle::GetBrush( "Level.NotVisibleHighlightIcon16x" ) :
				FEditorStyle::GetBrush( "Level.NotVisibleIcon16x" );
		}
	}

	/** Check if the specified actor is visible */
	bool IsVisible() const
	{
		auto TreeItem = WeakTreeItem.Pin();
		if (TreeItem.IsValid())
		{
			return TreeItem->IsVisible();
		}
		return false;
	}

	/** Set the actor this widget is responsible for to be hidden or shown */
	void SetIsVisible(const bool bVisible)
	{
		if (IsVisible() != bVisible)
		{
			auto TreeItem = WeakTreeItem.Pin();
			if (TreeItem.IsValid())
			{
				TreeItem->SetIsVisible(bVisible);
			}

			GEditor->RedrawAllViewports();
		}
	}

	/** Tree item that we relate to */
	TWeakPtr<SceneOutliner::TOutlinerTreeItem> WeakTreeItem;

	/** Scoped undo transaction */
	TUniquePtr<FScopedTransaction> UndoTransaction;
};

FSceneOutlinerGutter::FSceneOutlinerGutter()
{

}

FName FSceneOutlinerGutter::GetColumnID()
{
	return FName("Gutter");
}

SHeaderRow::FColumn::FArguments FSceneOutlinerGutter::ConstructHeaderRowColumn()
{
	return SHeaderRow::Column(GetColumnID())[ SNew(SSpacer) ];
}

const TSharedRef<SWidget> FSceneOutlinerGutter::ConstructRowWidget(const TSharedRef<SceneOutliner::TOutlinerTreeItem> TreeItem)
{
	return SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SVisibilityWidget)
			.TreeItem(TreeItem)
		];
}

void FSceneOutlinerGutter::SortItems(TArray<TSharedPtr<SceneOutliner::TOutlinerTreeItem>>& RootItems, const EColumnSortMode::Type SortMode) const
{
	using namespace SceneOutliner;
	struct SortPredicate
	{
		/** Constructor */
		SortPredicate(const bool InSence) : bSense(InSence) {}

		/** The sense of the sort (identical items are always sorted by name, Ascending) */
		bool bSense;

		/** Sort predicate operator */
		bool operator ()(TSharedPtr<TOutlinerTreeItem> A, TSharedPtr<TOutlinerTreeItem> B) const
		{
			if ((A->Type == TOutlinerTreeItem::Folder) != (B->Type == TOutlinerTreeItem::Folder))
			{
				// Folders appear first
				return A->Type == TOutlinerTreeItem::Folder;
			}

			const bool VisibleA = A->IsVisible();
			const bool VisibleB = A->IsVisible();

			if (VisibleA == VisibleB)
			{
				// Always sort identical items by name
				return GetLabelForItem(A.ToSharedRef()).ToString() < GetLabelForItem(B.ToSharedRef()).ToString();
			}
			else
			{
				return bSense == VisibleA;
			}
		}
	};

	if (SortMode == EColumnSortMode::Ascending)
	{
		RootItems.Sort(SortPredicate(true));
	}
	else if (SortMode == EColumnSortMode::Descending)
	{
		RootItems.Sort(SortPredicate(false));
	}
}

#undef LOCTEXT_NAMESPACE
