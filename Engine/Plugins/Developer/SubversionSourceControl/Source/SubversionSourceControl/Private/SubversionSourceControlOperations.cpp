// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SubversionSourceControlPrivatePCH.h"
#include "SubversionSourceControlOperations.h"
#include "SubversionSourceControlState.h"
#include "SubversionSourceControlCommand.h"
#include "SubversionSourceControlModule.h"
#include "SubversionSourceControlUtils.h"
#include "XmlParser.h"

#define LOCTEXT_NAMESPACE "SubversionSourceControl"

FName FSubversionConnectWorker::GetName() const
{
	return "Connect";
}

bool FSubversionConnectWorker::Execute(FSubversionSourceControlCommand& InCommand)
{
	TArray<FString> Parameters;
	Parameters.Add(InCommand.RepositoryName);
	
	check(InCommand.Operation->GetName() == "Connect");
	TSharedRef<FConnect, ESPMode::ThreadSafe> Operation = StaticCastSharedRef<FConnect>(InCommand.Operation);
	FString Password = InCommand.Password;
	// see if we are getting a password passed in from the calling code
	if(Operation->GetPassword().Len() > 0)
	{
		Password = Operation->GetPassword();
	}

	InCommand.bCommandSuccessful = SubversionSourceControlUtils::RunCommand(TEXT("info"), TArray<FString>(), Parameters, InCommand.InfoMessages, InCommand.ErrorMessages, InCommand.UserName, Password);

	// @todo: should we also check for the presence of a valid working copy?

	return InCommand.bCommandSuccessful;
}

bool FSubversionConnectWorker::UpdateStates() const
{
	return false;
}

FName FSubversionCheckOutWorker::GetName() const
{
	return "CheckOut";
}

bool FSubversionCheckOutWorker::Execute(FSubversionSourceControlCommand& InCommand)
{
	// @todo: we only need to lock binary files to simulate a 'checkout' state, so split files up according to type

	InCommand.bCommandSuccessful = SubversionSourceControlUtils::RunCommand(TEXT("lock"), InCommand.Files, TArray<FString>(), InCommand.InfoMessages, InCommand.ErrorMessages, InCommand.UserName);

	if(InCommand.bCommandSuccessful)
	{
		// annoyingly, we need remove any read-only flags here (for cross-working with Perforce)
		for(auto Iter(InCommand.Files.CreateConstIterator()); Iter; Iter++)
		{
			FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(**Iter, false);
		}
	}

	// now update the status of our files
	{
		TArray<FXmlFile> ResultsXml;
		TArray<FString> StatusParameters;
		StatusParameters.Add(TEXT("--show-updates"));
		StatusParameters.Add(TEXT("--verbose"));

		InCommand.bCommandSuccessful = SubversionSourceControlUtils::RunCommand(TEXT("status"), InCommand.Files, StatusParameters, ResultsXml, InCommand.ErrorMessages, InCommand.UserName);
		SubversionSourceControlUtils::ParseStatusResults(ResultsXml, InCommand.ErrorMessages, InCommand.UserName, OutStates);
	}

	return InCommand.bCommandSuccessful;
}

bool FSubversionCheckOutWorker::UpdateStates() const
{
	return SubversionSourceControlUtils::UpdateCachedStates(OutStates);
}

FName FSubversionCheckInWorker::GetName() const
{
	return "CheckIn";
}

/** Helper function for AddDirectoriesToCommit() - determines whether a direcotry is currently marked for add */
static bool IsDirectoryAdded(const FSubversionSourceControlCommand& InCommand, const FString& InDirectory)
{
	TArray<FXmlFile> ResultsXml;
	TArray<FString> ErrorMessages;
	TArray<FString> StatusParameters;
	StatusParameters.Add(TEXT("--verbose"));
	StatusParameters.Add(TEXT("--show-updates"));

	FString QuotedFilename = InDirectory;
	SubversionSourceControlUtils::QuoteFilename(QuotedFilename);

	TArray<FString> Files;
	Files.Add(QuotedFilename);

	if(SubversionSourceControlUtils::RunCommand(TEXT("status"), Files, StatusParameters, ResultsXml, ErrorMessages, InCommand.UserName))
	{
		TArray<FSubversionSourceControlState> OutStates;
		SubversionSourceControlUtils::ParseStatusResults(ResultsXml, ErrorMessages, InCommand.UserName, OutStates);
		
		for(auto It(OutStates.CreateConstIterator()); It; It++)
		{
			const FSubversionSourceControlState& State = *It;
			if(State.GetFilename() == InDirectory)
			{
				return State.IsAdded();
			}
		}
	}

	return false;
}

