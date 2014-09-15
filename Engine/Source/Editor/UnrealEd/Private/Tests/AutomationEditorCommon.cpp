// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "AutomationEditorCommon.h"
#include "AssetEditorManager.h"
#include "ModuleManager.h"
#include "LevelEditor.h"
#include "Editor/MainFrame/Public/MainFrame.h"
#include "EngineVersion.h"


DEFINE_LOG_CATEGORY_STATIC(LogAutomationEditorCommon, Log, All);

namespace AutomationEditorCommonUtils
{
	/**
	* Converts a package path to an asset path
	*
	* @param PackagePath - The package path to convert
	*/
	FString ConvertPackagePathToAssetPath(const FString& PackagePath)
	{
		const FString Filename = FPaths::ConvertRelativePathToFull(PackagePath);
		FString EngineFileName = Filename;
		FString GameFileName = Filename;
		if (FPaths::MakePathRelativeTo(EngineFileName, *FPaths::EngineContentDir()) && !FPaths::IsRelative(EngineFileName))
		{
			const FString ShortName = FPaths::GetBaseFilename(EngineFileName);
			const FString PathName = FPaths::GetPath(EngineFileName);
			const FString AssetName = FString::Printf(TEXT("/Engine/%s/%s.%s"), *PathName, *ShortName, *ShortName);
			return AssetName;
		}
		else if (FPaths::MakePathRelativeTo(GameFileName, *FPaths::GameContentDir()) && !FPaths::IsRelative(GameFileName))
		{
			const FString ShortName = FPaths::GetBaseFilename(GameFileName);
			const FString PathName = FPaths::GetPath(GameFileName);
			const FString AssetName = FString::Printf(TEXT("/Game/%s/%s.%s"), *PathName, *ShortName, *ShortName);
			return AssetName;
		}
		else
		{
			UE_LOG(LogAutomationEditorCommon, Error, TEXT("PackagePath (%s) is invalid for the current project"), *PackagePath);
			return TEXT("");
		}
	}

	/**
	* Imports an object using a given factory
	*
	* @param ImportFactory - The factory to use to import the object
	* @param ObjectName - The name of the object to create
	* @param PackagePath - The full path of the package file to create
	* @param ImportPath - The path to the object to import
	*/
	UObject* ImportAssetUsingFactory(UFactory* ImportFactory, const FString& ObjectName, const FString& PackagePath, const FString& ImportPath)
	{
		UObject* ImportedAsset = NULL;

		UPackage* Pkg = CreatePackage(NULL, *PackagePath);
		if (Pkg)
		{
			// Make sure the destination package is loaded
			Pkg->FullyLoad();

			UClass* ImportAssetType = ImportFactory->ResolveSupportedClass();
			bool bDummy = false;

			//If we are a texture factory suppress some warning dialog that we don't want
			if (ImportFactory->IsA(UTextureFactory::StaticClass()))
			{
				UTextureFactory::SuppressImportResolutionWarningDialog();
				UTextureFactory::SuppressImportOverwriteDialog();
			}

			ImportedAsset = UFactory::StaticImportObject(ImportAssetType, Pkg, FName(*ObjectName), RF_Public | RF_Standalone, bDummy, *ImportPath, NULL, ImportFactory, NULL, GWarn, 0);

			if (ImportedAsset)
			{
				UE_LOG(LogAutomationEditorCommon, Display, TEXT("Imported %s"), *ImportPath);
			}
			else
			{
				UE_LOG(LogAutomationEditorCommon, Error, TEXT("Failed to import asset using factory %s!"), *ImportFactory->GetName());
			}
		}
		else
		{
			UE_LOG(LogAutomationEditorCommon, Error, TEXT("Failed to create a package!"));
		}

		return ImportedAsset;
	}

