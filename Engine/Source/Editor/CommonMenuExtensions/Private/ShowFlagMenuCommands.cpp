// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "ShowFlagMenuCommands.h"
#include "EditorShowFlags.h"
#include "EditorStyleSet.h"

#define LOCTEXT_NAMESPACE "ShowFlagMenuCommands"

namespace
{
	inline FText GetLocalizedShowFlagName(const FShowFlagData& Flag)
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("ShowFlagName"), Flag.DisplayName);

		switch (Flag.Group)
		{
			case SFG_Visualize:
			{
				return FText::Format(LOCTEXT("VisualizeFlagLabel", "Visualize {ShowFlagName}"), Args);
			}
			
			default:
			{
				return FText::Format(LOCTEXT("ShowFlagLabel", "Show {ShowFlagName}"), Args);
			}
		}
	}
}

FShowFlagMenuCommands::FShowFlagMenuCommands()
	: TCommands<FShowFlagMenuCommands>
	(
		TEXT("ShowFlagsMenu"), // Context name for fast lookup
		NSLOCTEXT("Contexts", "ShowFlagsMenu", "Show Flags Menu"), // Localized context name for displaying
		NAME_None, // Parent context name.  
		FEditorStyle::GetStyleSetName() // Icon Style Set
	),
	ShowFlagCommands(),
	bCommandsInitialised(false)
{
}

void FShowFlagMenuCommands::BuildShowFlagsMenu(FMenuBuilder& Menu, const FShowFlagFilter& Filter) const
{
	check(bCommandsInitialised);

	const FShowFlagFilter::FGroupedShowFlagIndices& FlagIndices = Filter.GetFilteredIndices();
	if ( FlagIndices.TotalIndices() < 1 )
	{
		return;
	}

	CreateCommonShowFlagMenuItems(Menu, Filter);

	Menu.BeginSection("LevelViewportShowFlags", LOCTEXT("AllShowFlagHeader", "All Show Flags"));
	{
		CreateSubMenuIfRequired(Menu, Filter, SFG_PostProcess, LOCTEXT("PostProcessShowFlagsMenu", "Post Processing"), LOCTEXT("PostProcessShowFlagsMenu_ToolTip", "Post process show flags"));
		CreateSubMenuIfRequired(Menu, Filter, SFG_LightTypes, LOCTEXT("LightTypesShowFlagsMenu", "Light Types"), LOCTEXT("LightTypesShowFlagsMenu_ToolTip", "Light Types show flags"));
		CreateSubMenuIfRequired(Menu, Filter, SFG_LightingComponents, LOCTEXT("LightingComponentsShowFlagsMenu", "Lighting Components"), LOCTEXT("LightingComponentsShowFlagsMenu_ToolTip", "Lighting Components show flags"));
		CreateSubMenuIfRequired(Menu, Filter, SFG_LightingFeatures, LOCTEXT("LightingFeaturesShowFlagsMenu", "Lighting Features"), LOCTEXT("LightingFeaturesShowFlagsMenu_ToolTip", "Lighting Features show flags"));
		CreateSubMenuIfRequired(Menu, Filter, SFG_Developer, LOCTEXT("DeveloperShowFlagsMenu", "Developer"), LOCTEXT("DeveloperShowFlagsMenu_ToolTip", "Developer show flags"));
		CreateSubMenuIfRequired(Menu, Filter, SFG_Visualize, LOCTEXT("VisualizeShowFlagsMenu", "Visualize"), LOCTEXT("VisualizeShowFlagsMenu_ToolTip", "Visualize show flags"));
		CreateSubMenuIfRequired(Menu, Filter, SFG_Advanced, LOCTEXT("AdvancedShowFlagsMenu", "Advanced"), LOCTEXT("AdvancedShowFlagsMenu_ToolTip", "Advanced show flags"));
	}
	Menu.EndSection();
}

void FShowFlagMenuCommands::CreateCommonShowFlagMenuItems(FMenuBuilder& Menu, const FShowFlagFilter& Filter) const
{
	const FShowFlagFilter::FGroupedShowFlagIndices& GroupedFlagIndices = Filter.GetFilteredIndices();
	const TArray<uint32>& FlagIndices = GroupedFlagIndices[SFG_Normal];

	if (FlagIndices.Num() < 1)
	{
		return;
	}

	Menu.BeginSection("ShowFlagsMenuSectionCommon", LOCTEXT("CommonShowFlagHeader", "Common Show Flags"));
	{
		for (int32 ArrayIndex = 0; ArrayIndex < FlagIndices.Num(); ++ArrayIndex)
		{
			const uint32 FlagIndex = FlagIndices[ArrayIndex];
			const FShowFlagCommand& ShowFlagCommand = ShowFlagCommands[FlagIndex];

			Menu.AddMenuEntry(ShowFlagCommand.ShowMenuItem, NAME_None, ShowFlagCommand.LabelOverride);
		}
	}
	Menu.EndSection();
}

