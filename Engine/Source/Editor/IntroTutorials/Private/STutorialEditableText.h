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

 /**
  * This is used in conjunction with the TextStyle decorator to allow arbitrary styling of text within a rich-text editor
  * This struct defines a set of known font families, as well as providing some utility functions for converting the text style to and from a text layout run
  */
struct FTextStyles
{
	/** Flags controlling which TTF or OTF font should be picked from the given font family */
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

	/** This struct defines a font family, which combines multiple TTF or OTF fonts into a single group, allowing text to be styled as bold or italic */
	struct FFontFamily
	{
		FFontFamily(FText InDisplayName, FName InFamilyName, FName InRegularFont, FName InBoldFont, FName InItalicFont, FName InBoldItalicFont)
			: DisplayName(InDisplayName)
			, FamilyName(InFamilyName)
			, RegularFont(InRegularFont)
			, BoldFont(InBoldFont)
			, ItalicFont(InItalicFont)
			, BoldItalicFont(InBoldItalicFont)
		{
		}

		/** Name used for this font family in the UI */
		FText DisplayName;

		/** Named used to identify this family from the TextStyle decorator */
		FName FamilyName;

		/** File paths to the fonts on disk */
		FName RegularFont;
		FName BoldFont;
		FName ItalicFont;
		FName BoldItalicFont;
	};

	/** Convert the given text style into run meta-information, so that valid source rich-text formatting can be generated for it */
	static FRunInfo CreateRunInfo(const TSharedPtr<FFontFamily>& InFontFamily, const uint8 InFontSize, const EFontStyle::Flags InFontStyle, const FLinearColor& InFontColor)
	{
		FString FontStyleString;
		if(InFontStyle == EFontStyle::Regular)
		{
			FontStyleString = TEXT("Regular");
		}
		else
		{
			if(InFontStyle & EFontStyle::Bold)
			{
				FontStyleString += "Bold";
			}
			if(InFontStyle & EFontStyle::Italic)
			{
				FontStyleString += "Italic";
			}
		}

		FRunInfo RunInfo(TEXT("TextStyle"));
		RunInfo.MetaData.Add(TEXT("FontFamily"), InFontFamily->FamilyName.ToString());
		RunInfo.MetaData.Add(TEXT("FontSize"), FString::FromInt(InFontSize));
		RunInfo.MetaData.Add(TEXT("FontStyle"), FontStyleString);
		RunInfo.MetaData.Add(TEXT("FontColor"), InFontColor.ToString());
		return RunInfo;
	}

	/** Explode some run meta-information back out into its component text style parts */
	void ExplodeRunInfo(const FRunInfo& InRunInfo, TSharedPtr<FFontFamily>& OutFontFamily, uint8& OutFontSize, EFontStyle::Flags& OutFontStyle, FLinearColor& OutFontColor) const
	{
		check(AvailableFontFamilies.Num());

		const FString* const FontFamilyString = InRunInfo.MetaData.Find(TEXT("FontFamily"));
		if(FontFamilyString)
		{
			OutFontFamily = FindFontFamily(FName(**FontFamilyString));
		}
		if(!OutFontFamily.IsValid())
		{
			OutFontFamily = AvailableFontFamilies[0];
		}

		OutFontSize = 11;
		const FString* const FontSizeString = InRunInfo.MetaData.Find(TEXT("FontSize"));
		if(FontSizeString)
		{
			OutFontSize = static_cast<uint8>(FPlatformString::Atoi(**FontSizeString));
		}

		OutFontStyle = EFontStyle::Regular;
		const FString* const FontStyleString = InRunInfo.MetaData.Find(TEXT("FontStyle"));
		if(FontStyleString)
		{
			if(*FontStyleString == TEXT("Bold"))
			{
				 OutFontStyle = EFontStyle::Bold;
			}
			else if(*FontStyleString == TEXT("Italic"))
			{
				 OutFontStyle = EFontStyle::Italic;
			}
			else if(*FontStyleString == TEXT("BoldItalic"))
			{
				 OutFontStyle = EFontStyle::Bold | EFontStyle::Italic;
			}
		}

		OutFontColor = FLinearColor::Black;
		const FString* const FontColorString = InRunInfo.MetaData.Find(TEXT("FontColor"));
		if(FontColorString && !OutFontColor.InitFromString(*FontColorString))
		{
			OutFontColor = FLinearColor::Black;
		}
	}

	/** Convert the given text style into a text block style for use by Slate */
	static FTextBlockStyle CreateTextBlockStyle(const TSharedPtr<FFontFamily>& InFontFamily, const uint8 InFontSize, const EFontStyle::Flags InFontStyle, const FLinearColor& InFontColor)
	{
		FName FontName = InFontFamily->RegularFont;
		if((InFontStyle & EFontStyle::Bold) && (InFontStyle & EFontStyle::Italic))
		{
			FontName = InFontFamily->BoldItalicFont;
		}
		else if(InFontStyle & EFontStyle::Bold)
		{
			FontName = InFontFamily->BoldFont;
		}
		else if(InFontStyle & EFontStyle::Italic)
		{
			FontName = InFontFamily->ItalicFont;
		}

		FTextBlockStyle TextBlockStyle;
		TextBlockStyle.SetFontName(FontName);
		TextBlockStyle.SetFontSize(InFontSize);
		TextBlockStyle.SetColorAndOpacity(InFontColor);
		return TextBlockStyle;
	}

