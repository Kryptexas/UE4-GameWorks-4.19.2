// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#if WITH_FANCY_TEXT

#include "BaseTextLayoutMarshaller.h"

/**
 * Get/set the raw text to/from a text layout as plain text
 */
class SLATE_API FPlainTextLayoutMarshaller : public FBaseTextLayoutMarshaller
{
public:

	static TSharedRef< FPlainTextLayoutMarshaller > Create(FTextBlockStyle InDefaultTextStyle);

	virtual ~FPlainTextLayoutMarshaller();
	
	// ITextLayoutMarshaller
	virtual void SetText(const FString& SourceString, FTextLayout& TargetTextLayout) override;
	virtual void GetText(FString& TargetString, const FTextLayout& SourceTextLayout) override;

	void SetDefaultTextStyle(FTextBlockStyle InDefaultTextStyle);
	const FTextBlockStyle& GetDefaultTextStyle() const;

protected:

	FPlainTextLayoutMarshaller(FTextBlockStyle InDefaultTextStyle);

	/** Default style used by the TextLayout */
	FTextBlockStyle DefaultTextStyle;

};

#endif //WITH_FANCY_TEXT
