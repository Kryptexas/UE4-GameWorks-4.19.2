// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TextBlock.generated.h"

/** A simple static text widget */
UCLASS(meta=( Category="Common" ), ClassGroup=UserInterface )
class UMG_API UTextBlock : public UWidget
{
	GENERATED_UCLASS_BODY()

public:
	/**  
	 * Sets the color and opacity of the text in this text block
	 *
	 * @param InColorAndOpacity		The new text color and opacity
	 */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	void SetColorAndOpacity(FLinearColor InColorAndOpacity);

	/**  
	 * Sets the color and opacity of the text drop shadow
	 * Note: if opacity is zero no shadow will be drawn
	 *
	 * @param InShadowColorAndOpacity		The new drop shadow color and opacity
	 */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	void SetShadowColorAndOpacity(FLinearColor InShadowColorAndOpacity);

	/**  
	 * Sets the offset that the text drop shadow should be drawn at
	 *
	 * @param InShadowOffset		The new offset
	 */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	void SetShadowOffset(FVector2D InShadowOffset);


public:
	/** The style to use to render the text */
	UPROPERTY(EditDefaultsOnly, Category=Style, meta=( DisplayThumbnail = "true" ))
	USlateWidgetStyleAsset* Style;

	/** The text to display */
	UPROPERTY(EditDefaultsOnly, Category=Content)
	FText Text;
	
	/** A bindable delegate to allow logic to drive the text of the widget */
	UPROPERTY()
	FGetText TextDelegate;

	/** The color of the text */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FLinearColor ColorAndOpacity;

	/** A bindable delegate for the ColorAndOpacity. */
	UPROPERTY()
	FGetSlateColor ColorAndOpacityDelegate;
	
	/** The font to render the text with */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FSlateFontInfo Font;

	/** The direction the shadow is cast */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FVector2D ShadowOffset;

	/** The color of the shadow */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FLinearColor ShadowColorAndOpacity;

	/** A bindable delegate for the ShadowColorAndOpacity. */
	UPROPERTY()
	FGetLinearColor ShadowColorAndOpacityDelegate;

	/** Whether text wraps onto a new line when it's length exceeds this width; if this value is zero or negative, no wrapping occurs. */
	UPROPERTY(EditDefaultsOnly, Category=Appearance, AdvancedDisplay)
	float WrapTextAt;

	/** True if we're wrapping text automatically based on the computed horizontal space for this widget */
	UPROPERTY(EditDefaultsOnly, Category=Appearance, AdvancedDisplay)
	bool AutoWrapText;

	///** Called when this text is double clicked */
	//SLATE_EVENT(FOnClicked, OnDoubleClicked)

	// UWidget interface
	void SyncronizeProperties() override;
	// End of UWidget interface

#if WITH_EDITOR
	// UWidget interface
	virtual const FSlateBrush* GetEditorIcon() override;
	// End UWidget interface

	virtual FString GetLabelMetadata() const override;

	void HandleTextCommitted(const FText& InText, ETextCommit::Type CommitteType);

	virtual void OnDesignerDoubleClicked() override;
#endif

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface

protected:

	TSharedPtr<STextBlock> MyTextBlock;

#if WITH_EDITOR
	TSharedPtr<SInlineEditableTextBlock> MyEditorTextBlock;
#endif
};
