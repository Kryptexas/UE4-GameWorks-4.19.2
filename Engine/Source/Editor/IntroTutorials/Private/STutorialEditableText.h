// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

namespace TutorialTextHelpers
{
	extern void OnBrowserLinkClicked(const FSlateHyperlinkRun::FMetadata& Metadata);

	extern void OnDocLinkClicked(const FSlateHyperlinkRun::FMetadata& Metadata);

	extern void OnTutorialLinkClicked(const FSlateHyperlinkRun::FMetadata& Metadata);

	extern void OnCodeLinkClicked(const FSlateHyperlinkRun::FMetadata& Metadata);

	extern void OnAssetLinkClicked(const FSlateHyperlinkRun::FMetadata& Metadata);
}

namespace EHyperlinkType
{
	enum Type
	{
		Browser,
		UDN,
		Tutorial,
		Code,
		Asset
	};
}

/** Text style and name to display in the UI */
struct FTextStyleAndName
{
	/** Legacy: Flags controlling which TTF or OTF font should be picked from the given font family */
	struct EFontStyle
	{
		typedef uint8 Flags;
		enum Flag
		{
			Regular = 0,
			Bold = 1<<0,
			Italic = 1<<1,
		};
	};

	FTextStyleAndName(FName InStyle, FText InDisplayName)
		: Style(InStyle)
		, DisplayName(InDisplayName)
	{}

	FRunInfo CreateRunInfo() const
	{
		FRunInfo RunInfo(TEXT("TextStyle"));
		RunInfo.MetaData.Add(TEXT("Style"), Style.ToString());
		return RunInfo;
	}

	static FName GetStyleFromRunInfo(const FRunInfo& InRunInfo)
	{
		const FString* const StyleString = InRunInfo.MetaData.Find(TEXT("Style"));
		if(StyleString)
		{
			return **StyleString;
		}
		else
		{
			// legacy styling support - try to map older flexible styles to new fixed styles

			int32 FontSize = 11;
			const FString* const FontSizeString = InRunInfo.MetaData.Find(TEXT("FontSize"));
			if(FontSizeString)
			{
				FontSize = static_cast<uint8>(FPlatformString::Atoi(**FontSizeString));
			}

			if(FontSize > 24)
			{
				return TEXT("Tutorials.Content.HeaderText2");
			}
			else if(FontSize > 11 && FontSize <= 24)
			{
				return TEXT("Tutorials.Content.HeaderText1");
			}
			else
			{
				EFontStyle::Flags FontStyle = EFontStyle::Regular;
				const FString* const FontStyleString = InRunInfo.MetaData.Find(TEXT("FontStyle"));
				if(FontStyleString)
				{
					if(*FontStyleString == TEXT("Bold"))
					{
						FontStyle = EFontStyle::Bold;
					}
					else if(*FontStyleString == TEXT("Italic"))
					{
						FontStyle = EFontStyle::Italic;
					}
					else if(*FontStyleString == TEXT("BoldItalic"))
					{
						FontStyle = EFontStyle::Bold | EFontStyle::Italic;
					}
				}

				FLinearColor FontColor = FLinearColor::Black;
				const FString* const FontColorString = InRunInfo.MetaData.Find(TEXT("FontColor"));
				if(FontColorString && !FontColor.InitFromString(*FontColorString))
				{
					FontColor = FLinearColor::Black;
				}

				if(FontStyle != EFontStyle::Regular || FontColor != FLinearColor::Black)
				{
					return TEXT("Tutorials.Content.TextBold");
				}
				else
				{
					return TEXT("Tutorials.Content.Text");
				}
			}
		}

		return NAME_None;
	}

	FTextBlockStyle CreateTextBlockStyle() const
	{
		return FEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>(Style);
	}

	static FTextBlockStyle CreateTextBlockStyle(const FRunInfo& InRunInfo)
	{
		return FEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>(GetStyleFromRunInfo(InRunInfo));
	}

	/** The style identifier */
	FName Style;

	/** The text to display for this style */
	FText DisplayName;
};

/**
 * This is a custom decorator used to allow arbitrary styling of text within a rich-text editor
 * This is required since normal text styling can only work with known styles from a given Slate style-set
 */
class FTextStyleDecorator : public ITextDecorator
{
public:

	static TSharedRef<FTextStyleDecorator> Create()
	{
		return MakeShareable(new FTextStyleDecorator());
	}

	virtual ~FTextStyleDecorator()
	{
	}

	virtual bool Supports(const FTextRunParseResults& RunParseResult, const FString& Text) const override
	{
		return (RunParseResult.Name == TEXT("TextStyle"));
	}