/** 
 * Helper function for FCheckInWorker::Execute.
 * Makes sure directories are committed with files that are also marked for add.
 * If we don't do this, the commit will fail.
 */
static void AddDirectoriesToCommit(const FSubversionSourceControlCommand& InCommand, TArray<FString>& InOutFiles)
{
	// because of the use of "--parents" when we mark for add, we can just traverse up 
	// the directory tree until we meet a directory that isn't already marked for add

	TArray<FString> Directories;

	for(auto It(InOutFiles.CreateConstIterator()); It; It++)
	{
		FString Filename = *It;
		Filename = Filename.TrimQuotes();
		FString Directory = FPaths::GetPath(Filename);

		bool bDirectoryIsAdded = false;
		do 
		{
			FString QuotedDirectory = Directory;
			SubversionSourceControlUtils::QuoteFilename(QuotedDirectory);

			bDirectoryIsAdded = false;
			if(Directories.Find(QuotedDirectory) == INDEX_NONE && IsDirectoryAdded(InCommand, Directory))
			{
				Directories.Add(QuotedDirectory);
				bDirectoryIsAdded = true;

				FString ParentDir = Directory / TEXT("../");
				FPaths::CollapseRelativeDirectories(ParentDir);
				Directory = ParentDir;
			}
		} 
		while(bDirectoryIsAdded);
	}

	InOutFiles.Append(Directories);
}

bool FSubversionCheckInWorker::Execute(FSubversionSourceControlCommand& InCommand)
{
	check(InCommand.Operation->GetName() == "CheckIn");
	TSharedRef<FCheckIn, ESPMode::ThreadSafe> Operation = StaticCastSharedRef<FCheckIn>(InCommand.Operation);
	{
		// make a temp file to place our message in
		FScopedTempFile DescriptionFile(Operation->GetDescription());
		if(DescriptionFile.GetFilename().Len() > 0)
		{
			TArray<FString> Parameters;
			FString DescriptionFilename = DescriptionFile.GetFilename();
			SubversionSourceControlUtils::QuoteFilename(DescriptionFilename);
			Parameters.Add(FString(TEXT("--file ")) + DescriptionFilename);

			if(DescriptionFile.IsUnicode())
			{
				Parameters.Add(TEXT("--encoding utf-8"));
			}

			// we need commit directories that are marked for add here if we are committing any child files that are also marked for add
			TArray<FString> FilesToCommit = InCommand.Files;
			AddDirectoriesToCommit(InCommand, FilesToCommit);

			// we need another temp file to add our file list to (as this must be an atomic operation we cant risk overflowing command-line limits)
			FString Targets;
			for(auto It(FilesToCommit.CreateConstIterator()); It; It++)
			{
				FString Target = It->TrimQuotes();
				Targets += Target + LINE_TERMINATOR;
			}

			FScopedTempFile TargetsFile(Targets);
			if(TargetsFile.GetFilename().Len() > 0)
			{
				FString TargetsFilename = TargetsFile.GetFilename();
				SubversionSourceControlUtils::QuoteFilename(TargetsFilename);
				Parameters.Add(FString(TEXT("--targets ")) + TargetsFilename);

				InCommand.bCommandSuccessful = SubversionSourceControlUtils::RunAtomicCommand(TEXT("commit"), TArray<FString>(), Parameters, InCommand.InfoMessages, InCommand.ErrorMessages, InCommand.UserName);
			}
		}
	}

	// now update the status of our files
	{
		TArray<FXmlFile> ResultsXml;
		TArray<FString> StatusParameters;
		StatusParameters.Add(TEXT("--verbose"));
		StatusParameters.Add(TEXT("--show-updates"));

		InCommand.bCommandSuccessful &= SubversionSourceControlUtils::RunCommand(TEXT("status"), InCommand.Files, StatusParameters, ResultsXml, InCommand.ErrorMessages, InCommand.UserName);
		SubversionSourceControlUtils::ParseStatusResults(ResultsXml, InCommand.ErrorMessages, InCommand.UserName, OutStates);
	}

	return InCommand.bCommandSuccessful;
}

bool FSubversionCheckInWorker::UpdateStates() const
{
	return SubversionSourceControlUtils::UpdateCachedStates(OutStates);
}

FName FSubversionMarkForAddWorker::GetName() const
{
	return "MarkForAdd";
}

