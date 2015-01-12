// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "SFriendsAndChatCombo.h"

#define LOCTEXT_NAMESPACE "SFriendsAndChatCombo"

// Style values that shouldn't change
#define DROPDOWN_MAX_HEIGHT 36

class SFriendsAndChatComboButton : public SComboButton
{

private:

	/** Helper class to override standard button's behavior, to show it in "pressed" style depending on dropdown state. */
	class SCustomDropdownButton : public SButton
	{
	public:

		/** Show as pressed if dropdown is opened. */
		virtual bool IsPressed() const override
		{
			return IsMenuOpenDelegate.IsBound() && IsMenuOpenDelegate.Execute();
		}

		/** Delegate set by parent SFriendsAndChatCombo, checks open state. */
		FIsMenuOpen IsMenuOpenDelegate;
	};

public:

	SLATE_USER_ARGS(SFriendsAndChatComboButton)
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
		SLATE_ATTRIBUTE(SFriendsAndChatCombo::FItemsArray, DropdownItems)

		/** Should the dropdown list be closed automatically when user clicks an item. */
		SLATE_ARGUMENT(bool, bAutoCloseWhenClicked)

		/** Width of the button content. Needs to be supplied manually, because dropdown must also be scaled manually. */
		SLATE_ARGUMENT(int32, ContentWidth)

		/** Called when user clicks an item from the dropdown. */
		SLATE_EVENT(FOnDropdownItemClicked, OnDropdownItemClicked)

		/** Called when dropdown is opened (main button is clicked). */
		SLATE_EVENT(FOnComboBoxOpened, OnDropdownOpened)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		FriendStyle = *InArgs._FriendStyle;
		bAutoCloseWhenClicked = InArgs._bAutoCloseWhenClicked;
		ContentWidth = InArgs._ContentWidth;
		DropdownItems = InArgs._DropdownItems;
		OnItemClickedDelegate = InArgs._OnDropdownItemClicked;
		ButtonText = InArgs._ButtonText;
		bSetButtonTextToSelectedItem = InArgs._bSetButtonTextToSelectedItem;

		const FComboButtonStyle& ComboButtonStyle = FriendStyle.FriendListComboButtonStyle;

		MenuBorderBrush = &ComboButtonStyle.MenuBorderBrush;
		MenuBorderPadding = ComboButtonStyle.MenuBorderPadding;

		OnComboBoxOpened = InArgs._OnDropdownOpened;
		OnGetMenuContent = FOnGetContent::CreateSP(this, &SFriendsAndChatComboButton::GetMenuContent);

		// Purposefully not calling SComboButton's constructor as we want to override more of the visuals

		SMenuAnchor::Construct(SMenuAnchor::FArguments()
			.Placement(MenuPlacement_ComboBox)
			.Method(EPopupMethod::UseCurrentWindow)
			[
				SAssignNew(DropdownButton, SCustomDropdownButton)
				.ButtonStyle(&ComboButtonStyle.ButtonStyle)
				.ClickMethod(EButtonClickMethod::MouseDown)
				.OnClicked(this, &SFriendsAndChatComboButton::OnButtonClicked)
				.ContentPadding(FMargin(15, 0, 44, 0))
				.ForegroundColor(FLinearColor::White)
				[
					SNew(SBox)
					.WidthOverride(ContentWidth)
					.HeightOverride(DROPDOWN_MAX_HEIGHT)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(this, &SFriendsAndChatComboButton::GetButtonText)
						.Font(FriendStyle.FriendsFontStyleBold)
						.ShadowOffset(FVector2D(0, 1))
						.ShadowColorAndOpacity(FColor(0, 84, 80))
					]
				]
			]
		);

		if (DropdownButton.IsValid())
		{
			DropdownButton->IsMenuOpenDelegate = FIsMenuOpen::CreateSP(this, &SMenuAnchor::IsOpen);
		}
	}

protected:

	/** Overridden to pass button click to SComboBox */
	virtual FReply OnButtonClicked() override
	{
		return SComboButton::OnButtonClicked();
	}

	/** Returns pointer to dropdown button */
	TSharedPtr<SButton> GetDropdownButton()
	{
		return DropdownButton;
	}

