// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DesktopPlatformPrivatePCH.h"
#include "MacApplication.h"

#define LOCTEXT_NAMESPACE "DesktopPlatform"

class FCocoaScopeContext
{
public:
	FCocoaScopeContext(void)
	{
		SCOPED_AUTORELEASE_POOL;
		PreviousContext = [NSOpenGLContext currentContext];
	}
	
	~FCocoaScopeContext( void )
	{
		SCOPED_AUTORELEASE_POOL;
		NSOpenGLContext* NewContext = [NSOpenGLContext currentContext];
		if (PreviousContext != NewContext)
		{
			if (PreviousContext)
			{
				[PreviousContext makeCurrentContext];
			}
			else
			{
				[NSOpenGLContext clearCurrentContext];
			}
		}
	}
	
private:
	NSOpenGLContext*	PreviousContext;
};

/**
 * Custom accessory view class to allow choose kind of file extension
 */
@interface FFileDialogAccessoryView : NSView
{
@private
	NSPopUpButton*	PopUpButton;
	NSTextField*	TextField;
	NSSavePanel*	DialogPanel;
	NSMutableArray*	AllowedFileTypes;
}

- (id)initWithFrame:(NSRect)frameRect dialogPanel:(NSSavePanel*) panel;
- (void)PopUpButtonAction: (id) sender;
- (void)AddAllowedFileTypes: (NSArray*) array;
- (void)SetExtensionsAtIndex: (int32) index;

@end

@implementation FFileDialogAccessoryView

- (id)initWithFrame:(NSRect)frameRect dialogPanel:(NSSavePanel*) panel
{
	self = [super initWithFrame: frameRect];
	DialogPanel = panel;
	
	FString FieldText = LOCTEXT("FileExtension", "File extension:").ToString();
	CFStringRef FieldTextCFString = FPlatformString::TCHARToCFString(*FieldText);
	TextField = [[NSTextField alloc] initWithFrame: NSMakeRect(0.0, 48.0, 90.0, 25.0) ];
	[TextField setStringValue:(NSString*)FieldTextCFString];
	[TextField setEditable:NO];
	[TextField setBordered:NO];
	[TextField setBackgroundColor:[NSColor controlColor]];
	

	PopUpButton = [[NSPopUpButton alloc] initWithFrame: NSMakeRect(88.0, 50.0, 160.0, 25.0) ];
	[PopUpButton setTarget: self];
	[PopUpButton setAction:@selector(PopUpButtonAction:)];

	[self addSubview: TextField];
	[self addSubview: PopUpButton];

	return self;
}

- (void)AddAllowedFileTypes: (NSMutableArray*) array
{
	check( array );

	AllowedFileTypes = array;
	int32 ArrayCount = [AllowedFileTypes count];
	if( ArrayCount )
	{
		check( ArrayCount % 2 == 0 );

		[PopUpButton removeAllItems];

		for( int32 Index = 0; Index < ArrayCount; Index += 2 )
		{
			[PopUpButton addItemWithTitle: [AllowedFileTypes objectAtIndex: Index]];
		}

		// Set allowed extensions
		[self SetExtensionsAtIndex: 0];
	}
	else
	{
		// Allow all file types
		[DialogPanel setAllowedFileTypes:nil];
	}
}

- (void)PopUpButtonAction: (id) sender
{
	NSInteger Index = [PopUpButton indexOfSelectedItem];
	[self SetExtensionsAtIndex: Index];
}

- (void)SetExtensionsAtIndex: (int32) index
{
	check( [AllowedFileTypes count] >= index * 2 );

	NSString* ExtsToParse = [AllowedFileTypes objectAtIndex:index * 2 + 1];
	if( [ExtsToParse compare:@"*.*"] == NSOrderedSame )
	{
		[DialogPanel setAllowedFileTypes: nil];
	}
	else
	{
		NSArray* ExtensionsWildcards = [ExtsToParse componentsSeparatedByString:@";"];
		NSMutableArray* Extensions = [NSMutableArray arrayWithCapacity: [ExtensionsWildcards count]];

		for( int32 Index = 0; Index < [ExtensionsWildcards count]; ++Index )
		{
			NSString* Temp = [[ExtensionsWildcards objectAtIndex:Index] stringByTrimmingCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"*."]];
			[Extensions addObject: Temp];
		}

		[DialogPanel setAllowedFileTypes: Extensions];
	}
}

@end

/**
 * Custom accessory view class to allow choose kind of file extension
 */
@interface FFontDialogAccessoryView : NSView
{
@private

