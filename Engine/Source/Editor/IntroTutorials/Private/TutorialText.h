// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

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

/** Helper struct to hold info about hyperlink types */
struct FHyperlinkTypeDesc
{
	FHyperlinkTypeDesc(EHyperlinkType::Type InType, const FText& InText, const FText& InTooltipText, const FString& InId, FSlateHyperlinkRun::FOnClick InOnClickedDelegate, FSlateHyperlinkRun::FOnGetTooltipText InTooltipTextDelegate = FSlateHyperlinkRun::FOnGetTooltipText(), FSlateHyperlinkRun::FOnGenerateTooltip InTooltipDelegate = FSlateHyperlinkRun::FOnGenerateTooltip())
		: Type(InType)
		, Id(InId)
		, Text(InText)
		, TooltipText(InTooltipText)
		, OnClickedDelegate(InOnClickedDelegate)
		, TooltipTextDelegate(InTooltipTextDelegate)
		, TooltipDelegate(InTooltipDelegate)
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

	/** Delegate used to retrieve the text to display in the hyperlink's tooltip */
	FSlateHyperlinkRun::FOnGetTooltipText TooltipTextDelegate;

	/** Delegate used to generate hyperlink's tooltip */
	FSlateHyperlinkRun::FOnGenerateTooltip TooltipDelegate;
};


/** Helper functions for generating rich text */
struct FTutorialText
{
public:
	static void GetRichTextDecorators(TArray<TSharedRef<class ITextDecorator>>& OutDecorators);

	static const TArray<TSharedPtr<FHyperlinkTypeDesc>>& GetHyperLinkDescs();

private:
	static void Initialize();

private:
	static TArray<TSharedPtr<FHyperlinkTypeDesc>> HyperlinkDescs;
};