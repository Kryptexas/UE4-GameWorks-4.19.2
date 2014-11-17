// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


#define LOCTEXT_NAMESPACE "SProjectLauncherProfileNameDescEditor"

/**
 * Implements a build configuration selector widget.
 */
class SProjectLauncherProfileNameDescEditor
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SProjectLauncherProfileNameDescEditor) { }
		SLATE_ATTRIBUTE(ILauncherProfilePtr, LaunchProfile)
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 * @param InModel The data model.
	 */
	void Construct(const FArguments& InArgs, bool InShowAddDescriptionText)
	{
		EnterTextDescription = FText(LOCTEXT("LaunchProfileEnterDescription", "Enter a description here."));

		LaunchProfileAttr = InArgs._LaunchProfile;
		bShowAddDescriptionText = InShowAddDescriptionText;

		ChildSlot
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(40)
				.HeightOverride(40)
				[
					SNew(SImage)
					.Image(this, &SProjectLauncherProfileNameDescEditor::HandleProfileImage)
				]
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1)
			.VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(2, 4, 2, 4)
				[
					SAssignNew(NameEditableTextBlock, SInlineEditableTextBlock)
					.Text(this, &SProjectLauncherProfileNameDescEditor::OnGetNameText)
					.OnTextCommitted(this, &SProjectLauncherProfileNameDescEditor::OnNameTextCommitted)
					.Cursor(EMouseCursor::TextEditBeam)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(2, 4, 2, 4)
				[
					SNew(SInlineEditableTextBlock)
					.Text(this, &SProjectLauncherProfileNameDescEditor::OnGetDescriptionText)
					.Style(FCoreStyle::Get(), "InlineEditableTextBlockSmallStyle")
					.OnTextCommitted(this, &SProjectLauncherProfileNameDescEditor::OnDescriptionTextCommitted)
					.Cursor(EMouseCursor::TextEditBeam)
				]
			]
		];
	}

	/**
	 * Triggers a name edit for the profile.
	 */
	void TriggerNameEdit()
	{
		if (NameEditableTextBlock.IsValid())
		{
			NameEditableTextBlock->EnterEditingMode();
		}
	}

private:

	// Callback for getting the icon image of the profile.
	const FSlateBrush* HandleProfileImage() const
	{
		//const PlatformInfo::FPlatformInfo* const PlatformInfo = PlatformInfo::FindPlatformInfo(*DeviceProxy->GetTargetPlatformName(SimpleProfile->GetDeviceVariant()));
		//return (PlatformInfo) ? FEditorStyle::GetBrush(PlatformInfo->GetIconStyleName(PlatformInfo::EPlatformIconSize::Large)) : FStyleDefaults::GetNoBrush();
		return FEditorStyle::GetBrush("LauncherCommand.QuickLaunch");
	}

	// Callback to get the name of the launch profile.
	FText OnGetNameText() const
	{
		ILauncherProfilePtr LaunchProfile = LaunchProfileAttr.Get();
		if (LaunchProfile.IsValid())
		{
			return FText::FromString(LaunchProfile->GetName());
		}
		return FText();
	}

	// Callback to set the name of the launch profile.
	void OnNameTextCommitted(const FText& NewText, ETextCommit::Type InTextCommit)
	{
		ILauncherProfilePtr LaunchProfile = LaunchProfileAttr.Get();
		if (LaunchProfile.IsValid())
		{
			LaunchProfile->SetName(NewText.ToString());
		}
	}

	// Callback to get the description of the launch profile.
	FText OnGetDescriptionText() const
	{
		ILauncherProfilePtr LaunchProfile = LaunchProfileAttr.Get();
		if (LaunchProfile.IsValid())
		{
			FString Desc = LaunchProfile->GetDescription();
			if (!bShowAddDescriptionText || !Desc.IsEmpty())
			{
				return FText::FromString(Desc);
			}
		}
		return EnterTextDescription;
	}

	// Callback to set the description of the launch profile.
	void OnDescriptionTextCommitted(const FText& NewText, ETextCommit::Type InTextCommit)
	{
		ILauncherProfilePtr LaunchProfile = LaunchProfileAttr.Get();
		if (LaunchProfile.IsValid())
		{
			if (NewText.EqualTo(EnterTextDescription))
			{
				LaunchProfile->SetDescription("");
			}
			else
			{
				LaunchProfile->SetDescription(NewText.ToString());
			}
		}
	}

private:

	// Attribute for the launch profile this widget edits. 
	TAttribute<ILauncherProfilePtr> LaunchProfileAttr;

	// Cache the no description enter suggestion text
	FText EnterTextDescription;

	// Whether we show an add description blurb when the profile has no description. 
	bool bShowAddDescriptionText;

	TSharedPtr<SInlineEditableTextBlock> NameEditableTextBlock;
};


#undef LOCTEXT_NAMESPACE
