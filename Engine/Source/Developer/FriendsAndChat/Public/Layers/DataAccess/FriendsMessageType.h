// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Enum for the chat message type.
 */
namespace EChatMessageType
{
	enum Type : uint8
	{
		Custom = 0,			// Empty
		Empty = 1 << 0,		// Person whisper Item
		Whisper = 1 << 1,		// Person whisper Item
		Game = 1 << 2,		// Game Chat Item
		Global = 1 << 3,		// Global Chat Item
		Team = 1 << 4,		// Team Chat Item
		Clan = 1 << 5,		// Clan Chat Item
		Party = 1 << 6,		// Party Chat Item
		Invalid = 1 << 7,		// Invalid or max
	};

	/** @return the FTextified version of the enum passed in */
	inline FText ToText(EChatMessageType::Type Type)
	{
		static FText CustomText = NSLOCTEXT("FriendsList", "Custom", "Custom");
		static FText WhisperText = NSLOCTEXT("FriendsList", "Whisper", "Whisper");
		static FText GameText = NSLOCTEXT("FriendsList", "Game", "Game");
		static FText GlobalText = NSLOCTEXT("FriendsList", "Global", "Global");
		static FText TeamText = NSLOCTEXT("FriendsList", "Team", "Team");
		static FText ClanText = NSLOCTEXT("FriendsList", "Clan", "Clan");
		static FText PartyText = NSLOCTEXT("FriendsList", "Party", "Party");

		switch (Type)
		{
		case Custom: return CustomText;
		case Whisper: return WhisperText;
		case Game: return GameText;
		case Global: return GlobalText;
		case Team: return TeamText;
		case Clan: return ClanText;
		case Party: return PartyText;

		default: return FText::GetEmpty();
		}
	}

	inline FString ShortcutString(EChatMessageType::Type Type)
	{
		static FString CustomShortcut = TEXT("/c");
		static FString WhisperShortcut = TEXT("/w");
		static FString GameShortcut = TEXT("/i");
		static FString GlobalShortcut = TEXT("/g");
		static FString TeamShortcut = TEXT("/t");
		static FString ClanShortcut = TEXT("/c");
		static FString PartyShortcut = TEXT("/p");

		switch (Type)
		{
		case Custom: return CustomShortcut;
		case Whisper: return WhisperShortcut;
		case Game: return GameShortcut;
		case Global: return GlobalShortcut;
		case Team: return TeamShortcut;
		case Clan: return ClanShortcut;
		case Party: return PartyShortcut;

		default: return FString();
		}
	}

	inline EChatMessageType::Type EnumFromString(FString ShortcutString)
	{
		static FString EmptyShortcut = TEXT("/");
		static FString CustomShortcut = TEXT("/c");
		static FString WhisperShortcut = TEXT("/w");
		static FString GameShortcut = TEXT("/i");
		static FString GlobalShortcut = TEXT("/g");
		static FString TeamShortcut = TEXT("/t");
		static FString ClanShortcut = TEXT("/c");
		static FString PartyShortcut = TEXT("/p");

		if (ShortcutString == CustomShortcut)
		{
			return EChatMessageType::Custom;
		}
		else if (ShortcutString == WhisperShortcut)
		{
			return EChatMessageType::Whisper;
		}
		else if (ShortcutString == GameShortcut)
		{
			return EChatMessageType::Game;
		}
		else if (ShortcutString == GlobalShortcut)
		{
			return EChatMessageType::Global;
		}
		else if (ShortcutString == TeamShortcut)
		{
			return EChatMessageType::Team;
		}
		else if (ShortcutString == ClanShortcut)
		{
			return EChatMessageType::Clan;
		}
		else if (ShortcutString == PartyShortcut)
		{
			return EChatMessageType::Party;
		}
		else if( ShortcutString == EmptyShortcut)
		{
			return EChatMessageType::Empty;
		}
		return EChatMessageType::Invalid;
	}
};