// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ProjectsPrivatePCH.h"
#include "EngineVersion.h"


#define LOCTEXT_NAMESPACE "PluginManagerShared"


void FProjectOrPlugin::LoadModules( const ELoadingPhase::Type LoadingPhase, TMap<FName, ELoadModuleFailureReason::Type>& ModuleLoadErrors )
{
	const FProjectOrPluginInfo& ProjectOrPluginInfo = GetProjectOrPluginInfo();

	// cache the string for the current platform
	static FString UBTPlatform(FPlatformMisc::GetUBTPlatform());

	for( TArray< FProjectOrPluginInfo::FModuleInfo >::TConstIterator ModuleInfoIt( ProjectOrPluginInfo.Modules.CreateConstIterator() ); ModuleInfoIt; ++ModuleInfoIt )
	{
		const FProjectOrPluginInfo::FModuleInfo& ModuleInfo = *ModuleInfoIt;

		// Don't need to do anything if this module is already loaded
		if( !FModuleManager::Get().IsModuleLoaded( ModuleInfo.Name ) )
		{
			bool bShouldLoadModule = false;
			// Only load modules that support this platform
			if ((ModuleInfo.WhitelistPlatforms.Num() == 0 || ModuleInfo.WhitelistPlatforms.Contains(UBTPlatform)) &&
				(ModuleInfo.BlacklistPlatforms.Num() == 0 || !ModuleInfo.BlacklistPlatforms.Contains(UBTPlatform)))
			{
				bShouldLoadModule = true;
			}

			// Only load modules for the current loading phase
			if( bShouldLoadModule && LoadingPhase == ModuleInfo.LoadingPhase )
			{
				bShouldLoadModule = false;
				switch( ModuleInfo.Type )
				{
				case FProjectOrPluginInfo::EModuleType::Runtime:
					{
#if WITH_ENGINE || WITH_PLUGIN_SUPPORT
						bShouldLoadModule = true;
#endif		// WITH_ENGINE
					}
					break;

				case FProjectOrPluginInfo::EModuleType::RuntimeNoCommandlet:
					{
#if WITH_ENGINE || WITH_PLUGIN_SUPPORT
						if (!IsRunningCommandlet())
						{
							bShouldLoadModule = true;
						}
#endif		// WITH_ENGINE
					}
					break;

				case FProjectOrPluginInfo::EModuleType::Developer:
					{
#if WITH_UNREAL_DEVELOPER_TOOLS
						bShouldLoadModule = true;
#endif		// WITH_UNREAL_DEVELOPER_TOOLS
					}
					break;

				case FProjectOrPluginInfo::EModuleType::Editor:
					{
#if WITH_EDITOR
						if( GIsEditor )
						{
							bShouldLoadModule = true;
						}
#endif		// WITH_EDITOR
					}
					break;
					
				case FProjectOrPluginInfo::EModuleType::EditorNoCommandlet:
					{
#if WITH_EDITOR
						if( GIsEditor && !IsRunningCommandlet() )
						{
							bShouldLoadModule = true;
						}
#endif		// WITH_EDITOR
					}
					break;
				}


				if( bShouldLoadModule )
				{
					// @todo plugin: DLL search problems.  Plugins that statically depend on other modules within this plugin may not be found?  Need to test this.

					// NOTE: Loading this module may cause other modules to become loaded, both in the engine or game, or other modules 
					//       that are part of this project or plugin.  That's totally fine.
					ELoadModuleFailureReason::Type FailureReason;
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
		ReadStringFromJSON(FileObject, TEXT("FriendlyName"), ProjectOrPluginInfo.FriendlyName);
		ReadStringFromJSON(FileObject, TEXT("Description"), ProjectOrPluginInfo.Description);

		if ( !ReadStringFromJSON(FileObject, TEXT("Category"), ProjectOrPluginInfo.Category) )
		{
			// Category used to be called CategoryPath in .uplugin files
			ReadStringFromJSON(FileObject, TEXT("CategoryPath"), ProjectOrPluginInfo.Category);
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
	Writer->WriteValue(TEXT("FriendlyName"), ProjectOrPluginInfo.FriendlyName);
	Writer->WriteValue(TEXT("Description"), ProjectOrPluginInfo.Description);
	Writer->WriteValue(TEXT("Category"), ProjectOrPluginInfo.Category);
	Writer->WriteValue(TEXT("CreatedBy"), ProjectOrPluginInfo.CreatedBy);
	Writer->WriteValue(TEXT("CreatedByURL"), ProjectOrPluginInfo.CreatedByURL);
	
	if ( ProjectOrPluginInfo.Modules.Num() > 0 )
	{
		Writer->WriteArrayStart(TEXT("Modules"));
		for ( auto ModuleIt = ProjectOrPluginInfo.Modules.CreateConstIterator(); ModuleIt; ++ModuleIt )
		{
			const FProjectOrPluginInfo::FModuleInfo& Module = *ModuleIt;
			Writer->WriteObjectStart();
			Writer->WriteValue(TEXT("Name"), Module.Name.ToString());
			Writer->WriteValue(TEXT("Type"), FString(FProjectOrPluginInfo::EModuleType::ToString(Module.Type)));
			Writer->WriteValue(TEXT("LoadingPhase"), FString(ELoadingPhase::ToString(Module.LoadingPhase)));
			if (Module.WhitelistPlatforms.Num() > 0)
			{
				Writer->WriteArrayStart(TEXT("WhitelistPlatforms"));
				for ( auto PlatformIt = Module.WhitelistPlatforms.CreateConstIterator(); PlatformIt; ++PlatformIt )
				{
					Writer->WriteValue(*PlatformIt);
				}
				Writer->WriteArrayEnd();
			}
			if (Module.BlacklistPlatforms.Num() > 0)
			{
				Writer->WriteArrayStart(TEXT("BlacklistPlatforms"));
				for ( auto PlatformIt = Module.BlacklistPlatforms.CreateConstIterator(); PlatformIt; ++PlatformIt )
				{
					Writer->WriteValue(*PlatformIt);
				}
				Writer->WriteArrayEnd();
			}
			Writer->WriteObjectEnd();
		}
		Writer->WriteArrayEnd();
	}
	
	PerformAdditionalSerialization(Writer);
	
	Writer->WriteObjectEnd();
	Writer->Close();

	return JSONOutput;
}

bool FProjectOrPlugin::IsUpToDate( ) const
{
	const FProjectOrPluginInfo& ProjectOrPluginInfo = GetProjectOrPluginInfo();

	if ( ProjectOrPluginInfo.FileVersion < VER_LATEST_PROJECT_FILE
		|| ProjectOrPluginInfo.PackageFileUE4Version < GPackageFileUE4Version
		|| ProjectOrPluginInfo.EngineVersion.ToString() != GEngineVersion.ToString()
		|| ProjectOrPluginInfo.PackageFileLicenseeUE4Version < GPackageFileLicenseeUE4Version )
	{
		// This project file is out of date in at least one category
		return false;
	}
	else
	{
		return true;
	}
}

void FProjectOrPlugin::UpdateVersionToCurrent( )
{
	FProjectOrPluginInfo& ProjectOrPluginInfo = GetProjectOrPluginInfo();

	ProjectOrPluginInfo.FileVersion = VER_LATEST_PROJECT_FILE;
	ProjectOrPluginInfo.EngineVersion = GEngineVersion;
	ProjectOrPluginInfo.PackageFileUE4Version = GPackageFileUE4Version;
	ProjectOrPluginInfo.PackageFileLicenseeUE4Version = GPackageFileLicenseeUE4Version;
}

void FProjectOrPlugin::ReplaceModulesInProject(const TArray<FString>* StartupModuleNames)
{
	if ( StartupModuleNames )
	{
		FProjectOrPluginInfo& ProjectOrPluginInfo = GetProjectOrPluginInfo();

		ProjectOrPluginInfo.Modules.Empty();
		for ( auto ModuleIt = StartupModuleNames->CreateConstIterator(); ModuleIt; ++ModuleIt )
		{
			FProjectOrPluginInfo::FModuleInfo ModuleInfo;
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

bool FProjectOrPlugin::ReadModulesFromJSON(const TSharedRef< FJsonObject >& FileObject, TArray<FProjectOrPluginInfo::FModuleInfo>& OutModules, FText& OutFailReason ) const
{
	bool bLoadedSuccessfully = true;

	// This project or plugin might have some modules that we need to know about.  Let's take a look.
	if( FileObject->HasField( TEXT( "Modules" ) ) )
	{
		const TSharedPtr< FJsonValue >& ModulesValue = FileObject->GetField< EJson::Array >( TEXT( "Modules" ) );
		const TArray< TSharedPtr< FJsonValue > >& ModulesArray = ModulesValue->AsArray();
		for( auto ModuleValueIt( ModulesArray.CreateConstIterator() ); ModuleValueIt; ++ModuleValueIt )
		{
			const TSharedPtr< FJsonObject >& ModuleObjectPtr = ( *ModuleValueIt )->AsObject();
			if( ModuleObjectPtr.IsValid() )
			{
				const TSharedRef< FJsonObject >& ModuleObject = ModuleObjectPtr.ToSharedRef();
				FProjectOrPluginInfo::FModuleInfo ModuleInfo;

				// Module name
				{
					// All modules require a name to be set
					FString ModuleName;
					if ( ReadStringFromJSON(ModuleObject, TEXT("Name"), ModuleName) )
					{
						ModuleInfo.Name = FName(*ModuleName);
					}
					else
					{
						OutFailReason = LOCTEXT("ModuleWithoutAName", "Found a 'Module' entry with a missing 'Name' field");
						bLoadedSuccessfully = false;
						continue;
					}
				}


				// Module type
				{
					FString ModuleType;
					if ( ReadStringFromJSON(ModuleObject, TEXT("Type"), ModuleType) )
					{
						// Check to see if this is a valid type
						bool bFoundValidType = false;
						for( int32 PossibleTypeIndex = 0; PossibleTypeIndex < FProjectOrPluginInfo::EModuleType::Max; ++PossibleTypeIndex )
						{
							const FProjectOrPluginInfo::EModuleType::Type PossibleType = (FProjectOrPluginInfo::EModuleType::Type)PossibleTypeIndex;
							const TCHAR* PossibleTypeName = FProjectOrPluginInfo::EModuleType::ToString( PossibleType );

							if( FCString::Stricmp( PossibleTypeName, *ModuleType ) == 0 )
							{
								bFoundValidType = true;
								ModuleInfo.Type = PossibleType;
								break;
							}
						}

						if( !bFoundValidType )
						{
							OutFailReason = FText::Format( LOCTEXT( "ModuleWithInvalidType", "Module entry '{0}' specified an unrecognized module Type '{1}'" ), FText::FromName(ModuleInfo.Name), FText::FromString(ModuleType) );
							bLoadedSuccessfully = false;
							continue;
						}
					}
					else
					{
						OutFailReason = FText::Format( LOCTEXT( "ModuleWithoutAType", "Found Module entry '{0}' with a missing 'Type' field" ), FText::FromName(ModuleInfo.Name) );
						bLoadedSuccessfully = false;
						break;
					}
				}

				// Loading phase
				{
					FString ModuleLoadingPhase;
					if ( ReadStringFromJSON(ModuleObject, TEXT("LoadingPhase"), ModuleLoadingPhase) )
					{
						// Check to see if this is a valid Phase
						bool bFoundValidPhase = false;
						for( int32 PossiblePhaseIndex = 0; PossiblePhaseIndex < ELoadingPhase::Max; ++PossiblePhaseIndex )
						{
							const ELoadingPhase::Type PossiblePhase = (ELoadingPhase::Type)PossiblePhaseIndex;
							const TCHAR* PossiblePhaseName = ELoadingPhase::ToString( PossiblePhase );

							if( FCString::Stricmp( PossiblePhaseName, *ModuleLoadingPhase ) == 0 )
							{
								bFoundValidPhase = true;
								ModuleInfo.LoadingPhase = PossiblePhase;
								break;
							}
						}

						if( !bFoundValidPhase )
						{
							OutFailReason = FText::Format( LOCTEXT( "ModuleWithInvalidLoadingPhase", "Module entry '{0}' specified an unrecognized module LoadingPhase '{1}'" ), FText::FromName(ModuleInfo.Name), FText::FromString(ModuleLoadingPhase) );
							bLoadedSuccessfully = false;
							continue;
						}
					}
					else
					{
						// No 'LoadingPhase' was specified in the file.  That's totally fine, we'll use the default.
					}
				}

				// Whitelist platforms
				{
					if( ModuleObject->HasField( TEXT( "WhitelistPlatforms" ) ) )
					{
						// walk over the array values
						const TSharedPtr< FJsonValue >& PlatformsValue = ModuleObject->GetField< EJson::Array >( TEXT( "WhitelistPlatforms" ) );
						const TArray< TSharedPtr< FJsonValue > >& PlatformsArray = PlatformsValue->AsArray();
						for( auto PlatformValueIt( PlatformsArray.CreateConstIterator() ); PlatformValueIt; ++PlatformValueIt )
						{
							ModuleInfo.WhitelistPlatforms.Add((*PlatformValueIt)->AsString());
						}
					}
				}

				// Blacklist platforms
				{
					if( ModuleObject->HasField( TEXT( "BlacklistPlatforms" ) ) )
					{
						// walk over the array values
						const TSharedPtr< FJsonValue >& PlatformsValue = ModuleObject->GetField< EJson::Array >( TEXT( "BlacklistPlatforms" ) );
						const TArray< TSharedPtr< FJsonValue > >& PlatformsArray = PlatformsValue->AsArray();
						for( auto PlatformValueIt( PlatformsArray.CreateConstIterator() ); PlatformValueIt; ++PlatformValueIt )
						{
							ModuleInfo.BlacklistPlatforms.Add((*PlatformValueIt)->AsString());
						}
					}
				}

				OutModules.Add(ModuleInfo);
			}
			else
			{
				OutFailReason = LOCTEXT( "ModuleWithInvalidModulesArray", "The 'Modules' array has invalid contents and was not able to be loaded." );
				bLoadedSuccessfully = false;
			}
		}
	}
	
	return bLoadedSuccessfully;
}



ELoadingPhase::Type FProjectAndPluginManager::GetEarliestPhaseFromModules(const TArray<FProjectOrPluginInfo::FModuleInfo>& Modules)
{
	ELoadingPhase::Type EarliestPhase = ELoadingPhase::Default;

	for ( auto ModuleIt = Modules.CreateConstIterator(); ModuleIt; ++ModuleIt )
	{
		EarliestPhase = ELoadingPhase::GetEarlierPhase(EarliestPhase, (*ModuleIt).LoadingPhase);
	}

	return EarliestPhase;
}

#undef LOCTEXT_NAMESPACE
