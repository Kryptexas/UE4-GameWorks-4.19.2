// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

DECLARE_DELEGATE_RetVal(bool, FIsMenuOpen);
DECLARE_DELEGATE_OneParam(FOnDropdownItemClicked, FName);

class SFriendsAndChatCombo : public SUserWidget
{
public:
	/** Helper class used to define content of one item in SFriendsAndChatCombo. */
	class FItemData
	{

	public:

		/** Ctor */
		FItemData(TAttribute<FText> EntryText, FName ButtonTag, TAttribute<bool> bIsEnabled)
			: EntryText(EntryText)
			, bIsEnabled(bIsEnabled)
			, ButtonTag(ButtonTag)
		{}

		/** Text content */
		TAttribute<FText> EntryText;

		/** Is this item actually enabled/selectable */
		TAttribute<bool> bIsEnabled;

		/** Tag that will be returned by OnDropdownItemClicked delegate when button corresponding to this item is clicked */
		FName ButtonTag;
	};

	/** Helper class allowing to fill array of SItemData with syntax similar to Slate */
	class FItemsArray : public TArray < FItemData >
	{

	public:

		/** Adds item to array and returns itself */
		FItemsArray & operator + (const FItemData& TabData)
		{
			Add(TabData);
			return *this;
		}

		/** Adds item to array and returns itself */
		FItemsArray & AddItem(TAttribute<FText> EntryText, FName ButtonTag, TAttribute<bool> bIsEnabled = true)
		{
			return operator+(FItemData(EntryText, ButtonTag, bIsEnabled));
		}
	};

	SLATE_USER_ARGS(SFriendsAndChatCombo)
		: _bSetButtonTextToSelectedItem(false)
		, _bAutoCloseWhenClicked(true)
		, _ContentWidth(150)
	{}

		/** Text to display on main button. */
		SLATE_TEXT_ATTRIBUTE(ButtonText)

		SLATE_ARGUMENT(const FFriendsAndChatStyle*, FriendStyle)

		/** If true, text displayed on the main button will be set automatically after user selects a dropdown item */
		SLATE_ARGUMENT(bool, bSetButtonTextToSelectedItem)

		/** List of items to display in dropdown list. */
		SLATE_ATTRIBUTE(FItemsArray, DropdownItems)

		/** Should the dropdown list be closed automatically when user clicks an item. */
		SLATE_ARGUMENT(bool, bAutoCloseWhenClicked)

		/** Width of the button content. Needs to be supplied manually, because dropdown must also be scaled manually. */
		SLATE_ARGUMENT(int32, ContentWidth)

		/** Called when user clicks an item from the dropdown. */
		SLATE_EVENT(FOnDropdownItemClicked, OnDropdownItemClicked)

		/** Called when dropdown is opened (main button is clicked). */
		SLATE_EVENT(FOnComboBoxOpened, OnDropdownOpened)

	SLATE_END_ARGS()

	virtual void Construct(const FArguments& InArgs){}
};