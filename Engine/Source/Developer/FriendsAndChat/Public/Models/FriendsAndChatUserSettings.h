// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

namespace EChatSettingsType
{
	enum Type : uint8
	{
		ShowNotifications, // Show Notification
		NeverFadeMessages,
		ShowTimeStamps,
		ShowWhispersInAllTabs,
		ShowCustomTab,
		ShowWhispersTab,
		ShowCombatAndEventsTab,
		ShowClanTabs,
		FontSize,
		WindowHeight,
		WindowOpacity,
		/** Invalid enum type, may be used as a number of enumerations. */
		InvalidOrMax
	};

	inline FText ToText(EChatSettingsType::Type State)
	{
		switch (State)
		{
			case ShowNotifications: return NSLOCTEXT("ChatSettingsType","ShowNotifications", "Show notifications");
			case NeverFadeMessages: return NSLOCTEXT("ChatSettingsType", "NeverFadeMessages", "Never fade messages away");
			case ShowTimeStamps: return NSLOCTEXT("ChatSettingsType", "ShowTimeStamps", "Show time stamps");
			case ShowWhispersInAllTabs: return NSLOCTEXT("ChatSettingsType", "ShowWhispersInAllTabs", "Show whispers in all tabs");
			case ShowCustomTab: return NSLOCTEXT("ChatSettingsType", "ShowCustomTab", "Show custom tab");
			case ShowWhispersTab: return NSLOCTEXT("ChatSettingsType", "ShowWhispersTab", "Show whisper tab");
			case ShowCombatAndEventsTab: return NSLOCTEXT("ChatSettingsType", "ShowCombatAndEventsTab", "Show combat and events tab");
			case ShowClanTabs: return NSLOCTEXT("ChatSettingsType", "ShowClanTabs", "Show clan tab");
			case FontSize: return NSLOCTEXT("ChatSettingsType", "FontSize", "Font size");
			case WindowHeight: return NSLOCTEXT("ChatSettingsType", "WindowHeight", "Height");
			case WindowOpacity: return NSLOCTEXT("ChatSettingsType", "WindowOpacity", "Opacity");
			default: return FText::GetEmpty();
		}
	}
};

namespace EChatSettingsOptionType
{
	enum Type : uint8
	{
		CheckBox,
		RadioBox,
		Slider
	};

	inline EChatSettingsOptionType::Type GetOptionType(EChatSettingsType::Type State)
	{
		switch (State)
		{
		case EChatSettingsType::ShowNotifications:
		case EChatSettingsType::NeverFadeMessages:
		case EChatSettingsType::ShowTimeStamps:
		case EChatSettingsType::ShowWhispersInAllTabs:
		case EChatSettingsType::ShowCustomTab:
		case EChatSettingsType::ShowWhispersTab:
		case EChatSettingsType::ShowCombatAndEventsTab:
		case EChatSettingsType::ShowClanTabs:
			return EChatSettingsOptionType::CheckBox;
		case EChatSettingsType::FontSize:
		case EChatSettingsType::WindowHeight:
			return EChatSettingsOptionType::RadioBox;
		case EChatSettingsType::WindowOpacity:
			return EChatSettingsOptionType::Slider;
		default:
			return EChatSettingsOptionType::CheckBox;
		}
	}
};

namespace EChatFontType
{
	enum Type : uint8
	{
		Standard,
		Large,
		/** Invalid enum type, may be used as a number of enumerations. */
		InvalidOrMax
	};

	inline FText ToText(EChatFontType::Type State)
	{
		switch (State)
		{
		case Standard: return NSLOCTEXT("ChatSettingsType", "Standard", "Standard");
		case Large: return NSLOCTEXT("ChatSettingsType", "Large", "Large");
		default: return FText::GetEmpty();
		}
	}
};

namespace EChatWindowHeight
{
	enum Type : uint8
	{
		Standard,
		Tall,
		/** Invalid enum type, may be used as a number of enumerations. */
		InvalidOrMax
	};

	inline FText ToText(EChatWindowHeight::Type State)
	{
		switch (State)
		{
		case Standard: return NSLOCTEXT("ChatSettingsType", "Standard", "Standard");
		case Tall: return NSLOCTEXT("ChatSettingsType", "Tall", "Tall");
		default: return FText::GetEmpty();
		}
	}
};

struct FFriendsAndChatSettings
{
	bool bShowNotifications;
	bool bNeverFadeMessages;
	bool bShowTimeStamps;
	bool bShowWhispersInAllTabs;
	bool bShowCustomTab;
	bool bShowWhispersTab;
	bool bShowCombatAndEventsTab;
	bool bShowClanTabs;
	EChatFontType::Type	FontSize;
	EChatWindowHeight::Type WindowHeight;
	float WindowOpacity;
};
