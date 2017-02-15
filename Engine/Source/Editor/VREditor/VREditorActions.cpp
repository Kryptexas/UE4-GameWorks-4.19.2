// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "VREditorActions.h"
#include "VREditorModule.h"
#include "VREditorUISystem.h"
#include "VREditorMode.h"
#include "ViewportWorldInteraction.h"
#include "VREditorInteractor.h"
#include "EditorWorldManager.h"
#include "VREditorFloatingUI.h"
#include "VREditorTransformGizmo.h"
#include "SLevelViewport.h"
#include "ImageUtils.h"
#include "FileHelper.h"
#include "Framework/Application/SlateApplication.h"

#define LOCTEXT_NAMESPACE "VREditorActions"

ECheckBoxState FVREditorActionCallbacks::GetTranslationSnapState()
{
	return GetDefault<ULevelEditorViewportSettings>()->GridEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void FVREditorActionCallbacks::OnTranslationSnapSizeButtonClicked()
{
	const TArray<float>& GridSizes = GEditor->GetCurrentPositionGridArray();
	const int32 CurrentGridSize = GetDefault<ULevelEditorViewportSettings>()->CurrentPosGridSize;

	int32 NewGridSize = CurrentGridSize + 1;
	if (NewGridSize >= GridSizes.Num())
	{
		NewGridSize = 0;
	}

	GEditor->SetGridSize(NewGridSize);

	GetTranslationSnapSizeText();

}

FText FVREditorActionCallbacks::GetTranslationSnapSizeText() 
{
	return FText::AsNumber(GEditor->GetGridSize());

}

ECheckBoxState FVREditorActionCallbacks::GetRotationSnapState()
{
	return GetDefault<ULevelEditorViewportSettings>()->RotGridEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void FVREditorActionCallbacks::OnRotationSnapSizeButtonClicked()
{
	const TArray<float>& GridSizes = GEditor->GetCurrentRotationGridArray();
	const int32 CurrentGridSize = GetDefault<ULevelEditorViewportSettings>()->CurrentRotGridSize;
	const ERotationGridMode CurrentGridMode = GetDefault<ULevelEditorViewportSettings>()->CurrentRotGridMode;

	int32 NewGridSize = CurrentGridSize + 1;
	if (NewGridSize >= GridSizes.Num())
	{
		NewGridSize = 0;
	}

	GEditor->SetRotGridSize(NewGridSize, CurrentGridMode);

}

FText FVREditorActionCallbacks::GetRotationSnapSizeText() 
{
	return FText::AsNumber(GEditor->GetRotGridSize().Yaw);
}

ECheckBoxState FVREditorActionCallbacks::GetScaleSnapState()
{
	return GetDefault<ULevelEditorViewportSettings>()->SnapScaleEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void FVREditorActionCallbacks::OnScaleSnapSizeButtonClicked()
{
	const ULevelEditorViewportSettings* ViewportSettings = GetMutableDefault<ULevelEditorViewportSettings>();
	const int32 NumGridSizes = ViewportSettings->ScalingGridSizes.Num();

	const int32 CurrentGridSize = GetDefault<ULevelEditorViewportSettings>()->CurrentScalingGridSize;

	int32 NewGridSize = CurrentGridSize + 1;
	if (NewGridSize >= NumGridSizes)
	{
		NewGridSize = 0;
	}

	GEditor->SetScaleGridSize(NewGridSize);

	
}

FText FVREditorActionCallbacks::GetScaleSnapSizeText() 
{
	return FText::AsNumber(GEditor->GetScaleGridSize());
}

void FVREditorActionCallbacks::OnGizmoCoordinateSystemButtonClicked(class UVREditorMode* InVRMode)
{
	InVRMode->GetWorldInteraction().CycleTransformGizmoCoordinateSpace();
	UpdateGizmoModeText(InVRMode);
	UpdateGizmoCoordinateSystemText(InVRMode);

}

FText FVREditorActionCallbacks::GetGizmoCoordinateSystemText()
{
	return EVREditorActions::GizmoCoordinateSystemText;
}

void FVREditorActionCallbacks::UpdateGizmoCoordinateSystemText(class UVREditorMode* InVRMode) 
{
	const ECoordSystem CurrentCoordSystem = InVRMode->GetWorldInteraction().GetTransformGizmoCoordinateSpace(); //@todo VREditor
	EVREditorActions::GizmoCoordinateSystemText = (CurrentCoordSystem == COORD_World ? LOCTEXT("WorldCoordinateSystem", "World Space") : LOCTEXT("LocalCoordinateSystem", "Local Space"));
}

void FVREditorActionCallbacks::OnGizmoModeButtonClicked(class UVREditorMode* InVRMode)
{
	const EGizmoHandleTypes CurrentGizmoMode = InVRMode->GetWorldInteraction().GetCurrentGizmoType();
	InVRMode->CycleTransformGizmoHandleType();
	UpdateGizmoModeText(InVRMode);
	UpdateGizmoCoordinateSystemText(InVRMode);

}

FText FVREditorActionCallbacks::GetGizmoModeText()
{
	return EVREditorActions::GizmoModeText;
}

void FVREditorActionCallbacks::UpdateGizmoModeText(class UVREditorMode* InVRMode) 
{
	const EGizmoHandleTypes CurrentGizmoType = InVRMode->GetWorldInteraction().GetCurrentGizmoType();
	FText GizmoTypeText;
	switch (CurrentGizmoType)
	{
	case EGizmoHandleTypes::All:
		GizmoTypeText = LOCTEXT("AllGizmoType", "Universal");
		break;

	case EGizmoHandleTypes::Translate:
		GizmoTypeText = LOCTEXT("TranslateGizmoType", "Translate");
		break;

	case EGizmoHandleTypes::Rotate:
		GizmoTypeText = LOCTEXT("RotationGizmoType", "Rotation");
		break;

	case EGizmoHandleTypes::Scale:
		GizmoTypeText = LOCTEXT("ScaleGizmoType", "Scale");
		break;

	default:
		check(0);	// Unrecognized type
		break;
	}

	EVREditorActions::GizmoModeText = GizmoTypeText;
}

void FVREditorActionCallbacks::OnGizmoCycle(class UVREditorMode* InVRMode)
{
	InVRMode->CycleTransformGizmoHandleType();
	UpdateGizmoModeText(InVRMode);
	UpdateGizmoCoordinateSystemText(InVRMode);
}

void FVREditorActionCallbacks::OnUIToggleButtonClicked(class UVREditorMode* InVRMode, const UVREditorUISystem::EEditorUIPanel PanelToToggle)
{
	InVRMode->GetUISystem().TogglePanelVisibility(PanelToToggle);
}

ECheckBoxState FVREditorActionCallbacks::GetUIToggledState(class UVREditorMode* InVRMode, const UVREditorUISystem::EEditorUIPanel PanelToCheck)
{
	return InVRMode->GetUISystem().IsShowingEditorUIPanel(PanelToCheck) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}


void FVREditorActionCallbacks::OnLightButtonClicked(class UVREditorMode* InVRMode)
{
	// Always spawn the flashlight on the hand clicking on the UI
	UVREditorInteractor* VREditorInteractor = InVRMode->GetHandInteractor(EControllerHand::Left);
	if (!VREditorInteractor->IsHoveringOverUI())
	{
		VREditorInteractor = InVRMode->GetHandInteractor(EControllerHand::Right);

	}
	InVRMode->ToggleFlashlight(VREditorInteractor);

}

ECheckBoxState FVREditorActionCallbacks::GetFlashlightState(class UVREditorMode* InVRMode)
{
	return InVRMode->IsFlashlightOn() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}


void FVREditorActionCallbacks::OnScreenshotButtonClicked(class UVREditorMode* InVRMode)
{
	// @todo vreditor: update after direct buffer grab changes

	FString GeneratedFilename = TEXT("");
	FString Filename = TEXT("");
	FScreenshotRequest::CreateViewportScreenShotFilename(GeneratedFilename);
	const bool bRemovePath = false;
	GeneratedFilename = FPaths::GetBaseFilename(GeneratedFilename, bRemovePath);
	FFileHelper::GenerateNextBitmapFilename(GeneratedFilename, TEXT("png"), Filename);

	TSharedRef<SWidget> WindowRef = InVRMode->GetLevelViewportPossessedForVR().AsWidget();


	TArray<FColor> OutImageData;
	FIntVector OutImageSize;

	if (FSlateApplication::Get().TakeScreenshot(WindowRef, OutImageData, OutImageSize))
	{
		int32 Width = OutImageSize.X;
		int32 Height = OutImageSize.Y;

		TArray<FColor> ScaledBitmap;


		ScaledBitmap = OutImageData;
		//Clear the alpha channel before saving
		for (int32 Index = 0; Index < Width*Height; Index++)
		{
			ScaledBitmap[Index].A = 255;
		}


		TArray<uint8> CompressedBitmap;
		FImageUtils::CompressImageArray(Width, Height, ScaledBitmap, CompressedBitmap);


		//Save locally
		const bool bTree = true;
		const FString FinalFileName = Filename;
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(FinalFileName), bTree);
		FFileHelper::SaveArrayToFile(CompressedBitmap, *FinalFileName);

	}


}

void FVREditorActionCallbacks::OnPlayButtonClicked(class UVREditorMode* InVRMode)
{
	InVRMode->StartExitingVRMode(EVREditorExitType::PIE_VR);
}

void FVREditorActionCallbacks::OnSnapActorsToGroundClicked(class UVREditorMode* InVRMode)
{
	InVRMode->SnapSelectedActorsToGround();
}


void FVREditorActionCallbacks::SimulateCharacterEntry(const FString InChar)
{

	for (int32 CharIndex = 0; CharIndex < InChar.Len(); CharIndex++)
	{
		TCHAR CharKey = InChar[CharIndex];
		const bool bRepeat = false;
		FCharacterEvent CharacterEvent(CharKey, FModifierKeysState::FModifierKeysState(), 0, bRepeat);
		FSlateApplication::Get().ProcessKeyCharEvent(CharacterEvent);
	}

}

void FVREditorActionCallbacks::SimulateBackspace()
{
		// Slate editable text fields handle backspace as a character entry
		FString BackspaceString = FString(TEXT("\b"));
		bool bRepeat = false;
		SimulateCharacterEntry(BackspaceString);
}

void FVREditorActionCallbacks::SimulateKeyDown(FKey Key, bool bRepeat)
{
	const uint32* KeyCodePtr;
	const uint32* CharCodePtr;
	FInputKeyManager::Get().GetCodesFromKey( Key, KeyCodePtr, CharCodePtr );

	uint32 KeyCode = KeyCodePtr ? *KeyCodePtr : 0;
	uint32 CharCode = CharCodePtr ? *CharCodePtr : 0;

	FKeyEvent KeyEvent( Key, FModifierKeysState::FModifierKeysState(), 0, bRepeat, KeyCode, CharCode );
	bool DownResult = FSlateApplication::Get().ProcessKeyDownEvent( KeyEvent );

	if (CharCodePtr)
	{
		FCharacterEvent CharacterEvent( CharCode, FModifierKeysState::FModifierKeysState(), 0, bRepeat );
		FSlateApplication::Get().ProcessKeyCharEvent( CharacterEvent );
	}
}

void FVREditorActionCallbacks::SimulateKeyUp(FKey Key)
{
	const uint32* KeyCodePtr;
	const uint32* CharCodePtr;
	FInputKeyManager::Get().GetCodesFromKey( Key, KeyCodePtr, CharCodePtr );

	uint32 KeyCode = KeyCodePtr ? *KeyCodePtr : 0;
	uint32 CharCode = CharCodePtr ? *CharCodePtr : 0;

	FKeyEvent KeyEvent( Key, FModifierKeysState::FModifierKeysState(), 0, false, KeyCode, CharCode );
	FSlateApplication::Get().ProcessKeyUpEvent( KeyEvent );
}

#undef LOCTEXT_NAMESPACE
