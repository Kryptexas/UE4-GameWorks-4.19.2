// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "SClanListContainer.h"
#include "SClanList.h"
#include "ClanListViewModel.h"

#define LOCTEXT_NAMESPACE "SFriendsList"

/**
 * Declares the Clan List display widget
*/
class SClanListContainerImpl : public SClanListContainer
{
public:

	void Construct(const FArguments& InArgs, const TSharedRef<FClanListViewModel>& InViewModel)
	{
		FriendStyle = *InArgs._FriendStyle;
		FriendsVisibility = EVisibility::Visible;
		ViewModel = InViewModel;

		FClanListViewModel* ViewModelPtr = ViewModel.Get();
		MenuMethod = InArgs._Method;

		SUserWidget::Construct(SUserWidget::FArguments()
		[
			SNew(SVerticalBox)
			.Visibility(ViewModelPtr, &FClanListViewModel::GetListVisibility)
			+ SVerticalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoHeight()
			[
				SNew(SBorder)
				.Padding(FriendStyle.BorderPadding)
				.BorderImage(&FriendStyle.FriendListHeader)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox)
				.Padding(FMargin(0.0f, 2.0f, 0.0f, 8.0f))
				[
					CreateList()
				]
			]
		]);
	}

private:

	TSharedRef<SWidget> CreateList()
	{
		return SNew(SClanList, ViewModel.ToSharedRef())
			.FriendStyle(&FriendStyle)
			.Method(MenuMethod);
	}

private:
	/** Holds the style to use when making the widget. */
	FFriendsAndChatStyle FriendStyle;

	EVisibility  FriendsVisibility;

	TSharedPtr<FClanListViewModel> ViewModel;

	EPopupMethod MenuMethod;
};

TSharedRef<SClanListContainer> SClanListContainer::New()
{
	return MakeShareable(new SClanListContainerImpl());
}

#undef LOCTEXT_NAMESPACE
