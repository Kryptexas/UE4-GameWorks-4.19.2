// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	CookOnTheFlyServer.cpp: handles polite cook requests via network ;)
=============================================================================*/

#include "UnrealEd.h"


#include "PackageHelperFunctions.h"
#include "DerivedDataCacheInterface.h"
#include "ISourceControlModule.h"
#include "GlobalShader.h"
#include "TargetPlatform.h"
#include "IConsoleManager.h"
#include "Developer/PackageDependencyInfo/Public/PackageDependencyInfo.h"
#include "IPlatformFileSandboxWrapper.h"
#include "Messaging.h"
#include "NetworkFileSystem.h"
#include "AssetRegistryModule.h"
#include "UnrealEdMessages.h"
#include "GameDelegates.h"




DEFINE_LOG_CATEGORY_STATIC(LogCookOnTheFly, Log, All);

#define DEBUG_COOKONTHEFLY 0


////////////////////////////////////////////////////////////////
/// Cook on the fly server
///////////////////////////////////////////////////////////////


/* Static functions
 *****************************************************************************/



static FString GetPackageFilename( UPackage* Package )
{
	FString Filename;
	if (FPackageName::DoesPackageExist(Package->GetName(), NULL, &Filename))
	{
		Filename = FPaths::ConvertRelativePathToFull(Filename);
	}
	return Filename;
}


/* helper structs
 *****************************************************************************/

/** Helper to pass a recompile request to game thread */
struct FRecompileRequest
{
	struct FShaderRecompileData RecompileData;
	bool bComplete;
};


/* UCookOnTheFlyServer structors
 *****************************************************************************/

UCookOnTheFlyServer::UCookOnTheFlyServer(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bCookOnTheFly = true;
}



void UCookOnTheFlyServer::Tick(float DeltaTime)
{
	uint32 CookedPackagesCount = 0;
	const static float CookOnTheSideTimeSlice = 0.01f;
	TickCookOnTheSide( CookOnTheSideTimeSlice, CookedPackagesCount);
	TickRecompileShaderRequests();
}

bool UCookOnTheFlyServer::IsTickable() const 
{ 
	return bIsAutoTick; 
}

TStatId UCookOnTheFlyServer::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCookServer, STATGROUP_Tickables);
}



bool UCookOnTheFlyServer::StartNetworkFileServer( bool BindAnyPort )
{
	//GetDerivedDataCacheRef().WaitForQuiescence(false);

	// start the listening thread
	FFileRequestDelegate FileRequestDelegate(FFileRequestDelegate::CreateUObject(this, &UCookOnTheFlyServer::HandleNetworkFileServerFileRequest));
	FRecompileShadersDelegate RecompileShadersDelegate(FRecompileShadersDelegate::CreateUObject(this, &UCookOnTheFlyServer::HandleNetworkFileServerRecompileShaders));

	NetworkFileServer = FModuleManager::LoadModuleChecked<INetworkFileSystemModule>("NetworkFileSystem")
		.CreateNetworkFileServer(BindAnyPort ? 0 : -1, &FileRequestDelegate, &RecompileShadersDelegate);


	// loop while waiting for requests
	GIsRequestingExit = false;
	return true;
}


bool UCookOnTheFlyServer::BroadcastFileserverPresence( const FGuid &InstanceId )
{
	TArray<TSharedPtr<FInternetAddr> > AddressList;

	if ((NetworkFileServer == NULL) || !NetworkFileServer->GetAddressList(AddressList))
	{
		UE_LOG(LogCookOnTheFly, Error, TEXT("Failed to create network file server"));

		return false;
	}

	// broadcast our presence
	if (InstanceId.IsValid())
	{
		TArray<FString> AddressStringList;

		for (int32 AddressIndex = 0; AddressIndex < AddressList.Num(); ++AddressIndex)
		{
			AddressStringList.Add(AddressList[AddressIndex]->ToString(true));
		}

		FMessageEndpointPtr MessageEndpoint = FMessageEndpoint::Builder("UCookOnTheFlyServer").Build();

		if (MessageEndpoint.IsValid())
		{
			MessageEndpoint->Publish(new FFileServerReady(AddressStringList, InstanceId), EMessageScope::Network);
		}		
	}
	return true;
}

