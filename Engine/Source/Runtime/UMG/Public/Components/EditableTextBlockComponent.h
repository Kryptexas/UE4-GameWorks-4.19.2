// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EditableTextBlockComponent.generated.h"

/** Editable text box widget */
UCLASS(meta=(BlueprintSpawnableComponent), ClassGroup=UserInterface)
class UMG_API UEditableTextBlockComponent : public USlateWrapperComponent
{
	GENERATED_UCLASS_BODY()

protected:

	/** The text content for this editable text box widget */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Content)
	FText Text;

	/** Hint text that appears when there is no text in the text box */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Content)
	FText HintText;

	/** Font color and opacity (overrides Style) */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateFontInfo Font;

	/** Text color and opacity (overrides Style) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Appearance)
	FSlateColor ForegroundColor;

	/** The color of the background/border around the editable text (overrides Style) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Appearance)
	FSlateColor BackgroundColor;

	/** Text color and opacity when read-only (overrides Style) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Appearance)
	FSlateColor ReadOnlyForegroundColor;

	/** Sets whether this text box can actually be modified interactively by the user */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Appearance)
	bool IsReadOnly;

	/** Sets whether this text box is for storing a password */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Appearance)
	bool IsPassword;

	/** Minimum width that a text block should be */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Appearance)
	float MinimumDesiredWidth;

	/** Padding between the box/border and the text widget inside (overrides Style) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Appearance)
	FMargin Padding;

	/** Workaround as we loose focus when the auto completion closes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Behavior)
	bool IsCaretMovedWhenGainFocus;

	/** Whether to select all text when the user clicks to give focus on the widget */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Behavior)
	bool SelectAllTextWhenFocused;

	/** Whether to allow the user to back out of changes when they press the escape key */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Behavior)
	bool RevertTextOnEscape;

	/** Whether to clear keyboard focus when pressing enter to commit changes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Behavior)
	bool ClearKeyboardFocusOnCommit;

	/** Whether to select all text when pressing enter to commit changes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Behavior)
	bool SelectAllTextOnCommit;

	/** Called whenever the text is changed interactively by the user */
	//SLATE_EVENT(FOnTextChanged, OnTextChanged)

	/** Called whenever the text is committed.  This happens when the user presses enter or the text box loses focus. */
	//SLATE_EVENT(FOnTextCommitted, OnTextCommitted)

	/** Provide a alternative mechanism for error reporting. */
	//SLATE_ARGUMENT(TSharedPtr<class IErrorReportingWidget>, ErrorReporting)

protected:
	// USlateWrapperComponent interface
	virtual TSharedRef<SWidget> RebuildWidget() OVERRIDE;
	// End of USlateWrapperComponent
};