bool FSubversionMarkForAddWorker::Execute(FSubversionSourceControlCommand& InCommand)
{
	TArray<FString> Parameters;
	// Make sure we add files if we encounter one that has already been added.
	Parameters.Add(TEXT("--force"));
	// Add nonexistent/non-versioned parent directories too
	Parameters.Add(TEXT("--parents"));

	InCommand.bCommandSuccessful = SubversionSourceControlUtils::RunCommand(TEXT("add"), InCommand.Files, Parameters, InCommand.InfoMessages, InCommand.ErrorMessages, InCommand.UserName);

	// now update the status of our files
	{
		TArray<FXmlFile> ResultsXml;
		TArray<FString> StatusParameters;
		StatusParameters.Add(TEXT("--verbose"));

		InCommand.bCommandSuccessful &= SubversionSourceControlUtils::RunCommand(TEXT("status"), InCommand.Files, StatusParameters, ResultsXml, InCommand.ErrorMessages, InCommand.UserName);
		SubversionSourceControlUtils::ParseStatusResults(ResultsXml, InCommand.ErrorMessages, InCommand.UserName, OutStates);
	}

	return InCommand.bCommandSuccessful;
}

bool FSubversionMarkForAddWorker::UpdateStates() const
{
	return SubversionSourceControlUtils::UpdateCachedStates(OutStates);
}

FName FSubversionDeleteWorker::GetName() const
{
	return "Delete";
}

bool FSubversionDeleteWorker::Execute(FSubversionSourceControlCommand& InCommand)
{
	TArray<FString> Parameters;

	InCommand.bCommandSuccessful = SubversionSourceControlUtils::RunCommand(TEXT("delete"), InCommand.Files, Parameters, InCommand.InfoMessages, InCommand.ErrorMessages, InCommand.UserName);

	// now update the status of our files
	{
		TArray<FXmlFile> ResultsXml;
		TArray<FString> StatusParameters;
		StatusParameters.Add(TEXT("--verbose"));

		InCommand.bCommandSuccessful &= SubversionSourceControlUtils::RunCommand(TEXT("status"), InCommand.Files, StatusParameters, ResultsXml, InCommand.ErrorMessages, InCommand.UserName);
		SubversionSourceControlUtils::ParseStatusResults(ResultsXml, InCommand.ErrorMessages, InCommand.UserName, OutStates);
	}

	return InCommand.bCommandSuccessful;
}

bool FSubversionDeleteWorker::UpdateStates() const
{
	return SubversionSourceControlUtils::UpdateCachedStates(OutStates);
}

FName FSubversionRevertWorker::GetName() const
{
	return "Revert";
}

bool FSubversionRevertWorker::Execute(FSubversionSourceControlCommand& InCommand)
{
	// revert any changes
	{
		TArray<FString> Parameters;
		InCommand.bCommandSuccessful = SubversionSourceControlUtils::RunCommand(TEXT("revert"), InCommand.Files, Parameters, InCommand.InfoMessages, InCommand.ErrorMessages, InCommand.UserName);
	}

	// unlock any files
	{
		TArray<FString> Parameters;
		InCommand.bCommandSuccessful &= SubversionSourceControlUtils::RunCommand(TEXT("unlock"), InCommand.Files, Parameters, InCommand.InfoMessages, InCommand.ErrorMessages, InCommand.UserName);
	}

	// now update the status of our files
	{
		TArray<FXmlFile> ResultsXml;
		TArray<FString> StatusParameters;
		StatusParameters.Add(TEXT("--verbose"));
		StatusParameters.Add(TEXT("--show-updates"));

		InCommand.bCommandSuccessful &= SubversionSourceControlUtils::RunCommand(TEXT("status"), InCommand.Files, StatusParameters, ResultsXml, InCommand.ErrorMessages, InCommand.UserName);
		SubversionSourceControlUtils::ParseStatusResults(ResultsXml, InCommand.ErrorMessages, InCommand.UserName, OutStates);
	}

	return InCommand.bCommandSuccessful;
}

bool FSubversionRevertWorker::UpdateStates() const
{
	return SubversionSourceControlUtils::UpdateCachedStates(OutStates);
}

FName FSubversionSyncWorker::GetName() const
{
	return "Sync";
}

bool FSubversionSyncWorker::Execute(FSubversionSourceControlCommand& InCommand)
{
	InCommand.bCommandSuccessful = SubversionSourceControlUtils::RunCommand(TEXT("update"), InCommand.Files, TArray<FString>(), InCommand.InfoMessages, InCommand.ErrorMessages, InCommand.UserName);

	// now update the status of our files
	{
		TArray<FXmlFile> ResultsXml;
		TArray<FString> StatusParameters;
		StatusParameters.Add(TEXT("--verbose"));
		StatusParameters.Add(TEXT("--show-updates"));

		InCommand.bCommandSuccessful &= SubversionSourceControlUtils::RunCommand(TEXT("status"), InCommand.Files, StatusParameters, ResultsXml, InCommand.ErrorMessages, InCommand.UserName);
		SubversionSourceControlUtils::ParseStatusResults(ResultsXml, InCommand.ErrorMessages, InCommand.UserName, OutStates);
	}

	return InCommand.bCommandSuccessful;
}

