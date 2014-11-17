// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * A set of utility functions used at design time for the widget blueprint editor.
 */
class FDesignTimeUtils
{
public:
	static bool GetArrangedWidget(TSharedRef<SWidget> Widget, FArrangedWidget& ArrangedWidget);

	static bool GetArrangedWidgetRelativeToWindow(TSharedRef<SWidget> Widget, FArrangedWidget& ArrangedWidget);

	static bool GetArrangedWidgetRelativeToParent(TSharedRef<const SWidget> Widget, TSharedRef<const SWidget> Parent, FArrangedWidget& ArrangedWidget);
};
