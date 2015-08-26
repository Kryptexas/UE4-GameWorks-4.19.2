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
		Whisper = 1 << 1,	// Person whisper Item
		Global = 1 << 2,	// Global Chat Item
		Party = 1 << 3,		// Party Chat Item
		Game = 1 << 4,		// Game Chat Item
		Invalid = 1 << 5,		// Invalid or max
	};

	/** @return the FTextified version of the enum passed in */
	inline FText ToText(EChatMessageType::Type Type)
	{
		static FText WhisperText = NSLOCTEXT("FriendsList", "Whisper", "Whisper");
		static FText GlobalText = NSLOCTEXT("FriendsList", "Global", "Global");
		static FText PartyText = NSLOCTEXT("FriendsList", "Party", "Party");

		switch (Type)
		{
		case Whisper: return WhisperText;
		case Global: return GlobalText;
		case Party: return PartyText;
		case Game: return PartyText;

		default: return FText::GetEmpty();
		}
	}

	inline FString ShortcutString(EChatMessageType::Type Type)
	{
		static FString WhisperShortcut = TEXT("/w");
		static FString GlobalShortcut = TEXT("/g");
		static FString PartyShortcut = TEXT("/p");
		static FString GameShortcut = TEXT("/i");

		switch (Type)
		{
		case Whisper: return WhisperShortcut;
		case Global: return GlobalShortcut;
		case Party: return PartyShortcut;
		case Game: return GameShortcut;

		default: return FString();
		}
	}

	inline EChatMessageType::Type EnumFromString(FString ShortcutString)
	{
		static FString EmptyShortcut = TEXT("/");
		static FString WhisperShortcut = TEXT("/w");
		static FString GlobalShortcut = TEXT("/g");
		static FString PartyShortcut = TEXT("/p");
		static FString GameShortcut = TEXT("/i");

		if (ShortcutString == WhisperShortcut)
		{
			return EChatMessageType::Whisper;
		}
		else if (ShortcutString == GlobalShortcut)
		{
			return EChatMessageType::Global;
		}
		else if (ShortcutString == PartyShortcut)
		{
			return EChatMessageType::Party;
		}
		else if (ShortcutString == GameShortcut)
		{
			return EChatMessageType::Game;
		}
		else if( ShortcutString == EmptyShortcut)
		{
			return EChatMessageType::Empty;
		}
		return EChatMessageType::Invalid;
	}
};