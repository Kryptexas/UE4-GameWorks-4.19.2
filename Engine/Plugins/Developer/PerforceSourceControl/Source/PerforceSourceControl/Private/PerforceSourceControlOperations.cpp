// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "PerforceSourceControlPrivatePCH.h"
#include "PerforceSourceControlOperations.h"
#include "PerforceSourceControlState.h"
#include "PerforceSourceControlCommand.h"
#include "PerforceConnection.h"
#include "PerforceSourceControlModule.h"
#include "PerforceSourceControlRevision.h"
#include "SPerforceSourceControlSettings.h"

#define LOCTEXT_NAMESPACE "PerforceSourceControl"

/**
 * Helper struct for RemoveRedundantErrors()
 */
struct FRemoveRedundantErrors
{
	FRemoveRedundantErrors(const FString& InFilter)
		: Filter(InFilter)
	{
	}

	bool operator()(const FString& String) const
	{
		if(String.Contains(Filter))
		{
			return true;
		}

		return false;
	}

	/** The filter string we try to identify in the reported error */
	FString Filter;
};

/** 
 * Remove redundant errors (that contain a particular string) and also
 * update the commands success status if all errors were removed.
 */
static void RemoveRedundantErrors(FPerforceSourceControlCommand& InCommand, const FString& InFilter)
{
	bool bFoundRedundantError = false;
	for(auto Iter(InCommand.ErrorMessages.CreateConstIterator()); Iter; Iter++)
	{
		// Perforce reports files that are already synced as errors, so copy any errors
		// we get to the info list in this case
		if(Iter->Contains(InFilter))
		{
			InCommand.InfoMessages.Add(*Iter);
			bFoundRedundantError = true;
		}
	}

	InCommand.ErrorMessages.RemoveAll( FRemoveRedundantErrors(InFilter) );

	// if we have no error messages now, assume success!
	if(bFoundRedundantError && InCommand.ErrorMessages.Num() == 0 && !InCommand.bCommandSuccessful)
	{
		InCommand.bCommandSuccessful = true;
	}
}

/** Simple parsing of a record set into strings, one string per record */
static void ParseRecordSet(const FP4RecordSet& InRecords, TArray<FString>& OutResults)
{
	const FString Delimiter = FString(TEXT(" "));

	for (int32 RecordIndex = 0; RecordIndex < InRecords.Num(); ++RecordIndex)
	{
		const FP4Record& ClientRecord = InRecords[RecordIndex];
		for(FP4Record::TConstIterator It = ClientRecord.CreateConstIterator(); It; ++It)
		{
			OutResults.Add(It.Key() + Delimiter + It.Value());
		}
	}
}

/** Simple parsing of a record set to update state */
static void ParseRecordSetForState(const FP4RecordSet& InRecords, TMap<FString, EPerforceState::Type>& OutResults)
{
	// Iterate over each record found as a result of the command, parsing it for relevant information
	for (int32 Index = 0; Index < InRecords.Num(); ++Index)
	{
		const FP4Record& ClientRecord = InRecords[Index];
		FString FileName = ClientRecord(TEXT("clientFile"));
		FString Action = ClientRecord(TEXT("action"));

		check(FileName.Len());
		FString FullPath(FileName);
		FPaths::NormalizeFilename(FullPath);

		if(Action.Len() > 0)
		{
			if(Action == TEXT("add"))
			{
				OutResults.Add(FullPath, EPerforceState::OpenForAdd);
			}
			else if(Action == TEXT("edit"))
			{
				OutResults.Add(FullPath, EPerforceState::CheckedOut);
			}
			else if(Action == TEXT("delete"))
			{
				OutResults.Add(FullPath, EPerforceState::MarkedForDelete);
			}
			else if(Action == TEXT("abandoned"))
			{
				OutResults.Add(FullPath, EPerforceState::NotInDepot);
			}
			else if(Action == TEXT("reverted"))
			{
				FString OldAction = ClientRecord(TEXT("oldAction"));
				if(OldAction == TEXT("add"))
				{
					OutResults.Add(FullPath, EPerforceState::NotInDepot);
				}
				else if(OldAction == TEXT("edit"))
				{
					OutResults.Add(FullPath, EPerforceState::ReadOnly);
				}
			}
		}
	}
}

