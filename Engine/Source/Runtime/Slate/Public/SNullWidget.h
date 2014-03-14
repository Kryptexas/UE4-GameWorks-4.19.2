// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class SSpacer;

/**
 * Simple null widget, used as a placeholder with slots
 * All Slots should initialize their child widget to NullWidget
 */
class SLATE_API SNullWidget
{
public:
	static TSharedRef<class SWidget> NullWidget;
};


/**
 * Like a null widget, but visualizes itself as being explicitly missing
 */
class SLATE_API SMissingWidget
{
public:
	static TSharedRef<class SWidget> MakeMissingWidget();
};


/**
 * Like a missing widget, but says it's a document area
 */
class SLATE_API SDocumentAreaWidget
{
public:
	static TSharedRef<class SWidget> MakeDocumentAreaWidget();
};
