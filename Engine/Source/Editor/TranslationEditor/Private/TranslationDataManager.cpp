// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "TranslationEditorPrivatePCH.h"
#include "TranslationEditor.h"
#include "WorkspaceMenuStructureModule.h"
#include "TranslationDataObject.h"
#include "InternationalizationManifestJsonSerializer.h"
#include "InternationalizationArchiveJsonSerializer.h"
#include "ISourceControlModule.h"
#include "MessageLog.h"

DEFINE_LOG_CATEGORY_STATIC(LogTranslationEditor, Log, All);

#define LOCTEXT_NAMESPACE "TranslationDataManager"

FTranslationDataManager::FTranslationDataManager( const FName& ProjectName, const FName& TranslationTargetLanguage )
{
	ManifestName;
	ProjectPath;
	ArchiveName;

	// TODO: Hardcoded special cases for the Editor and Engine, should fix
	if (ProjectName == FName("Editor"))
	{
		ManifestName = FString("Editor.manifest");
		ArchiveName = FString("Editor.archive");
		ProjectPath = FPaths::EngineDir() + FString("Content/Localization/Editor");
	}
	else if (ProjectName == FName("Engine"))
	{
		ManifestName = FString("Engine.manifest");
		ArchiveName = FString("Engine.archive");
		ProjectPath = FPaths::EngineDir() + FString("Content/Localization/Engine");
	}

	CulturePath;

	// TODO: Detect language paths rather than hard code them
	if (TranslationTargetLanguage == FName("ja"))
	{
		CulturePath = ProjectPath + TEXT("/") + TEXT("ja");
	}
	else if (TranslationTargetLanguage == FName("ko"))
	{
		CulturePath = ProjectPath + TEXT("/") + TEXT("ko");
	}
	else if (TranslationTargetLanguage == FName("zh"))
	{
		CulturePath = ProjectPath + TEXT("/") + TEXT("zh");
	}

	GWarn->BeginSlowTask(LOCTEXT("LoadingTranslationData", "Loading Translation Data..."), true);

	TranslationData = NewObject<UTranslationDataObject>();
	check (TranslationData != NULL);

	// Used to use this to trick the AssetEditorToolkit into giving us save buttons, 
	// but it also added "Find in Content Browser" buttons that don't make sense at this time for Translation Editor
	// Maybe will re-enable in the future if things change.
	// TranslationData->SetFlags(RF_Public); //Otherwise IsAsset() fails

	// We want Undo/Redo support
	TranslationData->SetFlags(RF_Transactional);

	FString ManifestFilePath = ProjectPath / ManifestName;
	ManifestAtHeadRevisionPtr = ReadManifest( ManifestFilePath );
	if (ManifestAtHeadRevisionPtr.IsValid())
	{
		TSharedRef< FInternationalizationManifest > ManifestAtHeadRevision = ManifestAtHeadRevisionPtr.ToSharedRef();
		int32 ManifestEntriesCount = ManifestAtHeadRevision->GetNumEntriesBySourceText();

		if (ManifestEntriesCount < 1)
		{
			FFormatNamedArguments Arguments;
			Arguments.Add( TEXT("ManifestFilePath"), FText::FromString(ManifestFilePath) );
			Arguments.Add( TEXT("ManifestEntriesCount"), FText::AsNumber(ManifestEntriesCount) );
			FMessageLog TranslationEditorMessageLog("TranslationEditor");
			TranslationEditorMessageLog.Error(FText::Format(LOCTEXT("CurrentManifestEmpty", "Most current translation manifest ({ManifestFilePath}) has {ManifestEntriesCount} entries."), Arguments));
			TranslationEditorMessageLog.Notify(LOCTEXT("TranslationLoadError", "Error Loading Translations!"));
			TranslationEditorMessageLog.Open(EMessageSeverity::Error);
		}

		ArchiveFilePath = CulturePath + TEXT("/") + ArchiveName;
		ArchivePtr = ReadArchive( ManifestAtHeadRevision);
		if (ArchivePtr.IsValid())
		{
			TSharedRef< FInternationalizationArchive > Archive = ArchivePtr.ToSharedRef();
			TArray<FTranslationUnit> TranslationUnits;
			int32 NumManifestEntriesParsed = 0;

			GWarn->BeginSlowTask(LOCTEXT("LoadingCurrentManifest", "Loading Entries from Current Translation Manifest..."), true);
			// Get all manifest entries by source text (same source text in multiple contexts will only show up once)
			for (auto ManifestItr = ManifestAtHeadRevision->GetEntriesBySourceTextIterator(); ManifestItr; ++ManifestItr, ++NumManifestEntriesParsed)
			{
				GWarn->StatusUpdate(NumManifestEntriesParsed, ManifestEntriesCount, FText::Format(LOCTEXT("LoadingCurrentManifestEntries", "Loading Entry {0} of {1} from Current Translation Manifest..."), FText::AsNumber(NumManifestEntriesParsed), FText::AsNumber(ManifestEntriesCount)));
				const TSharedRef<FManifestEntry> ManifestEntry = ManifestItr.Value();
				FTranslationUnit TranslationUnit;
				TranslationUnit.HasBeenReviewed = false;
				TranslationUnit.Source = ManifestEntry->Source.Text.ReplaceEscapedCharWithChar();
				TranslationUnit.Namespace = ManifestEntry->Namespace;

				for(auto ContextIter( ManifestEntry->Contexts.CreateConstIterator() ); ContextIter; ++ContextIter)
				{
					FTranslationContextInfo ContextInfo;
					const FContext& AContext = *ContextIter;

					ContextInfo.Context = AContext.SourceLocation;
					ContextInfo.Key = AContext.Key;

					TranslationUnit.Contexts.Add(ContextInfo);
				}

				TranslationUnits.Add(TranslationUnit);
			}
			GWarn->EndSlowTask();

			// Once we have the context information, we can look up history
			GetHistoryForTranslationUnits(TranslationUnits, ManifestFilePath);

			GWarn->BeginSlowTask(LOCTEXT("LoadingArchiveEntries", "Loading Entries from Translation Archive..."), true);
			for (int32 CurrentTranslationUnitIndex = 0; CurrentTranslationUnitIndex < TranslationUnits.Num(); ++CurrentTranslationUnitIndex)
			{
				FTranslationUnit& TranslationUnit = TranslationUnits[CurrentTranslationUnitIndex];

				GWarn->StatusUpdate(CurrentTranslationUnitIndex, TranslationUnits.Num(), FText::Format(LOCTEXT("LoadingCurrentArchiveEntries", "Loading Entry {0} of {1} from Translation Archive..."), FText::AsNumber(CurrentTranslationUnitIndex), FText::AsNumber(TranslationUnits.Num())));

				const FLocItem SourceSearch(TranslationUnit.Source);
				TSharedPtr<FArchiveEntry> ArchiveEntry = Archive->FindEntryBySource(TranslationUnit.Namespace, SourceSearch, NULL);
				if ( ArchiveEntry.IsValid() )
				{
					const FString UnescapedTranslatedString = ArchiveEntry->Translation.Text.ReplaceEscapedCharWithChar();

					if(UnescapedTranslatedString.IsEmpty())
					{
						bool bHasTranslationHistory = false;
						int32 MostRecentNonNullTranslationIndex = -1;
						int32 ContextForRecentTranslation = -1;

						for (int32 ContextIndex = 0; ContextIndex < TranslationUnit.Contexts.Num(); ++ContextIndex)
						{
							for (int32 ChangeIndex = 0; ChangeIndex < TranslationUnit.Contexts[ContextIndex].Changes.Num(); ++ChangeIndex)
							{
								if (!(TranslationUnit.Contexts[ContextIndex].Changes[ChangeIndex].Translation.IsEmpty()))
								{
									bHasTranslationHistory = true;
									MostRecentNonNullTranslationIndex = ChangeIndex;
									ContextForRecentTranslation = ContextIndex;
									break;
								}
							}
					
							if (bHasTranslationHistory)
							{
								break;
							}
						}

						// If we have history, but current translation is empty, this goes in the Needs Review tab
						if (bHasTranslationHistory)
						{
							// Offer the most recent translation (for the first context in the list) as a suggestion or starting point (not saved unless user checks "Has Been Reviewed")
							TranslationUnit.Translation = TranslationUnit.Contexts[ContextForRecentTranslation].Changes[MostRecentNonNullTranslationIndex].Translation;
							TranslationData->Review.Add(TranslationUnit);
						}
						else
						{
							TranslationData->Untranslated.Add(TranslationUnit);
						}
					}
					else
					{
						TranslationUnit.Translation = UnescapedTranslatedString;
						TranslationUnit.HasBeenReviewed = true;
						TranslationData->Complete.Add(TranslationUnit);
					}
				}
			}
			GWarn->EndSlowTask();
		}
		else  // ArchivePtr.IsValid() is false
		{
			FFormatNamedArguments Arguments;
			Arguments.Add( TEXT("ArchiveFilePath"), FText::FromString(ArchiveFilePath) );
			FMessageLog TranslationEditorMessageLog("TranslationEditor");
			TranslationEditorMessageLog.Error(FText::Format(LOCTEXT("FailedToLoadCurrentArchive", "Failed to load most current translation archive ({ArchiveFilePath}), unable to load translations."), Arguments));
			TranslationEditorMessageLog.Notify(LOCTEXT("TranslationLoadError", "Error Loading Translations!"));
			TranslationEditorMessageLog.Open(EMessageSeverity::Error);
		}
	}
	else  // ManifestAtHeadRevisionPtr.IsValid() is false
	{
		FFormatNamedArguments Arguments;
		Arguments.Add( TEXT("ManifestFilePath"), FText::FromString(ManifestFilePath) );
		FMessageLog TranslationEditorMessageLog("TranslationEditor");
		TranslationEditorMessageLog.Error(FText::Format(LOCTEXT("FailedToLoadCurrentManifest", "Failed to load most current translation manifest ({ManifestFilePath}), unable to load translations."), Arguments));
		TranslationEditorMessageLog.Notify(LOCTEXT("TranslationLoadError", "Error Loading Translations!"));
		TranslationEditorMessageLog.Open(EMessageSeverity::Error);
	}

	GWarn->EndSlowTask();
}

