// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PanelWidget.generated.h"

/** The base class for all UMG panel widgets.  Panel widgets layout a collection of child widgets. */
UCLASS(Abstract)
class UMG_API UPanelWidget : public UWidget
{
	GENERATED_UCLASS_BODY()

public:

	/** The items placed in the panel */
	UPROPERTY()
	TArray<UPanelSlot*> Slots;

public:

	//TODO UMG Add ways to make adding slots callable by blueprints.

	int32 GetChildrenCount() const
	{
		return Slots.Num();
	}

	UWidget* GetChildAt(int32 Index) const
	{
		return Slots[Index]->Content;
	}

	int32 GetChildIndex(UWidget* Content) const
	{
		const int32 ChildCount = GetChildrenCount();
		for ( int32 ChildIndex = 0; ChildIndex < ChildCount; ChildIndex++ )
		{
			if ( GetChildAt(ChildIndex) == Content )
			{
				return ChildIndex;
			}
		}

		return -1;
	}

	bool RemoveChildAt(int32 Index)
	{
		UPanelSlot* Slot = Slots[Index];
		Slots.RemoveAt(Index);
		
		OnSlotRemoved(Slot);

		return true;
	}

	UPanelSlot* AddChild(UWidget* Content, FVector2D Position)
	{
		if ( !bCanHaveMultipleChildren && GetChildrenCount() > 0 )
		{
			return false;
		}

		UPanelSlot* Slot = ConstructObject<UPanelSlot>(GetSlotClass(), this);
		Slot->SetFlags(RF_Transactional);
		Slot->Content = Content;
		Slot->Parent = this;

		Content->Slot = Slot;

		Slots.Add(Slot);

		OnSlotAdded(Slot);

		return Slot;
	}

	void ReplaceChildAt(int32 Index, UWidget* Content)
	{
		UPanelSlot* Slot = Slots[Index];
		Slot->Content = Content;

		Content->Slot = Slot;

		Slot->Refresh();
	}

	void InsertChildAt(int32 Index, UWidget* Content)
	{
		UPanelSlot* Slot = ConstructObject<UPanelSlot>(GetSlotClass(), this);
		Slot->SetFlags(RF_Transactional);
		Slot->Content = Content;
		Slot->Parent = this;

		Content->Slot = Slot;

		Slots.Insert(Slot, Index);

		OnSlotAdded(Slot);
	}

	bool RemoveChild(UWidget* Content)
	{
		int32 ChildIndex = GetChildIndex(Content);
		if ( ChildIndex != -1 )
		{
			return RemoveChildAt(ChildIndex);
		}

		return false;
	}

#if WITH_EDITOR
	virtual void ConnectEditorData() override
	{
		for ( UPanelSlot* Slot : Slots )
		{
			Slot->Parent = this;
			Slot->Content->Slot = Slot;
		}
	}
#endif

protected:

	virtual UClass* GetSlotClass() const
	{
		return UPanelSlot::StaticClass();
	}

	virtual void OnSlotAdded(UPanelSlot* Slot)
	{

	}

	virtual void OnSlotRemoved(UPanelSlot* Slot)
	{

	}

protected:
	/** Can this panel allow for multiple children? */
	bool bCanHaveMultipleChildren;
};
