// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AddContentDialogPCH.h"
#include "FeaturePackContentSource.h"

#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "IPlatformFilePak.h"
#include "FileHelpers.h"
#include "Editor/MainFrame/Public/Interfaces/IMainFrameModule.h"
#include "SuperSearchModule.h"

#define LOCTEXT_NAMESPACE "ContentFeaturePacks"

DEFINE_LOG_CATEGORY_STATIC(LogFeaturePack, Log, All);

bool TryValidateTranslatedValue(TSharedPtr<FJsonValue> TranslatedValue, TSharedPtr<FString>& ErrorMessage)
{
	if (TranslatedValue.IsValid() == false)
	{
		ErrorMessage = MakeShareable(new FString("Invalid translated value"));
		return false;
	}

	const TSharedPtr<FJsonObject>* TranslatedObject;
	if (TranslatedValue->TryGetObject(TranslatedObject) == false)
	{
		ErrorMessage = MakeShareable(new FString("Invalid translated value"));
		return false;
	}

	if ((*TranslatedObject)->HasTypedField<EJson::String>("Language") == false)
	{
		ErrorMessage = MakeShareable(new FString("Translated value missing 'Language' field"));
		return false;
	}

	if ((*TranslatedObject)->HasTypedField<EJson::String>("Text") == false)
	{
		ErrorMessage = MakeShareable(new FString("Translated value missing 'Text' field"));
		return false;
	}

	return true;
}

bool TryValidateManifestObject(TSharedPtr<FJsonObject> ManifestObject, TSharedPtr<FString>& ErrorMessage)
{
	if (ManifestObject.IsValid() == false)
	{
		ErrorMessage = MakeShareable(new FString("Manifest object missing"));
		return false;
	}

	if (ManifestObject->HasTypedField<EJson::Array>("Name") == false)
	{
		ErrorMessage = MakeShareable(new FString("Manifest object missing 'Names' field"));
		return false;
	}

	for (TSharedPtr<FJsonValue> NameValue : ManifestObject->GetArrayField("Name"))
	{
		if (TryValidateTranslatedValue(NameValue, ErrorMessage) == false)
		{
			ErrorMessage = MakeShareable(new FString(FString::Printf(TEXT("Manifest object 'Names' field error: %s"), **ErrorMessage)));
		}
	}

	if (ManifestObject->HasTypedField<EJson::Array>("Description") == false)
	{
		ErrorMessage = MakeShareable(new FString("Manifest object missing 'Description' field"));
		return false;
	}

	for (TSharedPtr<FJsonValue> DescriptionValue : ManifestObject->GetArrayField("Description"))
	{
		if (TryValidateTranslatedValue(DescriptionValue, ErrorMessage) == false)
		{
			ErrorMessage = MakeShareable(new FString(FString::Printf(TEXT("Manifest object 'Description' field error: %s"), **ErrorMessage)));
		}
	}

	if (ManifestObject->HasTypedField<EJson::Array>("AssetTypes") == false)
	{
		ErrorMessage = MakeShareable(new FString("Manifest object missing 'AssetTypes' field"));
		return false;
	}

	for (TSharedPtr<FJsonValue> AssetTypesValue : ManifestObject->GetArrayField("AssetTypes"))
	{
		if (TryValidateTranslatedValue(AssetTypesValue, ErrorMessage) == false)
		{
			ErrorMessage = MakeShareable(new FString(FString::Printf(TEXT("Manifest object 'AssetTypes' field error: %s"), **ErrorMessage)));
		}
	}

	if (ManifestObject->HasTypedField<EJson::String>("ClassTypes") == false)
	{
		ErrorMessage = MakeShareable(new FString("Manifest object missing 'ClassTypes' field"));
		return false;
	}

	if (ManifestObject->HasTypedField<EJson::String>("Category") == false)
	{
		ErrorMessage = MakeShareable(new FString("Manifest object missing 'Category' field"));
		return false;
	}
		
	if (ManifestObject->HasTypedField<EJson::String>("Thumbnail") == false)
	{
		ErrorMessage = MakeShareable(new FString("Manifest object missing 'Thumbnail' field"));
		return false;
	}

	if (ManifestObject->HasTypedField<EJson::Array>("Screenshots") == false)
	{
		ErrorMessage = MakeShareable(new FString("Manifest object missing 'Screenshots' field"));
		return false;
	}

	return true;
}