uint32 UCookOnTheFlyServer::TickCookOnTheSide( const float TimeSlice, uint32 &CookedPackageCount )
{

	check( NetworkFileServer != NULL );


	double StartTime = FPlatformTime::Seconds();

	uint32 Result = 0;

	// This is all the target platforms which we needed to process requests for this iteration
	// we use this in the unsolicited packages processing below
	TArray<FName> AllTargetPlatformNames;

	while (!GIsRequestingExit)
	{
		bool bCookedPackage = false;
		FFilePlatformRequest ToBuild;

		bool bIsUnsolicitedRequest = false;
		if ( FileRequests.HasItems() )
		{
			FileRequests.Dequeue( &ToBuild );
		}
		else if ( UnsolicitedFileRequests.HasItems() )
		{
			bIsUnsolicitedRequest = true;
			UnsolicitedFileRequests.Dequeue( &ToBuild );
		}
		else
		{
			// no more to do this tick break out and do some other stuff
			break;
		}

		check( ToBuild.IsValid() );
		TArray<FName> TargetPlatformNames;
		
		if( ToBuild.Platformname != NAME_None )
		{
			TargetPlatformNames.AddUnique( ToBuild.Platformname );
		}

		bool bWasUpToDate = false;

		if (!ThreadSafeFilenameSet.Exists(ToBuild))
		{

			// if we have no target platforms then we want to cook because this will cook for all target platforms in that case
			bool bShouldCook = TargetPlatformNames.Num() > 0 ? false : ShouldCook( ToBuild.Filename, NAME_None );
			for ( int Index = 0; Index < TargetPlatformNames.Num(); ++Index )
			{
				bShouldCook |= ShouldCook( ToBuild.Filename, TargetPlatformNames[Index] );
			}

			
			if ( bShouldCook ) // if we should cook the package then cook it otherwise add it to the list of already cooked packages below
			{
				FString PackageName;	
				UPackage *Package = NULL;
				if ( FPackageName::TryConvertFilenameToLongPackageName(ToBuild.Filename, PackageName) )
				{
					Package = FindObject<UPackage>( ANY_PACKAGE, *PackageName );
				}

				UE_LOG( LogCookOnTheFly, Display, TEXT("Processing request %s"), *ToBuild.Filename)

				//  if the package is already loaded then try to avoid reloading it :)
				if ( ( Package == NULL ) || ( Package->IsFullyLoaded() == false ) )
				{
					Package = LoadPackage( NULL, *ToBuild.Filename, LOAD_None );
				}
				else
				{
					UE_LOG(LogCookOnTheFly, Display, TEXT("Package already loaded %s avoiding reload"), *ToBuild.Filename);
				}

				if( Package == NULL )
				{
					UE_LOG(LogCookOnTheFly, Error, TEXT("Error loading %s!"), *ToBuild.Filename );
					Result |= COSR_ErrorLoadingPackage;
				}
				else
				{
					if( SaveCookedPackage(Package, SAVE_KeepGUID | SAVE_Async, bWasUpToDate, TargetPlatformNames) )
					{
						FString Name = Package->GetPathName();
						FString PackageFilename(GetPackageFilename(Package));
						FPaths::MakeStandardFilename(PackageFilename);

						if ( PackageFilename != ToBuild.Filename )
						{
							// we have saved something which we didn't mean to load 
							//  sounds unpossible.... but it is due to searching for files and such
							//  mark the original request as processed (if this isn't actually the file they were requesting then it will fail)
							//	and then also save our new request as processed so we don't do it again
							ThreadSafeFilenameSet.Add( ToBuild );
							UE_LOG( LogCookOnTheFly, Display, TEXT("Request for %s received saved %s"), *ToBuild.Filename, *PackageFilename );
							ToBuild.Filename = PackageFilename;
						}
						// ToBuild.Filename = PackageFilename;

						// Update flags used to determine garbage collection.
						if (Package->ContainsMap())
						{
							Result |= COSR_CookedMap;
							//bCookedAMapSinceLastGC = true;
						}
						else
						{
							++CookedPackageCount;
							Result |= COSR_CookedPackage;
							//NonMapPackageCountSinceLastGC++;
							// LastCookActionTime = FPlatformTime::Seconds();
						}
						bCookedPackage = true;
					}

					//@todo ResetLoaders outside of this (ie when Package is NULL) causes problems w/ default materials
					if (Package->HasAnyFlags(RF_RootSet) == false)
					{
						ResetLoaders(Package);
					}
				}
			}

			for ( int Index = 0; Index < TargetPlatformNames.Num(); ++Index )
			{
				AllTargetPlatformNames.AddUnique(TargetPlatformNames[Index]);

				FFilePlatformRequest FileRequest( ToBuild.Filename, TargetPlatformNames[Index]);

				if ( bIsUnsolicitedRequest )
				{
					if ((FPaths::FileExists(FileRequest.Filename) == true) &&
						(bWasUpToDate == false))
					{
						UnsolicitedCookedPackages.EnqueueUnique( FileRequest );
						UE_LOG(LogCookOnTheFly, Display, TEXT("UnsolicitedCookedPackages: %s"), *FileRequest.Filename);
					}
				}

				ThreadSafeFilenameSet.Add( FileRequest );


			}
		}
		else
		{
			UE_LOG(LogCookOnTheFly, Display, TEXT("Package for platform already cooked %s, discarding request"), *ToBuild.Filename);
		}


		if ( bCookedPackage && !bIsUnsolicitedRequest)
		{
			TArray<UObject *> ObjectsInOuter;
			GetObjectsWithOuter(NULL, ObjectsInOuter, false);
			for( int32 Index = 0; Index < ObjectsInOuter.Num() && !GIsRequestingExit; Index++ )
			{
				UPackage* Package = Cast<UPackage>(ObjectsInOuter[Index]);

				if (Package)
				{
					FString Name = Package->GetPathName();
					FString PackageFilename(GetPackageFilename(Package));
					FPaths::MakeStandardFilename(PackageFilename);

					const FString FullPath = FPaths::ConvertRelativePathToFull(PackageFilename);

					bool AlreadyCooked = true;
					for ( const FName TargetPlatformName : AllTargetPlatformNames )
					{
						if ( !ThreadSafeFilenameSet.Exists(FFilePlatformRequest(PackageFilename, TargetPlatformName)) )
						{
							AlreadyCooked = false;
							break;
						}
					}
					if ( !AlreadyCooked )
					{
						for (const FName TargetPlatformName : AllTargetPlatformNames)
						{
							const FFilePlatformRequest Request( PackageFilename, TargetPlatformName );
							UnsolicitedFileRequests.EnqueueUnique(Request);
						}
					}
				}
			}
		}

		if ( (FPlatformTime::Seconds() - StartTime) > TimeSlice)
		{
			break;
		}
	}

	return Result;
}

