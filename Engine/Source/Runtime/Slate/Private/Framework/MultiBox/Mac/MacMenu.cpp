// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlatePrivatePCH.h"
#include "MacMenu.h"
#include "CocoaThread.h"

@interface FMacMenuItem : NSMenuItem
@property (assign) TSharedPtr<const FMenuEntryBlock> MenuEntryBlock;
- (void)performAction;
@end

@implementation FMacMenuItem

- (id)initWithMenuEntryBlock:(TSharedRef< const FMenuEntryBlock >&)Block
{
	self = [super initWithTitle:@"" action:nil keyEquivalent:@""];
	self.MenuEntryBlock = Block;
	return self;
}

- (void)performAction
{
	FCocoaMenu* CocoaMenu = [[self menu] isKindOfClass:[FCocoaMenu class]] ? (FCocoaMenu*)[self menu] : nil;
	if ( !CocoaMenu || ![CocoaMenu isHighlightingKeyEquivalent] )
	{
		FSlateMacMenu::ExecuteMenuItemAction(self.MenuEntryBlock.ToSharedRef());
	}
}

@end

@implementation FMacMenu

- (id)initWithMenuEntryBlock:(TSharedRef< const FMenuEntryBlock >&)Block
{
	self = [super initWithTitle:@""];
	[self setDelegate:self];
	self.MenuEntryBlock = Block;
	return self;
}

- (void)menuNeedsUpdate:(NSMenu*)Menu
{
	FSlateMacMenu::UpdateMenu(self);
}

@end

void FSlateMacMenu::UpdateWithMultiBox(const TSharedRef< FMultiBox >& MultiBox)
{
	MainThreadCall(^{
		int32 NumItems = [[NSApp mainMenu] numberOfItems];
		FText HelpTitle = NSLOCTEXT("MainMenu", "HelpMenu", "Help");
		NSMenuItem* HelpMenu = [[[NSApp mainMenu] itemWithTitle:HelpTitle.ToString().GetNSString()] retain];
		for (int32 Index = NumItems - 1; Index > 0; Index--)
		{
			[[NSApp mainMenu] removeItemAtIndex:Index];
		}

		const TArray<TSharedRef<const FMultiBlock> >& MenuBlocks = MultiBox->GetBlocks();

		for (int32 Index = 0; Index < MenuBlocks.Num(); Index++)
		{
			TSharedRef<const FMenuEntryBlock> Block = StaticCastSharedRef<const FMenuEntryBlock>(MenuBlocks[Index]);
			FMacMenu* Menu = [[FMacMenu alloc] initWithMenuEntryBlock:Block];
			NSString* Title = FSlateMacMenu::GetMenuItemTitle(Block);
			[Menu setTitle:Title];

			NSMenuItem* MenuItem = [[NSMenuItem new] autorelease];
			[MenuItem setTitle:Title];
			[[NSApp mainMenu] addItem:MenuItem];
			[MenuItem setSubmenu:Menu];
		}

		if ([[NSApp mainMenu] itemWithTitle:HelpTitle.ToString().GetNSString()] == nil && HelpMenu)
		{
			[[NSApp mainMenu] addItem:HelpMenu];
		}

		if (HelpMenu)
		{
			[HelpMenu release];
		}
	});
}