FFeaturePackContentSource::FFeaturePackContentSource(FString InFeaturePackPath, bool bDontRegisterForSearch)
{
	FeaturePackPath = InFeaturePackPath;
	bPackValid = false;
	// Create a pak platform file and mount the feature pack file.
	FPakPlatformFile PakPlatformFile;
	PakPlatformFile.Initialize(&FPlatformFileManager::Get().GetPlatformFile(), TEXT(""));
	FString MountPoint = "root:/";
	PakPlatformFile.Mount(*InFeaturePackPath, 0, *MountPoint);
	
	// Gets the manifest file as a JSon string
	TArray<uint8> ManifestBuffer;
	if( LoadPakFileToBuffer(PakPlatformFile, FPaths::Combine(*MountPoint, TEXT("manifest.json")), ManifestBuffer) == false )
	{
		RecordAndLogError( FString::Printf(TEXT("Error in Feature pack %s. Cannot find manifest."), *InFeaturePackPath));
		Category = EContentSourceCategory::Unknown;
		return;
	}
	FString ManifestString;
	FFileHelper::BufferToString(ManifestString, ManifestBuffer.GetData(), ManifestBuffer.Num());

	// Populate text fields from the manifest.
	TSharedPtr<FJsonObject> ManifestObject;
	TSharedRef<TJsonReader<>> ManifestReader = TJsonReaderFactory<>::Create(ManifestString);
	FJsonSerializer::Deserialize(ManifestReader, ManifestObject);

	if (ManifestReader->GetErrorMessage().IsEmpty() == false)
	{
		RecordAndLogError( FString::Printf(TEXT("Error in Feature pack %s. Failed to parse manifest: %s"), *InFeaturePackPath, *ManifestReader->GetErrorMessage()));
		Category = EContentSourceCategory::Unknown;
		return;
	}

	TSharedPtr<FString> ManifestObjectErrorMessage;
	if (TryValidateManifestObject(ManifestObject, ManifestObjectErrorMessage) == false)
	{
		RecordAndLogError( FString::Printf(TEXT("Error in Feature pack %s. Manifest object error: %s"), *InFeaturePackPath, **ManifestObjectErrorMessage));
		Category = EContentSourceCategory::Unknown;
		return;
	}

	for (TSharedPtr<FJsonValue> NameValue : ManifestObject->GetArrayField("Name"))
	{
		TSharedPtr<FJsonObject> LocalizedNameObject = NameValue->AsObject();
		LocalizedNames.Add(FLocalizedText(
			LocalizedNameObject->GetStringField("Language"),
			FText::FromString(LocalizedNameObject->GetStringField("Text"))));
	}

	for (TSharedPtr<FJsonValue> DescriptionValue : ManifestObject->GetArrayField("Description"))
	{
		TSharedPtr<FJsonObject> LocalizedDescriptionObject = DescriptionValue->AsObject();
		LocalizedDescriptions.Add(FLocalizedText(
			LocalizedDescriptionObject->GetStringField("Language"),
			FText::FromString(LocalizedDescriptionObject->GetStringField("Text"))));
	}

	// Parse asset types field
	for (TSharedPtr<FJsonValue> AssetTypesValue : ManifestObject->GetArrayField("AssetTypes"))
	{
		TSharedPtr<FJsonObject> LocalizedAssetTypesObject = AssetTypesValue->AsObject();
		LocalizedAssetTypesList.Add(FLocalizedText(
			LocalizedAssetTypesObject->GetStringField("Language"),
			FText::FromString(LocalizedAssetTypesObject->GetStringField("Text"))));
	}
	
	// Parse search tags field
	if( ManifestObject->HasField("SearchTags")==true)
	{
		for (TSharedPtr<FJsonValue> AssetTypesValue : ManifestObject->GetArrayField("SearchTags"))
		{
			TSharedPtr<FJsonObject> LocalizedAssetTypesObject = AssetTypesValue->AsObject();
			LocalizedSearchTags.Add(FLocalizedTextArray(
				LocalizedAssetTypesObject->GetStringField("Language"),
				LocalizedAssetTypesObject->GetStringField("Text")));
		}
	}

	// Parse class types field
	ClassTypes = ManifestObject->GetStringField("ClassTypes");	
	
	// Parse initial focus asset if we have one - this is not required
	if (ManifestObject->HasTypedField<EJson::String>("FocusAsset") == true)
	{
		FocusAssetIdent = ManifestObject->GetStringField("FocusAsset");
	}	

	// Use the path as the sort key - it will be alphabetical that way
	SortKey = FeaturePackPath;
	ManifestObject->TryGetStringField("SortKey", SortKey);

	FString CategoryString = ManifestObject->GetStringField("Category");	
	UEnum* Enum = FindObjectChecked<UEnum>(ANY_PACKAGE, TEXT("EContentSourceCategory"));
	int32 Index = Enum->FindEnumIndex(FName(*CategoryString));
	Category = Index != INDEX_NONE ? (EContentSourceCategory)Index : EContentSourceCategory::Unknown;

	// Load image data
	FString IconFilename = ManifestObject->GetStringField("Thumbnail");
	TSharedPtr<TArray<uint8>> IconImageData = MakeShareable(new TArray<uint8>());
	FString ThumbnailFile = FPaths::Combine(*MountPoint, TEXT("Media"), *IconFilename);
	if( LoadPakFileToBuffer(PakPlatformFile, ThumbnailFile, *IconImageData) == true)
	{
		IconData = MakeShareable(new FImageData(IconFilename, IconImageData));
	}
	else
	{
		RecordAndLogError( FString::Printf(TEXT("Error in Feature pack %s. Cannot find thumbnail %s."), *InFeaturePackPath, *ThumbnailFile ));
	}

	// parse the screenshots field
	const TArray<TSharedPtr<FJsonValue>> ScreenshotFilenameArray = ManifestObject->GetArrayField("Screenshots");
	for (const TSharedPtr<FJsonValue> ScreenshotFilename : ScreenshotFilenameArray)
	{
		TSharedPtr<TArray<uint8>> SingleScreenshotData = MakeShareable(new TArray<uint8>);
		if( LoadPakFileToBuffer(PakPlatformFile, FPaths::Combine(*MountPoint, TEXT("Media"), *ScreenshotFilename->AsString()), *SingleScreenshotData) )
		{
			ScreenshotData.Add(MakeShareable(new FImageData(ScreenshotFilename->AsString(), SingleScreenshotData)));
		}
		else
		{
			RecordAndLogError( FString::Printf(TEXT("Error in Feature pack %s. Cannot find screenshot %s."), *InFeaturePackPath, *ScreenshotFilename->AsString() ));
		}
	}

	// Parse additional packs data
	if (ManifestObject->HasTypedField<EJson::Array>("AdditionalFeaturePacks") == true)
	{		
		UEnum* DetailEnum = FindObjectChecked<UEnum>(ANY_PACKAGE, TEXT("EFeaturePackDetailLevel"));
		for (TSharedPtr<FJsonValue> AdditionalFeaturePackValue : ManifestObject->GetArrayField("AdditionalFeaturePacks"))
		{
			TSharedPtr<FJsonObject> EachAdditionalPack = AdditionalFeaturePackValue->AsObject();
			FString MountName = EachAdditionalPack->GetStringField("MountName");

			TArray<EFeaturePackDetailLevel>	Levels;
			for (TSharedPtr<FJsonValue> DetailValue: EachAdditionalPack->GetArrayField("DetailLevels"))
			{
				const FString DetailString = DetailValue->AsString();
				int32 eIndex = DetailEnum->FindEnumIndex(FName(*DetailString));
				EFeaturePackDetailLevel EachLevel = Index != INDEX_NONE ? (EFeaturePackDetailLevel)eIndex : EFeaturePackDetailLevel::Standard;
				Levels.AddUnique(EachLevel);
			}
			int32 PackIndex = AdditionalFeaturePacks.Add(FFeaturePackLevelSet(
				MountName,
				Levels));
			// Check that the additional feature packs listed exist
			FFeaturePackLevelSet& NewSet = AdditionalFeaturePacks[PackIndex];
			for (int32 iLevel = 0; iLevel < NewSet.DetailLevels.Num() ; iLevel++)
			{
				FString FullPath = FPaths::FeaturePackDir() + NewSet.GetFeaturePackNameForLevel(NewSet.DetailLevels[iLevel]);	
				if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*FullPath))
				{
					RecordAndLogError( FString::Printf(TEXT("Error in Feature pack %s. Cannot find additional pack %s."), *InFeaturePackPath, *FullPath));
				}
			}		
		}
	}

	if( bDontRegisterForSearch == false )
	{
		FSuperSearchModule& SuperSearchModule = FModuleManager::LoadModuleChecked< FSuperSearchModule >(TEXT("SuperSearch"));
		// Remove any existing delegates for this pack
		FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(SearchClickedHandle);
		FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(SearchChangedHandle);
		SearchClickedHandle = SuperSearchModule.GetActOnSearchTextClicked().AddRaw(this, &FFeaturePackContentSource::HandleActOnSearchText);
		SearchChangedHandle = SuperSearchModule.GetSearchTextChanged().AddRaw(this, &FFeaturePackContentSource::HandleSuperSearchTextChanged);
	}
	bPackValid = true;
}