static bool UpdateCachedStates(const TMap<FString, EPerforceState::Type>& InResults)
{
	FPerforceSourceControlModule& PerforceSourceControl = FModuleManager::LoadModuleChecked<FPerforceSourceControlModule>("PerforceSourceControl");
	for(TMap<FString, EPerforceState::Type>::TConstIterator It(InResults); It; ++It)
	{
		TSharedRef<FPerforceSourceControlState, ESPMode::ThreadSafe> State = PerforceSourceControl.GetProvider().GetStateInternal(It.Key());
		State->SetState(It.Value());
		State->TimeStamp = FDateTime::Now();
	}

	return InResults.Num() > 0;
}

FName FPerforceConnectWorker::GetName() const
{
	return "Connect";
}

bool FPerforceConnectWorker::Execute(FPerforceSourceControlCommand& InCommand)
{
	FScopedPerforceConnection ScopedConnection(InCommand);
	if(ScopedConnection.IsValid())
	{
		FPerforceConnection& Connection = ScopedConnection.GetConnection();
		TArray<FString> Parameters;
		FP4RecordSet Records;
		Parameters.Add(TEXT("-o"));
		Parameters.Add(InCommand.ClientSpec);
		InCommand.bCommandSuccessful = Connection.RunCommand(TEXT("client"), Parameters, Records, InCommand.ErrorMessages, FOnIsCancelled::CreateRaw(&InCommand, &FPerforceSourceControlCommand::IsCanceled), InCommand.bConnectionDropped);
		
		// If there are error messages, user name is most likely invalid. Otherwise, make sure workspace actually
		// exists on server by checking if we have it's update date.
		InCommand.bCommandSuccessful &= InCommand.ErrorMessages.Num() == 0 && Records.Num() > 0 && Records[0].Contains(TEXT("Update"));
		if (!InCommand.bCommandSuccessful && InCommand.ErrorMessages.Num() == 0)
		{
			InCommand.ErrorMessages.Add(TEXT("Invalid workspace."));
		}

		if(InCommand.bCommandSuccessful)
		{
			ParseRecordSet(Records, InCommand.InfoMessages);
		}
	}
	return InCommand.bCommandSuccessful;
}

bool FPerforceConnectWorker::UpdateStates() const
{
	return false;
}

FName FPerforceCheckOutWorker::GetName() const
{
	return "CheckOut";
}

bool FPerforceCheckOutWorker::Execute(FPerforceSourceControlCommand& InCommand)
{	
	FScopedPerforceConnection ScopedConnection(InCommand);
	if(ScopedConnection.IsValid())
	{
		FPerforceConnection& Connection = ScopedConnection.GetConnection();
		TArray<FString> Parameters;
		Parameters.Append(InCommand.Files);
		FP4RecordSet Records;
		InCommand.bCommandSuccessful = Connection.RunCommand(TEXT("edit"), Parameters, Records, InCommand.ErrorMessages, FOnIsCancelled::CreateRaw(&InCommand, &FPerforceSourceControlCommand::IsCanceled), InCommand.bConnectionDropped);
		ParseRecordSetForState(Records, OutResults);
	}

	return InCommand.bCommandSuccessful;
}

bool FPerforceCheckOutWorker::UpdateStates() const
{
	return UpdateCachedStates(OutResults);
}

FName FPerforceCheckInWorker::GetName() const
{
	return "CheckIn";
}

