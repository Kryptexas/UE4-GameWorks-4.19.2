// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Settings.cpp: Implements the constructors for the various settings classes.
=============================================================================*/

#include "UnrealEd.h"
#include "ISourceControlModule.h"


/* UContentBrowserSettings interface
 *****************************************************************************/

UContentBrowserSettings::FSettingChangedEvent UContentBrowserSettings::SettingChangedEvent;

UContentBrowserSettings::UContentBrowserSettings( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{ }


void UContentBrowserSettings::PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent )
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName Name = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (!FUnrealEdMisc::Get().IsDeletePreferences())
	{
		SaveConfig();
	}

	SettingChangedEvent.Broadcast(Name);
}


/* UDestructableMeshEditorSettings interface
 *****************************************************************************/

UDestructableMeshEditorSettings::UDestructableMeshEditorSettings( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{
	AnimPreviewLightingDirection = FRotator(-45.0f,45.0f,0);
	AnimPreviewSkyColor = FColor(0, 0, 255);
	AnimPreviewFloorColor = FColor(51, 51, 51);
	AnimPreviewSkyBrightness = 0.2f*PI;
	AnimPreviewDirectionalColor = FColor(255, 255, 255);
	AnimPreviewLightBrightness = 1.0f*PI;
}


/* UEditorExperimentalSettings interface
 *****************************************************************************/

UEditorExperimentalSettings::UEditorExperimentalSettings( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{
}


void UEditorExperimentalSettings::PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent )
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName Name = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (Name == FName(TEXT("bBehaviorTreeEditor")) && bBehaviorTreeEditor && !FModuleManager::Get().IsModuleLoaded(TEXT("BehaviorTreeEditor")))
	{
		FModuleManager::Get().LoadModule(TEXT("BehaviorTreeEditor"));
	}
	else if (Name == FName(TEXT("ConsoleForGamepadLabels")))
	{
		EKeys::SetConsoleForGamepadLabels(ConsoleForGamepadLabels);
	}

	if (!FUnrealEdMisc::Get().IsDeletePreferences())
	{
		SaveConfig();
	}

	SettingChangedEvent.Broadcast(Name);
}


/* UEditorLoadingSavingSettings interface
 *****************************************************************************/

UEditorLoadingSavingSettings::UEditorLoadingSavingSettings( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{
	TextDiffToolPath.FilePath = TEXT("P4Merge.exe");
}


// @todo thomass: proper settings support for source control module
void UEditorLoadingSavingSettings::SccHackInitialize()
{
	bSCCUseGlobalSettings = ISourceControlModule::Get().GetUseGlobalSettings();
}


void UEditorLoadingSavingSettings::PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent )
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName Name = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (Name == FName(TEXT("bSCCUseGlobalSettings")))
	{
		// unfortunately we cant use UserSettingChangedEvent here as the source control module cannot depend on the editor
		ISourceControlModule::Get().SetUseGlobalSettings(bSCCUseGlobalSettings);
	}

	if (!FUnrealEdMisc::Get().IsDeletePreferences())
	{
		SaveConfig();
	}

	SettingChangedEvent.Broadcast(Name);
}


/* UEditorMiscSettings interface
 *****************************************************************************/

UEditorMiscSettings::UEditorMiscSettings( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{ }


/* ULevelEditorMiscSettings interface
 *****************************************************************************/

ULevelEditorMiscSettings::ULevelEditorMiscSettings( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{
	bAutoApplyLightingEnable = true;
	bMonitorEditorPerformance = true;
}


void ULevelEditorMiscSettings::PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent )
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName Name = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (Name == FName(TEXT("bNavigationAutoUpdate")))
	{
		FWorldContext &EditorContext = GEditor->GetEditorWorldContext();
		UNavigationSystem::SetNavigationAutoUpdateEnabled(bNavigationAutoUpdate, EditorContext.World()->GetNavigationSystem());
	}

	if (!FUnrealEdMisc::Get().IsDeletePreferences())
	{
		SaveConfig();
	}
}


/* ULevelEditorPlaySettings interface
 *****************************************************************************/

