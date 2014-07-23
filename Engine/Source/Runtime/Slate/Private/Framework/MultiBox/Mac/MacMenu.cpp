// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlatePrivatePCH.h"
#include "MacMenu.h"

@interface FMacMenuItem : NSMenuItem
{
	TSharedPtr< const FMenuEntryBlock > MenuEntryBlock;
}

- (void)performAction;

@end

@implementation FMacMenuItem

- (id)initWithMenuEntryBlock:(TSharedRef< const FMenuEntryBlock >&)Block
{
	CFStringRef Title = FSlateMacMenu::GetMenuItemTitle(Block);
	uint32 KeyModifiers;
	CFStringRef KeyEquivalent = FSlateMacMenu::GetMenuItemKeyEquivalent(Block, &KeyModifiers);
	self = [super initWithTitle:(NSString*)Title action:nil keyEquivalent:(NSString*)KeyEquivalent];
	[self setKeyEquivalentModifierMask:KeyModifiers];
	NSImage* MenuImage = FSlateMacMenu::GetMenuItemIcon(Block);
	if(MenuImage)
	{
		[self setImage:MenuImage];
		[MenuImage release];
	}
	CFRelease(Title);
	CFRelease(KeyEquivalent);

	MenuEntryBlock = Block;

	if (FSlateMacMenu::IsMenuItemEnabled(Block))
	{
		[self setTarget:self];
		[self setAction:@selector(performAction)];
	}

	return self;
}

- (void)performAction
{
	FSlateMacMenu::ExecuteMenuItemAction(MenuEntryBlock.ToSharedRef());
}

@end

@implementation FMacMenu

- (id)initWithMenuEntryBlock:(TSharedRef< const FMenuEntryBlock >&)Block
{
	CFStringRef Title = FSlateMacMenu::GetMenuItemTitle(Block);
	self = [super initWithTitle:(NSString*)Title];
	CFRelease(Title);

	[self setDelegate:self];

	MenuEntryBlock = Block;
	
	return self;
}

- (void)dealloc
{
	[super dealloc];
}

- (void)menuNeedsUpdate:(NSMenu*)Menu
{
	FSlateMacMenu::UpdateMenu(self);
}

- ( TSharedPtr< const FMenuEntryBlock >& )GetMenuEntryBlock
{
	return MenuEntryBlock;
}

@end

void FSlateMacMenu::UpdateWithMultiBox(const TSharedRef< FMultiBox >& MultiBox)
{
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
		
		NSMenuItem* MenuItem = [[NSMenuItem new] autorelease];
		[MenuItem setTitle:[Menu title]];
		[[NSApp mainMenu] addItem:MenuItem];
		[MenuItem setSubmenu:Menu];
	}
	
	if([[NSApp mainMenu] itemWithTitle:HelpTitle.ToString().GetNSString()] == nil && HelpMenu)
	{
		[[NSApp mainMenu] addItem:HelpMenu];
	}
	if(HelpMenu)
	{
		[HelpMenu release];
	}
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

CFStringRef FSlateMacMenu::GetMenuItemTitle(const TSharedRef<const FMenuEntryBlock>& Block)
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
			Label = FText::FromString(StaticCastSharedRef<STextBlock>(TextBlockWidget)->GetText());
		}
	}

	CFStringRef Title = FPlatformString::TCHARToCFString(*Label.Get().ToString());
	return Title;
}

NSImage* FSlateMacMenu::GetMenuItemIcon(const TSharedRef<const FMenuEntryBlock>& Block)
{
	NSImage* MenuImage = nil;
	FSlateIcon Icon;
	if(Block->IconOverride.IsSet())
	{
		Icon = Block->IconOverride;
	}
	else if(Block->GetAction().IsValid() && Block->GetAction()->GetIcon().IsSet())
	{
		Icon = Block->GetAction()->GetIcon();
	}
	if(Icon.IsSet())
	{
		if(Icon.GetIcon())
		{
			FSlateBrush const* IconBrush = Icon.GetIcon();
			FName ResourceName = IconBrush->GetResourceName();
			MenuImage = [[NSImage alloc] initWithContentsOfFile: ResourceName.ToString().GetNSString()];
			if(MenuImage)
			{
				[MenuImage setSize:NSMakeSize(16.0f, 16.0f)];
			}
		}
	}
	return MenuImage;
}

void FSlateMacMenu::ExecuteMenuItemAction(const TSharedRef< const class FMenuEntryBlock >& Block)
{
	TSharedPtr< const FUICommandList > ActionList = Block->GetActionList();
	if (ActionList.IsValid() && Block->GetAction().IsValid())
	{
		ActionList->ExecuteAction(Block->GetAction().ToSharedRef());
	}
	else
	{
		// There is no action list or action associated with this block via a UI command.  Execute any direct action we have
		Block->GetDirectActions().Execute();
	}
}

