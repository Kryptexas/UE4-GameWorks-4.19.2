// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "ChatChromeViewModel.h"
#include "SChatChrome.h"
#include "SChatWindow.h"
#include "SChatChromeTabQuickSettings.h"
#include "SChatChromeQuickSettings.h"
#include "SWidgetSwitcher.h"
#include "IChatTabViewModel.h"
#include "ChatViewModel.h"

#define LOCTEXT_NAMESPACE "SChatChrome"

class SChatChromeImpl : public SChatChrome
{
public:

	void Construct(const FArguments& InArgs, const TSharedRef<FChatChromeViewModel>& InChromeViewModel) override
	{
		FriendStyle = *InArgs._FriendStyle;
		MenuMethod = InArgs._Method;
		ChromeViewModel = InChromeViewModel;

		ChromeViewModel->OnActiveTabChanged().AddSP(this, &SChatChromeImpl::HandleActiveTabChanged);
		ChromeViewModel->OnVisibleTabsChanged().AddSP(this, &SChatChromeImpl::HandleVisibleTabsChanged);
		FLinearColor BorderColor = ChromeViewModel->IsFading() ? FLinearColor::Transparent : FLinearColor::White;

		SUserWidget::Construct(SUserWidget::FArguments()
		[
			SNew(SBorder)
			.BorderImage(&FriendStyle.FriendsChatChromeStyle.ChatBackgroundBrush)
			.BorderBackgroundColor(BorderColor)
			.Visibility(this, &SChatChromeImpl::GetChatWindowVisibility)
			.Padding(0)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					.Visibility(this, &SChatChromeImpl::GetHeaderVisibility)
					+SHorizontalBox::Slot()
					[
						SAssignNew(TabContainer, SHorizontalBox)
					]
					+SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Center)
					[
						GetChatSettingsWidget().ToSharedRef()
					]
				]
				+ SVerticalBox::Slot()
				[
					SAssignNew(ChatWindowContainer, SWidgetSwitcher)
				]
			]
		]);

		RegenerateTabs();
		HandleActiveTabChanged();
	}


	virtual void HandleWindowActivated() override
	{
		
	}

	void HandleVisibleTabsChanged()
	{
	}

	void HandleActiveTabChanged()
	{
		if (ChromeViewModel.IsValid())
		{
			TSharedPtr<IChatTabViewModel> Tab = ChromeViewModel->GetActiveTab();
			if (Tab.IsValid())
			{
				TSharedPtr<SChatWindow> ChatWindow = *ChatWindows.Find(Tab);
				if (ChatWindow.IsValid())
				{
					ChatWindowContainer->SetActiveWidget(ChatWindow.ToSharedRef());
					ChatWindow->HandleWindowActivated();
				}
			}
		}
	}

