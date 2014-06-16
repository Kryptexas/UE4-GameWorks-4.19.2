// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TileView.generated.h"

/** A flow panel that presents the contents as a set of tiles all uniformly sized. */
UCLASS(meta=( Category="Misc" ), ClassGroup=UserInterface)
class UMG_API UTileView : public UTableViewBase
{
	GENERATED_UCLASS_BODY()

	/**  */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	float ItemWidth;

	/**  */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	float ItemHeight;

	/**  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Content)
	TArray<UObject*> Items;

	/**  */
	UPROPERTY(EditDefaultsOnly, Category=Content)
	TEnumAsByte<ESelectionMode::Type> SelectionMode;

	/**  */
	UPROPERTY(EditDefaultsOnly, Category=Events)
	FOnGenerateRowUObject OnGenerateTileEvent;

	/** Set item width */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	void SetItemWidth(float Width);

	/** Set item height */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	void SetItemHeight(float Height);

	/** Refreshes the list */
	UFUNCTION(BlueprintCallable, Category="Behavior")
	void RequestListRefresh();

protected:
	TSharedRef<ITableRow> HandleOnGenerateTile(UObject* Item, const TSharedRef< STableViewBase >& OwnerTable) const;

	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface

protected:
	TSharedPtr< STileView< UObject* > > MyTileView;
};
