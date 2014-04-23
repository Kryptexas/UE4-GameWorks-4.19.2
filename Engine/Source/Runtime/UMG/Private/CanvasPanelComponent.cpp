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
	DesiredCanvasSize = FVector2D(128.0f, 128.0f);
}

int32 UCanvasPanelComponent::GetChildrenCount() const
{
	return Slots.Num();
}

USlateWrapperComponent* UCanvasPanelComponent::GetChildAt(int32 Index) const
{
	return Slots[Index]->Content;
}

TSharedRef<SWidget> UCanvasPanelComponent::RebuildWidget()
{
	TSharedRef<SFixedSizeCanvas> NewCanvas = SNew(SFixedSizeCanvas, DesiredCanvasSize);
	MyCanvas = NewCanvas;

	OnKnownChildrenChanged();

	return NewCanvas;
}

void UCanvasPanelComponent::OnKnownChildrenChanged()
{
	TSharedPtr<SFixedSizeCanvas> Canvas = MyCanvas.Pin();
	if (Canvas.IsValid())
	{
		Canvas->ClearChildren();

		for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
		{
			UCanvasPanelSlot* Slot = Slots[SlotIndex];
			if ( Slot == NULL )
			{
				Slots[SlotIndex] = Slot = ConstructObject<UCanvasPanelSlot>(UCanvasPanelSlot::StaticClass(), this);
			}

			auto NewSlot = Canvas->AddSlot()
				.Position(Slot->Position)
				.Size(Slot->Size)
				.HAlign(Slot->HorizontalAlignment)
				.VAlign(Slot->VerticalAlignment)
			[
				Slot->Content == NULL ? SNullWidget::NullWidget : Slot->Content->GetWidget()
			];
		}
	}
}

UCanvasPanelSlot* UCanvasPanelComponent::AddSlot(USlateWrapperComponent* Content)
{
	UCanvasPanelSlot* Slot = ConstructObject<UCanvasPanelSlot>(UCanvasPanelSlot::StaticClass(), this);
	Slot->Content = Content;
	
	Slots.Add(Slot);

	return Slot;
}

#if WITH_EDITOR
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

		//FName& SlotName = Slots[SlotIndex].SlotName;

		//if ((SlotName == NAME_None) || UniqueSlotNames.Contains(SlotName))
		//{
		//	do 
		//	{
		//		SlotName = FName(TEXT("Slot"), SlotNumbering++);
		//	} while (UniqueSlotNames.Contains(SlotName));
		//}

		//UniqueSlotNames.Add(SlotName);
	}
}
#endif