	NSButton*	OKButton;
	NSButton*	CancelButton;
	bool		Result;
}

- (id)initWithFrame: (NSRect)frameRect;
- (bool)result;
- (IBAction)onCancel: (id)sender;
- (IBAction)onOK: (id)sender;

@end

@implementation FFontDialogAccessoryView : NSView

- (id)initWithFrame: (NSRect)frameRect
{
	[super initWithFrame: frameRect];

	CancelButton = [[NSButton alloc] initWithFrame: NSMakeRect(10, 10, 80, 24)];
	[CancelButton setTitle: @"Cancel"];
	[CancelButton setBezelStyle: NSRoundedBezelStyle];
	[CancelButton setButtonType: NSMomentaryPushInButton];
	[CancelButton setAction: @selector(onCancel:)];
	[CancelButton setTarget: self];
	[self addSubview: CancelButton];

	OKButton = [[NSButton alloc] initWithFrame: NSMakeRect(100, 10, 80, 24)];
	[OKButton setTitle: @"OK"];
	[OKButton setBezelStyle: NSRoundedBezelStyle];
	[OKButton setButtonType: NSMomentaryPushInButton];
	[OKButton setAction: @selector(onOK:)];
	[OKButton setTarget: self];
	[self addSubview: OKButton];

	Result = false;

	return self;
}

- (bool)result
{
	return Result;
}

- (IBAction)onCancel: (id)sender
{
	Result = false;
	[NSApp stopModal];
}

- (IBAction)onOK: (id)sender
{
	Result = true;
	[NSApp stopModal];
}

@end

bool FDesktopPlatformMac::OpenFileDialog(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes, uint32 Flags, TArray<FString>& OutFilenames)
{
	return FileDialogShared(false, ParentWindowHandle, DialogTitle, DefaultPath, DefaultFile, FileTypes, Flags, OutFilenames);
}

bool FDesktopPlatformMac::SaveFileDialog(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes, uint32 Flags, TArray<FString>& OutFilenames)
{
	return FileDialogShared(true, ParentWindowHandle, DialogTitle, DefaultPath, DefaultFile, FileTypes, Flags, OutFilenames);
}

bool FDesktopPlatformMac::OpenDirectoryDialog(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, FString& OutFolderName)
{
	SCOPED_AUTORELEASE_POOL;
	FCocoaScopeContext ContextGuard;

	MacApplication->SetCapture( NULL );

	NSOpenPanel* Panel = [NSOpenPanel openPanel];
	[Panel setCanChooseFiles: false];
	[Panel setCanChooseDirectories: true];
	[Panel setAllowsMultipleSelection: false];
	[Panel setCanCreateDirectories: true];

	CFStringRef Title = FPlatformString::TCHARToCFString(*DialogTitle);
	[Panel setTitle: (NSString*)Title];
	CFRelease(Title);

	CFStringRef DefaultPathCFString = FPlatformString::TCHARToCFString(*DefaultPath);
	NSURL* DefaultPathURL = [NSURL fileURLWithPath: (NSString*)DefaultPathCFString];
	[Panel setDirectoryURL: DefaultPathURL];
	CFRelease(DefaultPathCFString);

	bool bSuccess = false;

	{
		FScopedSystemModalMode SystemModalScope;
		NSInteger Result = [Panel runModal];

		if (Result == NSFileHandlingPanelOKButton)
		{
			NSURL *FolderURL = [[Panel URLs] objectAtIndex: 0];
			TCHAR FolderPath[MAX_PATH];
			FPlatformString::CFStringToTCHAR((CFStringRef)[FolderURL path], FolderPath);
			OutFolderName = FolderPath;
			FPaths::NormalizeFilename(OutFolderName);

			bSuccess = true;
		}
	}

	[Panel close];

	MacApplication->ResetModifierKeys();

	return bSuccess;
}

