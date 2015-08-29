// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "SChatChromeQuickSettings.h"
#include "SChatChromeSettings.h"
#include "FriendsAndChatUserSettings.h"
#include "ChatSettingsViewModel.h"
#include "ChatChromeViewModel.h"

#define LOCTEXT_NAMESPACE "SChatChromePreferencesImpl"

class SChatChromeQuickSettingsImpl : public SChatChromeQuickSettings
{
public:

	void Construct(const FArguments& InArgs, TSharedPtr<FChatSettingsViewModel> SettingsViewModel, const TSharedPtr<class FChatChromeViewModel> InChromeViewModel) override
	{
		ChatSettingsViewModel = SettingsViewModel;
		ChromeViewModel = InChromeViewModel;
		FriendStyle = *InArgs._FriendStyle;

		SUserWidget::Construct(SUserWidget::FArguments()
		[
 			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor::Black)
 			.Padding(8.0f)
 			.BorderImage(&FriendStyle.FriendsChatStyle.ChatBackgroundBrush)
 			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SChatChromeSettings, ChatSettingsViewModel)
					.FriendStyle(&FriendStyle)
					.bQuickSettings(true)
				]
				+ SVerticalBox::Slot()
				.Padding(FMargin(5))
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					[
						SNew(SButton)
						.OnClicked(this, &SChatChromeQuickSettingsImpl::FullSettings)
						.ButtonStyle(&FriendStyle.FriendsListStyle.FriendGeneralButtonStyle)
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						.Cursor(EMouseCursor::Hand)
						[
							SNew(STextBlock)
							.Font(FriendStyle.FriendsNormalFontStyle.FriendsFontSmallBold)
							.ColorAndOpacity(FriendStyle.FriendsNormalFontStyle.DefaultFontColor)
							.Text(LOCTEXT("ChatSettings", "Chat Settings"))
						]
					]
					+ SHorizontalBox::Slot()
					[
						SNew(SButton)
						.OnClicked(this, &SChatChromeQuickSettingsImpl::Reset)
						.ButtonStyle(&FriendStyle.FriendsListStyle.FriendGeneralButtonStyle)
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						.Cursor(EMouseCursor::Hand)
						[
							SNew(STextBlock)
							.Font(FriendStyle.FriendsNormalFontStyle.FriendsFontSmallBold)
							.ColorAndOpacity(FriendStyle.FriendsNormalFontStyle.DefaultFontColor)	
							.Text(LOCTEXT("Reset", "Reset"))
						]
					]
				]
			]
		]);
	}

private:
	
	FReply Reset()
	{
		return FReply::Handled();
	}

	FReply FullSettings()
	{
		ChromeViewModel->OnOpenSettings().Broadcast();
		return FReply::Handled();
	}

	FFriendsAndChatStyle FriendStyle;
	TSharedPtr<FChatSettingsViewModel> ChatSettingsViewModel;
	TSharedPtr<FChatChromeViewModel> ChromeViewModel;
};

TSharedRef<SChatChromeQuickSettings> SChatChromeQuickSettings::New()
{
	return MakeShareable(new SChatChromeQuickSettingsImpl());
}

#undef LOCTEXT_NAMESPACE