private:

	/** Unlike generic ComboBox, SFriendsAndChatCombo has well defined content, created right here. */
	TSharedRef<SWidget> GetMenuContent()
	{
		TSharedPtr<SVerticalBox> EntriesWidget;
		const FSlateBrush* BackgroundLeftImage = &FriendStyle.FriendComboBackgroundLeftBrush;
		const FSlateBrush* BackgroundRightImage = &FriendStyle.FriendComboBackgroundRightBrush;

		SAssignNew(EntriesWidget, SVerticalBox);

		const SFriendsAndChatCombo::FItemsArray& DropdownItemsRef = DropdownItems.Get();

		DropdownItemButtons.Empty();
		DropdownItemButtons.AddZeroed(DropdownItemsRef.Num());

		for (int32 Idx = 0; Idx < DropdownItemsRef.Num(); ++Idx)
		{
			EntriesWidget->AddSlot()
			.AutoHeight()
			.Padding(FMargin(0, 0, 0, 1))
			[
				SAssignNew(DropdownItemButtons[Idx], SButton)
				.ButtonStyle(&FriendStyle.FriendListItemButtonStyle)
				.ContentPadding(FMargin(8, 2, 8, 2))
				.IsEnabled(DropdownItemsRef[Idx].bIsEnabled)
				.OnClicked(this, &SFriendsAndChatComboButton::OnDropdownButtonClicked, Idx)
				[
					SNew(STextBlock)
					.Text(DropdownItemsRef[Idx].EntryText)
					.Font(FriendStyle.FriendsFontStyleSmallBold)
					.ColorAndOpacity(this, &SFriendsAndChatComboButton::GetTextColor, Idx)
					.ShadowOffset(FVector2D(0, 1))
					.ShadowColorAndOpacity(FColor(35, 14, 12))
				]
			];
		}

		return
		SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SBox)
				//.Padding(FMargin(1,0,0,0))
				.WidthOverride(ContentWidth + 30)
				[
					SNew(SImage)
					.Image(BackgroundLeftImage)
				]
			]
			+ SHorizontalBox::Slot()
			[
				SNew(SImage)
				.Image(BackgroundRightImage)
			]
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SBox)
			.Padding(FMargin(8, 8, 8, 8))
			[
				EntriesWidget.ToSharedRef()
			]
		];
	}

	/** Returns color of text on dropdown button Idx. */
	FSlateColor GetTextColor(int32 Idx) const
	{
		if (DropdownItemButtons.IsValidIndex(Idx) && DropdownItemButtons[Idx].IsValid() && DropdownItemButtons[Idx]->IsHovered())
		{
			return FSlateColor(FColor::White);
		}
		return FSlateColor(FColor(255, 246, 235));
	}

	/** Called when user clicks item from the dropdown. Calls OnItemClickedDelegate and potentially closes the menu. */
	FReply OnDropdownButtonClicked(int32 Idx)
	{
		if (bSetButtonTextToSelectedItem && DropdownItems.IsSet() && DropdownItems.Get().IsValidIndex(Idx))
		{
			ButtonText = DropdownItems.Get()[Idx].EntryText;
		}
		if (OnItemClickedDelegate.IsBound() && DropdownItems.IsSet() && DropdownItems.Get().IsValidIndex(Idx))
		{
			OnItemClickedDelegate.Execute(DropdownItems.Get()[Idx].ButtonTag);
		}
		if (bAutoCloseWhenClicked)
		{
			SetIsOpen(false);
		}

		return FReply::Handled();
	}

	/** Returns text to display on the main button */
	FText GetButtonText() const
	{
		return ButtonText.Get();
	}

	/** Holds the style to use when making the widget. */
	FFriendsAndChatStyle FriendStyle;

	/** String value displayed on the main button */
	TAttribute<FText> ButtonText;

	/** Delegate to call when user clicks item from the dropdown list. */
	FOnDropdownItemClicked OnItemClickedDelegate;

	/** Cached list of items to generate menu content with. */
	TAttribute<SFriendsAndChatCombo::FItemsArray> DropdownItems;

	/** Cached actual dropdown button */
	TSharedPtr<SCustomDropdownButton> DropdownButton;

	/** Cached list of buttons corresponding to items from DropdownItems. */
	TArray< TSharedPtr<SButton> > DropdownItemButtons;

	/** If true, text displayed on the main button will be set automatically after user selects a dropdown item */
	bool bSetButtonTextToSelectedItem;

	/** Should the dropdown list be closed automatically when user clicks an item. */
	bool bAutoCloseWhenClicked;

	/** Width of the button content. */
	int32 ContentWidth;
};

class SFriendsAndChatComboImpl : public SFriendsAndChatCombo
{
public:
	void Construct(const FArguments& InArgs)
	{
		SUserWidget::Construct(SUserWidget::FArguments()
			[
				SNew(SFriendsAndChatComboButton)
				.ButtonText(InArgs._ButtonText)
				.FriendStyle(InArgs._FriendStyle)
				.bSetButtonTextToSelectedItem(InArgs._bSetButtonTextToSelectedItem)
				.DropdownItems(InArgs._DropdownItems)
				.bAutoCloseWhenClicked(InArgs._bAutoCloseWhenClicked)
				.ContentWidth(InArgs._ContentWidth)
				.OnDropdownItemClicked(InArgs._OnDropdownItemClicked)
				.OnDropdownOpened(InArgs._OnDropdownOpened)
			]);
	}
};

TSharedRef<SFriendsAndChatCombo> SFriendsAndChatCombo::New()
{
	return MakeShareable(new SFriendsAndChatComboImpl());
}

#undef LOCTEXT_NAMESPACE
