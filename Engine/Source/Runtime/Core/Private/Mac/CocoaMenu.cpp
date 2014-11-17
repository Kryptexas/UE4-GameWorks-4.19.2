// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Core.h"
#include "CocoaMenu.h"
#include "MacApplication.h"

@implementation FCocoaMenu

- (id)initWithTitle:(NSString *)Title
{
	id Self = [super initWithTitle:Title];
	if (Self)
	{
		bHighlightingKeyEquivalent = false;
	}
	return Self;
}

- (bool)isHighlightingKeyEquivalent
{
	FCocoaMenu* SuperMenu = [[self supermenu] isKindOfClass:[FCocoaMenu class]] ? (FCocoaMenu*)[self supermenu] : nil;
	if ( SuperMenu )
	{
		return [SuperMenu isHighlightingKeyEquivalent];
	}
	else
	{
		return bHighlightingKeyEquivalent;
	}
}

- (bool)highlightKeyEquivalent:(NSEvent *)Event
{
	bHighlightingKeyEquivalent = true;
	bool bHighlighted = [super performKeyEquivalent:Event];
	bHighlightingKeyEquivalent = false;
	return bHighlighted;
}

@end