	virtual TSharedRef<ISlateRun> Create(const TSharedRef<class FTextLayout>& TextLayout, const FTextRunParseResults& RunParseResult, const FString& OriginalText, const TSharedRef< FString >& InOutModelText, const ISlateStyle* Style) override
	{
		FRunInfo RunInfo(RunParseResult.Name);
		for(const TPair<FString, FTextRange>& Pair : RunParseResult.MetaData)
		{
			RunInfo.MetaData.Add(Pair.Key, OriginalText.Mid(Pair.Value.BeginIndex, Pair.Value.EndIndex - Pair.Value.BeginIndex));
		}

		FTextRange ModelRange;
		ModelRange.BeginIndex = InOutModelText->Len();
		*InOutModelText += OriginalText.Mid(RunParseResult.ContentRange.BeginIndex, RunParseResult.ContentRange.EndIndex - RunParseResult.ContentRange.BeginIndex);
		ModelRange.EndIndex = InOutModelText->Len();

		return FSlateTextRun::Create(RunInfo, InOutModelText, FTextStyleAndName::CreateTextBlockStyle(RunInfo), ModelRange);
	}
};

/** Helper struct to hold info about hyperlink types */
struct FHyperlinkTypeDesc
{
	FHyperlinkTypeDesc(EHyperlinkType::Type InType, const FText& InText, const FText& InTooltipText, const FString& InId, FSlateHyperlinkRun::FOnClick InOnClickedDelegate)
		: Type(InType)
		, Text(InText)
		, TooltipText(InTooltipText)
		, Id(InId)
		, OnClickedDelegate(InOnClickedDelegate)
	{
	}

	/** The type of the link */
	EHyperlinkType::Type Type;

	/** Tag used by this hyperlink's run */
	FString Id;

	/** Text to display in the UI */
	FText Text;

	/** Tooltip text to display in the UI */
	FText TooltipText;

	/** Delegate to execute for this hyperlink's run */
	FSlateHyperlinkRun::FOnClick OnClickedDelegate;
};


class STutorialEditableText : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( STutorialEditableText ){}

	SLATE_ATTRIBUTE(FText, Text)

	SLATE_ARGUMENT(FOnTextCommitted, OnTextCommitted)

	SLATE_ARGUMENT(FOnTextChanged, OnTextChanged)

	SLATE_END_ARGS()

	/**
	 * Construct the widget
	 *
	 * @param InArgs   Declaration from which to construct the widget
	 */
	void Construct(const FArguments& InArgs);

	virtual bool SupportsKeyboardFocus() const override { return true; }

protected:
	
	TSharedPtr<const IRun> GetCurrentRun() const;

	void HandleRichEditableTextChanged(const FText& Text);

	void HandleRichEditableTextCommitted(const FText& Text, ETextCommit::Type Type);

	void HandleRichEditableTextCursorMoved(const FTextLocation& NewCursorPosition );

	FText GetActiveStyleName() const;

	void OnActiveStyleChanged(TSharedPtr<FTextStyleAndName> NewValue, ESelectInfo::Type);

	TSharedRef<SWidget> GenerateStyleComboEntry(TSharedPtr<FTextStyleAndName> SourceEntry);

	void StyleSelectedText();

	void HandleHyperlinkComboOpened();

	bool IsHyperlinkComboEnabled() const;

	FReply HandleInsertHyperLinkClicked();

	EVisibility GetToolbarVisibility() const;

	FText GetHyperlinkButtonText() const;

	void OnActiveHyperlinkChanged(TSharedPtr<FHyperlinkTypeDesc> NewValue, ESelectInfo::Type SelectionType);

	TSharedRef<SWidget> GenerateHyperlinkComboEntry(TSharedPtr<FHyperlinkTypeDesc> SourceEntry);

	FText GetActiveHyperlinkName() const;

	FText GetActiveHyperlinkTooltip() const;

	TSharedPtr<FHyperlinkTypeDesc> GetHyperlinkTypeFromId(const FString& InId) const;

	EVisibility GetOpenAssetVisibility() const;

	void HandleOpenAssetCheckStateChanged(ESlateCheckBoxState::Type InCheckState);

	ESlateCheckBoxState::Type IsOpenAssetChecked() const;

protected:
	TSharedPtr<SMultiLineEditableTextBox> RichEditableTextBox;

	FSlateHyperlinkRun::FOnClick OnBrowserLinkClicked;
	FSlateHyperlinkRun::FOnClick OnDocLinkClicked;
	FSlateHyperlinkRun::FOnClick OnTutorialLinkClicked;
	FSlateHyperlinkRun::FOnClick OnCodeLinkClicked;
	FSlateHyperlinkRun::FOnClick OnAssetLinkClicked;

	TSharedPtr<SComboButton> HyperlinkComboButton;
	TSharedPtr<SComboBox<TSharedPtr<FTextStyleAndName>>> FontComboBox;
	TSharedPtr<STextBlock> HyperlinkNameTextBlock;
	TSharedPtr<SEditableTextBox> HyperlinkURLTextBox;

	TSharedPtr<FTextStyleAndName> ActiveStyle;
	TSharedPtr<FTextStyleAndName> HyperlinkStyle;

	TArray<TSharedPtr<FTextStyleAndName>> StylesAndNames;

	FOnTextCommitted OnTextCommitted;
	FOnTextChanged OnTextChanged;

	TArray<TSharedPtr<FHyperlinkTypeDesc>> HyperlinkDescs;
	TSharedPtr<FHyperlinkTypeDesc> CurrentHyperlinkType;

	bool bOpenAsset;

	bool bNewHyperlink;
};
