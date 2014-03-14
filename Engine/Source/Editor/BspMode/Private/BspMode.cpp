// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BspModePrivatePCH.h"
#include "BspMode.h"
#include "BspModeActions.h"

TSharedRef< FBspMode > FBspMode::Create()
{
	TSharedRef< FBspMode > BspMode = MakeShareable( new FBspMode() );
	BspMode->Initialize();
	return BspMode;
}

FBspMode::FBspMode()
	: bNeedsToRegisterCommands(true)
	, LastActiveModeIdentifier()
{
	ID = TEXT("BSP");
	Name = NSLOCTEXT( "GeometryMode", "DisplayName", "Geometry" );
	IconBrush = FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.BspMode", "LevelEditor.BspMode.Small");
	bVisible = true;
	PriorityOrder = 100;
}

FBspMode::~FBspMode()
{
	FModuleManager::Get().OnModulesChanged().RemoveAll( this );
}

void FBspMode::Initialize()
{
	bVisible = false;
	FModuleManager::Get().OnModulesChanged().AddSP( this, &FBspMode::HandleModulesChanged );
}

bool FBspMode::UsesToolkits() const
{
	return false;
}


void FBspMode::HandleModulesChanged( FName ModuleThatChanged, EModuleChangeReason::Type ReasonForChange )
{
	static const FName LevelEditorModuleName("LevelEditor");
	if(ModuleThatChanged == LevelEditorModuleName && ReasonForChange == EModuleChangeReason::ModuleUnloaded)
	{
		// The LevelEditor module has been unloaded, so we need to re-add our commands next time
		bNeedsToRegisterCommands = true;
	}
}

void FBspMode::Enter()
{
	// Call parent implementation
	FEdMode::Enter();


}

void FBspMode::Exit()
{

	// Call parent implementation
	FEdMode::Exit();
}
