// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// SFixedSizeCanvas

class SFixedSizeCanvas : public SCanvas
{
	SLATE_BEGIN_ARGS( SFixedSizeCanvas )
		{
			_Visibility = EVisibility::SelfHitTestInvisible;
		}
	SLATE_END_ARGS()

	FVector2D CanvasSize;

	void Construct(const FArguments& InArgs, FVector2D InSize)
	{
		SCanvas::FArguments ParentArgs;
		SCanvas::Construct(ParentArgs);

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
// UCanvasPanelComponent

UCanvasPanelComponent::UCanvasPanelComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsVariable = false;
	DesiredCanvasSize = FVector2D(128.0f, 128.0f);
}

int32 UCanvasPanelComponent::GetChildrenCount() const
{
	return Slots.Num();
}

UWidget* UCanvasPanelComponent::GetChildAt(int32 Index) const
{
	return Slots[Index]->Content;
}

TSharedRef<SWidget> UCanvasPanelComponent::RebuildWidget()
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

UCanvasPanelSlot* UCanvasPanelComponent::AddSlot(UWidget* Content)
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

bool UCanvasPanelComponent::AddChild(UWidget* Child, FVector2D Position)
{
	UCanvasPanelSlot* Slot = AddSlot(Child);
	Slot->Position = Position;
	Slot->Size = FVector2D(100, 25);

	return true;
}

#if WITH_EDITOR

void UCanvasPanelComponent::ConnectEditorData()
{
	for ( UCanvasPanelSlot* Slot : Slots )
	{
		Slot->Parent = this;
		Slot->Content->Slot = Slot;
	}
}

void UCanvasPanelComponent::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
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