// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

//#define DO_LOCAL_TESTING 1

struct FPDBCacheEntry;
typedef TSharedRef<FPDBCacheEntry> FPDBCacheEntryRef;
typedef TSharedPtr<FPDBCacheEntry> FPDBCacheEntryPtr;

enum EProcessorArchitecture
{
	PA_UNKNOWN,
	PA_ARM,
	PA_X86,
	PA_X64,
};

/** 
 * Details of a module from a crash dump
 */
class FCrashModuleInfo
{
public:
	FString Report;

	FString Name;
	FString Extension;
	uint64 BaseOfImage;
	uint32 SizeOfImage;
	uint16 Major;
	uint16 Minor;
	uint16 Build;
	uint16 Revision;

	FCrashModuleInfo()
		: BaseOfImage( 0 )
		, SizeOfImage( 0 )
		, Major( 0 )
		, Minor( 0 )
		, Build( 0 )
		, Revision( 0 )
	{
	}
};

/** 
 * Details about a thread from a crash dump
 */
class FCrashThreadInfo
{
public:
	FString Report;

	uint32 ThreadId;
	uint32 SuspendCount;

	TArray<uint64> CallStack;

	FCrashThreadInfo()
		: ThreadId( 0 )
		, SuspendCount( 0 )
	{
	}

	~FCrashThreadInfo()
	{
	}
};

/** 
 * Details about the exception in the crash dump
 */
class FCrashExceptionInfo
{
public:
	FString Report;

	uint32 ProcessId;
	uint32 ThreadId;
	uint32 Code;
	FString ExceptionString;

	TArray<FString> CallStackString;

	FCrashExceptionInfo()
		: ProcessId( 0 )
		, ThreadId( 0 )
		, Code( 0 )
	{
	}

	~FCrashExceptionInfo()
	{
	}
};

/** 
 * Details about the system the crash dump occurred on
 */
class FCrashSystemInfo
{
public:
	FString Report;

	EProcessorArchitecture ProcessorArchitecture;
	uint32 ProcessorCount;

	uint16 OSMajor;
	uint16 OSMinor;
	uint16 OSBuild;
	uint16 OSRevision;

	FCrashSystemInfo()
		: ProcessorArchitecture( PA_UNKNOWN )
		, ProcessorCount( 0 )
		, OSMajor( 0 )
		, OSMinor( 0 )
		, OSBuild( 0 )
		, OSRevision( 0 )
	{
	}
};

/** A platform independent representation of a crash */
class CRASHDEBUGHELPER_API FCrashInfo
{
public:
	FString Report;

	TArray<FString> ModuleNames;
	int32 ChangelistBuiltFrom;
	FString LabelName;

	FString SourceFile;
	uint64 SourceLineNumber;
	TArray<FString> SourceContext;

	FCrashSystemInfo SystemInfo;
	FCrashExceptionInfo Exception;
	TArray<FCrashThreadInfo> Threads;
	TArray<FCrashModuleInfo> Modules;

	/** Shared pointer to the PDB Cache entry, if valid contains all information about synced PDBs. */
	FPDBCacheEntryPtr PDBCacheEntry;

	FCrashInfo()
		: ChangelistBuiltFrom( -1 )
		, SourceLineNumber( 0 )
	{
	}

	~FCrashInfo()
	{
	}

	/** 
	 * Generate a report for the crash in the requested path
	 */
	void GenerateReport( const FString& DiagnosticsPath );

	/** 
	 * Handle logging
	 */
	void Log( FString Line );

private:
	const TCHAR* GetProcessorArchitecture( EProcessorArchitecture PA );
	int64 StringSize( const ANSICHAR* Line );
	void WriteLine( FArchive* ReportFile, const ANSICHAR* Line = NULL );
};

/** Helper structure for tracking crash debug information */
struct CRASHDEBUGHELPER_API FCrashDebugInfo
{
	/** The name of the crash dump file */
	FString CrashDumpName;
	/** The engine version of the crash dump build */
	int32 EngineVersion;
	/** The platform of the crash dump build */
	FString PlatformName;
	/** The source control label of the crash dump build */
	FString SourceControlLabel;
};

