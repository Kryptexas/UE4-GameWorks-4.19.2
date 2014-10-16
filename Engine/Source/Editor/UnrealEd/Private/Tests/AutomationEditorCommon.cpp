// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "AutomationEditorCommon.h"
#include "AssetEditorManager.h"
#include "ModuleManager.h"
#include "LevelEditor.h"
#include "Editor/MainFrame/Public/MainFrame.h"
#include "EngineVersion.h"
#include "ShaderCompiler.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Engine/DestructibleMesh.h"
#include "FileManagerGeneric.h"


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
				if (Factory->bEditorImport)
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
* @param InTestName is the folder that has the same name as the test. (For Example: "Performance").
* @param InItemBeingTested is the name for the thing that is being tested. (For Example: "MapName").
* @param InFileName is the name of the file with an extension
* @param InNumberToBeWritten is the float number that is expected to be written to the file.
* @param Delimiter is the delimiter to be used. TEXT(",")
*/
void WriteToTextFile(const FString& InTestName ,const FString& InItemBeingTested, const FString& InFileName, const float& InNumberToBeWritten, const FString& Delimiter)
{
	//Performance file locations and setups.
	FString FileSaveLocation = FPaths::Combine(*FPaths::AutomationLogDir(), *InTestName, *InItemBeingTested, *InFileName);

	if (FPaths::FileExists(FileSaveLocation))
	{
		//The text files existing content.
		FString TextFileContents;

		//Write to the text file the combined contents from the text file with the number to write.
		FFileHelper::LoadFileToString(TextFileContents, *FileSaveLocation);
		FString FileSetup = TextFileContents + Delimiter + FString::SanitizeFloat(InNumberToBeWritten);
		FFileHelper::SaveStringToFile(FileSetup, *FileSaveLocation);
		return;
	}

	FFileHelper::SaveStringToFile(FString::SanitizeFloat(InNumberToBeWritten), *FileSaveLocation);
}

/**
* Returns the sum of the numbers available in an array of float.
* @param InFloatArray is the name of the array intended to be used.
* @param bisAveragedInstead will return the average of the available numbers instead of the sum.
*/
float TotalFromFloatArray(const TArray<float>& InFloatArray, bool bisAveragedInstead)
{
	//Total Value holds the sum of all the numbers available in the array.
	float TotalValue = 0;

	//Get the sum of the array.
	for (int32 I = 0; I < InFloatArray.Num(); ++I)
	{
		TotalValue += InFloatArray[I];
	}

	//If bAverageInstead equals true then only the average is returned.
	if (bisAveragedInstead)
	{
		UE_LOG(LogEditorAutomationTests, VeryVerbose, TEXT("Average value of the Array is %f"), (TotalValue / InFloatArray.Num()));
		return (TotalValue / InFloatArray.Num());
	}

	UE_LOG(LogEditorAutomationTests, VeryVerbose, TEXT("Total Value of the Array is %f"), TotalValue);
	return TotalValue;
}

/**
* Returns the largest value from an array of float numbers.
* @param InFloatArray is the name of the array intended to be used.
*/
float LargetValueInFloatArray(const TArray<float>& InFloatArray)
{
	//Total Value holds the sum of all the numbers available in the array.
	float LargestValue = 0;

	//Find the largest value
	for (int32 I = 0; I < InFloatArray.Num(); ++I)
	{
		if (LargestValue < InFloatArray[I])
		{
			LargestValue = InFloatArray[I];
		}
	}
	UE_LOG(LogEditorAutomationTests, VeryVerbose, TEXT("The Largest value of the array is %f"), LargestValue);
	return LargestValue;
}

