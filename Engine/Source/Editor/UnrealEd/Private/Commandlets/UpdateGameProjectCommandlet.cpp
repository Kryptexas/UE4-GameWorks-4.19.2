// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "UnrealEd.h"
#include "ISourceControlModule.h"
#include "GameProjectGenerationModule.h"

DEFINE_LOG_CATEGORY_STATIC(LogUpdateGameProjectCommandlet, Log, All);

UUpdateGameProjectCommandlet::UUpdateGameProjectCommandlet(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	
}

int32 UUpdateGameProjectCommandlet::Main( const FString& InParams )
{
	// Parse command line.
	TArray<FString> Tokens;
	TArray<FString> Switches;
	UCommandlet::ParseCommandLine(*InParams, Tokens, Switches);

	FString Category;
	FString ChangelistDescription = TEXT("Updated game project");
	bool bAutoCheckout = false;
	bool bAutoSubmit = false;
	bool bSignSampleProject = false;

	const FString CategorySwitch = TEXT("Category=");
	const FString ChangelistDescriptionSwitch = TEXT("ChangelistDescription=");
	for ( int32 SwitchIdx = 0; SwitchIdx < Switches.Num(); ++SwitchIdx )
	{
		const FString& Switch = Switches[SwitchIdx];
		if ( Switch == TEXT("AutoCheckout") )
		{
			bAutoCheckout = true;
		}
		else if ( Switch == TEXT("AutoSubmit") )
		{
			bAutoSubmit = true;
		}
		else if ( Switch == TEXT("SignSampleProject") )
		{
			bSignSampleProject = true;
		}
		else if ( Switch.StartsWith(CategorySwitch) )
		{
			Category = Switch.Mid( CategorySwitch.Len() );
		}
		else if ( Switch.StartsWith(ChangelistDescriptionSwitch) )
		{
			ChangelistDescription = Switch.Mid( ChangelistDescriptionSwitch.Len() );
		}
	}

	if ( !FPaths::IsProjectFilePathSet() )
	{
		UE_LOG(LogUpdateGameProjectCommandlet, Error, TEXT("You must launch with a project file to be able to update it"));
		return 1;
	}

	const FString ProjectFilePath = FPaths::GetProjectFilePath();

	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
	if ( bAutoCheckout )
	{
		SourceControlProvider.Init();
	}

	UE_LOG(LogUpdateGameProjectCommandlet, Display, TEXT("Updating project file %s..."), *ProjectFilePath);

	if ( !FGameProjectGenerationModule::Get().UpdateGameProject() )
	{
		// UpdateGameProject produces it's own error to the log
		return 1;
	}

	if ( bSignSampleProject )
	{
		UE_LOG(LogUpdateGameProjectCommandlet, Display, TEXT("Attempting to sign project file %s..."), *ProjectFilePath);

		FText LocalFailReason;
		if ( IProjectManager::Get().SignSampleProject(ProjectFilePath, Category, LocalFailReason) )
		{
			UE_LOG(LogUpdateGameProjectCommandlet, Display, TEXT("Signed project file %s saved."), *ProjectFilePath);
		}
		else
		{
			UE_LOG(LogUpdateGameProjectCommandlet, Warning, TEXT("%s"), *LocalFailReason.ToString());
		}
	}

	if ( bAutoSubmit )
	{
		if ( !bAutoCheckout )
		{
			// We didn't init SCC above so do it now
			SourceControlProvider.Init();
		}

		if ( ISourceControlModule::Get().IsEnabled() )
		{
			const FString AbsoluteFilename = FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath());
			FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState(AbsoluteFilename, EStateCacheUsage::ForceUpdate);
			if ( SourceControlState.IsValid() && SourceControlState->IsCheckedOut() )
			{
				TSharedRef<FCheckIn, ESPMode::ThreadSafe> CheckInOperation = ISourceControlOperation::Create<FCheckIn>();
				CheckInOperation->SetDescription(ChangelistDescription);
				SourceControlProvider.Execute(CheckInOperation, AbsoluteFilename);
			}
		}
	}

	return 0;
}
