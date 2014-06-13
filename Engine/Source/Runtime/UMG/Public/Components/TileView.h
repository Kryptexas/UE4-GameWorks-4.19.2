// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TileView.generated.h"

/** A flow panel that presents the contents as a set of tiles all uniformly sized. */
UCLASS(meta=( Category="Misc" ), ClassGroup=UserInterface)
class UMG_API UTileView : public UTableViewBase
{
	GENERATED_UCLASS_BODY()

public:

	UPROPERTY(EditDefaultsOnly, Category=Content)
	float ItemWidth;

	UPROPERTY(EditDefaultsOnly, Category=Content)
	float ItemHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Content)
	TArray<UObject*> Items;

	UPROPERTY(EditDefaultsOnly, Category=Content)
	TEnumAsByte<ESelectionMode::Type> SelectionMode;

	UPROPERTY(EditDefaultsOnly, Category=Events)
	FOnGenerateRowUObject OnGenerateTileEvent;

protected:
	TSharedPtr< STileView<UObject*> > ListWidget() const
	{
		if ( MyWidget.IsValid() )
		{
			return StaticCastSharedRef< STileView<UObject*> >(MyWidget.ToSharedRef());
		}

		return TSharedPtr< STileView<UObject*> >();
	}

	TSharedRef<ITableRow> HandleOnGenerateTile(UObject* Item, const TSharedRef< STableViewBase >& OwnerTable) const;

	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface
};