bool FFeaturePackContentSource::LoadPakFileToBuffer(FPakPlatformFile& PakPlatformFile, FString Path, TArray<uint8>& Buffer)
{
	bool bResult = false;
	TSharedPtr<IFileHandle> FileHandle(PakPlatformFile.OpenRead(*Path));
	if( FileHandle.IsValid())
	{
		Buffer.AddUninitialized(FileHandle->Size());
		bResult = FileHandle->Read(Buffer.GetData(), FileHandle->Size());
	}
	return bResult;
}

TArray<FLocalizedText> FFeaturePackContentSource::GetLocalizedNames() const
{
	return LocalizedNames;
}

TArray<FLocalizedText> FFeaturePackContentSource::GetLocalizedDescriptions() const
{
	return LocalizedDescriptions;
}

TArray<FLocalizedText> FFeaturePackContentSource::GetLocalizedAssetTypes() const
{
	return LocalizedAssetTypesList;
}


FString FFeaturePackContentSource::GetClassTypesUsed() const
{
	return ClassTypes;
}

EContentSourceCategory FFeaturePackContentSource::GetCategory() const
{
	return Category;
}

TSharedPtr<FImageData> FFeaturePackContentSource::GetIconData() const
{
	return IconData;
}

TArray<TSharedPtr<FImageData>> FFeaturePackContentSource::GetScreenshotData() const
{
	return ScreenshotData;
}