void UCookOnTheFlyServer::OnObjectModified( UObject *ObjectMoving )
{
	OnObjectUpdated( ObjectMoving );
}

void UCookOnTheFlyServer::OnObjectPropertyChanged(UObject* ObjectBeingModified, FPropertyChangedEvent& PropertyChangedEvent)
{
	OnObjectUpdated( ObjectBeingModified );
}

void UCookOnTheFlyServer::OnObjectUpdated( UObject *Object )
{
	// get the outer of the object
	UPackage *Package = Object->GetOutermost();

	MarkPackageDirtyForCooker( Package );
}

void UCookOnTheFlyServer::MarkPackageDirtyForCooker( UPackage *Package )
{
	// force that package to be recooked
	const FString Name = Package->GetPathName();
	FString PackageFilename(GetPackageFilename(Package));
	FPaths::MakeStandardFilename(PackageFilename);
	UE_LOG(LogCookOnTheFly, Display, TEXT("Modification detected to package %s"), *PackageFilename);
	ThreadSafeFilenameSet.RemoveAll( PackageFilename );

	// TODO: go through package dependencies of this package, if a found dependency is not loaded then mark it as need recooking.
}


void UCookOnTheFlyServer::EndNetworkFileServer()
{
	if ( NetworkFileServer )
	{
		// shutdown the server
		NetworkFileServer->Shutdown();
		delete NetworkFileServer;
		NetworkFileServer = NULL;
	}
}

void UCookOnTheFlyServer::BeginDestroy()
{
	EndNetworkFileServer();

	Super::BeginDestroy();

}

void UCookOnTheFlyServer::TickRecompileShaderRequests()
{
	// try to pull off a request
	FRecompileRequest* Request = NULL;

	RecompileRequests.Dequeue(&Request);

	// process it
	if (Request)
	{
		HandleNetworkFileServerRecompileShaders(Request->RecompileData);

		// all done! other thread can unblock now
		Request->bComplete = true;
	}
}


FString UCookOnTheFlyServer::GetOutputDirectory( const FString& PlatformName ) const
{
	// Use SandboxFile to get the correct sandbox directory.
	FString OutputDirectory = SandboxFile->GetSandboxDirectory();

	return OutputDirectory.Replace(TEXT("[Platform]"), *PlatformName);
}


