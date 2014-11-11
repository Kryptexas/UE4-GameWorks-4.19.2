// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "SFriendsStatus.h"
#include "FriendsStatusViewModel.h"

#define LOCTEXT_NAMESPACE "SFriendsStatus"

/**
 * Declares the Friends Status display widget
*/
class SFriendsStatusImpl : public SFriendsStatus
{
public:

	void Construct(const FArguments& InArgs, const TSharedRef<FFriendsStatusViewModel>& ViewModel)
	{
		FriendStyle = *InArgs._FriendStyle;
		this->ViewModel = ViewModel;

		FFriendsStatusViewModel* ViewModelPtr = &ViewModel.Get();

		SUserWidget::Construct(SUserWidget::FArguments()
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SAssignNew(ActionMenu, SMenuAnchor)
				.Placement(EMenuPlacement::MenuPlacement_ComboBox)
				.Method(InArgs._Method)
				.MenuContent(GetMenuContent())
				[
					SNew(SButton)
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Center)
					.OnClicked(this, &SFriendsStatusImpl::HandleVersionDropDownClicked)
					.ButtonStyle(&FriendStyle.FriendListActionButtonStyle)
					.Cursor(EMouseCursor::Hand)
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.HAlign(HAlign_Left)
						.VAlign(VAlign_Center)
						.AutoWidth()
						[
							SNew(SImage)
							.Image(this, &SFriendsStatusImpl::GetStatusBrush)
						]
						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.AutoWidth()
						[
							SNew(STextBlock)
							.Text(ViewModelPtr, &FFriendsStatusViewModel::GetStatusText)
							.Font(FriendStyle.FriendsFontStyleSmall)
							.ColorAndOpacity(FLinearColor::White)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(FMargin(5, 0))
						[
							SNew(SImage)
							.ColorAndOpacity(FLinearColor::White)
							.Image(&FriendStyle.FriendsComboDropdownImageBrush)
						]
					]
				]
			]
		]);
	}

private:
	FReply HandleVersionDropDownClicked() const
	{
		ActionMenu->SetIsOpen(true);
		return FReply::Handled();
	}

	/**
	* Generate the action menu.
	* @return the action menu widget
	*/
	TSharedRef<SWidget> GetMenuContent()
	{
		return SNew(SBorder)
			.Padding(FMargin(1, 5))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				[
					SNew(SButton)
					.OnClicked(this, &SFriendsStatusImpl::HandleStatusChanged, true)
					.ButtonStyle(&FriendStyle.FriendListItemButtonStyle)
					[
						SNew(STextBlock)
						.ColorAndOpacity(FLinearColor::White)
						.Font(FriendStyle.FriendsFontStyle)
						.Text(FText::FromString("Online"))
					]
				]
				+ SVerticalBox::Slot()
				[
					SNew(SButton)
					.OnClicked(this, &SFriendsStatusImpl::HandleStatusChanged, false)
					.ButtonStyle(&FriendStyle.FriendListItemButtonStyle)
					[
						SNew(STextBlock)
						.ColorAndOpacity(FLinearColor::White)
						.Font(FriendStyle.FriendsFontStyle)
						.Text(FText::FromString("Away"))
					]
				]
				+ SVerticalBox::Slot()
				[
					SNew(SButton)
					.OnClicked(this, &SFriendsStatusImpl::HandleStatusChanged, false)
					.ButtonStyle(&FriendStyle.FriendListItemButtonStyle)
					[
						SNew(STextBlock)
						.ColorAndOpacity(FLinearColor::White)
						.Font(FriendStyle.FriendsFontStyle)
						.Text(FText::FromString("Offline"))
					 ]
				 ]
			];
	}

	FReply HandleStatusChanged(bool bOnline)
	{
		ActionMenu->SetIsOpen(false);
		ViewModel->SetOnlineStatus(bOnline);
		return FReply::Handled();
	}


	const FSlateBrush* GetStatusBrush() const
	{
		return ViewModel->GetOnlineStatus() == true ? &FriendStyle.OnlineBrush : &FriendStyle.OfflineBrush;
	}

private:
	/** Holds the style to use when making the widget. */
	FFriendsAndChatStyle FriendStyle;
	TSharedPtr<SMenuAnchor> ActionMenu;
	TSharedPtr<FFriendsStatusViewModel> ViewModel;
};

TSharedRef<SFriendsStatus> SFriendsStatus::New()
{
	return MakeShareable(new SFriendsStatusImpl());
}

#undef LOCTEXT_NAMESPACE