private:

	FReply OnTabClicked(TWeakPtr<IChatTabViewModel> TabPtr)
	{
		TSharedPtr<IChatTabViewModel> Tab = TabPtr.Pin();
		if (Tab.IsValid())
		{
			ChromeViewModel->ActivateTab(Tab.ToSharedRef());
		}
		return FReply::Handled();
	}

	const FSlateBrush* GetTabBackgroundBrush(TWeakPtr<IChatTabViewModel> TabPtr) const
	{
		TSharedPtr<IChatTabViewModel> Tab = TabPtr.Pin();
		if (Tab.IsValid())
		{
			if (Tab == ChromeViewModel->GetActiveTab())
			{
				return &FriendStyle.FriendsChatChromeStyle.TabBackgroundBrush;
			}
		}
		return &FriendStyle.FriendsChatChromeStyle.ChatBackgroundBrush;
	}

	FSlateColor GetTabBackgroundColor(TWeakPtr<IChatTabViewModel> TabPtr) const
	{
		TSharedPtr<IChatTabViewModel> Tab = TabPtr.Pin();
		if (Tab.IsValid())
		{
			if (Tab == ChromeViewModel->GetActiveTab())
			{
				return FLinearColor::Gray;
			}
		}
		return FLinearColor::White;
	}

	FSlateColor GetTabFontColor(TWeakPtr<IChatTabViewModel> TabPtr) const
	{
		TSharedPtr<IChatTabViewModel> Tab = TabPtr.Pin();
		if (Tab.IsValid())
		{
			if (Tab == ChromeViewModel->GetActiveTab())
			{
				return FLinearColor::Black;
			}
		}
		return FLinearColor::White;
	}

	EVisibility GetTabVisibility(TWeakPtr<IChatTabViewModel> TabPtr) const
	{
		TSharedPtr<IChatTabViewModel> Tab = TabPtr.Pin();
		if (Tab.IsValid() && Tab->IsTabVisible())
		{
			return EVisibility::Visible;
		}
		return EVisibility::Collapsed;
	}
	
	void RegenerateTabs()
	{
		TabContainer->ClearChildren();

		// Clear Tabs
		for (const TSharedRef<IChatTabViewModel>& Tab : ChromeViewModel->GetAllTabs())
		{
			// Create Button for Tab
			TSharedRef<SButton> TabButton = SNew(SButton)
			.ButtonColorAndOpacity(FLinearColor::Transparent)
			.OnClicked(this, &SChatChromeImpl::OnTabClicked, TWeakPtr<IChatTabViewModel>(Tab))
			[
				SNew(STextBlock)
				.ColorAndOpacity(this, &SChatChromeImpl::GetTabFontColor, TWeakPtr<IChatTabViewModel>(Tab))
				.Font(FriendStyle.FriendsNormalFontStyle.FriendsFontSmallBold)
				.Text(Tab->GetTabText())
			];

			// Create container for a quick settings options
			TSharedPtr<SBox> QuickSettingsContainer = SNew(SBox);
			
			// Add Tab
			TabContainer->AddSlot()
			.Padding(0, 0, 0, 0)
			.AutoWidth()
			[
				SNew(SBox)
				.Visibility(this, &SChatChromeImpl::GetTabVisibility, TWeakPtr<IChatTabViewModel>(Tab))
				[
					SNew(SBorder)
					.BorderImage(this, &SChatChromeImpl::GetTabBackgroundBrush, TWeakPtr<IChatTabViewModel>(Tab))
					.BorderBackgroundColor(this, &SChatChromeImpl::GetTabBackgroundColor, TWeakPtr<IChatTabViewModel>(Tab))
					.Padding(5.0f)
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.AutoWidth()
						[
							TabButton
						]
						+ SHorizontalBox::Slot()
							.AutoWidth()
						[
							QuickSettingsContainer.ToSharedRef()
						]
					]
				]
			];

			// Add a quick settings if needed
			QuickSettingsContainer->SetContent(GetQuickSettingsWidget(Tab).ToSharedRef());
			
			if (!ChatWindows.Contains(Tab))
			{
				// Create Chat window for this tab
				TSharedRef<SChatWindow> ChatWindow = SNew(SChatWindow, Tab->GetChatViewModel().ToSharedRef())
					.FriendStyle(&FriendStyle)
					.Method(EPopupMethod::UseCurrentWindow);

				ChatWindowContainer->AddSlot()
					[
						ChatWindow
					];
				// Add Chat window to storage
				ChatWindows.Add(Tab, ChatWindow);
			}
		}
	}

	TSharedPtr<SWidget> GetChatSettingsWidget() const
	{
		return SNew(SComboButton)
			.ForegroundColor(FLinearColor::White)
			.ButtonColorAndOpacity(FLinearColor::Transparent)
			.ButtonContent()
			[
				SNew(SImage)
				.Image(&FriendStyle.FriendsChatStyle.ChatSettingsBrush)
			]
			.ContentPadding(FMargin(2, 2.5))
			.MenuContent()
			[
				SNew(SChatChromeQuickSettings, ChromeViewModel->GetChatSettingsViewModel(), ChromeViewModel)
				.FriendStyle(&FriendStyle)
			];
	}

	TSharedPtr<SWidget> GetQuickSettingsWidget(const TSharedRef<IChatTabViewModel>& Tab) const
	{
		return SNew(SComboButton)
			.ForegroundColor(FLinearColor::White)
			.ButtonColorAndOpacity(FLinearColor::Transparent)
			.ContentPadding(FMargin(2, 2.5))
			.MenuContent()
			[
				SNew(SChatChromeTabQuickSettings, Tab)
				.ChatType(Tab->GetChatViewModel()->GetChatChannelType())
				.FriendStyle(&FriendStyle)
			];
	}

	EVisibility GetChatWindowVisibility() const
	{
		return ChromeViewModel->IsActive() ? EVisibility::Visible : EVisibility::Collapsed;
	}

	EVisibility GetHeaderVisibility() const
	{
		return ChromeViewModel->GetHeaderVisibility();
	}

	// Holds the menu anchor
	TSharedPtr<SMenuAnchor> ActionMenu;

	TSharedPtr<FChatChromeViewModel> ChromeViewModel;
	TSharedPtr<SHorizontalBox> TabContainer;
	TSharedPtr<SWidgetSwitcher> ChatWindowContainer;

	TMap<TSharedPtr<IChatTabViewModel>, TSharedPtr<SChatWindow>> ChatWindows;
	/** Holds the style to use when making the widget. */
	FFriendsAndChatStyle FriendStyle;

	// Holds the menu method - Full screen requires use owning window or crashes.
	EPopupMethod MenuMethod;

};

TSharedRef<SChatChrome> SChatChrome::New()
{
	return MakeShareable(new SChatChromeImpl());
}

#undef LOCTEXT_NAMESPACE
