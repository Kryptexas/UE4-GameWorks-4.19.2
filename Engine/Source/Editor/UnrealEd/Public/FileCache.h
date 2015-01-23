// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Serialization/CustomVersion.h"
#include "IDirectoryWatcher.h"

/** An immutable string with a cached CRC for efficient comparison with other strings */
struct FImmutableString
{
	/** Constructible from a raw string */
	FImmutableString(FString InString = FString()) : String(MoveTemp(InString)), CachedHash(0) {}
	FImmutableString& operator=(FString InString) { String = MoveTemp(InString); CachedHash = 0; return *this; }

	FImmutableString(const FImmutableString&) = default;
	FImmutableString& operator=(const FImmutableString&) = default;

	/** Move construction/assignment */
	FImmutableString(FImmutableString&& In) : String(MoveTemp(In.String)), CachedHash(MoveTemp(In.CachedHash)) {}
	FImmutableString& operator=(FImmutableString&& In) { Swap(String, In.String); Swap(CachedHash, In.CachedHash); return *this; }

	/** Check for equality with another immutable string (case-sensitive) */
	friend bool operator==(const FImmutableString& A, const FImmutableString& B)
	{
		return GetTypeHash(A) == GetTypeHash(B) && A.String == B.String;
	}

	/** Calculate the type hash for this string */
	friend uint32 GetTypeHash(const FImmutableString& In)
	{
		if (In.CachedHash == 0 && In.String.Len() != 0)
		{
			In.CachedHash = GetTypeHash(In.String);
		}
		return In.CachedHash;
	}

	/** Get the underlying string */
	const FString& Get() const { return String; }

	/** Serialise this string */
	friend FArchive& operator<<(FArchive& Ar, FImmutableString& String)
	{
		Ar << String.String;
		if (Ar.IsSaving())
		{
			GetTypeHash(String);	
		}
		
		Ar << String.CachedHash;

		return Ar;
	}

private:
	/** The string itself. */
	FString String;

	/** The cached hash of our string. 0 until compared with another string. */
	mutable uint32 CachedHash;
};

/** Custom serialization version for FFileCache */
struct UNREALED_API FFileCacheCustomVersion
{
	static const FGuid Key;
	enum Type {	Initial, Latest = Initial };
};

/** Structure representing specific information about a particular file */
struct FFileData
{
	/** Constructiors */
	FFileData() {}
	FFileData(const FDateTime& InTimestamp) : Timestamp(InTimestamp) {}

	/** Serializer for this type */
	friend FArchive& operator<<(FArchive& Ar, FFileData& Data)
	{
		if (Ar.CustomVer(FFileCacheCustomVersion::Key) >= FFileCacheCustomVersion::Initial)
		{
			Ar << Data.Timestamp;
		}

		return Ar;
	}

	FDateTime Timestamp;
};

/** Structure representing the file data for a number of files in a directory */
struct FDirectoryState
{
	/** Default construction */
	FDirectoryState() {}

	/** Move construction */
	FDirectoryState(FDirectoryState&& In) : Files(MoveTemp(In.Files)) {}
	FDirectoryState& operator=(FDirectoryState&& In) { Swap(Files, In.Files); return *this; }

	/** Filename -> data map */
	TMap<FImmutableString, FFileData> Files;

	/** Serializer for this type */
	friend FArchive& operator<<(FArchive& Ar, FDirectoryState& State)
	{
		Ar.UsingCustomVersion(FFileCacheCustomVersion::Key);

		if (Ar.CustomVer(FFileCacheCustomVersion::Key) >= FFileCacheCustomVersion::Initial)
		{
			// Number of files
			int32 Num = State.Files.Num();
			Ar << Num;
			if (Ar.IsLoading())
			{
				State.Files.Reserve(Num);
			}

			Ar << State.Files;
		}

		return Ar;
	}
};

/** A transaction issued by FFileCache to describe a change to the cache. The change is only committed once the transaction is returned to the cache (see FFileCache::CompleteTransaction). */
struct FUpdateCacheTransaction
{
	/** The path of the file to which this transaction relates */
	FImmutableString Filename;

	/** The timestamp of the file at the time of this change */
	FDateTime Timestamp;

	/** The type of action that prompted this transaction */
	FFileChangeData::EFileChangeAction Action;

	/** Publically moveable */
	FUpdateCacheTransaction(FUpdateCacheTransaction&& In) : Filename(MoveTemp(In.Filename)), Timestamp(MoveTemp(In.Timestamp)), Action(MoveTemp(In.Action)) {}
	FUpdateCacheTransaction& operator=(FUpdateCacheTransaction&& In) { Swap(Filename, In.Filename); Swap(Timestamp, In.Timestamp); Swap(Action, In.Action); return *this; }

private:
	friend class FFileCache;
	
	/** Construction responsibility is held by FFileCache */
	FUpdateCacheTransaction(FImmutableString InFilename, FFileChangeData::EFileChangeAction InAction, const FDateTime& InTimestamp = 0)
		: Filename(MoveTemp(InFilename)), Timestamp(InTimestamp), Action(InAction)
	{}
};

/** A time limit that counts down from the time of construction, until it hits a given delay */
struct FTimeLimit
{
	/** Constructor specifying that we should never bail out early */
	FTimeLimit() : Delay(-1) { Reset(); }

	/** Constructor specifying not to run over the specified number of seconds */
	FTimeLimit(float NumSeconds) : Delay(NumSeconds) { Reset(); }

	/** Check whether we have exceeded the time limit */
	bool Exceeded() const { return Delay != -1 && FPlatformTime::Seconds() >= StartTime + Delay; }

	/** Reset the time limit to start timing again from the current time */
	void Reset() { StartTime = FPlatformTime::Seconds(); }

private:

	/** The delay specified by the user */
	float Delay;

	/** The time we started */
	double StartTime;
};

/** Enum specifying whether a path should be relative or absolute */
enum EPathType
{
	/** Paths should be cached relative to the root cache directory */
	Relative,

	/** Paths should be cached as absolute file system paths */
	Absolute
};

/**
 * Class responsible for 'asyncronously' scanning a folder for files and timestamps.
 * Example usage:
 *		FAsyncDirectoryReader Reader(TEXT("C:\\Path"), EPathType::Relative);
 *
 *		while(!Reader.IsComplete())
 *		{
 *			FPlatformProcess::Sleep(1);
 *			Reader.Tick(FTimedSignal(1));	// Do 1 second of work
 *		}
 *		TOptional<FDirectoryState> State = Reader.GetFinalState();
 */
struct FAsyncDirectoryReader
{
	enum class EProgressResult
	{
		Finished, Pending
	};

	/** Constructor that sets up the directory reader to the specified directory */
	FAsyncDirectoryReader(const FString& InDirectory, EPathType InPathType);

	/**
	 * Get the state of the directory once finished. Relinquishes the currently stored directory state to the client.
	 * Returns nothing if incomplete, or if GetFinalState() has already been called.
	 */
	TOptional<FDirectoryState> GetFinalState();

	/** Returns true when this directory reader has finished scanning the directory */
	bool IsComplete() const;

	/** Tick this reader (discover new directories / files). Returns progress state. */
	EProgressResult Tick(const FTimeLimit& Limit);

private:
	/** Non-recursively scan a single directory for its contents. Adds results to Pending arrays. */
	void ScanDirectory(const FString& InDirectory);

	/** Path to the root directory we want to scan */
	FString RootPath;

	/** Whether we should return relative or absolute paths */
	EPathType PathType;

	/** The currently discovered state of the directory */
	TOptional<FDirectoryState> State;

	/** A list of directories we have recursively found on our travels */
	TArray<FString> PendingDirectories;

	/** A list of files we have recursively found on our travels */
	TArray<FString> PendingFiles;
};

/** Configuration structure required to construct a FFileCache */
struct FFileCacheConfig
{
	FFileCacheConfig(FString InDirectory, FString InCacheFile)
		: Directory(InDirectory), CacheFile(InCacheFile), PathType(EPathType::Relative)
	{}

	/** String specifying the directory on disk that the cache should reflect */
	FString Directory;

	/** String specifying the file that the cache should be saved to */
	FString CacheFile;

	/**
	 * Strings specifying extensions to include/exclude in the cache (or empty to include everything).
	 * Specified as a list of semi-colon separated extensions (eg "png;jpg;gif;").
	 */
	FString IncludeExtensions, ExcludeExtensions;

	/** Path type to return, relative to the directory or absolute. */
	EPathType PathType;
};

/**
 * A class responsible for scanning a directory, and maintaining a cache of its state (files and timestamps).
 * Changes in the cache can be retrieved through GetOutstandingChanges(). Changes will be reported for any
 * change in the cached state even between runs of the process.
 */
class UNREALED_API FFileCache
{
public:

	/** Construction from a config */
	FFileCache(const FFileCacheConfig& InConfig);

	FFileCache(const FFileCache&) = delete;
	FFileCache& operator=(const FFileCache&) = delete;

	/** Destructor */
	~FFileCache();

	/** Destroy this cache. Cleans out the in-memory state and deletes the cache file, if present. */
	void Destroy();

	/** Get the absolute path of the directory this cache reflects */
	const FString& GetDirectory() const { return Config.Directory; }

	/** Tick this FileCache, optionally specifying a limiter to limit the amount of time spent in here */
	void Tick(const FTimeLimit& Limit);

	/** Write out the cached file, if we have any changes to write */
	void WriteCache();

	/**
	 * Return a transaction to the cache for completion. Will update the cached state with the change
	 * described in the transaction, and mark the cache as needing to be saved.
	 */
	void CompleteTransaction(FUpdateCacheTransaction&& Transaction);

	/** Get the number of pending changes to the cache. */
	int32 GetNumOutstandingChanges() const { return OutstandingChanges.Num(); }

	/** Get all pending changes to the cache. Transactions must be returned to CompleteTransaction to update the cache. */
	TArray<FUpdateCacheTransaction> GetOutstandingChanges();

private:

	/** Called when the directory we are monitoring has been changed in some way */
	void OnDirectoryChanged(const TArray<FFileChangeData>& FileChanges);

	/** Check whether a file is applicable to our cache */
	bool IsFileApplicable(const FString& InPath);

	/**
	 * Check whether a filename's extension matches the specified extension list.
	 * InExtensions must be of the form ";ext1;ext2;ext3;". Note the leading and trailing ;
	 */
	static bool MatchExtensionString(const FString& InExtensions, const FString& InFilename);

	/** Unbind the watcher from the directory. Called on destruction. */
	void UnbindWatcher();

	/** Read the cache file data and return the contents */
	TOptional<FDirectoryState> ReadCache() const;

private:

	/** Configuration settings applied on construction */
	FFileCacheConfig Config;

	/** 'Asynchronous' directory reader responsible for gathering all file/timestamp information recursively from our cache directory */
	FAsyncDirectoryReader DirectoryReader;

	/** Temporary array of transactions to be returned to the client through GetOutstandingChanges() */
	TArray<FUpdateCacheTransaction> OutstandingChanges;

	/** Our in-memory view of the cached directory state. */
	FDirectoryState CachedDirectoryState;

	/** Handle to the directory watcher delegate so we can delete it properly */
	FDelegateHandle WatcherDelegate;

	/** True when the cached state we have in memory is more up to date than the serialized file. Enables WriteCache() when true. */
	bool bSavedCacheDirty;
};