bool FPerforceCheckInWorker::Execute(FPerforceSourceControlCommand& InCommand)
{
	FScopedPerforceConnection ScopedConnection(InCommand);
	if(ScopedConnection.IsValid())
	{	
		FPerforceConnection& Connection = ScopedConnection.GetConnection();

		check(InCommand.Operation->GetName() == "CheckIn");
		TSharedRef<FCheckIn, ESPMode::ThreadSafe> Operation = StaticCastSharedRef<FCheckIn>(InCommand.Operation);

		int32 ChangeList = Connection.CreatePendingChangelist(Operation->GetDescription(), FOnIsCancelled::CreateRaw(&InCommand, &FPerforceSourceControlCommand::IsCanceled), InCommand.ErrorMessages);
		if (ChangeList > 0)
		{
			FP4RecordSet Records;

			// Batch reopen into multiple commands, to avoid command line limits
			const int32 BatchedCount = 100;
			InCommand.bCommandSuccessful = true;
			for (int32 StartingIndex = 0; StartingIndex < InCommand.Files.Num() && InCommand.bCommandSuccessful; StartingIndex += BatchedCount)
			{
				TArray< FString > ReopenParams;
						
				//Add changelist information to params
				ReopenParams.Insert(TEXT("-c"), 0);
				ReopenParams.Insert(FString::Printf(TEXT("%d"), ChangeList), 1);
				int32 NextIndex = FMath::Min(StartingIndex + BatchedCount, InCommand.Files.Num());

				for (int32 FileIndex = StartingIndex; FileIndex < NextIndex; FileIndex++)
				{
					ReopenParams.Add(InCommand.Files[FileIndex]);
				}

				InCommand.bCommandSuccessful = Connection.RunCommand(TEXT("reopen"), ReopenParams, Records, InCommand.ErrorMessages, FOnIsCancelled::CreateRaw(&InCommand, &FPerforceSourceControlCommand::IsCanceled), InCommand.bConnectionDropped);
			}

			if (InCommand.bCommandSuccessful)
			{
				// Only submit if reopen was successful
				InCommand.bCommandSuccessful = Connection.SubmitChangelist(ChangeList, FOnIsCancelled::CreateRaw(&InCommand, &FPerforceSourceControlCommand::IsCanceled), InCommand.ErrorMessages);

				if(InCommand.bCommandSuccessful)
				{
					for(auto Iter(InCommand.Files.CreateIterator()); Iter; Iter++)
					{
						OutResults.Add(*Iter, EPerforceState::ReadOnly);
					}
				}
			}
		}
		else
		{
			// Failed to create the changelist
			InCommand.bCommandSuccessful = false;
		}
	}

	return InCommand.bCommandSuccessful;
}

bool FPerforceCheckInWorker::UpdateStates() const
{
	return UpdateCachedStates(OutResults);
}

FName FPerforceMarkForAddWorker::GetName() const
{
	return "MarkForAdd";
}

static bool ShouldForceBinaryAdd(const TArray<FString>& FilesToAdd)
{
	bool bForceBinary = false;

	if ( FilesToAdd.Num() > 0 )
	{
		// Force a binary type when adding if all files are .uasset files and at least one does not exist on disk.
		// If a .uasset file is marked for add before it exists on disk, it will be added to perforce in text format.
		// This will corrupt the file and cause a crash at startup when scanning the file with the asset registry.
		// If the user is adding one or more uasset files and any are missing it will force binary type for all of them.
		bool bAllFilesUAsset = true;
		bool bAtLeastOneFileDoesNotExist = false;
		for ( int32 ParamIdx = 0; ParamIdx < FilesToAdd.Num(); ++ParamIdx )
		{
			const FString& Param = FilesToAdd[ParamIdx];
			if ( FPaths::GetExtension(Param, true).ToLower() == FPackageName::GetAssetPackageExtension().ToLower() )
			{
				if ( !ensureMsgf(FPaths::FileExists(Param), TEXT("Attempting to add a uasset file '%s' to Perforce that does not yet exist."), *Param) )
				{
					// This is a uasset file that does not yet exist on disk. This is invalid, find out why this is happening.
					bAtLeastOneFileDoesNotExist = true;
				}
			}
			else
			{
				bAllFilesUAsset = false;
			}
		}

		bForceBinary = bAllFilesUAsset && bAtLeastOneFileDoesNotExist;
	}

	return bForceBinary;
}

bool FPerforceMarkForAddWorker::Execute(FPerforceSourceControlCommand& InCommand)
{
	FScopedPerforceConnection ScopedConnection(InCommand);
	if(ScopedConnection.IsValid())
	{
		FPerforceConnection& Connection = ScopedConnection.GetConnection();
		TArray<FString> Parameters;
		FP4RecordSet Records;
		if ( ShouldForceBinaryAdd(InCommand.Files) )
		{
			Parameters.Add(TEXT("-t binary+l"));
		}
		Parameters.Append(InCommand.Files);
		InCommand.bCommandSuccessful = Connection.RunCommand(TEXT("add"), Parameters, Records, InCommand.ErrorMessages, FOnIsCancelled::CreateRaw(&InCommand, &FPerforceSourceControlCommand::IsCanceled), InCommand.bConnectionDropped);
		ParseRecordSetForState(Records, OutResults);
	}
	return InCommand.bCommandSuccessful;
}