ULevelEditorPlaySettings::ULevelEditorPlaySettings( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{
	ClientWindowWidth = 640;
	ClientWindowHeight = 480;
	PlayNumberOfClients = 1;
	PlayNetDedicated = false;
	RunUnderOneProcess = true;
	RouteGamepadToSecondWindow = false;
}


/* ULevelEditorViewportSettings interface
 *****************************************************************************/

ULevelEditorViewportSettings::ULevelEditorViewportSettings( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{
	bLevelStreamingVolumePrevis = false;
	BillboardScale = 1.0f;

	TransformWidgetSizeAdjustment = 0.0f;

	// Set a default preview mesh
	PreviewMeshes.Add(FStringAssetReference("/Engine/EditorMeshes/ColorCalibrator/SM_ColorCalibrator.SM_ColorCalibrator"));
}


void ULevelEditorViewportSettings::PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent )
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName Name = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (Name == FName(TEXT("bAllowTranslateRotateZWidget")))
	{
		if (bAllowTranslateRotateZWidget)
		{
			GLevelEditorModeTools().SetWidgetMode(FWidget::WM_TranslateRotateZ);
		}
		else if (GLevelEditorModeTools().GetWidgetMode() == FWidget::WM_TranslateRotateZ)
		{
			GLevelEditorModeTools().SetWidgetMode(FWidget::WM_Translate);
		}
	}
	else if (Name == FName(TEXT("bHighlightWithBrackets")))
	{
		GEngine->SetSelectedMaterialColor(bHighlightWithBrackets ? FLinearColor::Black : GetDefault<UEditorStyleSettings>()->SelectionColor);
	}
	else if (Name == FName(TEXT("HoverHighlightIntensity")))
	{
		GEngine->HoverHighlightIntensity = HoverHighlightIntensity;
	}
	else if (Name == FName(TEXT("SelectionHighlightIntensity")))
	{
		GEngine->SelectionHighlightIntensity = SelectionHighlightIntensity;
	}
	else if (Name == FName(TEXT("BSPSelectionHighlightIntensity")))
	{
		GEngine->BSPSelectionHighlightIntensity = BSPSelectionHighlightIntensity;
	}
	else if ((Name == FName(TEXT("UserDefinedPosGridSizes"))) || (Name == FName(TEXT("UserDefinedRotGridSizes"))) || (Name == FName(TEXT("ScalingGridSizes"))) || (Name == FName(TEXT("GridIntervals"))))
	{
		const float MinGridSize = (Name == FName(TEXT("GridIntervals"))) ? 4.0f : 0.0001f;
		TArray<float>* ArrayRef = NULL;
		int32* IndexRef = NULL;

		if (Name == FName(TEXT("ScalingGridSizes")))
		{
			ArrayRef = &(ScalingGridSizes);
			IndexRef = &(CurrentScalingGridSize);
		}
		// Don't allow an empty array of grid sizes
		if (ArrayRef->Num() == 0)
		{
			ArrayRef->Add(MinGridSize);
		}

		// Don't allow negative numbers
		for (int32 SizeIdx = 0; SizeIdx < ArrayRef->Num(); ++SizeIdx)
		{
			if ((*ArrayRef)[SizeIdx] < MinGridSize)
			{
				(*ArrayRef)[SizeIdx] = MinGridSize;
			}
		}
	}
	else if( Name == FName(TEXT("bUsePowerOf2SnapSize")) )
	{
		const float BSPSnapSize = bUsePowerOf2SnapSize ? 128.0f : 100.0f;
		UModel::SetGlobalBSPTexelScale( BSPSnapSize );
	}
	else if( Name == FName(TEXT("BillboardScale")))
	{
		UBillboardComponent::SetEditorScale(BillboardScale);
		UArrowComponent::SetEditorScale(BillboardScale);
	}

	if (!FUnrealEdMisc::Get().IsDeletePreferences())
	{
		SaveConfig();
	}

	GEditor->RedrawAllViewports();

	SettingChangedEvent.Broadcast(Name);
}

/* UProjectPackagingSettings interface
 *****************************************************************************/

UProjectPackagingSettings::UProjectPackagingSettings( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{
	BuildConfiguration = PPBC_Development;
	FullRebuild = true;
	UsePakFile = true;
	UseOBB_InAPK = false;
}


void UProjectPackagingSettings::PostEditChangeProperty( FPropertyChangedEvent& PropertyChangedEvent )
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName Name = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (Name == FName((TEXT("DirectoriesToAlwaysCook"))))
	{
		// fix up paths
		for(int32 PathIndex = 0; PathIndex < DirectoriesToAlwaysCook.Num(); PathIndex++)
		{
			FString Path = DirectoriesToAlwaysCook[PathIndex].Path;
			FPaths::MakePathRelativeTo(Path, FPlatformProcess::BaseDir());
			DirectoriesToAlwaysCook[PathIndex].Path = Path;
		}
	}
	else if (Name == FName((TEXT("StagingDirectory"))))
	{
		// fix up path
		FString Path = StagingDirectory.Path;
		FPaths::MakePathRelativeTo(Path, FPlatformProcess::BaseDir());
		StagingDirectory.Path = Path;
	}
}
