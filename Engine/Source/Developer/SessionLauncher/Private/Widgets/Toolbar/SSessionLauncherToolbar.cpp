// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherToolbar.cpp: Implements the SSessionLauncherToolbar class.
=============================================================================*/

#include "SessionLauncherPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SSessionLauncherToolbar"


/* SSessionLauncherTaskSelector interface
 *****************************************************************************/

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SSessionLauncherToolbar::Construct( const FArguments& InArgs, const FSessionLauncherModelRef& InModel, SSessionLauncher* InLauncher )
{
	FSessionLauncherCommands::Register();

	Model = InModel;
	Launcher = InLauncher;

	CreateCommands();

	ChildSlot
	[
		MakeToolbar(CommandList)
	];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION


void SSessionLauncherToolbar::CreateCommands( )
{
	const FSessionLauncherCommands& Commands = FSessionLauncherCommands::Get();

	// Build command
	CommandList->MapAction(Commands.Build,
		FExecuteAction::CreateRaw(this, &SSessionLauncherToolbar::HandleBuildActionExecute),
		FCanExecuteAction::CreateRaw(this, &SSessionLauncherToolbar::HandleActionCanExecute),
		FIsActionChecked::CreateRaw(this, &SSessionLauncherToolbar::HandleBuildActionIsChecked)
	);

	// Deploy a pre-made build command
	CommandList->MapAction(Commands.DeployBuild,
		FExecuteAction::CreateRaw(this, &SSessionLauncherToolbar::HandleDeployBuildActionExecute),
		FCanExecuteAction::CreateRaw(this, &SSessionLauncherToolbar::HandleActionCanExecute),
		FIsActionChecked::CreateRaw(this, &SSessionLauncherToolbar::HandleDeployBuildActionIsChecked)
	);

	// Build, cook, and launch command
	CommandList->MapAction(Commands.QuickLaunch,
		FExecuteAction::CreateRaw(this, &SSessionLauncherToolbar::HandleQuickLaunchActionExecute),
		FCanExecuteAction::CreateRaw(this, &SSessionLauncherToolbar::HandleActionCanExecute),
		FIsActionChecked::CreateRaw(this, &SSessionLauncherToolbar::HandleQuickLaunchActionIsChecked)
	);

	// Advanced command
	CommandList->MapAction(Commands.AdvancedBuild,
		FExecuteAction::CreateRaw(this, &SSessionLauncherToolbar::HandleAdvancedBuildActionExecute),
		FCanExecuteAction::CreateRaw(this, &SSessionLauncherToolbar::HandleActionCanExecute),
		FIsActionChecked::CreateRaw(this, &SSessionLauncherToolbar::HandleAdvancedBuildActionIsChecked)
	);
}


/* SSessionLauncherTaskSelector implementation
 *****************************************************************************/

TSharedRef<SWidget> SSessionLauncherToolbar::MakeToolbar( const TSharedRef<FUICommandList>& CommandList )
{
	FToolBarBuilder ToolBarBuilder(CommandList, FMultiBoxCustomization::None, TSharedPtr<FExtender>(), EOrientation::Orient_Vertical);

	ToolBarBuilder.BeginSection("Tasks");
	{
		ToolBarBuilder.AddToolBarButton(FSessionLauncherCommands::Get().QuickLaunch);
		ToolBarBuilder.AddToolBarButton(FSessionLauncherCommands::Get().Build);
		ToolBarBuilder.AddToolBarButton(FSessionLauncherCommands::Get().DeployBuild);
		ToolBarBuilder.AddToolBarButton(FSessionLauncherCommands::Get().AdvancedBuild);
	}

	return ToolBarBuilder.MakeWidget();
}


/* SSessionLauncherTaskSelector callbacks
 *****************************************************************************/

void SSessionLauncherToolbar::HandleAdvancedBuildActionExecute( )
{
	Launcher->ShowWidget(ESessionLauncherTasks::Advanced);
}


void SSessionLauncherToolbar::HandleBuildActionExecute( )
{
	Launcher->ShowWidget(ESessionLauncherTasks::Build);
}


void SSessionLauncherToolbar::HandleDeployBuildActionExecute( )
{
	Launcher->ShowWidget(ESessionLauncherTasks::DeployRepository);
}


void SSessionLauncherToolbar::HandleQuickLaunchActionExecute( )
{
	Launcher->ShowWidget(ESessionLauncherTasks::QuickLaunch);
}


#undef LOCTEXT_NAMESPACE