void FShowFlagMenuCommands::CreateSubMenuIfRequired(FMenuBuilder& Menu, const FShowFlagFilter& Filter, EShowFlagGroup Group, const FText& MenuLabel, const FText& ToolTip) const
{
	const FShowFlagFilter::FGroupedShowFlagIndices& GroupedFlagIndices = Filter.GetFilteredIndices();
	const TArray<uint32>& FlagIndices = GroupedFlagIndices[Group];

	if ( FlagIndices.Num() < 1 )
	{
		return;
	}

	Menu.AddSubMenu(MenuLabel, ToolTip, FNewMenuDelegate::CreateStatic(&FShowFlagMenuCommands::StaticCreateShowFlagsSubMenu, FlagIndices, 0));
}

void FShowFlagMenuCommands::CreateShowFlagsSubMenu(FMenuBuilder& Menu, TArray<uint32> FlagIndices, int32 EntryOffset) const
{
	// Generate entries for the standard show flags.
	// Assumption: the first 'n' entries types like 'Show All' and 'Hide All' buttons, so insert a separator after them.

	for (int32 ArrayIndex = 0; ArrayIndex < FlagIndices.Num(); ++ArrayIndex)
	{
		const uint32 FlagIndex = FlagIndices[ArrayIndex];
		const FShowFlagCommand& ShowFlagCommand = ShowFlagCommands[FlagIndex];

		Menu.AddMenuEntry(ShowFlagCommand.ShowMenuItem, NAME_None, ShowFlagCommand.LabelOverride);

		if (ArrayIndex == EntryOffset - 1)
		{
			Menu.AddMenuSeparator();
		}
	}
}

void FShowFlagMenuCommands::RegisterCommands()
{
	CreateShowFlagCommands();
	bCommandsInitialised = true;
}

void FShowFlagMenuCommands::CreateShowFlagCommands()
{
	const TArray<FShowFlagData>& AllShowFlags = GetShowFlagMenuItems();

	for (int32 ShowFlagIndex = 0; ShowFlagIndex < AllShowFlags.Num(); ++ShowFlagIndex)
	{
		const FShowFlagData& ShowFlag = AllShowFlags[ShowFlagIndex];
		const FText LocalizedName = GetLocalizedShowFlagName(ShowFlag);

		//@todo Slate: The show flags system does not support descriptions currently
		const FText ShowFlagDesc;

		TSharedPtr<FUICommandInfo> ShowFlagCommand = FUICommandInfoDecl(this->AsShared(), ShowFlag.ShowFlagName, LocalizedName, ShowFlagDesc)
			.UserInterfaceType(EUserInterfaceActionType::ToggleButton)
			.DefaultChord(ShowFlag.InputChord)
			.Icon(GetShowFlagIcon(ShowFlag));

		ShowFlagCommands.Add(FShowFlagCommand(static_cast<FEngineShowFlags::EShowFlag>(ShowFlag.EngineShowFlagIndex), ShowFlagCommand, ShowFlag.DisplayName));
	}
}

void FShowFlagMenuCommands::StaticCreateShowFlagsSubMenu(FMenuBuilder& MenuBuilder, TArray<uint32> FlagIndices, int32 EntryOffset)
{
	FShowFlagMenuCommands::Get().CreateShowFlagsSubMenu(MenuBuilder, FlagIndices, EntryOffset);
}

FSlateIcon FShowFlagMenuCommands::GetShowFlagIcon(const FShowFlagData& Flag) const
{
	return Flag.Group == EShowFlagGroup::SFG_Normal
		? FSlateIcon(FEditorStyle::GetStyleSetName(), FEditorStyle::Join(GetContextName(), TCHAR_TO_ANSI(*FString::Printf(TEXT(".%s"), *Flag.ShowFlagName.ToString()))))
		: FSlateIcon();
}

void FShowFlagMenuCommands::BindCommands(FUICommandList& CommandList, const TSharedPtr<FEditorViewportClient>& Client) const
{
	check(bCommandsInitialised);
	check(Client.IsValid());

	for (int32 ArrayIndex = 0; ArrayIndex < ShowFlagCommands.Num(); ++ArrayIndex)
	{
		const FShowFlagCommand& ShowFlagCommand = ShowFlagCommands[ArrayIndex];

		CommandList.MapAction(ShowFlagCommand.ShowMenuItem,
							  FExecuteAction::CreateStatic<const TSharedPtr<FEditorViewportClient>&>(&FShowFlagMenuCommands::ToggleShowFlag, Client, ShowFlagCommand.FlagIndex),
							  FCanExecuteAction(),
							  FIsActionChecked::CreateStatic<const TSharedPtr<FEditorViewportClient>&>(&FShowFlagMenuCommands::IsShowFlagEnabled, Client, ShowFlagCommand.FlagIndex));
	}
}

void FShowFlagMenuCommands::ToggleShowFlag(const TSharedPtr<FEditorViewportClient>& Client, FEngineShowFlags::EShowFlag EngineShowFlagIndex)
{
	check(Client.IsValid());

	Client->HandleToggleShowFlag(EngineShowFlagIndex);
}

bool FShowFlagMenuCommands::IsShowFlagEnabled(const TSharedPtr<FEditorViewportClient>& Client, FEngineShowFlags::EShowFlag EngineShowFlagIndex)
{
	check(Client.IsValid());

	return Client->HandleIsShowFlagEnabled(EngineShowFlagIndex);
}

#undef LOCTEXT_NAMESPACE