TSharedPtr< FInternationalizationManifest > FTranslationDataManager::ReadManifest( const FString& ManifestFilePath )
{
	
	TSharedPtr<FJsonObject> ManifestJsonObject = ReadJSONTextFile( ManifestFilePath );

	if( !ManifestJsonObject.IsValid() )
	{
		UE_LOG(LogTranslationEditor, Error, TEXT("Could not read manifest file %s."), *ManifestFilePath);
		return TSharedPtr< FInternationalizationManifest >();
	}

	TSharedRef< FInternationalizationManifest > InternationalizationManifest = MakeShareable( new FInternationalizationManifest );

	ManifestSerializer.DeserializeManifest( ManifestJsonObject.ToSharedRef(), InternationalizationManifest );

	return InternationalizationManifest;
}

TSharedPtr< FInternationalizationArchive > FTranslationDataManager::ReadArchive(TSharedRef< FInternationalizationManifest >& InternationalizationManifest)
{
	// Read in any existing archive for this culture.
	TSharedPtr< FJsonObject > ArchiveJsonObject = ReadJSONTextFile( ArchiveFilePath );

	if ( !ArchiveJsonObject.IsValid() )
	{
		UE_LOG(LogTranslationEditor, Error, TEXT("Could not read archive file %s."), *ArchiveFilePath);
		return NULL;
	}

	TSharedRef< FInternationalizationArchive > InternationalizationArchive = MakeShareable( new FInternationalizationArchive );

	ArchiveSerializer.DeserializeArchive( ArchiveJsonObject.ToSharedRef(), InternationalizationArchive );

	return InternationalizationArchive;
}

