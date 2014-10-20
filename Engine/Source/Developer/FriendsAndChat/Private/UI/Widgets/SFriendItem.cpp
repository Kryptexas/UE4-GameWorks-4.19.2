// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "SFriendItem.h"
#include "FriendViewModel.h"

#define LOCTEXT_NAMESPACE "SFriendItem"

class SFriendItemImpl : public SFriendItem
{
public:

	void Construct(const FArguments& InArgs, const TSharedRef<FFriendViewModel>& InViewModel)
	{
		FriendStyle = *InArgs._FriendStyle;
		this->ViewModel = InViewModel;
		FFriendViewModel* ViewModelPtr = ViewModel.Get();
		MenuMethod = InArgs._Method;

		SUserWidget::Construct(SUserWidget::FArguments()
		[
			SAssignNew(Anchor, SMenuAnchor)
			.Method(InArgs._Method)
			.OnGetMenuContent(this, &SFriendItemImpl::GetMenuContent)
			.Placement(MenuMethod == SMenuAnchor::UseCurrentWindow ? MenuPlacement_MenuLeft : MenuPlacement_MenuRight)
			.Content()
			[
				SNew( SHorizontalBox )
				+SHorizontalBox::Slot()
				.Padding( 10, 0 )
				.AutoWidth()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Left)
				[
					SNew(SImage)
					.Image(&FriendStyle.FriendImageBrush)
				]
				+SHorizontalBox::Slot()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					[
						SNew(STextBlock)
						.Font(FriendStyle.FriendsFontStyle)
						.Text(ViewModel->GetFriendName())
					]
					+ SVerticalBox::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SImage)
							.Visibility(this, &SFriendItemImpl::GetStatusVisibility, true)
							.Image(&FriendStyle.OnlineBrush)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SImage)
							.Visibility(this, &SFriendItemImpl::GetStatusVisibility, false)
							.Image(&FriendStyle.OfflineBrush)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(STextBlock)
							.Font(FriendStyle.FriendsFontStyleSmall)
							.Text(ViewModelPtr, &FFriendViewModel::GetFriendLocation)
						]
					]
				]
			]
		]);
	}

private:
	TSharedRef<SWidget> GetMenuContent()
	{
		TSharedPtr<SVerticalBox> ActionListBox;
		TSharedRef<SWidget> Contents =
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor::White)
			.Padding(10)
			[
				SAssignNew(ActionListBox, SVerticalBox)
			];

		TArray<EFriendActionType::Type> Actions;

		ViewModel->EnumerateActions(Actions);

		for(const auto& FriendAction : Actions)
		{
			ActionListBox->AddSlot()
			[
				SNew(SButton)
				.OnClicked(this, &SFriendItemImpl::HandleActionClicked, FriendAction)
				.ButtonStyle(&FriendStyle.FriendListActionButtonStyle)
				[
					SNew(STextBlock)
					.ColorAndOpacity(FLinearColor::White)
					.Font(FriendStyle.FriendsFontStyle)
					.Text(EFriendActionType::ToText(FriendAction))
				]
			];
		}

		MenuContent = Contents;
		return Contents;
	}

	FReply HandleActionClicked(const EFriendActionType::Type FriendAction) const
	{
		Anchor->SetIsOpen(false);
		ViewModel->PerformAction(FriendAction);
		return FReply::Handled();
	}

	EVisibility GetStatusVisibility(bool bOnlineCheck) const
	{
		return (ViewModel.IsValid() && ViewModel->IsOnline()) == bOnlineCheck ? EVisibility::Visible : EVisibility::Collapsed;
	}

	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		Anchor->SetIsOpen(true);
		OpenTime = 0.2f;
	}

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override
	{
		if (Anchor.IsValid() && Anchor->IsOpen())
		{
			if (IsHovered() || Anchor->IsHovered() || (MenuContent.IsValid() && MenuContent->IsHovered()))
			{
				OpenTime = 0.2f;
			}
			else
			{
				OpenTime -= InDeltaTime;
				if (OpenTime < 0 || MenuMethod != SMenuAnchor::CreateNewWindow)
				{
					Anchor->SetIsOpen(false);
				}
			}
		}
	}

private:

	TSharedPtr<FFriendViewModel> ViewModel;

	/** Holds the style to use when making the widget. */
	FFriendsAndChatStyle FriendStyle;

	TSharedPtr<SMenuAnchor> Anchor;

	TSharedPtr<SWidget> MenuContent;

	SMenuAnchor::EMethod MenuMethod;

	float OpenTime;
};

TSharedRef<SFriendItem> SFriendItem::New()
{
	return MakeShareable(new SFriendItemImpl());
}

#undef LOCTEXT_NAMESPACE
