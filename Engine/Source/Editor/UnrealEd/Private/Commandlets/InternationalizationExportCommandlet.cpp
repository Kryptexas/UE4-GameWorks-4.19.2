// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Commandlets/InternationalizationExportCommandlet.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Internationalization/InternationalizationMetadata.h"
#include "Serialization/JsonInternationalizationMetadataSerializer.h"
#include "Localization/PortableObjectFormatDOM.h"
#include "TextNamespaceUtil.h"
#include "LocalizationTargetTypes.h"
#include "UObject/Package.h"

DEFINE_LOG_CATEGORY_STATIC(LogInternationalizationExportCommandlet, Log, All);

static const TCHAR* NewLineDelimiter = TEXT("\n");


/**
*	Helper Functions
*/
bool operator==(const FPortableObjectEntryIdentity& LHS, const FPortableObjectEntryIdentity& RHS)
{
	return LHS.MsgCtxt == RHS.MsgCtxt && LHS.MsgId == RHS.MsgId && LHS.MsgIdPlural == RHS.MsgIdPlural;
}

uint32 GetTypeHash(const FPortableObjectEntryIdentity& ID)
{
	const uint32 HashA = HashCombine(GetTypeHash(ID.MsgCtxt), GetTypeHash(ID.MsgId));
	const uint32 HashB = GetTypeHash(ID.MsgIdPlural);
	return HashCombine(HashA, HashB);
}

namespace
{
	struct FCaseSensitiveStringPair
	{
	public:
		FCaseSensitiveStringPair(FString InFirst, FString InSecond)
			: First(MoveTemp(InFirst))
			, Second(MoveTemp(InSecond))
		{
		}

		FORCEINLINE bool operator==(const FCaseSensitiveStringPair& Other) const
		{
			return First.Equals(Other.First, ESearchCase::CaseSensitive)
				&& Second.Equals(Other.Second, ESearchCase::CaseSensitive);
		}

		FORCEINLINE bool operator!=(const FCaseSensitiveStringPair& Other) const
		{
			return !First.Equals(Other.First, ESearchCase::CaseSensitive)
				|| !Second.Equals(Other.Second, ESearchCase::CaseSensitive);
		}

		friend inline uint32 GetTypeHash(const FCaseSensitiveStringPair& Id)
		{
			uint32 Hash = 0;
			Hash = FCrc::StrCrc32(*Id.First, Hash);
			Hash = FCrc::StrCrc32(*Id.Second, Hash);
			return Hash;
		}

		FString First;
		FString Second;
	};
	typedef TMultiMap<FCaseSensitiveStringPair, FCaseSensitiveStringPair> FCaseSensitiveStringPairMultiMap;

	struct FCollapsedData
	{
		/** Mapping between a collapsed namespace (First) and key (Second), to an expanded namespace (First) and key (Second) */
		FCaseSensitiveStringPairMultiMap CollapsedNSKeyToExpandedNSKey;

		/** Mapping between a collapsed namespace (First) and source string/native translation (Second), to an expanded namespace (First) and key (Second) */
		FCaseSensitiveStringPairMultiMap CollapsedNSSourceStringToExpandedNSKey;
	};

	/**
	 * Declarations
	 */
	FString ConditionIdentityForPOMsgCtxt(const FString& Namespace, const FString& Key, const TSharedPtr<FLocMetadataObject>& KeyMetaData, const ELocalizedTextCollapseMode InTextCollapseMode);
	void ParsePOMsgCtxtForIdentity(const FString& MsgCtxt, FString& OutNamespace, FString& OutKey);
	FString ConditionArchiveStrForPo(const FString& InStr);
	FString ConditionPoStringForArchive(const FString& InStr);
	FString ConvertSrcLocationToPORef(const FString& InSrcLocation);
	FString GetConditionedKeyForExtractedComment(const FString& Key);
	FString GetConditionedReferenceForExtractedComment(const FString& PORefString);
	FString GetConditionedInfoMetaDataForExtractedComment(const FString& KeyName, const FString& ValueString);
	TSharedRef<FInternationalizationManifest> BuildCollapsedManifest(FLocTextHelper& InLocTextHelper, const ELocalizedTextCollapseMode InTextCollapseMode, FCollapsedData& OutCollapsedData);

