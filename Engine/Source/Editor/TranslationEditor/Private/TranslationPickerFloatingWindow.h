#pragma once
#include "SCompoundWidget.h"

#define LOCTEXT_NAMESPACE "TranslationPicker"

/** Translation picker window to show details of FText(s) under cursor, and allow in-place translation */
class STranslationPickerFloatingWindow : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(STranslationPickerFloatingWindow) {}

	SLATE_ARGUMENT(TWeakPtr<SWindow>, ParentWindow)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	FText GetTranslationInfoPreviewText()
	{
		return TranslationInfoPreviewText;
	}

private:
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	/** Pull the FText reference out of an SWidget */
	FText GetTextDescription(TSharedRef<SWidget> Widget);

	/** Format an FText for display */
	FText FormatFTextInfo(FText TextToFormat);

	FText GetPickerStatusText() const
	{
		return FText::Format(LOCTEXT("TootipHint", "{0}\n\n(Esc to pick)"), TranslationInfoPreviewText);
	}

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

	/** We need to support keyboard focus to process the 'Esc' key */
	virtual bool SupportsKeyboardFocus() const override
	{
		return true;
	}

	/** Handle to the window that contains this widget */
	TWeakPtr<SWindow> ParentWindow;

	/** The widget name we are picking */
	FText TranslationInfoPreviewText;
};

#undef LOCTEXT_NAMESPACE