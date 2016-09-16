// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SafeZone.generated.h"

UCLASS()
class UMG_API USafeZone : public UContentWidget
{
	GENERATED_BODY()
public:
	USafeZone();

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

	virtual void OnSlotAdded( UPanelSlot* Slot ) override;
	virtual void OnSlotRemoved( UPanelSlot* Slot ) override;
	virtual UClass* GetSlotClass() const override;

	void UpdateWidgetProperties();

	/** If this safe zone should pad for the left side of the screen's safe zone */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SafeZone")
	bool PadLeft;

	/** If this safe zone should pad for the right side of the screen's safe zone */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SafeZone")
	bool PadRight;

	/** If this safe zone should pad for the top side of the screen's safe zone */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SafeZone")
	bool PadTop;

	/** If this safe zone should pad for the bottom side of the screen's safe zone */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SafeZone")
	bool PadBottom;

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface

	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

	TSharedPtr< class SSafeZone > MySafeZone;
};