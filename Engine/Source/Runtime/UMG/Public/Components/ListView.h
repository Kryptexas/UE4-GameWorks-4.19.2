// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ListView.generated.h"

UCLASS(meta=(BlueprintSpawnableComponent), ClassGroup=UserInterface)
class UMG_API UListView : public UWidget
{
	GENERATED_UCLASS_BODY()

	DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(UWidget*, FOnGenerateRowUObject, UObject*, Item);
		
public:

	UPROPERTY(EditDefaultsOnly, Category=Content)
	float ItemHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Content)
	TArray<UObject*> Items;

	UPROPERTY(EditDefaultsOnly, Category=Content)
	TEnumAsByte<ESelectionMode::Type> SelectionMode;

	/** Called when the button is clicked */
	UPROPERTY(EditDefaultsOnly, Category=Events)
	FOnGenerateRowUObject OnGenerateRowEvent;

protected:
	TSharedPtr< SListView<UObject*> > ListWidget() const
	{
		if ( MyWidget.IsValid() )
		{
			return StaticCastSharedRef< SListView<UObject*> >(MyWidget.ToSharedRef());
		}

		return TSharedPtr< SListView<UObject*> >();
	}

	TSharedRef<ITableRow> HandleOnGenerateRow(UObject* Item, const TSharedRef< STableViewBase >& OwnerTable) const;

	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() OVERRIDE;
	// End of UWidget interface
};