	/**
	* Nulls out references to a given object
	*
	* @param InObject - Object to null references to
	*/
	void NullReferencesToObject(UObject* InObject)
	{
		TArray<UObject*> ReplaceableObjects;
		TMap<UObject*, UObject*> ReplacementMap;
		ReplacementMap.Add(InObject, NULL);
		ReplacementMap.GenerateKeyArray(ReplaceableObjects);

		// Find all the properties (and their corresponding objects) that refer to any of the objects to be replaced
		TMap< UObject*, TArray<UProperty*> > ReferencingPropertiesMap;
		for (FObjectIterator ObjIter; ObjIter; ++ObjIter)
		{
			UObject* CurObject = *ObjIter;

			// Find the referencers of the objects to be replaced
			FFindReferencersArchive FindRefsArchive(CurObject, ReplaceableObjects);

			// Inform the object referencing any of the objects to be replaced about the properties that are being forcefully
			// changed, and store both the object doing the referencing as well as the properties that were changed in a map (so that
			// we can correctly call PostEditChange later)
			TMap<UObject*, int32> CurNumReferencesMap;
			TMultiMap<UObject*, UProperty*> CurReferencingPropertiesMMap;
			if (FindRefsArchive.GetReferenceCounts(CurNumReferencesMap, CurReferencingPropertiesMMap) > 0)
			{
				TArray<UProperty*> CurReferencedProperties;
				CurReferencingPropertiesMMap.GenerateValueArray(CurReferencedProperties);
				ReferencingPropertiesMap.Add(CurObject, CurReferencedProperties);
				for (TArray<UProperty*>::TConstIterator RefPropIter(CurReferencedProperties); RefPropIter; ++RefPropIter)
				{
					CurObject->PreEditChange(*RefPropIter);
				}
			}

		}

		// Iterate over the map of referencing objects/changed properties, forcefully replacing the references and then
		// alerting the referencing objects the change has completed via PostEditChange
		int32 NumObjsReplaced = 0;
		for (TMap< UObject*, TArray<UProperty*> >::TConstIterator MapIter(ReferencingPropertiesMap); MapIter; ++MapIter)
		{
			++NumObjsReplaced;

			UObject* CurReplaceObj = MapIter.Key();
			const TArray<UProperty*>& RefPropArray = MapIter.Value();

			FArchiveReplaceObjectRef<UObject> ReplaceAr(CurReplaceObj, ReplacementMap, false, true, false);

			for (TArray<UProperty*>::TConstIterator RefPropIter(RefPropArray); RefPropIter; ++RefPropIter)
			{
				FPropertyChangedEvent PropertyEvent(*RefPropIter);
				CurReplaceObj->PostEditChangeProperty(PropertyEvent);
			}

			if (!CurReplaceObj->HasAnyFlags(RF_Transient) && CurReplaceObj->GetOutermost() != GetTransientPackage())
			{
				if (!CurReplaceObj->RootPackageHasAnyFlags(PKG_CompiledIn))
				{
					CurReplaceObj->MarkPackageDirty();
				}
			}
		}
	}

	/**
	* gets a factory class based off an asset file extension
	*
	* @param AssetExtension - The file extension to use to find a supporting UFactory
	*/
	UClass* GetFactoryClassForType(const FString& AssetExtension)
	{
		// First instantiate one factory for each file extension encountered that supports the extension
		for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
		{
			if ((*ClassIt)->IsChildOf(UFactory::StaticClass()) && !((*ClassIt)->HasAnyClassFlags(CLASS_Abstract)))
			{
				UFactory* Factory = Cast<UFactory>((*ClassIt)->GetDefaultObject());
				if (Factory->bEditorImport && Factory->ValidForCurrentGame())
				{
					TArray<FString> FactoryExtensions;
					Factory->GetSupportedFileExtensions(FactoryExtensions);

					// Case insensitive string compare with supported formats of this factory
					if (FactoryExtensions.Contains(AssetExtension))
					{
						return *ClassIt;
					}
				}
			}
		}

		return NULL;
	}

	/**
	* Applies settings to an object by finding UProperties by name and calling ImportText
	*
	* @param InObject - The object to search for matching properties
	* @param PropertyChain - The list UProperty names recursively to search through
	* @param Value - The value to import on the found property
	*/
	void ApplyCustomFactorySetting(UObject* InObject, TArray<FString>& PropertyChain, const FString& Value)
	{
		const FString PropertyName = PropertyChain[0];
		PropertyChain.RemoveAt(0);

		UProperty* TargetProperty = FindField<UProperty>(InObject->GetClass(), *PropertyName);
		if (TargetProperty)
		{
			if (PropertyChain.Num() == 0)
			{
				TargetProperty->ImportText(*Value, TargetProperty->ContainerPtrToValuePtr<uint8>(InObject), 0, InObject);
			}
			else
			{
				UStructProperty* StructProperty = Cast<UStructProperty>(TargetProperty);
				UObjectProperty* ObjectProperty = Cast<UObjectProperty>(TargetProperty);

				UObject* SubObject = NULL;
				bool bValidPropertyType = true;

				if (StructProperty)
				{
					SubObject = StructProperty->Struct;
				}
				else if (ObjectProperty)
				{
					SubObject = ObjectProperty->GetObjectPropertyValue(ObjectProperty->ContainerPtrToValuePtr<UObject>(InObject));
				}
				else
				{
					//Unknown nested object type
					bValidPropertyType = false;
					UE_LOG(LogAutomationEditorCommon, Error, TEXT("ERROR: Unknown nested object type for property: %s"), *PropertyName);
				}

				if (SubObject)
				{
					ApplyCustomFactorySetting(SubObject, PropertyChain, Value);
				}
				else if (bValidPropertyType)
				{
					UE_LOG(LogAutomationEditorCommon, Error, TEXT("Error accessing null property: %s"), *PropertyName);
				}
			}
		}
		else
		{
			UE_LOG(LogAutomationEditorCommon, Error, TEXT("ERROR: Could not find factory property: %s"), *PropertyName);
		}
	}

