// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Button.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnButtonClickedEvent);

/** Buttons are clickable widgets */
UCLASS(meta=( Category="Common" ), ClassGroup=UserInterface)
class UMG_API UButton : public UContentWidget
{
	GENERATED_UCLASS_BODY()

public:
	/** Style of the button */
	UPROPERTY(EditDefaultsOnly, Category=Style, meta=( DisplayThumbnail = "true" ))
	USlateWidgetStyleAsset* Style;

	/** Horizontal positioning of the content within the button */
	UPROPERTY(EditDefaultsOnly, Category="Content Layout")
	TEnumAsByte<EHorizontalAlignment> HorizontalAlignment;

	/** Vertical positioning of the content within the button */
	UPROPERTY(EditDefaultsOnly, Category="Content Layout")
	TEnumAsByte<EVerticalAlignment> VerticalAlignment;

	/** The padding to add around the button content. */
	UPROPERTY(EditDefaultsOnly, Category="Content Layout")
	FMargin ContentPadding;

	/** The scaling factor for the button border */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FVector2D DesiredSizeScale;

	/** The scaling factor for the button content */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FVector2D ContentScale;
	
	/** The color multiplier for the button images */
	UPROPERTY(EditDefaultsOnly, Category=Appearance )
	FLinearColor ColorAndOpacity;
	
	/** The color multiplier for the button background */
	UPROPERTY(EditDefaultsOnly, Category=Appearance )
	FLinearColor BackgroundColor;

	/** The foreground color of the button */
	UPROPERTY(EditDefaultsOnly, Category=Appearance )
	FLinearColor ForegroundColor;

	UPROPERTY(EditDefaultsOnly, Category=Sound)
	FSlateSound PressedSound;

	UPROPERTY(EditDefaultsOnly, Category=Sound)
	FSlateSound HoveredSound;

	/** Called when the button is clicked */
	UPROPERTY(EditDefaultsOnly, Category=Events)
	FOnReply OnClickedEvent;
	
	/**  */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	void SetColorAndOpacity(FLinearColor InColorAndOpacity);

	/**  */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	void SetBackgroundColor(FLinearColor InBackgroundColor);

	/**  */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	void SetForegroundColor(FLinearColor InForegroundColor);

	/**  */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	void SetContentPadding(FMargin InContentPadding);
	
	/**  */
	UFUNCTION(BlueprintCallable, Category="Widget")
	bool IsPressed() const;

	// UContentWidget interface
	virtual void SetContent(UWidget* Content) override;
	// End UContentWidget interface

	// UWidget interface
	virtual void SyncronizeProperties() override;
	// End of UWidget interface

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface

	FReply HandleOnClicked();

protected:
	TSharedPtr<SButton> MyButton;
};
