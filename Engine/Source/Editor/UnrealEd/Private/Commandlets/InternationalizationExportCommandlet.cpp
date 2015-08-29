// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "Culture.h"
#include "JsonInternationalizationManifestSerializer.h"
#include "JsonInternationalizationArchiveSerializer.h"
#include "PortableObjectFormatDOM.h"

DEFINE_LOG_CATEGORY_STATIC(LogInternationalizationExportCommandlet, Log, All);

static const TCHAR* NewLineDelimiter = TEXT("\n");


/**
*	Helper Functions
*/
FString ConditionArchiveStrForPo(const FString& InStr)
{
	FString Result;
	for (const TCHAR C : InStr)
	{
		switch (C)
		{
		case TEXT('\\'):	Result += TEXT("\\\\");	break;
		case TEXT('"'):		Result += TEXT("\\\"");	break;
		case TEXT('\r'):	Result += TEXT("\\r");	break;
		case TEXT('\n'):	Result += TEXT("\\n");	break;
		case TEXT('\t'):	Result += TEXT("\\t");	break;
		default:			Result += C;			break;
		}
	}
	return Result;
}

FString ConditionPoStringForArchive(const FString& InStr)
{
	FString Result;
	FString EscapeSequenceBuffer;

	auto HandleEscapeSequenceBuffer = [&]()
	{
		// Insert unescaped sequence if needed.
		if (!EscapeSequenceBuffer.IsEmpty())
		{
			bool EscapeSequenceIdentified = true;

			// Identify escape sequence
			TCHAR UnescapedCharacter = 0;
			if (EscapeSequenceBuffer == TEXT("\\\\"))
			{
				UnescapedCharacter = '\\';
			}
			else if (EscapeSequenceBuffer == TEXT("\\\""))
			{
				UnescapedCharacter = '"';
			}
			else if (EscapeSequenceBuffer == TEXT("\\r"))
			{
				UnescapedCharacter = '\r';
			}
			else if (EscapeSequenceBuffer == TEXT("\\n"))
			{
				UnescapedCharacter = '\n';
			}
			else if (EscapeSequenceBuffer == TEXT("\\t"))
			{
				UnescapedCharacter = '\t';
			}
			else
			{
				EscapeSequenceIdentified = false;
			}

			// If identified, append the processed sequence as the unescaped character.
			if (EscapeSequenceIdentified)
			{
				Result += UnescapedCharacter;
			}
			// If it was not identified, preserve the escape sequence and append it.
			else
			{
				Result += EscapeSequenceBuffer;
			}
			// Either way, we've appended something based on the buffer and it should be reset.
			EscapeSequenceBuffer.Empty();
		}
	};

	for (const TCHAR C : InStr)
	{
		// Not in an escape sequence.
		if (EscapeSequenceBuffer.IsEmpty())
		{
			// Regular character, just copy over.
			if (C != TEXT('\\'))
			{
				Result += C;
			}
			// Start of an escape sequence, put in escape sequence buffer.
			else
			{
				EscapeSequenceBuffer += C;
			}
		}
		// If already in an escape sequence.
		else
		{
			// Append to escape sequence buffer.
			EscapeSequenceBuffer += C;

			HandleEscapeSequenceBuffer();
		}
	}
	// Catch any trailing backslashes.
	HandleEscapeSequenceBuffer();
	return Result;
}


FString ConvertSrcLocationToPORef(const FString& InSrcLocation)
{
	// Source location format: /Path1/Path2/file.cpp - line 123
	// PO Reference format: /Path1/Path2/file.cpp:123
	// @TODO: Note, we assume the source location format here but it could be arbitrary.
	return InSrcLocation.Replace(TEXT(" - line "), TEXT(":"));
}

/**
 *	UInternationalizationExportCommandlet
 */