	/** Convert the given run meta-information into a text block style for use by Slate */
	FTextBlockStyle CreateTextBlockStyle(const FRunInfo& InRunInfo) const
	{
		TSharedPtr<FFontFamily> FontFamily;
		uint8 FontSize;
		EFontStyle::Flags FontStyle;
		FLinearColor FontColor;
		ExplodeRunInfo(InRunInfo, FontFamily, FontSize, FontStyle, FontColor);
		return CreateTextBlockStyle(FontFamily, FontSize, FontStyle, FontColor);
	}

	/** Try and find a font family with the given name */
	TSharedPtr<FFontFamily> FindFontFamily(const FName InFamilyName) const
	{
		const TSharedPtr<FFontFamily>* const FoundFontFamily = AvailableFontFamilies.FindByPredicate([InFamilyName](TSharedPtr<FFontFamily>& Entry) -> bool
		{
			return Entry->FamilyName == InFamilyName;
		});
		return (FoundFontFamily) ? *FoundFontFamily : nullptr;
	}

	TArray<TSharedPtr<FFontFamily>> AvailableFontFamilies;
};

/**
 * This is a custom decorator used to allow arbitrary styling of text within a rich-text editor
 * This is required since normal text styling can only work with known styles from a given Slate style-set
 */
class FTextStyleDecorator : public ITextDecorator
{
public:

	static TSharedRef<FTextStyleDecorator> Create(FTextStyles* const InTextStyles)
	{
		return MakeShareable(new FTextStyleDecorator(InTextStyles));
	}

	virtual ~FTextStyleDecorator()
	{
	}

	virtual bool Supports(const FTextRunParseResults& RunParseResult, const FString& Text) const override
	{
		return (RunParseResult.Name == TEXT("TextStyle"));
	}

	virtual TSharedRef<ISlateRun> Create(const FTextRunParseResults& RunParseResult, const FString& OriginalText, const TSharedRef<FString>& InOutModelText, const ISlateStyle* Style) override
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

		return FSlateTextRun::Create(RunInfo, InOutModelText, TextStyles->CreateTextBlockStyle(RunInfo), ModelRange);
	}

private:

	FTextStyleDecorator(FTextStyles* const InTextStyles)
		: TextStyles(InTextStyles)
	{
	}

	FTextStyles* TextStyles;
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
	
	void HandleRichEditableTextChanged(const FText& Text);

	void HandleRichEditableTextCommitted(const FText& Text, ETextCommit::Type Type);

	void HandleRichEditableTextCursorMoved(const FTextLocation& NewCursorPosition );

	FText GetActiveFontFamilyName() const;

	void OnActiveFontFamilyChanged(TSharedPtr<FTextStyles::FFontFamily> NewValue, ESelectInfo::Type);

	TSharedRef<SWidget> GenerateFontFamilyComboEntry(TSharedPtr<FTextStyles::FFontFamily> SourceEntry);

	TOptional<uint8> GetFontSize() const;

	void SetFontSize(uint8 NewValue, ETextCommit::Type);

	ESlateCheckBoxState::Type IsFontStyleBold() const;

	void OnFontStyleBoldChanged(ESlateCheckBoxState::Type InState);

	ESlateCheckBoxState::Type IsFontStyleItalic() const;

	void OnFontStyleItalicChanged(ESlateCheckBoxState::Type InState);

	FLinearColor GetFontColor() const;

	void SetFontColor(FLinearColor NewValue);

	FReply OpenFontColorPicker();

	void HandleColorPickerWindowClosed(const TSharedRef<SWindow>& InWindow);

	void StyleSelectedText();

	void HandleHyperlinkComboOpened();

	FReply HandleInsertBrowserLinkClicked();

	EVisibility GetToolbarVisibility() const;

	ESlateCheckBoxState::Type IsCreatingBrowserLink() const;

	void OnCheckBrowserLink(ESlateCheckBoxState::Type State);

	ESlateCheckBoxState::Type IsCreatingUDNLink() const;

	void OnCheckUDNLink(ESlateCheckBoxState::Type State);

	ESlateCheckBoxState::Type IsCreatingTutorialLink() const;

	void OnCheckTutorialLink(ESlateCheckBoxState::Type State);

	ESlateCheckBoxState::Type IsCreatingCodeLink() const;

	void OnCheckCodeLink(ESlateCheckBoxState::Type State);

	ESlateCheckBoxState::Type IsCreatingAssetLink() const;

	void OnCheckAssetLink(ESlateCheckBoxState::Type State);

protected:
	TSharedPtr<SMultiLineEditableTextBox> RichEditableTextBox;

	FSlateHyperlinkRun::FOnClick OnBrowserLinkClicked;
	FSlateHyperlinkRun::FOnClick OnDocLinkClicked;
	FSlateHyperlinkRun::FOnClick OnTutorialLinkClicked;
	FSlateHyperlinkRun::FOnClick OnCodeLinkClicked;
	FSlateHyperlinkRun::FOnClick OnAssetLinkClicked;

	TSharedPtr<SComboButton> HyperlinkComboButton;
	TSharedPtr<SComboBox<TSharedPtr<FTextStyles::FFontFamily>>> FontComboBox;
	TSharedPtr<SEditableTextBox> HyperlinkNameTextBox;
	TSharedPtr<SEditableTextBox> HyperlinkURLTextBox;

	FTextStyles TextStyles;

	TSharedPtr<FTextStyles::FFontFamily> ActiveFontFamily;
	uint8 FontSize;
	FTextStyles::EFontStyle::Flags FontStyle;
	FLinearColor FontColor;
	bool bPickingColor;

	FOnTextCommitted OnTextCommitted;
	FOnTextChanged OnTextChanged;

	EHyperlinkType::Type CurrentHyperlinkType;
};
