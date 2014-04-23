// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "ModuleInterface.h"

#define VSACCESSOR_HAS_DTE !WITH_VSEXPRESS

/**
 * Delegate type notifying that VS has launched
 */
DECLARE_EVENT( FVSAccessorModule, FLaunchingVS );
DECLARE_EVENT_OneParam( FVSAccessorModule, FDoneLaunchingVS, const bool );
DECLARE_EVENT_OneParam( FVSAccessorModule, FLaunchVSDeferred, const FString& );
DECLARE_EVENT_OneParam( FVSAccessorModule, FOpenFileFailed, const FString& );

/**
 * Module for accessing Visual Studio from the editor/in-game
 */
class FVSAccessorModule : public IModuleInterface, FSelfRegisteringExec
{
public:

	/** Struct representing identifying information about Visual Studio versions */
	struct VisualStudioLocation
	{
		FString ExecutablePath;
#if VSACCESSOR_HAS_DTE
		FString ROTMoniker;
#endif
	};

	virtual void StartupModule();
	virtual void ShutdownModule();

	/** Checks to see if we can run visual studio, also outputs the executable path if it's needed */
	virtual bool CanRunVisualStudio( FString& OutPath ) const;

	/** Opens the module solution. */
	virtual bool OpenVisualStudioSolution();

	/** Opens multiple files in the correct running instance of Visual Studio. */
	virtual bool OpenVisualStudioFiles(const TArray<FString>& FullPaths);

	/** Opens a file in the correct running instance of Visual Studio at a line and optionally to a column. */
	virtual bool OpenVisualStudioFileAtLine(const FString& FullPath, int32 LineNumber, int32 ColumnNumber = 0);
	
	/**
	 * Saves all open visual studio documents if they need to be saved.
	 * Will block if there is any read-only files open that need to be saved.
	 * This behavior is identical to the default VS behavior when compiling.
	 */
	virtual bool SaveAllOpenVisualStudioDocuments();

	/**
	 * Gets the Event that is broad casted when attempting to launch visual studio
	 */
	virtual FLaunchingVS& OnLaunchingVS() { return LaunchingVS; }

	/**
	 * Gets the Event that is broad casted when an attempted launch of visual studio was successful or failed
	 */
	virtual FDoneLaunchingVS& OnDoneLaunchingVS() { return DoneLaunchingVS; }

	/**
	 * Gets a delegate to be invoked when the the open command needs to be deferred
	 *
	 * @return The delegate.
	 */
	virtual FLaunchVSDeferred& OnLaunchVSDeferred() { return LaunchVSDeferred; }

	/**
	 * Gets the Event that is broadcast when an attempt to load a file through Visual Studio failed
	 */
	virtual FOpenFileFailed& OnOpenFileFailed() { return OpenFileFailed; }

private:

	struct FileOpenRequest
	{
		FString FullPath;
		int32 LineNumber;
		int32 ColumnNumber;

		FileOpenRequest()
			: LineNumber(0)
			, ColumnNumber(0)
		{
		}

		FileOpenRequest(const FString& FullPath, int32 LineNumber, int32 ColumnNumber)
			: FullPath(FullPath)
			, LineNumber(LineNumber)
			, ColumnNumber(ColumnNumber)
		{
		}
	};

	// Begin Exec Interface
	virtual bool Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar ) OVERRIDE;
	// End Exec Interface

	/** Run an instance of visual studio instance if possible. */
	bool RunVisualStudioAndOpenSolution() const;

	/** Opens a file in the correct running instance of Visual Studio at a line and optionally to a column. */
	bool OpenVisualStudioFileAtLineInternal(const FString& FullPath, int32 LineNumber, int32 ColumnNumber = 0);

	/** Opens multiple files in the correct running instance of Visual Studio. */
	bool OpenVisualStudioFilesInternal(const TArray<FileOpenRequest>& Requests);
		
	/** An instance of VS it attempting to be opened */
	void VSLaunchStarted();

	/** 
	 * An instance of VS has finished processing 
	 * 
	 * @param	bSuccess	Whether VS has finished successfully or not
	 */
	void VSLaunchFinished( bool bSuccess );

	/** 
	 * Are we trying to launch an instance of VS
	 * 
	 * @return	true if we're in the middle of a VS instance being launched
	 */
	bool IsVSLaunchInProgress() const { return( ( VSLaunchTime != 0.0) ? true : false ); }

	/** 
	 * Add a new version of Visual Studio to the supported locations array
	 * 
	 * @param	MajorVersion	The major version number of Visual Studio (eg, 11 for VS2012, 12 for VS2013)
	 * @param	bAllowExpress	true to also add the express version of Visual Studio to the list of locations
	 */
	void AddVisualStudioVersion(const int MajorVersion, const bool bAllowExpress = true);

	/** 
	 * Run a new instance Visual Studio, optionally opening the provided solution and list of files
	 * 
	 * @param	ExecutablePath	Path to the Visual Studio executable to open
	 * @param	SolutionPath	Path to the solution to open, or an empty string to open no solution
	 * @param	Requests		Array of files to open, or null to open no files
	 */
	bool RunVisualStudioAndOpenSolutionAndFiles(const FString& ExecutablePath, const FString& SolutionPath, const TArray<FileOpenRequest>* const Requests) const;

#if VSACCESSOR_HAS_DTE
	/** DTE specific implementations */
	bool OpenVisualStudioSolutionViaDTE();
	bool OpenVisualStudioFilesInternalViaDTE(const TArray<FileOpenRequest>& Requests, bool& bWasDeferred);
#endif
	/** Fallback (non-DTE) implementations */
	bool OpenVisualStudioSolutionViaProcess();
	bool OpenVisualStudioFilesInternalViaProcess(const TArray<FileOpenRequest>& Requests);

private:

	/** The versions of VS we support, in preference order */
	TArray<VisualStudioLocation> Locations;

	/** Holds the delegate to inform of a VS launch */
	FLaunchingVS LaunchingVS;

	FDoneLaunchingVS DoneLaunchingVS;

	/** Event that is broadcast when an attempt to open a file has failed */
	FOpenFileFailed OpenFileFailed;

	/** Holds the delegate to inform deferred open of VS */
	FLaunchVSDeferred LaunchVSDeferred;

	/** String storing the solution path obtained from the module manager to avoid having to use it on a thread */
	FString SolutionPath;

	/** If !0 it represents the time at which the a VS instance was opened */
	double	VSLaunchTime;
};