bool FPerforceMarkForAddWorker::UpdateStates() const
{
	return UpdateCachedStates(OutResults);
}

FName FPerforceDeleteWorker::GetName() const
{
	return "Delete";
}

bool FPerforceDeleteWorker::Execute(FPerforceSourceControlCommand& InCommand)
{
	FScopedPerforceConnection ScopedConnection(InCommand);
	if(ScopedConnection.IsValid())
	{
		FPerforceConnection& Connection = ScopedConnection.GetConnection();
		TArray<FString> Parameters;
		Parameters.Append(InCommand.Files);
		FP4RecordSet Records;
		InCommand.bCommandSuccessful = Connection.RunCommand(TEXT("delete"), Parameters, Records, InCommand.ErrorMessages, FOnIsCancelled::CreateRaw(&InCommand, &FPerforceSourceControlCommand::IsCanceled), InCommand.bConnectionDropped);
		ParseRecordSetForState(Records, OutResults);
	}
	return InCommand.bCommandSuccessful;
}

bool FPerforceDeleteWorker::UpdateStates() const
{
	return UpdateCachedStates(OutResults);
}

FName FPerforceRevertWorker::GetName() const
{
	return "Revert";
}

bool FPerforceRevertWorker::Execute(FPerforceSourceControlCommand& InCommand)
{
	FScopedPerforceConnection ScopedConnection(InCommand);
	if(ScopedConnection.IsValid())
	{
		FPerforceConnection& Connection = ScopedConnection.GetConnection();
		TArray<FString> Parameters;
		Parameters.Append(InCommand.Files);
		FP4RecordSet Records;
		InCommand.bCommandSuccessful = Connection.RunCommand(TEXT("revert"), Parameters, Records, InCommand.ErrorMessages, FOnIsCancelled::CreateRaw(&InCommand, &FPerforceSourceControlCommand::IsCanceled), InCommand.bConnectionDropped);
		ParseRecordSetForState(Records, OutResults);
	}
	return InCommand.bCommandSuccessful;
}

bool FPerforceRevertWorker::UpdateStates() const
{
	return UpdateCachedStates(OutResults);
}

FName FPerforceSyncWorker::GetName() const
{
	return "Sync";
}

static void ParseSyncResults(const FP4RecordSet& InRecords, TMap<FString, EPerforceState::Type>& OutResults)
{
	// Iterate over each record found as a result of the command, parsing it for relevant information
	for (int32 Index = 0; Index < InRecords.Num(); ++Index)
	{
		const FP4Record& ClientRecord = InRecords[Index];
		FString FileName = ClientRecord(TEXT("clientFile"));
		FString Action = ClientRecord(TEXT("action"));

		check(FileName.Len());
		FString FullPath(FileName);
		FPaths::NormalizeFilename(FullPath);

		if(Action.Len() > 0)
		{
			if(Action == TEXT("updated"))
			{
				OutResults.Add(FullPath, EPerforceState::ReadOnly);
			}
		}
	}
}

bool FPerforceSyncWorker::Execute(FPerforceSourceControlCommand& InCommand)
{
	FScopedPerforceConnection ScopedConnection(InCommand);
	if(ScopedConnection.IsValid())
	{
		FPerforceConnection& Connection = ScopedConnection.GetConnection();
		TArray<FString> Parameters;
		Parameters.Append(InCommand.Files);
		FP4RecordSet Records;
		InCommand.bCommandSuccessful = Connection.RunCommand(TEXT("sync"), Parameters, Records, InCommand.ErrorMessages, FOnIsCancelled::CreateRaw(&InCommand, &FPerforceSourceControlCommand::IsCanceled), InCommand.bConnectionDropped);
		ParseSyncResults(Records, OutResults);

		RemoveRedundantErrors(InCommand, TEXT("file(s) up-to-date"));
	}
	return InCommand.bCommandSuccessful;
}

bool FPerforceSyncWorker::UpdateStates() const
{
	return UpdateCachedStates(OutResults);
}

