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
	UFUNCTION(BlueprintCallable, Category="Appearance")
	int32 GetChildrenCount() const;

	/**  */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	UWidget* GetChildAt(int32 Index) const;

	/**  */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	int32 GetChildIndex(UWidget* Content) const;

	/**  */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	bool RemoveChildAt(int32 Index);

	/**  */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	UPanelSlot* AddChild(UWidget* Content);

	/**  */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	void ReplaceChildAt(int32 Index, UWidget* Content);

	/**  */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	void InsertChildAt(int32 Index, UWidget* Content);

	/**  */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	bool RemoveChild(UWidget* Content);

	/**  */
	bool CanHaveMultipleChildren() const
	{
		return bCanHaveMultipleChildren;
	}

	virtual void ReleaseNativeWidget() override;

#if WITH_EDITOR
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
