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

// cook by the book requirements
#include "Commandlets/ChunkManifestGenerator.h"
#include "Engine/WorldComposition.h"

// error message log
#include "TokenizedMessage.h"
#include "MessageLog.h"

// shader compiler processAsyncResults
#include "ShaderCompiler.h"

DEFINE_LOG_CATEGORY_STATIC(LogCookOnTheFly, Log, All);

#define DEBUG_COOKONTHEFLY 0
#define OUTPUT_TIMING 0

#if OUTPUT_TIMING

struct FTimerInfo
{
public:

	FTimerInfo( FTimerInfo &&InTimerInfo )
	{
		Swap( Name, InTimerInfo.Name );
		Length = InTimerInfo.Length;
	}

	FTimerInfo( const FTimerInfo &InTimerInfo )
	{
		Name = InTimerInfo.Name;
		Length = InTimerInfo.Length;
	}

	FTimerInfo( FString &&InName, double InLength ) : Name(MoveTemp(InName)), Length(InLength) { }

	FString Name;
	double Length;
};


static TArray<FTimerInfo> GTimerInfo;

struct FScopeTimer
{
private:
	bool Started;
	bool DecrementScope;
	static int GScopeDepth;
public:

	FScopeTimer( const FScopeTimer &outer )
	{
		Index = outer.Index;
		DecrementScope = false;
		Started = false;	
	}

	FScopeTimer( const FString &InName, bool IncrementScope = false )
	{
		DecrementScope = IncrementScope;
		
		FString Name = InName;
		for ( int i =0; i < GScopeDepth; ++i )
		{
			Name = FString(TEXT("  ")) + Name;
		}
		if( DecrementScope)
		{
			++GScopeDepth;
		}
		Index = GTimerInfo.Emplace(MoveTemp(Name), 0.0);
		Started = false;
	}

	void Start()
	{
		if ( !Started )
		{
			GTimerInfo[Index].Length -= FPlatformTime::Seconds();
			Started = true;
		}
	}

	void Stop()
	{
		if ( Started )
		{
			GTimerInfo[Index].Length += FPlatformTime::Seconds();
			Started = false;
		}
	}

	~FScopeTimer()
	{
		Stop();
		if ( DecrementScope )
		{
			--GScopeDepth;
		}
	}

	int Index;
};

int FScopeTimer::GScopeDepth = 0;



void OutputTimers()
{
	
	if ( GTimerInfo.Num() <= 0 )
		return;
	
	static FArchive *OutputDevice = NULL;

	static TMap<FString, int> TimerIndexMap;


	if ( OutputDevice == NULL )
	{
		OutputDevice = IFileManager::Get().CreateFileWriter(TEXT("CookOnTheFlyServerTiming.csv") );
	}

	TArray<FString> OutputValues;
	OutputValues.AddZeroed(TimerIndexMap.Num());

	bool OutputTimerIndexMap = false;
	for ( auto TimerInfo : GTimerInfo )
	{
		int *IndexPtr = TimerIndexMap.Find(TimerInfo.Name);
		int Index = 0;
		if (IndexPtr == NULL)
		{
			Index = TimerIndexMap.Num();
			TimerIndexMap.Add( TimerInfo.Name, Index );
			OutputValues.AddZeroed();
			OutputTimerIndexMap = true;
		}
		else
			Index = *IndexPtr;

		OutputValues[Index] = FString::Printf(TEXT("%f"),TimerInfo.Length);
	}
	static FString NewLine = FString(TEXT("\n"));

	if (OutputTimerIndexMap)
	{
		TArray<FString> OutputHeader;
		OutputHeader.AddZeroed( TimerIndexMap.Num() );
		for ( auto TimerIndex : TimerIndexMap )
		{
			int LocalIndex = TimerIndex.Value;
			OutputHeader[LocalIndex] = TimerIndex.Key;
		}

		for ( auto OutputString : OutputHeader)
		{
			OutputString.Append(TEXT(", "));
			OutputDevice->Serialize( (void*)(*OutputString), OutputString.Len() * sizeof(TCHAR));
		}

		
		OutputDevice->Serialize( (void*)*NewLine, NewLine.Len() * sizeof(TCHAR) );
	}

	for ( auto OutputString : OutputValues)
	{
		OutputString.Append(TEXT(", "));
		OutputDevice->Serialize( (void*)(*OutputString), OutputString.Len() * sizeof(TCHAR));
	}
	OutputDevice->Serialize( (void*)*NewLine, NewLine.Len() * sizeof(TCHAR) );

	OutputDevice->Flush();

	UE_LOG( LogCookOnTheFly, Display, TEXT("Timing information for cook") );
	UE_LOG( LogCookOnTheFly, Display, TEXT("Name\tlength(ms)") );
	for ( auto TimerInfo : GTimerInfo )
	{
		UE_LOG( LogCookOnTheFly, Display, TEXT("%s\t%.2f"), *TimerInfo.Name, TimerInfo.Length * 1000.0f );
	}

	GTimerInfo.Empty();
}


#define CREATE_TIMER(name, incrementScope) FScopeTimer ScopeTimer##name(#name, incrementScope); 

#define SCOPE_TIMER(name) CREATE_TIMER(name, true); ScopeTimer##name.Start();
#define STOP_TIMER( name ) ScopeTimer##name.Stop();


#define ACCUMULATE_TIMER(name) CREATE_TIMER(name, false);
#define ACCUMULATE_TIMER_SCOPE(name) FScopeTimer ScopeTimerInner##name(ScopeTimer##name); ScopeTimerInner##name.Start();
#define ACCUMULATE_TIMER_START(name) ScopeTimer##name.Start();
#define ACCUMULATE_TIMER_STOP(name) ScopeTimer##name.Stop();

#define OUTPUT_TIMERS() OutputTimers();

#else
#define CREATE_TIMER(name)

#define SCOPE_TIMER(name)
#define STOP_TIMER(name)

#define ACCUMULATE_TIMER(name) 
#define ACCUMULATE_TIMER_SCOPE(name) 
#define ACCUMULATE_TIMER_START(name) 
#define ACCUMULATE_TIMER_STOP(name) 

#define OUTPUT_TIMERS()

#endif
////////////////////////////////////////////////////////////////
/// Cook on the fly server
///////////////////////////////////////////////////////////////


/* Static functions
 *****************************************************************************/

struct FCachedPackageFilename
{
public:
	FCachedPackageFilename(FString &&InPackageFilename, FString &&InStandardFilename, FName InStandardFileFName ) :
		PackageFilename( MoveTemp( InPackageFilename )),
		StandardFilename(MoveTemp(InStandardFilename)),
		StandardFileFName( InStandardFileFName )
	{
	}

	FCachedPackageFilename( const FCachedPackageFilename &In )
	{
		PackageFilename = In.PackageFilename;
		StandardFilename = In.StandardFilename;
		StandardFileFName = In.StandardFileFName;
	}

	FCachedPackageFilename( FCachedPackageFilename &&In )
	{
		PackageFilename = MoveTemp(In.PackageFilename);
		StandardFilename = MoveTemp(In.StandardFilename);
		StandardFileFName = In.StandardFileFName;
	}

	FString PackageFilename; // this is also full path
	FString StandardFilename;
	FName StandardFileFName;
};

static TMap<FName, FCachedPackageFilename> PackageFilenameCache;

static const FCachedPackageFilename &Cache(const FName& PackageName)
{
	FCachedPackageFilename *Cached = PackageFilenameCache.Find( PackageName );
	if ( Cached != NULL )
	{
		return *Cached;
	}

	FString Filename;
	FString PackageFilename;
	FString StandardFilename;
	FName StandardFileFName = NAME_None;
	if (FPackageName::DoesPackageExist(PackageName.ToString(), NULL, &Filename))
	{
		StandardFilename = PackageFilename = FPaths::ConvertRelativePathToFull(Filename);
		

		FPaths::MakeStandardFilename(StandardFilename);
		StandardFileFName = FName(*StandardFilename);
	}

	return PackageFilenameCache.Emplace( PackageName, MoveTemp(FCachedPackageFilename(MoveTemp(PackageFilename),MoveTemp(StandardFilename), StandardFileFName)) );
}


/*static FString GetPackageFilename( UPackage* Package )
{
	FString Filename;
	if (FPackageName::DoesPackageExist(Package->GetName(), NULL, &Filename))
	{
		Filename = FPaths::ConvertRelativePathToFull(Filename);
	}

	return Filename;
}*/

static FString GetCachedPackageFilename( const FName& PackageName )
{
	return Cache( PackageName ).PackageFilename;
}

static FString GetCachedStandardPackageFilename( const FName& PackageName )
{
	return Cache( PackageName ).StandardFilename;
}

static FName GetCachedStandardPackageFileFName( const FName& PackageName )
{
	return Cache( PackageName ).StandardFileFName;
}


static FString GetCachedPackageFilename( UPackage* Package )
{
	check( Package->GetName() == Package->GetFName().ToString() );
	return Cache( Package->GetFName() ).PackageFilename;
}

static FString GetCachedStandardPackageFilename( UPackage* Package )
{
	check( Package->GetName() == Package->GetFName().ToString() );
	return Cache( Package->GetFName() ).StandardFilename;
}

static FName GetCachedStandardPackageFileFName( UPackage* Package )
{
	check( Package->GetName() == Package->GetFName().ToString() );
	return Cache( Package->GetFName() ).StandardFileFName;
}


static const FString& GetCachedSandboxFilename( UPackage* Package, TAutoPtr<class FSandboxPlatformFile>& SandboxFile )
{
	FName PackageFName = Package->GetFName();
	static TMap<FName, FString> CachedSandboxFilenames;
	FString* CachedSandboxFilename = CachedSandboxFilenames.Find(PackageFName);
	if ( CachedSandboxFilename )
		return *CachedSandboxFilename;

	const FString& PackageFilename = GetCachedPackageFilename(Package);
	FString SandboxFilename = SandboxFile->ConvertToAbsolutePathForExternalAppForWrite(*PackageFilename );

	return CachedSandboxFilenames.Add( PackageFName, MoveTemp(SandboxFilename) );
}

/* helper structs
 *****************************************************************************/

/** Helper to pass a recompile request to game thread */
struct FRecompileRequest
{
	struct FShaderRecompileData RecompileData;
	bool bComplete;
};



/**
 * Uses the FMessageLog to log a message
 * 
 * @param Message to log
 * @param Severity of the message
 */
