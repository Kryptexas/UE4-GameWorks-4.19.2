// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "SChatChromeFullSettings.h"
#include "SChatChromeSettings.h"
#include "FriendsAndChatUserSettings.h"
#include "ChatSettingsViewModel.h"

#define LOCTEXT_NAMESPACE "SChatChromeFullSettingsImple"

class SChatChromeFullSettingsImpl : public SChatChromeFullSettings
{
public:

	void Construct(const FArguments& InArgs, TSharedPtr<class FChatSettingsViewModel> InSettingsViewModel) override
	{
		ChatSettingsViewModel = InSettingsViewModel;
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
					SNew(STextBlock)
					.Font(FriendStyle.FriendsNormalFontStyle.FriendsFontNormalBold)
					.ColorAndOpacity(FriendStyle.FriendsNormalFontStyle.DefaultFontColor)
					.Text(LOCTEXT("ChatSettings", "Chat Settings"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SChatChromeSettings, ChatSettingsViewModel)
					.FriendStyle(&FriendStyle)
					.bQuickSettings(false)
				]
				+ SVerticalBox::Slot()
					.Padding(FMargin(5))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						[
							SNew(SButton)
							.OnClicked(this, &SChatChromeFullSettingsImpl::Save)
							.ButtonStyle(&FriendStyle.FriendsListStyle.FriendGeneralButtonStyle)
							.VAlign(VAlign_Center)
							.HAlign(HAlign_Center)
							.Cursor(EMouseCursor::Hand)
							[
								SNew(STextBlock)
								.Font(FriendStyle.FriendsNormalFontStyle.FriendsFontSmallBold)
								.ColorAndOpacity(FriendStyle.FriendsNormalFontStyle.DefaultFontColor)
								.Text(LOCTEXT("Save", "Save"))
							]
						]
						+ SHorizontalBox::Slot()
						[
							SNew(SButton)
							.OnClicked(this, &SChatChromeFullSettingsImpl::Cancel)
							.ButtonStyle(&FriendStyle.FriendsListStyle.FriendGeneralButtonStyle)
							.VAlign(VAlign_Center)
							.HAlign(HAlign_Center)
							.Cursor(EMouseCursor::Hand)
							[
								SNew(STextBlock)
								.Font(FriendStyle.FriendsNormalFontStyle.FriendsFontSmallBold)
								.ColorAndOpacity(FriendStyle.FriendsNormalFontStyle.DefaultFontColor)
								.Text(LOCTEXT("Cancel", "Cancel"))
							]
						]
						+ SHorizontalBox::Slot()
							[
								SNew(SButton)
								.OnClicked(this, &SChatChromeFullSettingsImpl::Reset)
								.ButtonStyle(&FriendStyle.FriendsListStyle.FriendGeneralButtonStyle)
								.VAlign(VAlign_Center)
								.HAlign(HAlign_Center)
								.Cursor(EMouseCursor::Hand)
								[
									SNew(STextBlock)
									.Font(FriendStyle.FriendsNormalFontStyle.FriendsFontSmallBold)
									.ColorAndOpacity(FriendStyle.FriendsNormalFontStyle.DefaultFontColor)
									.Text(LOCTEXT("Defaults", "Defaults"))
								]
							]
					]
			]
		]);
	}

private:

	FReply Save()
	{
		return FReply::Handled();
	}

	FReply Cancel()
	{
		return FReply::Handled();
	}

	FReply Reset()
	{
		return FReply::Handled();
	}

	FFriendsAndChatStyle FriendStyle;
	TSharedPtr<FChatSettingsViewModel> ChatSettingsViewModel;
};

TSharedRef<SChatChromeFullSettings> SChatChromeFullSettings::New()
{
	return MakeShareable(new SChatChromeFullSettingsImpl());
}

#undef LOCTEXT_NAMESPACE
