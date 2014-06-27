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

	/**  */
	int32 GetChildrenCount() const;

	/**  */
	UWidget* GetChildAt(int32 Index) const;

	/**  */
	int32 GetChildIndex(UWidget* Content) const;

	/**  */
	bool RemoveChildAt(int32 Index);

	/**  */
	UPanelSlot* AddChild(UWidget* Content);

	/**  */
	void ReplaceChildAt(int32 Index, UWidget* Content);

	/**  */
	void InsertChildAt(int32 Index, UWidget* Content);

	/**  */
	bool RemoveChild(UWidget* Content);

	/**  */
	bool CanHaveMultipleChildren() const { return bCanHaveMultipleChildren; }

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
