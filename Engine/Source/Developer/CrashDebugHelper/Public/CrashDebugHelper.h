// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

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
public:
	/** The name of the crash dump file */
	FString CrashDumpName;
	/** The engine version of the crash dump build */
	int32 EngineVersion;
	/** The platform of the crash dump build */
	FString PlatformName;
	/** The source control label of the crash dump build */
	FString SourceControlLabel;
};

/** The public interface for the crash dump handler singleton. */
class CRASHDEBUGHELPER_API ICrashDebugHelper
{
protected:
	/** The depot name that the handler is using to sync from */
	FString DepotName;
	/** The local folder where symbol files should be stored */
	FString LocalSymbolStore;
	/** The database that the builder version information is stored in */
	FString DatabaseName;
	/** The catalog that the builder version info is stored in */
	FString DatabaseCatalog;
	/** In the event that a database query for a label fails, use this pattern to search in source control for the label */
	FString SourceControlBuildLabelPattern;
	/** The builtFromCL to use instead of extracting from the minidump */
	int32 BuiltFromCL;

	/** Indicates that the crash handler is ready to do work */
	bool bInitialized;

public:
	/** A platform independent representation of a crash */
	FCrashInfo CrashInfo;

	/** Virtual destructor */
	virtual ~ICrashDebugHelper()
	{
	};

	/**
	 *	Initialize the helper
	 *
	 *	@return	bool		true if successful, false if not
	 */
	virtual bool Init();

	/** Get the database name */
	const FString& GetDatabaseName() const
	{
		return DatabaseName;
	}

	/** Get the catalog name */
	const FString& GetDatabaseCatalog() const
	{
		return DatabaseCatalog;
	}

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