bool FDesktopPlatformMac::OpenFontDialog(const void* ParentWindowHandle, FString& OutFontName, float& OutHeight, EFontImportFlags::Type& OutFlags)
{
	SCOPED_AUTORELEASE_POOL;
	FCocoaScopeContext ContextGuard;

	MacApplication->SetCapture( NULL );

	NSFontPanel* Panel = [NSFontPanel sharedFontPanel];
	[Panel setFloatingPanel: false];
	[[Panel standardWindowButton: NSWindowCloseButton] setEnabled: false];
	FFontDialogAccessoryView* AccessoryView = [[FFontDialogAccessoryView alloc] initWithFrame: NSMakeRect(0.0, 0.0, 190.0, 80.0)];
	[Panel setAccessoryView: AccessoryView];

	{
		FScopedSystemModalMode SystemModalScope;
		[NSApp runModalForWindow: Panel];
	}

	[Panel close];

	bool bSuccess = [AccessoryView result];

	[Panel setAccessoryView: NULL];
	[AccessoryView release];
	[[Panel standardWindowButton: NSWindowCloseButton] setEnabled: true];

	if( bSuccess )
	{
		NSFont* Font = [Panel panelConvertFont: [NSFont userFontOfSize: 0]];

		TCHAR FontName[MAX_PATH];
		FPlatformString::CFStringToTCHAR((CFStringRef)[Font fontName], FontName);

		OutFontName = FontName;
		OutHeight = [Font pointSize];

		uint32 FontFlags = EFontImportFlags::None;

		if( [Font underlineThickness] >= 1.0 )
		{
			FontFlags |= EFontImportFlags::EnableUnderline;
		}

		OutFlags = static_cast<EFontImportFlags::Type>(FontFlags);
	}

	MacApplication->ResetModifierKeys();

	return bSuccess;
}

void OpenLauncherCommandLine( const FString& InCommandLine )
{
	FString CommandLine = InCommandLine;
	FString TransferFilePath = FPaths::Combine(FPlatformProcess::UserSettingsDir(), TEXT("UnrealEngineLauncher"), TEXT("com"), TEXT("transfer.tmp"));

	IFileManager& FileManager = IFileManager::Get();
	FArchive* TransferFile = NULL;
	int32 RetryCount = 0;

	while (RetryCount < 2000)
	{
		TransferFile = FileManager.CreateFileWriter(*TransferFilePath, FILEWRITE_EvenIfReadOnly);

		if (TransferFile != nullptr)
		{
			break;
		}

		++RetryCount;
		FPlatformProcess::Sleep(0.01f);
	}

	if (TransferFile == NULL)
	{
		TransferFile = FileManager.CreateFileWriter(*TransferFilePath, FILEWRITE_EvenIfReadOnly | FILEWRITE_NoFail);
	}

	if (TransferFile != NULL)
	{
		*TransferFile << CommandLine;
		TransferFile->Close();
		delete TransferFile;
	}
	
}

bool FDesktopPlatformMac::OpenLauncher(bool Install, const FString& CommandLineParams )
{
	// If the launcher is already running, bring it to front
	NSArray* RunningLaunchers = [NSRunningApplication runningApplicationsWithBundleIdentifier: @"com.epicgames.UnrealEngineLauncher"];
	if ([RunningLaunchers count] > 0)
	{
		NSRunningApplication* Launcher = [RunningLaunchers objectAtIndex: 0];
		[Launcher activateWithOptions: NSApplicationActivateAllWindows | NSApplicationActivateIgnoringOtherApps];
		OpenLauncherCommandLine(CommandLineParams); // create a temp file that will tell running Launcher instance to switch to Marketplace tab
		return true;
	}

	FString LaunchPath;

	bool bWasLaunched = false;
	if (FParse::Param(FCommandLine::Get(), TEXT("Dev")))
	{
		LaunchPath = FPaths::Combine(*FPaths::EngineDir(), TEXT("Binaries"), TEXT("Mac"), TEXT("UnrealEngineLauncher-Mac-Debug.app/Contents/MacOS/UnrealEngineLauncher-Mac-Debug"));
	}
	else
	{
		LaunchPath = TEXT("/Applications/Unreal Engine.app/Contents/MacOS/UnrealEngineLauncher-Mac-Shipping");
	}

	if( !FPaths::FileExists(LaunchPath) )
	{
		// The app could have been installed manually from the dmg, launch it where ever it is
		NSWorkspace* Workspace = [NSWorkspace sharedWorkspace];
		NSString* Path = [Workspace fullPathForApplication:@"Unreal Engine"];
		if( Path )
		{
			NSURL* URL = [NSURL fileURLWithPath:Path];
			if( URL )
			{
				NSArray* Arguments = [NSArray arrayWithObjects:CommandLineParams.GetNSString(),nil];
				bWasLaunched = [Workspace launchApplicationAtURL:URL options:0 configuration:[NSDictionary dictionaryWithObject:Arguments forKey:NSWorkspaceLaunchConfigurationArguments] error:nil];
				
				OpenLauncherCommandLine(CommandLineParams);
			}
		}
	}

	if( !bWasLaunched )
	{
		if( FPaths::FileExists(LaunchPath) )
		{
			bWasLaunched = FPlatformProcess::CreateProc(*LaunchPath, *CommandLineParams, true, false, false, NULL, 0, *FPaths::GetPath(LaunchPath), NULL).IsValid();
		}
		else if (Install)
		{
			// ... or run the installer if desired
			LaunchPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(*FPaths::EngineDir(), TEXT("Extras/UnrealEngineLauncher/UnrealEngine.dmg")));

			if (FPaths::FileExists(LaunchPath))
			{
				FPlatformProcess::LaunchFileInDefaultExternalApplication(*LaunchPath);
				bWasLaunched = true;
			}
			else
			{
				bWasLaunched = false;
			}
		}

	}
	return bWasLaunched;
}

