// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// SFixedSizeConstraintCanvas

class SFixedSizeConstraintCanvas : public SConstraintCanvas
{
	SLATE_BEGIN_ARGS(SFixedSizeConstraintCanvas)
		{
			_Visibility = EVisibility::SelfHitTestInvisible;
		}
	SLATE_END_ARGS()

	FVector2D CanvasSize;

	void Construct(const FArguments& InArgs, FVector2D InSize)
	{
		SConstraintCanvas::FArguments ParentArgs;
		SConstraintCanvas::Construct(ParentArgs);

		CanvasSize = InSize;
	}

	void SetDesiredSize(FVector2D InSize)
	{
		CanvasSize = InSize;
	}

	// SWidget interface
	virtual FVector2D ComputeDesiredSize() const override
	{
		return CanvasSize;
	}
	// End of SWidget interface
};

/////////////////////////////////////////////////////
// UCanvasPanel

UCanvasPanel::UCanvasPanel(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsVariable = false;
	DesiredCanvasSize = FVector2D(128.0f, 128.0f);

	SConstraintCanvas::FArguments Defaults;
	Visiblity = UWidget::ConvertRuntimeToSerializedVisiblity(Defaults._Visibility.Get());
}

UClass* UCanvasPanel::GetSlotClass() const
{
	return UCanvasPanelSlot::StaticClass();
}

void UCanvasPanel::OnSlotAdded(UPanelSlot* Slot)
{
	// Add the child to the live canvas if it already exists
	if ( MyCanvas.IsValid() )
	{
		Cast<UCanvasPanelSlot>(Slot)->BuildSlot(MyCanvas.ToSharedRef());
	}
}

void UCanvasPanel::OnSlotRemoved(UPanelSlot* Slot)
{
	// Remove the widget from the live slot if it exists.
	if ( MyCanvas.IsValid() )
	{
		MyCanvas->RemoveSlot(Slot->Content->GetWidget());
	}
}

TSharedRef<SWidget> UCanvasPanel::RebuildWidget()
{
	MyCanvas = SNew(SFixedSizeConstraintCanvas, DesiredCanvasSize);

	for ( UPanelSlot* Slot : Slots )
	{
		if ( UCanvasPanelSlot* TypedSlot = Cast<UCanvasPanelSlot>(Slot) )
		{
			TypedSlot->Parent = this;
			TypedSlot->BuildSlot(MyCanvas.ToSharedRef());
		}
	}

	return BuildDesignTimeWidget( MyCanvas.ToSharedRef() );
}

void UCanvasPanel::SyncronizeProperties()
{
	Super::SyncronizeProperties();

	MyCanvas->SetDesiredSize(DesiredCanvasSize);
}

TSharedPtr<SConstraintCanvas> UCanvasPanel::GetCanvasWidget() const
{
	return MyCanvas;
}

bool UCanvasPanel::GetGeometryForSlot(int32 SlotIndex, FGeometry& ArrangedGeometry) const
{
	UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Slots[SlotIndex]);
	return GetGeometryForSlot(Slot, ArrangedGeometry);
}

bool UCanvasPanel::GetGeometryForSlot(UCanvasPanelSlot* Slot, FGeometry& ArrangedGeometry) const
{
	if ( Slot->Content == NULL )
	{
		return false;
	}

	TSharedPtr<SConstraintCanvas> Canvas = GetCanvasWidget();
	if ( Canvas.IsValid() )
	{
		FArrangedChildren ArrangedChildren(EVisibility::All);
		Canvas->ArrangeChildren(Canvas->GetCachedGeometry(), ArrangedChildren);

		for ( int32 ChildIndex = 0; ChildIndex < ArrangedChildren.Num(); ChildIndex++ )
		{
			if ( ArrangedChildren(ChildIndex).Widget == Slot->Content->GetWidget() )
			{
				ArrangedGeometry = ArrangedChildren(ChildIndex).Geometry;
				return true;
			}
		}
	}

	return false;
}

#if WITH_EDITOR

const FSlateBrush* UCanvasPanel::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.Canvas");
}

#endif