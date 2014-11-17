// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MenuAnchor.generated.h"

/**
 * The Menu Anchor allows you to specify an location that a popup menu should be anchored to, and should be
 * summoned from.
 */
UCLASS(meta=( Category="Popup" ), ClassGroup=UserInterface)
class UMG_API UMenuAnchor : public UContentWidget
{
	GENERATED_UCLASS_BODY()

public:

	/** The widget class to spawn when the menu is required.  Creates the object freshly each time. */
	UPROPERTY(EditDefaultsOnly, Category=Menu)
	TSubclassOf<class UUserWidget> MenuClass;

	/** Called when the menu content is requested to allow dynamic decision over what to display */
	UPROPERTY(EditDefaultsOnly, Category=Events)
	FGetContent OnGetMenuContentEvent;
	
	/** The placement location of the summoned widget. */
	UPROPERTY(EditDefaultsOnly, Category=Menu)
	TEnumAsByte<EMenuPlacement> Placement;

	/**
	 * Open or close the popup
	 *
	 * @param InIsOpen    If true, open the popup. Otherwise close it.
	 * @param bFocusMenu  Should we focus the popup as soon as it opens?
	 */
	UFUNCTION(BlueprintCallable, Category="Widget")
	void SetIsOpen(bool InIsOpen, bool bFocusMenu = true);

	/** @return true if the popup is open; false otherwise. */
	UFUNCTION(BlueprintCallable, Category="Widget")
	bool IsOpen() const;

	///** @return true if we should open the menu due to a click. Sometimes we should not, if
	//the same MouseDownEvent that just closed the menu is about to re-open it because it happens to land on the button.*/
	//bool ShouldOpenDueToClick() const;

	///** @return The current menu position */
	//FVector2D GetMenuPosition() const;

	///** @return Whether this menu has open submenus */
	//bool HasOpenSubMenus() const;

	// UWidget interface
	virtual void SyncronizeProperties() override;
	// End of UWidget interface

#if WITH_EDITOR
	virtual const FSlateBrush* GetEditorIcon() override;
#endif

protected:

	// UPanelWidget
	virtual void OnSlotAdded(UPanelSlot* Slot) override;
	virtual void OnSlotRemoved(UPanelSlot* Slot) override;
	// End UPanelWidget

	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface

protected:
	TSharedRef<SWidget> HandleGetMenuContent();

protected:
	TSharedPtr<SMenuAnchor> MyMenuAnchor;
};