CFStringRef FSlateMacMenu::GetMenuItemKeyEquivalent(const TSharedRef<const class FMenuEntryBlock>& Block, uint32* OutModifiers)
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

		FString KeyString = Gesture->GetKeyText().ToString().ToLower();
		return FPlatformString::TCHARToCFString(*KeyString);
	}
	return FPlatformString::TCHARToCFString(TEXT(""));
}

void FSlateMacMenu::UpdateMenu(FMacMenu* Menu)
{
	FText WindowLabel = NSLOCTEXT("MainMenu", "WindowMenu", "Window");
	
	bool bIsWindowMenu = (WindowLabel.ToString().Compare(FString([Menu title])) == 0);
	
	[Menu removeAllItems];
	
	if(bIsWindowMenu)
	{
		NSMenuItem* MinimizeItem = [[[NSMenuItem alloc] initWithTitle:@"Minimize" action:@selector(miniaturize:) keyEquivalent:@"m"] autorelease];
		NSMenuItem* ZoomItem = [[[NSMenuItem alloc] initWithTitle:@"Zoom" action:@selector(performZoom:) keyEquivalent:@""] autorelease];
		NSMenuItem* BringAllToFrontItem = [[[NSMenuItem alloc] initWithTitle:@"Bring All to Front" action:@selector(arrangeInFront:) keyEquivalent:@""] autorelease];
		
		[Menu addItem:MinimizeItem];
		[Menu addItem:ZoomItem];
		[Menu addItem:[NSMenuItem separatorItem]];
		[Menu addItem:BringAllToFrontItem];
		[Menu addItem:[NSMenuItem separatorItem]];
	}

	const bool bShouldCloseWindowAfterMenuSelection = true;
	FMenuBuilder MenuBuilder(bShouldCloseWindowAfterMenuSelection, [Menu GetMenuEntryBlock]->GetActionList(), [Menu GetMenuEntryBlock]->Extender);
	{
		// Have the menu fill its contents
		[Menu GetMenuEntryBlock]->EntryBuilder.ExecuteIfBound(MenuBuilder);
	}
	TSharedRef< SWidget > Widget = MenuBuilder.MakeWidget();

	TSharedRef<const FMultiBox> MultiBox = StaticCastSharedRef<SMultiBoxWidget>(Widget)->GetMultiBox();
	const TArray<TSharedRef<const FMultiBlock>>& MenuBlocks = MultiBox->GetBlocks();

	for (int32 Index = 0; Index < MenuBlocks.Num(); Index++)
	{
		EMultiBlockType::Type Type = MenuBlocks[Index]->GetType();

		if (Type == EMultiBlockType::MenuEntry)
		{
			TSharedRef<const FMenuEntryBlock> Block = StaticCastSharedRef<const FMenuEntryBlock>(MenuBlocks[Index]);
			FMacMenuItem* MenuItem = [[[FMacMenuItem alloc] initWithMenuEntryBlock:Block] autorelease];

			if (Block->bIsSubMenu)
			{
				FMacMenu* SubMenu = [[FMacMenu alloc] initWithMenuEntryBlock:Block];
				[MenuItem setSubmenu:SubMenu];
			}
			else
			{
				[MenuItem setState:FSlateMacMenu::GetMenuItemState(Block)];
			}
			
			if(!bIsWindowMenu)
			{
				bool bIsWindowSubMenu = false;
				NSMenu* SuperMenu = [Menu supermenu];
				while(!bIsWindowSubMenu && SuperMenu)
				{
					bIsWindowSubMenu = (WindowLabel.ToString().Compare(FString([SuperMenu title])) == 0);
					SuperMenu = [SuperMenu supermenu];
				}
				
				if(!bIsWindowSubMenu)
				{
					[MenuItem setImage:nil];
				}
			}

			[Menu addItem:MenuItem];
		}
		else if (Type == EMultiBlockType::MenuSeparator)
		{
			[Menu addItem:[NSMenuItem separatorItem]];
		}
	}
	
	if(bIsWindowMenu)
	{
		[NSApp setWindowsMenu:Menu];
	}
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
	TSharedPtr<const FUICommandList> ActionList = Block->GetActionList();
	TSharedPtr<const FUICommandInfo> Action = Block->GetAction();
	const FUIAction& DirectActions = Block->GetDirectActions();

	bool bIsChecked = true;
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