	/**
	* Applies the custom factory settings
	*
	* @param InFactory - The factory to apply custom settings to
	* @param FactorySettings - An array of custom settings to apply to the factory
	*/
	void ApplyCustomFactorySettings(UFactory* InFactory, const TArray<FImportFactorySettingValues>& FactorySettings)
	{
		bool bCallConfigureProperties = true;

		for (int32 i = 0; i < FactorySettings.Num(); ++i)
		{
			if (FactorySettings[i].SettingName.Len() > 0 && FactorySettings[i].Value.Len() > 0)
			{
				//Check if we are setting an FBX import type override.  If we are, we don't want to call ConfigureProperties because that enables bDetectImportTypeOnImport
				if (FactorySettings[i].SettingName.Contains(TEXT("MeshTypeToImport")))
				{
					bCallConfigureProperties = false;
				}

				TArray<FString> PropertyChain;
				FactorySettings[i].SettingName.ParseIntoArray(&PropertyChain, TEXT("."), false);
				ApplyCustomFactorySetting(InFactory, PropertyChain, FactorySettings[i].Value);
			}
		}

		if (bCallConfigureProperties)
		{
			InFactory->ConfigureProperties();
		}
	}
}

/**
* Writes a number to a text file.
* @param FolderNameForTest is the folder that has the same name as the test. (For Example: "Performance").
* @param FolderNameForBeingTested is the name for the thing that is being tested. (For Example: "MapName").
* @param FileName is the name of the file with an extension (no need to add the .txt)
* @param NumberToWriteToFile is the float number that is expected to be written to the file.
* @param Delimiter is the delimiter to be used. TEXT(",")
*/
void WriteToTextFile(const FString& FolderNameForTest ,const FString& FolderNameForBeingTested, const FString& FileName, const float& NumberToWriteToFile, const FString& Delimiter)
{
	//Performance file locations and setups.
	FString FileSaveLocation = FPaths::AutomationLogDir() + FolderNameForTest + TEXT("/") + FolderNameForBeingTested + TEXT("/") + FileName + TEXT(".txt");

	//Variables that hold the content from the text files
	FString ReadFromTextFile;

	//Log out to a text file the Duration.
	FString CurrentNumberToWrite = FString::Printf(TEXT("%f"), NumberToWriteToFile);
	FFileHelper::LoadFileToString(ReadFromTextFile, *FileSaveLocation);
	FString FileSetup = ReadFromTextFile + CurrentNumberToWrite + Delimiter;
	FFileHelper::SaveStringToFile(FileSetup, *FileSaveLocation);

}

/**
* Returns the sum of the numbers available in a FString array.
* @param NumberArray is the name of the array intended to be used.
* @param bAverageOnly will return the average of the available numbers instead of the sum.
*/
float ValueFromStringArrayofNumbers(const TArray<FString>& NumberArray, bool bAverageInstead)
{
	//Total Value holds the sum of all the numbers available in the array.
	float TotalValue = 0;
	int32 NumberCounter = 0;

	//Loop through the array.
	for (int32 Index = 0; Index < NumberArray.Num(); ++Index)
	{
		//If the character in the array is a number then we'll want to add it to our total.
		if (NumberArray[Index].IsNumeric())
		{
			NumberCounter++;
			TotalValue += FCString::Atof(NumberArray[Index].GetCharArray().GetData());
		}
	}	

	//If bAverageInstead equals true then only the average is returned.
	//Otherwise return the total value.
	if (bAverageInstead)
	{
		UE_LOG(LogEditorAutomationTests, Log, TEXT("Average value of the Array is %f"), (TotalValue / NumberCounter));
		return (TotalValue / NumberCounter);
	}
	
	UE_LOG(LogEditorAutomationTests, Log, TEXT("Total Value of the Array is %f"), TotalValue);
	return TotalValue;

}

TArray<FString> CreateArrayFromFile(const FString& FullFileLocation)
{

	FString RawData;
	TArray<FString> DataArray;

	UE_LOG(LogEditorAutomationTests, Log, TEXT("Loading and parsing the data from '%s' into an array."), *FullFileLocation);
	FFileHelper::LoadFileToString(RawData, *FullFileLocation);
	RawData.ParseIntoArray(&DataArray, TEXT(","), false);
	
	return DataArray;
}

///////////////////////////////////////////////////////////////////////
// Common Latent commands

//Latent Undo and Redo command
//If bUndo is true then the undo action will occur otherwise a redo will happen.
bool FUndoRedoCommand::Update()
{
	if (bUndo == true)
	{
		//Undo
		GEditor->UndoTransaction();
	}
	else
	{
		//Redo
		GEditor->RedoTransaction();
	}

	return true;
}

