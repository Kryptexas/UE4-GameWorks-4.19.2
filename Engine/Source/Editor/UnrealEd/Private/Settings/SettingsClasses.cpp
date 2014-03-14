// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Settings.cpp: Implements the constructors for the various settings classes.
=============================================================================*/

#include "UnrealEd.h"
#include "ISourceControlModule.h"


/* UContentBrowserSettings interface
 *****************************************************************************/

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


/* ULevelEditorPlaySettings interface
 *****************************************************************************/

UEditorMiscSettings::UEditorMiscSettings( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{ }


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
}


void ULevelEditorViewportSettings::PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent )
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName Name = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (Name == FName(TEXT("bHighlightWithBrackets")))
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
	if (!FUnrealEdMisc::Get().IsDeletePreferences())
	{
		SaveConfig();
	}

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

void UProjectPackagingSettings::UpdateBuildConfigurationVisibility()
{
	// Find if there are any .target.cs files in the project source directory.
	TArray<FString> TargetFileNames;
	IFileManager::Get().FindFiles(TargetFileNames, *(FPaths::GameSourceDir() / TEXT("*.target.cs")), true, false);

	// Get the EProjectPackagingBuildConfigurations enum
	extern UEnum *Z_Construct_UEnum_UProjectPackagingSettings_EProjectPackagingBuildConfigurations();
	UEnum *ProjectPackagingConfigEnum = Z_Construct_UEnum_UProjectPackagingSettings_EProjectPackagingBuildConfigurations();

	// Mark the DebugGame value as hidden if it's a content only project.
	if (TargetFileNames.Num() == 0)
	{
		ProjectPackagingConfigEnum->SetMetaData(TEXT("Hidden"), TEXT("1"), PPBC_DebugGame);
	}
	else
	{
		ProjectPackagingConfigEnum->RemoveMetaData(TEXT("Hidden"), PPBC_DebugGame);
	}
}

