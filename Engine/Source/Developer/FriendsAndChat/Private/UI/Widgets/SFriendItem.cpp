// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "SFriendItem.h"
#include "FriendViewModel.h"
#include "SFriendsList.h"

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
		PendingAction = EFriendActionType::MAX_None;

		SUserWidget::Construct(SUserWidget::FArguments()
		[
			SNew(SButton)
			.ButtonStyle(&FriendStyle.FriendListItemButtonSimpleStyle)
			.ContentPadding(9.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.Padding(10, 0)
				.AutoWidth()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Left)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						SNew(SImage)
						.Image(this, &SFriendItemImpl::GetPresenceBrush)

					]
					+ SOverlay::Slot()
					.VAlign(VAlign_Top)
					.HAlign(HAlign_Right)
					[
						SNew(SImage)
						.Image(this, &SFriendItemImpl::GetStatusBrush)
					]
				]
				+ SHorizontalBox::Slot()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					[
						SNew(STextBlock)
						.Font(FriendStyle.FriendsFontStyleBold)
						.ColorAndOpacity(FriendStyle.DefaultFontColor)
						.Text(ViewModel->GetFriendName())
					]
					+ SVerticalBox::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Font(FriendStyle.FriendsFontStyleSmallBold)
						.ColorAndOpacity(FriendStyle.DefaultFontColor)
						.Text(ViewModelPtr, &FFriendViewModel::GetFriendLocation)
					]
				]
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Right)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Bottom)
					.Padding(0, 0, 5, 0)
					[
						SNew(SUniformGridPanel)
						.Visibility(this, &SFriendItemImpl::PendingActionVisibility, EFriendActionType::RemoveFriend)
						+ SUniformGridPanel::Slot(0, 0)
						[
							SNew(SBox)
							.Padding(5)
							[
								SNew(SButton)
								.OnClicked(this, &SFriendItemImpl::HandlePendingActionClicked, true)
								.ButtonStyle(SFriendsList::GetActionButtonStyle(FriendStyle, EFriendActionType::ToActionLevel(EFriendActionType::RemoveFriend)))
								.VAlign(VAlign_Center)
								.HAlign(HAlign_Center)
								[
									SNew(STextBlock)
									.ColorAndOpacity(FriendStyle.DefaultFontColor)
									.Font(FriendStyle.FriendsFontStyleSmallBold)
									.Text(EFriendActionType::ToText(EFriendActionType::RemoveFriend))
								]
							]
						]
						+ SUniformGridPanel::Slot(1, 0)
						[
							SNew(SBox)
							.Padding(5)
							[
								SNew(SButton)
								.OnClicked(this, &SFriendItemImpl::HandlePendingActionClicked, false)
								.ButtonStyle(SFriendsList::GetActionButtonStyle(FriendStyle, EFriendActionType::ToActionLevel(EFriendActionType::CancelFriendRequest)))
								.VAlign(VAlign_Center)
								.HAlign(HAlign_Center)
								[
									SNew(STextBlock)
									.ColorAndOpacity(FriendStyle.DefaultFontColor)
									.Font(FriendStyle.FriendsFontStyleSmallBold)
									.Text(EFriendActionType::ToText(EFriendActionType::CancelFriendRequest))
								]
							]
						]
					]
					+ SOverlay::Slot()
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Bottom)
					.Padding(0, 0, 5, 0)
					[
						SNew(SUniformGridPanel)
						.Visibility(this, &SFriendItemImpl::PendingActionVisibility, EFriendActionType::JoinGame)
						+ SUniformGridPanel::Slot(0, 0)
						[
							SNew(SBox)
							.Padding(5)
							[
								SNew(SButton)
								.OnClicked(this, &SFriendItemImpl::HandlePendingActionClicked, true)
								.ButtonStyle(SFriendsList::GetActionButtonStyle(FriendStyle, EFriendActionType::ToActionLevel(EFriendActionType::JoinGame)))
								.VAlign(VAlign_Center)
								.HAlign(HAlign_Center)
								[
									SNew(STextBlock)
									.ColorAndOpacity(FriendStyle.DefaultFontColor)
									.Font(FriendStyle.FriendsFontStyleSmallBold)
									.Text(EFriendActionType::ToText(EFriendActionType::JoinGame))
								]
							]
						]
						+ SUniformGridPanel::Slot(1, 0)
						[
							SNew(SBox)
							.Padding(5)
							[
								SNew(SButton)
								.OnClicked(this, &SFriendItemImpl::HandlePendingActionClicked, false)
								.ButtonStyle(SFriendsList::GetActionButtonStyle(FriendStyle, EFriendActionType::ToActionLevel(EFriendActionType::CancelFriendRequest)))
								.VAlign(VAlign_Center)
								.HAlign(HAlign_Center)
								[
									SNew(STextBlock)
									.ColorAndOpacity(FriendStyle.DefaultFontColor)
									.Font(FriendStyle.FriendsFontStyleSmallBold)
									.Text(EFriendActionType::ToText(EFriendActionType::CancelFriendRequest))
								]
							]
						]
					]
					+ SOverlay::Slot()
					.VAlign(VAlign_Center)
					.Padding(15, 0)
					[
						SAssignNew(Anchor, SMenuAnchor)
						.Method(InArgs._Method)
						.OnGetMenuContent(this, &SFriendItemImpl::GetMenuContent)
						.Placement(MenuPlacement_BelowAnchor)
						.Visibility(this, &SFriendItemImpl::ActionMenuButtonVisibility)
						.Content()
						[
							SNew(SButton)
							.ButtonStyle(&FriendStyle.FriendActionDropdownButtonStyle)
							.OnClicked(this, &SFriendItemImpl::HandleItemClicked)
						]
					]
				]
			]
		]);
	}

