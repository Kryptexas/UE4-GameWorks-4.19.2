// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ProjectsPrivatePCH.h"
#include "EngineVersion.h"


#define LOCTEXT_NAMESPACE "PluginManagerShared"


void FProjectOrPlugin::LoadModules( const ELoadingPhase::Type LoadingPhase, TMap<FName, EModuleLoadResult>& ModuleLoadErrors )
{
	const FProjectOrPluginInfo& ProjectOrPluginInfo = GetProjectOrPluginInfo();

	for( TArray< FModuleDescriptor >::TConstIterator ModuleInfoIt( ProjectOrPluginInfo.Modules.CreateConstIterator() ); ModuleInfoIt; ++ModuleInfoIt )
	{
		const FModuleDescriptor& ModuleInfo = *ModuleInfoIt;

		// Don't need to do anything if this module is already loaded
		if( !FModuleManager::Get().IsModuleLoaded( ModuleInfo.Name ) )
		{
			if( LoadingPhase == ModuleInfo.LoadingPhase && ModuleInfo.IsLoadedInCurrentConfiguration() )
			{
				// @todo plugin: DLL search problems.  Plugins that statically depend on other modules within this plugin may not be found?  Need to test this.

				// NOTE: Loading this module may cause other modules to become loaded, both in the engine or game, or other modules 
				//       that are part of this project or plugin.  That's totally fine.
				EModuleLoadResult FailureReason;
				const TSharedPtr<IModuleInterface>& ModuleInterface = FModuleManager::Get().LoadModuleWithFailureReason( ModuleInfo.Name, FailureReason );
				if( ModuleInterface.IsValid() )
				{
					// Module loaded OK (or was already loaded.)
				}
				else 
				{
					// The module failed to load. Note this in the ModuleLoadErrors list.
					ModuleLoadErrors.Add(ModuleInfo.Name, FailureReason);
				}
			}
		}
	}
}

bool FProjectOrPlugin::LoadFromFile( const FString& FileToLoad, FText& OutFailureReason )
{
	const FText FileDisplayName = FText::FromString(FileToLoad);

	FString FileContents;
	if (!FFileHelper::LoadFileToString(FileContents, *FileToLoad))
	{
		// Failed to read project file
		OutFailureReason = FText::Format( LOCTEXT("FailedToLoadDescriptorFile", "Failed to open descriptor file '{0}'"), FileDisplayName );
		return false;
	}

	if (FileContents.IsEmpty())
	{
		// Empty project file
		OutFailureReason = FText::Format( LOCTEXT("DescriptorFileEmpty", "Descriptor file '{0}' was empty."), FileDisplayName );
		return false;
	}

	// Serialize the JSON file contents
	FText DeserializeFailReason;
	if (!DeserializeFromJSON(FileContents, DeserializeFailReason))
	{
		OutFailureReason = FText::Format( LOCTEXT("DescriptorDeserializeFailed", "'Descriptor file '{0}' could not be loaded. {1}"), FileDisplayName, DeserializeFailReason );
		return false;
	}

	FProjectOrPluginInfo& ProjectOrPluginInfo = GetProjectOrPluginInfo();
	ProjectOrPluginInfo.Name = FPaths::GetBaseFilename(FileToLoad);
	ProjectOrPluginInfo.LoadedFromFile = FileToLoad;

	return true;
}