static void ParseUpdateStatusResults(const FP4RecordSet& InRecords, const TArray<FString>& ErrorMessages, TArray<FPerforceSourceControlState>& OutStates)
{
	// Iterate over each record found as a result of the command, parsing it for relevant information
	for (int32 Index = 0; Index < InRecords.Num(); ++Index)
	{
		const FP4Record& ClientRecord = InRecords[Index];
		FString FileName = ClientRecord(TEXT("clientFile"));
		FString DepotFileName = ClientRecord(TEXT("depotFile"));
		FString HeadRev  = ClientRecord(TEXT("headRev"));
		FString HaveRev  = ClientRecord(TEXT("haveRev"));
		FString OtherOpen = ClientRecord(TEXT("otherOpen"));
		FString OpenType = ClientRecord(TEXT("type"));
		FString HeadAction = ClientRecord(TEXT("headAction"));
		FString Action = ClientRecord(TEXT("action"));
		FString HeadType = ClientRecord(TEXT("headType"));

		FString FullPath(FileName);
		FPaths::NormalizeFilename(FullPath);
		OutStates.Add(FPerforceSourceControlState(FullPath));
		FPerforceSourceControlState& State = OutStates.Last();
		State.DepotFilename = DepotFileName;

		State.State = EPerforceState::ReadOnly;
		if (Action.Len() > 0 && Action == TEXT("add")) 
		{
			State.State = EPerforceState::OpenForAdd;
		}
		else if (Action.Len() > 0 && Action == TEXT("delete")) 
		{
			State.State = EPerforceState::MarkedForDelete;
		}
		else if (OpenType.Len() > 0)
		{
			State.State = EPerforceState::CheckedOut;
		}
		else if (OtherOpen.Len() > 0)
		{
			// OtherOpen just reports the number of developers that have a file open, now add a string for every entry
			int32 OtherOpenNum = FCString::Atoi(*OtherOpen);
			for ( int32 OpenIdx = 0; OpenIdx < OtherOpenNum; ++OpenIdx )
			{
				const FString OtherOpenRecordKey = FString::Printf(TEXT("otherOpen%d"), OpenIdx);
				const FString OtherOpenRecordValue = ClientRecord(OtherOpenRecordKey);

				State.OtherUserCheckedOut += OtherOpenRecordValue;
				if(OpenIdx < OtherOpenNum - 1)
				{
					State.OtherUserCheckedOut += TEXT(", ");
				}
			}

			State.State = EPerforceState::CheckedOutOther;
		}
		else if (HeadRev.Len() > 0 && HaveRev.Len() > 0 && HaveRev != HeadRev)
		{
			State.State = EPerforceState::NotCurrent;
		}
		//file has been previously deleted, ok to add again
		else if (HeadAction.Len() > 0 && HeadAction == TEXT("delete")) 
		{
			State.State = EPerforceState::NotInDepot;
		}

		// Check binary status
		State.bBinary = false;
		if (HeadType.Len() > 0 && HeadType.Contains(TEXT("binary")))
		{
			State.bBinary = true;
		}

		// Check exclusive checkout flag
		State.bExclusiveCheckout = false;
		if (HeadType.Len() > 0 && HeadType.Contains(TEXT("+l")))
		{
			State.bExclusiveCheckout = true;
		}
	}

	// also see if we can glean anything from the error messages
	for (int32 Index = 0; Index < ErrorMessages.Num(); ++Index)
	{
		const FString& Error = ErrorMessages[Index];
		int32 TruncatePos = Error.Find(TEXT(" - no such file(s).\n"), ESearchCase::IgnoreCase, ESearchDir::FromStart);
		if(TruncatePos != INDEX_NONE)
		{
			// found an error about a file that is not in the depot
			FString FullPath(Error.Left(TruncatePos));
			FPaths::NormalizeFilename(FullPath);
			OutStates.Add(FPerforceSourceControlState(FullPath));
			FPerforceSourceControlState& State = OutStates.Last();
			State.State = EPerforceState::NotInDepot;
		}
	}
}