/**
* Open editor for a particular asset
*/
bool FOpenEditorForAssetCommand::Update()
{
	UObject* Object = StaticLoadObject(UObject::StaticClass(), NULL, *AssetName);
	if (Object)
	{
		FAssetEditorManager::Get().OpenEditorForAsset(Object);
		//This checks to see if the asset sub editor is loaded.
		if (FAssetEditorManager::Get().FindEditorForAsset(Object, true) != NULL)
		{
			UE_LOG(LogEditorAutomationTests, Log, TEXT("Verified asset editor for: %s."), *AssetName);
			UE_LOG(LogEditorAutomationTests, Display, TEXT("The editor successffully loaded for: %s."), *AssetName);
			return true;
		}
	}
	else
	{
		UE_LOG(LogEditorAutomationTests, Error, TEXT("Failed to find object: %s."), *AssetName);
	}
	return true;
}

/**
* Close all sub-editors
*/
bool FCloseAllAssetEditorsCommand::Update()
{
	FAssetEditorManager::Get().CloseAllAssetEditors();

	//Get all assets currently being tracked with open editors and make sure they are not still opened.
	if(FAssetEditorManager::Get().GetAllEditedAssets().Num() >= 1)
	{
		UE_LOG(LogEditorAutomationTests, Warning, TEXT("Not all of the editors were closed."));
		return true;
	}

	UE_LOG(LogEditorAutomationTests, Log, TEXT("Verified asset editors were closed"));
	UE_LOG(LogEditorAutomationTests, Display, TEXT("The asset editors closed successffully"));
	return true;
}

/**
* Start PIE session
*/
bool FStartPIECommand::Update()
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::Get().GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
	TSharedPtr<class ILevelViewport> ActiveLevelViewport = LevelEditorModule.GetFirstActiveViewport();

	GUnrealEd->RequestPlaySession(false, ActiveLevelViewport, bSimulateInEditor, NULL, NULL, -1, false);
	return true;
}

/**
* End PlayMap session
*/
bool FEndPlayMapCommand::Update()
{
	GUnrealEd->RequestEndPlayMap();
	return true;
}

/**
* This command grabs the FPS and Memory for the current editor session.
*/
bool FEditorPerformanceCommand::Update()
{
	//Gets the main frame module to get the name of our current level.
	const IMainFrameModule& MainFrameModule = FModuleManager::GetModuleChecked< IMainFrameModule >("MainFrame");
	FString ShortMapName = MainFrameModule.GetLoadedLevelName();
	
	//Variables that hold the content from the text files
	FString SavedAverageFPS;
	FString SavedMemory;
	FString SavedDuration;
	static SIZE_T StaticLastTotalAllocated = 0;

	//Variables needed for the WriteToTextFile function.
	FString TestName = TEXT("Performance");
	FString Delimiter = TEXT(",");
	FString FileName;
	
	//Grab the FPS and working memory number.
	float CurrentTime = FPlatformTime::Seconds();
	if ((CurrentTime - StartTime) <= Duration)
	{
		//Find the Average FPS
		//Clamp to avoid huge averages at startup or after hitches
		const float CurrentFPS = 1.0f / FSlateApplication::Get().GetAverageDeltaTime();
		const float ClampedFPS = (CurrentFPS < 0.0f || CurrentFPS > 4000.0f) ? 0.0f : CurrentFPS;
		
		//Find the Frame Time in ms.
		//Clamp to avoid huge averages at startup or after hitches
		const float AverageMS = FSlateApplication::Get().GetAverageDeltaTime() * 1000.0f;
		const float ClampedMS = (AverageMS < 0.0f || AverageMS > 4000.0f) ? 0.0f : AverageMS;

		//Query OS for process memory used.
		FPlatformMemoryStats MemoryStats = FPlatformMemory::GetStats();
		StaticLastTotalAllocated = MemoryStats.UsedPhysical;

		//Log out to a text file the Duration.
		WriteToTextFile(TestName, ShortMapName, FileName = TEXT("RAWDuration"), CurrentTime, Delimiter);
		//Log out to a text file the AverageFPS
		WriteToTextFile(TestName, ShortMapName, FileName = TEXT("RAWAverageFPS"), ClampedFPS, Delimiter);
		//Log out to a text file the AverageFrameTime
		WriteToTextFile(TestName, ShortMapName, FileName = TEXT("RAWFrameTime"), ClampedMS, Delimiter);
		//Log out to a text file the Memory in kilobytes.
		WriteToTextFile(TestName, ShortMapName, FileName = TEXT("RAWMemoryUsedPhysical"), (float)StaticLastTotalAllocated / 1024, Delimiter);

		return false;
	}
	
	UE_LOG(LogEditorAutomationTests, Log, TEXT("Raw performance data has been captured and saved to the '//Game/Saved/Automation/Log/Performance/%s' folder location."), *ShortMapName);
	
	return true;
}

