
#include "TranslationEditorPrivatePCH.h"
#include "TranslationPickerFloatingWindow.h"
#include "Editor/Documentation/Public/SDocumentationToolTip.h"

#define LOCTEXT_NAMESPACE "TranslationPicker"

void STranslationPickerFloatingWindow::Construct(const FArguments& InArgs)
{
	ParentWindow = InArgs._ParentWindow;

	ChildSlot
		[
			SNew(SToolTip)
			.Text(this, &STranslationPickerFloatingWindow::GetPickerStatusText)
		];
}

void STranslationPickerFloatingWindow::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	TranslationInfoPreviewText = FText::GetEmpty();

	FWidgetPath Path = FSlateApplication::Get().LocateWindowUnderMouse(FSlateApplication::Get().GetCursorPos(), FSlateApplication::Get().GetInteractiveTopLevelWindows(), true);

	// Search everything under the cursor for any FText we know how to parse
	for (int32 PathIndex = Path.Widgets.Num() - 1; PathIndex >= 0; PathIndex--)
	{
		// General Widget case
		TSharedRef<SWidget> PathWidget = Path.Widgets[PathIndex].Widget;
		FText TextBlockDescription = GetTextDescription(PathWidget);

		if (!TextBlockDescription.IsEmpty())
		{
			TranslationInfoPreviewText = FText::Format(LOCTEXT("TextBlockLocalizationDescription", "{0}\n\n\n\n\nTextBlockInfo: {1}"),
				TranslationInfoPreviewText, TextBlockDescription);
		}

		// Tooltip case
		TSharedPtr<IToolTip> Tooltip = PathWidget->GetToolTip();
		FText TooltipDescription = FText::GetEmpty();

		if (Tooltip.IsValid() && !Tooltip->IsEmpty())
		{
			TooltipDescription = GetTextDescription(Tooltip->AsWidget());
			if (!TooltipDescription.IsEmpty())
			{
				TranslationInfoPreviewText = FText::Format(LOCTEXT("TooltipLocalizationDescription", "{0}\n\n\n\n\nTooltip Info:{1}"),
					TranslationInfoPreviewText, TooltipDescription);
			}
		}
		
	}

	// kind of a hack, but we need to maintain keyboard focus otherwise we wont get our keypress to 'pick'
	FSlateApplication::Get().SetKeyboardFocus(SharedThis(this), EKeyboardFocusCause::SetDirectly);
	if (ParentWindow.IsValid())
	{
		// also kind of a hack, but this is the only way at the moment to get a 'cursor decorator' without using the drag-drop code path
		ParentWindow.Pin()->MoveWindowTo(FSlateApplication::Get().GetCursorPos() + FSlateApplication::Get().GetCursorSize());
	}
}

FText STranslationPickerFloatingWindow::GetTextDescription(TSharedRef<SWidget> Widget)
{
	FText OutText = FText::GetEmpty();

	// Parse STextBlocks
	STextBlock* TextBlock = (STextBlock*)&Widget.Get();
	if ((Widget->GetTypeAsString() == "STextBlock" && TextBlock))
	{
		FText Text = TextBlock->GetText();
		OutText = FormatFTextInfo(Text);
	}

	// Parse SToolTips
	else if (Widget->GetTypeAsString() == "SToolTip")
	{
		SToolTip* ToolTip = ((SToolTip*)&Widget.Get());
		if (ToolTip != nullptr)
		{
			OutText = GetTextDescription(ToolTip->GetContentWidget());
			if (OutText.IsEmpty())
			{
				OutText = FormatFTextInfo(ToolTip->GetTextTooltip());
			}
		}
	}

	// Parse SDocumentationToolTips
	else if (Widget->GetTypeAsString() == "SDocumentationToolTip")
	{
		SDocumentationToolTip* DocumentationToolTip = (SDocumentationToolTip*)&Widget.Get();
		if (DocumentationToolTip != nullptr)
		{
			OutText = FormatFTextInfo(DocumentationToolTip->GetTextTooltip());
		}
	}
	return OutText;
}

FText STranslationPickerFloatingWindow::FormatFTextInfo(FText TextToFormat)
{
	FText OutText = FText::GetEmpty();

	if (FTextInspector::ShouldGatherForLocalization(TextToFormat))
	{
		const FString* TextNamespace = FTextInspector::GetNamespace(TextToFormat);
		const FString* TextKey = FTextInspector::GetKey(TextToFormat);
		const FString* TextSource = FTextInspector::GetSourceString(TextToFormat);
		const FString& TextTranslation = FTextInspector::GetDisplayString(TextToFormat);

		TSharedPtr<FString, ESPMode::ThreadSafe> TextTableName = TSharedPtr<FString, ESPMode::ThreadSafe>(nullptr);
		if (TextNamespace && TextKey)
		{
			TextTableName = FTextLocalizationManager::Get().GetTableName(*TextNamespace, *TextKey);
		}

		FText Namespace = TextNamespace != nullptr ? FText::FromString(*TextNamespace) : FText::GetEmpty();
		FText Key = TextKey != nullptr ? FText::FromString(*TextKey) : FText::GetEmpty();
		FText Source = TextSource != nullptr ? FText::FromString(*TextSource) : FText::GetEmpty();
		FText TableName = TextTableName.IsValid() ? FText::FromString(*(TextTableName.Get())) : FText::GetEmpty();
		FText Translation = FText::FromString(TextTranslation);

		OutText = FText::Format(LOCTEXT("LocalizationStringDescription", "\n\n\n\n\n\nText Namespace: {0}\nText Key: {1}\nText Source: {2}\nTable Name: {3}"), Namespace, Key, Source, TableName);
		OutText = FText::Format(LOCTEXT("LocalizationStringDescription2", "{0}\nTranslation: {1}"), OutText, Translation);
	}

	return OutText; 
}

FReply STranslationPickerFloatingWindow::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		//TODO: pop up window for in-place translation

		TranslationInfoPreviewText = FText::GetEmpty();

		if (ParentWindow.IsValid())
		{
			FSlateApplication::Get().RequestDestroyWindow(ParentWindow.Pin().ToSharedRef());
			ParentWindow.Reset();
		}

		return FReply::Handled();
	}

	return FReply::Unhandled();
}


#undef LOCTEXT_NAMESPACE