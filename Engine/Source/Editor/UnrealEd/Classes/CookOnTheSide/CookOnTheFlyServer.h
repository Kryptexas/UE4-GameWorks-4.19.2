// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	CookOnTheFlyServer.h : handles polite cook requests via network ;)
=============================================================================*/

#pragma once

#include "CookOnTheFlyServer.generated.h"



UCLASS()
class UCookOnTheFlyServer : public UObject, public FTickableEditorObject
{
	GENERATED_UCLASS_BODY()

private:
	/** Cook ont he fly is almost always true, just leaving this option here if we want to switch the cook commandlet over to using this structure to cook */
	bool bCookOnTheFly;
	/** If true, iterative cooking is being done */
	bool bIterativeCooking;
	/** If true, packages are cooked compressed */
	bool bCompressed;
	/** Skip saving any packages in Engine/COntent/Editor* UNLESS TARGET HAS EDITORONLY DATA (in which case it will save those anyway) */
	bool bSkipEditorContent;
	/** Should we use Async loading and saving (good for the editor) */
	bool bAsyncSaveLoad;
	/** Should we auto tick (used in the editor) */
	bool bIsAutoTick;
	/** Holds the sandbox file wrapper to handle sandbox path conversion. */
	TAutoPtr<class FSandboxPlatformFile> SandboxFile;
	/** Cook on the fly server uses the NetworkFileServer */
	class INetworkFileServer* NetworkFileServer;

	/** We hook this up to a delegate to avoid reloading textures and whatnot */
	TSet<FString> PackagesToNotReload;

private:

	/** Array which has been made thread safe :) */
	template<typename Type, typename SynchronizationObjectType, typename ScopeLockType>
	struct FUnsyncronizedQueue
	{
	private:
		mutable SynchronizationObjectType	SynchronizationObject; // made this mutable so this class can have const functions and still be thread safe
		TArray<Type>		Items;
	public:
		void Enqueue(const Type& Item)
		{
			ScopeLockType ScopeLock(&SynchronizationObject);
			Items.Add(Item);
		}
		void EnqueueUnique( const Type& Item )
		{
			ScopeLockType ScopeLock(&SynchronizationObject);
			Items.AddUnique(Item);
		}
		bool Dequeue(Type* Result)
		{
			ScopeLockType ScopeLock(&SynchronizationObject);
			if (Items.Num())
			{
				*Result = Items[0];
				Items.RemoveAt(0);
				return true;
			}
			return false;
		}
		void DequeueAll(TArray<Type>& Results)
		{
			ScopeLockType ScopeLock(&SynchronizationObject);
			Results += Items;
			Items.Empty();
		}

		bool HasItems() const
		{
			ScopeLockType ScopeLock( &SynchronizationObject );
			return Items.Num() > 0;
		}

		void Remove( const Type& Item ) 
		{
			ScopeLockType ScopeLock( &SynchronizationObject );
			Items.Remove( Item );
		}

		void CopyItems( TArray<Type> &InItems ) const
		{
			ScopeLockType ScopeLock( &SynchronizationObject );
			InItems = Items;
		}

		int Num() const 
		{
			ScopeLockType ScopeLock( &SynchronizationObject );
			return Items.Num();
		}
	};



	struct FDummyCriticalSection
	{
	public:
		FORCEINLINE void Lock() { }
		FORCEINLINE void Unlock() { }
	};

	struct FDummyScopeLock
	{
	public:
		FDummyScopeLock( FDummyCriticalSection * ) { }
	};

public:


	template<typename Type>
	struct FThreadSafeQueue : public FUnsyncronizedQueue<Type, FCriticalSection, FScopeLock>
	{
		/**
		 * Don't add any functions here, this is just a overqualified typedef
		 * Add functions / functionality to the FUnsyncronizedQueue
		 */
	};

	template<typename Type>
	struct FQueue : public FUnsyncronizedQueue<Type, FDummyCriticalSection, FDummyScopeLock>
	{
		/**
		 * Don't add any functions here, this is just a overqualified typedef
		 * Add functions / functionality to the FUnsyncronizedQueue
		 */
	};

public:
	/** cooked file requests which includes platform which file is requested for */
	struct FFilePlatformRequest
	{
	public:
		FFilePlatformRequest() { }
		FFilePlatformRequest( const FString &InFilename, const FName &InPlatformname ) : Filename( InFilename ), Platformname( InPlatformname ) 
		{
		}