bool FGenerateEditorPerformanceCharts::Update()
{
	UE_LOG(LogEditorAutomationTests, Log, TEXT("Begin generating the editor performance charts."));
	//Get the current changelist number
	FString Changelist = GEngineVersion.ToString(EVersionComponent::Changelist);

	//Gets the main frame module to get the name of our current level.
	const IMainFrameModule& MainFrameModule = FModuleManager::GetModuleChecked< IMainFrameModule >("MainFrame");
	FString LoadedMapName = MainFrameModule.GetLoadedLevelName();

	//The locations of the raw data.
	FString FileSaveLocation = FPaths::AutomationLogDir() + TEXT("Performance/") + LoadedMapName + TEXT("/");
	FString FPSFileSaveLocation = FileSaveLocation + TEXT("RAWAverageFPS.txt");
	FString FrameTimeSaveLocation = FileSaveLocation + TEXT("RAWFrameTime.txt");
	FString MemoryFileSaveLocation = FileSaveLocation + TEXT("RAWMemoryUsedPhysical.txt");
	FString MapLoadTimeFileSaveLocation = FileSaveLocation + TEXT("RAWMapLoadTime.txt");
	FString DurationFileSaveLocation = FileSaveLocation + TEXT("RAWDuration.txt");
	//Get the data from the raw files and put them into an array.
	TArray<FString> FPSRawArray = CreateArrayFromFile(FPSFileSaveLocation);
	TArray<FString> FrameTimeRawArray = CreateArrayFromFile(FrameTimeSaveLocation);
	TArray<FString> MemoryRawArray = CreateArrayFromFile(MemoryFileSaveLocation);
	TArray<FString> MapLoadTimeRawArray = CreateArrayFromFile(MapLoadTimeFileSaveLocation);
	TArray<FString> TestRunTimeRawArray = CreateArrayFromFile(DurationFileSaveLocation);

	//Variable that will hold the old performance csv file data.
	FString OldPerformanceCSVFile;

	//The CSV files.
	//RAW csv holds all of the information from the text files
	//Final csv only holds the averaged information.
	FString FinalCSVFilename = FString::Printf(TEXT("%s%s_Performance.csv"), *FileSaveLocation, *LoadedMapName);
	FString RAWCSVFilename = FString::Printf(TEXT("%sRAW_%s_%s.csv"), *FileSaveLocation, *LoadedMapName, *FDateTime::Now().ToString());

	//Create the .csv file that will hold the raw data from the text files.
	FArchive* RAWCSVFile = IFileManager::Get().CreateFileWriter(*RAWCSVFilename);
	if (!RAWCSVFile)
	{
		UE_LOG(LogEditorAutomationTests, Error, TEXT("Failed to create the csv file: %s"), *RAWCSVFilename);
		return true;
	}

	//Add the top title row to the raw csv file
	FString RAWCSVLine = (TEXT("Map Name, Changelist, Test Run Time, Map Load Time, Average FPS, Frame Time, Used Physical Memory\n"));
	RAWCSVFile->Serialize(TCHAR_TO_ANSI(*RAWCSVLine), RAWCSVLine.Len());
	
	//Loop through the text files and add them to the raw csv file.
	for (int32 IterLoop = 0; IterLoop < FPSRawArray.Num(); IterLoop++)
	{
		//If the raw file isn't available to write to then we'll fail back this test.
		if (!RAWCSVFile)
		{
			UE_LOG(LogEditorAutomationTests, Error, TEXT("Failed to write to the csv file: '%s'.  \nIt may have been opened or deleted."), *RAWCSVFilename);
			return true;
		}

		//Create the line that will be written and then write it to the csv file.
		RAWCSVLine = LoadedMapName + TEXT(",") + Changelist + TEXT(",") + TestRunTimeRawArray[IterLoop] + TEXT(",") + MapLoadTimeRawArray[0] + TEXT(",") + FPSRawArray[IterLoop] + TEXT(",") + FrameTimeRawArray[IterLoop] + TEXT(",") + MemoryRawArray[IterLoop] + LINE_TERMINATOR;
		RAWCSVFile->Serialize(TCHAR_TO_ANSI(*RAWCSVLine), RAWCSVLine.Len());
	}
	
	//Close the raw csv file.
	RAWCSVFile->Close();
	
	//Get the final data for the Performance csv file.
	//This gets the averaged numbers using the raw data arrays.
	float FPSAverage = ValueFromStringArrayofNumbers(FPSRawArray, true);
	float FrameTimeAverage = ValueFromStringArrayofNumbers(FrameTimeRawArray, true);
	float MemoryAverage = ValueFromStringArrayofNumbers(MemoryRawArray, true);
	
	//This gets the actual test run time by subtracting the last test run entry against the first test run entry.
	FString TestRunStartTime = TestRunTimeRawArray[0];
	FString TestRunEndTime;
	for (int32 i = (TestRunTimeRawArray.Num() - 1); i >= 0 ; --i)
	{
		if (!TestRunTimeRawArray[i].IsEmpty())
		{
			TestRunEndTime = TestRunTimeRawArray[i];
			break;
		}
	}
	float TestRunTime = FCString::Atof(TestRunEndTime.GetCharArray().GetData()) - FCString::Atof(TestRunStartTime.GetCharArray().GetData());
	//Get the map load time.
	FString MapLoadTime = MapLoadTimeRawArray[0];
	
	//If the Performance csv file does not have a size then it must be new.
	//This adds the title row to the performance csv file.
	if (IFileManager::Get().FileSize(FinalCSVFilename.GetCharArray().GetData()) <= 0)
	{
		FArchive* FinalCSVFile = IFileManager::Get().CreateFileWriter(*FinalCSVFilename);
		//If the csv file can't be created or written to then we'll fail back this test.
		if (!FinalCSVFile)
		{
			UE_LOG(LogEditorAutomationTests, Error, TEXT("Failed to create the performance csv file: '%s'."), *FPaths::ConvertRelativePathToFull(FinalCSVFilename));
			return true;
		}
		
		//Create the .csv file that will hold the raw data from the text files.
		FString FinalCSVLine = (TEXT("Date, Map Name, Changelist, Test Run Time, Map Load Time, Average FPS, Average MS, Used Physical KB\n"));
		FinalCSVFile->Serialize(TCHAR_TO_ANSI(*FinalCSVLine), FinalCSVLine.Len());
		FinalCSVFile->Close();
	}

	//Load the currently existing performance csv data.
	FFileHelper::LoadFileToString(OldPerformanceCSVFile, *FinalCSVFilename);
	FArchive* FinalCSVFile = IFileManager::Get().CreateFileWriter(*FinalCSVFilename);
	//If the performance file isn't available to write to then we'll fail back this test.
	if (!FinalCSVFile)
	{
		UE_LOG(LogEditorAutomationTests, Error, TEXT("Failed to write to the Performance csv file: '%s'.  \nIt may have been opened or deleted while in use. \nData Text files will not be deleted at this time."), *FPaths::ConvertRelativePathToFull(FinalCSVFilename));
		return true;
	}
	
	//Write the old performance csv file data to the new csv file.
	FinalCSVFile->Serialize(TCHAR_TO_ANSI(*OldPerformanceCSVFile), OldPerformanceCSVFile.Len());

	//Write the averaged numbers to the Performance CSV file.
	FString FinalCSVLine = FString::Printf(TEXT("%s,%s,%s,%.0f,%s,%.3f,%.3f,%.0f%s"), *FDateTime::Today().ToString(), *LoadedMapName, *Changelist, TestRunTime, *MapLoadTime, FPSAverage, FrameTimeAverage, MemoryAverage, LINE_TERMINATOR);
	FinalCSVFile->Serialize(TCHAR_TO_ANSI(*FinalCSVLine), FinalCSVLine.Len());

	//Close the csv files so we can use them while the editor is still open.
	FinalCSVFile->Close();

	//Display the averaged numbers.
	UE_LOG(LogEditorAutomationTests, Display, TEXT("AVG FPS: '%.1f'"), FPSAverage);
	UE_LOG(LogEditorAutomationTests, Display, TEXT("AVG Frame Time: '%.1f' ms"), FrameTimeAverage);
	UE_LOG(LogEditorAutomationTests, Display, TEXT("AVG Used Physical Memory: '%.0f' kb"), MemoryAverage);
	UE_LOG(LogEditorAutomationTests, Display, TEXT("Performance csv file is located here: %s"), *FPaths::ConvertRelativePathToFull(FinalCSVFilename));
	UE_LOG(LogEditorAutomationTests, Log, TEXT("Performance csv file is located here: %s"), *FPaths::ConvertRelativePathToFull(FinalCSVFilename));

	//Delete the text files.
	UE_LOG(LogEditorAutomationTests, Log, TEXT("Deleting temporary data text files."));
	IFileManager::Get().Delete(*FPSFileSaveLocation);
	IFileManager::Get().Delete(*FrameTimeSaveLocation);
	IFileManager::Get().Delete(*MemoryFileSaveLocation);
	IFileManager::Get().Delete(*MapLoadTimeFileSaveLocation);
	IFileManager::Get().Delete(*DurationFileSaveLocation);

	return true;
}

