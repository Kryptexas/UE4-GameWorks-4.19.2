// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ControlRigEditModeSettings.h"
#include "EditorModeManager.h"
#include "ControlRigEditMode.h"
#include "ControlRigSequence.h"
#include "AssetEditorManager.h"

void UControlRigEditModeSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	if (UProperty* Property = PropertyChangedEvent.Property)
	{
		if (Property->GetFName() == GET_MEMBER_NAME_CHECKED(UControlRigEditModeSettings, Actor))
		{
			// user set the actor we are targeting, so bind our (standalone) sequence to it
			if (FControlRigEditMode* ControlRigEditMode = static_cast<FControlRigEditMode*>(GLevelEditorModeTools().GetActiveMode(FControlRigEditMode::ModeName)))
			{
				ControlRigEditMode->HandleBindToActor(Actor.Get());
			}
		}
		else if (Property->GetFName() == GET_MEMBER_NAME_CHECKED(UControlRigEditModeSettings, Sequence))
		{
			if (Sequence)
			{
				FAssetEditorManager::Get().OpenEditorForAsset(Sequence);
			}
		}
	}
}