bool FProjectOrPlugin::DeserializeFromJSON( const FString& JSONInput, FText& OutFailReason )
{
	FProjectOrPluginInfo& ProjectOrPluginInfo = GetProjectOrPluginInfo();

	TSharedPtr< FJsonObject > FileObjectPtr;
	TSharedRef< TJsonReader<> > Reader = TJsonReaderFactory<>::Create(JSONInput);
	if ( !FJsonSerializer::Deserialize(Reader, FileObjectPtr) || !FileObjectPtr.IsValid() )
	{
		OutFailReason = FText::Format( LOCTEXT("FailedToReadDescriptorFile", "Failed to read file. {1}"), FText::FromString(Reader->GetErrorMessage()) );
		return false;
	}

	TSharedRef< FJsonObject > FileObject = FileObjectPtr.ToSharedRef();

	bool bSuccessful = true;

	// FileVersion. This is not optional.
	if ( ReadFileVersionFromJSON(FileObject, ProjectOrPluginInfo.FileVersion) )
	{
		if ( ProjectOrPluginInfo.FileVersion == INDEX_NONE )
		{
			OutFailReason = LOCTEXT("InvalidProjectFileVersion", "File does not have a valid 'FileVersion' number.");
			bSuccessful = false;
		}

		if ( ProjectOrPluginInfo.FileVersion > VER_LATEST_PROJECT_FILE )
		{
			OutFailReason = FText::Format( LOCTEXT("ProjectFileVersionTooLarge", "File appears to be in a newer version ({0}) of the file format that we can load (max version: {1})."),
				FText::FromString( FString::Printf( TEXT( "%d" ), ProjectOrPluginInfo.FileVersion ) ),
				FText::FromString( FString::Printf( TEXT( "%d" ), (int32)VER_LATEST_PROJECT_FILE ) )
				);

			bSuccessful = false;
		}
	}
	else
	{
		OutFailReason = LOCTEXT("NoProjectFileVersion", "File is missing a 'FileVersion' field.  This is required in order to load the file.");
		bSuccessful = false;
	}

	if ( bSuccessful )
	{
		ReadNumberFromJSON(FileObject, TEXT("Version"), ProjectOrPluginInfo.Version);
		ReadStringFromJSON(FileObject, TEXT("VersionName"), ProjectOrPluginInfo.VersionName);
	
		FString EngineVersionString;
		ReadStringFromJSON(FileObject, TEXT("EngineVersion"), EngineVersionString);
		FEngineVersion::Parse(EngineVersionString, ProjectOrPluginInfo.EngineVersion);

		ReadNumberFromJSON(FileObject, TEXT("PackageFileUE4Version"), ProjectOrPluginInfo.PackageFileUE4Version);
		ReadNumberFromJSON(FileObject, TEXT("PackageFileLicenseeUE4Version"), ProjectOrPluginInfo.PackageFileLicenseeUE4Version);
		ReadStringFromJSON(FileObject, TEXT("EngineAssociation"), ProjectOrPluginInfo.EngineAssociation);
		ReadStringFromJSON(FileObject, TEXT("FriendlyName"), ProjectOrPluginInfo.FriendlyName);
		ReadStringFromJSON(FileObject, TEXT("Description"), ProjectOrPluginInfo.Description);

		if ( !ReadStringFromJSON(FileObject, TEXT("Category"), ProjectOrPluginInfo.Category) )
		{
			// Category used to be called CategoryPath in .uplugin files
			ReadStringFromJSON(FileObject, TEXT("CategoryPath"), ProjectOrPluginInfo.Category);
		}
        
		// Due to a difference in command line parsing between Windows and Mac, we shipped a few Mac samples containing
		// a category name with escaped quotes. Remove them here to make sure we can list them in the right category.
		if(ProjectOrPluginInfo.Category.Len() >= 2 && ProjectOrPluginInfo.Category.StartsWith(TEXT("\"")) && ProjectOrPluginInfo.Category.EndsWith(TEXT("\"")))
		{
			ProjectOrPluginInfo.Category = ProjectOrPluginInfo.Category.Mid(1, ProjectOrPluginInfo.Category.Len() - 2);
		}

		ReadStringFromJSON(FileObject, TEXT("CreatedBy"), ProjectOrPluginInfo.CreatedBy);
		ReadStringFromJSON(FileObject, TEXT("CreatedByURL"), ProjectOrPluginInfo.CreatedByURL);

		if ( ReadModulesFromJSON(FileObject, ProjectOrPluginInfo.Modules, OutFailReason) )
		{
			ProjectOrPluginInfo.EarliestModulePhase = FProjectAndPluginManager::GetEarliestPhaseFromModules(ProjectOrPluginInfo.Modules);
		}
		else
		{
			bSuccessful = false;
		}
	}

	if ( bSuccessful )
	{
		bSuccessful = PerformAdditionalDeserialization(FileObject);
	}

	return bSuccessful;
}

