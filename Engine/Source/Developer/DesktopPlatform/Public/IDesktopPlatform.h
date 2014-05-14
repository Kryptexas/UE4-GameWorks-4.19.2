// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

namespace EFileDialogFlags
{
	enum Type
	{
		None					= 0x00, // No flags
		Multiple				= 0x01  // Allow multiple file selections
	};
}

namespace EFontImportFlags
{
	enum Type
	{
		None = 0x0,						// No flags
		EnableAntialiasing = 0x1,		// Whether the font should be antialiased or not.  Usually you should leave this enabled.
		EnableBold = 0x2,				// Whether the font should be generated in bold or not
		EnableItalic = 0x4,				// Whether the font should be generated in italics or not
		EnableUnderline = 0x8,			// Whether the font should be generated with an underline or not
		AlphaOnly = 0x10,				// Forces PF_G8 and only maintains Alpha value and discards color
		CreatePrintableOnly = 0x20,		// Skips generation of glyphs for any characters that are not considered 'printable'
		IncludeASCIIRange = 0x40,		// When specifying a range of characters and this is enabled, forces ASCII characters (0 thru 255) to be included as well
		EnableDropShadow = 0x80,		// Enables a very simple, 1-pixel, black colored drop shadow for the generated font
		EnableLegacyMode = 0x100,		// Enables legacy font import mode.  This results in lower quality antialiasing and larger glyph bounds, but may be useful when debugging problems
		UseDistanceFieldAlpha = 0x200	// Alpha channel of the font textures will store a distance field instead of a color mask
	};
};


/**
 * When constructed leaves system wide modal mode (all windows disabled except for the OS modal window)
 * When destructed leaves this mode
 */
class FScopedSystemModalMode
{
public:
	FScopedSystemModalMode()
	{
#if WITH_EDITOR
		FCoreDelegates::PreModal.Broadcast();
#endif
	}

	~FScopedSystemModalMode()
	{
#if WITH_EDITOR
		FCoreDelegates::PostModal.Broadcast();
#endif
	}
};


class IDesktopPlatform
{
public:
	/** Virtual destructor */
	virtual ~IDesktopPlatform() {}

	/** 
	 * Opens the "open file" dialog for the platform
	 *
	 * @param ParentWindowHandle		The native handle to the parent window for this dialog
	 * @param DialogTitle				The text for the title of the dialog window
	 * @param DefaultPath				The path where the file dialog will open initially
	 * @param DefaultFile				The file that the dialog will select initially
	 * @param Flags						Details about the dialog. See EFileDialogFlags.
	 * @param FileTypes					The type filters to show in the dialog. This string should be a "|" delimited list of (Description|Extensionlist) pairs. Extensionlists are ";" delimited.
	 * @param OutFilenames				The filenames that were selected in the dialog
	 * @return true if files were successfully selected
	 */
	virtual bool OpenFileDialog(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes, uint32 Flags, TArray<FString>& OutFilenames) = 0;

	/** 
	 * Opens the "save file" dialog for the platform
	 *
	 * @param ParentWindowHandle		The native handle to the parent window for this dialog
	 * @param DialogTitle				The text for the title of the dialog window
	 * @param DefaultPath				The path where the file dialog will open initially
	 * @param DefaultFile				The file that the dialog will select initially
	 * @param Flags						Details about the dialog. See EFileDialogFlags.
	 * @param FileTypes					The type filters to show in the dialog. This string should be a "|" delimited list of (Description|Extensionlist) pairs. Extensionlists are ";" delimited.
	 * @param OutFilenames				The filenames that were selected in the dialog
	 * @return true if files were successfully selected
	 */
	virtual bool SaveFileDialog(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes, uint32 Flags, TArray<FString>& OutFilenames) = 0;

	/** 
	 * Opens the "choose folder" dialog for the platform
	 *
	 * @param ParentWindowHandle		The native handle to the parent window for this dialog
	 * @param DialogTitle				The text for the title of the dialog window
	 * @param DefaultPath				The path where the file dialog will open initially
	 * @param OutFolderName				The foldername that was selected in the dialog
	 * @return true if folder choice was successfully selected
	 */
	virtual bool OpenDirectoryDialog(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, FString& OutFolderName) = 0;

	/** 
	 * Opens the "choose font" dialog for the platform
	 *
	 * @param ParentWindowHandle		The native handle to the parent window for this dialog
	 * @param OutFontName				The name of the font
	 * @param OutHeight					The height of the font
	 * @param OutFlags					Any special flags the font has been tagged with
	 * @return true if font choice was successfully selected
	 */
	virtual bool OpenFontDialog(const void* ParentWindowHandle, FString& OutFontName, float& OutHeight, EFontImportFlags::Type& OutFlags) = 0;

	/**
	 * Opens the marketplace user interface.
	 *
	 * @param Install					Whether to install the marketplace if it is missing.
	 * @return true if the marketplace was opened, false if it is not installed or could not be installed/opened.
	 */
	virtual bool OpenLauncher(bool Install, FString CommandLineParams ) = 0;

	/**
	* Gets the identifier for the currently executing engine installation
	*
	* @return	Identifier for the current engine installation. Empty string if it isn't registered.
	*/
	virtual FString GetCurrentEngineIdentifier() = 0;

	/**
	* Registers a directory as containing an engine installation
	*
	* @param	RootDir				Root directory for the engine installation
	* @param	OutIdentifier		Identifier which is assigned to the engine
	* @return true if the directory was added. OutIdentifier will be set to the assigned identifier.
	*/
	virtual bool RegisterEngineInstallation(const FString &RootDir, FString &OutIdentifier) = 0;