	/**
	 * Definitions
	 */
	FString ConditionIdentityForPOMsgCtxt(const FString& Namespace, const FString& Key, const TSharedPtr<FLocMetadataObject>& KeyMetaData, const ELocalizedTextCollapseMode InTextCollapseMode)
	{
		const auto& EscapeMsgCtxtParticle = [](const FString& InStr) -> FString
		{
			FString Result;
			for (const TCHAR C : InStr)
			{
				switch (C)
				{
				case TEXT(','):		Result += TEXT("\\,");	break;
				default:			Result += C;			break;
				}
			}
			return Result;
		};

		const FString EscapedNamespace = EscapeMsgCtxtParticle(Namespace);
		const FString EscapedKey = EscapeMsgCtxtParticle(Key);

		const bool bAppendKey = InTextCollapseMode != ELocalizedTextCollapseMode::IdenticalNamespaceAndSource || KeyMetaData.IsValid();
		const FString MsgCtxt = bAppendKey ? FString::Printf(TEXT("%s,%s"), *EscapedNamespace, *EscapedKey) : EscapedNamespace;
		return ConditionArchiveStrForPo(MsgCtxt);
	}

	void ParsePOMsgCtxtForIdentity(const FString& MsgCtxt, FString& OutNamespace, FString& OutKey)
	{
		const FString ConditionedMsgCtxt = ConditionPoStringForArchive(MsgCtxt);

		static const int32 OutputBufferCount = 2;
		FString* OutputBuffers[OutputBufferCount] = { &OutNamespace, &OutKey };
		int32 OutputBufferIndex = 0;

		FString EscapeSequenceBuffer;

		auto HandleEscapeSequenceBuffer = [&]()
		{
			// Insert unescaped sequence if needed.
			if (!EscapeSequenceBuffer.IsEmpty())
			{
				bool EscapeSequenceIdentified = true;

				// Identify escape sequence
				TCHAR UnescapedCharacter = 0;
				if (EscapeSequenceBuffer == TEXT("\\,"))
				{
					UnescapedCharacter = ',';
				}
				else
				{
					EscapeSequenceIdentified = false;
				}

				// If identified, append the processed sequence as the unescaped character.
				if (EscapeSequenceIdentified)
				{
					*OutputBuffers[OutputBufferIndex] += UnescapedCharacter;
				}
				// If it was not identified, preserve the escape sequence and append it.
				else
				{
					*OutputBuffers[OutputBufferIndex] += EscapeSequenceBuffer;
				}
				// Either way, we've appended something based on the buffer and it should be reset.
				EscapeSequenceBuffer.Empty();
			}
		};

		for (const TCHAR C : ConditionedMsgCtxt)
		{
			// If we're out of buffers, break out. The particle list is longer than expected.
			if (OutputBufferIndex >= OutputBufferCount)
			{
				UE_LOG( LogInternationalizationExportCommandlet, Warning, TEXT("msgctxt found in PO has too many parts: %s"), *ConditionedMsgCtxt );
				break;
			}

			// Not in an escape sequence.
			if (EscapeSequenceBuffer.IsEmpty())
			{
				// Comma marks the delimiter between namespace and key, if present.
				if(C == TEXT(','))
				{
					++OutputBufferIndex;
				}
				// Regular character, just copy over.
				else if (C != TEXT('\\'))
				{
					*OutputBuffers[OutputBufferIndex] += C;
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
	}

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

	FString GetConditionedKeyForExtractedComment(const FString& Key)
	{
		return FString::Printf(TEXT("Key:\t%s"), *Key);
	}

	FString GetConditionedReferenceForExtractedComment(const FString& PORefString)
	{
		return FString::Printf(TEXT("SourceLocation:\t%s"), *PORefString);
	}

	FString GetConditionedInfoMetaDataForExtractedComment(const FString& KeyName, const FString& ValueString)
	{
		return FString::Printf(TEXT("InfoMetaData:\t\"%s\" : \"%s\""), *KeyName, *ValueString);
	}

	TSharedRef<FInternationalizationManifest> BuildCollapsedManifest(FLocTextHelper& InLocTextHelper, const ELocalizedTextCollapseMode InTextCollapseMode, FCollapsedData& OutCollapsedData)
	{
		TSharedRef<FInternationalizationManifest> CollapsedManifest = MakeShared<FInternationalizationManifest>();

		InLocTextHelper.EnumerateSourceTexts([&](TSharedRef<FManifestEntry> InManifestEntry) -> bool
		{
			const FString CollapsedNamespace = InTextCollapseMode == ELocalizedTextCollapseMode::IdenticalPackageIdTextIdAndSource ? InManifestEntry->Namespace : TextNamespaceUtil::StripPackageNamespace(InManifestEntry->Namespace);

			for (const FManifestContext& Context : InManifestEntry->Contexts)
			{
				bool bAddedContext = false;

				// Check if the entry already exists in the manifest
				TSharedPtr<FManifestEntry> ExistingEntry = CollapsedManifest->FindEntryByContext(CollapsedNamespace, Context);
				if (ExistingEntry.IsValid())
				{
					if (InManifestEntry->Source.IsExactMatch(ExistingEntry->Source))
					{
						bAddedContext = true;
					}
					else
					{
						// Grab the source location of the conflicting context
						const FManifestContext* ConflictingContext = ExistingEntry->FindContext(Context.Key, Context.KeyMetadataObj);

						const FString Message = FLocTextHelper::SanitizeLogOutput(
							FString::Printf(TEXT("Found previously entered localized string: %s [%s] %s %s=\"%s\" %s. It was previously \"%s\" %s in %s."),
								*Context.SourceLocation,
								*CollapsedNamespace,
								*Context.Key,
								*FJsonInternationalizationMetaDataSerializer::MetadataToString(Context.KeyMetadataObj),
								*InManifestEntry->Source.Text,
								*FJsonInternationalizationMetaDataSerializer::MetadataToString(InManifestEntry->Source.MetadataObj),
								*ExistingEntry->Source.Text,
								*FJsonInternationalizationMetaDataSerializer::MetadataToString(ExistingEntry->Source.MetadataObj),
								*ConflictingContext->SourceLocation
							)
						);
						UE_LOG(LogInternationalizationExportCommandlet, Warning, TEXT("%s"), *Message);

						InLocTextHelper.AddConflict(CollapsedNamespace, Context.Key, Context.KeyMetadataObj, InManifestEntry->Source, Context.SourceLocation);
						InLocTextHelper.AddConflict(CollapsedNamespace, Context.Key, Context.KeyMetadataObj, ExistingEntry->Source, ConflictingContext->SourceLocation);
					}
				}
				else
				{
					if (CollapsedManifest->AddSource(CollapsedNamespace, InManifestEntry->Source, Context))
					{
						bAddedContext = true;
					}
					else
					{
						UE_LOG(LogInternationalizationExportCommandlet, Error, TEXT("Could not process localized string: %s [%s] %s=\"%s\" %s."),
							*Context.SourceLocation,
							*CollapsedNamespace,
							*Context.Key,
							*InManifestEntry->Source.Text,
							*FJsonInternationalizationMetaDataSerializer::MetadataToString(InManifestEntry->Source.MetadataObj)
						);
					}
				}

				if (bAddedContext)
				{
					// Add this collapsed namespace/key pair to our mapping so we can expand it again during import
					OutCollapsedData.CollapsedNSKeyToExpandedNSKey.AddUnique(FCaseSensitiveStringPair(CollapsedNamespace, Context.Key), FCaseSensitiveStringPair(InManifestEntry->Namespace, Context.Key));

					// Add this collapsed namespace/source string pair to our mapping so we expand it again during import (also map it against any native "translation" as that's what foreign imports will use as their source for translations)
					if (!Context.KeyMetadataObj.IsValid())
					{
						OutCollapsedData.CollapsedNSSourceStringToExpandedNSKey.AddUnique(FCaseSensitiveStringPair(CollapsedNamespace, InManifestEntry->Source.Text), FCaseSensitiveStringPair(InManifestEntry->Namespace, Context.Key));

						if (InLocTextHelper.HasNativeArchive())
						{
							TSharedPtr<FArchiveEntry> NativeTranslation = InLocTextHelper.FindTranslation(InLocTextHelper.GetNativeCulture(), InManifestEntry->Namespace, Context.Key, nullptr);
							if (NativeTranslation.IsValid() && !NativeTranslation->Translation.Text.Equals(InManifestEntry->Source.Text))
							{
								OutCollapsedData.CollapsedNSSourceStringToExpandedNSKey.AddUnique(FCaseSensitiveStringPair(CollapsedNamespace, NativeTranslation->Translation.Text), FCaseSensitiveStringPair(InManifestEntry->Namespace, Context.Key));
							}
						}
					}
				}
			}

			return true; // continue enumeration
		}, true);

		return CollapsedManifest;
	}
}

/**
*	UInternationalizationExportCommandlet
*/
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

	// We may only have a single culture if using this setting
	if (!bUseCultureDirectory && CulturesToGenerate.Num() > 1)
	{
		UE_LOG( LogInternationalizationExportCommandlet, Error, TEXT("bUseCultureDirectory may only be used with a single culture.") );
		return false;
	}

	// Read in the text collapse mode to use
	ELocalizedTextCollapseMode TextCollapseMode = ELocalizedTextCollapseMode::IdenticalTextIdAndSource;
	{
		FString TextCollapseModeName;
		if (GetStringFromConfig(*SectionName, TEXT("LocalizedTextCollapseMode"), TextCollapseModeName, ConfigPath))
		{
			UEnum* LocalizedTextCollapseModeEnum = FindObjectChecked<UEnum>(ANY_PACKAGE, TEXT("ELocalizedTextCollapseMode"));
			const int64 TextCollapseModeInt = LocalizedTextCollapseModeEnum->GetValueByName(*TextCollapseModeName);
			if (TextCollapseModeInt != INDEX_NONE)
			{
				TextCollapseMode = (ELocalizedTextCollapseMode)TextCollapseModeInt;
			}
		}
	}

	bool ShouldAddSourceLocationsAsComments = true;
	GetBoolFromConfig(*SectionName, TEXT("ShouldAddSourceLocationsAsComments"), ShouldAddSourceLocationsAsComments, ConfigPath);

	// Load the manifest and all archives
	FLocTextHelper LocTextHelper(SourcePath, ManifestName, ArchiveName, NativeCultureName, CulturesToGenerate, MakeShareable(new FLocFileSCCNotifies(SourceControlInfo)));
	{
		FText LoadError;
		if (!LocTextHelper.LoadAll(ELocTextHelperLoadFlags::LoadOrCreate, &LoadError))
		{
			UE_LOG(LogInternationalizationExportCommandlet, Error, TEXT("%s"), *LoadError.ToString());
			return false;
		}
	}

	// Build the collapsed manifest data to export
	FCollapsedData CollapsedData;
	TSharedRef<FInternationalizationManifest> CollapsedManifest = BuildCollapsedManifest(LocTextHelper, TextCollapseMode, CollapsedData);

	// Process the desired cultures
	for (const FString& CultureName : CulturesToGenerate)
	{
		FPortableObjectFormatDOM NewPortableObject;

		FString LocLang;
		if( !NewPortableObject.SetLanguage( CultureName ) )
		{
			UE_LOG( LogInternationalizationExportCommandlet, Error, TEXT("Skipping export of loc language %s because it is not recognized."), *LocLang );
			continue;
		}

		NewPortableObject.SetProjectName( FPaths::GetBaseFilename( ManifestName ) );
		NewPortableObject.CreateNewHeader();

		// Add each manifest entry to the PO file
		for (FManifestEntryByStringContainer::TConstIterator ManifestIterator = CollapsedManifest->GetEntriesByKeyIterator(); ManifestIterator; ++ManifestIterator)
		{
			const TSharedRef<FManifestEntry> ManifestEntry = ManifestIterator.Value();

			// For each context, we may need to create a different or even multiple PO entries.
			for (const FManifestContext& Context : ManifestEntry->Contexts)
			{
				TSharedRef<FPortableObjectEntry> PoEntry = MakeShareable(new FPortableObjectEntry());

				// For export we just use the first expanded namespace/key pair to find the current translation (they should all be identical due to how the import works)
				const FCaseSensitiveStringPair& ExportNamespaceKeyPair = CollapsedData.CollapsedNSKeyToExpandedNSKey.FindChecked(FCaseSensitiveStringPair(ManifestEntry->Namespace, Context.Key));

				// Find the correct translation based upon the native source text
				FLocItem ExportedSource;
				FLocItem ExportedTranslation;
				LocTextHelper.GetExportText(CultureName, ExportNamespaceKeyPair.First, ExportNamespaceKeyPair.Second, Context.KeyMetadataObj, ELocTextExportSourceMethod::NativeText, ManifestEntry->Source, ExportedSource, ExportedTranslation);

				PoEntry->MsgId = ConditionArchiveStrForPo(ExportedSource.Text);
				PoEntry->MsgCtxt = ConditionIdentityForPOMsgCtxt(ManifestEntry->Namespace, Context.Key, Context.KeyMetadataObj, TextCollapseMode);
				PoEntry->MsgStr.Add(ConditionArchiveStrForPo(ExportedTranslation.Text));

				//@TODO: We support additional metadata entries that can be translated.  How do those fit in the PO file format?  Ex: isMature
				const FString PORefString = ConvertSrcLocationToPORef(Context.SourceLocation);
				PoEntry->AddReference(PORefString); // Source location.

				PoEntry->AddExtractedComment(FString::Printf(TEXT("Key:\t%s"), *Context.Key)); // "Notes from Programmer" in the form of the Key.
				PoEntry->AddExtractedComment(FString::Printf(TEXT("SourceLocation:\t%s"), *PORefString)); // "Notes from Programmer" in the form of the Source Location, since this comes in handy too and OneSky doesn't properly show references, only comments.

				TArray<FString> InfoMetaDataStrings;
				if (Context.InfoMetadataObj.IsValid())
				{
					for (auto InfoMetaDataPair : Context.InfoMetadataObj->Values)
					{
						const FString KeyName = InfoMetaDataPair.Key;
						const TSharedPtr<FLocMetadataValue> Value = InfoMetaDataPair.Value;
						InfoMetaDataStrings.Add(FString::Printf(TEXT("InfoMetaData:\t\"%s\" : \"%s\""), *KeyName, *Value->ToString()));
					}
				}
				if (InfoMetaDataStrings.Num())
				{
					PoEntry->AddExtractedComments(InfoMetaDataStrings);
				}

				NewPortableObject.AddEntry(PoEntry);
			}
		}

		// Write out the Portable Object to .po file.
		{
			FString OutputFileName;
			if (bUseCultureDirectory)
			{
				OutputFileName = DestinationPath / CultureName / Filename;
			}
			else
			{
				OutputFileName = DestinationPath / Filename;
			}

			// Persist comments if requested.
			if (ShouldPersistComments)
			{
				// Preserve comments from the specified file now, if they haven't already been.
				if (!HasPreservedComments)
				{
					FPortableObjectFormatDOM ExistingPortableObject;
					const bool HasLoadedPOFile = LoadPOFile(OutputFileName, ExistingPortableObject);
					if (HasLoadedPOFile)
					{
						PreserveExtractedCommentsForPersistence(ExistingPortableObject);
					}
				}

				// Persist the comments into the new portable object we're going to be saving.
				for (const auto& Pair : POEntryToCommentMap)
				{
					const TSharedPtr<FPortableObjectEntry> FoundEntry = NewPortableObject.FindEntry(Pair.Key.MsgId, Pair.Key.MsgIdPlural, Pair.Key.MsgCtxt);
					if (FoundEntry.IsValid())
					{
						FoundEntry->AddExtractedComments(Pair.Value);
					}
				}
			}

			NewPortableObject.SortEntries();

			const bool bPOFileSaved = FLocalizedAssetSCCUtil::SaveFileWithSCC(SourceControlInfo, OutputFileName, [&NewPortableObject](const FString& InSaveFileName) -> bool
			{
				//@TODO We force UTF8 at the moment but we want this to be based on the format found in the header info.
				const FString OutputString = NewPortableObject.ToString();
				return FFileHelper::SaveStringToFile(OutputString, *InSaveFileName, FFileHelper::EEncodingOptions::ForceUTF8);
			});

			if (!bPOFileSaved)
			{
				UE_LOG(LogInternationalizationExportCommandlet, Error, TEXT("Could not write file %s"), *OutputFileName);
				return false;
			}
		}
	}

	return true;
}

bool UInternationalizationExportCommandlet::DoImport(const FString& SourcePath, const FString& DestinationPath, const FString& Filename)
{
	// Get native culture.
	FString NativeCultureName;
	if (!GetStringFromConfig(*SectionName, TEXT("NativeCulture"), NativeCultureName, ConfigPath))
	{
		UE_LOG(LogInternationalizationExportCommandlet, Error, TEXT("No native culture specified."));
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

	// Get culture directory setting, default to true if not specified (used to allow picking of import directory with file open dialog from Translation Editor)
	bool bUseCultureDirectory = true;
	if (!(GetBoolFromConfig(*SectionName, TEXT("bUseCultureDirectory"), bUseCultureDirectory, ConfigPath)))
	{
		bUseCultureDirectory = true;
	}

	// We may only have a single culture if using this setting
	if (!bUseCultureDirectory && CulturesToGenerate.Num() > 1)
	{
		UE_LOG(LogInternationalizationExportCommandlet, Error, TEXT("bUseCultureDirectory may only be used with a single culture."));
		return false;
	}

	// Read in the text collapse mode to use
	ELocalizedTextCollapseMode TextCollapseMode = ELocalizedTextCollapseMode::IdenticalTextIdAndSource;
	{
		FString TextCollapseModeName;
		if (GetStringFromConfig(*SectionName, TEXT("LocalizedTextCollapseMode"), TextCollapseModeName, ConfigPath))
		{
			UEnum* LocalizedTextCollapseModeEnum = FindObjectChecked<UEnum>(ANY_PACKAGE, TEXT("ELocalizedTextCollapseMode"));
			const int64 TextCollapseModeInt = LocalizedTextCollapseModeEnum->GetValueByName(*TextCollapseModeName);
			if (TextCollapseModeInt != INDEX_NONE)
			{
				TextCollapseMode = (ELocalizedTextCollapseMode)TextCollapseModeInt;
			}
		}
	}

	// Load the manifest and all archives
	FLocTextHelper LocTextHelper(DestinationPath, ManifestName, ArchiveName, NativeCultureName, CulturesToGenerate, MakeShareable(new FLocFileSCCNotifies(SourceControlInfo)));
	{
		FText LoadError;
		if (!LocTextHelper.LoadAll(ELocTextHelperLoadFlags::LoadOrCreate, &LoadError))
		{
			UE_LOG(LogInternationalizationExportCommandlet, Error, TEXT("%s"), *LoadError.ToString());
			return false;
		}
	}

	// Build the collapsed manifest data needed to import
	FCollapsedData CollapsedData;
	TSharedRef<FInternationalizationManifest> CollapsedManifest = BuildCollapsedManifest(LocTextHelper, TextCollapseMode, CollapsedData);

	// Process the desired cultures
	for (const FString& CultureName : CulturesToGenerate)
	{
		// Load the Portable Object file if found
		FString POFilePath;
		if (bUseCultureDirectory)
		{
			POFilePath = SourcePath / CultureName / Filename;
		}
		else
		{
			POFilePath = SourcePath / Filename;
		}

		FPortableObjectFormatDOM PortableObject;
		const bool HasLoadedPOFile = LoadPOFile(POFilePath, PortableObject);
		if (!HasLoadedPOFile)
		{
			continue;
		}

		if (ShouldPersistComments)
		{
			PreserveExtractedCommentsForPersistence(PortableObject);
		}

		bool bModifiedArchive = false;
		{
			for (auto EntryIter = PortableObject.GetEntriesIterator(); EntryIter; ++EntryIter)
			{
				auto POEntry = *EntryIter;
				if (POEntry->MsgId.IsEmpty() || POEntry->MsgStr.Num() == 0 || POEntry->MsgStr[0].Trim().IsEmpty())
				{
					// We ignore the header entry or entries with no translation.
					continue;
				}

				// Some warning messages for data we don't process at the moment
				if (!POEntry->MsgIdPlural.IsEmpty() || POEntry->MsgStr.Num() > 1)
				{
					UE_LOG(LogInternationalizationExportCommandlet, Error, TEXT("Portable Object entry has plural form we did not process.  File: %s  MsgCtxt: %s  MsgId: %s"), *POFilePath, *POEntry->MsgCtxt, *POEntry->MsgId);
				}

				const FString SourceText = ConditionPoStringForArchive(POEntry->MsgId);
				const FString Translation = ConditionPoStringForArchive(POEntry->MsgStr[0]);

				TArray<FCaseSensitiveStringPair> NamespacesAndKeys; // Namespace (First) and Key (Second)
				{
					FString ParsedNamespace;
					FString ParsedKey;
					ParsePOMsgCtxtForIdentity(POEntry->MsgCtxt, ParsedNamespace, ParsedKey);

					if (ParsedKey.IsEmpty())
					{
						// Legacy non-keyed PO entry - need to look-up the expanded namespace key/pairs via the namespace and source string
						CollapsedData.CollapsedNSSourceStringToExpandedNSKey.MultiFind(FCaseSensitiveStringPair(ParsedNamespace, SourceText), NamespacesAndKeys);
					}
					else
					{
						// Keyed PO entry - need to look-up the expanded namespace/key pairs via the namespace and key
						CollapsedData.CollapsedNSKeyToExpandedNSKey.MultiFind(FCaseSensitiveStringPair(ParsedNamespace, ParsedKey), NamespacesAndKeys);
					}
				}

				if (NamespacesAndKeys.Num() == 0)
				{
					UE_LOG(LogInternationalizationExportCommandlet, Log, TEXT("Could not import PO entry as it did not map to any known entries in the collapsed manifest data.  File: %s  MsgCtxt: %s  MsgId: %s"), *POFilePath, *POEntry->MsgCtxt, *POEntry->MsgId);
					continue;
				}
				
				for (const FCaseSensitiveStringPair& NamespaceAndKey : NamespacesAndKeys)
				{
					// Alias for convenience of reading
					const FString& Namespace = NamespaceAndKey.First;
					const FString& Key = NamespaceAndKey.Second;

					// Get key metadata from the manifest, using the namespace and key.
					const FManifestContext* ItemContext = nullptr;
					{
						// Find manifest entry by namespace and key
						TSharedPtr<FManifestEntry> ManifestEntry = LocTextHelper.FindSourceText(Namespace, Key);
						if (ManifestEntry.IsValid())
						{
							ItemContext = ManifestEntry->FindContextByKey(Key);
						}
					}

					//@TODO: Take into account optional entries and entries that differ by keymetadata.  Ex. Each optional entry needs a unique msgCtxt

					// Attempt to import the new text (if required)
					const TSharedPtr<FArchiveEntry> FoundEntry = LocTextHelper.FindTranslation(CultureName, Namespace, Key, ItemContext ? ItemContext->KeyMetadataObj : nullptr);
					if (!FoundEntry.IsValid() || !FoundEntry->Source.Text.Equals(SourceText, ESearchCase::CaseSensitive) || !FoundEntry->Translation.Text.Equals(Translation, ESearchCase::CaseSensitive))
					{
						if (LocTextHelper.ImportTranslation(CultureName, Namespace, Key, ItemContext ? ItemContext->KeyMetadataObj : nullptr, FLocItem(SourceText), FLocItem(Translation), ItemContext && ItemContext->bIsOptional))
						{
							bModifiedArchive = true;
						}
					}
				}
			}
		}

		if (bModifiedArchive)
		{
			// Trim any dead entries out of the archive
			LocTextHelper.TrimArchive(CultureName);

			FText SaveError;
			if (!LocTextHelper.SaveForeignArchive(CultureName, &SaveError))
			{
				UE_LOG(LogInternationalizationExportCommandlet, Error, TEXT("%s"), *SaveError.ToString());
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
	ShouldPersistComments = false;

	GetBoolFromConfig( *SectionName, TEXT("bImportLoc"), bDoImport, ConfigPath );
	GetBoolFromConfig( *SectionName, TEXT("bExportLoc"), bDoExport, ConfigPath );
	GetBoolFromConfig(*SectionName, TEXT("ShouldPersistCommentsOnExport"), ShouldPersistComments, ConfigPath);

	// Reject the ShouldPersistComments flag and warn if not exporting - we're not writing to anything, so we can't persist.
	if (ShouldPersistComments && !bDoExport)
	{
		UE_LOG(LogInternationalizationExportCommandlet, Warning, TEXT("ShouldPersistComments is true, but bExportLoc is false - can't persist comments if not writing PO files."));
		ShouldPersistComments = false;
	}

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

bool UInternationalizationExportCommandlet::LoadPOFile(const FString& POFilePath, FPortableObjectFormatDOM& OutPortableObject)
{
	if (!FPaths::FileExists(POFilePath))
	{
		UE_LOG(LogInternationalizationExportCommandlet, Log, TEXT("Could not find file %s"), *POFilePath);
		return false;
	}

	FString POFileContents;
	if (!FFileHelper::LoadFileToString(POFileContents, *POFilePath))
	{
		UE_LOG(LogInternationalizationExportCommandlet, Error, TEXT("Failed to load file %s."), *POFilePath);
		return false;
	}

	if (!OutPortableObject.FromString(POFileContents))
	{
		UE_LOG(LogInternationalizationExportCommandlet, Error, TEXT("Failed to parse Portable Object file %s."), *POFilePath);
		return false;
	}

	return true;
}

void UInternationalizationExportCommandlet::PreserveExtractedCommentsForPersistence(FPortableObjectFormatDOM& PortableObject)
{
	// Preserve comments for later.
	for (auto EntriesIterator = PortableObject.GetEntriesIterator(); EntriesIterator; ++EntriesIterator)
	{
		const TSharedPtr< FPortableObjectEntry >& Entry = *EntriesIterator;

		// Preserve only non-procedurally generated extracted comments.
		const TArray<FString> CommentsToPreserve = Entry->ExtractedComments.FilterByPredicate([=](const FString& ExtractedComment) -> bool
		{
			return !ExtractedComment.StartsWith("Key:") && !ExtractedComment.StartsWith("SourceLocation:") && !ExtractedComment.StartsWith("InfoMetaData:");
		});

		if (CommentsToPreserve.Num())
		{
			POEntryToCommentMap.Add(FPortableObjectEntryIdentity{ Entry->MsgCtxt, Entry->MsgId, Entry->MsgIdPlural }, CommentsToPreserve);
		}
	}
	HasPreservedComments = true;
}
