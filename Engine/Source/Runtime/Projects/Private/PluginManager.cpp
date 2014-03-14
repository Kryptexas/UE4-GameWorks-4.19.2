// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ProjectsPrivatePCH.h"


DEFINE_LOG_CATEGORY_STATIC( LogPluginManager, Log, All );

#define LOCTEXT_NAMESPACE "PluginManager"


namespace PluginSystemDefs
{
	/** Latest supported version for plugin descriptor files.  We can still usually load older versions, but not newer version.
	    NOTE: This constant exists in UnrealBuildTool code as well. */
	static const int32 LatestPluginDescriptorFileVersion = 1;	// IMPORTANT: Remember to also update Plugins.cs (and Plugin system documentation) when this changes!
		
	/** File extension of plugin descriptor files.
	    NOTE: This constant exists in UnrealBuildTool code as well. */
	static const TCHAR PluginDescriptorFileExtension[] = TEXT( ".uplugin" );

	/** Relative path to the plugin's 128x128 icon resource file */
	static const FString RelativeIcon128FilePath( TEXT( "Resources/Icon128.png" ) );

}



FPluginManager::FPluginManager()
	: bEarliestPhaseProcessed(false)
	, bRestartRequired(false)
{
	DiscoverAllPlugins();
}



FPluginManager::~FPluginManager()
{
	// NOTE: All plugins and modules should be cleaned up or abandoned by this point

	// @todo plugin: Really, we should "reboot" module manager's unloading code so that it remembers at which startup phase
	//  modules were loaded in, so that we can shut groups of modules down (in reverse-load order) at the various counterpart
	//  shutdown phases.  This will fix issues where modules that are loaded after game modules are shutdown AFTER many engine
	//  systems are already killed (like GConfig.)  Currently the only workaround is to listen to global exit events, or to
	//  explicitly unload your module somewhere.  We should be able to handle most cases automatically though!
}

