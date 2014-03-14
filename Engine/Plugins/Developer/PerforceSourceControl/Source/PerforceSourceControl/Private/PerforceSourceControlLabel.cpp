// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "PerforceSourceControlPrivatePCH.h"
#include "PerforceSourceControlLabel.h"
#include "PerforceSourceControlRevision.h"
#include "PerforceSourceControlModule.h"
#include "PerforceSourceControlProvider.h"
#include "PerforceConnection.h"
#include "MessageLog.h"

const FString& FPerforceSourceControlLabel::GetName() const
{
	return Name;
}	

static void ParseFilesResults(const FP4RecordSet& InRecords, TArray< TSharedRef<ISourceControlRevision, ESPMode::ThreadSafe> >& OutRevisions, const FString& InClientRoot)
{
	// Iterate over each record found as a result of the command, parsing it for relevant information
	for (int32 Index = 0; Index < InRecords.Num(); ++Index)
	{
		const FP4Record& ClientRecord = InRecords[Index];
		FString DepotFile = ClientRecord(TEXT("depotFile"));
		FString RevisionNumber = ClientRecord(TEXT("rev"));
		FString Date = ClientRecord(TEXT("time"));
		FString ChangelistNumber = ClientRecord(TEXT("change"));
		FString Action = ClientRecord(TEXT("action"));
		check(RevisionNumber.Len() != 0);

		// @todo: this revision is incomplete, but is sufficient for now given the usage of labels to get files, rather
		// than review revision histories.
		TSharedRef<FPerforceSourceControlRevision, ESPMode::ThreadSafe> Revision = MakeShareable( new FPerforceSourceControlRevision() );
		Revision->FileName = DepotFile;
		Revision->RevisionNumber = FCString::Atoi(*RevisionNumber);
		Revision->ChangelistNumber = FCString::Atoi(*ChangelistNumber);
		Revision->Action = Action;
		Revision->Date = FDateTime(1970, 1, 1, 0, 0, 0, 0) + FTimespan(0, 0, FCString::Atoi(*Date));

		OutRevisions.Add(Revision);
	}
}

bool FPerforceSourceControlLabel::GetFileRevisions( const TArray<FString>& InFiles, TArray< TSharedRef<ISourceControlRevision, ESPMode::ThreadSafe> >& OutRevisions ) const
{
	bool bCommandOK = false;

	FPerforceSourceControlModule& PerforceSourceControl = FModuleManager::LoadModuleChecked<FPerforceSourceControlModule>("PerforceSourceControl");
	FPerforceSourceControlProvider& Provider = PerforceSourceControl.GetProvider();
	FScopedPerforceConnection ScopedConnection(EConcurrency::Synchronous, Provider.GetPort(), Provider.GetUser(), Provider.GetClientSpec(), Provider.GetTicket());
	if(ScopedConnection.IsValid())
	{
		FPerforceConnection& Connection = ScopedConnection.GetConnection();
		FP4RecordSet Records;
		TArray<FString> Parameters;
		TArray<FString> ErrorMessages;
		for(auto Iter(InFiles.CreateConstIterator()); Iter; Iter++)
		{
			Parameters.Add(*Iter + TEXT("@") + Name);
		}
		bool bConnectionDropped = false;
		bCommandOK = Connection.RunCommand(TEXT("files"), Parameters, Records, ErrorMessages, FOnIsCancelled(), bConnectionDropped);
		if(bCommandOK)
		{
			ParseFilesResults(Records, OutRevisions, Connection.ClientRoot);
		}
		else
		{
			// output errors if any
			for (int32 ErrorIndex = 0; ErrorIndex < ErrorMessages.Num(); ++ErrorIndex)
			{
				FMessageLog("SourceControl").Error(FText::FromString(ErrorMessages[ErrorIndex]));
			}
		}
	}

	return bCommandOK;
}

bool FPerforceSourceControlLabel::Sync( const FString& InFilename ) const
{
	bool bCommandOK = false;

	FPerforceSourceControlModule& PerforceSourceControl = FModuleManager::LoadModuleChecked<FPerforceSourceControlModule>("PerforceSourceControl");
	FPerforceSourceControlProvider& Provider = PerforceSourceControl.GetProvider();
	FScopedPerforceConnection ScopedConnection(EConcurrency::Synchronous, Provider.GetPort(), Provider.GetUser(), Provider.GetClientSpec(), Provider.GetTicket());
	if(ScopedConnection.IsValid())
	{
		FPerforceConnection& Connection = ScopedConnection.GetConnection();
		FP4RecordSet Records;
		TArray<FString> Parameters;
		TArray<FString> ErrorMessages;
		Parameters.Add(InFilename + TEXT("@") + Name);
		bool bConnectionDropped = false;
		bCommandOK = Connection.RunCommand(TEXT("sync"), Parameters, Records, ErrorMessages, FOnIsCancelled(), bConnectionDropped);
		if(!bCommandOK)
		{
			// output errors if any
			for (int32 ErrorIndex = 0; ErrorIndex < ErrorMessages.Num(); ++ErrorIndex)
			{
				FMessageLog("SourceControl").Error(FText::FromString(ErrorMessages[ErrorIndex]));
			}
		}
	}

	return bCommandOK;
}