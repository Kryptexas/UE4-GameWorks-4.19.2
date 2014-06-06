// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// SFixedSizeCanvas

class SFixedSizeCanvas : public SConstraintCanvas
{
	SLATE_BEGIN_ARGS( SFixedSizeCanvas )
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

	// SWidget interface
	virtual FVector2D ComputeDesiredSize() const OVERRIDE
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
}

int32 UCanvasPanel::GetChildrenCount() const
{
	return Slots.Num();
}

UWidget* UCanvasPanel::GetChildAt(int32 Index) const
{
	return Slots[Index]->Content;
}

TSharedPtr<SFixedSizeCanvas> UCanvasPanel::GetCanvasWidget() const
{
	return MyCanvas.Pin();
}

TSharedRef<SWidget> UCanvasPanel::RebuildWidget()
{
	TSharedRef<SFixedSizeCanvas> NewCanvas = SNew(SFixedSizeCanvas, DesiredCanvasSize);
	MyCanvas = NewCanvas;

	TSharedPtr<SFixedSizeCanvas> Canvas = MyCanvas.Pin();
	if ( Canvas.IsValid() )
	{
		Canvas->ClearChildren();

		for ( int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex )
		{
			UCanvasPanelSlot* Slot = Slots[SlotIndex];
			if ( Slot == NULL )
			{
				Slots[SlotIndex] = Slot = ConstructObject<UCanvasPanelSlot>(UCanvasPanelSlot::StaticClass(), this);
			}

			Slot->BuildSlot(Canvas.ToSharedRef());
		}
	}

	return NewCanvas;
}

UCanvasPanelSlot* UCanvasPanel::AddSlot(UWidget* Content)
{
	UCanvasPanelSlot* Slot = ConstructObject<UCanvasPanelSlot>(UCanvasPanelSlot::StaticClass(), this);
	Slot->Content = Content;
	Slot->Parent = this;

#if WITH_EDITOR
	Content->Slot = Slot;
#endif
	
	Slots.Add(Slot);

	return Slot;
}

bool UCanvasPanel::AddChild(UWidget* Child, FVector2D Position)
{
	UCanvasPanelSlot* Slot = AddSlot(Child);
	Slot->LayoutData.Offsets = FMargin(Position.X, Position.Y, 100, 25);

	return true;
}

bool UCanvasPanel::GetGeometryForSlot(UCanvasPanelSlot* Slot, FGeometry& ArrangedGeometry) const
{
	if ( Slot->Content == NULL )
	{
		return false;
	}

	TSharedPtr<SFixedSizeCanvas> Canvas = GetCanvasWidget();
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

void UCanvasPanel::ConnectEditorData()
{
	for ( UCanvasPanelSlot* Slot : Slots )
	{
		Slot->Parent = this;
		Slot->Content->Slot = Slot;
	}
}

void UCanvasPanel::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	// Ensure the slots have unique names
	int32 SlotNumbering = 1;

	TSet<FName> UniqueSlotNames;
	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		if ( Slots[SlotIndex] == NULL )
		{
			Slots[SlotIndex] = ConstructObject<UCanvasPanelSlot>(UCanvasPanelSlot::StaticClass(), this);
		}
	}
}
#endif