bool UCookOnTheFlyServer::GetPackageTimestamp( const FString& InFilename, FDateTime& OutDateTime )
{
	FPackageDependencyInfoModule& PDInfoModule = FModuleManager::LoadModuleChecked<FPackageDependencyInfoModule>("PackageDependencyInfo");
	FDateTime DependentTime;

	if (PDInfoModule.DeterminePackageDependentTimeStamp(*InFilename, DependentTime) == true)
	{
		OutDateTime = DependentTime;

		return true;
	}

	return false;
}


bool UCookOnTheFlyServer::SaveCookedPackage( UPackage* Package, uint32 SaveFlags, bool& bOutWasUpToDate ) 
{
	TArray<FName> TargetPlatformNames; 
	return SaveCookedPackage( Package, SaveFlags, bOutWasUpToDate, TargetPlatformNames );
}


bool UCookOnTheFlyServer::ShouldCook(const FString& InFileName, const FName &InPlatformName)
{
	bool bDoCook = false;


	FString PkgFile;
	FString PkgFilename;
	FDateTime DependentTimeStamp = FDateTime::MinValue();

	if (bIterativeCooking && FPackageName::DoesPackageExist(InFileName, NULL, &PkgFile))
	{
		PkgFilename = PkgFile;

		if (GetPackageTimestamp(FPaths::GetBaseFilename(PkgFilename, false), DependentTimeStamp) == false)
		{
			UE_LOG(LogCookOnTheFly, Display, TEXT("Failed to find dependency timestamp for: %s"), *PkgFilename);
		}
	}

	// Use SandboxFile to do path conversion to properly handle sandbox paths (outside of standard paths in particular).
	PkgFilename = SandboxFile->ConvertToAbsolutePathForExternalAppForWrite(*PkgFilename);

	ITargetPlatformManagerModule& TPM = GetTargetPlatformManagerRef();

	static const TArray<ITargetPlatform*> &ActiveTargetPlatforms = TPM.GetActiveTargetPlatforms();

	TArray<ITargetPlatform*> Platforms;

	if ( InPlatformName != NAME_None )
	{
		Platforms.Add( TPM.FindTargetPlatform( InPlatformName.ToString() ) );
	}
	else 
	{
		Platforms = ActiveTargetPlatforms;
	}

	for (int32 Index = 0; Index < Platforms.Num() && !bDoCook; Index++)
	{
		ITargetPlatform* Target = Platforms[Index];
		FString PlatFilename = PkgFilename.Replace(TEXT("[Platform]"), *Target->PlatformName());

		// If we are not iterative cooking, then cook the package
		bool bCookPackage = (bIterativeCooking == false);

		if (bCookPackage == false)
		{
			// If the cooked package doesn't exist, or if the cooked is older than the dependent, re-cook it
			FDateTime CookedTimeStamp = IFileManager::Get().GetTimeStamp(*PlatFilename);
			int32 CookedTimespanSeconds = (CookedTimeStamp - DependentTimeStamp).GetTotalSeconds();
			bCookPackage = (CookedTimeStamp == FDateTime::MinValue()) || (CookedTimespanSeconds < 0);
		}
		bDoCook |= bCookPackage;
	}

	return bDoCook;
}