/**
* Returns the contents of a text file as an array of FString.
* @param InFileLocation is the location of the file.
*/
TArray<FString> CreateArrayFromFile(const FString& InFileLocation)
{
	FString RawData;
	TArray<FString> DataArray;

	if (FPaths::FileExists(*InFileLocation))
	{
		UE_LOG(LogEditorAutomationTests, VeryVerbose, TEXT("Loading and parsing the data from '%s' into an array."), *InFileLocation);
		FFileHelper::LoadFileToString(RawData, *InFileLocation);
		RawData.ParseIntoArray(&DataArray, TEXT(","), false);

		return DataArray;
	}

	UE_LOG(LogEditorAutomationTests, Warning, TEXT("Unable to create an array.  '%s' does not exist."), *InFileLocation);
	RawData = TEXT("0");
	DataArray.Add(RawData);
	
	return DataArray;
}

bool IsArchiveWriteable(const FString InFilePath, const FArchive* InArchiveName)
{
	if (!InArchiveName)
	{
		UE_LOG(LogEditorAutomationTests, Error, TEXT("Failed to write to the csv file: %s"), *FPaths::ConvertRelativePathToFull(InFilePath));
		return false;
	}
	return true;
}

/**
* Dumps the information held within the EditorPerfCaptureParameters struct into a CSV file.
* @param EditorPerfStats is the name of the struct that holds the needed performance information.
*/
void EditorPerfDump(EditorPerfCaptureParameters& EditorPerfStats)
{
	UE_LOG(LogEditorAutomationTests, Log, TEXT("Begin generating the editor performance charts."));

	//The file location where to save the data.
	FString DataFileLocation = FPaths::Combine(*FPaths::AutomationLogDir(), TEXT("Performance"), *EditorPerfStats.MapName);

	//Get the map load time (in seconds) from the text file that is created when the load map latent command is ran.
	EditorPerfStats.MapLoadTime = 0;
	FString MapLoadTimeFileLocation = FPaths::Combine(*DataFileLocation, TEXT("RAWMapLoadTime.txt"));
	if (FPaths::FileExists(*MapLoadTimeFileLocation))
	{
		TArray<FString> SavedMapLoadTimes = CreateArrayFromFile(MapLoadTimeFileLocation);
		EditorPerfStats.MapLoadTime = FCString::Atof(*SavedMapLoadTimes.Last());
	}

	//Filename for the RAW csv which holds the data gathered from a single test ran.
	FString RAWCSVFilePath = FString::Printf(TEXT("%s/RAW_%s_%s.csv"), *DataFileLocation, *EditorPerfStats.MapName, *FDateTime::Now().ToString());

	//Filename for the pretty csv file.
	FString PerfCSVFilePath = FString::Printf(TEXT("%s/%s_Performance.csv"), *DataFileLocation, *EditorPerfStats.MapName);

	//Create the raw csv and then add the title row it.
	FArchive* RAWCSVArchive = IFileManager::Get().CreateFileWriter(*RAWCSVFilePath);
	FString RAWCSVLine = (TEXT("Map Name, Changelist, Time Stamp, Map Load Time, Average FPS, Frame Time, Used Physical Memory, Used Virtual Memory, Used Peak Physical, Used Peak Virtual, Available Physical Memory, Available Virtual Memory\n"));
	RAWCSVArchive->Serialize(TCHAR_TO_ANSI(*RAWCSVLine), RAWCSVLine.Len());

	//Dump the stats from each run to the raw csv file and then close it.
	for (int32 I = 0; I < EditorPerfStats.TimeStamp.Num(); I++)
	{
		//If the raw file isn't available to write to then we'll fail back this test.
		if (IsArchiveWriteable(RAWCSVFilePath, RAWCSVArchive))
		{
			RAWCSVLine = FString::Printf(TEXT("%s,%s,%s,%.3f,%.1f,%.1f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f%s"), *EditorPerfStats.MapName, *GEngineVersion.ToString(EVersionComponent::Changelist), *EditorPerfStats.TimeStamp[I].ToString(), EditorPerfStats.MapLoadTime, EditorPerfStats.AverageFPS[I], EditorPerfStats.AverageFrameTime[I], EditorPerfStats.UsedPhysical[I], EditorPerfStats.UsedVirtual[I], EditorPerfStats.PeakUsedPhysical[I], EditorPerfStats.PeakUsedVirtual[I], EditorPerfStats.AvailablePhysical[I], EditorPerfStats.AvailableVirtual[I], LINE_TERMINATOR);
			RAWCSVArchive->Serialize(TCHAR_TO_ANSI(*RAWCSVLine), RAWCSVLine.Len());
		}
	}
	RAWCSVArchive->Close();

	//Get the final pretty data for the Performance csv file.
	float AverageFPS = TotalFromFloatArray(EditorPerfStats.AverageFPS, true);
	float AverageFrameTime = TotalFromFloatArray(EditorPerfStats.AverageFrameTime, true);
	float MemoryUsedPhysical = TotalFromFloatArray(EditorPerfStats.UsedPhysical, true);
	float MemoryAvailPhysAvg = TotalFromFloatArray(EditorPerfStats.AvailablePhysical, true);
	float MemoryAvailVirtualAvg = TotalFromFloatArray(EditorPerfStats.AvailableVirtual, true);
	float MemoryUsedVirtualAvg = TotalFromFloatArray(EditorPerfStats.UsedVirtual, true);
	float MemoryUsedPeak = LargetValueInFloatArray(EditorPerfStats.PeakUsedPhysical);
	float MemoryUsedPeakVirtual = LargetValueInFloatArray(EditorPerfStats.PeakUsedVirtual);

	//TestRunDuration is the length of time the test lasted in ticks.
	FTimespan TestRunDuration = (EditorPerfStats.TimeStamp.Last().GetTicks() - EditorPerfStats.TimeStamp[0].GetTicks()) + ETimespan::TicksPerSecond;

	//The performance csv file will be created if it didn't exist prior to the start of this test.
	if (!FPaths::FileExists(*PerfCSVFilePath))
	{
		FArchive* FinalCSVArchive = IFileManager::Get().CreateFileWriter(*PerfCSVFilePath);
		if (IsArchiveWriteable(PerfCSVFilePath, FinalCSVArchive))
		{
			FString FinalCSVLine = (TEXT("Date, Map Name, Changelist, Test Run Time , Map Load Time, Average FPS, Average MS, Used Physical KB, Used Virtual KB, Used Peak Physcial KB, Used Peak Virtual KB, Available Physical KB, Available Virtual KB\n"));
			FinalCSVArchive->Serialize(TCHAR_TO_ANSI(*FinalCSVLine), FinalCSVLine.Len());
			FinalCSVArchive->Close();
		}
	}

	//Load the existing performance csv so that it doesn't get saved over and lost.
	FString OldPerformanceCSVFile;
	FFileHelper::LoadFileToString(OldPerformanceCSVFile, *PerfCSVFilePath);
	FArchive* FinalCSVArchive = IFileManager::Get().CreateFileWriter(*PerfCSVFilePath);
	if (IsArchiveWriteable(PerfCSVFilePath, FinalCSVArchive))
	{
		//Dump the old performance csv file data to the new csv file.
		FinalCSVArchive->Serialize(TCHAR_TO_ANSI(*OldPerformanceCSVFile), OldPerformanceCSVFile.Len());

		//Dump the pretty stats to the Performance CSV file and then close it so we can edit it while the engine is still running.
		FString FinalCSVLine = FString::Printf(TEXT("%s,%s,%s,%.0f,%.3f,%.1f,%.1f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f%s"), *FDateTime::Now().ToString(), *EditorPerfStats.MapName, *GEngineVersion.ToString(EVersionComponent::Changelist), TestRunDuration.GetTotalSeconds(), EditorPerfStats.MapLoadTime, AverageFPS, AverageFrameTime, MemoryUsedPhysical, MemoryUsedVirtualAvg, MemoryUsedPeak, MemoryUsedPeakVirtual, MemoryAvailPhysAvg, MemoryAvailVirtualAvg, LINE_TERMINATOR);
		FinalCSVArchive->Serialize(TCHAR_TO_ANSI(*FinalCSVLine), FinalCSVLine.Len());
		FinalCSVArchive->Close();
	}

	//Display the test results to the user.
	UE_LOG(LogEditorAutomationTests, Display, TEXT("AVG FPS: '%.1f'"), AverageFPS);
	UE_LOG(LogEditorAutomationTests, Display, TEXT("AVG Frame Time: '%.1f' ms"), AverageFrameTime);
	UE_LOG(LogEditorAutomationTests, Display, TEXT("AVG Used Physical Memory: '%.0f' kb"), MemoryUsedPhysical);
	UE_LOG(LogEditorAutomationTests, Display, TEXT("AVG Used Virtual Memory: '%.0f' kb"), MemoryUsedVirtualAvg);
	UE_LOG(LogEditorAutomationTests, Display, TEXT("Performance csv file is located here: %s"), *FPaths::ConvertRelativePathToFull(PerfCSVFilePath));
	UE_LOG(LogEditorAutomationTests, Log, TEXT("Performance csv file is located here: %s"), *FPaths::ConvertRelativePathToFull(PerfCSVFilePath));
	UE_LOG(LogEditorAutomationTests, Log, TEXT("Raw performance csv file is located here: %s"), *FPaths::ConvertRelativePathToFull(RAWCSVFilePath));
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
* This command grabs the FPS and Memory stats for the current editor session.
*/
bool FEditorPerfCaptureCommand::Update()
{
	//Capture the current time stamp and format it to YYYY-MM-DD HH:MM:SS.mmm.
	FDateTime CurrentDateAndTime = FDateTime::Now();

	//This is how long it has been since the last run through.
	FTimespan ElapsedTime = 0;
	FTimespan TimeBetweenCaptures;

	//We want to only capture data every whole second.
	if (EditorPerfStats.TimeStamp.Num() > 0)
	{
		TimeBetweenCaptures = CurrentDateAndTime.GetTicks() - EditorPerfStats.TimeStamp.Last().GetTicks();
		if (TimeBetweenCaptures.GetTicks() < ETimespan::TicksPerSecond)
		{
			return false;
		}
		
		ElapsedTime = CurrentDateAndTime.GetTicks() - EditorPerfStats.TimeStamp[0].GetTicks();
	}

	if (ElapsedTime.GetTotalSeconds() <= EditorPerfStats.TestDuration)
	{
		//Find the Average FPS
		//Clamp to avoid huge averages at startup or after hitches
		const float CurrentFPS = 1.0f / FSlateApplication::Get().GetAverageDeltaTime();
		const float ClampedFPS = (CurrentFPS < 0.0f || CurrentFPS > 4000.0f) ? 0.0f : CurrentFPS;
		EditorPerfStats.AverageFPS.Add(ClampedFPS);

		//Find the Frame Time in ms.
		//Clamp to avoid huge averages at startup or after hitches
		const float AverageMS = FSlateApplication::Get().GetAverageDeltaTime() * 1000.0f;
		const float ClampedMS = (AverageMS < 0.0f || AverageMS > 4000.0f) ? 0.0f : AverageMS;
		EditorPerfStats.AverageFrameTime.Add(ClampedMS);

		//Query OS for process memory used.
		FPlatformMemoryStats MemoryStats = FPlatformMemory::GetStats();
		EditorPerfStats.UsedPhysical.Add((float)MemoryStats.UsedPhysical / 1024);

		//Query OS for available physical memory
		EditorPerfStats.AvailablePhysical.Add((float)MemoryStats.AvailablePhysical / 1024);

		//Query OS for available virtual memory
		EditorPerfStats.AvailableVirtual.Add((float)MemoryStats.AvailableVirtual / 1024);

		//Query OS for used virtual memory
		EditorPerfStats.UsedVirtual.Add((float)MemoryStats.UsedVirtual / 1024);

		//Query OS for used Peak Used physical memory
		EditorPerfStats.PeakUsedPhysical.Add((float)MemoryStats.PeakUsedPhysical / 1024);

		//Query OS for used Peak Used virtual memory
		EditorPerfStats.PeakUsedVirtual.Add((float)MemoryStats.PeakUsedVirtual / 1024);

		//Capture the time stamp.
		FString FormatedTimeStamp = FString::Printf(TEXT("%04i-%02i-%02i %02i:%02i:%02i.%03i"), CurrentDateAndTime.GetYear(), CurrentDateAndTime.GetMonth(), CurrentDateAndTime.GetDay(), CurrentDateAndTime.GetHour(), CurrentDateAndTime.GetMinute(), CurrentDateAndTime.GetSecond(), CurrentDateAndTime.GetMillisecond());
		EditorPerfStats.FormattedTimeStamp.Add(FormatedTimeStamp);
		EditorPerfStats.TimeStamp.Add(CurrentDateAndTime);

		return false;
	}

	//Dump the performance data in a csv file.
	EditorPerfDump(EditorPerfStats);

	return true;
}

/**
* This this command loads a map into the editor.
*/
bool FEditorLoadMap::Update()
{
	//Get the base filename for the map that will be used.
	FString ShortMapName = FPaths::GetBaseFilename(MapName);

	//Get the current number of seconds before loading the map.
	double MapLoadStartTime = FPlatformTime::Seconds();

	//Load the map
	FEditorAutomationTestUtilities::LoadMap(MapName);

	//This is the time it took to load the map in the editor.
	double MapLoadTime = FPlatformTime::Seconds() - MapLoadStartTime;

	//Gets the main frame module to get the name of our current level.
	const IMainFrameModule& MainFrameModule = FModuleManager::GetModuleChecked< IMainFrameModule >("MainFrame");
	FString LoadedMapName = MainFrameModule.GetLoadedLevelName();

	UE_LOG(LogEditorAutomationTests, Log, TEXT("%s has been loaded."), *ShortMapName);

	//Log out to a text file the time it takes to load the map.
	WriteToTextFile(TEXT("Performance"), LoadedMapName, TEXT("RAWMapLoadTime.txt"), MapLoadTime, TEXT(","));
	
	UE_LOG(LogEditorAutomationTests, Display, TEXT("%s took %.3f to load."), *LoadedMapName, MapLoadTime);
	
	return true;
}

/**
* This will cause the test to wait for the shaders to finish compiling before moving on.
*/
bool FWaitForShadersToFinishCompiling::Update()
{
	UE_LOG(LogEditorAutomationTests, Log, TEXT("Waiting for %i shaders to finish."), GShaderCompilingManager->GetNumRemainingJobs());
	GShaderCompilingManager->FinishAllCompilation();
	UE_LOG(LogEditorAutomationTests, Log, TEXT("Done waiting for shaders to finish."));
	return true;
}

/**
* Latent command that changes the editor viewport to the first available bookmarked view.
*/
bool FChangeViewportToFirstAvailableBookmarkCommand::Update()
{
	FEditorModeTools EditorModeTools;
	FLevelEditorViewportClient* ViewportClient;
	uint32 ViewportIndex = 0;

	UE_LOG(LogEditorAutomationTests, Log, TEXT("Attempting to change the editor viewports view to the first set bookmark."));

	//Move the perspective viewport view to show the test.
	for (int32 i = 0; i < GEditor->LevelViewportClients.Num(); i++)
	{
		ViewportClient = GEditor->LevelViewportClients[i];

		for (ViewportIndex = 0; ViewportIndex <= AWorldSettings::MAX_BOOKMARK_NUMBER; ViewportIndex++)
		{
			if (EditorModeTools.CheckBookmark(ViewportIndex, ViewportClient))
			{
				UE_LOG(LogEditorAutomationTests, VeryVerbose, TEXT("Changing a viewport view to the set bookmark %i"), ViewportIndex);
				EditorModeTools.JumpToBookmark(ViewportIndex, true, ViewportClient);
				break;
			}
		}
	}
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
		const FAssetData& Asset = *ObjIter;
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
		const FAssetData& Asset = *ObjIter;
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
	ExcludeClassesList.Add(UAimOffsetBlendSpace::StaticClass()->GetFName());
	ExcludeClassesList.Add(UAimOffsetBlendSpace1D::StaticClass()->GetFName());
	ExcludeClassesList.Add(UAnimBlueprint::StaticClass()->GetFName());
	ExcludeClassesList.Add(UAnimMontage::StaticClass()->GetFName());
	ExcludeClassesList.Add(UAnimSequence::StaticClass()->GetFName());
	ExcludeClassesList.Add(UBehaviorTree::StaticClass()->GetFName());
	ExcludeClassesList.Add(UBlendSpace::StaticClass()->GetFName());
	ExcludeClassesList.Add(UBlendSpace1D::StaticClass()->GetFName());
	ExcludeClassesList.Add(UBlueprint::StaticClass()->GetFName());
	ExcludeClassesList.Add(UDestructibleMesh::StaticClass()->GetFName());
	ExcludeClassesList.Add(UDialogueVoice::StaticClass()->GetFName());
	ExcludeClassesList.Add(UDialogueWave::StaticClass()->GetFName());
	ExcludeClassesList.Add(UFont::StaticClass()->GetFName());
	ExcludeClassesList.Add(UMaterial::StaticClass()->GetFName());
	ExcludeClassesList.Add(UMaterialFunction::StaticClass()->GetFName());
	ExcludeClassesList.Add(UMaterialInstanceConstant::StaticClass()->GetFName());
	ExcludeClassesList.Add(UMaterialParameterCollection::StaticClass()->GetFName());
	ExcludeClassesList.Add(UParticleSystem::StaticClass()->GetFName());
	ExcludeClassesList.Add(UPhysicalMaterial::StaticClass()->GetFName());
	ExcludeClassesList.Add(UPhysicsAsset::StaticClass()->GetFName());
	ExcludeClassesList.Add(UReverbEffect::StaticClass()->GetFName());
	ExcludeClassesList.Add(USkeletalMesh::StaticClass()->GetFName());
	ExcludeClassesList.Add(USkeleton::StaticClass()->GetFName());
	ExcludeClassesList.Add(USlateBrushAsset::StaticClass()->GetFName());
	ExcludeClassesList.Add(USlateWidgetStyleAsset::StaticClass()->GetFName());
	ExcludeClassesList.Add(USoundAttenuation::StaticClass()->GetFName());
	ExcludeClassesList.Add(USoundClass::StaticClass()->GetFName());
	ExcludeClassesList.Add(USoundCue::StaticClass()->GetFName());
	ExcludeClassesList.Add(USoundMix::StaticClass()->GetFName());
	ExcludeClassesList.Add(USoundWave::StaticClass()->GetFName());
	ExcludeClassesList.Add(UStaticMesh::StaticClass()->GetFName());
	ExcludeClassesList.Add(USubsurfaceProfile::StaticClass()->GetFName());
	ExcludeClassesList.Add(UTexture::StaticClass()->GetFName());
	ExcludeClassesList.Add(UTexture2D::StaticClass()->GetFName());
	ExcludeClassesList.Add(UTextureCube::StaticClass()->GetFName());
	ExcludeClassesList.Add(UTextureRenderTarget::StaticClass()->GetFName());
	ExcludeClassesList.Add(UTextureRenderTarget2D::StaticClass()->GetFName());
	ExcludeClassesList.Add(UUserDefinedEnum::StaticClass()->GetFName());
	ExcludeClassesList.Add(UWorld::StaticClass()->GetFName());

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
			const FAssetData& Asset = *ObjIter;
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

