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

		SUserWidget::Construct(SUserWidget::FArguments()
		[
			SNew(SBorder)
			.Padding(0)
			.BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
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
					.Expose(SettingBox)
					.AutoWidth()
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Center)
					+SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Center)
					[
						SNew(SButton)
						.ButtonStyle(&FriendStyle.FriendsChatStyle.FriendsMinimizeButtonStyle)
						.Visibility(this, &SChatChromeImpl::GetMinimizeVisibility)
						.OnClicked(this, &SChatChromeImpl::ToggleHeader)
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

		if(SettingBox && ChromeViewModel->DisplayChatSettings())
		{
			SettingBox->AttachWidget(
				GetChatSettingsWidget().ToSharedRef()
			);
		}
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
				return FriendStyle.FriendsNormalFontStyle.DefaultFontColor;
			}
		}
		return FriendStyle.FriendsNormalFontStyle.InvertedFontColor;
	}

	FSlateColor GetTabFontColor(TWeakPtr<IChatTabViewModel> TabPtr) const
	{
		TSharedPtr<IChatTabViewModel> Tab = TabPtr.Pin();
		if (Tab.IsValid())
		{
			if (Tab == ChromeViewModel->GetActiveTab())
			{
				EChatMessageType::Type Channel = Tab->GetTabID();
				return FriendStyle.FriendsChatStyle.GetChannelTextColor(Channel);
			}
		}
		return FriendStyle.FriendsNormalFontStyle.InvertedFontColor;
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

			TSharedPtr<SBox> NotificationContainer = SNew(SBox);
			
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
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							NotificationContainer.ToSharedRef()
						]
					]
				]
			];

			// Add a quick settings if needed
			//QuickSettingsContainer->SetContent(GetQuickSettingsWidget(Tab).ToSharedRef());
			
			// Add Message notification indicator
			NotificationContainer->SetContent(GetNotificationWidget(Tab).ToSharedRef());


			if (!ChatWindows.Contains(Tab))
			{
				// Create Chat window for this tab
				TSharedRef<SChatWindow> ChatWindow = SNew(SChatWindow, Tab->GetChatViewModel().ToSharedRef())
					.FriendStyle(&FriendStyle)
					.Method(EPopupMethod::UseCurrentWindow);

				ChatWindow->SetVisibility(EVisibility::SelfHitTestInvisible);

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
				.ChatType(Tab->GetChatViewModel()->GetDefaultChannelType())
				.FriendStyle(&FriendStyle)
			];
	}

	EVisibility GetMessageNotificationVisibility(TWeakPtr<IChatTabViewModel> TabPtr) const
	{
		TSharedPtr<IChatTabViewModel> Tab = TabPtr.Pin();
		if (Tab.IsValid())
		{
			if (Tab->GetChatViewModel()->GetUnreadChannelMessageCount() > 0)
			{
				return EVisibility::Visible;
			}
		}
		return EVisibility::Collapsed;
	}

	TSharedPtr<SWidget> GetNotificationWidget(const TSharedRef<IChatTabViewModel>& Tab) const
	{
		return SNew(SBox)
		.Visibility(this, &SChatChromeImpl::GetMessageNotificationVisibility, TWeakPtr<IChatTabViewModel>(Tab))
		[
			SNew(SBorder)
			.Padding(FMargin(4.0f))
			.BorderImage(&FriendStyle.FriendsChatStyle.MessageNotificationBrush)
			[
				SNew(STextBlock)
				.ColorAndOpacity(FriendStyle.FriendsNormalFontStyle.DefaultFontColor)
				.Font(FriendStyle.FriendsNormalFontStyle.FriendsFontSmallBold)
				.Text(this, &SChatChromeImpl::GetMessageNotificationsText, TWeakPtr<IChatTabViewModel>(Tab))
			]
		];
	}

	FText GetMessageNotificationsText(TWeakPtr<IChatTabViewModel> TabPtr) const
	{
		TSharedPtr<IChatTabViewModel> Tab = TabPtr.Pin();
		if (Tab.IsValid())
		{
			return Tab->GetMessageNotificationsText();
		}
		return FText::GetEmpty();
	}


	EVisibility GetChatWindowVisibility() const
	{
		return ChromeViewModel->IsActive() ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
	}

	EVisibility GetHeaderVisibility() const
	{
		return ChromeViewModel->GetHeaderVisibility();
	}

	FReply ToggleHeader()
	{
		ChromeViewModel->ToggleChatMinimized();
		return FReply::Handled();
	}

	EVisibility GetMinimizeVisibility() const
	{
		return (!ChromeViewModel->IsMinimizeEnabled() || ChromeViewModel->IsChatMinimized()) ? EVisibility::Collapsed : EVisibility::Visible;
	}

	// Holds the menu anchor
	TSharedPtr<SMenuAnchor> ActionMenu;

	TSharedPtr<FChatChromeViewModel> ChromeViewModel;
	TSharedPtr<SHorizontalBox> TabContainer;
	TSharedPtr<SWidgetSwitcher> ChatWindowContainer;
	SHorizontalBox::FSlot* SettingBox;

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