FString FProjectOrPlugin::SerializeToJSON( ) const
{
	const FProjectOrPluginInfo& ProjectOrPluginInfo = GetProjectOrPluginInfo();

	FString JSONOutput;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&JSONOutput);

	Writer->WriteObjectStart();

	Writer->WriteValue(TEXT("FileVersion"), (float)ProjectOrPluginInfo.FileVersion);
	Writer->WriteValue(TEXT("Version"), (float)ProjectOrPluginInfo.Version);
	Writer->WriteValue(TEXT("VersionName"), ProjectOrPluginInfo.VersionName);
	Writer->WriteValue(TEXT("EngineVersion"), ProjectOrPluginInfo.EngineVersion.ToString());
	Writer->WriteValue(TEXT("PackageFileUE4Version"), (float)ProjectOrPluginInfo.PackageFileUE4Version);
	Writer->WriteValue(TEXT("PackageFileLicenseeUE4Version"), (float)ProjectOrPluginInfo.PackageFileLicenseeUE4Version);
	Writer->WriteValue(TEXT("EngineAssociation"), ProjectOrPluginInfo.EngineAssociation);
	Writer->WriteValue(TEXT("FriendlyName"), ProjectOrPluginInfo.FriendlyName);
	Writer->WriteValue(TEXT("Description"), ProjectOrPluginInfo.Description);
	Writer->WriteValue(TEXT("Category"), ProjectOrPluginInfo.Category);
	Writer->WriteValue(TEXT("CreatedBy"), ProjectOrPluginInfo.CreatedBy);
	Writer->WriteValue(TEXT("CreatedByURL"), ProjectOrPluginInfo.CreatedByURL);
	
	FModuleDescriptor::WriteArray(Writer.Get(), TEXT("Modules"), ProjectOrPluginInfo.Modules);
	
	PerformAdditionalSerialization(Writer);
	
	Writer->WriteObjectEnd();
	Writer->Close();

	return JSONOutput;
}

bool FProjectOrPlugin::RequiresUpdate( ) const
{
	const FProjectOrPluginInfo& ProjectOrPluginInfo = GetProjectOrPluginInfo();
	return (ProjectOrPluginInfo.FileVersion < VER_LATEST_PROJECT_FILE);
}

bool FProjectOrPlugin::AreModulesUpToDate( ) const
{
	const FProjectOrPluginInfo& ProjectOrPluginInfo = GetProjectOrPluginInfo();
	for (TArray< FModuleDescriptor >::TConstIterator Iter(ProjectOrPluginInfo.Modules); Iter; ++Iter)
	{
		const FModuleDescriptor &ModuleInfo = *Iter;
		if (ModuleInfo.IsCompiledInCurrentConfiguration() && !FModuleManager::Get().IsModuleUpToDate(ModuleInfo.Name))
		{
			return false;
		}
	}
	return true;
}

void FProjectOrPlugin::UpdateVersionToCurrent( const FString &EngineIdentifier )
{
	FProjectOrPluginInfo& ProjectOrPluginInfo = GetProjectOrPluginInfo();

	ProjectOrPluginInfo.FileVersion = VER_LATEST_PROJECT_FILE;
	ProjectOrPluginInfo.EngineVersion = GEngineVersion;
	ProjectOrPluginInfo.PackageFileUE4Version = GPackageFileUE4Version;
	ProjectOrPluginInfo.PackageFileLicenseeUE4Version = GPackageFileLicenseeUE4Version;
	ProjectOrPluginInfo.EngineAssociation = EngineIdentifier;
}

void FProjectOrPlugin::ReplaceModulesInProject(const TArray<FString>* StartupModuleNames)
{
	if ( StartupModuleNames )
	{
		FProjectOrPluginInfo& ProjectOrPluginInfo = GetProjectOrPluginInfo();

		ProjectOrPluginInfo.Modules.Empty();
		for ( auto ModuleIt = StartupModuleNames->CreateConstIterator(); ModuleIt; ++ModuleIt )
		{
			FModuleDescriptor ModuleInfo;
			ModuleInfo.Name = FName(**ModuleIt);
			ProjectOrPluginInfo.Modules.Add(ModuleInfo);
		}
	}
}

