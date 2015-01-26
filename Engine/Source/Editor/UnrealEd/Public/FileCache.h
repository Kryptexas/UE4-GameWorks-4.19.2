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

	/** Copyable only by FFileCache*/
	FUpdateCacheTransaction(const FUpdateCacheTransaction& In) = default;
	FUpdateCacheTransaction& operator=(const FUpdateCacheTransaction& In) = default;
};

/** Configuration structure required to construct a FFileCache */
struct FFileCacheConfig
{
	/** String specifying the directory on disk that the cache should reflect */
	FString Directory;

	/** String specifying the file that the cache should be saved to */
	FString CacheFile;

	/**
	 * Strings specifying extensions to include/exclude in the cache (or empty to include everything).
	 * Specified as a list of semi-colon separated extensions (eg "png;jpg;gif;").
	 */
	FString IncludeExtensions, ExcludeExtensions;
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

	/** Force a scan the directory to detect any changes in the cache. */
	void Scan();

	/** Write out the cached file, if we have any changes to write */
	void WriteCache();

	/**
	 * Return a transaction to the cache for completion. Will update the cached state with the change
	 * described in the transaction, and mark the cache as needing to be saved.
	 */
	void CompleteTransaction(FUpdateCacheTransaction&& Transaction);

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
	
	/** Walk the directory and return all the file information we find pertaining to all applicable files */
	FDirectoryState WalkDirectory() const;

	/** Read the cache file data and return the contents */
	TOptional<FDirectoryState> ReadCache() const;

private:

	/** Temporary array of transactions to be returned to the client through GetOutstandingChanges() */
	TArray<FUpdateCacheTransaction> OutstandingChanges;

	/** Our in-memory view of the cached directory state. */
	FDirectoryState CachedDirectoryState;

	/** Configuration settings applied on construction */
	FFileCacheConfig Config;

	/** Handle to the directory watcher delegate so we can delete it properly */
	FDelegateHandle WatcherDelegate;

	/** True when the cached state we have in memory is more up to date than the serialized file. Enables WriteCache() when true. */
	bool bSavedCacheDirty;
};
