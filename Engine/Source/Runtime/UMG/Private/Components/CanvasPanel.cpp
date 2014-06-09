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

TSharedPtr<SConstraintCanvas> UCanvasPanel::GetCanvasWidget() const
{
	return MyCanvas;
}

int32 UCanvasPanel::GetChildIndex(UWidget* Content) const
{
	for ( int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex )
	{
		UCanvasPanelSlot* Slot = Slots[SlotIndex];

		if ( Slot->Content == Content )
		{
			return SlotIndex;
		}
	}

	return -1;
}

bool UCanvasPanel::AddChild(UWidget* Child, FVector2D Position)
{
	UCanvasPanelSlot* Slot = AddSlot(Child);
	Slot->LayoutData.Offsets = FMargin(Position.X, Position.Y, 100, 25);

	return true;
}

bool UCanvasPanel::RemoveChild(UWidget* Child)
{
	int32 SlotIndex = GetChildIndex(Child);
	if ( SlotIndex != -1 )
	{
		Slots.RemoveAt(SlotIndex);
		return true;
	}

	return false;
}

void UCanvasPanel::ReplaceChildAt(int32 Index, UWidget* Content)
{
	UCanvasPanelSlot* Slot = Slots[Index];
	Slot->Content = Content;

#if WITH_EDITOR
	Content->Slot = Slot;
#endif
}

void UCanvasPanel::InsertChildAt(int32 Index, UWidget* Content)
{
	UCanvasPanelSlot* Slot = ConstructObject<UCanvasPanelSlot>(UCanvasPanelSlot::StaticClass(), this);
	Slot->SetFlags(RF_Transactional);
	Slot->Content = Content;
	Slot->Parent = this;

#if WITH_EDITOR
	Content->Slot = Slot;
#endif

	Slots.Insert(Slot, Index);
}

TSharedRef<SWidget> UCanvasPanel::RebuildWidget()
{
	MyCanvas = SNew(SFixedSizeCanvas, DesiredCanvasSize);

	for ( auto Slot : Slots )
	{
		Slot->Parent = this;
		Slot->BuildSlot(MyCanvas.ToSharedRef());
	}

	return MyCanvas.ToSharedRef();
}

UCanvasPanelSlot* UCanvasPanel::AddSlot(UWidget* Content)
{
	UCanvasPanelSlot* Slot = ConstructObject<UCanvasPanelSlot>(UCanvasPanelSlot::StaticClass(), this);
	Slot->SetFlags(RF_Transactional);
	Slot->Content = Content;
	Slot->Parent = this;

#if WITH_EDITOR
	Content->Slot = Slot;
#endif
	
	Slots.Add(Slot);

	return Slot;
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
}
#endif