TSharedPtr<FJsonObject> FTranslationDataManager::ReadJSONTextFile(const FString& InFilePath)
{
	//read in file as string
	FString FileContents;
	if ( !FFileHelper::LoadFileToString(FileContents, *InFilePath) )
	{
		UE_LOG(LogTranslationEditor, Error,TEXT("Failed to load file %s."), *InFilePath);
		return NULL;
	}

	//parse as JSON
	TSharedPtr<FJsonObject> JSONObject;

	TSharedRef< TJsonReader<> > Reader = TJsonReaderFactory<>::Create( FileContents );

	if( !FJsonSerializer::Deserialize( Reader, JSONObject ) || !JSONObject.IsValid())
	{
		UE_LOG(LogTranslationEditor, Error,TEXT("Invalid JSON in file %s."), *InFilePath);
		return NULL;
	}

	return JSONObject;
}

void FTranslationDataManager::WriteTranslationData()
{
	check (ArchivePtr.IsValid());
	TSharedRef< FInternationalizationArchive > Archive = ArchivePtr.ToSharedRef();

	bool bNeedsWrite = false;

	for (FTranslationUnit TranslationUnit : TranslationData->Untranslated)
	{
		const FLocItem SearchSource(TranslationUnit.Source);
		FString OldTranslation = Archive->FindEntryBySource(TranslationUnit.Namespace, SearchSource, NULL)->Translation.Text;
		FString TranslationToWrite = TranslationUnit.Translation.ReplaceCharWithEscapedChar();
		if ( !TranslationToWrite.Equals(OldTranslation) )
		{
			Archive->SetTranslation(TranslationUnit.Namespace, TranslationUnit.Source, TranslationToWrite, NULL);
			bNeedsWrite = true;
		}
	}

	for (FTranslationUnit TranslationUnit : TranslationData->Review)
	{
		const FLocItem SearchSource(TranslationUnit.Source);
		FString OldTranslation = Archive->FindEntryBySource(TranslationUnit.Namespace, SearchSource, NULL)->Translation.Text;
		FString TranslationToWrite = TranslationUnit.Translation.ReplaceCharWithEscapedChar();
		if ( TranslationUnit.HasBeenReviewed && !TranslationToWrite.Equals(OldTranslation) )
		{
			Archive->SetTranslation(TranslationUnit.Namespace, TranslationUnit.Source, TranslationToWrite, NULL);
			bNeedsWrite = true;
		}
	}

	for (FTranslationUnit TranslationUnit : TranslationData->Complete)
	{
		const FLocItem SearchSource(TranslationUnit.Source);
		FString OldTranslation = Archive->FindEntryBySource(TranslationUnit.Namespace, SearchSource, NULL)->Translation.Text;
		FString TranslationToWrite = TranslationUnit.Translation.ReplaceCharWithEscapedChar();
		if ( !TranslationToWrite.Equals(OldTranslation) )
		{
			Archive->SetTranslation(TranslationUnit.Namespace, TranslationUnit.Source, TranslationToWrite, NULL);
			bNeedsWrite = true;
		}
	}

	if (bNeedsWrite)
	{
		TSharedRef<FJsonObject> FinalArchiveJsonObj = MakeShareable( new FJsonObject );
		ArchiveSerializer.SerializeArchive( Archive, FinalArchiveJsonObj );

		WriteJSONToTextFile( FinalArchiveJsonObj, ArchiveFilePath );
	}
}

