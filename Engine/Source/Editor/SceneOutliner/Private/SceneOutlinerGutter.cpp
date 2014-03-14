// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "SceneOutlinerPrivatePCH.h"

#define LOCTEXT_NAMESPACE "SceneOutlinerGutter"

class FVisibilityDragDropOp : public FDragDropOperation, public TSharedFromThis<FVisibilityDragDropOp>
{
public:
	
	/** Flag which defines whether to hide destination actors or not */
	bool bHidden;

	/** Get the type ID for this drag/drop operation */
	static FString GetTypeId() { static FString Type = TEXT("FVisibilityDragDropOp"); return Type; }

	/** The widget decorator to use */
	virtual TSharedPtr<SWidget> GetDefaultDecorator() const OVERRIDE
	{
		return SNullWidget::NullWidget;
	}

	/** Create a new drag and drop operation out of the specified flag */
	static TSharedRef<FVisibilityDragDropOp> New(const bool _bHidden)
	{
		TSharedRef<FVisibilityDragDropOp> Operation = MakeShareable(new FVisibilityDragDropOp);
		FSlateApplication::GetDragDropReflector().RegisterOperation<FVisibilityDragDropOp>(Operation);

		Operation->bHidden = _bHidden;

		Operation->Construct();
		return Operation;
	}
};

/** Widget responsible for managing the visibility for a single actor */
class SVisibilityWidget : public SImage
{
public:
	SLATE_BEGIN_ARGS(SVisibilityWidget){}
		SLATE_ARGUMENT(TWeakObjectPtr<AActor>, Actor);
	SLATE_END_ARGS()

	/** Construct this widget */
	void Construct(const FArguments& InArgs)
	{
		WeakActor = InArgs._Actor;

		SImage::Construct(
			SImage::FArguments()
				.Image(this, &SVisibilityWidget::GetBrush)
		);
	}

	/** Check if the specified actor is visible */
	static bool IsActorVisible(TWeakObjectPtr<AActor> WeakActor)
	{
		const auto* Actor = WeakActor.Get();
		return Actor && !Actor->IsTemporarilyHiddenInEditor();
	}

private:

	/** Start a new drag/drop operation for this widget */
	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) OVERRIDE
	{
		const auto* Actor = WeakActor.Get();

		if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && Actor)
		{
			return FReply::Handled().BeginDragDrop(FVisibilityDragDropOp::New(Actor->IsTemporarilyHiddenInEditor()));
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
			SetActorHidden(DragOp->bHidden);
		}
	}

	/** Called when the mouse button is pressed down on this widget */
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) OVERRIDE
	{
		if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
		{
			return FReply::Unhandled();
		}

		auto* Actor = WeakActor.Get();
		if (Actor)
		{
			SetActorHidden(IsActorVisible());
		}

		return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
	}

	/** Get the brush for this widget */
	const FSlateBrush* GetBrush() const
	{
		if (IsActorVisible())
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
	bool IsActorVisible() const
	{
		return IsActorVisible(WeakActor);
	}

	/** Set the actor this widget is responsible for to be hidden or shown */
	void SetActorHidden(const bool bHidden)
	{
		if (IsActorVisible() == bHidden)
		{
			auto* Actor = WeakActor.Get();
			if (Actor)
			{
				// Save the actor to the transaction buffer to support undo/redo, but do
				// not call Modify, as we do not want to dirty the actor's package and
				// we're only editing temporary, transient values
				SaveToTransactionBuffer(Actor, false);
				Actor->SetIsTemporarilyHiddenInEditor( bHidden );
				GEditor->RedrawAllViewports();
			}
		}
	}

	/** Pointer to the actor that we relate to */
	TWeakObjectPtr<AActor> WeakActor;
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

const TSharedRef<SWidget> FSceneOutlinerGutter::ConstructRowWidget(const TWeakObjectPtr<AActor>& Actor)
{
	return SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SVisibilityWidget)
				.Actor(Actor)
		];
}

void FSceneOutlinerGutter::SortItems(TArray<TSharedPtr<SceneOutliner::TOutlinerTreeItem>>& RootItems, const EColumnSortMode::Type SortMode) const
{
	if (SortMode == EColumnSortMode::Ascending)
	{
		RootItems.Sort(
			[=](TSharedPtr<SceneOutliner::TOutlinerTreeItem> A, TSharedPtr<SceneOutliner::TOutlinerTreeItem> B)
			{
				return SVisibilityWidget::IsActorVisible(A->Actor);
			}
		);
	}
	else if (SortMode == EColumnSortMode::Descending)
	{
		RootItems.Sort(
			[=](TSharedPtr<SceneOutliner::TOutlinerTreeItem> A, TSharedPtr<SceneOutliner::TOutlinerTreeItem> B)
			{
				return SVisibilityWidget::IsActorVisible(B->Actor);
			}
		);
	}
}

#undef LOCTEXT_NAMESPACE