bool UCookOnTheFlyServer::SaveCookedPackage( UPackage* Package, uint32 SaveFlags, bool& bOutWasUpToDate, TArray<FName> &TargetPlatformNames )
{


	bool bSavedCorrectly = true;

	FString Filename(GetPackageFilename(Package));

	if (Filename.Len())
	{
		FString PkgFilename;
		FDateTime DependentTimeStamp = FDateTime::MinValue();

		// We always want to use the dependent time stamp when saving a cooked package...
		// Iterative or not!
		FString PkgFile;
		FString Name = Package->GetPathName();

		if (bIterativeCooking && FPackageName::DoesPackageExist(Name, NULL, &PkgFile))
		{
			PkgFilename = PkgFile;

			if (GetPackageTimestamp(FPaths::GetBaseFilename(PkgFilename, false), DependentTimeStamp) == false)
			{
				UE_LOG(LogCookOnTheFly, Display, TEXT("Failed to find depedency timestamp for: %s"), *PkgFilename);
			}
		}

		// Use SandboxFile to do path conversion to properly handle sandbox paths (outside of standard paths in particular).
		Filename = SandboxFile->ConvertToAbsolutePathForExternalAppForWrite(*Filename);

		uint32 OriginalPackageFlags = Package->PackageFlags;
 		UWorld* World = NULL;
 		EObjectFlags Flags = RF_NoFlags;
		bool bPackageFullyLoaded = false;

		if (bCompressed)
		{
			Package->PackageFlags |= PKG_StoreCompressed;
		}

		ITargetPlatformManagerModule& TPM = GetTargetPlatformManagerRef();

		static TArray<ITargetPlatform*> ActiveStartupPlatforms = TPM.GetActiveTargetPlatforms();

		TArray<ITargetPlatform*> Platforms;

		if ( TargetPlatformNames.Num() )
		{
			const TArray<ITargetPlatform*>& TargetPlatforms = TPM.GetTargetPlatforms();

			for (const FName TargetPlatformFName : TargetPlatformNames)
			{
				const FString TargetPlatformName = TargetPlatformFName.ToString();

				for (ITargetPlatform *TargetPlatform  : TargetPlatforms)
				{
					if ( TargetPlatform->PlatformName() == TargetPlatformName )
					{
						Platforms.Add( TargetPlatform );
					}
				}
			}
		}
		else
		{
			Platforms = ActiveStartupPlatforms;

			for (ITargetPlatform *Platform : Platforms)
			{
				TargetPlatformNames.Add(FName(*Platform->PlatformName()));
			}
		}
		

		for (ITargetPlatform* Target : Platforms)
		{
			FString PlatFilename = Filename.Replace(TEXT("[Platform]"), *Target->PlatformName());

			// If we are not iterative cooking, then cook the package
			bool bCookPackage = (bIterativeCooking == false);

			if (bCookPackage == false)
			{
				// If the cooked package doesn't exist, or if the cooked is older than the dependent, re-cook it
				FDateTime CookedTimeStamp = IFileManager::Get().GetTimeStamp(*PlatFilename);
				int32 CookedTimespanSeconds = (CookedTimeStamp - DependentTimeStamp).GetTotalSeconds();
				bCookPackage = (CookedTimeStamp == FDateTime::MinValue()) || (CookedTimespanSeconds < 0);
			}

			// don't save Editor resources from the Engine if the target doesn't have editoronly data
			if (bSkipEditorContent && Name.StartsWith(TEXT("/Engine/Editor")) && !Target->HasEditorOnlyData())
			{
				bCookPackage = false;
			}

			if (bCookPackage == true)
			{
				if (bPackageFullyLoaded == false)
				{
					Package->FullyLoad();
					if (!Package->IsFullyLoaded())
					{
						UE_LOG(LogCookOnTheFly, Warning, TEXT("Package %s supposed to be fully loaded but isn't. RF_WasLoaded is %s"), 
							*Package->GetName(), Package->HasAnyFlags(RF_WasLoaded) ? TEXT("set") : TEXT("not set"));
					}
					bPackageFullyLoaded = true;

					// If fully loading has caused a blueprint to be regenerated, make sure we eliminate all meta data outside the package
					UMetaData* MetaData = Package->GetMetaData();
					MetaData->RemoveMetaDataOutsidePackage();

					// look for a world object in the package (if there is one, there's a map)
					World = UWorld::FindWorldInPackage(Package);
					Flags = World ? RF_NoFlags : RF_Standalone;
				}

				UE_LOG(LogCookOnTheFly, Display, TEXT("Cooking %s -> %s"), *Package->GetName(), *PlatFilename);

				bool bSwap = (!Target->IsLittleEndian()) ^ (!PLATFORM_LITTLE_ENDIAN);


				if (!Target->HasEditorOnlyData())
				{
					Package->PackageFlags |= PKG_FilterEditorOnly;
				}
				else
				{
					Package->PackageFlags &= ~PKG_FilterEditorOnly;
				}

				if (World)
				{
					World->PersistentLevel->OwningWorld = World;
				}

				const FString FullFilename = FPaths::ConvertRelativePathToFull( PlatFilename );
				if( FullFilename.Len() >= PLATFORM_MAX_FILEPATH_LENGTH )
				{
					UE_LOG( LogCookOnTheFly, Error, TEXT( "Couldn't save package, filename is too long :%s" ), *PlatFilename );
					bSavedCorrectly = false;
				}
				else
				{
					bSavedCorrectly &= GEditor->SavePackage( Package, World, Flags, *PlatFilename, GError, NULL, bSwap, false, SaveFlags, Target, FDateTime::MinValue() );
				}

				
				bOutWasUpToDate = false;
			}
			else
			{
				UE_LOG(LogCookOnTheFly, Display, TEXT("Up to date: %s"), *PlatFilename);

				bOutWasUpToDate = true;
			}
		}

		Package->PackageFlags = OriginalPackageFlags;
	}

	// return success
	return bSavedCorrectly;
}


