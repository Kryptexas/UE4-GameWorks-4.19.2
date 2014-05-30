// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ComboBox.generated.h"

UCLASS(meta=(BlueprintSpawnableComponent), ClassGroup=UserInterface)
class UMG_API UComboBox : public UWidget
{
	GENERATED_UCLASS_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Content)
	TArray<UObject*> Items;

	/** Called when the widget is needed for the item. */
	UPROPERTY(EditDefaultsOnly, Category=Events)
	FGenerateWidgetUObject OnGenerateWidget;
	
protected:
	TSharedPtr< SComboBox<UObject*> > ComboBoxWidget() const
	{
		if ( MyWidget.IsValid() )
		{
			return StaticCastSharedRef< SComboBox<UObject*> >(MyWidget.ToSharedRef());
		}

		return TSharedPtr< SComboBox<UObject*> >();
	}

protected:
	TSharedRef<SWidget> HandleGenerateWidget(UObject* Item) const;

	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() OVERRIDE;
	// End of UWidget interface
};
