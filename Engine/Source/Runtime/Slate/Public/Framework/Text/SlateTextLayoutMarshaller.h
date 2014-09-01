// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#if WITH_FANCY_TEXT

#include "BaseTextLayoutMarshaller.h"

/**
 * Base class implementing some common functionality for all Slate text marshallers
 */
class SLATE_API FSlateTextLayoutMarshaller : public FBaseTextLayoutMarshaller
{
public:

	void SetDefaultTextStyle(FTextBlockStyle InDefaultTextStyle);
	const FTextBlockStyle& GetDefaultTextStyle() const;

protected:

	FSlateTextLayoutMarshaller(FTextBlockStyle InDefaultTextStyle);

	/** Default style used by the TextLayout */
	FTextBlockStyle DefaultTextStyle;
};

#endif //WITH_FANCY_TEXT