void LogCookerMessage( const FString& MessageText, EMessageSeverity::Type Severity)
{
	FMessageLog MessageLog("CookResults");

	TSharedRef<FTokenizedMessage> Message = FTokenizedMessage::Create(Severity);

	Message->AddToken( FTextToken::Create( FText::FromString(MessageText) ) );
	// Message->AddToken(FTextToken::Create(MessageLogTextDetail)); 
	// Message->AddToken(FDocumentationToken::Create(TEXT("https://docs.unrealengine.com/latest/INT/Platforms/iOS/QuickStart/6/index.html"))); 
	MessageLog.AddMessage(Message);

	MessageLog.Notify();
}


/* UCookOnTheFlyServer structors
 *****************************************************************************/

UCookOnTheFlyServer::UCookOnTheFlyServer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	CurrentCookMode(ECookMode::CookOnTheFly),
	CookByTheBookOptions(NULL),
	CookFlags(ECookInitializationFlags::None),
	bIsSavingPackage( false )
{
}

UCookOnTheFlyServer::~UCookOnTheFlyServer()
{
	if ( CookByTheBookOptions )
	{		
		delete CookByTheBookOptions;
		CookByTheBookOptions = NULL;
	}
}

void UCookOnTheFlyServer::Tick(float DeltaTime)
{
	uint32 CookedPackagesCount = 0;
	const static float CookOnTheSideTimeSlice = 0.1f; // seconds
	TickCookOnTheSide( CookOnTheSideTimeSlice, CookedPackagesCount);
	TickRecompileShaderRequests();
}

bool UCookOnTheFlyServer::IsTickable() const 
{ 
	return IsCookFlagSet(ECookInitializationFlags::AutoTick); 
}

TStatId UCookOnTheFlyServer::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCookServer, STATGROUP_Tickables);
}

bool UCookOnTheFlyServer::StartNetworkFileServer( const bool BindAnyPort )
{
	check( CurrentCookMode == ECookMode::CookOnTheFly );
	//GetDerivedDataCacheRef().WaitForQuiescence(false);

	// start the listening thread
	FFileRequestDelegate FileRequestDelegate(FFileRequestDelegate::CreateUObject(this, &UCookOnTheFlyServer::HandleNetworkFileServerFileRequest));
	FRecompileShadersDelegate RecompileShadersDelegate(FRecompileShadersDelegate::CreateUObject(this, &UCookOnTheFlyServer::HandleNetworkFileServerRecompileShaders));

	INetworkFileServer *TcpFileServer = FModuleManager::LoadModuleChecked<INetworkFileSystemModule>("NetworkFileSystem")
		.CreateNetworkFileServer(true, BindAnyPort ? 0 : -1, &FileRequestDelegate, &RecompileShadersDelegate, ENetworkFileServerProtocol::NFSP_Tcp);
	if ( TcpFileServer )
	{
		NetworkFileServers.Add(TcpFileServer);
	}

	INetworkFileServer *HttpFileServer = FModuleManager::LoadModuleChecked<INetworkFileSystemModule>("NetworkFileSystem")
		.CreateNetworkFileServer(true, BindAnyPort ? 0 : -1, &FileRequestDelegate, &RecompileShadersDelegate, ENetworkFileServerProtocol::NFSP_Http);
	if ( HttpFileServer )
	{
		NetworkFileServers.Add( HttpFileServer );
	}
	// loop while waiting for requests
	GIsRequestingExit = false;
	return true;
}


bool UCookOnTheFlyServer::BroadcastFileserverPresence( const FGuid &InstanceId )
{
	
	TArray<FString> AddressStringList;

	for ( int i = 0; i < NetworkFileServers.Num(); ++i )
	{
		TArray<TSharedPtr<FInternetAddr> > AddressList;
		INetworkFileServer *NetworkFileServer = NetworkFileServers[i];
		if ((NetworkFileServer == NULL || !NetworkFileServer->IsItReadyToAcceptConnections() || !NetworkFileServer->GetAddressList(AddressList)))
		{
			LogCookerMessage( FString(TEXT("Failed to create network file server")), EMessageSeverity::Error );
			UE_LOG(LogCookOnTheFly, Error, TEXT("Failed to create network file server"));
			continue;
		}

		// broadcast our presence
		if (InstanceId.IsValid())
		{
			for (int32 AddressIndex = 0; AddressIndex < AddressList.Num(); ++AddressIndex)
			{
				AddressStringList.Add(FString::Printf( TEXT("%s://%s"), *NetworkFileServer->GetSupportedProtocol(),  *AddressList[AddressIndex]->ToString(true)));
			}

		}
	}



	FMessageEndpointPtr MessageEndpoint = FMessageEndpoint::Builder("UCookOnTheFlyServer").Build();

	if (MessageEndpoint.IsValid())
	{
		MessageEndpoint->Publish(new FFileServerReady(AddressStringList, InstanceId), EMessageScope::Network);
	}		
	
	return true;
}

/*----------------------------------------------------------------------------
	FArchiveFindReferences.
----------------------------------------------------------------------------*/
/**
 * Archive for gathering all the object references to other objects
 */
class FArchiveFindReferences : public FArchiveUObject
{
private:
	/**
	 * I/O function.  Called when an object reference is encountered.
	 *
	 * @param	Obj		a pointer to the object that was encountered
	 */
	FArchive& operator<<( UObject*& Obj )
	{
		if( Obj )
		{
			FoundObject( Obj );
		}
		return *this;
	}

	virtual FArchive& operator<< (class FAssetPtr& Value) override
	{
		if ( Value.Get() )
		{
			Value.Get()->Serialize( *this );
		}
		return *this;
	}
	virtual FArchive& operator<< (struct FStringAssetReference& Value) override
	{
		if ( Value.ResolveObject() )
		{
			Value.ResolveObject()->Serialize( *this );
		}
		return *this;
	}


	void FoundObject( UObject* Object )
	{
		if ( RootSet.Find(Object) == NULL )
		{
			if ( Exclude.Find(Object) == INDEX_NONE )
			{
				// remove this check later because don't want this happening in development builds
				check(RootSetArray.Find(Object)==INDEX_NONE);

				RootSetArray.Add( Object );
				RootSet.Add(Object);
				Found.Add(Object);
			}
		}
	}


	/**
	 * list of Outers to ignore;  any objects encountered that have one of
	 * these objects as an Outer will also be ignored
	 */
	TArray<UObject*> &Exclude;

	/** list of objects that have been found */
	TSet<UObject*> &Found;
	
	/** the objects to display references to */
	TArray<UObject*> RootSetArray;
	/** Reflection of the rootsetarray */
	TSet<UObject*> RootSet;

public:

	/**
	 * Constructor
	 * 
	 * @param	inOutputAr		archive to use for logging results
	 * @param	inOuter			only consider objects that do not have this object as its Outer
	 * @param	inSource		object to show references for
	 * @param	inExclude		list of objects that should be ignored if encountered while serializing SourceObject
	 */
	FArchiveFindReferences( TSet<UObject*> InRootSet, TSet<UObject*> &inFound, TArray<UObject*> &inExclude )
		: Exclude(inExclude)
		, Found(inFound)
		, RootSet(InRootSet)
	{
		ArIsObjectReferenceCollector = true;

		for ( const auto& Object : RootSet )
		{
			RootSetArray.Add( Object );
		}
		
		// loop through all the objects in the root set and serialize them
		for ( int RootIndex = 0; RootIndex < RootSetArray.Num(); ++RootIndex )
		{
			UObject* SourceObject = RootSetArray[RootIndex];

			// quick sanity check
			check(SourceObject);
			check(SourceObject->IsValidLowLevel());

			SourceObject->Serialize( *this );
		}

	}

	/**
  	 * Returns the name of the Archive.  Useful for getting the name of the package a struct or object
	 * is in when a loading error occurs.
	 *
	 * This is overridden for the specific Archive Types
	 **/
	virtual FString GetArchiveName() const { return TEXT("FArchiveFindReferences"); }
};

void UCookOnTheFlyServer::GetDependencies( const TSet<UPackage*>& Packages, TSet<UObject*>& Found)
{

	TSet<UObject*> RootSet;


	// Iterate through the object list
	for( FObjectIterator It; It; ++It )
	{
		UPackage* Package = It->GetOutermost();
		if ( Packages.Find( Package ) )
		{
			RootSet.Add(*It);
			Found.Add(*It);
		}

		/*if (It->IsIn(Package))
		{
		}*/
	}

	TArray<UObject*> Exclude;
	FArchiveFindReferences ArFindReferences( RootSet, Found, Exclude );




	// Iterate through the object list
	/*for( FObjectIterator It; It; ++It )
	{
		// if this object is within the package specified, serialize the object
		// into a specialized archive which logs object names encountered during
		// serialization -- rjp
		if ( It->IsIn(Pkg) )
		{
			if ( It->GetOuter() == Pkg )
			{
				FArchiveFindReferences ArFindReferences( Pkg, *It, Found, Exclude );
			}
			else if ( bRecurse )
			{
				// Two options -
				// a) this object is a function or something (which we don't care about)
				// b) this object is inside a group inside the specified package (which we do care about)
				UObject* CurrentObject = *It;
				UObject* CurrentOuter = It->GetOuter();
				while ( CurrentObject && CurrentOuter )
				{
					// this object is a UPackage (a group inside a package)
					// abort
					if ( CurrentObject->GetClass() == UPackage::StaticClass() )
						break;

					// see if this object's outer is a UPackage
					if ( CurrentOuter->GetClass() == UPackage::StaticClass() )
					{
						// if this object's outer is our original package, the original object (It)
						// wasn't inside a group, it just wasn't at the base level of the package
						// (its Outer wasn't the Pkg, it was something else e.g. a function, state, etc.)
						/// ....just skip it
						if ( CurrentOuter == Pkg )
							break;

						// otherwise, we've successfully found an object that was in the package we
						// were searching, but would have been hidden within a group - let's log it
						FArchiveFindReferences ArFindReferences( CurrentOuter, CurrentObject, Found, Exclude );
						break;
					}

					CurrentObject = CurrentOuter;
					CurrentOuter = CurrentObject->GetOuter();
				}
			}
		}
	}*/
}