/**
* This this command loads a map into the editor.
*/
bool FEditorLoadMap::Update()
{
	//Get the base filename for the map that will be used.
	FString ShortMapName = FPaths::GetBaseFilename(MapName);
	
	double MapLoadStartTime = 0.0f;
	double MapLoadTime = 0.0f;

	//Variables needed for the WriteToTextFile function.
	FString TestName = TEXT("Performance");
	FString Delimiter = TEXT(",");
	FString FileName;

	//Get the current number of seconds.  This will be used to track how long it took to load the map.
	MapLoadStartTime = FPlatformTime::Seconds();

	//Load the map
	FEditorAutomationTestUtilities::LoadMap(MapName);

	//This is the time it took to load the map in the editor.
	MapLoadTime = FPlatformTime::Seconds() - MapLoadStartTime;

	//Gets the main frame module to get the name of our current level.
	const IMainFrameModule& MainFrameModule = FModuleManager::GetModuleChecked< IMainFrameModule >("MainFrame");
	FString LoadedMapName = MainFrameModule.GetLoadedLevelName();

	UE_LOG(LogEditorAutomationTests, Log, TEXT("%s has been loaded."), *ShortMapName);

	//Log out to a text file the time it takes to load the map.
	WriteToTextFile(TestName, LoadedMapName, FileName = TEXT("RAWMapLoadTime"), MapLoadTime, Delimiter);
	
	UE_LOG(LogEditorAutomationTests, Display, TEXT("%s took %.3f to load."), *LoadedMapName, MapLoadTime);
	
	return true;
}