/** Helper struct that holds various information about one PDB Cache entry. */
struct FPDBCacheEntry
{
	/** Initialization constructor. */
	FPDBCacheEntry( const FString& InLabel, const int32 InSizeGB )
		: Label( InLabel )
		, SizeGB( InSizeGB )
	{}

	void SetLastAccessTimeToNow()
	{
		LastAccessTime = FDateTime::UtcNow();
	}

	/** Paths to files associated with this PDB Cache entry. */
	TArray<FString> Files;

	/** P4 Label associated with this PDB Cache entry. */
	const FString Label;
	
	/** Last access time, changed every time this PDB cache entry is used. */
	FDateTime LastAccessTime;

	/** Size of the cache entry, in GBs. Rounded-up. */
	const int32 SizeGB;
};

struct FPDBCacheEntryByAccessTime
{
	FORCEINLINE bool operator()( const FPDBCacheEntryRef& A, const FPDBCacheEntryRef& B ) const
	{
		return A->LastAccessTime.GetTicks() < B->LastAccessTime.GetTicks();
	}
};

/** Implements PDB cache. */
struct FPDBCache
{
	//friend class ICrashDebugHelper;
protected:

	// Defaults for the PDB cache.
	enum
	{
		/** Size of the PDB cache, in GBs. */
		PDB_CACHE_SIZE_GB = 128,
		MIN_FREESPACE_GB = 64,
		/** Age of file when it should be deleted from the PDB cache. */
		DAYS_TO_DELETE_UNUSED_FILES = 14,

		/**
		*	Number of iterations inside the CleanPDBCache method.
		*	Mostly to verify that MinDiskFreeSpaceGB requirement is met.
		*/
		CLEAN_PDBCACHE_NUM_ITERATIONS = 2,

		/** Number of bytes per one gigabyte. */
		NUM_BYTES_PER_GB = 1024 * 1024 * 1024
	};

	/** Dummy file used to read/set the file timestamp. */
	static const TCHAR* PDBTimeStampFile;

	/** Map of the PDB Cache entries. */
	TMap<FString, FPDBCacheEntryRef> PDBCacheEntries;

	/** Path to the folder where the PDB cache is stored. */
	FString PDBCachePath;

	/** The builtFromCL to use instead of extracting from the minidump */
	int32 BuiltFromCL;

	/** Age of file when it should be deleted from the PDB cache. */
	int32 DaysToDeleteUnusedFilesFromPDBCache;

	/** Size of the PDB cache, in GBs. */
	int32 PDBCacheSizeGB;

	/**
	*	Minimum disk free space that should be available on the disk where the PDB cache is stored, in GBs.
	*	Minidump diagnostics runs usually on the same drive as the crash reports drive, so we need to leave some space for the crash receiver.
	*	If there is not enough disk free space, we will run the clean-up process.
	*/
	int32 MinDiskFreeSpaceGB;

	/** Whether to use the PDB cache. */
	bool bUsePDBCache;

public:
	/** Default constructor. */
	FPDBCache()
		: BuiltFromCL( 0 )
		, DaysToDeleteUnusedFilesFromPDBCache( DAYS_TO_DELETE_UNUSED_FILES )
		, PDBCacheSizeGB( PDB_CACHE_SIZE_GB )
		, MinDiskFreeSpaceGB( MIN_FREESPACE_GB )
		, bUsePDBCache( false )
	{}

	/** Basic initialization, reading config etc.. */
	void Init();

	/**
	 * @return whether to use the PDB cache.
	 */
	bool UsePDBCache() const
	{
		return bUsePDBCache;
	}
	
	/**
	 * @return true, if the PDB Cache contains the specified label.
	 */
	bool ContainsPDBCacheEntry( const FString& InLabel ) const
	{
		return PDBCacheEntries.Contains( CleanLabelName( InLabel ) );
	}

	/**
	 *	Touches a PDB Cache entry by updating the timestamp.
	 */
	void TouchPDBCacheEntry( const FString& InLabel );

