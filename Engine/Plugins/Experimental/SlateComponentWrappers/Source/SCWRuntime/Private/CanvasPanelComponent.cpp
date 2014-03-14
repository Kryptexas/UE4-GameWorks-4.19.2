// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlateComponentWrappersPrivatePCH.h"

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

TSharedRef<SWidget> UCanvasPanelComponent::RebuildWidget()
{
	TSharedRef<SFixedSizeCanvas> NewCanvas = SNew(SFixedSizeCanvas, DesiredCanvasSize);
	MyCanvas = NewCanvas;

	return NewCanvas;
}

void UCanvasPanelComponent::OnKnownChildrenChanged()
{
	TSharedPtr<SFixedSizeCanvas> Canvas = MyCanvas.Pin();
	if (Canvas.IsValid())
	{
		Canvas->ClearChildren();

		// Add slots
		TMap<FName, int32> SlotNameToIndex;

		for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
		{
			const FCanvasPanelSlot& SlotConfig = Slots[SlotIndex];

			SlotNameToIndex.Add(SlotConfig.SlotName, SlotIndex);

			auto NewSlot = Canvas->AddSlot()
				.Position(SlotConfig.Position)
				.Size(SlotConfig.Size)
				.HAlign(SlotConfig.HorizontalAlignment)
				.VAlign(SlotConfig.VerticalAlignment)
			[
				SNullWidget::NullWidget
			];
		}

		// Place widgets in their slots, and add anything left over to the end
		for (int32 ComponentIndex = 0; ComponentIndex < AttachChildren.Num(); ++ComponentIndex)
		{
			if (USlateWrapperComponent* Wrapper = Cast<USlateWrapperComponent>(AttachChildren[ComponentIndex]))
			{
				if (Wrapper->IsRegistered())
				{
					if (int32* pSlotIndex = SlotNameToIndex.Find(Wrapper->AttachSocketName))
					{
						auto ExistingSlot = (TPanelChildren<SCanvas::FSlot>*)(Canvas->GetChildren());
						(*ExistingSlot)[*pSlotIndex].Widget = Wrapper->GetWidget();
					}
					else
					{
						Canvas->AddSlot()
						[
							Wrapper->GetWidget()
						];
					}
				}
			}
		}
	}
}

bool UCanvasPanelComponent::HasAnySockets() const
{
	return true;
}

FTransform UCanvasPanelComponent::GetSocketTransform(FName InSocketName, ERelativeTransformSpace TransformSpace) const
{
	return FTransform::Identity;
}

void UCanvasPanelComponent::QuerySupportedSockets(TArray<FComponentSocketDescription>& OutSockets) const
{
	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		FName SocketName = Slots[SlotIndex].SlotName;
		new (OutSockets) FComponentSocketDescription(SocketName, EComponentSocketType::Socket);
	}
}

#if WITH_EDITOR
void UCanvasPanelComponent::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	// Ensure the slots have unique names
	int32 SlotNumbering = 1;

	TSet<FName> UniqueSlotNames;
	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		FName& SlotName = Slots[SlotIndex].SlotName;

		if ((SlotName == NAME_None) || UniqueSlotNames.Contains(SlotName))
		{
			do 
			{
				SlotName = FName(TEXT("Slot"), SlotNumbering++);
			} while (UniqueSlotNames.Contains(SlotName));
		}

		UniqueSlotNames.Add(SlotName);
	}
}
#endif