void UCookOnTheFlyServer::MaybeMarkPackageAsAlreadyLoaded(UPackage *Package)
{
	FString Name = Package->GetName();
	if (PackagesToNotReload.Contains(Name))
	{
		UE_LOG(LogCookOnTheFly, Verbose, TEXT("Marking %s already loaded."), *Name);
		Package->PackageFlags |= PKG_ReloadingForCooker;
	}
}


void UCookOnTheFlyServer::Initialize( bool inCompressed, bool inIterativeCooking, bool inSkipEditorContent, bool inIsAutoTick, bool inAsyncSaveLoad, const FString &OutputDirectoryOverride )
{
	bCompressed = inCompressed;
	bIterativeCooking = inIterativeCooking;
	bSkipEditorContent = inSkipEditorContent;
	bAsyncSaveLoad = inAsyncSaveLoad;
	bIsAutoTick = inIsAutoTick;

	
	ITargetPlatformManagerModule& TPM = GetTargetPlatformManagerRef();
	const TArray<ITargetPlatform*>& Platforms = TPM.GetTargetPlatforms();

	// Local sandbox file wrapper. This will be used to handle path conversions,
	// but will not be used to actually write/read files so we can safely
	// use [Platform] token in the sandbox directory name and then replace it
	// with the actual platform name.
	SandboxFile = new FSandboxPlatformFile(false);

	// Output directory override.	
	FString OutputDirectory = GetOutputDirectoryOverride(OutputDirectoryOverride);

	// Use SandboxFile to do path conversion to properly handle sandbox paths (outside of standard paths in particular).
	SandboxFile->Initialize(&FPlatformFileManager::Get().GetPlatformFile(), *FString::Printf(TEXT("-sandbox=\"%s\""), *OutputDirectory));

	CleanSandbox(Platforms);

	// allow the game to fill out the asset registry, as well as get a list of objects to always cook
	TArray<FString> FilesInPath;
	FGameDelegates::Get().GetCookModificationDelegate().ExecuteIfBound(FilesInPath);

	// always generate the asset registry before starting to cook, for either method
	GenerateAssetRegistry(Platforms);

}

uint32 UCookOnTheFlyServer::NumConnections() const
{
	if ( NetworkFileServer )
	{
		return NetworkFileServer->NumConnections();
	}
	return 0;
}

FString UCookOnTheFlyServer::GetOutputDirectoryOverride( const FString &OutputDirectoryOverride ) const
{
	FString OutputDirectory = OutputDirectoryOverride;
	// Output directory override.	
	if (OutputDirectory.Len() <= 0)
	{
		// Full path so that the sandbox wrapper doesn't try to re-base it under Sandboxes
		OutputDirectory = FPaths::Combine(*FPaths::GameDir(), TEXT("Saved"), TEXT("Cooked"), TEXT("[Platform]"));
		OutputDirectory = FPaths::ConvertRelativePathToFull(OutputDirectory);
	}
	else if (!OutputDirectory.Contains(TEXT("[Platform]"), ESearchCase::IgnoreCase, ESearchDir::FromEnd) )
	{
		// Output directory needs to contain [Platform] token to be able to cook for multiple targets.
		OutputDirectory = FPaths::Combine(*OutputDirectory, TEXT("[Platform]"));
	}
	FPaths::NormalizeDirectoryName(OutputDirectory);

	return OutputDirectory;
}