//////////////////////////////////////////////////////////////////////
//Find Asset Commands

/**
* Generates a list of assets from the ENGINE and the GAME by a specific type.
* This is to be used by the GetTest() function.
*/
void FEditorAutomationTestUtilities::CollectTestsByClass(UClass * Class, TArray<FString>& OutBeautifiedNames, TArray <FString>& OutTestCommands)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> ObjectList;
	AssetRegistryModule.Get().GetAssetsByClass(Class->GetFName(), ObjectList);

	for (TObjectIterator<UClass> AllClassesIt; AllClassesIt; ++AllClassesIt)
	{
		UClass* ClassList = *AllClassesIt;
		FName ClassName = ClassList->GetFName();
	}

	for (auto ObjIter = ObjectList.CreateConstIterator(); ObjIter; ++ObjIter)
	{
		const FAssetData & Asset = *ObjIter;
		FString Filename = Asset.ObjectPath.ToString();
		//convert to full paths
		Filename = FPackageName::LongPackageNameToFilename(Filename);
		if (FAutomationTestFramework::GetInstance().ShouldTestContent(Filename))
		{
			FString BeautifiedFilename = Asset.AssetName.ToString();
			OutBeautifiedNames.Add(BeautifiedFilename);
			OutTestCommands.Add(Asset.ObjectPath.ToString());
		}
	}
}

/**
* Generates a list of assets from the GAME by a specific type.
* This is to be used by the GetTest() function.
*/
void FEditorAutomationTestUtilities::CollectGameContentTestsByClass(UClass * Class, bool bRecursiveClass, TArray<FString>& OutBeautifiedNames, TArray <FString>& OutTestCommands)
{
	//Setting the Asset Registry

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	//Variable setups
	TArray<FAssetData> ObjectList;
	FARFilter AssetFilter;

	//Generating the list of assets.
	//This list is being filtered by the game folder and class type.  The results are placed into the ObjectList variable.
	AssetFilter.ClassNames.Add(Class->GetFName());
	AssetFilter.PackagePaths.Add("/Game");
	AssetFilter.bRecursiveClasses = bRecursiveClass;
	AssetFilter.bRecursivePaths = true;
	AssetRegistryModule.Get().GetAssets(AssetFilter, ObjectList);

	//Loop through the list of assets, make their path full and a string, then add them to the test.
	for (auto ObjIter = ObjectList.CreateConstIterator(); ObjIter; ++ObjIter)
	{
		const FAssetData & Asset = *ObjIter;
		FString Filename = Asset.ObjectPath.ToString();
		//convert to full paths
		Filename = FPackageName::LongPackageNameToFilename(Filename);
		if (FAutomationTestFramework::GetInstance().ShouldTestContent(Filename))
		{
			FString BeautifiedFilename = Asset.AssetName.ToString();
			OutBeautifiedNames.Add(BeautifiedFilename);
			OutTestCommands.Add(Asset.ObjectPath.ToString());
		}
	}
}