void FSlateMacMenu::UpdateMenu(FMacMenu* Menu)
{
	GameThreadCall(^{
		FText WindowLabel = NSLOCTEXT("MainMenu", "WindowMenu", "Window");
		const bool bIsWindowMenu = (WindowLabel.ToString().Compare(FString([Menu title])) == 0);

		if ([[Menu itemArray] count] == 0)
		{
			if (bIsWindowMenu)
			{
				NSMenuItem* MinimizeItem = [[[NSMenuItem alloc] initWithTitle:@"Minimize" action:@selector(miniaturize:) keyEquivalent:@"m"] autorelease];
				NSMenuItem* ZoomItem = [[[NSMenuItem alloc] initWithTitle:@"Zoom" action:@selector(performZoom:) keyEquivalent:@""] autorelease];
				NSMenuItem* CloseItem = [[[NSMenuItem alloc] initWithTitle:@"Close" action:@selector(performClose:) keyEquivalent:@"w"] autorelease];
				NSMenuItem* BringAllToFrontItem = [[[NSMenuItem alloc] initWithTitle:@"Bring All to Front" action:@selector(arrangeInFront:) keyEquivalent:@""] autorelease];

				[Menu addItem:MinimizeItem];
				[Menu addItem:ZoomItem];
				[Menu addItem:CloseItem];
				[Menu addItem:[NSMenuItem separatorItem]];
				[Menu addItem:BringAllToFrontItem];
				[Menu addItem:[NSMenuItem separatorItem]];
			}
		}

		if (!Menu.MultiBox.IsValid())
		{
			const bool bShouldCloseWindowAfterMenuSelection = true;
			FMenuBuilder MenuBuilder(bShouldCloseWindowAfterMenuSelection, Menu.MenuEntryBlock->GetActionList(), Menu.MenuEntryBlock->Extender);
			{
				// Have the menu fill its contents
				Menu.MenuEntryBlock->EntryBuilder.ExecuteIfBound(MenuBuilder);
			}
			TSharedRef<SWidget> Widget = MenuBuilder.MakeWidget();
			Menu.MultiBox = TSharedPtr<const FMultiBox>(StaticCastSharedRef<SMultiBoxWidget>(Widget)->GetMultiBox());
		}

		const TArray<TSharedRef<const FMultiBlock>>& MenuBlocks = Menu.MultiBox->GetBlocks();
		int32 ItemIndexAdjust = 0;
		for (int32 Index = 0; Index < MenuBlocks.Num(); Index++)
		{
			const int32 ItemIndex = (bIsWindowMenu ? Index + 6 : Index) - ItemIndexAdjust;
			NSMenuItem* MenuItem = [Menu numberOfItems] > ItemIndex ? [Menu itemAtIndex:ItemIndex] : nil;

			EMultiBlockType::Type Type = MenuBlocks[Index]->GetType();
			if (Type == EMultiBlockType::MenuEntry)
			{
				TSharedRef<const FMenuEntryBlock> Block = StaticCastSharedRef<const FMenuEntryBlock>(MenuBlocks[Index]);
				if (MenuItem && (![MenuItem isKindOfClass:[FMacMenuItem class]] || (Block->bIsSubMenu && [MenuItem submenu] == nil) || (!Block->bIsSubMenu && [MenuItem submenu] != nil)))
				{
					[Menu removeItem:MenuItem];
					MenuItem = nil;
				}
				if (!MenuItem)
				{
					MenuItem = [[[FMacMenuItem alloc] initWithMenuEntryBlock:Block] autorelease];

					if (Block->bIsSubMenu)
					{
						FMacMenu* SubMenu = [[FMacMenu alloc] initWithMenuEntryBlock:Block];
						[MenuItem setSubmenu:SubMenu];
					}

					if ([Menu numberOfItems] > ItemIndex)
					{
						[Menu insertItem:MenuItem atIndex:ItemIndex];
					}
					else
					{
						[Menu addItem:MenuItem];
					}
				}

				[MenuItem setTitle:FSlateMacMenu::GetMenuItemTitle(Block)];

				uint32 KeyModifiers;
				NSString* KeyEquivalent = FSlateMacMenu::GetMenuItemKeyEquivalent(Block, &KeyModifiers);
				[MenuItem setKeyEquivalent:KeyEquivalent];
				[MenuItem setKeyEquivalentModifierMask:KeyModifiers];

				if (bIsWindowMenu)
				{
					NSImage* MenuImage = FSlateMacMenu::GetMenuItemIcon(Block);
					if(MenuImage)
					{
						[MenuItem setImage:MenuImage];
						[MenuImage release];
					}
				}
				else
				{
					[MenuItem setImage:nil];
				}

				if (FSlateMacMenu::IsMenuItemEnabled(Block))
				{
					[MenuItem setTarget:MenuItem];
					[MenuItem setAction:@selector(performAction)];
				}

				if (!Block->bIsSubMenu)
				{
					[MenuItem setState:FSlateMacMenu::GetMenuItemState(Block)];
				}
			}
			else if (Type == EMultiBlockType::MenuSeparator)
			{
				if (MenuItem && ![MenuItem isSeparatorItem])
				{
					[Menu removeItem:MenuItem];
				}
				else if (!MenuItem)
				{
					if ([Menu numberOfItems] > ItemIndex)
					{
						[Menu insertItem:[NSMenuItem separatorItem] atIndex:ItemIndex];
					}
					else
					{
						[Menu addItem:[NSMenuItem separatorItem]];
					}
				}
			}
			else
			{
				// If it's a type we skip, update ItemIndexAdjust so we can properly calculate item's index in NSMenu
				ItemIndexAdjust++;
			}
		}
	}, InGameRunLoopMode(@[ UE4NilEventMode, UE4ShowEventMode, UE4ResizeEventMode, UE4FullscreenEventMode, UE4CloseEventMode, UE4IMEEventMode ]));
}

void FSlateMacMenu::ExecuteMenuItemAction(const TSharedRef< const class FMenuEntryBlock >& Block)
{
    TSharedPtr< const class FMenuEntryBlock>* MenuBlock = new TSharedPtr< const class FMenuEntryBlock>(Block);
	GameThreadCall(^{
		TSharedPtr< const FUICommandList > ActionList = (*MenuBlock)->GetActionList();
		if (ActionList.IsValid() && (*MenuBlock)->GetAction().IsValid())
		{
			ActionList->ExecuteAction((*MenuBlock)->GetAction().ToSharedRef());
		}
		else
		{
			// There is no action list or action associated with this block via a UI command.  Execute any direct action we have
			(*MenuBlock)->GetDirectActions().Execute();
		}
        delete MenuBlock;
	}, NSDefaultRunLoopMode, false);
}