	/**
	* Enumerates all the registered engine installations.
	*
	* @param	OutInstallations	Array which is filled in with identifier/root-directory pairs for all known installations. Identifiers are typically
	*								version strings for canonical UE4 releases or GUID strings for GitHub releases.
	*/
	virtual void EnumerateEngineInstallations(TMap<FString, FString> &OutInstallations) = 0;

	/**
	* Enumerates all the registered binary engine installations.
	*
	* @param	OutInstallations	Array which is filled in with identifier/root-directory pairs for all known binary installations.
	*/
	virtual void EnumerateLauncherEngineInstallations(TMap<FString, FString> &OutInstallations) = 0;

	/**
	* Returns the identifier for the engine with the given root directory.
	*
	* @param	RootDirName			Root directory for the engine.
	* @param	OutIdentifier		Identifier used to refer to this installation.
	*/
	virtual bool GetEngineRootDirFromIdentifier(const FString &Identifier, FString &OutRootDir) = 0;

	/**
	* Returns the identifier for the engine with the given root directory.
	*
	* @param	RootDirName			Root directory for the engine.
	* @param	OutIdentifier		Identifier used to refer to this installation.
	*/
	virtual bool GetEngineIdentifierFromRootDir(const FString &RootDir, FString &OutIdentifier) = 0;

	/**
	* Gets the identifier for the default engine. This will be the newest installed engine.
	*
	* @param	OutIdentifier	String to hold to the default engine's identifier
	* @return	true if OutIdentifier is valid.
	*/
	virtual bool GetDefaultEngineIdentifier(FString &OutIdentifier) = 0;

	/**
	* Compares two identifiers and checks whether the first is preferred to the second.
	*
	* @param	Identifier		First identifier
	* @param	OtherIdentifier	Second identifier
	* @return	true if Identifier is preferred over OtherIdentifier
	*/
	virtual bool IsPreferredEngineIdentifier(const FString &Identifier, const FString &OtherIdentifier) = 0;

	/**
	* Gets the root directory for the default engine installation.
	*
	* @param	OutRootDir	String to hold to the default engine's root directory
	* @return	true if OutRootDir is valid
	*/
	virtual bool GetDefaultEngineRootDir(FString &OutRootDir) = 0;

	/**
	* Checks if the given engine identifier is for an stock engine release.
	*
	* @param	Identifier			Engine identifier to check
	* @return	true if the identifier is for a binary engine release
	*/
	virtual bool IsStockEngineRelease(const FString &Identifier) = 0;

	/**
	* Tests whether an engine installation is a source distribution.
	*
	* @return	true if the engine contains source.
	*/
	virtual bool IsSourceDistribution(const FString &RootDir) = 0;

	/**
	* Tests whether an engine installation is a perforce build.
	*
	* @return	true if the engine is a perforce build.
	*/
	virtual bool IsPerforceBuild(const FString &RootDir) = 0;

	/**
	* Tests whether a root directory is a valid Unreal Engine installation
	*
	* @return	true if the engine is a valid installation
	*/
	virtual bool IsValidRootDirectory(const FString &RootDir) = 0;

	/**
	* Checks that the current file associations are correct.
	*
	* @return	true if file associations are up to date.
	*/
	virtual bool VerifyFileAssociations() = 0;

	/**
	* Updates file associations.
	*
	* @return	true if file associations were successfully updated.
	*/
	virtual bool UpdateFileAssociations() = 0;

	/**
	* Sets the engine association for a project.
	*
	* @param ProjectFileName	Filename of the project to update
	* @param Identifier			Identifier of the engine to associate it with
	* @return true if the project was successfully updated
	*/
	virtual bool SetEngineIdentifierForProject(const FString &ProjectFileName, const FString &Identifier) = 0;

	/**
	* Gets the engine association for a project.
	*
	* @param ProjectFileName	Filename of the project to update
	* @param OutIdentifier		Identifier of the engine to associate it with
	* @return true if OutIdentifier is set to the project's engine association
	*/
	virtual bool GetEngineIdentifierForProject(const FString &ProjectFileName, FString &OutIdentifier) = 0;

	/**
	* Compiles a game project.
	*
	* @param RootDir			Engine root directory for the project to use.
	* @param ProjectFileName	Filename of the project to update
	* @param Warn				Feedback context to use for progress updates
	* @return true if project files were generated successfully.
	*/
	virtual bool CompileGameProject(const FString& RootDir, const FString& ProjectFileName, FFeedbackContext* Warn) = 0;

	/**
	* Generates project files for the given project.
	*
	* @param RootDir			Engine root directory for the project to use.
	* @param ProjectFileName	Filename of the project to update
	* @param Warn				Feedback context to use for progress updates
	* @return true if project files were generated successfully.
	*/
	virtual bool GenerateProjectFiles(const FString& RootDir, const FString& ProjectFileName, FFeedbackContext* Warn) = 0;

	/**
	* Runs UnrealBuildTool with the given arguments.
	*
	* @param Description		Task description for FFeedbackContext
	* @param RootDir			Engine root directory for the project to use.
	* @param Arguments			Parameters for UnrealBuildTool
	* @param Warn				Feedback context to use for progress updates
	* @return true if the task completed successfully.
	*/
	virtual bool RunUnrealBuildTool(const FText& Description, const FString& RootDir, const FString& Arguments, FFeedbackContext* Warn) = 0;
};
