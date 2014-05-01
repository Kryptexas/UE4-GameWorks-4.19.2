// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ActorPickerModePrivatePCH.h"

IMPLEMENT_MODULE( FActorPickerModeModule, ActorPickerMode );

void FActorPickerModeModule::StartupModule()
{
	EdModeActorPicker = MakeShareable(new FEdModeActorPicker);
	GEditorModeTools().RegisterMode(EdModeActorPicker.ToSharedRef());

	// we want to be able to perform this action with all the built-in editor modes
	GEditorModeTools().RegisterCompatibleModes(EdModeActorPicker->GetID(), FBuiltinEditorModes::EM_Default);
	GEditorModeTools().RegisterCompatibleModes(EdModeActorPicker->GetID(), FBuiltinEditorModes::EM_Placement);
	GEditorModeTools().RegisterCompatibleModes(EdModeActorPicker->GetID(), FBuiltinEditorModes::EM_Bsp);
	GEditorModeTools().RegisterCompatibleModes(EdModeActorPicker->GetID(), FBuiltinEditorModes::EM_Geometry);
	GEditorModeTools().RegisterCompatibleModes(EdModeActorPicker->GetID(), FBuiltinEditorModes::EM_InterpEdit);
	GEditorModeTools().RegisterCompatibleModes(EdModeActorPicker->GetID(), FBuiltinEditorModes::EM_Texture);
	GEditorModeTools().RegisterCompatibleModes(EdModeActorPicker->GetID(), FBuiltinEditorModes::EM_MeshPaint);
	GEditorModeTools().RegisterCompatibleModes(EdModeActorPicker->GetID(), FBuiltinEditorModes::EM_Landscape);
	GEditorModeTools().RegisterCompatibleModes(EdModeActorPicker->GetID(), FBuiltinEditorModes::EM_Foliage);
	GEditorModeTools().RegisterCompatibleModes(EdModeActorPicker->GetID(), FBuiltinEditorModes::EM_Level);
	GEditorModeTools().RegisterCompatibleModes(EdModeActorPicker->GetID(), FBuiltinEditorModes::EM_StreamingLevel);
	GEditorModeTools().RegisterCompatibleModes(EdModeActorPicker->GetID(), FBuiltinEditorModes::EM_Physics);
}

void FActorPickerModeModule::ShutdownModule()
{
	EndActorPickingMode();

	GEditorModeTools().UnregisterCompatibleModes(EdModeActorPicker->GetID(), FBuiltinEditorModes::EM_Default);
	GEditorModeTools().UnregisterCompatibleModes(EdModeActorPicker->GetID(), FBuiltinEditorModes::EM_Placement);
	GEditorModeTools().UnregisterCompatibleModes(EdModeActorPicker->GetID(), FBuiltinEditorModes::EM_Bsp);
	GEditorModeTools().UnregisterCompatibleModes(EdModeActorPicker->GetID(), FBuiltinEditorModes::EM_Geometry);
	GEditorModeTools().UnregisterCompatibleModes(EdModeActorPicker->GetID(), FBuiltinEditorModes::EM_InterpEdit);
	GEditorModeTools().UnregisterCompatibleModes(EdModeActorPicker->GetID(), FBuiltinEditorModes::EM_Texture);
	GEditorModeTools().UnregisterCompatibleModes(EdModeActorPicker->GetID(), FBuiltinEditorModes::EM_MeshPaint);
	GEditorModeTools().UnregisterCompatibleModes(EdModeActorPicker->GetID(), FBuiltinEditorModes::EM_Landscape);
	GEditorModeTools().UnregisterCompatibleModes(EdModeActorPicker->GetID(), FBuiltinEditorModes::EM_Foliage);
	GEditorModeTools().UnregisterCompatibleModes(EdModeActorPicker->GetID(), FBuiltinEditorModes::EM_Level);
	GEditorModeTools().UnregisterCompatibleModes(EdModeActorPicker->GetID(), FBuiltinEditorModes::EM_StreamingLevel);
	GEditorModeTools().UnregisterCompatibleModes(EdModeActorPicker->GetID(), FBuiltinEditorModes::EM_Physics);
	GEditorModeTools().UnregisterMode(EdModeActorPicker.ToSharedRef());
	EdModeActorPicker = nullptr;
}

void FActorPickerModeModule::BeginActorPickingMode(FOnGetAllowedClasses InOnGetAllowedClasses, FOnShouldFilterActor InOnShouldFilterActor, FOnActorSelected InOnActorSelected)
{
	EndActorPickingMode();
	EdModeActorPicker->BeginMode(InOnGetAllowedClasses, InOnShouldFilterActor, InOnActorSelected);
}

void FActorPickerModeModule::EndActorPickingMode()
{
	if(IsInActorPickingMode())
	{
		EdModeActorPicker->EndMode();
	}
}

bool FActorPickerModeModule::IsInActorPickingMode() const
{
	return GEditorModeTools().IsModeActive(EdModeActorPicker->GetID());
}