void FPluginManager::DiscoverAllPlugins()
{
	struct Local
	{

	private:
		/**
		 * Recursively searches for plugins and generates a list of plugin descriptors
		 *
		 * @param	PluginsDirectory	Directory we're currently searching
		 * @param	LoadedFrom			Where we're loading these plugins from (game, engine, etc)
		 * @param	Plugins				The array to be filled in with new plugins including descriptors
		 */
		static void FindPluginsRecursively( const FString& PluginsDirectory, const FPluginInfo::ELoadedFrom::Type LoadedFrom, TArray< TSharedRef<FPlugin> >& Plugins )
		{
			// NOTE: The logic in this function generally matches that of the C# code for FindPluginsRecursively
			//       in UnrealBuildTool.  These routines should be kept in sync.

			// This directory scanning needs to be fast because it will happen every time at startup!  We don't
			// want to blindly recurse down every subdirectory, because plugin content and code directories could
			// contain a lot of files in total!

			// Each sub-directory is possibly a plugin.  If we find that it contains a plugin, we won't recurse any
			// further -- you can't have plugins within plugins.  If we didn't find a plugin, we'll keep recursing.
			TArray<FString> PossiblePluginDirectories;
			{
				class FPluginDirectoryVisitor : public IPlatformFile::FDirectoryVisitor
				{
				public:
					FPluginDirectoryVisitor( TArray<FString>& InitFoundDirectories )
						: FoundDirectories( InitFoundDirectories )
					{
					}

					virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) OVERRIDE
					{
						if( bIsDirectory )
						{
							FoundDirectories.Add( FString( FilenameOrDirectory ) );
						}

						const bool bShouldContinue = true;
						return bShouldContinue;
					}

					TArray<FString>& FoundDirectories;
				};

				FPluginDirectoryVisitor PluginDirectoryVisitor( PossiblePluginDirectories );
				FPlatformFileManager::Get().GetPlatformFile().IterateDirectory( *PluginsDirectory, PluginDirectoryVisitor );
			}
				
			for( auto PossiblePluginDirectoryIter( PossiblePluginDirectories.CreateConstIterator() ); PossiblePluginDirectoryIter; ++PossiblePluginDirectoryIter )
			{
				const FString PossiblePluginDirectory = *PossiblePluginDirectoryIter;

				FString PluginDescriptorFilename;
				{
					// Usually the plugin descriptor is named the same as the directory.  It doesn't have to match the directory
					// name but we'll check that first because its much faster than scanning!
					const FString ProbablePluginDescriptorFilename = PossiblePluginDirectory / (FPaths::GetCleanFilename(PossiblePluginDirectory) + PluginSystemDefs::PluginDescriptorFileExtension);
					if( FPlatformFileManager::Get().GetPlatformFile().FileExists( *ProbablePluginDescriptorFilename ) )
					{
						PluginDescriptorFilename = ProbablePluginDescriptorFilename;
					}
					else
					{
						// Scan the directory for a plugin descriptor.
						TArray<FString> PluginDescriptorFilenames;
						{
							class FPluginDescriptorFileVisitor : public IPlatformFile::FDirectoryVisitor
							{
							public:
								FPluginDescriptorFileVisitor( const FWildcardString& InitSearchPattern, TArray<FString>& InitFoundPluginDescriptorFilenames )
									: SearchPattern( InitSearchPattern ),
									  FoundPluginDescriptorFilenames( InitFoundPluginDescriptorFilenames )
								{
								}

								virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) OVERRIDE
								{
									bool bShouldContinue = true;
									if( !bIsDirectory )
									{
										if( SearchPattern.IsMatch( FilenameOrDirectory ) )
										{
											FoundPluginDescriptorFilenames.Add( FString( FilenameOrDirectory ) );
											bShouldContinue = false;
										}
									}
									return bShouldContinue;
								}

								const FWildcardString& SearchPattern;
								TArray<FString>& FoundPluginDescriptorFilenames;
							};

							const FWildcardString PluginDescriptorSearchString = PossiblePluginDirectory / FString(TEXT("*")) + PluginSystemDefs::PluginDescriptorFileExtension;

							FPluginDescriptorFileVisitor PluginDescriptorFileVisitor( PluginDescriptorSearchString, PluginDescriptorFilenames );
							FPlatformFileManager::Get().GetPlatformFile().IterateDirectory( *PossiblePluginDirectory, PluginDescriptorFileVisitor );
						}

						// Did we find any plugin descriptor files?
						if( PluginDescriptorFilenames.Num() > 0 )
						{
							PluginDescriptorFilename = PluginDescriptorFilenames[ 0 ];;
						}
					}
				}

				if( !PluginDescriptorFilename.IsEmpty() )
				{
					// Found a plugin directory!  No need to recurse any further.

					// Load the descriptor file for this plugin into a new plugin info entry
					TSharedRef< FPlugin > NewPlugin = MakeShareable( new FPlugin() );

					FText FailureReason;
					const bool bLoadedSuccessfully = NewPlugin->LoadFromFile(PluginDescriptorFilename, FailureReason);
					if( bLoadedSuccessfully )
					{
						NewPlugin->SetLoadedFrom(LoadedFrom);

						if (FPaths::IsProjectFilePathSet())
						{
							FString GameProjectFolder = FPaths::GetPath(FPaths::GetProjectFilePath());
							if (PluginDescriptorFilename.StartsWith(GameProjectFolder))
							{
								FString PluginBinariesFolder = FPaths::Combine(*FPaths::GetPath(PluginDescriptorFilename), TEXT("Binaries"), FPlatformProcess::GetBinariesSubdirectory());
								FModuleManager::Get().AddBinariesDirectory(*PluginBinariesFolder, (LoadedFrom == FPluginInfo::ELoadedFrom::GameProject));
							}
						}

						Plugins.Add(NewPlugin);
					}
					else
					{
						// NOTE: Even though loading of this plugin failed, we'll keep processing other plugins
						UE_LOG(LogPluginManager, Error, TEXT("%s"), *FailureReason.ToString());
					}
				}
				else
				{
					// Didn't find a plugin in this directory.  Continue to look in subfolders.
					FindPluginsRecursively( PossiblePluginDirectory, LoadedFrom, Plugins );
				}
			}
		}


	public:

		/**
		 * Searches for plugins and generates a list of plugin descriptors
		 *
		 * @param	PluginsDirectory	The base directory to search for plugins
		 * @param	LoadedFrom			Where we're loading these plugins from (game, engine, etc)
		 * @param	Plugins				The array to be filled in with loaded plugin descriptors
		 */
		static void FindPluginsIn( const FString& PluginsDirectory, const FPluginInfo::ELoadedFrom::Type LoadedFrom, TArray< TSharedRef<FPlugin> >& Plugins )
		{
			// Make sure the directory even exists
			if( FPlatformFileManager::Get().GetPlatformFile().DirectoryExists( *PluginsDirectory ) )
			{
				FindPluginsRecursively( PluginsDirectory, LoadedFrom, Plugins );
			}
		}
	};


	{
		TArray< TSharedRef<FPlugin> > Plugins;

#if (WITH_ENGINE && !IS_PROGRAM) || WITH_PLUGIN_SUPPORT
		// Find "built-in" plugins.  That is, plugins situated right within the Engine directory.
		Local::FindPluginsIn( FPaths::EnginePluginsDir(), FPluginInfo::ELoadedFrom::Engine, Plugins );

		// Find plugins in the game project directory (<MyGameProject>/Plugins)
		if( FApp::HasGameName() )
		{
			Local::FindPluginsIn( FPaths::GamePluginsDir(), FPluginInfo::ELoadedFrom::GameProject, Plugins );
		}
#endif		// (WITH_ENGINE && !IS_PROGRAM) || WITH_PLUGIN_SUPPORT


		// Create plugin objects for all of the plugins that we found
		ensure( AllPlugins.Num() == 0 );		// Should not have already been initialized!
		AllPlugins = Plugins;
		for( auto PluginIt( Plugins.CreateConstIterator() ); PluginIt; ++PluginIt )
		{
			const TSharedRef<FPlugin>& Plugin = *PluginIt;
			const FPluginInfo& PluginInfo = Plugin->GetPluginInfo();

			// Add the plugin binaries directory
			const FString PluginBinariesPath = FPaths::Combine(*FPaths::GetPath(PluginInfo.LoadedFromFile), TEXT("Binaries"), FPlatformProcess::GetBinariesSubdirectory());
			FModuleManager::Get().AddBinariesDirectory(*PluginBinariesPath, PluginInfo.LoadedFrom == FPluginInfo::ELoadedFrom::GameProject);

			// Make sure to tell the module manager about any of our plugin's modules, so it will know where on disk to find them.
			for( auto ModuleInfoIt( PluginInfo.Modules.CreateConstIterator() ); ModuleInfoIt; ++ModuleInfoIt )
			{
				const FProjectOrPluginInfo::FModuleInfo& ModuleInfo = *ModuleInfoIt;

				// @todo plugin: We're adding all modules always here, even editor modules which might not exist at runtime. Hope that's OK.
				FModuleManager::Get().AddModule(ModuleInfo.Name);

				// Add to the module->plugin map
				ModulePluginMap.Add( ModuleInfo.Name, Plugin );
			}
		}
	}
}

