// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SChatEntryWidget.h: Declares SChatEntryWidget class.
	This widget is used for the chat entry box. We can use it to determine chat markup etc
=============================================================================*/

#pragma once

class SChatEntryWidget : public SUserWidget
{
public:
	SLATE_USER_ARGS( SChatEntryWidget )
		: _MaxChatLength(128)
		{}

		SLATE_ARGUMENT(const FFriendsAndChatStyle*, FriendStyle)	

		/** The marshaller used to get/set the raw text to/from the text layout. */
		SLATE_ARGUMENT(TSharedPtr< ITextLayoutMarshaller >, Marshaller)

		// The hint that shows what key activates chat
		SLATE_ATTRIBUTE(FText, HintText)

		// The max length a chat message can be (Move to somewhere configurable)
		SLATE_ARGUMENT(int32, MaxChatLength)

	SLATE_END_ARGS()


	/**
	 * Construct the chat entry widget.
	 * @param InArgs		Widget args
	 * @param InViewModel	The chat view model - used for accessing chat markup etc.
	 */
	virtual void Construct(const FArguments& InArgs, const TSharedRef<class FChatViewModel>& InViewModel) = 0;

	/**
	 * Rebuild the Text Entry Widget
	 * @param bLargeFont - True if using large font, else standard font
	 */
	virtual void RebuildTextEntry() = 0;
};
