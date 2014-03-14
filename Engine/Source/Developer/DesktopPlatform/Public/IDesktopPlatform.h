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
	virtual bool OpenLauncher(bool Install, const FString& CommandLineParams ) = 0;
};