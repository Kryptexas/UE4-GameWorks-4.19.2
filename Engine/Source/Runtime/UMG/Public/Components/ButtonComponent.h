// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ButtonComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnButtonClickedEvent);

/** Buttons are clickable widgets */
UCLASS(meta=(BlueprintSpawnableComponent), ClassGroup=UserInterface)
class UMG_API UButtonComponent : public UContentWidget
{
	GENERATED_UCLASS_BODY()

public:
	/** Style of the button */
	UPROPERTY(EditDefaultsOnly, Category=Style, meta=( DisplayThumbnail = "true" ))
	USlateWidgetStyleAsset* Style;

	/** Horizontal positioning of the content within the button */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	TEnumAsByte<EHorizontalAlignment> HorizontalAlignment;

	/** Vertical positioning of the content within the button */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	TEnumAsByte<EVerticalAlignment> VerticalAlignment;

	/** The padding to add around the button content. */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FMargin ContentPadding;

	/** The scaling factor for the button border */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FVector2D DesiredSizeScale;

	/** The scaling factor for the button content */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FVector2D ContentScale;

	/** The color multiplier for the button images */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FLinearColor ButtonColorAndOpacity;

	/** The foreground color of the button */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FLinearColor ForegroundColor;

	UPROPERTY(EditDefaultsOnly, Category=Sound)
	FSlateSound PressedSound;

	UPROPERTY(EditDefaultsOnly, Category=Sound)
	FSlateSound HoveredSound;
	
	/** Called when the button is clicked */
	UPROPERTY(BlueprintAssignable)
	FOnButtonClickedEvent OnClicked;

	/*  */
	UFUNCTION(BlueprintPure, Category="Appearance")
	FLinearColor GetButtonColorAndOpacity();
	
	/*  */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	void SetButtonColorAndOpacity(FLinearColor InButtonColorAndOpacity);

	/*  */
	UFUNCTION(BlueprintPure, Category="Appearance")
	FLinearColor GetForegroundColor();

	/*  */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	void SetForegroundColor(FLinearColor InForegroundColor);
	
	/*  */
	UFUNCTION(BlueprintPure, Category="Appearance")
	FMargin GetContentPadding();

	/*  */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	void SetContentPadding(FMargin InContentPadding);


	// UContentWidget interface
	virtual void SetContent(USlateWrapperComponent* Content) OVERRIDE;
	// End UContentWidget interface

protected:
	TSharedPtr<class SButton> ButtonWidget() const
	{
		if ( MyWidget.IsValid() )
		{
			return StaticCastSharedRef<SButton>(MyWidget.ToSharedRef());
		}

		return TSharedPtr<class SButton>();
	}

protected:
	// USlateWrapperComponent interface
	virtual TSharedRef<SWidget> RebuildWidget() OVERRIDE;
	// End of USlateWrapperComponent interface

	FMargin GetContentPadding() const;

	FReply HandleOnClicked();
};