void FPluginManager::EnablePluginsThatAreConfiguredToBeEnabled()
{
	TArray< FString > EnabledPluginNames;
	GetPluginsConfiguredToBeEnabled(EnabledPluginNames);

	TSet< FString > AllEnabledPlugins;
	AllEnabledPlugins.Append( EnabledPluginNames );

	for( TArray< TSharedRef< FPlugin > >::TConstIterator PluginIt( AllPlugins.CreateConstIterator() ); PluginIt; ++PluginIt )
	{
		const TSharedRef< FPlugin > Plugin = *PluginIt;
		if ( AllEnabledPlugins.Contains(Plugin->GetPluginName()) )
		{
			Plugin->SetEnabled(true);
		}
	}
}

void FPluginManager::RegisterEnabledPluginMountPoints()
{
	for( TArray< TSharedRef< FPlugin > >::TConstIterator PluginIt( AllPlugins.CreateConstIterator() ); PluginIt; ++PluginIt )
	{
		const TSharedRef< FPlugin > Plugin = *PluginIt;
		if ( Plugin->IsEnabled() )
		{
			Plugin->RegisterPluginMountPoints( RegisterMountPointDelegate );
		}
	}
}

void FPluginManager::LoadModulesForEnabledPlugins( const ELoadingPhase::Type LoadingPhase )
{
	if ( !bEarliestPhaseProcessed )
	{
		EnablePluginsThatAreConfiguredToBeEnabled();
		RegisterEnabledPluginMountPoints();
		bEarliestPhaseProcessed = true;
	}

	// Load plugins!
	for( auto PluginIt( AllPlugins.CreateConstIterator() ); PluginIt; ++PluginIt )
	{
		const TSharedRef< FPlugin > Plugin = *PluginIt;

		if ( Plugin->IsEnabled() )
		{
			TMap<FName, ELoadModuleFailureReason::Type> ModuleLoadFailures;
			Plugin->LoadModules( LoadingPhase, ModuleLoadFailures );

			FText FailureMessage;
			for( auto FailureIt( ModuleLoadFailures.CreateConstIterator() ); FailureIt; ++FailureIt )
			{
				const auto ModuleNameThatFailedToLoad = FailureIt.Key();
				const auto FailureReason = FailureIt.Value();

				if( FailureReason != ELoadModuleFailureReason::Success )
				{
					const FText PluginNameText = FText::FromString(Plugin->GetPluginName());
					const FText TextModuleName = FText::FromName(FailureIt.Key());

					if ( FailureReason == ELoadModuleFailureReason::FileNotFound )
					{
						FailureMessage = FText::Format( LOCTEXT("PluginModuleNotFound", "Plugin '{0}' failed to load because module '{1}' could not be found.  This plugin's functionality will not be available.  Please ensure the plugin is properly installed, otherwise consider disabling the plugin for this project."), PluginNameText, TextModuleName );
					}
					else if ( FailureReason == ELoadModuleFailureReason::FileIncompatible )
					{
						FailureMessage = FText::Format( LOCTEXT("PluginModuleIncompatible", "Plugin '{0}' failed to load because module '{1}' does not appear to be compatible with the current version of the engine.  This plugin's functionality will not be available.  The plugin may need to be recompiled."), PluginNameText, TextModuleName );
					}
					else if ( FailureReason == ELoadModuleFailureReason::CouldNotBeLoadedByOS )
					{
						FailureMessage = FText::Format( LOCTEXT("PluginModuleCouldntBeLoaded", "Plugin '{0}' failed to load because module '{1}' could not be loaded.  This plugin's functionality will not be available.  There may be an operating system error or the module may not be properly set up."), PluginNameText, TextModuleName );
					}
					else if ( FailureReason == ELoadModuleFailureReason::FailedToInitialize )
					{
						FailureMessage = FText::Format( LOCTEXT("PluginModuleFailedToInitialize", "Plugin '{0}' failed to load because module '{1}' could be initialized successfully after it was loaded.  This plugin's functionality will not be available."), PluginNameText, TextModuleName );
					}
					else 
					{
						ensure(0);	// If this goes off, the error handling code should be updated for the new enum values!
						FailureMessage = FText::Format( LOCTEXT("PluginGenericLoadFailure", "Plugin '{0}' failed to load because module '{1}' could not be loaded for an unspecified reason.  This plugin's functionality will not be available.  Please report this error."), PluginNameText, TextModuleName );
					}

					// Don't need to display more than one module load error per plugin that failed to load
					break;
				}
			}

			if( !FailureMessage.IsEmpty() )
			{
				FMessageDialog::Open(EAppMsgType::Ok, FailureMessage);
			}
		}
	}
}

