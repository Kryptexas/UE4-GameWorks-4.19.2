// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MultiLineEditableText.generated.h"

/** Editable text box widget */
UCLASS(ClassGroup=UserInterface)
class UMG_API UMultiLineEditableText : public UWidget
{
	GENERATED_UCLASS_BODY()

public:

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMultiLineEditableTextChangedEvent, const FText&, Text);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMultiLineEditableTextCommittedEvent, const FText&, Text, ETextCommit::Type, CommitMethod);

public:

	/** The text content for this editable text box widget */
	UPROPERTY(EditDefaultsOnly, Category=Content)
	FText Text;

	/** The justification of the text in the multilinebox */
	UPROPERTY(EditDefaultsOnly, Category=Content)
	TEnumAsByte<ETextJustify::Type> Justification;

	/** Font color and opacity (overrides Style) */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FSlateFontInfo Font;

	/** Called whenever the text is changed interactively by the user */
	UPROPERTY(BlueprintAssignable, Category="Widget Event")
	FOnMultiLineEditableTextChangedEvent OnTextChanged;

	/** Called whenever the text is committed.  This happens when the user presses enter or the text box loses focus. */
	UPROPERTY(BlueprintAssignable, Category="Widget Event")
	FOnMultiLineEditableTextCommittedEvent OnTextCommitted;

	/**  */
	UFUNCTION(BlueprintCallable, Category="Widget")
	FText GetText() const;

	/**  */
	UFUNCTION(BlueprintCallable, Category="Widget")
	void SetText(FText InText);
	
	// UWidget interface
	virtual void SynchronizeProperties() override;
	// End of UWidget interface

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
	TSharedPtr<SMultiLineEditableText> MyMultiLineEditableText;
};
