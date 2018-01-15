// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "BufferVisualizationMenuCommands.h"
#include "Containers/UnrealString.h"
#include "Framework/Commands/InputChord.h"
#include "Materials/Material.h"
#include "Internationalization/Text.h"
#include "BufferVisualizationData.h"
#include "Templates/Function.h"
#include "EditorStyleSet.h"
#include "EditorViewportClient.h"

#define LOCTEXT_NAMESPACE "BufferVisualizationMenuCommands"

namespace
{
	struct FMaterialIteratorWithLambda
	{
		typedef TFunction<void(const FString&, const UMaterial*, const FText&)> LambdaFunctionType;
		LambdaFunctionType Lambda;

		inline FMaterialIteratorWithLambda(const LambdaFunctionType& InLambda)
			: Lambda(InLambda)
		{
		}

		inline void ProcessValue(const FString& InMaterialName, const UMaterial* InMaterial, const FText& InDisplayName)
		{
			Lambda(InMaterialName, InMaterial, InDisplayName);
		}
	};

	inline FName NameFromMaterial(const FString& MaterialName)
	{
		return *(FString(TEXT("BufferVisualizationMenu")) + MaterialName);
	}
}

namespace
{
	const FName OVERVIEW_COMMAND_NAME = *(FString(TEXT("BufferVisualizationOverview")));
}

FBufferVisualizationMenuCommands::FBufferVisualizationMenuCommands()
	: TCommands<FBufferVisualizationMenuCommands>
	(
		TEXT("VisualizationMenu"), // Context name for fast lookup
		NSLOCTEXT("Contexts", "VisualizationMenu", "Render Buffer Visualization"), // Localized context name for displaying
		NAME_None, // Parent context name.  
		FEditorStyle::GetStyleSetName() // Icon Style Set
	),
	CommandMap()
{
}

void FBufferVisualizationMenuCommands::BuildCommandMap()
{
	CommandMap.Empty();

	CreateOverviewCommand();
	CreateVisualizationCommands();
}

void FBufferVisualizationMenuCommands::CreateOverviewCommand()
{
	const FName& CommandName = OVERVIEW_COMMAND_NAME;

	FBufferVisualizationRecord& OverviewRecord = CommandMap.Add(CommandName, FBufferVisualizationRecord());
	OverviewRecord.Name = NAME_None;
	OverviewRecord.Command = FUICommandInfoDecl(this->AsShared(), CommandName, LOCTEXT("BufferVisualization", "Overview"), LOCTEXT("BufferVisualization", "Overview"))
		.UserInterfaceType(EUserInterfaceActionType::RadioButton)
		.DefaultChord(FInputChord());
}

void FBufferVisualizationMenuCommands::CreateVisualizationCommands()
{
	FMaterialIteratorWithLambda Iterator([this](const FString& InMaterialName, const UMaterial* InMaterial, const FText& InDisplayName)
	{
		const FName CommandName = NameFromMaterial(InMaterialName);

		FBufferVisualizationRecord& Record = CommandMap.Add(CommandName, FBufferVisualizationRecord());
		Record.Name = *InMaterialName;
		const FText MaterialNameText = FText::FromString(InMaterialName);
		Record.Command = FUICommandInfoDecl(this->AsShared(), CommandName, MaterialNameText, MaterialNameText)
			.UserInterfaceType(EUserInterfaceActionType::RadioButton)
			.DefaultChord(FInputChord());
	});

	GetBufferVisualizationData().IterateOverAvailableMaterials(Iterator);
}

void FBufferVisualizationMenuCommands::BuildVisualisationSubMenu(FMenuBuilder& Menu)
{
	const FBufferVisualizationMenuCommands& Commands = FBufferVisualizationMenuCommands::Get();

	Menu.BeginSection("LevelViewportBufferVisualizationMode", LOCTEXT( "BufferVisualizationHeader", "Buffer Visualization Mode" ) );

	Commands.AddOverviewCommandToMenu(Menu);
	Menu.AddMenuSeparator();
	Commands.AddVisualizationCommandsToMenu(Menu);

	Menu.EndSection();
}

void FBufferVisualizationMenuCommands::AddOverviewCommandToMenu(FMenuBuilder& Menu) const
{
	check(CommandMap.Num() > 0);

	Menu.AddMenuEntry(OverviewCommand().Command, NAME_None, LOCTEXT("BufferVisualization", "Overview"));
}

void FBufferVisualizationMenuCommands::AddVisualizationCommandsToMenu(FMenuBuilder& Menu) const
{
	check(CommandMap.Num() > 0);

	const TBufferVisualizationModeCommandMap& Commands = CommandMap;

	FMaterialIteratorWithLambda Iterator([&Menu, &Commands](const FString& InMaterialName, const UMaterial* InMaterial, const FText& InDisplayName)
	{
		const FName CommandName = NameFromMaterial(InMaterialName);

		const FBufferVisualizationRecord* Record = Commands.Find(CommandName);
		if (ensureMsgf(Record != NULL, TEXT("Buffer visualization commands don't contain entry [%s]"), *CommandName.ToString()))
		{
			Menu.AddMenuEntry(Record->Command, NAME_None, InDisplayName);
		}
	});

	GetBufferVisualizationData().IterateOverAvailableMaterials(Iterator);
}

FBufferVisualizationMenuCommands::TCommandConstIterator FBufferVisualizationMenuCommands::CreateCommandConstIterator() const
{
	return CommandMap.CreateConstIterator();
}

const FBufferVisualizationMenuCommands::FBufferVisualizationRecord& FBufferVisualizationMenuCommands::OverviewCommand() const
{
	check(CommandMap.Num() > 0);

	return CommandMap.FindChecked(OVERVIEW_COMMAND_NAME);
}

void FBufferVisualizationMenuCommands::RegisterCommands()
{
	BuildCommandMap();
}

void FBufferVisualizationMenuCommands::BindCommands(FUICommandList& CommandList, const TSharedPtr<FEditorViewportClient>& Client) const
{
	// Map Buffer visualization mode actions
	for (FBufferVisualizationMenuCommands::TCommandConstIterator It = FBufferVisualizationMenuCommands::Get().CreateCommandConstIterator(); It; ++It)
	{
		const FBufferVisualizationMenuCommands::FBufferVisualizationRecord& Record = It.Value();
		CommandList.MapAction(
			Record.Command,
			FExecuteAction::CreateStatic<const TSharedPtr<FEditorViewportClient>&>(&FBufferVisualizationMenuCommands::ChangeBufferVisualizationMode, Client, Record.Name ),
			FCanExecuteAction(),
			FIsActionChecked::CreateStatic<const TSharedPtr<FEditorViewportClient>&>(&FBufferVisualizationMenuCommands::IsBufferVisualizationModeSelected, Client, Record.Name ) );
	}
}

void FBufferVisualizationMenuCommands::ChangeBufferVisualizationMode(const TSharedPtr<FEditorViewportClient>& Client, FName InName)
{
	check(Client.IsValid());

	Client->ChangeBufferVisualizationMode(InName);
}

bool FBufferVisualizationMenuCommands::IsBufferVisualizationModeSelected(const TSharedPtr<FEditorViewportClient>& Client, FName InName)
{
	check(Client.IsValid());

	return Client->IsBufferVisualizationModeSelected(InName);
}

#undef LOCTEXT_NAMESPACE
