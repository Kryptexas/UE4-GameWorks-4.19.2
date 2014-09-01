// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#if WITH_FANCY_TEXT

#include "SlateTextLayoutMarshaller.h"

/**
 * Get/set the raw text to/from a text layout as plain text
 */
class SLATE_API FPlainTextLayoutMarshaller : public FSlateTextLayoutMarshaller
{
public:

	static TSharedRef< FPlainTextLayoutMarshaller > Create(FTextBlockStyle InDefaultTextStyle);

	virtual ~FPlainTextLayoutMarshaller();
	
	// ITextLayoutMarshaller
	virtual void SetText(const FString& SourceString, FTextLayout& TargetTextLayout) override;
	virtual void GetText(FString& TargetString, const FTextLayout& SourceTextLayout) override;

protected:

	FPlainTextLayoutMarshaller(FTextBlockStyle InDefaultTextStyle);

};

#endif //WITH_FANCY_TEXT