bool FFeaturePackContentSource::InstallToProject(FString InstallPath)
{
	bool bResult = false;
	if( IsDataValid() == false )
	{
		UE_LOG(LogFeaturePack, Warning, TEXT("Trying to install invalid pack %s"), *InstallPath);
	}
	else
	{
		// We need to insert additional packs before we import the main assets since the code in the main pack may reference them
		InsertAdditionalFeaturePacks();

		FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
		TArray<FString> AssetPaths;
		AssetPaths.Add(FeaturePackPath);

		TArray<UObject*> ImportedObjects = AssetToolsModule.Get().ImportAssets(AssetPaths, InstallPath );
		if( ImportedObjects.Num() == 0 )
		{
			UE_LOG(LogFeaturePack, Warning, TEXT("No objects imported installing pack %s"), *InstallPath);
		}
		else
		{
			// Save any imported assets.
			TArray<UPackage*> ToSave;
			for (auto ImportedObject : ImportedObjects)
			{
				ToSave.AddUnique(ImportedObject->GetOutermost());
			}
			FEditorFileUtils::PromptForCheckoutAndSave( ToSave, /*bCheckDirty=*/ false, /*bPromptToSave=*/ false );
			
			bResult = true;
			
			// Focus on a specific asset if we want to.
			if( GetFocusAssetName().IsEmpty() == false )
			{
				UObject* FocusAsset = LoadObject<UObject>(nullptr, *GetFocusAssetName());
				if (FocusAsset)
				{
					FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
					TArray<UObject*> SyncObjects;
					SyncObjects.Add(FocusAsset);
					ContentBrowserModule.Get().SyncBrowserToAssets(SyncObjects);
				}
			}
		}
	}
	return bResult;
}