		FString Filename;
		FName Platformname;

		bool IsValid()  const
		{
			return Filename.Len() > 0;
		}

		void Clear()
		{
			Filename = TEXT("");
			Platformname = TEXT("");
		}

		bool operator ==( const FFilePlatformRequest &InFileRequest ) const
		{
			if ( InFileRequest.Filename == Filename )
			{
				if ( InFileRequest.Platformname == Platformname )
					return true;
			}
			return false;
		}

		FORCEINLINE FString ToString() const
		{
			return FString::Printf( TEXT("%s %s"), *Filename, *Platformname.ToString() );
		}

	};

private:

	FThreadSafeQueue<FFilePlatformRequest> FileRequests; // list of requested files
	FQueue<FFilePlatformRequest> UnsolicitedFileRequests; // list of files which haven't been requested but we think should cook based on previous requests
	FThreadSafeQueue<FFilePlatformRequest> UnsolicitedCookedPackages; // list of files which weren't requested but were cooked (based on UnsolicitedFileRequests)


	/** Helper list of all files which have been cooked */
	struct FThreadSafeFilenameSet
	{
	private:
		mutable FCriticalSection	SynchronizationObject;
		TSet<FFilePlatformRequest> FilesProcessed;
	public:
		void Add(const FFilePlatformRequest &Filename)
		{
			FScopeLock ScopeLock(&SynchronizationObject);
			check(Filename.IsValid());
			FilesProcessed.Add(Filename);
		}
		bool Exists(const FFilePlatformRequest &Filename) const
		{
			FScopeLock ScopeLock(&SynchronizationObject);
			return FilesProcessed.Contains(Filename);
		}
		int RemoveAll( const FString &Filename )
		{
			FScopeLock ScopeLock( &SynchronizationObject );
			int NumRemoved = 0;
			// for ( TSet<FFilePlatformRequest>::TIterator It( FilesProcessed ); It; ++It )
			for ( auto It = FilesProcessed.CreateIterator(); It; ++It )
			{
				if ( It->Filename == Filename )
				{
					It.RemoveCurrent();
					++NumRemoved;
				}
			}
			return NumRemoved;
		}
	};
	FThreadSafeFilenameSet ThreadSafeFilenameSet; // set of files which have been cooked when needing to recook a file the entry will need to be removed from here


	FThreadSafeQueue<struct FRecompileRequest*> RecompileRequests;


public:


	

	enum ECookOnTheSideResult
	{
		COSR_CookedMap			= 0x00000001,
		COSR_CookedPackage		= 0x00000002,
		COSR_ErrorLoadingPackage= 0x00000004,
	};



	/**
	 * FTickableEditorObject interface used by cook on the side
	 */
	TStatId GetStatId() const OVERRIDE;
	 void Tick(float DeltaTime) OVERRIDE;
	 bool IsTickable() const OVERRIDE;





	/**
	 * Initialize the CookServer so that either CookOnTheFly can be called or Cook on the side can be started and ticked
	 */
	void Initialize( bool inCompressed, bool inIterativeCooking, bool inSkipEditorContent, bool inIsAutoTick = false, bool inAsyncLoadSave = false, const FString &OutputDirectoryOverride = FString(TEXT("")) );

	/**
	 * Cook on the side, cooks while also running the editor...
	 *
	 * @param  BindAnyPort					Whether to bind on any port or the default port.
	 *
	 * @return true on success, false otherwise.
	 */
	bool StartNetworkFileServer( bool BindAnyPort );

	/**
	 * Broadcast our the fileserver presence on the network
	 */
	bool BroadcastFileserverPresence( const FGuid &InstanceId );
	/** 
	 * Stop the network file server
	 *
	 */
	void EndNetworkFileServer();

	
	/**
	 * Handles cook package requests until there are no more requests, then returns
	 *
	 * @param  CookFlags output of the things that might have happened in the cook on the side
	 * 
	 * @return returns ECookOnTheSideResult
	 */
	uint32 TickCookOnTheSide( const float TimeSlice, uint32 &CookedPackagesCount );
	
	/** 
	 * Process any shader recompile requests
	 *
	 */
	void TickRecompileShaderRequests();