bool FTranslationDataManager::WriteJSONToTextFile(TSharedRef<FJsonObject>& Output, const FString& Filename)
{
	bool CheckoutAndSaveWasSuccessful = true;
	bool bPreviouslyCheckedOut = false;

	// If the user specified a reference file - write the entries read from code to a ref file
	if ( !Filename.IsEmpty() )
	{
		// Already checked out?
		if(CheckedOutFiles.Contains(Filename))
		{
			bPreviouslyCheckedOut = true;
		}
		else if( !SourceControlHelpers::CheckOutFile(Filename) )
		{
			FFormatNamedArguments Arguments;
			Arguments.Add( TEXT("Filename"), FText::FromString(Filename) );
			// Use Source Control Message Log here because there might be other useful information in that log for the user.
			FMessageLog SourceControlMessageLog("SourceControl");
			SourceControlMessageLog.Error(FText::Format(LOCTEXT("CheckoutFailed", "Check out of file '{Filename}' failed."), Arguments));
			SourceControlMessageLog.Notify(LOCTEXT("TranslationArchiveCheckoutFailed", "Failed to Check Out Translation Archive!"));
			SourceControlMessageLog.Open(EMessageSeverity::Error);
			CheckoutAndSaveWasSuccessful = false;
		}
		else
		{
			CheckedOutFiles.Add(Filename);
		}

		if( CheckoutAndSaveWasSuccessful )
		{
			//Print the JSON data out to the ref file.
			FString OutputString;
			TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create( &OutputString );
			FJsonSerializer::Serialize( Output, Writer );

			if (!FFileHelper::SaveStringToFile(OutputString, *Filename, FFileHelper::EEncodingOptions::ForceUnicode))
			{
				// If we already checked out the file, but cannot write it, perhaps the user checked it in via perforce, so try to check it out again
				if (bPreviouslyCheckedOut)
				{
					bPreviouslyCheckedOut = false;

					if( !SourceControlHelpers::CheckOutFile(Filename) )
					{
						FFormatNamedArguments Arguments;
						Arguments.Add( TEXT("Filename"), FText::FromString(Filename) );
						// Use Source Control Message Log here because there might be other useful information in that log for the user.
						FMessageLog SourceControlMessageLog("SourceControl");
						SourceControlMessageLog.Error(FText::Format(LOCTEXT("CheckoutFailed", "Check out of file '{Filename}' failed."), Arguments));
						SourceControlMessageLog.Notify(LOCTEXT("TranslationArchiveCheckoutFailed", "Failed to Check Out Translation Archive!"));
						SourceControlMessageLog.Open(EMessageSeverity::Error);
						CheckoutAndSaveWasSuccessful = false;

						CheckedOutFiles.Remove(Filename);
					}
				}

				FFormatNamedArguments Arguments;
				Arguments.Add( TEXT("Filename"), FText::FromString(Filename) );
				FMessageLog TranslationEditorMessageLog("TranslationEditor");
				TranslationEditorMessageLog.Error(FText::Format(LOCTEXT("WriteFileFailed", "Failed to write localization entries to file '{Filename}'."), Arguments));
				TranslationEditorMessageLog.Notify(LOCTEXT("FileWriteFailed", "Failed to Write Translations to File!"));
				TranslationEditorMessageLog.Open(EMessageSeverity::Error);
				CheckoutAndSaveWasSuccessful = false;
			}
		}
	}
	else
	{
		CheckoutAndSaveWasSuccessful = false;
	}

	// If this is the first time, let the user know the file was checked out
	if (!bPreviouslyCheckedOut && CheckoutAndSaveWasSuccessful)
	{
		struct Local
		{
			/**
			* Called by our notification's hyperlink to open the Source Control message log
			*/
			static void OpenSourceControlMessageLog(  )
			{
				FMessageLog("SourceControl").Open();
			}
		};

		FFormatNamedArguments Arguments;
		Arguments.Add( TEXT("Filename"), FText::FromString(Filename) );

		// Make a note in the Source Control log, including a note to check in the file later via source control application
		FMessageLog TranslationEditorMessageLog("SourceControl");
		TranslationEditorMessageLog.Info(FText::Format(LOCTEXT("TranslationArchiveCheckedOut", "Successfully checked out and saved translation archive '{Filename}'. Please check-in this file later via your source control application."), Arguments));

		// Display notification that save was successful, along with a link to the Source Control log so the user can see the above message.
		FNotificationInfo Info( LOCTEXT("ArchiveCheckedOut", "Translation Archive Successfully Checked Out and Saved.") );
		Info.ExpireDuration = 5;
		Info.Hyperlink = FSimpleDelegate::CreateStatic(&Local::OpenSourceControlMessageLog);
		Info.bFireAndForget = true;
		Info.bUseSuccessFailIcons = true;
		Info.Image = FEditorStyle::GetBrush(TEXT("NotificationList.SuccessImage"));
		FSlateNotificationManager::Get().AddNotification(Info);
	}

	return CheckoutAndSaveWasSuccessful;
}