	/**
	 * @return a PDB Cache entry for the specified label.
	 */
	FPDBCacheEntryRef FindPDBCacheEntry( const FString& InLabel )
	{
		return PDBCacheEntries.FindChecked( CleanLabelName( InLabel ) );
	}

	/**
	 *	Creates a new PDB Cache entry, initializes it and adds to the database.
	 */
	FPDBCacheEntryRef CreateAndAddPDBCacheEntry( const FString& OriginalLabelName, const FString& DepotName, const TArray<FString>& SyncedFiles );

protected:
	/** Initializes the PDB Cache. */
	void InitializePDBCache();

	/**
	 * @return the size of the PDB cache entry, in GBs.
	 */
	int32 GetPDBCacheEntrySizeGB( const FString& InLabel ) const
	{
		return PDBCacheEntries.FindChecked( InLabel )->SizeGB;
	}

	/**
	 * @return the size of the PDB cache directory, in GBs.
	 */
	int32 GetPDBCacheSizeGB() const
	{
		int32 Result = 0;
		if( bUsePDBCache )
		{
			for( const auto& It : PDBCacheEntries )
			{
				Result += It.Value->SizeGB;
			}
		}
		return Result;
	}

	/**
	 * Cleans the PDB Cache.
	 *
	 * @param DaysToKeep - removes all PDB Cache entries that are older than this value
	 * @param NumberOfGBsToBeCleaned - if specifies, will try to remove as many PDB Cache entries as needed
	 * to free disk space specified by this value
	 *
	 */
	void CleanPDBCache( int32 DaysToKeep, int32 NumberOfGBsToBeCleaned = 0 );


	/**
	 *	Reads an existing PDB Cache entry.
	 */
	FPDBCacheEntryRef ReadPDBCacheEntry( const FString& InLabel );

	/**
	 *	Sort PDB Cache entries by last access time.
	 */
	void SortPDBCache()
	{
		PDBCacheEntries.ValueSort( FPDBCacheEntryByAccessTime() );
	}

	/**
	 *	Removes a PDB Cache entry from the database.
	 *	Also removes all external files associated with this PDB Cache entry.
	 */
	void RemovePDBCacheEntry( const FString& InLabel );

	/** Replaces all / chars with _ for the specified label name. */
	FString CleanLabelName( const FString& InLabel ) const
	{
		return InLabel.Replace( TEXT( "/" ), TEXT( "_" ) );
	}
};

/** The public interface for the crash dump handler singleton. */
class CRASHDEBUGHELPER_API ICrashDebugHelper
{
protected:
	/** The depot name that the handler is using to sync from */
	FString DepotName;
	/** The local folder where symbol files should be stored */
	FString LocalSymbolStore;
	/** In the event that a database query for a label fails, use this pattern to search in source control for the label */
	FString SourceControlBuildLabelPattern;
	/** The builtFromCL to use instead of extracting from the minidump */
	int32 BuiltFromCL;
	
	/** Indicates that the crash handler is ready to do work */
	bool bInitialized;

	/** Instance of the PDB Cache. */
	FPDBCache PDBCache;

public:
	/** A platform independent representation of a crash */
	FCrashInfo CrashInfo;
	
	/** Virtual destructor */
	virtual ~ICrashDebugHelper()
	{}

	/**
	 *	Initialize the helper
	 *
	 *	@return	bool		true if successful, false if not
	 */
	virtual bool Init();

	/** Get the depot name */
	const FString& GetDepotName() const
	{
		return DepotName;
	}

	/** Set the depot name */
	void SetDepotName(const FString& InDepotName)
	{
		DepotName = InDepotName;
	}

	/**
	 *	Parse the given crash dump, determining EngineVersion of the build that produced it - if possible. 
	 *
	 *	@param	InCrashDumpName		The crash dump file to process
	 *	@param	OutCrashDebugInfo	The crash dump info extracted from the file
	 *
	 *	@return	bool				true if successful, false if not
	 */
	virtual bool ParseCrashDump(const FString& InCrashDumpName, FCrashDebugInfo& OutCrashDebugInfo)
	{
		return false;
	}

