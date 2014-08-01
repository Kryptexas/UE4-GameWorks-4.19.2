// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	BuildPatchManifest.h: Declares the manifest classes.
=============================================================================*/

#pragma once

// A delegate returning a bool. Used to pass a paused state
DECLARE_DELEGATE_RetVal( bool, FBuildPatchBoolRetDelegate );

typedef TSharedPtr< class FBuildPatchCustomField, ESPMode::ThreadSafe > FBuildPatchCustomFieldPtr;
typedef TSharedRef< class FBuildPatchCustomField, ESPMode::ThreadSafe > FBuildPatchCustomFieldRef;
typedef TSharedPtr< class FBuildPatchAppManifest, ESPMode::ThreadSafe > FBuildPatchAppManifestPtr;
typedef TSharedRef< class FBuildPatchAppManifest, ESPMode::ThreadSafe > FBuildPatchAppManifestRef;

/**
 * An enum type to describe supported features of a certain manifest
 */
namespace EBuildPatchAppManifestVersion
{
	enum Type
	{
		// The original version
		Original = 0,
		// Support for custom fields
		CustomFields,
		// Started storing the version number
		StartStoringVersion,
		// Made after data files where renamed to include the hash value, these chunks now go to ChunksV2
		DataFileRenames,
		// Manifest stores whether build was constructed with chunk or file data
		StoresIfChunkOrFileData,
		// Manifest stores group number for each chunk/file data for reference so that external readers don't need to know how to calculate them
		StoresDataGroupNumbers,
		// Added support for chunk compression, these chunks now go to ChunkV3. NB: Not File Data Compression yet
		ChunkCompressionSupport,
		// Manifest stores product prerequisites info
		StoresPrerequisitesInfo,
		// Manifest stores chunk download sizes
		StoresChunkFileSizes,

		// Always last, signifies the latest version plus 1 to allow initialization simplicity
		LatestPlusOne
	};

	/** @return The last known manifest feature of this codebase. Handy for manifest constructor */
	const Type GetLatestVersion();

	/**
	 * Get the chunk subdirectory for used for a specific manifest version, e.g. Chunks, ChunksV2 etc
	 * @param ManifestVersion     The version of the manifest
	 * @return The subdirectory name that this manifest version will access
	 */
	const FString& GetChunkSubdir( const Type ManifestVersion );

	/**
	 * Get the file data subdirectory for used for a specific manifest version, e.g. Files, FilesV2 etc
	 * @param ManifestVersion     The version of the manifest
	 * @return The subdirectory name that this manifest version will access
	 */
	const FString& GetFileSubdir( const Type ManifestVersion );
};

/**
 * Declare the FBuildPatchCustomField object class
 */
class FBuildPatchCustomField
	: public IManifestField
{
private:
	// Holds the underlying value
	FString CustomValue;

	/**
	 * Hide the default constructor
	 */
	FBuildPatchCustomField(){}
public:
	
	/**
	 * Constructor taking the custom value
	 */
	FBuildPatchCustomField( const FString& Value );

	// START IBuildManifest Interface
	virtual const FString AsString() const override;
	virtual const double AsDouble() const override;
	virtual const int64 AsInteger() const override;
	// END IBuildManifest Interface
};

/**
 * Declare the FBuildPatchFileManifest object class
 */
class FBuildPatchFileManifest
{
public:

	// The filename of the file, local to a build's root
	FString Filename;

	// The Integrity Verification hash
	FSHAHash FileHash;

	// The sequential list of chunk parts this file is made up of
	TArray< FChunkPart > FileChunkParts;

#if PLATFORM_MAC
	// Additional properties for Unix files
	bool bIsUnixExecutable;
	FString SymlinkTarget;
#endif

public:
	// Default constructor
	FBuildPatchFileManifest();
	
	/**
	 * Get the total size for this file
	 * @return		The size in bytes.
	 */
	const int64& GetFileSize();

private:
	// Holds the file size, calculated from the FileChunkParts
	int64 FileSize;
};