FFeaturePackContentSource::~FFeaturePackContentSource()
{
	// Remove any search handler delegates for this pack
	FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(SearchClickedHandle);
	FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(SearchChangedHandle);
}

bool FFeaturePackContentSource::IsDataValid() const
{
	if( bPackValid == false )
	{
		return false;
	}
	// To Do maybe validate other data here
	return true;	
}

FString FFeaturePackContentSource::GetFocusAssetName() const
{
	return FocusAssetIdent;
}

FString FFeaturePackContentSource::GetSortKey() const
{
	return SortKey;
}


void FFeaturePackContentSource::HandleActOnSearchText(TSharedPtr<FSearchEntry> SearchEntry)
{
	if (SearchEntry.IsValid())
	{
		if (SearchEntry->bCategory == false)
		{
			UEnum* Enum = FindObjectChecked<UEnum>(ANY_PACKAGE, TEXT("EContentSourceCategory"));
			FString CurrentLanguage = FInternationalization::Get().GetCurrentCulture()->GetTwoLetterISOLanguageName();
			FLocalizedText CurrentName = ChooseLocalizedText(LocalizedNames,CurrentLanguage);
			FString MyTitle = FText::Format( LOCTEXT("FeaturePackSearchResult", "{0} ({1})"), CurrentName.GetText(), Enum->GetDisplayNameText((int32)Category)).ToString();
			if (SearchEntry->Title == MyTitle)
			{
				IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
				IAddContentDialogModule& AddContentDialogModule = FModuleManager::LoadModuleChecked<IAddContentDialogModule>("AddContentDialog");				
				AddContentDialogModule.ShowDialog(MainFrameModule.GetParentWindow().ToSharedRef());
			}
		}
	}
}

void FFeaturePackContentSource::TryAddFeaturePackCategory(FString CategoryTitle, TArray< TSharedPtr<FSearchEntry> >& OutSuggestions)
{
	if (OutSuggestions.ContainsByPredicate([&CategoryTitle](TSharedPtr<FSearchEntry>& InElement)
		{ return ((InElement->Title == CategoryTitle) && (InElement->bCategory == true)); }) == false)
	{
		TSharedPtr<FSearchEntry> FeaturePackCat = MakeShareable(new FSearchEntry());
		FeaturePackCat->bCategory = true;
		FeaturePackCat->Title = CategoryTitle;
		OutSuggestions.Add(FeaturePackCat);
	}
}