static void ParseOpenedResults(const FP4RecordSet& InRecords, const FString& ClientName, const FString& ClientRoot, TMap<FString, EPerforceState::Type>& OutResults)
{
	// Iterate over each record found as a result of the command, parsing it for relevant information
	for (int32 Index = 0; Index < InRecords.Num(); ++Index)
	{
		const FP4Record& ClientRecord = InRecords[Index];
		FString ClientFileName = ClientRecord(TEXT("clientFile"));
		FString Action = ClientRecord(TEXT("action"));

		check(ClientFileName.Len() > 0);

		// Convert the depot file name to a local file name
		FString FullPath = ClientFileName;
		const FString PathRoot = FString::Printf(TEXT("//%s"), *ClientName);

		if (FullPath.StartsWith(PathRoot))
		{
			const bool bIsNullClientRootPath = (ClientRoot == TEXT("null"));
			if ( bIsNullClientRootPath )
			{
				// Null clients use the pattern in PathRoot: //Workspace/FileName
				// Here we chop off the '//Workspace/' to return the workspace filename
				FullPath = FullPath.RightChop(PathRoot.Len() + 1);
			}
			else
			{
				// This is a normal workspace where we can simply replace the pathroot with the client root to form the filename
				FullPath = FullPath.Replace(*PathRoot, *ClientRoot);
			}
		}
		else
		{
			// This file is not in the workspace, ignore it
			continue;
		}

		if (Action.Len() > 0)
		{
			if(Action == TEXT("add"))
			{
				OutResults.Add(FullPath, EPerforceState::OpenForAdd);
			}
			else if(Action == TEXT("edit"))
			{
				OutResults.Add(FullPath, EPerforceState::CheckedOut);
			}
			else if(Action == TEXT("delete"))
			{
				OutResults.Add(FullPath, EPerforceState::MarkedForDelete);
			}
		}
	}
}

static const FString& FindWorkspaceFile(const TArray<FPerforceSourceControlState>& InStates, const FString& InDepotFile)
{
	for(auto It(InStates.CreateConstIterator()); It; It++)
	{
		if(It->DepotFilename == InDepotFile)
		{
			return It->LocalFilename;
		}
	}

	return InDepotFile;
}

static void ParseHistoryResults(const FP4RecordSet& InRecords, const TArray<FPerforceSourceControlState>& InStates, FPerforceUpdateStatusWorker::FHistoryMap& OutHistory)
{
	if (InRecords.Num() > 0)
	{
		// Iterate over each record, extracting the relevant information for each
		for (int32 RecordIndex = 0; RecordIndex < InRecords.Num(); ++RecordIndex)
		{
			const FP4Record& ClientRecord = InRecords[RecordIndex];

			// Extract the file name 
			check(ClientRecord.Contains(TEXT("depotFile")));
			FString DepotFileName = ClientRecord(TEXT("depotFile"));
			FString LocalFileName = FindWorkspaceFile(InStates, DepotFileName);

			TArray< TSharedRef<FPerforceSourceControlRevision, ESPMode::ThreadSafe> > Revisions;
			int32 RevisionNumbers = 0;
			for (;;)
			{
				// Extract the revision number
				FString VarName = FString::Printf(TEXT("rev%d"), RevisionNumbers);
				if (!ClientRecord.Contains(*VarName))
				{
					// No more revisions
					break;
				}
				FString RevisionNumber = ClientRecord(*VarName);

				// Extract the user name
				VarName = FString::Printf(TEXT("user%d"), RevisionNumbers);
				check(ClientRecord.Contains(*VarName));
				FString UserName = ClientRecord(*VarName);

				// Extract the date
				VarName = FString::Printf(TEXT("time%d"), RevisionNumbers);
				check(ClientRecord.Contains(*VarName));
				FString Date = ClientRecord(*VarName);

				// Extract the changelist number
				VarName = FString::Printf(TEXT("change%d"), RevisionNumbers);
				check(ClientRecord.Contains(*VarName));
				FString ChangelistNumber = ClientRecord(*VarName);

				// Extract the description
				VarName = FString::Printf(TEXT("desc%d"), RevisionNumbers);
				check(ClientRecord.Contains(*VarName));
				FString Description = ClientRecord(*VarName);

				// Extract the action
				VarName = FString::Printf(TEXT("action%d"), RevisionNumbers);
				check(ClientRecord.Contains(*VarName));
				FString Action = ClientRecord(*VarName);

				FString FileSize(TEXT("0"));

				// Extract the file size
				if(Action.ToLower() != TEXT("delete") && Action.ToLower() != TEXT("move/delete")) //delete actions don't have a fileSize from PV4
				{
					VarName = FString::Printf(TEXT("fileSize%d"), RevisionNumbers);
					check(ClientRecord.Contains(*VarName));
					FileSize = ClientRecord(*VarName);
				}
		
				// Extract the clientspec/workspace
				VarName = FString::Printf(TEXT("client%d"), RevisionNumbers);
				check(ClientRecord.Contains(*VarName));
				FString ClientSpec = ClientRecord(*VarName);

				TSharedRef<FPerforceSourceControlRevision, ESPMode::ThreadSafe> Revision = MakeShareable( new FPerforceSourceControlRevision() );
				Revision->FileName = LocalFileName;
				Revision->RevisionNumber = FCString::Atoi(*RevisionNumber);
				Revision->ChangelistNumber = FCString::Atoi(*ChangelistNumber);
				Revision->Description = Description;
				Revision->UserName = UserName;
				Revision->ClientSpec = ClientSpec;
				Revision->Action = Action;
				Revision->Date = FDateTime(1970, 1, 1, 0, 0, 0, 0) + FTimespan(0, 0, FCString::Atoi(*Date));
				Revision->FileSize = FCString::Atoi(*FileSize);

				Revisions.Add(Revision);

				RevisionNumbers++;
			}

			if(Revisions.Num() > 0)
			{
				OutHistory.Add(LocalFileName, Revisions);
			}
		}
	}
}