void UCookOnTheFlyServer::GenerateManifestInfo( UPackage* Package, const TArray<FName>& TargetPlatformNames )
{
	if ( !CookByTheBookOptions )
		return;
	
	// generate dependency information for this package
	

	TSet<UPackage*> RootPackages;
	RootPackages.Add(Package);

	FString LastLoadedMapName;

	// load sublevels
	UWorld* World = UWorld::FindWorldInPackage(Package);

	if ( World )
	{
		for (const auto& StreamingLevel : World->StreamingLevels)
		{
			// Dependencies.Add( StreamingLevel->GetLoadedLevel() );
			RootPackages.Add(StreamingLevel->GetLoadedLevel()->GetOutermost() );
			//GetDependencies( StreamingLevel->GetLoadedLevel()->GetOutermost(), Dependencies );
		}

		TArray<FString> NewPackagesToCook;

		// Collect world composition tile packages to cook
		if (World->WorldComposition)
		{
			World->WorldComposition->CollectTilesToCook(NewPackagesToCook);
		}

		for ( const auto& PackageName : NewPackagesToCook)
		{
			UPackage* Package = LoadPackage( NULL, *PackageName, LOAD_None );

			RootPackages.Add( Package );
			//GetDependencies( Package, Dependencies );

			// Dependencies.Add(Package);
		}

		LastLoadedMapName = Package->GetName();
	}

	TSet<UObject*> Dependencies; 
	GetDependencies( RootPackages, Dependencies );


	FName StandardFilename = GetCachedStandardPackageFileFName(Package);

	TSet<UPackage*> Packages;
	for ( const auto& Object : Dependencies )
	{
		Packages.Add( Object->GetOutermost() );
	}

	// update the manifests with generated dependencies
	for ( const auto& PlatformName : TargetPlatformNames )
	{
		FChunkManifestGenerator* ManifestGenerator = CookByTheBookOptions->ManifestGenerators.FindChecked(PlatformName);

		if ( CookByTheBookOptions->bGenerateStreamingInstallManifests )
		{
			ManifestGenerator->PrepareToLoadNewPackage( StandardFilename.ToString() );
		}

		for ( const auto& DependentPackage : Packages )
		{
			ManifestGenerator->OnLastPackageLoaded( DependentPackage  );
		}
	
		for ( const auto& DependentPackage  : Packages )
		{
			FString Filename = GetCachedPackageFilename( DependentPackage );
			if (!Filename.IsEmpty())
			{
				// Populate streaming install manifests
				FString SandboxFilename = SandboxFile->ConvertToAbsolutePathForExternalAppForWrite(*Filename);
				//UE_LOG(LogCookOnTheFly, Display, TEXT("Adding package to manifest %s, %s, %s"), *DependentPackage->GetName(), *SandboxFilename, *LastLoadedMapName);
				ManifestGenerator->AddPackageToChunkManifest(DependentPackage, SandboxFilename, LastLoadedMapName, SandboxFile.GetOwnedPointer());
			}
		}
	}
}

bool UCookOnTheFlyServer::IsRealtimeMode()
{
	return CurrentCookMode == ECookMode::CookByTheBookFromTheEditor || CurrentCookMode == ECookMode::CookByTheBook;
}