void FFeaturePackContentSource::HandleSuperSearchTextChanged(const FString& InText, TArray< TSharedPtr<FSearchEntry> >& OutSuggestions)
{
	FString FeaturePackSearchCat = LOCTEXT("FeaturePackSearchCategory", "Feature Packs").ToString();

	FString CurrentLanguage = FInternationalization::Get().GetCurrentCulture()->GetTwoLetterISOLanguageName();
	FLocalizedText CurrentName = ChooseLocalizedText(LocalizedNames,CurrentLanguage);
	FLocalizedTextArray CurrentTagSet = ChooseLocalizedTextArray(LocalizedSearchTags,CurrentLanguage);
	FText AsText = FText::FromString(InText);
	TArray<FText> TagArray = CurrentTagSet.GetTags();
	if (TagArray.Num() != 0)
	{
		UEnum* Enum = FindObjectChecked<UEnum>(ANY_PACKAGE, TEXT("EContentSourceCategory"));
		// Add a feature packs category		
		for (int32 iTag = 0; iTag < TagArray.Num(); iTag++)
		{
			if (TagArray[iTag].EqualToCaseIgnored(AsText))
			{
				// This will add the category if one doesnt exist
				TryAddFeaturePackCategory(FeaturePackSearchCat,OutSuggestions);
				TSharedPtr<FSearchEntry> FeaturePackEntry = MakeShareable(new FSearchEntry());
				FeaturePackEntry->Title = FText::Format( LOCTEXT("FeaturePackSearchResult", "{0} ({1})"), CurrentName.GetText(), Enum->GetDisplayNameText((int32)Category)).ToString();
				FeaturePackEntry->bCategory = false;
				OutSuggestions.Add(FeaturePackEntry);
				return;
			}
		}
	}
}

FLocalizedText FFeaturePackContentSource::ChooseLocalizedText(TArray<FLocalizedText> Choices, FString LanguageCode)
{
	FLocalizedText Default;
	for (const FLocalizedText& Choice : Choices)
	{
		if (Choice.GetTwoLetterLanguage() == LanguageCode)
		{
			return Choice;
			break;
		}
		else if (Choice.GetTwoLetterLanguage() == TEXT("en"))
		{
			Default = Choice;
		}
	}
	return Default;
}

FLocalizedTextArray FFeaturePackContentSource::ChooseLocalizedTextArray(TArray<FLocalizedTextArray> Choices, FString LanguageCode)
{
	FLocalizedTextArray Default;
	for (const FLocalizedTextArray& Choice : Choices)
	{
		if (Choice.GetTwoLetterLanguage() == LanguageCode)
		{
			return Choice;
			break;
		}
		else if (Choice.GetTwoLetterLanguage() == TEXT("en"))
		{
			Default = Choice;
		}
	}
	return Default;
}

bool FFeaturePackContentSource::GetAdditionalFilesForPack(TArray<FString>& FileList, bool& bContainsSource)
{
	bool bParsedFiles = false;
	if (bPackValid == false)
	{
		RecordAndLogError(FString::Printf(TEXT("Cannot extract files from invalid Pack %s"), *FeaturePackPath));
	}
	else
	{
		// Create a pak platform file and mount the feature pack file.
		FPakPlatformFile PakPlatformFile;
		PakPlatformFile.Initialize(&FPlatformFileManager::Get().GetPlatformFile(), TEXT(""));
		FString MountPoint = "root:/";
		PakPlatformFile.Mount(*FeaturePackPath, 0, *MountPoint);

		// Gets the manifest file as a JSon string
		TArray<uint8> ManifestBuffer;
		if (LoadPakFileToBuffer(PakPlatformFile, FPaths::Combine(*MountPoint, TEXT("Config/Config.ini")), ManifestBuffer) == false)
		{
			RecordAndLogError(FString::Printf(TEXT("Error in Feature pack %s. Cannot find Config.ini"), *FeaturePackPath));			
		}
		else
		{
			FString ConfigFileString;
			FFileHelper::BufferToString(ConfigFileString, ManifestBuffer.GetData(), ManifestBuffer.Num());
			bParsedFiles = ExtractListOfAdditionalFiles(ConfigFileString, FileList, bContainsSource);
		}
	}
	return bParsedFiles;
}

