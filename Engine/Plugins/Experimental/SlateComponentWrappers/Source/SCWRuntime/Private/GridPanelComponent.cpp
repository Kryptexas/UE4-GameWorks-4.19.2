// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlateComponentWrappersPrivatePCH.h"

/////////////////////////////////////////////////////
// UGridPanelComponent

UGridPanelComponent::UGridPanelComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

TSharedRef<SWidget> UGridPanelComponent::RebuildWidget()
{
	RebuildUniqueSlotTable();

	TSharedRef<SGridPanel> NewGrid = SNew(SGridPanel);
	MyGrid = NewGrid;

	//@TODO: Expose FillColumn and FillRow

	return NewGrid;
}

void UGridPanelComponent::OnKnownChildrenChanged()
{
	TSharedPtr<SGridPanel> Grid = MyGrid.Pin();
	if (Grid.IsValid())
	{
		// Create a mapping of slot names to slot indices
		TMap<FName, int32> SlotNameToIndex;
		for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
		{
			const FGridPanelSlot& SlotConfig = Slots[SlotIndex];
			SlotNameToIndex.Add(SlotConfig.SlotName, SlotIndex);
		}
		
		// Make sure all children are assigned to a slot
		TArray<USlateWrapperComponent*> WrappedComponentsInSlotOrder;
		WrappedComponentsInSlotOrder.AddZeroed(Slots.Num());

		for (int32 ComponentIndex = 0; ComponentIndex < AttachChildren.Num(); ++ComponentIndex)
		{
			if (USlateWrapperComponent* Wrapper = Cast<USlateWrapperComponent>(AttachChildren[ComponentIndex]))
			{
				if (Wrapper->IsRegistered())
				{
					WrappedComponentsInSlotOrder.Add(Wrapper);

					if (int32* pSlotIndex = SlotNameToIndex.Find(Wrapper->AttachSocketName))
					{
						WrappedComponentsInSlotOrder[*pSlotIndex] = Wrapper;
					}
					else
					{
						FGridPanelSlot& NewSlot = *new (Slots) FGridPanelSlot();
						EnsureSlotIsUnique(NewSlot);
						WrappedComponentsInSlotOrder.Add(Wrapper);

						Wrapper->AttachSocketName = NewSlot.SlotName;
					}
				}						
			}
		}

		// Add slots to the grid panel
		Grid->ClearChildren();
		for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
		{
			const FGridPanelSlot& SlotConfig = Slots[SlotIndex];

			TSharedRef<SWidget> ContentWidget = SNullWidget::NullWidget;
			if (WrappedComponentsInSlotOrder[SlotIndex] != NULL)
			{
				ContentWidget = WrappedComponentsInSlotOrder[SlotIndex]->GetWidget();
			}

			Grid->AddSlot(SlotConfig.Column, SlotConfig.Row)
				.RowSpan(SlotConfig.RowSpan)
				.ColumnSpan(SlotConfig.ColumnSpan)
				.Padding(SlotConfig.Padding)
				.HAlign(SlotConfig.HorizontalAlignment)
				.VAlign(SlotConfig.VerticalAlignment)
			[
				ContentWidget
			];
		}
	}
}

bool UGridPanelComponent::HasAnySockets() const
{
	return true;
}

FTransform UGridPanelComponent::GetSocketTransform(FName InSocketName, ERelativeTransformSpace TransformSpace) const
{
	return FTransform::Identity;
}

void UGridPanelComponent::QuerySupportedSockets(TArray<FComponentSocketDescription>& OutSockets) const
{
	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		FName SocketName = Slots[SlotIndex].SlotName;
		new (OutSockets) FComponentSocketDescription(SocketName, EComponentSocketType::Socket);
	}
}

#if WITH_EDITOR
void UGridPanelComponent::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	RebuildUniqueSlotTable();
}
#endif

void UGridPanelComponent::RebuildUniqueSlotTable()
{
	// Ensure the slots have unique names and coordinates
	UniqueSlotCoordinates.Empty();

	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		EnsureSlotIsUnique(Slots[SlotIndex]);
	}
}

void UGridPanelComponent::EnsureSlotIsUnique(FGridPanelSlot& SlotConfig)
{
	FName& SlotName = SlotConfig.SlotName;

	while (UniqueSlotCoordinates.Contains(SlotConfig.AsPoint()))
	{
		SlotConfig.Row++;
	}

	UniqueSlotCoordinates.Add(SlotConfig.AsPoint());
	SlotName = FName(*FString::Printf(TEXT("X%d_Y%d"), SlotConfig.Column, SlotConfig.Row));
}