uint32 UCookOnTheFlyServer::TickCookOnTheSide( const float TimeSlice, uint32 &CookedPackageCount )
{
	struct FCookerTimer
	{
		const bool bIsRealtimeMode;
		const double StartTime;
		const float &TimeSlice;
		FCookerTimer(const float &InTimeSlice, bool bInIsRealtimeMode ) : 
			bIsRealtimeMode( bInIsRealtimeMode), StartTime(FPlatformTime::Seconds()), TimeSlice(InTimeSlice)
		{
		}
		double GetTimeTillNow()
		{
			return FPlatformTime::Seconds() - StartTime;
		}
		bool IsTimeUp()
		{
			return (FPlatformTime::Seconds() - StartTime) > TimeSlice && bIsRealtimeMode;
		}
	};
	FCookerTimer Timer(TimeSlice, IsRealtimeMode());

	uint32 Result = 0;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	if ( AssetRegistry.IsLoadingAssets() )
	{
		// early out
		return Result;
	}

	// This is all the target platforms which we needed to process requests for this iteration
	// we use this in the unsolicited packages processing below
	TArray<FName> AllTargetPlatformNames;

	while (!GIsRequestingExit || CurrentCookMode == ECookMode::CookByTheBook)
	{
		// if we just cooked a map then don't process anything the rest of this tick
		if ( Result & COSR_CookedMap )
		{
			break;
		}

		if ( IsCookByTheBookMode() )
		{
			check( CookByTheBookOptions );
			if ( CookByTheBookOptions->bCancel )
			{
				CancelCookByTheBook();
			}
		}

		FFilePlatformRequest ToBuild;

		//bool bIsUnsolicitedRequest = false;
		if ( CookRequests.HasItems() )
		{
			CookRequests.Dequeue( &ToBuild );
		}
		else
		{
			// no more to do this tick break out and do some other stuff
			break;
		}

		if (CookedPackages.Exists(ToBuild))
		{
			UE_LOG(LogCookOnTheFly, Display, TEXT("Package for platform already cooked %s, discarding request"), *ToBuild.GetFilename().ToString());
			continue;
		}

		UE_LOG(LogCookOnTheFly, Display, TEXT("Processing package %s"), *ToBuild.GetFilename().ToString());

		SCOPE_TIMER(TickCookOnTheSide);

		check( ToBuild.IsValid() );
		const TArray<FName> &TargetPlatformNames = ToBuild.GetPlatformnames();

		TArray<UPackage*> PackagesToSave;

#if OUTPUT_TIMING
		//FScopeTimer PackageManualTimer( ToBuild.GetFilename().ToString(), false );
		UE_LOG(LogCookOnTheFly, Display,  TEXT("ProcessingPackage %s"), *ToBuild.GetFilename().ToString() );
#endif

		for ( const auto &PlatformName : TargetPlatformNames )
		{
			AllTargetPlatformNames.AddUnique(PlatformName);
		}

		bool bWasUpToDate = false;

		bool bLastLoadWasMap = false;
		FString LastLoadedMapName;

		const FString BuildFilename = ToBuild.GetFilename().ToString();

		// if we have no target platforms then we want to cook because this will cook for all target platforms in that case
		bool bShouldCook = TargetPlatformNames.Num() > 0 ? false : ShouldCook( BuildFilename, NAME_None );
		{
			SCOPE_TIMER(ShouldCook);
			for ( int Index = 0; Index < TargetPlatformNames.Num(); ++Index )
			{
				bShouldCook |= ShouldCook( ToBuild.GetFilename().ToString(), TargetPlatformNames[Index] );
			}
		}
		
		if ( bShouldCook ) // if we should cook the package then cook it otherwise add it to the list of already cooked packages below
		{
			SCOPE_TIMER(AllOfLoadPackage);

			FString PackageName;
			UPackage *Package = NULL;
			if ( FPackageName::TryConvertFilenameToLongPackageName(BuildFilename, PackageName) )
			{
				Package = FindObject<UPackage>( ANY_PACKAGE, *PackageName );
			}

			UE_LOG( LogCookOnTheFly, Display, TEXT("Processing request %s"), *BuildFilename);

			//  if the package is already loaded then try to avoid reloading it :)
			if ( ( Package == NULL ) || ( Package->IsFullyLoaded() == false ) )
			{
				// moved to GenerateDependencies 
				/*if (CookByTheBookOptions && CookByTheBookOptions->bGenerateStreamingInstallManifests)
				{
					CookByTheBookOptions->ManifestGenerator->PrepareToLoadNewPackage(BuildFilename);
				}*/

				SCOPE_TIMER(LoadPackage);
				Package = LoadPackage( NULL, *BuildFilename, LOAD_None );
			}
			else
			{
				UE_LOG(LogCookOnTheFly, Display, TEXT("Package already loaded %s avoiding reload"), *BuildFilename );
			}

			if( Package == NULL )
			{
				LogCookerMessage( FString::Printf(TEXT("Error loading %s!"), *BuildFilename), EMessageSeverity::Error );
				UE_LOG(LogCookOnTheFly, Error, TEXT("Error loading %s!"), *BuildFilename );
				Result |= COSR_ErrorLoadingPackage;
			}
			else
			{
				check(Package);


				if (Package->ContainsMap())
				{
					if ( IsCookByTheBookMode() )
					{
						// load sublevels
						UWorld* World = UWorld::FindWorldInPackage(Package);

						// TArray<FString> PreviouslyCookedPackages;
						if (World->StreamingLevels.Num())
						{
							//World->LoadSecondaryLevels(true, &PreviouslyCookedPackages);
							World->LoadSecondaryLevels(true, NULL);
						}

						TArray<FString> NewPackagesToCook;

						// Collect world composition tile packages to cook
						if (World->WorldComposition)
						{
							World->WorldComposition->CollectTilesToCook(NewPackagesToCook);
						}

						for ( auto PackageName : NewPackagesToCook )
						{
							FString Filename;
							if ( FPackageName::DoesPackageExist(PackageName, NULL, &Filename) )
							{
								FString StandardFilename = FPaths::ConvertRelativePathToFull(Filename);
								FPaths::MakeStandardFilename(StandardFilename);

								CookRequests.EnqueueUnique(FFilePlatformRequest(MoveTemp(FName(*StandardFilename)), MoveTemp(TargetPlatformNames)));
							}
						}

						// maps don't compile level script actors correctly unless we do FULL GC's, they may also hold weak pointer refs that need to be reset
						// NumProcessedSinceLastGC = GCInterval; 
						// ForceGarbageCollect();

						LastLoadedMapName = Package->GetName();
						bLastLoadWasMap = true;
					}

				}

				FString Name = Package->GetPathName();
				FString PackageFilename( GetCachedStandardPackageFilename( Package ) );
				if ( PackageFilename != BuildFilename )
				{
					// we have saved something which we didn't mean to load 
					//  sounds unpossible.... but it is due to searching for files and such
					//  mark the original request as processed (if this isn't actually the file they were requesting then it will fail)
					//	and then also save our new request as processed so we don't do it again
					PackagesToSave.AddUnique( Package );
					UE_LOG( LogCookOnTheFly, Display, TEXT("Request for %s received going to save %s"), *BuildFilename, *PackageFilename );
					// CookedPackages.Add( ToBuild );
					
					// ToBuild.SetFilename( PackageFilename );
				}
				else
				{
					PackagesToSave.AddUnique( Package );
				}
			}
		}

		
		if ( PackagesToSave.Num() == 0 )
		{
			// if we are iterative cooking the package might already be cooked
			// so just add the package to the cooked packages list
			// this could also happen if the source file doesn't exist which is often as we request files with different extensions when we are searching for files
			// just return that we processed the cook request
			// the network file manager will then handle the missing file and search somewhere else
			UE_LOG(LogCookOnTheFly, Display, TEXT("Not cooking package %s"), *ToBuild.GetFilename().ToString());
			CookedPackages.Add( FFilePlatformRequest( ToBuild.GetFilename(), TargetPlatformNames ) );
			continue;
		}


		
		// if we are doing cook by the book we need to resolve string asset references
		// this will load more packages :)
		if ( IsCookByTheBookMode() )
		{

			// if we are doing cook by the book then we are not waiting on this single package to be cooked
			// so we could just load some more packages and then save them all at once
			/*if ( bLastLoadWasMap && !bNeedsGC )
			{
			// early out and load some more packages to save
			continue;
			}*/



			SCOPE_TIMER(ResolveRedirectors);
			GRedirectCollector.ResolveStringAssetReference();
		}


		bool bIsAllDataCached = true;
		if ( PackagesToSave.Num() )
		{
			SCOPE_TIMER(CallBeginCacheForCookedPlatformData);
			// cache the resources for this package for each platform
			ITargetPlatformManagerModule& TPM = GetTargetPlatformManagerRef();
			TArray<UObject*> ObjectsInPackage;
			GetObjectsWithOuter( PackagesToSave[0], ObjectsInPackage );
			for ( const auto& TargetPlatformName : AllTargetPlatformNames )
			{
				const ITargetPlatform* TargetPlatform = TPM.FindTargetPlatform( TargetPlatformName.ToString() );
				
				for ( const auto& Obj : ObjectsInPackage )
				{
					Obj->BeginCacheForCookedPlatformData( TargetPlatform );
					if ( Obj->IsCachedCookedPlatformDataLoaded(TargetPlatform) == false )
					{
						UE_LOG(LogCookOnTheFly, Display, TEXT("Object %s isn't cached yet"), *Obj->GetFullName());
						bIsAllDataCached = false;
						break;
					}

					if ( Timer.IsTimeUp() && IsRealtimeMode() )
					{
						UE_LOG(LogCookOnTheFly, Display, TEXT("Object %s took too long to cache"), *Obj->GetFullName());
						bIsAllDataCached = false;
						break;
					}
				}

				if ( bIsAllDataCached == false )
				{
					break;
				}
			}
		}

		if ( IsRealtimeMode() )
		{
			if ( bIsAllDataCached == false )
			{
				// reque the current task and process it next tick
				CookRequests.EnqueueUnique(ToBuild, true);
				break;
			}
		}

		int32 FirstUnsolicitedPackage = PackagesToSave.Num();

		// generate a list of other packages which were loaded with this one
		{
			SCOPE_TIMER(UnsolicitedMarkup);

			TArray<UObject *> ObjectsInOuter;
			{
				SCOPE_TIMER(GetObjectsWithOuter);
				GetObjectsWithOuter(NULL, ObjectsInOuter, false);
			}

			TArray<FName> PackageNames;
			PackageNames.Empty(ObjectsInOuter.Num());
			{
				SCOPE_TIMER(GeneratePackageNames);
				ACCUMULATE_TIMER(UnsolicitedPackageAlreadyCooked);
				ACCUMULATE_TIMER(PackageCast);
				ACCUMULATE_TIMER(AddUnassignedPackageToManifest);
				ACCUMULATE_TIMER(GetCachedName);
				for( int32 Index = 0; Index < ObjectsInOuter.Num(); Index++ )
				{
					ACCUMULATE_TIMER_START(PackageCast);
					UPackage* Package = Cast<UPackage>(ObjectsInOuter[Index]);
					ACCUMULATE_TIMER_STOP(PackageCast);

					if (Package)
					{
						FName StandardPackageFName = GetCachedStandardPackageFileFName(Package);
						if ( StandardPackageFName == NAME_None )
							continue;

						ACCUMULATE_TIMER_START(UnsolicitedPackageAlreadyCooked);
						// package is already cooked don't care about processing it again here
						if ( CookRequests.Exists( StandardPackageFName, AllTargetPlatformNames) )
							continue;
						if ( CookedPackages.Exists(StandardPackageFName,AllTargetPlatformNames) )
							continue;
						ACCUMULATE_TIMER_STOP(UnsolicitedPackageAlreadyCooked);
						// this has been replaced to use the GenerateDependencies call
						/*if ( CookByTheBookOptions && CookByTheBookOptions->ManifestGenerator != NULL )
						{
							check( CurrentCookMode == ECookMode::CookByTheBookFromTheEditor || CurrentCookMode == ECookMode::CookByTheBook );
							// if we are generating a manifest add the package to the manifest :)
							FString Filename = GetCachedPackageFilename(Package);
							if (!Filename.IsEmpty())
							{
								// Populate streaming install manifests
								FString SandboxFilename = SandboxFile->ConvertToAbsolutePathForExternalAppForWrite(*Filename);
								CookByTheBookOptions->ManifestGenerator->AddPackageToChunkManifest(Package, SandboxFilename, LastLoadedMapName, SandboxFile.GetOwnedPointer());
							}
						}*/


						if ( CookByTheBookOptions )
						{
							ACCUMULATE_TIMER_START(AddUnassignedPackageToManifest);
							for ( const auto &TargetPlatform : AllTargetPlatformNames )
							{
								FChunkManifestGenerator*& Manifest = CookByTheBookOptions->ManifestGenerators.FindChecked(TargetPlatform);

								FString Filename = GetCachedPackageFilename(Package);
								if (!Filename.IsEmpty())
								{
									// Populate streaming install manifests
									// FString SandboxFilename = SandboxFile->ConvertToAbsolutePathForExternalAppForWrite(*Filename);
									const FString& SandboxFilename = GetCachedSandboxFilename( Package, SandboxFile );
									Manifest->AddUnassignedPackageToManifest(Package, SandboxFilename);
								}
							}
							ACCUMULATE_TIMER_STOP(AddUnassignedPackageToManifest);
						}

						ACCUMULATE_TIMER_START(GetCachedName);
						FName StandardFilename = GetCachedStandardPackageFileFName(Package);
						if ( StandardFilename != NAME_None ) // if we have name none that means we are in core packages or something...
						{
							// check if the package has already been saved
							PackagesToSave.AddUnique( Package );
						}
						ACCUMULATE_TIMER_STOP(GetCachedName);
					}
				}
			}
		}

		{
			SCOPE_TIMER(GenerateManifestInfo);
			// update manifest with cooked package info
			GenerateManifestInfo( PackagesToSave[0], AllTargetPlatformNames );
		}

		bool bFinishedSave = true;

		if ( PackagesToSave.Num() )
		{
			SCOPE_TIMER(SavingPackages);
			for ( int32 I = 0; I < PackagesToSave.Num(); ++I )
			{
				// if we are processing unsolicited packages we can optionally not save these right now
				// the unsolicited packages which we missed now will be picked up on next run
				if ( (CurrentCookMode == ECookMode::CookOnTheFly || CurrentCookMode == ECookMode::CookByTheBookFromTheEditor) && (I >= FirstUnsolicitedPackage) )
				{
					bool bShouldFinishTick = false;

					if ( CookRequests.HasItems() )
					{
						bShouldFinishTick = true;
					}

					if ( Timer.IsTimeUp() )
					{
						bShouldFinishTick = true;
						// our timeslice is up
					}


					if ( bShouldFinishTick )
					{
						SCOPE_TIMER(EnqueueUnsavedPackages);
						// don't save these packages because we have a real request
						// enqueue all the packages which we were about to save
						if (CurrentCookMode==ECookMode::CookByTheBookFromTheEditor)
						{
							for ( int32 RemainingIndex = I; RemainingIndex < PackagesToSave.Num(); ++RemainingIndex )
							{
								FName StandardFilename = GetCachedStandardPackageFileFName(PackagesToSave[RemainingIndex]);
								CookRequests.EnqueueUnique(FFilePlatformRequest(StandardFilename, AllTargetPlatformNames));
							}
						}

						// break out of the loop
						bFinishedSave = false;
						break;
					}
				}

				UPackage *Package = PackagesToSave[I];
				FName PackageFName = GetCachedStandardPackageFileFName(Package);
				TArray<FName> SaveTargetPlatformNames = AllTargetPlatformNames;
				TArray<FName> CookedTargetPlatforms;
				if ( CookedPackages.GetCookedPlatforms(PackageFName, CookedTargetPlatforms) )
				{
					for ( auto const &CookedPlatform : CookedTargetPlatforms )
					{
						SaveTargetPlatformNames.Remove( CookedPlatform );
					}
				}

				if ( SaveTargetPlatformNames.Num() == 0 )
				{
					// already saved this package
					continue;
				}

				SCOPE_TIMER(SaveCookedPackage);
				if( SaveCookedPackage(Package, SAVE_KeepGUID | SAVE_Async | (IsCookFlagSet(ECookInitializationFlags::Unversioned) ? SAVE_Unversioned : 0), bWasUpToDate, AllTargetPlatformNames ) )
				{
					// Update flags used to determine garbage collection.
					if (Package->ContainsMap())
					{
						Result |= COSR_CookedMap;
					}
					else
					{
						++CookedPackageCount;
						Result |= COSR_CookedPackage;
					}
				}


				/*{
					SCOPE_TIMER(GenerateManifestInfo);
					// update manifest with cooked package info
					GenerateManifestInfo( Package, AllTargetPlatformNames );
				}*/
				


				//@todo ResetLoaders outside of this (ie when Package is NULL) causes problems w/ default materials
				if (Package->HasAnyFlags(RF_RootSet) == false && ((CurrentCookMode==ECookMode::CookOnTheFly)) )
				{
					SCOPE_TIMER(ResetLoaders);
					ResetLoaders(Package);
				}

				FName StandardFilename = GetCachedStandardPackageFileFName(Package);

				if ( StandardFilename != NAME_None )
				{
					// mark the package as cooked
					FFilePlatformRequest FileRequest( StandardFilename, AllTargetPlatformNames);
					CookedPackages.Add( FileRequest );

					if ( (CurrentCookMode == ECookMode::CookOnTheFly) && (I >= FirstUnsolicitedPackage) ) 
					{
						// this is an unsolicited package
						if ((FPaths::FileExists(FileRequest.GetFilename().ToString()) == true) &&
							(bWasUpToDate == false))
						{
							UnsolicitedCookedPackages.AddCookedPackage( FileRequest );
							UE_LOG(LogCookOnTheFly, Display, TEXT("UnsolicitedCookedPackages: %s"), *FileRequest.GetFilename().ToString());
						}
					}
				}
			}
		}

		// if after all this our requested file didn't get saved then we need to mark it as saved (this can happen if the file loaded didn't match the one we saved)
		// for example say we request A.uasset which doesn't exist however A.umap exists, our load call will succeed as it will find A.umap, then the save will save A.umap
		// our original package request will fail however
		if ( !CookedPackages.Exists(ToBuild) )
		{
			CookedPackages.Add( ToBuild );
		}

		// TODO: Daniel: this is reference code needs to be reimplemented on the callee side.
		//  cooker can't depend on callee being able to garbage collect
		// collect garbage
		if ( CookByTheBookOptions && CookByTheBookOptions->bLeakTest && bFinishedSave )
		{
			check( CurrentCookMode == ECookMode::CookByTheBook);
			UE_LOG(LogCookOnTheFly, Display, TEXT("Full GC..."));

			CollectGarbage( RF_Native );

			for (FObjectIterator It; It; ++It)
			{
				if (!CookByTheBookOptions->LastGCItems.Contains(FWeakObjectPtr(*It)))
				{
					UE_LOG(LogCookOnTheFly, Warning, TEXT("\tLeaked %s"), *(It->GetFullName()));
					CookByTheBookOptions->LastGCItems.Add(FWeakObjectPtr(*It));
				}
			}
		}

		if ( Timer.IsTimeUp() )
		{
			break;
		}
	}

	OUTPUT_TIMERS();

	if ( CookByTheBookOptions )
	{
		CookByTheBookOptions->CookTime += Timer.GetTimeTillNow();
	}


	if ( IsCookByTheBookRunning() && 
		( CookRequests.HasItems() == false ) )
	{
		check(IsCookByTheBookMode());
		// if we are out of stuff and we are in cook by the book from the editor mode then we finish up
		CookByTheBookFinished();
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
	if ( !bIsSavingPackage )
	{
		// could have just cooked a file which we might need to write
		UPackage::WaitForAsyncFileWrites();

		// force that package to be recooked
		const FString Name = Package->GetPathName();
		/*FString PackageFilename(GetPackageFilename(Package));
		FPaths::MakeStandardFilename(PackageFilename);*/
		FString PackageFilename = GetCachedStandardPackageFilename(Package);
		// UE_LOG(LogCookOnTheFly, Display, TEXT("Modification detected to package %s"), *PackageFilename);
		const FName PackageFFileName = FName(*PackageFilename);

		if ( CurrentCookMode == ECookMode::CookByTheBookFromTheEditor )
		{
			TArray<FName> CookedPlatforms;
			if ( CookedPackages.GetCookedPlatforms(PackageFFileName, CookedPlatforms) )
			{
				// if this package was previously cooked and we are doing a cook by the book 
				// we need to recook this package before finishing cook by the book
				CookRequests.EnqueueUnique( FFilePlatformRequest(PackageFFileName, CookedPlatforms) );
			}
		}
		CookedPackages.RemoveAll( PackageFFileName );
	}
}


void UCookOnTheFlyServer::EndNetworkFileServer()
{
	for ( int i = 0; i < NetworkFileServers.Num(); ++i )
	{
		INetworkFileServer *NetworkFileServer = NetworkFileServers[i];
		// shutdown the server
		NetworkFileServer->Shutdown();
		delete NetworkFileServer;
		NetworkFileServer = NULL;
	}
	NetworkFileServers.Empty();
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

	if (IsCookFlagSet(ECookInitializationFlags::Iterative) && FPackageName::DoesPackageExist(InFileName, NULL, &PkgFile))
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

	static const TArray<ITargetPlatform*> &ActiveTargetPlatforms = TPM.GetCookingTargetPlatforms();

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
		bool bCookPackage = (IsCookFlagSet(ECookInitializationFlags::Iterative) == false);

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
	check( bIsSavingPackage == false );
	bIsSavingPackage = true;
	FString Filename(GetCachedPackageFilename(Package));

	if (Filename.Len())
	{
		FString PkgFilename;
		FDateTime DependentTimeStamp = FDateTime::MinValue();

		// We always want to use the dependent time stamp when saving a cooked package...
		// Iterative or not!
		FString PkgFile;
		FString Name = Package->GetPathName();

		if (IsCookFlagSet(ECookInitializationFlags::Iterative) && FPackageName::DoesPackageExist(Name, NULL, &PkgFile))
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

		if (IsCookFlagSet(ECookInitializationFlags::Compressed) )
		{
			Package->PackageFlags |= PKG_StoreCompressed;
		}

		ITargetPlatformManagerModule& TPM = GetTargetPlatformManagerRef();

		static TArray<ITargetPlatform*> ActiveStartupPlatforms = TPM.GetCookingTargetPlatforms();

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
			bool bCookPackage = (IsCookFlagSet(ECookInitializationFlags::Iterative) == false);

			if (bCookPackage == false)
			{
				// If the cooked package doesn't exist, or if the cooked is older than the dependent, re-cook it
				FDateTime CookedTimeStamp = IFileManager::Get().GetTimeStamp(*PlatFilename);
				int32 CookedTimespanSeconds = (CookedTimeStamp - DependentTimeStamp).GetTotalSeconds();
				bCookPackage = (CookedTimeStamp == FDateTime::MinValue()) || (CookedTimespanSeconds < 0);
			}

			// don't save Editor resources from the Engine if the target doesn't have editoronly data
			if (IsCookFlagSet(ECookInitializationFlags::SkipEditorContent) && Name.StartsWith(TEXT("/Engine/Editor")) && !Target->HasEditorOnlyData())
			{
				bCookPackage = false;
			}

			if (bCookPackage == true)
			{
				if (bPackageFullyLoaded == false)
				{
					SCOPE_TIMER(LoadPackage);

					Package->FullyLoad();
					if (!Package->IsFullyLoaded())
					{
						LogCookerMessage( FString::Printf(TEXT("Package %s supposed to be fully loaded but isn't. RF_WasLoaded is %s"), 
							*Package->GetName(), Package->HasAnyFlags(RF_WasLoaded) ? TEXT("set") : TEXT("not set")), EMessageSeverity::Warning);
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
					if ( !World->bIsWorldInitialized)
					{
						// we need to initialize the world - at least need physics scene since BP construction script runs during cooking, otherwise trace won't work
						World->InitWorld(UWorld::InitializationValues().RequiresHitProxies(false).ShouldSimulatePhysics(false).EnableTraceCollision(false).CreateNavigation(false).AllowAudioPlayback(false).CreatePhysicsScene(true));
					}
				}

				const FString FullFilename = FPaths::ConvertRelativePathToFull( PlatFilename );
				if( FullFilename.Len() >= PLATFORM_MAX_FILEPATH_LENGTH )
				{
					LogCookerMessage( FString::Printf(TEXT("Couldn't save package, filename is too long: %s"), *PlatFilename), EMessageSeverity::Error );
					UE_LOG( LogCookOnTheFly, Error, TEXT( "Couldn't save package, filename is too long :%s" ), *PlatFilename );
					bSavedCorrectly = false;
				}
				else
				{
					SCOPE_TIMER(GEditorSavePackage);
					bSavedCorrectly &= GEditor->SavePackage( Package, World, Flags, *PlatFilename, GError, NULL, bSwap, false, SaveFlags, Target, FDateTime::MinValue(), false );
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

	check( bIsSavingPackage == true );
	bIsSavingPackage = false;

	// return success
	return bSavedCorrectly;
}

void UCookOnTheFlyServer::Initialize( ECookMode::Type DesiredCookMode, ECookInitializationFlags InCookFlags, const FString &OutputDirectoryOverride )
{

	CurrentCookMode = DesiredCookMode;
	CookFlags = InCookFlags;
	
	if ( IsCookByTheBookMode() )
	{
		CookByTheBookOptions = new FCookByTheBookOptions();
		CookByTheBookOptions->bGenerateStreamingInstallManifests = IsCookFlagSet(ECookInitializationFlags::GenerateStreamingInstallManifest);
	}


	ITargetPlatformManagerModule& TPM = GetTargetPlatformManagerRef();
	const TArray<ITargetPlatform*>& Platforms = TPM.GetCookingTargetPlatforms();

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

	// generate version info for use by -iterate to figure out what files need recooking
	SaveIniVersionStrings(Platforms);

	// always generate the asset registry before starting to cook, for either method
	GenerateAssetRegistry(Platforms);

}

uint32 UCookOnTheFlyServer::NumConnections() const
{
	int Result= 0;
	for ( int i = 0; i < NetworkFileServers.Num(); ++i )
	{
		INetworkFileServer *NetworkFileServer = NetworkFileServers[i];
		if ( NetworkFileServer )
		{
			Result += NetworkFileServer->NumConnections();
		}
	}
	return Result;
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

template<class T>
void GetVersionFormatNumbersForIniVersionStrings( TArray<FString>& IniVersionStrings, const FString& FormatName, const TArray<T> &FormatArray )
{
	for ( const auto& Format : FormatArray )
	{
		TArray<FName> SupportedFormats;
		Format->GetSupportedFormats(SupportedFormats);
		for ( const auto& SupportedFormat : SupportedFormats )
		{
			int32 VersionNumber = Format->GetVersion(SupportedFormat);
			FString IniVersionString = FString::Printf( TEXT("%s:%s:VersionNumber%d"), *FormatName, *SupportedFormat.ToString(), VersionNumber);
			IniVersionStrings.Emplace( IniVersionString );
		}
	}
}

bool UCookOnTheFlyServer::GetCurrentIniVersionStrings( const ITargetPlatform* TargetPlatform, TArray<FString>& IniVersionStrings ) const
{
	// there is a list of important ini settings in the Editor config 
	TArray<FString> IniVersionedParams;
	GConfig->GetArray( TEXT("CookSettings"), TEXT("VersionedIniParams"), IniVersionedParams, GEditorIni );


	// used to store temporary platform specific ini files
	TMap<FString,FConfigFile*> PlatformIniFiles;

	// if the old one doesn't contain all the settings in the new one then we fail this check
	for ( const auto& IniVersioned : IniVersionedParams )
	{

		TArray<FString> IniVersionedArray;
		IniVersioned.ParseIntoArray(&IniVersionedArray, TEXT(":"), false);

		if ( IniVersionedArray.Num() != 3 )
		{
			// UE_LOG(LogCookOnTheFly, Warning, TEXT("Invalid entry in CookSettings, VersionedIniParams %s"), *IniVersioned);
			return false;
		}

		const FString& Filename = IniVersionedArray[0];
		const FString& Section = IniVersionedArray[1];
		const FString& Key = IniVersionedArray[2];

		// const FString& IniFilename = FPaths::GeneratedConfigDir() / TargetPlatform->IniPlatformName() / Filename + TEXT(".ini");
		FConfigFile *PlatformIniFile = PlatformIniFiles.FindRef( Filename );
		if ( PlatformIniFile == NULL )
		{
			PlatformIniFile = new FConfigFile();
			FConfigCacheIni::LoadLocalIniFile( *PlatformIniFile, *Filename, true, *TargetPlatform->IniPlatformName() );
			PlatformIniFiles.Add( Filename, PlatformIniFile );
		}

		// get the value of the entry
		FString Value;
		if ( !PlatformIniFile->GetString(*Section, *Key, Value) )
		{
			// UE_LOG(LogCookOnTheFly, Warning, TEXT("Unable to find entry in CookSettings, VersionedIniParams %s, assume default is being used"), *IniVersioned);
			continue;
		}

		FString CurrentVersionString = FString::Printf(TEXT("%s:%s:%s:%s"), *Filename, *Section, *Key, *Value );

		IniVersionStrings.Emplace( MoveTemp(CurrentVersionString) );
	}

	// clean up our temporary platform ini files
	for ( const auto& PlatformIniFile : PlatformIniFiles )
	{
		delete PlatformIniFile.Value;
	}
	PlatformIniFiles.Empty();


	const FTextureLODSettings& LodSettings = TargetPlatform->GetTextureLODSettings();

	UEnum* TextureGroupEnum = FindObject<UEnum>( NULL, TEXT("Engine.TextureGroup") );
	UEnum* TextureMipGenSettingsEnum = FindObject<UEnum>( NULL, TEXT("Engine.TextureMipGenSettings") );

	for ( int I = 0; I < TextureGroup::TEXTUREGROUP_MAX; ++I )
	{
		const TextureMipGenSettings& MipGenSettings = LodSettings.GetTextureMipGenSettings((TextureGroup)(I));
		FString MipGenVersionString = FString::Printf( TEXT("TextureLODGroupMipGenSettings:%s:%s"), *TextureGroupEnum->GetEnumName( I ), *TextureMipGenSettingsEnum->GetEnumName((int32)(MipGenSettings)) );
		IniVersionStrings.Emplace( MoveTemp( MipGenVersionString ) );

		const int32 MinMipCount = LodSettings.GetMinLODMipCount((TextureGroup)(I));
		FString MinMipVersionString = FString::Printf( TEXT("TextureLODGroupMinMipCount:%s:%d"), *TextureGroupEnum->GetEnumName( I ), MinMipCount);
		IniVersionStrings.Emplace( MoveTemp( MinMipVersionString ) );

		const int32 MaxMipCount = LodSettings.GetMaxLODMipCount((TextureGroup)(I));
		FString MaxMipVersionString = FString::Printf( TEXT("TextureLODGroupMaxMipCount:%s:%d"), *TextureGroupEnum->GetEnumName( I ), MaxMipCount);
		IniVersionStrings.Emplace( MoveTemp( MaxMipVersionString ) );
	}

	// save off the ddc version numbers also
	ITargetPlatformManagerModule* TPM = GetTargetPlatformManager();
	check(TPM);

	GetVersionFormatNumbersForIniVersionStrings( IniVersionStrings, TEXT("AudioFormat"), TPM->GetAudioFormats() );
	GetVersionFormatNumbersForIniVersionStrings( IniVersionStrings, TEXT("TextureFormat"), TPM->GetTextureFormats() );
	GetVersionFormatNumbersForIniVersionStrings( IniVersionStrings, TEXT("ShaderFormat"), TPM->GetShaderFormats() );
	/*
	for ( const auto& AudioFormat : TPM->GetAudioFormats() )
	{
		TArray<FName> SupportedFormats;
		AudioFormat.GetSupportedFormats(SupportedFormats);
		for ( const auto& SupportedFormat : SupportedFormats )
		{
			int32 VersionNumber = AudioFormat.GetVersion(SupportedFormat);
			FString AudioString = FString::Printf( TEXT("AudioFormat:%s:VersionNumber%d"), SupportedFormat.ToString(), VersionNumber);
			IniVersionStrings.Emplace( AudioString );
		}
	}

	for ( const auto& TextureFormat : TPM->GetTextureFormats() )
	{
		TArray<FName> SupportedFormats;
		TextureFormat.GetSupportedFormats(SupportedFormats);
		for ( const auto& SupportedFormat : SupportedFormats )
		{
			int32 VersionNumber = AudioFormat.GetVersion(SupportedFormat);
			FString AudioString = FString::Printf( TEXT("AudioFormat:%s:VersionNumber%d"), SupportedFormat.ToString(), VersionNumber);
			IniVersionStrings.Emplace( AudioString );
		}
	}

	for ( const auto& AudioFormat : TPM->GetAudioFormats() )
	{
		TArray<FName> SupportedFormats;
		AudioFormat.GetSupportedFormats(SupportedFormats);
		for ( const auto& SupportedFormat : SupportedFormats )
		{
			int32 VersionNumber = AudioFormat.GetVersion(SupportedFormat);
			FString AudioString = FString::Printf( TEXT("AudioFormat:%s:VersionNumber%d"), SupportedFormat.ToString(), VersionNumber);
			IniVersionStrings.Emplace( AudioString );
		}
	}
	*/


	return true;
}

bool UCookOnTheFlyServer::GetCookedIniVersionStrings( const ITargetPlatform* TargetPlatform, TArray<FString>& IniVersionStrings ) const
{

	const FString EditorIni = FPaths::GameDir() / TEXT("CookedIniVersion.txt");
	const FString SandboxEditorIni = SandboxFile->ConvertToAbsolutePathForExternalAppForWrite(*EditorIni);


	const FString PlatformSandboxEditorIni = SandboxEditorIni.Replace(TEXT("[Platform]"), *TargetPlatform->PlatformName());

	TArray<FString> SavedIniVersionedParams;
	return GConfig->GetArray( TEXT("CookSettings"), TEXT("VersionedIniParams"), IniVersionStrings, PlatformSandboxEditorIni ) > 0;
}


bool UCookOnTheFlyServer::IniSettingsOutOfDate( const TArray<ITargetPlatform*>& TargetPlatforms, TArray<ITargetPlatform*>& OutOfDateTargetPlatforms ) const
{


	for ( const auto& TargetPlatform : TargetPlatforms )
	{
		TArray<FString> CurrentIniVersionStrings;
		if ( GetCurrentIniVersionStrings(TargetPlatform, CurrentIniVersionStrings) == false )
		{
			// previous cook seems half baked... bomb out and recook all the things
			OutOfDateTargetPlatforms.Add( TargetPlatform);
			continue;
		}

		TArray<FString> CookedIniVersionStrings;
		if ( GetCookedIniVersionStrings( TargetPlatform, CookedIniVersionStrings ) == false )
		{
			// can't even get the cooked version strings for this platform add it to the out of date list
			OutOfDateTargetPlatforms.Add(TargetPlatform);
			continue;
		}

		for ( const auto& CurrentVersionString : CurrentIniVersionStrings ) 
		{
			if ( CookedIniVersionStrings.Contains(CurrentVersionString) == false )
			{
				OutOfDateTargetPlatforms.Add(TargetPlatform);
				break;
			}
		}
	}

	return true;
}

void UCookOnTheFlyServer::SaveIniVersionStrings( const TArray<ITargetPlatform*>& TargetPlatforms ) const
{
	const FString CookedIni = FPaths::GameDir() / TEXT("CookedIniVersion.txt");
	const FString SandboxCookedIni = SandboxFile->ConvertToAbsolutePathForExternalAppForWrite(*CookedIni);


	for ( const auto& TargetPlatform : TargetPlatforms )
	{
		// there is a list of important ini settings in the Editor config 
		TArray<FString> IniVersionStrings;
		GetCurrentIniVersionStrings(TargetPlatform, IniVersionStrings);

		const FString PlatformSandboxCookedIni = SandboxCookedIni.Replace(TEXT("[Platform]"), *TargetPlatform->PlatformName());

		GConfig->SetArray(TEXT("CookSettings"), TEXT("VersionedIniParams"), IniVersionStrings, PlatformSandboxCookedIni);

		GConfig->Flush(false, PlatformSandboxCookedIni);
	}
}

void UCookOnTheFlyServer::CleanSandbox(const TArray<ITargetPlatform*>& Platforms)
{
	double SandboxCleanTime = 0.0;
	{
		SCOPE_SECONDS_COUNTER(SandboxCleanTime);

		if (IsCookFlagSet(ECookInitializationFlags::Iterative) == false)
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
			TArray<ITargetPlatform*> OutOfDatePlatforms;
			IniSettingsOutOfDate(Platforms, OutOfDatePlatforms);
			// wipe entire directory for any platforms which are out of date
			for ( const auto& Target : OutOfDatePlatforms )
			{
				FString SandboxDirectory = GetOutputDirectory(Target->PlatformName());
				IFileManager::Get().DeleteDirectory(*SandboxDirectory, false, true);
			}

			// check each package to see if it's out of date
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
		bool bEditor = IsRealtimeMode();

		// editor will scan asset registry automagically 
		if ( !bEditor )
		{
			TArray<FString> ScanPaths;
			if (GConfig->GetArray(TEXT("AssetRegistry"), TEXT("PathsToScanForCook"), ScanPaths, GEngineIni) > 0)
			{
				AssetRegistry.ScanPathsSynchronous(ScanPaths);
			}
			else
			{
				AssetRegistry.SearchAllAssets(true);
			}
		}

		// When not cooking on the fly the registry will be saved after the cooker has finished
		if (CurrentCookMode == ECookMode::CookOnTheFly)
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
				LogCookerMessage( FString::Printf(TEXT("Unable to generate long package name for %s"), *FileInPath), EMessageSeverity::Warning);
				UE_LOG(LogCookOnTheFly, Warning, TEXT("Unable to generate long package name for %s"), *FileInPath);
			}
		}
	}
	Exchange(FilesInPathReverse, FilesInPath);
}

void UCookOnTheFlyServer::AddFileToCook( TArray<FString>& InOutFilesToCook, const FString &InFilename ) const
{ 
	if (!FPackageName::IsScriptPackage(InFilename))
	{
		InOutFilesToCook.AddUnique(InFilename);
	}
}

void UCookOnTheFlyServer::CollectFilesToCook(TArray<FString>& FilesInPath, const TArray<FString> &CookMaps, const TArray<FString> &InCookDirectories, const TArray<FString> &CookCultures, const TArray<FString> &IniMapSections, bool bCookAll, bool bMapsOnly, bool bNoDev)
{
	TArray<FString> MapList;
	TArray<FString> CookDirectories = InCookDirectories;
	// Add the default map section
	GEditor->LoadMapListFromIni(TEXT("AlwaysCookMaps"), MapList);

	for ( const auto &IniMapSection : IniMapSections )
	{
		GEditor->LoadMapListFromIni(*IniMapSection, MapList);
	}

	// Add any map sections specified on command line
	/*GEditor->ParseMapSectionIni(*Params, MapList);
	for (int32 MapIdx = 0; MapIdx < MapList.Num(); MapIdx++)
	{
		AddFileToCook( FilesInPath, MapList[MapIdx]);
	}*/


	// Also append any cookdirs from the project ini files; these dirs are relative to the game content directory
	{
		const FString AbsoluteGameContentDir = FPaths::ConvertRelativePathToFull(FPaths::GameContentDir());
		const UProjectPackagingSettings* const PackagingSettings = GetDefault<UProjectPackagingSettings>();
		for(const auto& DirToCook : PackagingSettings->DirectoriesToAlwaysCook)
		{
			CookDirectories.Add(AbsoluteGameContentDir / DirToCook.Path);
		}
	}

	for ( const auto &CurrEntry : CookMaps )
	{
		if (FPackageName::IsShortPackageName(CurrEntry))
		{
			FString OutFilename;
			if (FPackageName::SearchForPackageOnDisk(CurrEntry, NULL, &OutFilename) == false)
			{
				LogCookerMessage( FString::Printf(TEXT("Unable to find package for map %s."), *CurrEntry), EMessageSeverity::Warning);
				UE_LOG(LogCookOnTheFly, Warning, TEXT("Unable to find package for map %s."), *CurrEntry);
			}
			else
			{
				AddFileToCook( FilesInPath, OutFilename);
			}
		}
		else
		{
			AddFileToCook( FilesInPath,CurrEntry);
		}
	}

	const FString ExternalMountPointName(TEXT("/Game/"));
	for ( const auto &CurrEntry : CookDirectories )
	{
		TArray<FString> Files;
		IFileManager::Get().FindFilesRecursive(Files, *CurrEntry, *(FString(TEXT("*")) + FPackageName::GetAssetPackageExtension()), true, false);
		for (int32 Index = 0; Index < Files.Num(); Index++)
		{
			FString StdFile = Files[Index];
			FPaths::MakeStandardFilename(StdFile);
			AddFileToCook( FilesInPath,StdFile);

			// this asset may not be in our currently mounted content directories, so try to mount a new one now
			FString LongPackageName;
			if(!FPackageName::IsValidLongPackageName(StdFile) && !FPackageName::TryConvertFilenameToLongPackageName(StdFile, LongPackageName))
			{
				FPackageName::RegisterMountPoint(ExternalMountPointName, CurrEntry);
			}
		}
	}

	if ((FilesInPath.Num() == 0) || bCookAll)
	{
		TArray<FString> Tokens;
		Tokens.Empty(2);
		Tokens.Add(FString("*") + FPackageName::GetAssetPackageExtension());
		Tokens.Add(FString("*") + FPackageName::GetMapPackageExtension());

		uint8 PackageFilter = NORMALIZE_DefaultFlags | NORMALIZE_ExcludeEnginePackages;
		if ( bMapsOnly )
		{
			PackageFilter |= NORMALIZE_ExcludeContentPackages;
		}

		if ( bNoDev )
		{
			PackageFilter |= NORMALIZE_ExcludeDeveloperPackages;
		}

		// assume the first token is the map wildcard/pathname
		TArray<FString> Unused;
		for ( int32 TokenIndex = 0; TokenIndex < Tokens.Num(); TokenIndex++ )
		{
			TArray<FString> TokenFiles;
			if ( !NormalizePackageNames( Unused, TokenFiles, Tokens[TokenIndex], PackageFilter) )
			{
				UE_LOG(LogCookOnTheFly, Display, TEXT("No packages found for parameter %i: '%s'"), TokenIndex, *Tokens[TokenIndex]);
				continue;
			}

			for (int32 TokenFileIndex = 0; TokenFileIndex < TokenFiles.Num(); ++TokenFileIndex)
			{
				AddFileToCook( FilesInPath, TokenFiles[TokenFileIndex]);
			}
		}
	}

	// make sure we cook the default maps
	ITargetPlatformManagerModule& TPM = GetTargetPlatformManagerRef();
	static const TArray<ITargetPlatform*>& Platforms =  TPM.GetTargetPlatforms();
	for (int32 Index = 0; Index < Platforms.Num(); Index++)
	{
		// load the platform specific ini to get its DefaultMap
		FConfigFile PlatformEngineIni;
		FConfigCacheIni::LoadLocalIniFile(PlatformEngineIni, TEXT("Engine"), true, *Platforms[Index]->IniPlatformName());

		// get the server and game default maps and cook them
		FString Obj;
		if (PlatformEngineIni.GetString(TEXT("/Script/EngineSettings.GameMapsSettings"), TEXT("GameDefaultMap"), Obj))
		{
			if (Obj != FName(NAME_None).ToString())
			{
				AddFileToCook( FilesInPath, Obj);
			}
		}
		if ( IsCookFlagSet(ECookInitializationFlags::IncludeServerMaps) )
		{
			if (PlatformEngineIni.GetString(TEXT("/Script/EngineSettings.GameMapsSettings"), TEXT("ServerDefaultMap"), Obj))
			{
				if (Obj != FName(NAME_None).ToString())
				{
					AddFileToCook( FilesInPath, Obj);
				}
			}
		}
		if (PlatformEngineIni.GetString(TEXT("/Script/EngineSettings.GameMapsSettings"), TEXT("GlobalDefaultGameMode"), Obj))
		{
			if (Obj != FName(NAME_None).ToString())
			{
				AddFileToCook( FilesInPath, Obj);
			}
		}
		if (PlatformEngineIni.GetString(TEXT("/Script/EngineSettings.GameMapsSettings"), TEXT("GlobalDefaultServerGameMode"), Obj))
		{
			if (Obj != FName(NAME_None).ToString())
			{
				AddFileToCook( FilesInPath, Obj);
			}
		}
	}

	// make sure we cook any extra assets for the default touch interface
	// @todo need a better approach to cooking assets which are dynamically loaded by engine code based on settings
	FConfigFile InputIni;
	FString InterfaceFile;
	FConfigCacheIni::LoadLocalIniFile(InputIni, TEXT("Input"), true);
	if (InputIni.GetString(TEXT("/Script/Engine.InputSettings"), TEXT("DefaultTouchInterface"), InterfaceFile))
	{
		if (InterfaceFile != TEXT("None") && InterfaceFile != TEXT(""))
		{
			AddFileToCook( FilesInPath, InterfaceFile);
		}
	}

	//@todo SLATE: This is a hack to ensure all slate referenced assets get cooked.
	// Slate needs to be refactored to properly identify required assets at cook time.
	// Simply jamming everything in a given directory into the cook list is error-prone
	// on many levels - assets not required getting cooked/shipped; assets not put under 
	// the correct folder; etc.
	{
		TArray<FString> UIContentPaths;
		if (GConfig->GetArray(TEXT("UI"), TEXT("ContentDirectories"), UIContentPaths, GEditorIni) > 0)
		{
			for (int32 DirIdx = 0; DirIdx < UIContentPaths.Num(); DirIdx++)
			{
				FString ContentPath = FPackageName::LongPackageNameToFilename(UIContentPaths[DirIdx]);

				TArray<FString> Files;
				IFileManager::Get().FindFilesRecursive(Files, *ContentPath, *(FString(TEXT("*")) + FPackageName::GetAssetPackageExtension()), true, false);
				for (int32 Index = 0; Index < Files.Num(); Index++)
				{
					FString StdFile = Files[Index];
					FPaths::MakeStandardFilename(StdFile);
					AddFileToCook( FilesInPath, StdFile);
				}
			}
		}
	}
}


bool UCookOnTheFlyServer::IsCookByTheBookRunning() const
{
	return CookByTheBookOptions && CookByTheBookOptions->bRunning;
}


void UCookOnTheFlyServer::SaveGlobalShaderMapFiles(const TArray<ITargetPlatform*>& Platforms)
{
	for (int32 Index = 0; Index < Platforms.Num(); Index++)
	{
		// make sure global shaders are up to date!
		TArray<FString> Files;
		FShaderRecompileData RecompileData;
		RecompileData.PlatformName = Platforms[Index]->PlatformName();
		// Compile for all platforms
		RecompileData.ShaderPlatform = -1;
		RecompileData.ModifiedFiles = &Files;
		RecompileData.MeshMaterialMaps = NULL;

		check( IsInGameThread() );

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
}


void UCookOnTheFlyServer::CookByTheBookFinished()
{
	check( IsInGameThread() );
	check( IsCookByTheBookMode() );
	check( CookByTheBookOptions->bRunning == true );

	UPackage::WaitForAsyncFileWrites();

	GetDerivedDataCacheRef().WaitForQuiescence(true);

	{
		// Save modified asset registry with all streaming chunk info generated during cook
		FString RegistryFilename = FPaths::GameDir() / TEXT("AssetRegistry.bin");
		FString SandboxRegistryFilename = SandboxFile->ConvertToAbsolutePathForExternalAppForWrite(*RegistryFilename);
		// the registry filename will be modified when we call save asset registry
		
		FString CookedAssetRegistry = FPaths::GameDir() / TEXT("CookedAssetRegistry.json");
		FString SandboxCookedAssetRegistryFilename = SandboxFile->ConvertToAbsolutePathForExternalAppForWrite(*CookedAssetRegistry);

		for ( auto& Manifest : CookByTheBookOptions->ManifestGenerators )
		{
			// Always try to save the manifests, this is required to make the asset registry work, but doesn't necessarily write a file
			Manifest.Value->SaveManifests(SandboxFile.GetOwnedPointer());
			Manifest.Value->SaveAssetRegistry(SandboxRegistryFilename);

			Manifest.Value->SaveCookedPackageAssetRegistry(SandboxCookedAssetRegistryFilename, true);
		}
	}

	CookByTheBookOptions->LastGCItems.Empty();
	const float TotalCookTime = (float)(FPlatformTime::Seconds() - CookByTheBookOptions->CookStartTime);
	UE_LOG(LogCookOnTheFly, Display, TEXT("Cook by the book total time in tick %fs total time %f"), CookByTheBookOptions->CookTime, TotalCookTime);

	CookByTheBookOptions->bRunning = false;
}

void UCookOnTheFlyServer::QueueCancelCookByTheBook()
{
	if ( IsCookByTheBookMode() )
	{
		check( CookByTheBookOptions != NULL );
		CookByTheBookOptions->bCancel = true;
	}
}

void UCookOnTheFlyServer::CancelCookByTheBook()
{
	if ( IsCookByTheBookMode() && CookByTheBookOptions->bRunning )
	{
		check(CookByTheBookOptions);
		check( IsInGameThread() );

		// save the cook requests 
		CookRequests.DequeueAllRequests(CookByTheBookOptions->PreviousCookRequests);
		CookByTheBookOptions->bRunning = false;
	}	
}

void UCookOnTheFlyServer::StopAndClearCookedData()
{
	if ( IsCookByTheBookMode() )
	{
		check( CookByTheBookOptions != NULL );
		check( CookByTheBookOptions->bRunning == false );
		CancelCookByTheBook();
		CookByTheBookOptions->PreviousCookRequests.Empty();
	}

	RecompileRequests.Empty();
	CookRequests.Empty();
	UnsolicitedCookedPackages.Empty();
	CookedPackages.Empty(); // set of files which have been cooked when needing to recook a file the entry will need to be removed from here
}

void UCookOnTheFlyServer::StartCookByTheBook(const TArray<ITargetPlatform*>& TargetPlatforms, 
											 const TArray<FString> &CookMaps, const TArray<FString> &CookDirectories, 
											 const TArray<FString> &CookCultures, const TArray<FString> &IniMapSections, 
											 ECookByTheBookOptions CookOptions )
{
	check( IsInGameThread() );

	// can transition from cook on the fly to cook by the book from the editor
	check( IsCookByTheBookMode() );

	/// reference ini parsing code
	/*FString SectionStr;
	if (FParse::Value(InCmdParams, TEXT("MAPINISECTION="), SectionStr))
	{
		if (SectionStr.Contains(TEXT("+")))
		{
			TArray<FString> Sections;
			SectionStr.ParseIntoArray(&Sections,TEXT("+"),true);
			for (int32 Index = 0; Index < Sections.Num(); Index++)
			{
				LoadMapListFromIni(Sections[Index], OutMapList);
			}
		}
		else
		{
			LoadMapListFromIni(SectionStr, OutMapList);
		}
	}*/
	

	/*
	// copied from original cook commandlet
	// need to fill these in
	bCookAll = Switches.Contains(TEXT("COOKALL"));   // Cook everything
	bLeakTest = Switches.Contains(TEXT("LEAKTEST"));   // Test for UObject leaks

	*/

	CookByTheBookOptions->bRunning = true;
	CookByTheBookOptions->bCancel = false;
	CookByTheBookOptions->CookTime = 0.0f;
	CookByTheBookOptions->CookStartTime = FPlatformTime::Seconds();

	bool bCookAll = (CookOptions & ECookByTheBookOptions::CookAll) != ECookByTheBookOptions::None;
	bool bMapsOnly = (CookOptions & ECookByTheBookOptions::MapsOnly) != ECookByTheBookOptions::None;
	bool bNoDev = (CookOptions & ECookByTheBookOptions::NoDevContent) != ECookByTheBookOptions::None;

	CookByTheBookOptions->bLeakTest = (CookOptions & ECookByTheBookOptions::LeakTest) != ECookByTheBookOptions::None; // this won't work from the editor this needs to be standalone
	check( !CookByTheBookOptions->bLeakTest || CurrentCookMode == ECookMode::CookByTheBook );


	CookByTheBookOptions->LastGCItems.Empty();
	if (CookByTheBookOptions->bLeakTest )
	{
		for (FObjectIterator It; It; ++It)
		{
			CookByTheBookOptions->LastGCItems.Add(FWeakObjectPtr(*It));
		}
	}


	
	// allow the game to fill out the asset registry, as well as get a list of objects to always cook
	TArray<FString> FilesInPath;
	FGameDelegates::Get().GetCookModificationDelegate().ExecuteIfBound(FilesInPath);

	SaveGlobalShaderMapFiles(TargetPlatforms);

	CollectFilesToCook(FilesInPath, CookMaps, CookDirectories, CookCultures, IniMapSections, bCookAll, bMapsOnly, bNoDev );
	if (FilesInPath.Num() == 0)
	{
		LogCookerMessage( FString::Printf(TEXT("No files found to cook.")), EMessageSeverity::Warning );
		UE_LOG(LogCookOnTheFly, Warning, TEXT("No files found."));
	}


	TArray<FName> TargetPlatformNames;
	for( const auto &Platform : TargetPlatforms )
	{
		FName PlatformName = FName(*Platform->PlatformName());
		TargetPlatformNames.Add( PlatformName ); // build list of all target platform names

		// make sure we have a manifest for all the platforms 
		// we want a seperate manifest for each platform because they can all be in different states of cooked content
		FChunkManifestGenerator* ManifestGenerator = CookByTheBookOptions->ManifestGenerators.FindRef( PlatformName );
		if ( ManifestGenerator == NULL )
		{
			TArray<ITargetPlatform*> Platforms;
			Platforms.Add( Platform );
			ManifestGenerator = new FChunkManifestGenerator(Platforms);
			ManifestGenerator->CleanManifestDirectories();
			ManifestGenerator->Initialize( CookByTheBookOptions->bGenerateStreamingInstallManifests);

			CookByTheBookOptions->ManifestGenerators.Add(PlatformName, ManifestGenerator);
		}
	}

	GenerateLongPackageNames(FilesInPath);

	// add all the files for the requested platform to the cook list
	for ( const auto& FileName : FilesInPath )
	{
		FName FileFName = FName(*FileName);
		FName PackageFileFName = GetCachedStandardPackageFileFName(FileFName);
		
		if (PackageFileFName != NAME_None)
		{
			CookRequests.EnqueueUnique( MoveTemp( FFilePlatformRequest( PackageFileFName, TargetPlatformNames ) ) );
		}
		else
		{
			LogCookerMessage( FString::Printf(TEXT("Unable to find package for cooking %s"), *FileName), EMessageSeverity::Warning );
			UE_LOG(LogCookOnTheFly, Warning, TEXT("Unable to find package for cooking %s"), *FileName)
		}	
	}

	for ( const auto& PreviousRequest : CookByTheBookOptions->PreviousCookRequests )
	{
		CookRequests.EnqueueUnique( MoveTemp( PreviousRequest ) );
	}
	CookByTheBookOptions->PreviousCookRequests.Empty();

}





/* UCookOnTheFlyServer callbacks
 *****************************************************************************/


void UCookOnTheFlyServer::HandleNetworkFileServerFileRequest( const FString& Filename, const FString &Platformname, TArray<FString>& UnsolicitedFiles )
{
	check( CurrentCookMode == ECookMode::CookOnTheFly );	
	
	bool bIsCookable = FPackageName::IsPackageExtension(*FPaths::GetExtension(Filename, true));


	FName PlatformFname = FName( *Platformname );

	if (!bIsCookable)
	{
		TArray<FName> UnsolicitedFilenames;
		UnsolicitedCookedPackages.GetPackagesForPlatformAndRemove(PlatformFname, UnsolicitedFilenames);

		for ( const auto &UnsolicitedFile : UnsolicitedFilenames )
		{	
			FString StandardFilename = UnsolicitedFile.ToString();
			FPaths::MakeStandardFilename( StandardFilename );
			UnsolicitedFiles.Add( StandardFilename );
		}
		UPackage::WaitForAsyncFileWrites();
		return;
	}

	FString StandardFileName = Filename;
	FPaths::MakeStandardFilename( StandardFileName );

	FName StandardFileFname = FName( *StandardFileName );
	TArray<FName> Platforms;
	Platforms.Add( PlatformFname );
	FFilePlatformRequest FileRequest( StandardFileFname, Platforms);
	CookRequests.EnqueueUnique(FileRequest, true);

	do
	{
		FPlatformProcess::Sleep(0.0f);
	}
	while (!CookedPackages.Exists(FileRequest));

	UE_LOG( LogCookOnTheFly, Display, TEXT("Cook complete %s"), *FileRequest.GetFilename().ToString())

	TArray<FName> UnsolicitedFilenames;
	UnsolicitedCookedPackages.GetPackagesForPlatformAndRemove(PlatformFname, UnsolicitedFilenames);
	UnsolicitedFilenames.Remove(FileRequest.GetFilename());

	for ( const auto &UnsolicitedFile : UnsolicitedFilenames )
	{	
		FString StandardFilename = UnsolicitedFile.ToString();
		FPaths::MakeStandardFilename( StandardFilename );
		UnsolicitedFiles.Add( StandardFilename );
	}

	UPackage::WaitForAsyncFileWrites();


#if DEBUG_COOKONTHEFLY
	UE_LOG( LogCookOnTheFly, Display, TEXT("Processed file request %s"), *Filename );
#endif

}


void UCookOnTheFlyServer::HandleNetworkFileServerRecompileShaders(const FShaderRecompileData& RecompileData)
{
	// shouldn't receive network requests unless we are in cook on the fly mode
	check( CurrentCookMode == ECookMode::CookOnTheFly );
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
		RecompileData.ModifiedFiles,
		RecompileData.bCompileChangedShaders);
}

void UCookOnTheFlyServer::WarmCookedPackages(const FString& AssetRegistryPath, const TArray<FName>& TargetPlatformNames)
{
	FArrayReader SerializedAssetData;
	if (FFileHelper::LoadFileToArray(SerializedAssetData, *AssetRegistryPath))
	{
		int32 LocalNumAssets = 0;
		SerializedAssetData << LocalNumAssets;

		// allocate one single block for all asset data structs (to reduce tens of thousands of heap allocations)
		FAssetData* PreallocatedAssetDataBuffer = new FAssetData[LocalNumAssets];
		CookedPackages.Empty(LocalNumAssets);

		FFilePlatformRequest FileRequest;
		for (const auto &Platform : TargetPlatformNames)
		{
			FileRequest.AddPlatform(Platform);
		}

		for (int32 AssetIndex = 0; AssetIndex < LocalNumAssets; AssetIndex++)
		{
			// make a new asset data object
			FAssetData* NewAssetData = &PreallocatedAssetDataBuffer[AssetIndex];

			// load it
			SerializedAssetData << *NewAssetData;

			UE_LOG(LogCookOnTheFly, Verbose, TEXT("Read package %s from %s"), *GetCachedStandardPackageFilename(NewAssetData->ObjectPath), *AssetRegistryPath);

			FileRequest.SetFilename(GetCachedStandardPackageFilename(NewAssetData->ObjectPath));
			if (FileRequest.IsValid())
			{
				CookedPackages.Add(FileRequest);
			}
		}

		delete[] PreallocatedAssetDataBuffer;
	}
}