void UCookOnTheFlyServer::CleanSandbox(const TArray<ITargetPlatform*>& Platforms)
{
	double SandboxCleanTime = 0.0;
	{
		SCOPE_SECONDS_COUNTER(SandboxCleanTime);

		if (bIterativeCooking == false)
		{
			// for now we are going to wipe the cooked directory
			for (int32 Index = 0; Index < Platforms.Num(); Index++)
			{
				ITargetPlatform* Target = Platforms[Index];
				FString SandboxDirectory = GetOutputDirectory(Target->PlatformName());
				IFileManager::Get().DeleteDirectory(*SandboxDirectory, false, true);
			}
		}
		else
		{
			FPackageDependencyInfoModule& PDInfoModule = FModuleManager::LoadModuleChecked<FPackageDependencyInfoModule>("PackageDependencyInfo");
			
			// list of directories to skip
			TArray<FString> DirectoriesToSkip;
			TArray<FString> DirectoriesToNotRecurse;

			// See what files are out of date in the sandbox folder
			for (int32 Index = 0; Index < Platforms.Num(); Index++)
			{
				ITargetPlatform* Target = Platforms[Index];
				FString SandboxDirectory = GetOutputDirectory(Target->PlatformName());

				// use the timestamp grabbing visitor
				IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
				FLocalTimestampDirectoryVisitor Visitor(PlatformFile, DirectoriesToSkip, DirectoriesToNotRecurse, false);
			
				PlatformFile.IterateDirectory(*SandboxDirectory, Visitor);

				for (TMap<FString, FDateTime>::TIterator TimestampIt(Visitor.FileTimes); TimestampIt; ++TimestampIt)
				{
					FString CookedFilename = TimestampIt.Key();
					FDateTime CookedTimestamp = TimestampIt.Value();
					FString StandardCookedFilename = CookedFilename.Replace(*SandboxDirectory, *(FPaths::GetRelativePathToRoot()));
					FDateTime DependentTimestamp;

					if (PDInfoModule.DeterminePackageDependentTimeStamp(*(FPaths::GetBaseFilename(StandardCookedFilename, false)), DependentTimestamp) == true)
					{
						double Diff = (CookedTimestamp - DependentTimestamp).GetTotalSeconds();

						if (Diff < 0.0)
						{
							UE_LOG(LogCookOnTheFly, Display, TEXT("Deleting out of date cooked file: %s"), *CookedFilename);

							IFileManager::Get().Delete(*CookedFilename);
						}
					}
				}
			}

			// Collect garbage to ensure we don't have any packages hanging around from dependent time stamp determination
			CollectGarbage(RF_Native);
		}
	}

	UE_LOG(LogCookOnTheFly, Display, TEXT("Sandbox cleanup took %5.3f seconds"), SandboxCleanTime);
}

void UCookOnTheFlyServer::GenerateAssetRegistry(const TArray<ITargetPlatform*>& Platforms)
{
	// load the interface
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	double GenerateAssetRegistryTime = 0.0;
	{
		SCOPE_SECONDS_COUNTER(GenerateAssetRegistryTime);
		UE_LOG(LogCookOnTheFly, Display, TEXT("Creating asset registry [is editor: %d]"), GIsEditor);

		// Perform a synchronous search of any .ini based asset paths (note that the per-game delegate may
		// have already scanned paths on its own)
		// We want the registry to be fully initialized when generating streaming manifests too.
		TArray<FString> ScanPaths;
		if (GConfig->GetArray(TEXT("AssetRegistry"), TEXT("PathsToScanForCook"), ScanPaths, GEngineIni) > 0)
		{
			AssetRegistry.ScanPathsSynchronous(ScanPaths);
		}
		else
		{
			AssetRegistry.SearchAllAssets(true);
		}
		
		// When not cooking on the fly the registry will be saved after the cooker has finished
		if (bCookOnTheFly)
		{
			// write it out to a memory archive
			FArrayWriter SerializedAssetRegistry;
			AssetRegistry.Serialize(SerializedAssetRegistry);
			UE_LOG(LogCookOnTheFly, Display, TEXT("Generated asset registry size is %5.2fkb"), (float)SerializedAssetRegistry.Num() / 1024.f);

			// now save it in each cooked directory
			FString RegistryFilename = FPaths::GameDir() / TEXT("AssetRegistry.bin");
			// Use SandboxFile to do path conversion to properly handle sandbox paths (outside of standard paths in particular).
			FString SandboxFilename = SandboxFile->ConvertToAbsolutePathForExternalAppForWrite(*RegistryFilename);

			for (int32 Index = 0; Index < Platforms.Num(); Index++)
			{
				FString PlatFilename = SandboxFilename.Replace(TEXT("[Platform]"), *Platforms[Index]->PlatformName());
				FFileHelper::SaveArrayToFile(SerializedAssetRegistry, *PlatFilename);
			}
		}
	}
	UE_LOG(LogCookOnTheFly, Display, TEXT("Done creating registry. It took %5.2fs."), GenerateAssetRegistryTime);
	
}