UInternationalizationExportCommandlet::UInternationalizationExportCommandlet(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


bool UInternationalizationExportCommandlet::DoExport( const FString& SourcePath, const FString& DestinationPath, const FString& Filename )
{
	// Get native culture.
	FString NativeCultureName;
	if( !GetStringFromConfig( *SectionName, TEXT("NativeCulture"), NativeCultureName, ConfigPath ) )
	{
		UE_LOG( LogInternationalizationExportCommandlet, Error, TEXT("No native culture specified.") );
		return false;
	}

	// Get manifest name.
	FString ManifestName;
	if( !GetStringFromConfig( *SectionName, TEXT("ManifestName"), ManifestName, ConfigPath ) )
	{
		UE_LOG( LogInternationalizationExportCommandlet, Error, TEXT("No manifest name specified.") );
		return false;
	}

	// Get archive name.
	FString ArchiveName;
	if( !( GetStringFromConfig(* SectionName, TEXT("ArchiveName"), ArchiveName, ConfigPath ) ) )
	{
		UE_LOG(LogInternationalizationExportCommandlet, Error, TEXT("No archive name specified."));
		return false;
	}

	// Get culture directory setting, default to true if not specified (used to allow picking of export directory with windows file dialog from Translation Editor)
	bool bUseCultureDirectory = true;
	if (!(GetBoolFromConfig(*SectionName, TEXT("bUseCultureDirectory"), bUseCultureDirectory, ConfigPath)))
	{
		bUseCultureDirectory = true;
	}


	TSharedRef< FInternationalizationManifest > InternationalizationManifest = MakeShareable( new FInternationalizationManifest );
	// Load the manifest info
	{
		FString ManifestFilePath = SourcePath / ManifestName;
		if( !FPaths::FileExists(ManifestFilePath) )
		{
			UE_LOG(LogInternationalizationExportCommandlet, Error, TEXT("Could not find manifest file %s."), *ManifestFilePath);
			return false;
		}

		TSharedPtr<FJsonObject> ManifestJsonObject = ReadJSONTextFile( ManifestFilePath );

		if( !ManifestJsonObject.IsValid() )
		{
			UE_LOG(LogInternationalizationExportCommandlet, Error, TEXT("Could not read manifest file %s."), *ManifestFilePath);
			return false;
		}

		FJsonInternationalizationManifestSerializer ManifestSerializer;
		ManifestSerializer.DeserializeManifest( ManifestJsonObject.ToSharedRef(), InternationalizationManifest );
	}

	TArray< TSharedPtr<FInternationalizationArchive> > NativeArchives;
	{
		const FString NativeCulturePath = DestinationPath / *(NativeCultureName);
		TArray<FString> NativeArchiveFileNames;
		IFileManager::Get().FindFiles(NativeArchiveFileNames, *(NativeCulturePath / TEXT("*.archive")), true, false);

		for (const FString& NativeArchiveFileName : NativeArchiveFileNames)
		{
			// Read each archive file from the culture-named directory in the source path.
			FString ArchiveFilePath = NativeCulturePath / NativeArchiveFileName;
			ArchiveFilePath = FPaths::ConvertRelativePathToFull(ArchiveFilePath);
			TSharedRef<FInternationalizationArchive> InternationalizationArchive = MakeShareable(new FInternationalizationArchive);
			TSharedPtr< FJsonObject > ArchiveJsonObject = ReadJSONTextFile( ArchiveFilePath );
			FJsonInternationalizationArchiveSerializer ArchiveSerializer;
			ArchiveSerializer.DeserializeArchive( ArchiveJsonObject.ToSharedRef(), InternationalizationArchive );

			NativeArchives.Add(InternationalizationArchive);
		}
	}

	// Process the desired cultures
	for(int32 Culture = 0; Culture < CulturesToGenerate.Num(); Culture++)
	{
		// Load the archive
		const FString CultureName = CulturesToGenerate[Culture];
		const FString CulturePath = SourcePath / CultureName;
		FString ArchiveFileName = CulturePath / ArchiveName;
		TSharedPtr< FJsonObject > ArchiveJsonObject = NULL;

		if( FPaths::FileExists(ArchiveFileName) )
		{
			ArchiveJsonObject = ReadJSONTextFile( ArchiveFileName );

			FJsonInternationalizationArchiveSerializer ArchiveSerializer;
			TSharedRef< FInternationalizationArchive > InternationalizationArchive = MakeShareable( new FInternationalizationArchive );
			ArchiveSerializer.DeserializeArchive( ArchiveJsonObject.ToSharedRef(), InternationalizationArchive );

			{
				FPortableObjectFormatDOM PortableObj;

				FString LocLang;
				if( !PortableObj.SetLanguage( CultureName ) )
				{
					UE_LOG( LogInternationalizationExportCommandlet, Error, TEXT("Skipping export of loc language %s because it is not recognized."), *LocLang );
					continue;
				}

				PortableObj.SetProjectName( FPaths::GetBaseFilename( ManifestName ) );
				PortableObj.CreateNewHeader();

				{
					for(TManifestEntryBySourceTextContainer::TConstIterator ManifestIter = InternationalizationManifest->GetEntriesBySourceTextIterator(); ManifestIter; ++ManifestIter)
					{
						// Gather relevant info from manifest entry.
						const TSharedRef<FManifestEntry>& ManifestEntry = ManifestIter.Value();
						const FString& Namespace = ManifestEntry->Namespace;
						const FLocItem& Source = ManifestEntry->Source;

						for( auto ContextIter = ManifestEntry->Contexts.CreateConstIterator(); ContextIter; ++ContextIter )
						{
							{
								const TSharedPtr<FArchiveEntry> ArchiveEntry = InternationalizationArchive->FindEntryBySource( Namespace, Source, ContextIter->KeyMetadataObj );
							if( ArchiveEntry.IsValid() )
							{
								const FString ConditionedArchiveSource = ConditionArchiveStrForPo(ArchiveEntry->Source.Text);
								const FString ConditionedArchiveTranslation = ConditionArchiveStrForPo(ArchiveEntry->Translation.Text);

								TSharedRef<FPortableObjectEntry> PoEntry = MakeShareable( new FPortableObjectEntry );
								//@TODO: We support additional metadata entries that can be translated.  How do those fit in the PO file format?  Ex: isMature
								PoEntry->MsgId = ConditionedArchiveSource;
								//@TODO: Take into account optional entries and entries that differ by keymetadata.  Ex. Each optional entry needs a unique msgCtxt
								PoEntry->MsgCtxt = Namespace;
								PoEntry->MsgStr.Add( ConditionedArchiveTranslation );

								FString PORefString = ConvertSrcLocationToPORef( ContextIter->SourceLocation );
								PoEntry->AddReference( PORefString ); // Source location.
								PoEntry->AddExtractedComment( ContextIter->Key ); // "Notes from Programmer" in the form of the Key.
								PoEntry->AddExtractedComment( PORefString ); // "Notes from Programmer" in the form of the Source Location, since this comes in handy too and OneSky doesn't properly show references, only comments.
								PortableObj.AddEntry( PoEntry );
							}
						}

							if (CultureName != NativeCultureName)
							{
								TSharedPtr<FArchiveEntry> NativeArchiveEntry;
								for (const auto& NativeArchive : NativeArchives)
								{
									const TSharedPtr<FArchiveEntry> PotentialNativeArchiveEntry = NativeArchive->FindEntryBySource( Namespace, Source, ContextIter->KeyMetadataObj );
									if (PotentialNativeArchiveEntry.IsValid())
									{
										NativeArchiveEntry = PotentialNativeArchiveEntry;
										break;
									}
								}

								if (NativeArchiveEntry.IsValid())
								{
									if (!NativeArchiveEntry->Source.IsExactMatch(NativeArchiveEntry->Translation))
									{
										const TSharedPtr<FArchiveEntry> ArchiveEntry = InternationalizationArchive->FindEntryBySource( Namespace, NativeArchiveEntry->Translation, NativeArchiveEntry->KeyMetadataObj );

										const FString ConditionedArchiveSource = ConditionArchiveStrForPo(ArchiveEntry->Source.Text);
										const FString ConditionedArchiveTranslation = ConditionArchiveStrForPo(ArchiveEntry->Translation.Text);

										TSharedRef<FPortableObjectEntry> PoEntry = MakeShareable( new FPortableObjectEntry );
										//@TODO: We support additional metadata entries that can be translated.  How do those fit in the PO file format?  Ex: isMature
										PoEntry->MsgId = ConditionedArchiveSource;
										//@TODO: Take into account optional entries and entries that differ by keymetadata.  Ex. Each optional entry needs a unique msgCtxt
										PoEntry->MsgCtxt = Namespace;
										PoEntry->MsgStr.Add( ConditionedArchiveTranslation );

										FString PORefString = ConvertSrcLocationToPORef( ContextIter->SourceLocation );
										PoEntry->AddReference( PORefString ); // Source location.
										PoEntry->AddExtractedComment( ContextIter->Key ); // "Notes from Programmer" in the form of the Key.
										PoEntry->AddExtractedComment( PORefString ); // "Notes from Programmer" in the form of the Source Location, since this comes in handy too and OneSky doesn't properly show references, only comments.
										PortableObj.AddEntry( PoEntry );
									}
								}
					}
				}
					}
				}

				// Write out the Portable Object to .po file.
				{
					PortableObj.SortEntries();
					FString OutputString = PortableObj.ToString();
					FString OutputFileName = "";
					if (bUseCultureDirectory)
					{
						OutputFileName = DestinationPath / CultureName / Filename;
					}
					else
					{
						OutputFileName = DestinationPath / Filename;
					}

					if( SourceControlInfo.IsValid() )
					{
						FText SCCErrorText;
						if (!SourceControlInfo->CheckOutFile(OutputFileName, SCCErrorText))
						{
							UE_LOG(LogInternationalizationExportCommandlet, Error, TEXT("Check out of file %s failed: %s"), *OutputFileName, *SCCErrorText.ToString());
							return false;
						}
					}

					//@TODO We force UTF8 at the moment but we want this to be based on the format found in the header info.
					if( !FFileHelper::SaveStringToFile(OutputString, *OutputFileName, FFileHelper::EEncodingOptions::ForceUTF8) )
					{
						UE_LOG( LogInternationalizationExportCommandlet, Error, TEXT("Could not write file %s"), *OutputFileName );
						return false;
					}
				}
			}
		}
	}
	return true;
}

bool UInternationalizationExportCommandlet::DoImport(const FString& SourcePath, const FString& DestinationPath, const FString& Filename)
{
	// Get manifest name.
	FString ManifestName;
	if( !GetStringFromConfig( *SectionName, TEXT("ManifestName"), ManifestName, ConfigPath ) )
	{
		UE_LOG( LogInternationalizationExportCommandlet, Error, TEXT("No manifest name specified.") );
		return false;
	}

	// Get archive name.
	FString ArchiveName;
	if( !( GetStringFromConfig(* SectionName, TEXT("ArchiveName"), ArchiveName, ConfigPath ) ) )
	{
		UE_LOG(LogInternationalizationExportCommandlet, Error, TEXT("No archive name specified."));
		return false;
	}

	// Get culture directory setting, default to true if not specified (used to allow picking of import directory with file open dialog from Translation Editor)
	bool bUseCultureDirectory = true;
	if (!(GetBoolFromConfig(*SectionName, TEXT("bUseCultureDirectory"), bUseCultureDirectory, ConfigPath)))
	{
		bUseCultureDirectory = true;
	}

	// Process the desired cultures
	for(int32 Culture = 0; Culture < CulturesToGenerate.Num(); Culture++)
	{
		// Load the Portable Object file if found
		const FString CultureName = CulturesToGenerate[Culture];
		FString POFilePath = "";
		if (bUseCultureDirectory)
		{
			POFilePath = SourcePath / CultureName / Filename;
		}
		else
		{
			POFilePath = SourcePath / Filename;
		}

		if( !FPaths::FileExists(POFilePath) )
		{
			UE_LOG( LogInternationalizationExportCommandlet, Warning, TEXT("Could not find file %s"), *POFilePath );
			continue;
		}

		FString POFileContents;
		if ( !FFileHelper::LoadFileToString( POFileContents, *POFilePath ) )
		{
			UE_LOG( LogInternationalizationExportCommandlet, Error, TEXT("Failed to load file %s."), *POFilePath);
			continue;
		}

		FPortableObjectFormatDOM PortableObject;
		if( !PortableObject.FromString( POFileContents ) )
		{
			UE_LOG( LogInternationalizationExportCommandlet, Error, TEXT("Failed to parse Portable Object file %s."), *POFilePath);
			continue;
		}

		if( PortableObject.GetProjectName() != ManifestName.Replace(TEXT(".manifest"), TEXT("")) )
		{
			UE_LOG( LogInternationalizationExportCommandlet, Warning, TEXT("The project name (%s) in the file (%s) did not match the target manifest project (%s)."), *POFilePath, *PortableObject.GetProjectName(), *ManifestName.Replace(TEXT(".manifest"), TEXT("")));
		}


		const FString DestinationCulturePath = DestinationPath / CultureName;
		FString ArchiveFileName = DestinationCulturePath / ArchiveName;
		
		if( !FPaths::FileExists(ArchiveFileName) )
		{
			UE_LOG( LogInternationalizationExportCommandlet, Error, TEXT("Failed to find destination archive %s."), *ArchiveFileName);
			continue;
		}

		TSharedPtr< FJsonObject > ArchiveJsonObject = NULL;
		ArchiveJsonObject = ReadJSONTextFile( ArchiveFileName );

		FJsonInternationalizationArchiveSerializer ArchiveSerializer;
		TSharedRef< FInternationalizationArchive > InternationalizationArchive = MakeShareable( new FInternationalizationArchive );
		ArchiveSerializer.DeserializeArchive( ArchiveJsonObject.ToSharedRef(), InternationalizationArchive );

		bool bModifiedArchive = false;
		{
			for( auto EntryIter = PortableObject.GetEntriesIterator(); EntryIter; ++EntryIter )
			{
				auto POEntry = *EntryIter;
				if( POEntry->MsgId.IsEmpty() || POEntry->MsgStr.Num() == 0 || POEntry->MsgStr[0].Trim().IsEmpty() )
				{
					// We ignore the header entry or entries with no translation.
					continue;
				}

				// Some warning messages for data we don't process at the moment
				if( !POEntry->MsgIdPlural.IsEmpty() || POEntry->MsgStr.Num() > 1 )
				{
					UE_LOG( LogInternationalizationExportCommandlet, Error, TEXT("Portable Object entry has plural form we did not process.  File: %s  MsgCtxt: %s  MsgId: %s"), *POFilePath, *POEntry->MsgCtxt, *POEntry->MsgId );
				}
				
				const FString& Namespace = POEntry->MsgCtxt;
				const FString& SourceText = ConditionPoStringForArchive(POEntry->MsgId);
				const FString& Translation = ConditionPoStringForArchive(POEntry->MsgStr[0]);

				//@TODO: Take into account optional entries and entries that differ by keymetadata.  Ex. Each optional entry needs a unique msgCtxt
				TSharedPtr< FArchiveEntry > FoundEntry = InternationalizationArchive->FindEntryBySource( Namespace, SourceText, NULL );
				if( !FoundEntry.IsValid() )
				{
					UE_LOG(LogInternationalizationExportCommandlet, Warning, TEXT("Could not find corresponding archive entry for PO entry.  File: %s  MsgCtxt: %s  MsgId: %s"), *POFilePath, *POEntry->MsgCtxt, *POEntry->MsgId );
					continue;
				}
				
				if( FoundEntry->Translation != Translation )
				{
					FoundEntry->Translation = Translation;
					bModifiedArchive = true;
				}
			}
		}

		if( bModifiedArchive )
		{
			TSharedRef<FJsonObject> FinalArchiveJsonObj = MakeShareable( new FJsonObject );
			ArchiveSerializer.SerializeArchive( InternationalizationArchive, FinalArchiveJsonObj );

			if( !WriteJSONToTextFile(FinalArchiveJsonObj, ArchiveFileName, SourceControlInfo ) )
			{
				UE_LOG( LogInternationalizationExportCommandlet, Error, TEXT("Failed to write archive to %s."), *ArchiveFileName );				
				return false;
			}
		}
	}

	return true;
}

int32 UInternationalizationExportCommandlet::Main( const FString& Params )
{
	TArray<FString> Tokens;
	TArray<FString> Switches;
	TMap<FString, FString> ParamVals;


	UCommandlet::ParseCommandLine(*Params, Tokens, Switches, ParamVals);
	
	const FString* ParamVal = ParamVals.Find(FString(TEXT("Config")));

	if ( ParamVal )
	{
		ConfigPath = *ParamVal;
	}
	else
	{
		UE_LOG(LogInternationalizationExportCommandlet, Error, TEXT("No config specified."));
		return -1;
	}

	//Set config section
	ParamVal = ParamVals.Find(FString(TEXT("Section")));
	

	if ( ParamVal )
	{
		SectionName = *ParamVal;
	}
	else
	{
		UE_LOG(LogInternationalizationExportCommandlet, Error, TEXT("No config section specified."));
		return -1;
	}


	FString SourcePath; // Source path to the root folder that manifest/archive files live in.
	if( !GetPathFromConfig( *SectionName, TEXT("SourcePath"), SourcePath, ConfigPath ) )
	{
		UE_LOG( LogInternationalizationExportCommandlet, Error, TEXT("No source path specified.") );
		return -1;
	}

	FString DestinationPath; // Destination path that we will write files to.
	if( !GetPathFromConfig( *SectionName, TEXT("DestinationPath"), DestinationPath, ConfigPath ) )
	{
		UE_LOG( LogInternationalizationExportCommandlet, Error, TEXT("No destination path specified.") );
		return -1;
	}

	FString Filename; // Name of the file to read or write from
	if (!GetStringFromConfig(*SectionName, TEXT("PortableObjectName"), Filename, ConfigPath))
	{
		UE_LOG(LogInternationalizationExportCommandlet, Error, TEXT("No portable object name specified."));
		return -1;
	}

	// Get cultures to generate.
	if( GetStringArrayFromConfig(*SectionName, TEXT("CulturesToGenerate"), CulturesToGenerate, ConfigPath) == 0 )
	{
		UE_LOG(LogInternationalizationExportCommandlet, Error, TEXT("No cultures specified for generation."));
		return -1;
	}


	bool bDoExport = false;
	bool bDoImport = false;

	GetBoolFromConfig( *SectionName, TEXT("bImportLoc"), bDoImport, ConfigPath );
	GetBoolFromConfig( *SectionName, TEXT("bExportLoc"), bDoExport, ConfigPath );
	
	if( !bDoImport && !bDoExport )
	{
		UE_LOG(LogInternationalizationExportCommandlet, Error, TEXT("Import/Export operation not detected.  Use bExportLoc or bImportLoc in config section."));
		return -1;
	}

	if( bDoImport )
	{
		if (!DoImport(SourcePath, DestinationPath, Filename))
		{
			UE_LOG(LogInternationalizationExportCommandlet, Error, TEXT("Failed to import localization files."));
			return -1;
		}
	}

	if( bDoExport )
	{
		if (!DoExport(SourcePath, DestinationPath, Filename))
		{
			UE_LOG(LogInternationalizationExportCommandlet, Error, TEXT("Failed to export localization files."));
			return -1;
		}
	}

	return 0;
}