/**
* Generates a list of misc. assets from the GAME.
* This is to be used by the GetTest() function.
*/
void FEditorAutomationTestUtilities::CollectMiscGameContentTestsByClass(TArray<FString>& OutBeautifiedNames, TArray <FString>& OutTestCommands)
{
	//Setting the Asset Registry
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> ObjectList;
	FARFilter AssetFilter;

	//This is a list of classes that we don't want to be in the misc category.
	TArray<FName> ExcludeClassesList;
	ExcludeClassesList.Add(UTexture::StaticClass()->GetFName());
	ExcludeClassesList.Add(UTexture2D::StaticClass()->GetFName());
	ExcludeClassesList.Add(UTextureRenderTarget::StaticClass()->GetFName());
	ExcludeClassesList.Add(UTextureRenderTarget2D::StaticClass()->GetFName());
	ExcludeClassesList.Add(UTextureCube::StaticClass()->GetFName());
	ExcludeClassesList.Add(UAimOffsetBlendSpace::StaticClass()->GetFName());
	ExcludeClassesList.Add(UAimOffsetBlendSpace1D::StaticClass()->GetFName());
	ExcludeClassesList.Add(UAnimMontage::StaticClass()->GetFName());
	ExcludeClassesList.Add(UAnimSequence::StaticClass()->GetFName());
	ExcludeClassesList.Add(UBlendSpace::StaticClass()->GetFName());
	ExcludeClassesList.Add(UBlendSpace1D::StaticClass()->GetFName());
	ExcludeClassesList.Add(UDestructibleMesh::StaticClass()->GetFName());
	ExcludeClassesList.Add(UDialogueVoice::StaticClass()->GetFName());
	ExcludeClassesList.Add(UDialogueWave::StaticClass()->GetFName());
	ExcludeClassesList.Add(UMaterial::StaticClass()->GetFName());
	ExcludeClassesList.Add(UMaterialFunction::StaticClass()->GetFName());
	ExcludeClassesList.Add(UMaterialInstanceConstant::StaticClass()->GetFName());
	ExcludeClassesList.Add(UMaterialParameterCollection::StaticClass()->GetFName());
	ExcludeClassesList.Add(UParticleSystem::StaticClass()->GetFName());
	ExcludeClassesList.Add(UPhysicalMaterial::StaticClass()->GetFName());
	ExcludeClassesList.Add(UReverbEffect::StaticClass()->GetFName());
	ExcludeClassesList.Add(USlateWidgetStyleAsset::StaticClass()->GetFName());
	ExcludeClassesList.Add(USlateBrushAsset::StaticClass()->GetFName());
	ExcludeClassesList.Add(USoundAttenuation::StaticClass()->GetFName());
	ExcludeClassesList.Add(UStaticMesh::StaticClass()->GetFName());
	ExcludeClassesList.Add(USkeletalMesh::StaticClass()->GetFName());
	ExcludeClassesList.Add(USkeleton::StaticClass()->GetFName());
	ExcludeClassesList.Add(USoundClass::StaticClass()->GetFName());
	ExcludeClassesList.Add(USoundCue::StaticClass()->GetFName());
	ExcludeClassesList.Add(USoundMix::StaticClass()->GetFName());
	ExcludeClassesList.Add(USoundWave::StaticClass()->GetFName());
	ExcludeClassesList.Add(USubsurfaceProfile::StaticClass()->GetFName());
	ExcludeClassesList.Add(UFont::StaticClass()->GetFName());
	ExcludeClassesList.Add(UBlueprint::StaticClass()->GetFName());
	ExcludeClassesList.Add(UAnimBlueprint::StaticClass()->GetFName());

	//Generating the list of assets.
	//This list isn't expected to have anything that may be obtained with another function.
	//This list is being filtered by the game folder and class type.  The results are placed into the ObjectList variable.
	AssetFilter.PackagePaths.Add("/Game");
	AssetFilter.bRecursivePaths = true;
	AssetRegistryModule.Get().GetAssets(AssetFilter, ObjectList);
	
	//if (ObjectList.Num() < 25000)
	//{

		//Loop through the list of assets, make their path full and a string, then add them to the test.
		for (auto ObjIter = ObjectList.CreateConstIterator(); ObjIter; ++ObjIter)
		{
			const FAssetData & Asset = *ObjIter;
			//First we check if the class is valid.  If not then we move onto the next object.
			if (Asset.GetClass() != NULL)
			{
				//Variable that holds the class FName for the current asset iteration.
				FName AssetClassFName = Asset.GetClass()->GetFName();

				//Counter used to keep track for the following for loop.
				float ExcludedClassesCounter = 1;

				for (auto ExcludeIter = ExcludeClassesList.CreateConstIterator(); ExcludeIter; ++ExcludeIter)
				{
					FName ExludedName = *ExcludeIter;

					//If the classes are the same then we don't want this asset. So we move onto the next one instead.
					if (AssetClassFName == ExludedName)
					{
						break;
					}

					//We run out of class names in our Excluded list then we want the current ObjectList asset.
					if ((ExcludedClassesCounter + 1) > ExcludeClassesList.Num())
					{
						FString Filename = Asset.ObjectPath.ToString();
						//convert to full paths
						Filename = FPackageName::LongPackageNameToFilename(Filename);

						if (FAutomationTestFramework::GetInstance().ShouldTestContent(Filename))
						{
							FString BeautifiedFilename = Asset.AssetName.ToString();
							OutBeautifiedNames.Add(BeautifiedFilename);
							OutTestCommands.Add(Asset.ObjectPath.ToString());
						}

						break;
					}

					//increment the counter.
					ExcludedClassesCounter++;
				}
			}
		}
	//}
}