void FPluginManager::SetRegisterMountPointDelegate( const FRegisterMountPointDelegate& Delegate )
{
	RegisterMountPointDelegate = Delegate;
}

bool FPluginManager::IsPluginModule( const FName ModuleName ) const
{
	return ( ModulePluginMap.Find( ModuleName ) != NULL );
}

void FPluginManager::GetPluginsConfiguredToBeEnabled(TArray<FString>& OutPluginNames) const
{
	GConfig->GetArray( TEXT("Plugins"), TEXT("EnabledPlugins"), OutPluginNames, GEngineIni );
}

void FPluginManager::ConfigurePluginToBeEnabled(const FString& PluginName, bool bEnabled)
{
	TArray< FString > EnabledPluginNames;
	GetPluginsConfiguredToBeEnabled(EnabledPluginNames);

	bool bSaveEnabledPluginNames = false;
	if ( bEnabled )
	{
		if ( !EnabledPluginNames.Contains(PluginName) )
		{
			EnabledPluginNames.Add(PluginName);
			bSaveEnabledPluginNames = true;
		}
	}
	else
	{
		int32 ExistingEnabledIdx = EnabledPluginNames.Find(PluginName);
		if ( ExistingEnabledIdx != INDEX_NONE )
		{
			EnabledPluginNames.RemoveAt(ExistingEnabledIdx);
			bSaveEnabledPluginNames = true;
		}
	}

	bRestartRequired = true;

	if ( bSaveEnabledPluginNames )
	{
		GConfig->SetArray( TEXT("Plugins"), TEXT("EnabledPlugins"), EnabledPluginNames, GEngineIni );
		GConfig->Flush(false);
	}
}


IPluginManager& IPluginManager::Get()
{
	// Single instance of manager, allocated on demand and destroyed on program exit.
	static FPluginManager* PluginManager = NULL;
	if( PluginManager == NULL )
	{
		PluginManager = new FPluginManager();
	}
	return *PluginManager;
}



FPlugin::FPlugin()
	: bPluginEnabled( false )
{
}

