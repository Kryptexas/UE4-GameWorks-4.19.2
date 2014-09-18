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

	/**  */
	UFUNCTION(BlueprintCallable, Category="Widget|Panel")
	int32 GetChildrenCount() const;

	/**  */
	UFUNCTION(BlueprintCallable, Category="Widget|Panel")
	UWidget* GetChildAt(int32 Index) const;

	/**  */
	UFUNCTION(BlueprintCallable, Category="Widget|Panel")
	int32 GetChildIndex(UWidget* Content) const;

	/**  */
	UFUNCTION(BlueprintCallable, Category="Widget|Panel")
	bool RemoveChildAt(int32 Index);

	/**  */
	UFUNCTION(BlueprintCallable, Category="Widget|Panel")
	UPanelSlot* AddChild(UWidget* Content);

	/** Swaps the widget out of the slot at the given index, replacing it with a different widget. */
	UFUNCTION(BlueprintCallable, Category="Widget|Panel")
	void ReplaceChildAt(int32 Index, UWidget* Content);

	/** Inserts a widget at a specific index */
	UFUNCTION(BlueprintCallable, Category="Widget|Panel")
	void InsertChildAt(int32 Index, UWidget* Content);

	/**
	 * Removes a specific widget from the container.
	 * @return true if the widget was found and removed.
	 */
	UFUNCTION(BlueprintCallable, Category="Widget|Panel")
	bool RemoveChild(UWidget* Content);

	/** @return true if there are any child widgets in the panel */
	UFUNCTION(BlueprintCallable, Category="Widget|Panel")
	bool HasAnyChildren() const;

	/** Remove all child widgets from the panel widget. */
	UFUNCTION(BlueprintCallable, Category="Widget|Panel")
	void ClearChildren();

	/** @returns true if the panel supports more than one child. */
	bool CanHaveMultipleChildren() const
	{
		return bCanHaveMultipleChildren;
	}

	/** Sets that this widget is being designed sets it on all children as well. */
	virtual void SetIsDesignTime(bool bInDesignTime) override;

	virtual void ReleaseNativeWidget() override;

#if WITH_EDITOR
	virtual bool LockToPanelOnDrag() const
	{
		return false;
	}

	virtual void ConnectEditorData() override
	{
		for ( UPanelSlot* Slot : Slots )
		{
			Slot->Parent = this;
			if ( Slot->Content )
			{
				Slot->Content->Slot = Slot;
			}
		}
	}
#endif

	// Begin UObject
	virtual void PostLoad() override;
	// End UObject

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
