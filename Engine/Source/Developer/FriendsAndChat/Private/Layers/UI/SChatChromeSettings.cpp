// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "SChatChromeSettings.h"
#include "IChatTabViewModel.h"
#include "ChatSettingsViewModel.h"

#define LOCTEXT_NAMESPACE "SChatChromeSettingsImpl"

class SChatChromeSettingsImpl : public SChatChromeSettings
{
public:

	void Construct(const FArguments& InArgs, TSharedPtr<class FChatSettingsViewModel> InChatSettingsViewModel) override
	{
		bQuickSettings = InArgs._bQuickSettings;
		FriendStyle = *InArgs._FriendStyle;
		ChatSettingsViewModel = InChatSettingsViewModel;

		SAssignNew(OptionsContainer, SVerticalBox);

		SUserWidget::Construct(SUserWidget::FArguments()
		[
			OptionsContainer.ToSharedRef()
		]);
		GenerateOptions();
	}

private:

	void GenerateOptions()
	{
		OptionsContainer->ClearChildren();

		TArray<EChatSettingsType::Type> Settings;
		ChatSettingsViewModel->EnumerateUserSettings(Settings, bQuickSettings);

		for (EChatSettingsType::Type SettingType : Settings)
		{
			EChatSettingsOptionType::Type OptionType = EChatSettingsOptionType::GetOptionType(SettingType);
			if (OptionType == EChatSettingsOptionType::CheckBox)
			{
				OptionsContainer->AddSlot()
					.AutoHeight()
					.Padding(FMargin(5))
					[
						GetCheckBoxOption(SettingType).ToSharedRef()
					];
			}
			else if (OptionType == EChatSettingsOptionType::RadioBox)
			{
				OptionsContainer->AddSlot()
					.AutoHeight()
					.Padding(FMargin(5))
					[
						GetRadioBoxOption(SettingType).ToSharedRef()
					];
			}
			else if (OptionType == EChatSettingsOptionType::Slider)
			{
				OptionsContainer->AddSlot()
					.AutoHeight()
					.Padding(FMargin(5))
					[

						GetSliderOption(SettingType).ToSharedRef()
					];
			}
		}
	}

	TSharedPtr<SWidget> GetCheckBoxOption(EChatSettingsType::Type SettingType)
	{
		return SNew(SCheckBox)
			.IsChecked(ChatSettingsViewModel.Get(), &FChatSettingsViewModel::GetOptionCheckState, SettingType)
			.OnCheckStateChanged(ChatSettingsViewModel.Get(), &FChatSettingsViewModel::HandleCheckboxStateChanged, SettingType)
			.Content()
			[
				SNew(STextBlock)
				.Font(FriendStyle.FriendsNormalFontStyle.FriendsFontSmallBold)
				.ColorAndOpacity(FriendStyle.FriendsNormalFontStyle.DefaultFontColor)
				.Text(EChatSettingsType::ToText(SettingType))
			];
	}

	TSharedPtr<SWidget> GetRadioBoxOption(EChatSettingsType::Type SettingType)
	{
		TSharedPtr<SVerticalBox> RadioBox = SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Font(FriendStyle.FriendsNormalFontStyle.FriendsFontSmallBold)
				.ColorAndOpacity(FriendStyle.FriendsNormalFontStyle.DefaultFontColor)
				.Text(EChatSettingsType::ToText(SettingType))
			];

		uint8 MaxStates = ChatSettingsViewModel->GetNumberOfStatesForSetting(SettingType);
		for (uint8 RadioIndex = 0; RadioIndex < MaxStates; ++RadioIndex)
		{
			RadioBox->AddSlot()
			[
				SNew(SCheckBox)
				.Style(FCoreStyle::Get(), "RadioButton")
				.IsChecked(ChatSettingsViewModel.Get(), &FChatSettingsViewModel::GetRadioOptionCheckState, SettingType, RadioIndex)
				.OnCheckStateChanged(ChatSettingsViewModel.Get(), &FChatSettingsViewModel::HandleRadioboxStateChanged, SettingType, RadioIndex)
				.Content()
				[
					SNew(STextBlock)
					.Font(FriendStyle.FriendsNormalFontStyle.FriendsFontSmallBold)
					.ColorAndOpacity(FriendStyle.FriendsNormalFontStyle.DefaultFontColor)
					.Text(ChatSettingsViewModel->GetNameOfStateForSetting(SettingType, RadioIndex))
				]
			];
		}
		return RadioBox;
	}

	TSharedPtr<SWidget> GetSliderOption(EChatSettingsType::Type SettingType)
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Font(FriendStyle.FriendsNormalFontStyle.FriendsFontSmallBold)
				.ColorAndOpacity(FriendStyle.FriendsNormalFontStyle.DefaultFontColor)
				.Text(EChatSettingsType::ToText(SettingType))
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SSlider)
				.Value(ChatSettingsViewModel.Get(), &FChatSettingsViewModel::GetSliderValue, SettingType)
				.OnValueChanged(ChatSettingsViewModel.Get(), &FChatSettingsViewModel::HandleSliderValueChanged, SettingType)
			];
	}

	bool bQuickSettings;
	TSharedPtr<SVerticalBox> OptionsContainer;
	FFriendsAndChatStyle FriendStyle;
	TSharedPtr<FChatSettingsViewModel> ChatSettingsViewModel;
};

TSharedRef<SChatChromeSettings> SChatChromeSettings::New()
{
	return MakeShareable(new SChatChromeSettingsImpl());
}

#undef LOCTEXT_NAMESPACE