void FPlugin::RegisterPluginMountPoints( const IPluginManager::FRegisterMountPointDelegate& RegisterMountPointDelegate )
{
	// If this plugin has content, mount it before loading the first module it contains
	// @todo plugin: Should we do this even if there is no Content directory?  Probably, so that we can add content in Content Browser!
	if( PluginInfo.bCanContainContent )
	{
		if( ensure( RegisterMountPointDelegate.IsBound() ) )
		{
			// @todo plugin content: Consider prefixing path with "Plugins" or more specifically, either "EnginePlugins" or "GamePlugins", etc.
			// @todo plugin content: We may need the full relative path to the plugin under the respective Plugins directory if we want to fully avoid collisions
			const FString ContentRootPath( FString::Printf( TEXT( "/%s/" ), *PluginInfo.Name ) );
			const FString PluginContentDirectory( FPaths::GetPath(PluginInfo.LoadedFromFile) / TEXT( "Content" ) );

			RegisterMountPointDelegate.Execute( ContentRootPath, PluginContentDirectory );
		}
	}
}


const FString& FPlugin::GetPluginName() const
{
	return PluginInfo.Name;
}


void FPlugin::SetEnabled(bool bNewPluginEnabled)
{
	bPluginEnabled = bNewPluginEnabled;
}


bool FPlugin::IsEnabled() const
{
	return bPluginEnabled;
}

void FPlugin::SetLoadedFrom(FPluginInfo::ELoadedFrom::Type NewLoadedFrom)
{
	PluginInfo.LoadedFrom = NewLoadedFrom;
}

bool FPlugin::PerformAdditionalDeserialization(const TSharedRef< FJsonObject >& FileObject)
{
	ReadBoolFromJSON(FileObject, TEXT("CanContainContent"), PluginInfo.bCanContainContent);
	ReadBoolFromJSON(FileObject, TEXT("IsBetaVersion"), PluginInfo.bIsBetaVersion);

	return true;
}

void FPlugin::PerformAdditionalSerialization(const TSharedRef< TJsonWriter<> >& Writer) const
{
	Writer->WriteValue(TEXT("CanContainContent"), PluginInfo.bCanContainContent);
	Writer->WriteValue(TEXT("IsBetaVersion"), PluginInfo.bIsBetaVersion);
}


TArray< FPluginStatus > FPluginManager::QueryStatusForAllPlugins() const
{
	TArray< FPluginStatus > PluginStatuses;

	for( auto PluginIt( AllPlugins.CreateConstIterator() ); PluginIt; ++PluginIt )
	{
		const TSharedRef< FPlugin >& Plugin = *PluginIt;
		const auto& PluginInfo = Plugin->GetPluginInfo();
		
		FPluginStatus PluginStatus;
		PluginStatus.Name = PluginInfo.Name;
		PluginStatus.FriendlyName = PluginInfo.FriendlyName;
		PluginStatus.Version = PluginInfo.Version;
		PluginStatus.VersionName = PluginInfo.VersionName;
		PluginStatus.Description = PluginInfo.Description;
		PluginStatus.CreatedBy = PluginInfo.CreatedBy;
		PluginStatus.CreatedByURL = PluginInfo.CreatedByURL;
		PluginStatus.CategoryPath = PluginInfo.Category;
		PluginStatus.bIsEnabled = Plugin->IsEnabled();
		PluginStatus.bIsBuiltIn = ( PluginInfo.LoadedFrom == FPluginInfo::ELoadedFrom::Engine );
		PluginStatus.bIsBetaVersion = PluginInfo.bIsBetaVersion;

		// @todo plugedit: Maybe we should do the FileExists check ONCE at plugin load time and not at query time
		const FString Icon128FilePath = FPaths::GetPath(PluginInfo.LoadedFromFile) / PluginSystemDefs::RelativeIcon128FilePath;
		if( FPlatformFileManager::Get().GetPlatformFile().FileExists( *Icon128FilePath ) )
		{
			PluginStatus.Icon128FilePath = Icon128FilePath;
		}

		PluginStatuses.Add( PluginStatus );
	}

	return PluginStatuses;
}

void FPluginManager::SetPluginEnabled( const FString& PluginName, bool bEnabled )
{
	for( auto PluginIt( AllPlugins.CreateConstIterator() ); PluginIt; ++PluginIt )
	{
		const TSharedRef< FPlugin >& Plugin = *PluginIt;

		if ( Plugin->GetPluginName() == PluginName )
		{
			Plugin->SetEnabled(bEnabled);
			ConfigurePluginToBeEnabled(PluginName, bEnabled);
			break;
		}
	}
}

bool FPluginManager::IsRestartRequired() const
{
	return bRestartRequired;
}

#undef LOCTEXT_NAMESPACE