bool FProjectOrPlugin::ReadNumberFromJSON(const TSharedRef< FJsonObject >& FileObject, const FString& PropertyName, int32& OutNumber ) const
{
	int64 LargeNumber;
	if(ReadNumberFromJSON(FileObject, PropertyName, LargeNumber))
	{
		OutNumber = (int32)FMath::Clamp<int64>(LargeNumber, INT_MIN, INT_MAX);
		return true;
	}
	return false;
}

bool FProjectOrPlugin::ReadNumberFromJSON(const TSharedRef< FJsonObject >& FileObject, const FString& PropertyName, uint32& OutNumber ) const
{
	int64 LargeNumber;
	if(ReadNumberFromJSON(FileObject, PropertyName, LargeNumber))
	{
		OutNumber = (uint32)FMath::Clamp<int64>(LargeNumber, 0, UINT_MAX);
		return true;
	}
	return false;
}

bool FProjectOrPlugin::ReadNumberFromJSON(const TSharedRef< FJsonObject >& FileObject, const FString& PropertyName, int64& OutNumber ) const
{
	if ( FileObject->HasTypedField<EJson::Number>(PropertyName) )
	{
		TSharedPtr<FJsonValue> NumberValue = FileObject->GetField<EJson::Number>(PropertyName);
		if ( NumberValue.IsValid() )
		{
			OutNumber = (int64)( NumberValue->AsNumber() + 0.5 );
			return true;
		}
	}
	// We are tolerant to number fields with quotes around them
	else if ( FileObject->HasTypedField<EJson::String>(PropertyName) )
	{
		TSharedPtr<FJsonValue> StringValue = FileObject->GetField<EJson::String>(PropertyName);
		if ( StringValue.IsValid() )
		{
			OutNumber = FCString::Atoi64( *StringValue->AsString() );
			return true;
		}
	}

	return false;
}

bool FProjectOrPlugin::ReadStringFromJSON(const TSharedRef< FJsonObject >& FileObject, const FString& PropertyName, FString& OutString ) const
{
	if ( FileObject->HasTypedField<EJson::String>(PropertyName) )
	{
		TSharedPtr<FJsonValue> StringValue = FileObject->GetField<EJson::String>(PropertyName);
		if ( StringValue.IsValid() )
		{
			OutString = StringValue->AsString();
			return true;
		}
	}

	return false;
}

bool FProjectOrPlugin::ReadBoolFromJSON(const TSharedRef< FJsonObject >& FileObject, const FString& PropertyName, bool& OutBool ) const
{
	if ( FileObject->HasTypedField<EJson::Boolean>(PropertyName) )
	{
		TSharedPtr<FJsonValue> BoolValue = FileObject->GetField<EJson::Boolean>(PropertyName);
		if ( BoolValue.IsValid() )
		{
			OutBool = BoolValue->AsBool();
			return true;
		}
	}

	return false;
}

bool FProjectOrPlugin::ReadFileVersionFromJSON(const TSharedRef< FJsonObject >& FileObject, int32& OutVersion ) const
{
	// Modern file version
	if ( ReadNumberFromJSON(FileObject, TEXT("FileVersion"), OutVersion) )
	{
		return true;
	}

	// Back compat with old project files
	if ( ReadNumberFromJSON(FileObject, TEXT("ProjectFileVersion"), OutVersion) )
	{
		return true;
	}

	// Back compat with old plugin files
	if ( ReadNumberFromJSON(FileObject, TEXT("PluginFileVersion"), OutVersion) )
	{
		return true;
	}

	return false;
}

bool FProjectOrPlugin::ReadModulesFromJSON(const TSharedRef< FJsonObject >& FileObject, TArray<FModuleDescriptor>& OutModules, FText& OutFailReason ) const
{
	return FModuleDescriptor::ReadArray(FileObject.Get(), TEXT("Modules"), OutModules, OutFailReason);
}



ELoadingPhase::Type FProjectAndPluginManager::GetEarliestPhaseFromModules(const TArray<FModuleDescriptor>& Modules)
{
	ELoadingPhase::Type EarliestPhase = ELoadingPhase::Default;

	for ( auto ModuleIt = Modules.CreateConstIterator(); ModuleIt; ++ModuleIt )
	{
		EarliestPhase = ELoadingPhase::Earliest(EarliestPhase, (*ModuleIt).LoadingPhase);
	}

	return EarliestPhase;
}

#undef LOCTEXT_NAMESPACE