bool FSubversionSyncWorker::UpdateStates() const
{
	return SubversionSourceControlUtils::UpdateCachedStates(OutStates);
}

FName FSubversionUpdateStatusWorker::GetName() const
{
	return "UpdateStatus";
}

bool FSubversionUpdateStatusWorker::Execute(FSubversionSourceControlCommand& InCommand)
{
	if(InCommand.Files.Num() > 0)
	{
		TArray<FXmlFile> ResultsXml;
		TArray<FString> Parameters;
		Parameters.Add(TEXT("--show-updates"));
		Parameters.Add(TEXT("--verbose"));

		InCommand.bCommandSuccessful = SubversionSourceControlUtils::RunCommand(TEXT("status"), InCommand.Files, Parameters, ResultsXml, InCommand.ErrorMessages, InCommand.UserName);
		SubversionSourceControlUtils::ParseStatusResults(ResultsXml, InCommand.ErrorMessages, InCommand.UserName, OutStates);
		SubversionSourceControlUtils::RemoveRedundantErrors(InCommand, TEXT("' is not a working copy"));
	}
	else
	{
		InCommand.bCommandSuccessful = true;
	}

	// update using any special hints passed in via the operation
	check(InCommand.Operation->GetName() == "UpdateStatus");
	TSharedRef<FUpdateStatus, ESPMode::ThreadSafe> Operation = StaticCastSharedRef<FUpdateStatus>(InCommand.Operation);

	if(Operation->ShouldUpdateHistory())
	{
		for(auto Iter(InCommand.Files.CreateConstIterator()); Iter; Iter++)
		{
			TArray<FXmlFile> ResultsXml;
			TArray<FString> Parameters;

			//limit to last 100 changes
			Parameters.Add(TEXT("--limit 100"));
			// output all properties
			Parameters.Add(TEXT("--with-all-revprops"));
			// we want to view over merge boundaries
			Parameters.Add(TEXT("--use-merge-history"));
			// we want all the output!
			Parameters.Add(TEXT("--verbose"));

			TArray<FString> Files;
			Files.Add(*Iter);

			InCommand.bCommandSuccessful &= SubversionSourceControlUtils::RunCommand(TEXT("log"), Files, Parameters, ResultsXml, InCommand.ErrorMessages, InCommand.UserName);
			SubversionSourceControlUtils::ParseLogResults(Iter->TrimQuotes(), ResultsXml, InCommand.UserName, OutHistory);
		}
	}

	if(Operation->ShouldGetOpenedOnly())
	{
		// No server access if we use this option (note no "--show-updates" param)

		TArray<FXmlFile> ResultsXml;
		TArray<FString> Parameters;
		Parameters.Add(TEXT("--verbose"));

		TArray<FString> Files;
		Files.Add(FPaths::RootDir());

		InCommand.bCommandSuccessful &= SubversionSourceControlUtils::RunCommand(TEXT("status"), Files, Parameters, ResultsXml, InCommand.ErrorMessages, InCommand.UserName);
		SubversionSourceControlUtils::ParseStatusResults(ResultsXml, InCommand.ErrorMessages, InCommand.UserName, OutStates);
	}

	// NOTE: we dont use the ShouldUpdateModifiedState() hint here as a normal svn status will tell us this information

	return InCommand.bCommandSuccessful;
}

bool FSubversionUpdateStatusWorker::UpdateStates() const
{
	bool bUpdated = false;

	FSubversionSourceControlModule& SubversionSourceControl = FModuleManager::LoadModuleChecked<FSubversionSourceControlModule>( "SubversionSourceControl" );
	FSubversionSourceControlProvider& Provider = SubversionSourceControl.GetProvider();

	bUpdated |= SubversionSourceControlUtils::UpdateCachedStates(OutStates);

	// add history, if any
	for(SubversionSourceControlUtils::FHistoryOutput::TConstIterator It(OutHistory); It; ++It)
	{
		TSharedRef<FSubversionSourceControlState, ESPMode::ThreadSafe> State = Provider.GetStateInternal(It.Key());
		State->History = It.Value();
		State->TimeStamp = FDateTime::Now();
		bUpdated = true;
	}

	return bUpdated;
}

#undef LOCTEXT_NAMESPACE