void FTranslationDataManager::GetHistoryForTranslationUnits( TArray<FTranslationUnit>& TranslationUnits, const FString& ManifestFilePath )
{
	GWarn->BeginSlowTask(LOCTEXT("LoadingSourceControlHistory", "Loading Translation History from Source Control..."), true);

	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
	TSharedRef<FUpdateStatus, ESPMode::ThreadSafe> UpdateStatusOperation = ISourceControlOperation::Create<FUpdateStatus>();
	UpdateStatusOperation->SetUpdateHistory( true );
	SourceControlProvider.Execute( UpdateStatusOperation, ManifestFilePath );
	FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState( ManifestFilePath, EStateCacheUsage::ForceUpdate );
	if ( SourceControlState.IsValid() )
	{
		int32 HistorySize = SourceControlState->GetHistorySize();

		for (int HistoryItemIndex = 0; HistoryItemIndex < HistorySize; ++HistoryItemIndex)
		{
			GWarn->StatusUpdate(HistoryItemIndex, HistorySize, FText::Format(LOCTEXT("LoadingOldManifestRevisionNumber", "Loading Translation History from Manifest Revision {0} of {1} from Source Control..."), FText::AsNumber(HistoryItemIndex), FText::AsNumber(HistorySize)));

			TSharedPtr<ISourceControlRevision, ESPMode::ThreadSafe> Revision = SourceControlState->GetHistoryItem(HistoryItemIndex);
			if(Revision.IsValid())
			{
				FString TempFileName;
				if(Revision->Get(TempFileName))
				{
					TSharedPtr< FInternationalizationManifest > OldManifestPtr = ReadManifest( TempFileName );
					if (OldManifestPtr.IsValid())	// There may be corrupt manifests in the history, so ignore them.
					{
						TSharedRef< FInternationalizationManifest > OldManifest = OldManifestPtr.ToSharedRef();

						for (FTranslationUnit& TranslationUnit : TranslationUnits)
						{
							if(TranslationUnit.Contexts.Num() > 0)
							{
								for (FTranslationContextInfo& ContextInfo : TranslationUnit.Contexts)
								{
									FString PreviousSourceText = TranslationUnit.Source;

									// If we already have history, then compare against the oldest history so far
									if (ContextInfo.Changes.Num() > 0)
									{
										PreviousSourceText = ContextInfo.Changes[ContextInfo.Changes.Num()-1].Source;
									}

									FContext SearchContext;
									SearchContext.Key = ContextInfo.Key;
									TSharedPtr< FManifestEntry > OldManifestEntryPtr = OldManifest->FindEntryByContext(TranslationUnit.Namespace, SearchContext);
									if (!OldManifestEntryPtr.IsValid())
									{
										continue;
									}

									if (!OldManifestEntryPtr->Source.Text.Equals(PreviousSourceText))
									{
										TSharedPtr< FArchiveEntry > OldArchiveEntry = ArchivePtr->FindEntryBySource(OldManifestEntryPtr->Namespace, OldManifestEntryPtr->Source, NULL);
										if (OldArchiveEntry.IsValid())
										{
											FTranslationChange Change;
											Change.Source = OldManifestEntryPtr->Source.Text.ReplaceEscapedCharWithChar();
											Change.Translation = OldArchiveEntry->Translation.Text.ReplaceEscapedCharWithChar();
											Change.DateAndTime = Revision->GetDate();
											Change.Version = FString::FromInt(Revision->GetRevisionNumber());
											ContextInfo.Changes.Add(Change);
										}
									}
								}
							}
						}
					}
					else // OldManifestPtr.IsValid() is false
					{
						FFormatNamedArguments Arguments;
						Arguments.Add( TEXT("ManifestFilePath"), FText::FromString(ManifestFilePath) );
						Arguments.Add( TEXT("ManifestRevisionNumber"), FText::AsNumber(Revision->GetRevisionNumber()) );
						FMessageLog TranslationEditorMessageLog("TranslationEditor");
						TranslationEditorMessageLog.Warning(FText::Format(LOCTEXT("PreviousManifestCorrupt", "Previous revision {ManifestRevisionNumber} of {ManifestFilePath} failed to load correctly. Ignoring."), Arguments));
					}
				}
			}
		}
	}
	else // SourceControlState.IsValid() is false
	{
		FFormatNamedArguments Arguments;
		Arguments.Add( TEXT("ManifestFilePath"), FText::FromString(ManifestFilePath) );
		FMessageLog TranslationEditorMessageLog("SourceControl");
		TranslationEditorMessageLog.Warning(FText::Format(LOCTEXT("SourceControlStateQueryFailed", "Failed to query source control state of file {ManifestFilePath}."), Arguments));
		TranslationEditorMessageLog.Notify(LOCTEXT("RetrieveTranslationHistoryFailed", "Unable to Retrieve Translation History from Source Control!"));
	}

	GWarn->EndSlowTask();
}

#undef LOCTEXT_NAMESPACE