	/**
	 *	Parse the given crash dump, and generate a report. 
	 *
	 *	@param	InCrashDumpName		The crash dump file to process
	 *
	 *	@return	bool				true if successful, false if not
	 */
	virtual bool CreateMinidumpDiagnosticReport( const FString& InCrashDumpName )
	{
		return false;
	}

	/**
	 *	Get the source control label for the given engine version
	 *
	 *	@param	InEngineVersion		The engine version to retrieve the label for
	 *
	 *	@return	FString				The label containing that engine version build, empty if not found
	 */
	virtual FString GetLabelFromEngineVersion(int32 InEngineVersion)
	{
		return RetrieveBuildLabel(InEngineVersion, -1);
	}

	/**
	 *	Get the source control label for the given changelist number
	 *
	 *	@param	InChangelistNumber	The changelist number to retrieve the label for
	 *
	 *	@return	FString				The label containing that changelist number build, empty if not found
	 */
	virtual FString GetLabelFromChangelistNumber(int32 InChangelistNumber)
	{
		return RetrieveBuildLabel(-1, InChangelistNumber);
	}

	/**
	 *	Sync the required files from source control for debugging
	 *
	 *	@param	InLabel			The label to sync files from
	 *	@param	InPlatform		The platform to sync files for
	 *
	 *	@return bool			true if successful, false if not
	 */
	virtual bool SyncRequiredFilesForDebuggingFromLabel(const FString& InLabel, const FString& InPlatform);

	/**
	 *	Sync the required files from source control for debugging
	 *	NOTE: Currently this will only handle changelists of 'valid' builds from the build machine
	 *			Ie the changelist must map to a build label.
	 *
	 *	@param	InChangelist	The changelist to sync files from
	 *	@param	InPlatform		The platform to sync files for
	 *
	 *	@return bool			true if successful, false if not
	 */
	virtual bool SyncRequiredFilesForDebuggingFromChangelist(int32 InChangelistNumber, const FString& InPlatform);

	/**
	 *	Sync required files and launch the debugger for the crash dump.
	 *	Will call parse crash dump if it has not been called already.
	 *
	 *	@params	InCrashDumpName		The name of the crash dump file to debug
	 *
	 *	@return	bool				true if successful, false if not
	 */
	virtual bool SyncAndDebugCrashDump(const FString& InCrashDumpName)
	{
		return false;
	}

	/**
	 *	Sync the binaries and associated pdbs to the requested label.
	 *
	 *	@params	InLabel			The name of the crash dump file to debug
	 *	@params	InModuleNames	An array of modules to sync
	 *
	 *	@return	bool		true if successful, false if not
	 */
	virtual bool SyncModules( FCrashInfo* CrashInfo );

	/**
	 *	Sync a single source file to the requested label.
	 *
	 *	@params	CrashInfo	A description of a crash
	 *
	 *	@return	bool		true if successful, false if not
	 */
	virtual bool SyncSourceFile( FCrashInfo* CrashInfo );

	/**
	 *	Extract lines from a source file, and add to the crash report.
	 *
	 *	@params	CrashInfo	A description of a crash
	 */
	virtual void AddSourceToReport( FCrashInfo* CrashInfo );

	/**
	 *	Extract annotated lines from a source file stored in Perforce, and add to the crash report.
	 *
	 *	@params	CrashInfo	A description of a crash
	 */
	virtual bool AddAnnotatedSourceToReport( FCrashInfo* CrashInfo );

protected:
	/**
	 *	Retrieve the build label for the given engine version or changelist number.
	 *
	 *	@param	InEngineVersion		The engine version to retrieve the label for. If -1, then will use InChangelistNumber
	 *	@param	InChangelistNumber	The changelist number to retrieve the label for
	 *	@return	FString				The label if successful, empty if it wasn't found or another error
	 */
	virtual FString RetrieveBuildLabel(int32 InEngineVersion, int32 InChangelistNumber);

	bool ReadSourceFile( const TCHAR* InFilename, TArray<FString>& OutStrings );

public:
	bool InitSourceControl(bool bShowLogin);
	void ShutdownSourceControl();
};