static void ParseDiffResults(const FP4RecordSet& InRecords, TArray<FString>& OutModifiedFiles)
{
	if (InRecords.Num() > 0)
	{
		// Iterate over each record found as a result of the command, parsing it for relevant information
		for (int32 Index = 0; Index < InRecords.Num(); ++Index)
		{
			const FP4Record& ClientRecord = InRecords[Index];
			FString FileName = ClientRecord(TEXT("clientFile"));
			FPaths::NormalizeFilename(FileName);
			OutModifiedFiles.Add(FileName);	
		}
	}
}


FName FPerforceUpdateStatusWorker::GetName() const
{
	return "UpdateStatus";
}

bool FPerforceUpdateStatusWorker::Execute(FPerforceSourceControlCommand& InCommand)
{
	FScopedPerforceConnection ScopedConnection(InCommand);
	if(ScopedConnection.IsValid())
	{
		FPerforceConnection& Connection = ScopedConnection.GetConnection();
		if(InCommand.Files.Num())
		{
			TArray<FString> Parameters = InCommand.Files;
			FP4RecordSet Records;
			InCommand.bCommandSuccessful = Connection.RunCommand(TEXT("fstat"), Parameters, Records, InCommand.ErrorMessages, FOnIsCancelled::CreateRaw(&InCommand, &FPerforceSourceControlCommand::IsCanceled), InCommand.bConnectionDropped);
			ParseUpdateStatusResults(Records, InCommand.ErrorMessages, OutStates);
			RemoveRedundantErrors(InCommand, TEXT(" - no such file(s)."));
			RemoveRedundantErrors(InCommand, TEXT("' is not under client's root '"));
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
			TArray<FString> Parameters;
			FP4RecordSet Records;
			// disregard non-contributory integrations
			Parameters.Add(TEXT("-s"));
			//include branching history
			Parameters.Add(TEXT("-i"));
			//include truncated change list descriptions
			Parameters.Add(TEXT("-L"));
			//include time stamps
			Parameters.Add(TEXT("-t"));
			//limit to last 100 changes
			Parameters.Add(TEXT("-m 100"));
			Parameters.Append(InCommand.Files);
			InCommand.bCommandSuccessful &= Connection.RunCommand(TEXT("filelog"), Parameters, Records, InCommand.ErrorMessages, FOnIsCancelled::CreateRaw(&InCommand, &FPerforceSourceControlCommand::IsCanceled), InCommand.bConnectionDropped);
			ParseHistoryResults(Records, OutStates, OutHistory);
		}

		if(Operation->ShouldGetOpenedOnly())
		{
			const FString ContentFolder = FPaths::ConvertRelativePathToFull(FPaths::RootDir());
			const FString FileQuery = FString::Printf(TEXT("%s..."), *ContentFolder);
			TArray<FString> Parameters = InCommand.Files;
			Parameters.Add(FileQuery);
			FP4RecordSet Records;
			InCommand.bCommandSuccessful &= Connection.RunCommand(TEXT("opened"), Parameters, Records, InCommand.ErrorMessages, FOnIsCancelled::CreateRaw(&InCommand, &FPerforceSourceControlCommand::IsCanceled), InCommand.bConnectionDropped);
			ParseOpenedResults(Records, ANSI_TO_TCHAR(Connection.P4Client.GetClient().Text()), Connection.ClientRoot, OutStateMap);
		}

		if(Operation->ShouldUpdateModifiedState())
		{
			TArray<FString> Parameters;
			FP4RecordSet Records;
			// Query for open files different than the versions stored in Perforce
			Parameters.Add(TEXT("-sa"));
			Parameters.Append(InCommand.Files);
			InCommand.bCommandSuccessful &= Connection.RunCommand(TEXT("diff"), Parameters, Records, InCommand.ErrorMessages, FOnIsCancelled::CreateRaw(&InCommand, &FPerforceSourceControlCommand::IsCanceled), InCommand.bConnectionDropped);

			// Parse the results and store them in the command
			ParseDiffResults(Records, OutModifiedFiles);
		}
	}

	return InCommand.bCommandSuccessful;
}

