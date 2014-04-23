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

	/** Holds the instance identifier. */
	FGuid InstanceId;

	/** Holds the sandbox file wrapper to handle sandbox path conversion. */
	TAutoPtr<class FSandboxPlatformFile> SandboxFile;
	/** Cook on the fly server uses the NetworkFileServer */
	class INetworkFileServer* NetworkFileServer;

	/** We hook this up to a delegate to avoid reloading textures and whatnot */
	TSet<FString> PackagesToNotReload;



	/** Array which has been made thread safe :) */
	template<typename Type>
	struct FThreadSafeQueue
	{
	private:
		mutable FCriticalSection	SynchronizationObject; // made this mutable so this class can have const functions and still be thread safe
		TArray<Type>		Items;
	public:
		void Enqueue(const Type& Item)
		{
			FScopeLock ScopeLock(&SynchronizationObject);
			Items.Add(Item);
		}
		void EnqueueUnique( const Type& Item )
		{
			FScopeLock ScopeLock(&SynchronizationObject);
			Items.AddUnique(Item);
		}
		bool Dequeue(Type* Result)
		{
			FScopeLock ScopeLock(&SynchronizationObject);
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
			FScopeLock ScopeLock(&SynchronizationObject);
			Results += Items;
			Items.Empty();
		}

		bool HasItems() const
		{
			FScopeLock ScopeLock( &SynchronizationObject );
			return Items.Num() > 0;
		}

		void Remove( const Type& Item ) 
		{
			FScopeLock ScopeLock( &SynchronizationObject );
			Items.Remove( Item );
		}
	};

	/** Normal queue to match the interface of FThreadSafeQueue :) */
	template<typename Type>
	struct FQueue
	{
	private:
		TArray<Type>		Items;
	public:
		void Enqueue(const Type& Item)
		{
			Items.Add(Item);
		}
		void EnqueueUnique( const Type& Item )
		{
			Items.AddUnique(Item);
		}
		bool Dequeue(Type* Result)
		{
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
			Results += Items;
			Items.Empty();
		}

		bool HasItems() const
		{
			return Items.Num() > 0;
		}


		void Remove( const Type& Item ) 
		{
			Items.Remove( Item );
		}
	};

public:
	/** cooked file requests which includes platform which file is requested for */
	struct FFilePlatformRequest
	{
	public:
		FFilePlatformRequest() { }
		FFilePlatformRequest( const FString &InFilename, const FString &InPlatformname ) : Filename( FPaths::ConvertRelativePathToFull( InFilename) ), Platformname( InPlatformname ) 
		{
		}

		FString Filename;
		FString Platformname;

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
			return FString::Printf( TEXT("%s %s"), *Filename, *Platformname );
		}

	};

private:

	FThreadSafeQueue<FFilePlatformRequest> FileRequests; // list of requested files
	FQueue<FFilePlatformRequest> UnsolicitedFileRequests;
	FThreadSafeQueue<FFilePlatformRequest> UnsolicitedCookedPackages; // list of files which haven't been requested but we think should cook based on previous requests



	/** Helper list of all files which have been cooked */
	struct FThreadSafeFilenameSet
	{
		FCriticalSection	SynchronizationObject;
		TSet<FFilePlatformRequest> FilesProcessed;

		void Add(const FFilePlatformRequest &Filename)
		{
			FScopeLock ScopeLock(&SynchronizationObject);
			check(Filename.IsValid());
			FilesProcessed.Add(Filename);
		}
		bool Exists(const FFilePlatformRequest &Filename)
		{
			FScopeLock ScopeLock(&SynchronizationObject);
			return FilesProcessed.Contains(Filename);
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

	/** 
	 * Stop the network file server
	 *
	 */
	void EndNetworkFileServer();


	uint32 NumConnections() const;

	


	virtual void BeginDestroy() OVERRIDE;




private:
	

	bool UCookOnTheFlyServer::ShouldCook(const FString& InFileName, const FString &InPlatformName);

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
	bool SaveCookedPackage( UPackage* Package, uint32 SaveFlags, bool& bOutWasUpToDate, TArray<FString> &TargetPlatformNames );

	// bool ShouldCook(const FString& InFilename, const FString &InPlatformname = TEXT(""));

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