static const TSharedRef<SWidget> FindTextBlockWidget(TSharedRef<SWidget> Content)
{
	if (Content->GetType() == FName(TEXT("STextBlock")))
	{
		return Content;
	}

	FChildren* Children = Content->GetChildren();
	int32 NumChildren = Children->Num();

	for (int32 Index=0; Index < NumChildren; ++Index)
	{
		const TSharedRef<SWidget> Found = FindTextBlockWidget(Children->GetChildAt(Index));
		if (Found != SNullWidget::NullWidget)
		{
			return Found;
		}
	}
	return SNullWidget::NullWidget;
}

NSString* FSlateMacMenu::GetMenuItemTitle(const TSharedRef<const FMenuEntryBlock>& Block)
{
	TAttribute<FText> Label;
	if (!Block->LabelOverride.IsBound() && Block->LabelOverride.Get().IsEmpty() && Block->GetAction().IsValid())
	{
		Label = Block->GetAction()->GetLabel();
	}
	else if (!Block->LabelOverride.Get().IsEmpty())
	{
		Label = Block->LabelOverride;
	}
	else if (Block->EntryWidget.IsValid())
	{
		const TSharedRef<SWidget>& TextBlockWidget = FindTextBlockWidget(Block->EntryWidget.ToSharedRef());
		if (TextBlockWidget != SNullWidget::NullWidget)
		{
			Label = StaticCastSharedRef<STextBlock>(TextBlockWidget)->GetText();
		}
	}

	return Label.Get().ToString().GetNSString();
}

NSImage* FSlateMacMenu::GetMenuItemIcon(const TSharedRef<const FMenuEntryBlock>& Block)
{
	NSImage* MenuImage = nil;
	FSlateIcon Icon;
	if (Block->IconOverride.IsSet())
	{
		Icon = Block->IconOverride;
	}
	else if (Block->GetAction().IsValid() && Block->GetAction()->GetIcon().IsSet())
	{
		Icon = Block->GetAction()->GetIcon();
	}
	if (Icon.IsSet())
	{
		if (Icon.GetIcon())
		{
			FSlateBrush const* IconBrush = Icon.GetIcon();
			FName ResourceName = IconBrush->GetResourceName();
			MenuImage = [[NSImage alloc] initWithContentsOfFile:ResourceName.ToString().GetNSString()];
			if (MenuImage)
			{
				[MenuImage setSize:NSMakeSize(16.0f, 16.0f)];
			}
		}
	}
	return MenuImage;
}

NSString* FSlateMacMenu::GetMenuItemKeyEquivalent(const TSharedRef<const class FMenuEntryBlock>& Block, uint32* OutModifiers)
{
	if (Block->GetAction().IsValid())
	{
		const TSharedRef<const FInputGesture>& Gesture = Block->GetAction()->GetActiveGesture();

		*OutModifiers = 0;
		if (Gesture->bCtrl)
		{
			*OutModifiers |= NSCommandKeyMask;
		}
		if (Gesture->bShift)
		{
			*OutModifiers |= NSShiftKeyMask;
		}
		if (Gesture->bAlt)
		{
			*OutModifiers |= NSAlternateKeyMask;
		}
		if (Gesture->bCmd)
		{
			*OutModifiers |= NSControlKeyMask;
		}

		FString KeyString = Gesture->GetKeyText().ToString().ToLower();
		return KeyString.GetNSString();
	}
	return @"";
}

bool FSlateMacMenu::IsMenuItemEnabled(const TSharedRef<const class FMenuEntryBlock>& Block)
{
	TSharedPtr<const FUICommandList> ActionList = Block->GetActionList();
	TSharedPtr<const FUICommandInfo> Action = Block->GetAction();
	const FUIAction& DirectActions = Block->GetDirectActions();

	bool bEnabled = true;
	if (ActionList.IsValid() && Action.IsValid())
	{
		bEnabled = ActionList->CanExecuteAction(Action.ToSharedRef());
	}
	else
	{
		// There is no action list or action associated with this block via a UI command.  Execute any direct action we have
		bEnabled = DirectActions.CanExecute();
	}

	return bEnabled;
}

int32 FSlateMacMenu::GetMenuItemState(const TSharedRef<const class FMenuEntryBlock>& Block)
{
	bool bIsChecked = false;
	TSharedPtr<const FUICommandList> ActionList = Block->GetActionList();
	TSharedPtr<const FUICommandInfo> Action = Block->GetAction();
	const FUIAction& DirectActions = Block->GetDirectActions();

	if (ActionList.IsValid() && Action.IsValid())
	{
		bIsChecked = ActionList->IsChecked(Action.ToSharedRef());
	}
	else
	{
		// There is no action list or action associated with this block via a UI command.  Execute any direct action we have
		bIsChecked = DirectActions.IsChecked();
	}

	return bIsChecked ? NSOnState : NSOffState;
}