bool FFeaturePackContentSource::ExtractListOfAdditionalFiles(const FString& ConfigFileAsString,TArray<FString>& FileList, bool& bContainsSource) const
{
	FConfigFile PackConfig;
	PackConfig.ProcessInputFileContents(ConfigFileAsString);
	FConfigSection* AdditionalFilesSection = PackConfig.Find("AdditionalFilesToAdd");
	
	bContainsSource = false;
	bool bParsedAdditionFiles = false;
	if (AdditionalFilesSection)
	{
		bParsedAdditionFiles = true;
		for (auto FilePair : *AdditionalFilesSection)
		{
			if (FilePair.Key.ToString().Contains("Files"))
			{
				FString Filename = FPaths::GetCleanFilename(FilePair.Value);
				FString Directory = FPaths::RootDir() / FPaths::GetPath(FilePair.Value);
				FPaths::MakeStandardFilename(Directory);
				FPakFile::MakeDirectoryFromPath(Directory);

				if (Filename.Contains(TEXT("*")))
				{
					TArray<FString> FoundFiles;
					IFileManager::Get().FindFilesRecursive(FoundFiles, *Directory, *Filename, true, false);
					FileList.Append(FoundFiles);
					if (!bContainsSource)
					{
						for (const FString& FoundFile : FoundFiles)
						{
							if (FoundFile.StartsWith(TEXT("Source/")) || FoundFile.Contains(TEXT("/Source/")))
							{
								bContainsSource = true;
								break;
							}
						}
					}
				}
				else
				{
					FileList.Add(Directory / Filename);
					if (!bContainsSource && (FileList.Last().StartsWith(TEXT("Source/")) || FileList.Last().Contains(TEXT("/Source/"))))
					{
						bContainsSource = true;
					}
				}
			}
		}
	}
	return bParsedAdditionFiles;
}

void FFeaturePackContentSource::ImportPendingPacks()
{ 
	bool bAddPacks;
	if (GConfig->GetBool(TEXT("StartupActions"), TEXT("bAddPacks"), bAddPacks, GGameIni) == true)
	{
		if (bAddPacks == true)
		{
			ParseAndImportPacks();			
			GConfig->SetBool(TEXT("StartupActions"), TEXT("bAddPacks"), false, GGameIni);
			GConfig->Flush(true, GGameIni);
		}
	}
}

void FFeaturePackContentSource::ParseAndImportPacks()
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
	// Look for pack insertions in the startup actions section
	TArray<FString> PacksToAdd;
	GConfig->GetArray(TEXT("StartupActions"), TEXT("InsertPack"), PacksToAdd, GGameIni);
	int32 PacksInserted = 0;
	for (int32 iPackEntry = 0; iPackEntry < PacksToAdd.Num(); iPackEntry++)
	{
		FPackData EachPackData;
		TArray<FString> PackEntries;
		PacksToAdd[iPackEntry].ParseIntoArray(PackEntries, TEXT(","), true);
		FString PackSource;
		FString PackName;
		// Parse the pack name and source
		for (int32 iEntry = 0; iEntry < PackEntries.Num(); iEntry++)
		{
			FString EachString = PackEntries[iEntry];
			// remove the parenthesis
			EachString = EachString.Replace(TEXT("("), TEXT(""));
			EachString = EachString.Replace(TEXT(")"), TEXT(""));
			if (EachString.StartsWith(TEXT("PackSource=")) == true)
			{
				EachString = EachString.Replace(TEXT("PackSource="), TEXT(""));
				EachString = EachString.TrimQuotes();
				EachPackData.PackSource = EachString;
			}
			if (EachString.StartsWith(TEXT("PackName=")) == true)
			{
				EachString = EachString.Replace(TEXT("PackName="), TEXT(""));
				EachString = EachString.TrimQuotes();
				EachPackData.PackName = EachString;
			}
		}

		// If we found anything to insert, insert it !
		if ((EachPackData.PackSource.IsEmpty() == false) && (EachPackData.PackName.IsEmpty() == false))
		{
			TArray<FString> EachImport;
			FString FullPath = FPaths::FeaturePackDir() + EachPackData.PackSource;
			EachImport.Add(FullPath);
			EachPackData.ImportedObjects = AssetToolsModule.Get().ImportAssets(EachImport, TEXT("/Game"), nullptr, false);

			if (EachPackData.ImportedObjects.Num() == 0)
			{
				UE_LOG(LogFeaturePack, Warning, TEXT("No objects imported installing pack %s"), *EachPackData.PackSource);
			}
			else
			{
				// Save any imported assets.
				TArray<UPackage*> ToSave;
				for (auto ImportedObject : EachPackData.ImportedObjects)
				{
					ToSave.AddUnique(ImportedObject->GetOutermost());
				}
				FEditorFileUtils::PromptForCheckoutAndSave(ToSave, /*bCheckDirty=*/ false, /*bPromptToSave=*/ false);
				PacksInserted++;
			}
		}
	}
	UE_LOG(LogFeaturePack, Warning, TEXT("Inserted %d feature packs"), PacksInserted++);
}