bool FPerforceUpdateStatusWorker::UpdateStates() const
{
	bool bUpdated = false;

	FPerforceSourceControlModule& PerforceSourceControl = FModuleManager::LoadModuleChecked<FPerforceSourceControlModule>("PerforceSourceControl");

	// first update cached state from 'fstat' call
	for(int StatusIndex = 0; StatusIndex < OutStates.Num(); StatusIndex++)
	{
		const FPerforceSourceControlState& Status = OutStates[StatusIndex];
		TSharedRef<FPerforceSourceControlState, ESPMode::ThreadSafe> State = PerforceSourceControl.GetProvider().GetStateInternal(Status.LocalFilename);
		State->SetState(Status.GetState());
		State->OtherUserCheckedOut = Status.OtherUserCheckedOut;
		State->TimeStamp = FDateTime::Now();
		State->bBinary = Status.bBinary;
		State->bExclusiveCheckout = Status.bExclusiveCheckout;
		State->DepotFilename = Status.DepotFilename;
		bUpdated = true;
	}

	// next update state from 'opened' call
	bUpdated |= UpdateCachedStates(OutStateMap);

	// add history, if any
	for(FHistoryMap::TConstIterator It(OutHistory); It; ++It)
	{
		TSharedRef<FPerforceSourceControlState, ESPMode::ThreadSafe> State = PerforceSourceControl.GetProvider().GetStateInternal(It.Key());
		State->History = It.Value();
		State->TimeStamp = FDateTime::Now();
		bUpdated = true;
	}

	// add modified state
	for(int ModifiedIndex = 0; ModifiedIndex < OutModifiedFiles.Num(); ModifiedIndex++)
	{
		const FString& FileName = OutModifiedFiles[ModifiedIndex];
		TSharedRef<FPerforceSourceControlState, ESPMode::ThreadSafe> State = PerforceSourceControl.GetProvider().GetStateInternal(FileName);
		State->bModifed = true;
		State->TimeStamp = FDateTime::Now();
		bUpdated = true;
	}

	return bUpdated;
}

FName FPerforceGetWorkspacesWorker::GetName() const
{
	return "GetWorkspaces";
}

bool FPerforceGetWorkspacesWorker::Execute(FPerforceSourceControlCommand& InCommand)
{
	FScopedPerforceConnection ScopedConnection(InCommand);
	if(ScopedConnection.IsValid())
	{
		FPerforceConnection& Connection = ScopedConnection.GetConnection();
		TArray<FString> ClientSpecList;
		InCommand.bCommandSuccessful = Connection.GetWorkspaceList(InCommand.UserName, FOnIsCancelled::CreateRaw(&InCommand, &FPerforceSourceControlCommand::IsCanceled), ClientSpecList, InCommand.ErrorMessages);

		check(InCommand.Operation->GetName() == "GetWorkspaces");
		TSharedRef<FGetWorkspaces, ESPMode::ThreadSafe> Operation = StaticCastSharedRef<FGetWorkspaces>(InCommand.Operation);
		Operation->Results = ClientSpecList;
	}
	return InCommand.bCommandSuccessful;
}

bool FPerforceGetWorkspacesWorker::UpdateStates() const
{
	return false;
}

#undef LOCTEXT_NAMESPACE