/**
 * Declare the FBuildPatchAppManifest object class. This will contain FBuildPatchFileManifest and other build information.
 */
class FBuildPatchAppManifest
	: public IBuildManifest
{
	// Allow access to build processor classes
	friend class FBuildDataGenerator;
	friend class FBuildDataChunkProcessor;
	friend class FBuildDataFileProcessor;
	friend class FBuildPatchInstaller;
public:
	
	/**
	 * Default constructor
	 */
	FBuildPatchAppManifest();

	/**
	 * Basic details constructor
	 */
	FBuildPatchAppManifest( const uint32& InAppID, const FString& AppName );

	/**
	 * Default destructor
	 */
	~FBuildPatchAppManifest();

	// START IBuildManifest Interface
	virtual const uint32&  GetAppID() const override;
	virtual const FString& GetAppName() const override;
	virtual const FString& GetVersionString() const override;
	virtual const FString& GetLaunchExe() const override;
	virtual const FString& GetLaunchCommand() const override;
	virtual const FString& GetPrereqName() const override;
	virtual const FString& GetPrereqPath() const override;
	virtual const FString& GetPrereqArgs() const override;
	virtual const int64 GetDownloadSize() const override;
	virtual const int64 GetBuildSize() const override;
	virtual void GetRemovableFiles(IBuildManifestRef OldManifest, TArray< FString >& RemovableFiles) const override;
	virtual void GetRemovableFiles(const TCHAR* InstallPath, TArray< FString >& RemovableFiles) const override;
	virtual IBuildManifestRef Duplicate() const override;
	virtual const IManifestFieldPtr GetCustomField(const FString& FieldName) const override;
	virtual const IManifestFieldPtr SetCustomField( const FString& FieldName, const FString& Value ) override;
	virtual const IManifestFieldPtr SetCustomField( const FString& FieldName, const double& Value ) override;
	virtual const IManifestFieldPtr SetCustomField( const FString& FieldName, const int64& Value ) override;
	// END IBuildManifest Interface

	/**
	 * Sets up the internal map from a file
	 * @param Filename		The file to load JSON from
	 * @return		True if successful.
	 */
	bool LoadFromFile( FString Filename );

	/**
	 * Sets up the object from the passed in JSON string
	 * @param JSONInput		The JSON string to deserialize from
	 * @return		True if successful.
	 */
	bool DeserializeFromJSON( const FString& JSONInput );

	/**
	 * Saves out the manifest information
	 * @param Filename		The file to save a JSON representation to
	 * @return		True if successful.
	 */
	bool SaveToFile( FString Filename );

	/**
	 * Creates the object in JSON format
	 * @param JSONOutput		A string to receive the JSON representation
	 */
	void SerializeToJSON( FString& JSONOutput );

	/**
	 * Gets the version for this manifest. Useful for manifests that were loaded from JSON.
	 * @return		The highest available feature support
	 */
	const EBuildPatchAppManifestVersion::Type GetManifestVersion() const;

	/**
	 * Provides a list of chunks required to produce the list of given files.
	 * @param FileList			IN		The list of files.
	 * @param RequiredChunks	OUT		The list of chunk GUIDs needed for those files.
	 */
	void GetChunksRequiredForFiles( const TArray< FString >& FileList, TArray< FGuid >& RequiredChunks, bool bAddUnique = true ) const;

	/**
	 * Get the number of times a chunks is referenced in this manifest
	 * @param ChunkGuid		The chunk GUID
	 * @return	The number of references to this chunk
	 */
	const uint32 GetNumberOfChunkReferences( const FGuid& ChunkGuid ) const;

	/**
	 * Returns the size of a particular data file by it's GUID.
	 * @param DataGuid		The GUID for the data
	 * @return		File size.
	 */
	const int64 GetDataSize( const FGuid& DataGuid ) const;

	/**
	 * Returns the total size of all data files in it's list.
	 * @param DataGuids		The GUID array for the data
	 * @return		File size.
	 */
	const int64 GetDataSize( const TArray< FGuid >& DataGuids ) const;

	/**
	 * Returns the size of a particular file in the build
	 * VALID FOR ANY MANIFEST
	 * @param Filename		The file.
	 * @return		File size.
	 */
	const int64 GetFileSize( const FString& Filename ) const;

	/**
	 * Returns the total size of all files in the array
	 * VALID FOR ANY MANIFEST
	 * @param Filenames		The array of files.
	 * @return		Total size of files in array.
	 */
	const int64 GetFileSize( const TArray< FString >& Filenames ) const;

	/**
	 * Returns the number of files in this build.
	 * @return		The number of files.
	 */
	const uint32 GetNumFiles() const;

	/**
	 * Get the list of files described by this manifest
	 * @param Filenames		OUT		Receives the array of files.
	 */
	void GetFileList( TArray< FString >& Filenames ) const;

	/**
	* Get the list of Guids for all files described by this manifest
	* @param DataGuids		OUT		Receives the array of Guids.
	*/
	void GetDataList( TArray< FGuid >& DataGuids ) const;

	/**
	 * Returns the manifest for a particular file in the app, invalid if non-existing
	 * @param Filename	The filename.
	 * @return	The file manifest, or invalid ptr
	 */
	const TSharedPtr< FBuildPatchFileManifest > GetFileManifest( const FString& Filename );

	/**
	 * Gets whether this manifest is made up of file data instead of chunk data
	 * @return	True if the build is made from file data. False if the build is constructed from chunk data.
	 */
	const bool IsFileDataManifest() const;

	/**
	 * Gets the chunk hash for a given chunk
	 * @param ChunkGuid		IN		The guid of the chunk to get hash for
	 * @param OutHash		OUT		Receives the hash value if found
	 * @return	true if we had the hash for this chunk
	 */
	const bool GetChunkHash( const FGuid& ChunkGuid, uint64& OutHash ) const;

	/**
	 * Gets the file hash for given file data
	 * @param FileGuid		IN		The guid of the file data to get hash for
	 * @param OutHash		OUT		Receives the hash value if found
	 * @return	true if we had the hash for this file
	 */
	const bool GetFileDataHash( const FGuid& FileGuid, FSHAHash& OutHash ) const;

	/**
	 * Populates an array of chunks that should be producible from this local build, given the list of chunks needed. Also checks source files exist and match size.
	 * @param InstallDirectory	IN		The directory of the build where chunks would be sourced from.
	 * @param ChunksRequired	IN		A list of chunks that are needed.
	 * @param ChunksAvailable	OUT		A list to receive the chunks that could be constructed locally.
	 */
	void EnumerateProducibleChunks( const FString& InstallDirectory, const TArray< FGuid >& ChunksRequired, TArray< FGuid >& ChunksAvailable ) const;

	/**
	 * Verifies a local directory structure against a given manifest.
	 * NOTE: This function is blocking and will not return until finished. Don't run on main thread.
	 * @param VerifyDirectory		IN		The directory to analyze.
	 * @param OutDatedFiles			OUT		The array of files that do not match or are locally missing.
	 * @param ProgressDelegate		IN		Delegate to receive progress updates in the form of a float range 0.0f to 1.0f
	 * @param ShouldPauseDelegate	IN		Delegate that returns a bool, which if true will pause the process
	 * @param TimeSpentPaused		OUT		The amount of time we were paused for, in seconds.
	 * @return		true if no file errors occurred AND the verification was successful
	 */
	bool VerifyAgainstDirectory( const FString& VerifyDirectory, TArray < FString >& OutDatedFiles, FBuildPatchFloatDelegate ProgressDelegate, FBuildPatchBoolRetDelegate ShouldPauseDelegate, double& TimeSpentPaused );

	/**
	 * Gets a list of files that were installed with the Old Manifest, but no longer required by the New Manifest.
	 * @param OldManifest		IN		The Build Manifest that is currently installed
	 * @param NewManifest		IN		The Build Manifest that is being patched to
	 * @param RemovableFiles	OUT		A list to receive the files that may be removed.
	 */
	static void GetRemovableFiles( FBuildPatchAppManifestRef OldManifest, FBuildPatchAppManifestRef NewManifest, TArray< FString >& RemovableFiles );

	/**
	 * Gets a list of files that have different hash values in the New Manifest, to those in the Old Manifest.
	 * @param OldManifest		IN		The Build Manifest that is currently installed. Shared Ptr - Can be invalid.
	 * @param NewManifest		IN		The Build Manifest that is being patched to. Shared Ref - Implicitly valid.
	 * @param InstallDirectory	IN		The Build installation directory, so that it can be checked for missing files.
	 * @param OutDatedFiles		OUT		The array of files that do not match or are new.
	 */
	static void GetOutdatedFiles( FBuildPatchAppManifestPtr OldManifest, FBuildPatchAppManifestRef NewManifest, const FString& InstallDirectory, TArray< FString >& OutDatedFiles );

	/**
	 * Check a single file to see if it will be effected by patching
	 * @param OldManifest		The Build Manifest that is currently installed. Shared Ref - Implicitly valid.
	 * @param NewManifest		The Build Manifest that is being patched to. Shared Ref - Implicitly valid.
	 * @param Filename			The Build installation directory, so that it can be checked for missing files.
	 */
	static bool IsFileOutdated( FBuildPatchAppManifestRef OldManifest, FBuildPatchAppManifestRef NewManifest, const FString& Filename );

	/**
	 * Populates a map of FFileChunkParts referring to the chunks in the given array, that should be accessible locally.
	 * @param ChunksRequired		IN		A list of chunks that are needed.
	 * @param ChunkPartsAvailable	OUT		A map to receive the FFileChunkParts keyed by chunk guid.
	 */
	void EnumerateChunkPartInventory( const TArray< FGuid >& ChunksRequired, TMap< FGuid, TArray< FFileChunkPart > >& ChunkPartsAvailable ) const;

private:

	/**
	 * Destroys any memory we have allocated and clears out ready for generation of a new manifest
	 */
	void DestroyData();

private:

	/**
	 * Holds the version support for this manifest.
	 */
	EBuildPatchAppManifestVersion::Type ManifestFileVersion;

	/**
	 * Stores whether the manifest was produced from a file data build instead of chunk data
	 */
	bool bIsFileData;

	/**
	 * Holds the ID of this app.
	 */
	uint32 AppID;

	/**
	 * Holds the name of the app this Build is for
	 */
	FString AppNameString;

	/**
	 * Holds the version string of this Build
	 */
	FString BuildVersionString;

	/**
	 * Holds the app local exe path
	 */
	FString LaunchExeString;

	/**
	 * Holds the name of prerequisites installer
	 */
	FString PrereqNameString;

	/**
	 * Holds the exe path of the prerequisites installer
	 */
	FString PrereqPathString;

	/**
	 * Holds commandline arguments for the prerequisites installer
	 */
	FString PrereqArgsString;
	
	/**
	 * Holds the app launch command
	 */
	FString LaunchCommandString;

	/**
	 * References filename by file data GUID
	 */
	TMap< FGuid, FString > FileNameReference;

	/**
	 * Holds the list of files and their generated file manifest
	 */
	TMap< FString, TSharedPtr< FBuildPatchFileManifest > > FileManifestList;

	/**
	 * Holds the list of chunks with their rolling chunk hash, does not apply to file data.
	 */
	TMap< FGuid, uint64 > ChunkHashList;

	/**
	 * Holds the filesize for each chunk, does not apply to file data.
	 */
	TMap< FGuid, int64 > ChunkFilesizeList;

	/**
	 * Holds which group the chunk (containing chunk or file data) is placed into for file system optimisations
	 */
	TMap< FGuid, uint8 > DataGroupList;

	/**
	 * Holds the list of custom fields
	 */
	TMap< FString, FString > CustomFields;
};
