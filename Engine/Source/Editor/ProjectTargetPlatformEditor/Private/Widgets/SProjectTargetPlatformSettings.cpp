// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ProjectTargetPlatformEditorPrivatePCH.h"

#define LOCTEXT_NAMESPACE "SProjectTargetPlatformSettings"

/**
 * Implements a row widget for the target platform list view
 */
class SProjectTargetPlatformsListRow : public SMultiColumnTableRow<TSharedPtr<FProjectTargetPlatform>>
{
public:

	SLATE_BEGIN_ARGS(SProjectTargetPlatformsListRow)
		: _TargetPlatform()
	{}

	SLATE_ARGUMENT(TSharedPtr<FProjectTargetPlatform>, TargetPlatform)

	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The arguments.
	 */
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
	{
		check(InArgs._TargetPlatform.IsValid());
		TargetPlatform = InArgs._TargetPlatform;

		SMultiColumnTableRow<TSharedPtr<FProjectTargetPlatform>>::Construct(FSuperRowType::FArguments(), InOwnerTableView);
	}

public:

	/**
	 * Generates the widget for the specified column.
	 *
	 * @param ColumnName - The name of the column to generate the widget for.
	 *
	 * @return The widget.
	 */
	BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) OVERRIDE
	{
		if(ColumnName == "Enabled")
		{
			return SNew(SBox)
			.Padding(2.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SCheckBox)
				.IsChecked(this, &SProjectTargetPlatformsListRow::HandleEnabledCheckBoxIsChecked)
				.OnCheckStateChanged(this, &SProjectTargetPlatformsListRow::HandleEnabledCheckBoxStateChanged)
			];
		}
		else if(ColumnName == "Icon")
		{
			return SNew(SBox)
			.Padding(FMargin(4.0f, 0.0f))
			[
				SNew(SImage)
				.Image(TargetPlatform->Icon)
			];
		}
		else if(ColumnName == "Platform")
		{
			return SNew(SBox)
			.Padding(FMargin(4.0f, 0.0f))
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(TargetPlatform->DisplayName)
				.ToolTipText(TargetPlatform->PlatformName.ToString())
			];
		}

		return SNullWidget::NullWidget;
	}
	END_SLATE_FUNCTION_BUILD_OPTIMIZATION

private:
	/** Check to see if the "enabled" checkbox should be checked for this platform */
	ESlateCheckBoxState::Type HandleEnabledCheckBoxIsChecked() const
	{
		// We need to check that the platform is explicitly supported as a target, rather than implicitly supported due to an empty list
		FProjectStatus ProjectStatus;
		return (IProjectManager::Get().QueryStatusForCurrentProject(ProjectStatus) && ProjectStatus.IsTargetPlatformSupported(TargetPlatform->PlatformName, false/*bAllowSupportedIfEmptyList*/)) 
			? ESlateCheckBoxState::Checked 
			: ESlateCheckBoxState::Unchecked;
	}

	/** Handle the "enabled" checkbox state being changed for this platform */
	void HandleEnabledCheckBoxStateChanged(ESlateCheckBoxState::Type InState)
	{
		FGameProjectGenerationModule::Get().UpdateSupportedTargetPlatforms(TargetPlatform->PlatformName, InState == ESlateCheckBoxState::Checked);
	}

	TSharedPtr<FProjectTargetPlatform> TargetPlatform;
};

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SProjectTargetPlatformSettings::Construct(const FArguments& InArgs)
{
	{
		TArray<ITargetPlatform*> AllPlatforms = GetTargetPlatformManager()->GetTargetPlatforms();
		for(ITargetPlatform* Platform : AllPlatforms)
		{
			// We are only interested in standalone games
			const bool bIsGamePlatform = !Platform->IsClientOnly() && !Platform->IsServerOnly() && !Platform->HasEditorOnlyData();
			if(bIsGamePlatform)
			{
				// We use FProjectTargetPlatform rather than ITargetPlatform* to avoid trying to access ITargetPlatform
				// instances after their module has been unloaded (which can happen on editor shutdown)
				AvailablePlatforms.Add(MakeShareable(new FProjectTargetPlatform(
					Platform->DisplayName(), 
					*Platform->PlatformName(), 
					FEditorStyle::GetBrush(*FString::Printf(TEXT("Launcher.Platform_%s"), *Platform->PlatformName()))
					)));
			}
		}
	}

	ChildSlot
	[
		SNew(SVerticalBox)

		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(5.0f)
			[
				SNew(SListView<TSharedPtr<FProjectTargetPlatform>>)
				.ItemHeight(20.0f)
				.ListItemsSource(&AvailablePlatforms)
				.OnGenerateRow(this, &SProjectTargetPlatformSettings::HandlePlatformsListViewGenerateRow)
				.SelectionMode(ESelectionMode::Single)
				.HeaderRow
				(
					SNew(SHeaderRow)

					+SHeaderRow::Column("Enabled")
					.DefaultLabel(FText::FromString(TEXT(" ")))
					.FixedWidth(28.0f)
					.HAlignCell(HAlign_Center)
					.HAlignHeader(HAlign_Center)

					+SHeaderRow::Column("Icon")
					.DefaultLabel(FText::FromString(TEXT(" ")))
					.FixedWidth(32.0f)

					+SHeaderRow::Column("Platform")
					.DefaultLabel(LOCTEXT("PlatformsListPlatformColumnHeader", "Platform"))
				)
			]
		]

		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.0f, 5.0f))
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(5.0f)
			[
				SNew(SVerticalBox)

				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Text(LOCTEXT("PlatformsListDescription", "Use the list above to select the platforms that your project is targeting. Attempting to run your project on a platform that hasn't been selected will result in a warning stating that the project may not run as expected."))
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Text(LOCTEXT("PlatformsListNote", "Please note that selecting no platforms is considered the same as selecting all platforms."))
				]
			]
		]
	];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

TSharedRef<ITableRow> SProjectTargetPlatformSettings::HandlePlatformsListViewGenerateRow(TSharedPtr<FProjectTargetPlatform> TargetPlatform, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SProjectTargetPlatformsListRow, OwnerTable)
		.TargetPlatform(TargetPlatform);
}

#undef LOCTEXT_NAMESPACE