void FFeaturePackContentSource::RecordAndLogError(const FString& ErrorString)
{
	UE_LOG(LogFeaturePack, Warning, TEXT("%s"), *ErrorString);
	ParseErrors.Add(ErrorString);
}

void FFeaturePackContentSource::CopyAdditionalFilesToFolder( const FString& DestinationFolder, TArray<FString>& FilesCopied, bool &bHasSourceFiles )
{
	FString ContentIdent = TEXT("Content/");
	TArray<FString> FilesToAdd;
	GetAdditionalFilesForPack(FilesToAdd, bHasSourceFiles);
	for (int32 iFile = 0; iFile < FilesToAdd.Num(); ++iFile)
	{
		FString EachFile = FilesToAdd[iFile];
		int32 ContentIndex;
		ContentIndex = EachFile.Find(*ContentIdent);
		if (ContentIndex != INDEX_NONE)
		{
			FString ContentFile = EachFile.RightChop(ContentIndex);
			FString GameFolder = EachFile.Left(ContentIndex - 1);
			int32 NameStart = GameFolder.Find(TEXT("/"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
			GameFolder = GameFolder.RightChop(NameStart);
			FPaths::NormalizeFilename(ContentFile);
			ContentFile.InsertAt(ContentIdent.Len() - 1, GameFolder);
			FString FinalDestination = DestinationFolder / ContentFile;
			if (IFileManager::Get().Copy(*FinalDestination, *EachFile) == COPY_OK)
			{
				FilesCopied.Add(FinalDestination);
			}
			else
			{
				UE_LOG(LogFeaturePack, Warning, TEXT("Failed to copy %s to %s"), *EachFile, *FinalDestination);
			}
		}
	}
}

void FFeaturePackContentSource::InsertAdditionalFeaturePacks()
{
	// Now copy files in any additional packs (These are for the shared assets)
	EFeaturePackDetailLevel RequiredLevel = EFeaturePackDetailLevel::High;
	for (int32 iExtraPack = 0; iExtraPack < AdditionalFeaturePacks.Num(); iExtraPack++)
	{
		FString DestinationFolder = FPaths::GameDir();
		FString FullPath = FPaths::FeaturePackDir() + AdditionalFeaturePacks[iExtraPack].GetFeaturePackNameForLevel(RequiredLevel);
		if (FPlatformFileManager::Get().GetPlatformFile().FileExists(*FullPath))
		{
			FString InsertPath = DestinationFolder + *AdditionalFeaturePacks[iExtraPack].MountName;

			TUniquePtr<FFeaturePackContentSource> NewContentSource = MakeUnique<FFeaturePackContentSource>(FullPath, true);
			if (NewContentSource->IsDataValid() == true)
			{
				bool bHasSourceFiles = false;
				TArray<FString> FilesCopied;
				NewContentSource->CopyAdditionalFilesToFolder(DestinationFolder, FilesCopied, bHasSourceFiles);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE 