private:

	const FSlateBrush* GetPresenceBrush() const
	{
		if (ViewModel->IsOnline())
		{
			FString ClientId = ViewModel->GetClientId();
			//@todo samz - better way of finding known ids
			if (ClientId == TEXT("300d79839c914445948e3c1100f211db"))
			{
				return &FriendStyle.FortniteImageBrush;
			}
			else if (ClientId == TEXT("f3e80378aed4462498774a7951cd263f"))
			{
				return &FriendStyle.LauncherImageBrush;
			}
		}
		return &FriendStyle.FriendImageBrush;
	}

	const FSlateBrush* GetStatusBrush() const
	{
		switch (ViewModel->GetOnlineStatus())
		{
		case EOnlinePresenceState::Away:
		case EOnlinePresenceState::ExtendedAway:
			return &FriendStyle.AwayBrush;
		case EOnlinePresenceState::Chat:
		case EOnlinePresenceState::DoNotDisturb:
		case EOnlinePresenceState::Online:
			return &FriendStyle.OnlineBrush;
		case EOnlinePresenceState::Offline:
		default:
			return &FriendStyle.OfflineBrush;
		};
	}

	TSharedRef<SWidget> GetMenuContent()
	{
		TSharedPtr<SVerticalBox> ActionListBox;
		TSharedRef<SWidget> Contents =
			SNew(SBorder)
			.BorderImage(&FriendStyle.Background)
			.Padding(FMargin(5, 5, 15, 15))
			[
				SAssignNew(ActionListBox, SVerticalBox)
			];

		TArray<EFriendActionType::Type> Actions;

		ViewModel->EnumerateActions(Actions);

		for (const auto& FriendAction : Actions)
		{
			ActionListBox->AddSlot()
			[
				SNew(SButton)
				.IsEnabled(this, &SFriendItemImpl::IsActionEnabled, FriendAction)
				.OnClicked(this, &SFriendItemImpl::HandleActionClicked, FriendAction)
				.ButtonStyle(&FriendStyle.FriendListItemButtonStyle)
				.ContentPadding(FMargin(1, 1, 15, 1))
				[
					SNew(STextBlock)
					.ColorAndOpacity(FriendStyle.DefaultFontColor)
					.Font(FriendStyle.FriendsFontStyle)
					.Text(EFriendActionType::ToText(FriendAction))
				]
			];
		}

		MenuContent = Contents;
		return Contents;
	}

	EVisibility PendingActionVisibility(EFriendActionType::Type ActionType) const
	{
		return PendingAction == ActionType ? EVisibility::Visible : EVisibility::Collapsed;
	}

	bool IsActionEnabled(const EFriendActionType::Type FriendAction) const
	{
		return ViewModel->CanPerformAction(FriendAction);
	}

	FReply HandleActionClicked(const EFriendActionType::Type FriendAction)
	{
		Anchor->SetIsOpen(false, false);

		if (FriendAction == EFriendActionType::RemoveFriend || FriendAction == EFriendActionType::JoinGame)
		{
			PendingAction = FriendAction;
			FSlateApplication::Get().SetKeyboardFocus(SharedThis(this));
		}
		else
		{
			ViewModel->PerformAction(FriendAction);
		}
		return FReply::Handled();
	}

	FReply HandlePendingActionClicked(bool bConfirm)
	{
		if (bConfirm)
		{
			ViewModel->PerformAction(PendingAction);
		}
		PendingAction = EFriendActionType::MAX_None;
		return FReply::Handled();
	}

	FReply HandleItemClicked()
	{
		Anchor->SetIsOpen(true, false);
		OpenTime = 0.2f;
		return FReply::Handled();
	}

	EVisibility ActionMenuButtonVisibility() const
	{
		return (bIsHovered && PendingAction == EFriendActionType::MAX_None) || Anchor->IsOpen() ? EVisibility::Visible : EVisibility::Hidden;
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
				if (OpenTime < 0 || MenuMethod != EPopupMethod::CreateNewWindow)
				{
					Anchor->SetIsOpen(false, false);
				}
			}
		}
	}

	virtual bool SupportsKeyboardFocus() const override
	{
		return true;
	}

	virtual void OnFocusChanging(const FWeakWidgetPath& PreviousFocusPath, const FWidgetPath& NewWidgetPath) override
	{
		if (!NewWidgetPath.ContainsWidget(SharedThis(this)))
		{
			PendingAction = EFriendActionType::MAX_None;
		}
	}

private:

	TSharedPtr<FFriendViewModel> ViewModel;

	/** Holds the style to use when making the widget. */
	FFriendsAndChatStyle FriendStyle;

	TSharedPtr<SMenuAnchor> Anchor;

	TSharedPtr<SWidget> MenuContent;

	EPopupMethod MenuMethod;

	float OpenTime;

	EFriendActionType::Type PendingAction;
};

TSharedRef<SFriendItem> SFriendItem::New()
{
	return MakeShareable(new SFriendItemImpl());
}

#undef LOCTEXT_NAMESPACE
