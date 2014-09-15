// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MultiLineEditableTextBox.generated.h"

/** Editable text box widget */
UCLASS(ClassGroup=UserInterface)
class UMG_API UMultiLineEditableTextBox : public UWidget
{
	GENERATED_UCLASS_BODY()

public:

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMultiLineEditableTextBoxChangedEvent, const FText&, Text);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMultiLineEditableTextBoxCommittedEvent, const FText&, Text, ETextCommit::Type, CommitMethod);

public:

	/** The style */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style", meta=( DisplayName="Style" ))
	FEditableTextBoxStyle WidgetStyle;

	/** The text style */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style", meta=( DisplayName="Text Style" ))
	FTextBlockStyle TextStyle;

	UPROPERTY()
	USlateWidgetStyleAsset* Style_DEPRECATED;

	/** The text content for this editable text box widget */
	UPROPERTY(EditDefaultsOnly, Category=Content)
	FText Text;

	/** The justification of the text in the multilinebox */
	UPROPERTY(EditDefaultsOnly, Category=Content)
	TEnumAsByte<ETextJustify::Type> Justification;

	/** Font color and opacity (overrides Style) */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FSlateFontInfo Font;

	/** Text color and opacity (overrides Style) */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FLinearColor ForegroundColor;

	/** The color of the background/border around the editable text (overrides Style) */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FLinearColor BackgroundColor;

	/** Text color and opacity when read-only (overrides Style) */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FLinearColor ReadOnlyForegroundColor;

	/** Called whenever the text is changed interactively by the user */
	UPROPERTY(BlueprintAssignable, Category="Widget Event")
	FOnMultiLineEditableTextBoxChangedEvent OnTextChanged;

	/** Called whenever the text is committed.  This happens when the user presses enter or the text box loses focus. */
	UPROPERTY(BlueprintAssignable, Category="Widget Event")
	FOnMultiLineEditableTextBoxCommittedEvent OnTextCommitted;

	/** Provide a alternative mechanism for error reporting. */
	//SLATE_ARGUMENT(TSharedPtr<class IErrorReportingWidget>, ErrorReporting)

	/**  */
	UFUNCTION(BlueprintCallable, Category="Widget")
	FText GetText() const;

	/**  */
	UFUNCTION(BlueprintCallable, Category="Widget")
	void SetText(FText InText);

	UFUNCTION(BlueprintCallable, Category="Widget")
	void SetError(FText InError);

	// UWidget interface
	virtual void SynchronizeProperties() override;
	// End of UWidget interface

	// UVisual interface
	virtual void ReleaseNativeWidget() override;
	// End of UVisual interface

	// Begin UObject interface
	virtual void PostLoad() override;
	// End of UObject interface

#if WITH_EDITOR
	virtual const FSlateBrush* GetEditorIcon() override;
	virtual const FText GetPaletteCategory() override;
#endif

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget

	void HandleOnTextChanged(const FText& Text);
	void HandleOnTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

protected:
	TSharedPtr<SMultiLineEditableTextBox> MyEditableTextBlock;
};