void UCookOnTheFlyServer::GenerateLongPackageNames(TArray<FString>& FilesInPath)
{
	TArray<FString> FilesInPathReverse;
	FilesInPathReverse.Reserve(FilesInPath.Num());
	for( int32 FileIndex = 0; FileIndex < FilesInPath.Num(); FileIndex++ )
	{
		const FString& FileInPath = FilesInPath[FilesInPath.Num() - FileIndex - 1];
		if (FPackageName::IsValidLongPackageName(FileInPath))
		{
			FilesInPathReverse.AddUnique(FileInPath);
		}
		else
		{
			FString LongPackageName;
			if (FPackageName::TryConvertFilenameToLongPackageName(FileInPath, LongPackageName))
			{
				FilesInPathReverse.AddUnique(LongPackageName);
			}
			else
			{
				UE_LOG(LogCookOnTheFly, Warning, TEXT("Unable to generate long package name for %s"), *FileInPath);
			}
		}
	}
	Exchange(FilesInPathReverse, FilesInPath);
}


/* UCookOnTheFlyServer callbacks
 *****************************************************************************/


void UCookOnTheFlyServer::HandleNetworkFileServerFileRequest( const FString& Filename, const FString &Platformname, TArray<FString>& UnsolicitedFiles )
{
	bool bIsCookable = FPackageName::IsPackageExtension(*FPaths::GetExtension(Filename, true));


	FName PlatformFname = FName( *Platformname );

	if (!bIsCookable)
	{
		TArray<FFilePlatformRequest> UnsolicitedRequests;
		UnsolicitedCookedPackages.DequeueAll(UnsolicitedRequests);
		for (const FFilePlatformRequest &Request : UnsolicitedRequests)
		{
			if ( Request.Platformname == PlatformFname )
			{
				FString StandardFilename = Request.Filename;
				FPaths::MakeStandardFilename( StandardFilename );
				UnsolicitedFiles.Add( StandardFilename );
			}
			else
			{
				UnsolicitedCookedPackages.Enqueue( Request );
			}
		}
		UPackage::WaitForAsyncFileWrites();
		return;
	}
	FString StandardFileName = Filename;
	FPaths::MakeStandardFilename( StandardFileName );
	FFilePlatformRequest FileRequest( StandardFileName, PlatformFname );
	FileRequests.Enqueue(FileRequest);

	do
	{
		FPlatformProcess::Sleep(0.0f);
	}
	while (!ThreadSafeFilenameSet.Exists(FileRequest));

	UE_LOG( LogCookOnTheFly, Display, TEXT("Cook complete %s"), *FileRequest.Filename)

	TArray<FFilePlatformRequest> UnsolicitedRequests;
	UnsolicitedCookedPackages.DequeueAll(UnsolicitedRequests);
	for (const FFilePlatformRequest &Request : UnsolicitedRequests)
	{
		if ( Request.Filename == FileRequest.Filename )
		{
			// don't do anything we don't want this guy
		}
		else if ( Request.Platformname == PlatformFname )
		{
			UE_LOG( LogCookOnTheFly, Display, TEXT("Returning unsolicited file %s"), *Request.Filename );
			UnsolicitedFiles.Add( Request.Filename );
		}
		else
		{
			UnsolicitedCookedPackages.Enqueue( Request );
		}
	}

	UPackage::WaitForAsyncFileWrites();


#if DEBUG_COOKONTHEFLY
	UE_LOG( LogCookOnTheFly, Display, TEXT("Processed file request %s"), *Filename );
#endif

}


void UCookOnTheFlyServer::HandleNetworkFileServerRecompileShaders(const FShaderRecompileData& RecompileData)
{
	// if we aren't in the game thread, we need to push this over to the game thread and wait for it to finish
	if (!IsInGameThread())
	{
		UE_LOG(LogCookOnTheFly, Display, TEXT("Got a recompile request on non-game thread"));

		// make a new request
		FRecompileRequest* Request = new FRecompileRequest;
		Request->RecompileData = RecompileData;
		Request->bComplete = false;

		// push the request for the game thread to process
		RecompileRequests.Enqueue(Request);

		// wait for it to complete (the game thread will pull it out of the TArray, but I will delete it)
		while (!Request->bComplete)
		{
			FPlatformProcess::Sleep(0);
		}
		delete Request;
		UE_LOG(LogCookOnTheFly, Display, TEXT("Completed recompile..."));

		// at this point, we are done on the game thread, and ModifiedFiles will have been filled out
		return;
	}

	FString OutputDir = GetOutputDirectory(RecompileData.PlatformName);

	RecompileShadersForRemote
		(RecompileData.PlatformName, 
		RecompileData.ShaderPlatform == -1 ? SP_NumPlatforms : (EShaderPlatform)RecompileData.ShaderPlatform,
		OutputDir, 
		RecompileData.MaterialsToLoad, 
		RecompileData.SerializedShaderResources, 
		RecompileData.MeshMaterialMaps, 
		RecompileData.ModifiedFiles);
}
