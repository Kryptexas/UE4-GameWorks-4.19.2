// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MultiBox.h"
#include "SMenuEntryBlock.h"

#ifdef __OBJC__
@interface FMacMenu : NSMenu <NSMenuDelegate>
{
	TSharedPtr<const FMenuEntryBlock> MenuEntryBlock;
}
- (TSharedPtr<const FMenuEntryBlock>&)GetMenuEntryBlock;
@end
#else
class FMacMenu;
#endif

class SLATE_API FSlateMacMenu
{
public:

	static void UpdateWithMultiBox(const TSharedRef< FMultiBox >& MultiBox);
	static CFStringRef GetMenuItemTitle(const TSharedRef< const class FMenuEntryBlock >& Block);
	static NSImage* GetMenuItemIcon(const TSharedRef<const FMenuEntryBlock>& Block);
	static void ExecuteMenuItemAction(const TSharedRef< const class FMenuEntryBlock >& Block);
	static CFStringRef GetMenuItemKeyEquivalent(const TSharedRef< const class FMenuEntryBlock >& Block, uint32* OutModifiers);
	static void UpdateMenu(FMacMenu* InMenu);
	static bool IsMenuItemEnabled(const TSharedRef<const class FMenuEntryBlock>& Block);
	static int32 GetMenuItemState(const TSharedRef<const class FMenuEntryBlock>& Block);
};