bool FDesktopPlatformMac::FileDialogShared(bool bSave, const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath, const FString& DefaultFile, const FString& FileTypes, uint32 Flags, TArray<FString>& OutFilenames)
{
	SCOPED_AUTORELEASE_POOL;
	FCocoaScopeContext ContextGuard;

	MacApplication->SetCapture( NULL );

	NSSavePanel* Panel = bSave ? [NSSavePanel savePanel] : [NSOpenPanel openPanel];

	if (!bSave)
	{
		NSOpenPanel* OpenPanel = (NSOpenPanel*)Panel;
		[OpenPanel setCanChooseFiles: true];
		[OpenPanel setCanChooseDirectories: false];
		[OpenPanel setAllowsMultipleSelection: Flags & EFileDialogFlags::Multiple];
	}

	[Panel setCanCreateDirectories: bSave];

	CFStringRef Title = FPlatformString::TCHARToCFString(*DialogTitle);
	[Panel setTitle: (NSString*)Title];
	CFRelease(Title);

	CFStringRef DefaultPathCFString = FPlatformString::TCHARToCFString(*DefaultPath);
	NSURL* DefaultPathURL = [NSURL fileURLWithPath: (NSString*)DefaultPathCFString];
	[Panel setDirectoryURL: DefaultPathURL];
	CFRelease(DefaultPathCFString);

	CFStringRef FileNameCFString = FPlatformString::TCHARToCFString(*DefaultFile);
	[Panel setNameFieldStringValue: (NSString*)FileNameCFString];
	CFRelease(FileNameCFString);

	FFileDialogAccessoryView* AccessoryView = [[FFileDialogAccessoryView alloc] initWithFrame: NSMakeRect( 0.0, 0.0, 250.0, 85.0 ) dialogPanel: Panel];
	[Panel setAccessoryView: AccessoryView];

	TArray<FString> FileTypesArray;
	int32 NumFileTypes = FileTypes.ParseIntoArray(&FileTypesArray, TEXT("|"), true);

	NSMutableArray* AllowedFileTypes = [NSMutableArray arrayWithCapacity: NumFileTypes];

	if( NumFileTypes > 0 )
	{
		for( int32 Index = 0; Index < NumFileTypes; ++Index )
		{
			CFStringRef Type = FPlatformString::TCHARToCFString(*FileTypesArray[Index]);
			[AllowedFileTypes addObject: (NSString*)Type];
			CFRelease(Type);
		}
	}

	[AccessoryView AddAllowedFileTypes:AllowedFileTypes];

	bool bSuccess = false;

	{
		FScopedSystemModalMode SystemModalScope;

		NSInteger Result = [Panel runModal];
		[AccessoryView release];

		if (Result == NSFileHandlingPanelOKButton)
		{
			if (bSave)
			{
				TCHAR FilePath[MAX_PATH];
				FPlatformString::CFStringToTCHAR((CFStringRef)[[Panel URL] path], FilePath);
				new(OutFilenames) FString(FilePath);
			}
			else
			{
				NSOpenPanel* OpenPanel = (NSOpenPanel*)Panel;
				for (NSURL *FileURL in [OpenPanel URLs])
				{
					TCHAR FilePath[MAX_PATH];
					FPlatformString::CFStringToTCHAR((CFStringRef)[FileURL path], FilePath);
					new(OutFilenames) FString(FilePath);
				}
			}

			// Make sure all filenames gathered have their paths normalized
			for (auto FilenameIt = OutFilenames.CreateIterator(); FilenameIt; ++FilenameIt)
			{
				FPaths::NormalizeFilename(*FilenameIt);
			}

			bSuccess = true;
		}
	}

	[Panel close];

	MacApplication->ResetModifierKeys();

	return bSuccess;
}

#undef LOCTEXT_NAMESPACE
