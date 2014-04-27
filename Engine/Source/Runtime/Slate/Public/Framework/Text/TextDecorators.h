// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "ITextDecorator.h"

class SLATE_API FWidgetDecorator : public ITextDecorator
{
public:

	DECLARE_DELEGATE_RetVal_TwoParams( FSlateWidgetRun::FWidgetRunInfo, FCreateWidget, const FTextRunInfo& /*RunInfo*/, const ISlateStyle* /*Style*/ )

public:

	static TSharedRef< FWidgetDecorator > Create( FString InRunName, const FCreateWidget& InCreateWidgetDelegate );
	virtual ~FWidgetDecorator() {}

public:

	virtual bool Supports( const FTextRunParseResults& RunInfo, const FString& Text ) const OVERRIDE;

	virtual TSharedRef< ISlateRun > Create( const FTextRunParseResults& RunInfo, const FString& OriginalText, const TSharedRef< FString >& InOutModelText, const ISlateStyle* Style ) OVERRIDE;

private:

	FWidgetDecorator( FString InRunName, const FCreateWidget& InCreateWidgetDelegate );

private:

	FString RunName;
	FCreateWidget CreateWidgetDelegate;
};

class SLATE_API FImageDecorator : public ITextDecorator
{
public:

	static TSharedRef< FImageDecorator > Create( FString InRunName, const ISlateStyle* const InOverrideStyle = NULL );
	virtual ~FImageDecorator() {}

public:

	virtual bool Supports( const FTextRunParseResults& RunInfo, const FString& Text ) const OVERRIDE;

	virtual TSharedRef< ISlateRun > Create( const FTextRunParseResults& RunInfo, const FString& OriginalText, const TSharedRef< FString >& InOutModelText, const ISlateStyle* Style ) OVERRIDE;

private:

	FImageDecorator( FString InRunName, const ISlateStyle* const InOverrideStyle );

private:

	FString RunName;
	const ISlateStyle* OverrideStyle;
};

class SLATE_API FHyperlinkDecorator : public ITextDecorator
{
public:

	static TSharedRef< FHyperlinkDecorator > Create( FString Id, const FSlateHyperlinkRun::FOnClick& NavigateDelegate );
	virtual ~FHyperlinkDecorator() {}

public:

	virtual bool Supports( const FTextRunParseResults& RunInfo, const FString& Text ) const OVERRIDE;

	virtual TSharedRef< ISlateRun > Create( const FTextRunParseResults& RunInfo, const FString& OriginalText, const TSharedRef< FString >& InOutModelText, const ISlateStyle* Style ) OVERRIDE;

private:

	FHyperlinkDecorator( FString InId, const FSlateHyperlinkRun::FOnClick& InNavigateDelegate );

private:

	FString Id;
	FSlateHyperlinkRun::FOnClick NavigateDelegate;
};