	bool HasCookRequests() const { return FileRequests.HasItems(); }
	bool HasUnsolicitedCookRequests() const { return UnsolicitedFileRequests.HasItems(); }

	bool HasRecompileShaderRequests() const { return RecompileRequests.HasItems(); }


	uint32 NumConnections() const;

	


	virtual void BeginDestroy() OVERRIDE;
	



	/**
	 * Callbacks from editor 
	 */

	void OnObjectModified( UObject *ObjectMoving );
	void OnObjectPropertyChanged(UObject* ObjectBeingModified, FPropertyChangedEvent& PropertyChangedEvent);
	void OnObjectUpdated( UObject *Object );

	/**
	 * Marks a package as dirty for cook
	 * causes package to be recooked on next request (and all dependent packages which are currently cooked)
	 */
	void MarkPackageDirtyForCooker( UPackage *Package );



private:
	

	bool ShouldCook(const FString& InFileName, const FName &InPlatformName);

	/**
	 * Returns cooker output directory.
	 *
	 * @param PlatformName Target platform name.
	 * @return Cooker output directory for the specified platform.
	 */
	FString GetOutputDirectory( const FString& PlatformName ) const;

	/**
	 *	Get the given packages 'cooked' timestamp (i.e. account for dependencies)
	 *
	 *	@param	InFilename			The filename of the package
	 *	@param	OutDateTime			The timestamp the cooked file should have
	 *
	 *	@return	bool				true if the package timestamp was found, false if not
	 */
	bool GetPackageTimestamp( const FString& InFilename, FDateTime& OutDateTime );

	/**
	 *	Cook (save) the given package
	 *
	 *	@param	Package				The package to cook/save
	 *	@param	SaveFlags			The flags to pass to the SavePackage function
	 *	@param	bOutWasUpToDate		Upon return, if true then the cooked package was cached (up to date)
	 *
	 *	@return	bool			true if packages was cooked
	 */
	bool SaveCookedPackage( UPackage* Package, uint32 SaveFlags, bool& bOutWasUpToDate );
	/**
	 *	Cook (save) the given package
	 *
	 *	@param	Package				The package to cook/save
	 *	@param	SaveFlags			The flags to pass to the SavePackage function
	 *	@param	bOutWasUpToDate		Upon return, if true then the cooked package was cached (up to date)
	 *	@param  TargetPlatformNames Only cook for target platforms which are included in this array (if empty cook for all target platforms specified on commandline options)
	 *									TargetPlatformNames is in and out value returns the platforms which the SaveCookedPackage function saved for
	 *
	 *	@return	bool			true if packages was cooked
	 */
	bool SaveCookedPackage( UPackage* Package, uint32 SaveFlags, bool& bOutWasUpToDate, TArray<FName> &TargetPlatformNames );

	// Callback for handling a network file request.
	void HandleNetworkFileServerFileRequest( const FString& Filename, const FString &Platformname, TArray<FString>& UnsolicitedFiles );

	// Callback for recompiling shaders
	void HandleNetworkFileServerRecompileShaders(const struct FShaderRecompileData& RecompileData);


	void MaybeMarkPackageAsAlreadyLoaded(UPackage *Package);

	/** Gets the output directory respecting any command line overrides */
	FString GetOutputDirectoryOverride( const FString &OutputDirectoryOverride ) const;

	/** Cleans sandbox folders for all target platforms */
	void CleanSandbox(const TArray<ITargetPlatform*>& Platforms);

	/** Generates asset registry */
	void GenerateAssetRegistry(const TArray<ITargetPlatform*>& Platforms);

	/** Saves global shader map files */
	// void SaveGlobalShaderMapFiles(const TArray<ITargetPlatform*>& Platforms);

	/** Collects all files to be cooked. This includes all commandline specified maps */
	//void CollectFilesToCook(TArray<FString>& FilesInPath);

	/** Generates long package names for all files to be cooked */
	void GenerateLongPackageNames(TArray<FString>& FilesInPath);

	/** Cooks all files */
	//bool Cook(const TArray<ITargetPlatform*>& Platforms, TArray<FString>& FilesInPath);

};

FORCEINLINE uint32 GetTypeHash(const UCookOnTheFlyServer::FFilePlatformRequest &Key)
{
	return GetTypeHash( Key.Filename ) ^ GetTypeHash( Key.Platformname );
}

