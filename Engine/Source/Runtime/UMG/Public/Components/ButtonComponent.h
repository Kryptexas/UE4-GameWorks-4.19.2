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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Style, meta=( DisplayThumbnail = "true" ))
	USlateWidgetStyleAsset* Style;

	/** Horizontal positioning of the content within the button */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	TEnumAsByte<EHorizontalAlignment> HorizontalAlignment;

	/** Vertical positioning of the content within the button */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	TEnumAsByte<EVerticalAlignment> VerticalAlignment;

	/** The padding to add around the button content. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Appearance)
	FMargin ContentPadding;

	/** The scaling factor for the button border */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	FVector2D DesiredSizeScale;

	/** The scaling factor for the button content */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	FVector2D ContentScale;

	/** The color multiplier for the button images */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Appearance)
	FSlateColor ButtonColorAndOpacity;

	/** The foreground color of the button */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Appearance)
	FSlateColor ForegroundColor;

	/** Called when the button is clicked */
	UPROPERTY(BlueprintAssignable)
	FOnButtonClickedEvent OnClicked;

	// UContentWidget interface
	virtual void SetContent(USlateWrapperComponent* Content) OVERRIDE;
	// End UContentWidget interface

protected:
	TWeakPtr<class SButton> MyButton;

protected:
	// USlateWrapperComponent interface
	virtual TSharedRef<SWidget> RebuildWidget() OVERRIDE;
	// End of USlateWrapperComponent interface

	FMargin GetContentPadding() const;
	FSlateColor GetButtonColor() const;
	FSlateColor GetForegroundColor() const;

	virtual FReply SlateOnClickedCallback();
};
