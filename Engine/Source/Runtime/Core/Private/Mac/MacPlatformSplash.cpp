// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"
#include "EngineVersion.h"
#include "EngineBuildSettings.h"

/**
 * Simple window class that overrides a couple of functions, so the splash window can be moved to front even if it's borderless
 */
@interface FSplashWindow : NSWindow <NSWindowDelegate>

@end

@implementation FSplashWindow

- (BOOL)canBecomeMainWindow
{
	return YES;
}

- (BOOL)canBecomeKeyWindow
{
	return YES;
}

@end

/**
 * Splash screen functions and static globals
 */

static FSplashWindow *GSplashWindow = NULL;
static NSImage *GSplashScreenImage = NULL;
static FText GSplashScreenAppName;
static FText GSplashScreenText[ SplashTextType::NumTextTypes ];
static NSRect GSplashScreenTextRects[ SplashTextType::NumTextTypes ];


/**
 * Finds a usable splash pathname for the given filename
 * 
 * @param SplashFilename Name of the desired splash name ("Splash.bmp")
 * @param OutPath String containing the path to the file, if this function returns true
 *
 * @return true if a splash screen was found
 */
static bool GetSplashPath(const TCHAR* SplashFilename, FString& OutPath)
{
	// first look in game's splash directory
	OutPath = FPaths::GameContentDir() + TEXT("Splash/") + SplashFilename;
	
	// if this was found, then we're done
	if (IFileManager::Get().FileSize(*OutPath) != -1)
	{
		return true;
	}

	// next look in Engine/Splash
	OutPath = FPaths::EngineContentDir() + TEXT("Splash/") + SplashFilename;

	// if this was found, then we're done
	if (IFileManager::Get().FileSize(*OutPath) != -1)
	{
		return true;
	}

	// if not found yet, then return failure
	return false;
}

/**
 * Sets the text displayed on the splash screen (for startup/loading progress)
 *
 * @param	InType		Type of text to change
 * @param	InText		Text to display
 */
static void StartSetSplashText( const SplashTextType::Type InType, const FText& InText )
{
	// Only allow copyright text displayed while loading the game.  Editor displays all.
	if( InType == SplashTextType::CopyrightInfo || GIsEditor )
	{
		// Update splash text
		GSplashScreenText[ InType ] = InText;
	}
}

@interface UE4SplashView : NSView
{
}

- (void)drawRect: (NSRect)DirtyRect;
- (void)drawText: (NSString *)Text inRect: (NSRect)Rect withColor: (NSColor *)Color fontSize: (int32)FontSize;

@end

@implementation UE4SplashView

- (void)drawRect: (NSRect)DirtyRect
{
	SCOPED_AUTORELEASE_POOL;

	// Draw background
	[GSplashScreenImage drawAtPoint: NSMakePoint(0,0) fromRect: NSZeroRect operation: NSCompositeCopy fraction: 1.0];

	for( int32 CurTypeIndex = 0; CurTypeIndex < SplashTextType::NumTextTypes; ++CurTypeIndex )
	{
		const FText& SplashText = GSplashScreenText[ CurTypeIndex ];
		const NSRect& TextRect = GSplashScreenTextRects[ CurTypeIndex ];

		if( SplashText.ToString().Len() > 0 )
		{
			int32 FontSize = (CurTypeIndex == SplashTextType::StartupProgress || CurTypeIndex == SplashTextType::VersionInfo1) ? 12 : 11;

			float Brightness;

			if( CurTypeIndex == SplashTextType::StartupProgress )
			{
				Brightness = 180.0f / 255.0f;
			}
			else if( CurTypeIndex == SplashTextType::VersionInfo1 )
			{
				Brightness = 240.0f / 255.0f;
			}
			else
			{
				Brightness = 160.0f / 255.0f;
			}

			NSColor *TextColor = [NSColor colorWithDeviceRed: Brightness green: Brightness blue: Brightness alpha: 1.0f];

			NSString *Text = (NSString *)FPlatformString::TCHARToCFString(*SplashText.ToString());
			[self drawText: Text inRect: TextRect withColor: TextColor fontSize: FontSize];
			[Text release];
		}
	}
}

- (void)drawText: (NSString *)Text inRect: (NSRect)Rect withColor: (NSColor *)Color fontSize: (int32)FontSize
{
	SCOPED_AUTORELEASE_POOL;

	NSDictionary* Dict =
		[NSDictionary dictionaryWithObjects:
			[NSArray arrayWithObjects:
				Color,
				[NSFont fontWithName:@"Helvetica-Bold" size: FontSize],
				[NSColor colorWithDeviceRed: 0.0 green: 0.0 blue: 0.0 alpha: 1.0],
				[NSNumber numberWithFloat:-4.0],
				nil ]
		forKeys:
			[NSArray arrayWithObjects:
				NSForegroundColorAttributeName,
				NSFontAttributeName,
				NSStrokeColorAttributeName,
				NSStrokeWidthAttributeName,
				nil]
		];
	[Text drawInRect: Rect withAttributes: Dict];
}

@end

void FMacPlatformSplash::Show()
{
	if( !GSplashWindow && FParse::Param(FCommandLine::Get(),TEXT("NOSPLASH")) != true )
	{
		SCOPED_AUTORELEASE_POOL;

		const TCHAR* SplashImage =
				( GIsEditor ? TEXT("EdSplash.bmp") : TEXT("Splash.bmp") );

		// make sure a splash was found
		FString SplashPath;
		if (GetSplashPath( SplashImage, SplashPath ) == true)
		{
			// In the editor, we'll display loading info
			if( GIsEditor )
			{
				// Set initial startup progress info
				{
					StartSetSplashText( SplashTextType::StartupProgress,
						NSLOCTEXT("UnrealEd", "SplashScreen_InitialStartupProgress", "Loading..." ) );
				}

				// Set version info
				{
#if PLATFORM_64BITS
					//These are invariant strings so they don't need to be localized
					const FText PlatformBits = FText::FromString( TEXT( "64" ) );
#else	//PLATFORM_64BITS
					const FText PlatformBits = FText::FromString( TEXT( "32" ) );
#endif	//PLATFORM_64BITS

					const FText GameName = FText::FromString( FApp::GetGameName() );
					const FText Version = FText::FromString( GEngineVersion.ToString( FEngineBuildSettings::IsPerforceBuild() ? EVersionComponent::Branch : EVersionComponent::Patch ) );

					FText VersionInfo = FText::Format( NSLOCTEXT("UnrealEd", "UnrealEdTitleWithVersion_F", "Unreal Editor - {0} ({1}-bit) [Version {2}]" ), GameName, PlatformBits, Version );
					FText AppName =     FText::Format( NSLOCTEXT("UnrealEd", "UnrealEdTitle_F",            "Unreal Editor - {0} ({1}-bit)" ), GameName, PlatformBits );

					StartSetSplashText( SplashTextType::VersionInfo1, VersionInfo );

					// Change the window text (which will be displayed in the taskbar)
					GSplashScreenAppName = AppName;
				}

				// Display copyright information in editor splash screen
				{
					const FText CopyrightInfo = NSLOCTEXT( "UnrealEd", "SplashScreen_CopyrightInfo", "Copyright \x00a9 1998-2014   Epic Games, Inc.   All rights reserved." );
					StartSetSplashText( SplashTextType::CopyrightInfo, CopyrightInfo );
				}
			}
		}

		NSString *SplashScreenFileName = (NSString *)FPlatformString::TCHARToCFString(*(FString(FPlatformProcess::BaseDir()) / SplashPath));
		GSplashScreenImage = [[NSImage alloc] initWithContentsOfFile: SplashScreenFileName];
		[SplashScreenFileName release];

		NSBitmapImageRep* BitmapRep = [NSBitmapImageRep imageRepWithData: [GSplashScreenImage TIFFRepresentation]];
		float ImageWidth = [BitmapRep pixelsWide];
		float ImageHeight = [BitmapRep pixelsHigh];

		NSRect ContentRect;
		ContentRect.origin.x = 0;
		ContentRect.origin.y = 0;
		ContentRect.size.width = ImageWidth;
		ContentRect.size.height = ImageHeight;

		int32 OriginX = 10;
		int32 OriginY = 6;
		int32 FontHeight = 14;

		// Setup bounds for texts
		GSplashScreenTextRects[ SplashTextType::VersionInfo1 ].origin.x =
		GSplashScreenTextRects[ SplashTextType::CopyrightInfo ].origin.x =
		GSplashScreenTextRects[ SplashTextType::StartupProgress ].origin.x = OriginX;

		GSplashScreenTextRects[ SplashTextType::VersionInfo1 ].size.width =
		GSplashScreenTextRects[ SplashTextType::CopyrightInfo ].size.width =
		GSplashScreenTextRects[ SplashTextType::StartupProgress ].size.width = ImageWidth - 2 * OriginX;

		GSplashScreenTextRects[ SplashTextType::VersionInfo1 ].size.height =
		GSplashScreenTextRects[ SplashTextType::CopyrightInfo ].size.height =
		GSplashScreenTextRects[ SplashTextType::StartupProgress ].size.height = FontHeight;

		GSplashScreenTextRects[ SplashTextType::VersionInfo1 ].origin.y = OriginY + 3 * FontHeight;
		GSplashScreenTextRects[ SplashTextType::StartupProgress ].origin.y = OriginY;

		if( GIsEditor )
		{
			GSplashScreenTextRects[ SplashTextType::CopyrightInfo ].origin.y = OriginY + 2 * FontHeight;
		}
		else
		{
			GSplashScreenTextRects[ SplashTextType::CopyrightInfo ].origin.y = OriginY;
		}

		// Create bordeless window with size from NSImage
		GSplashWindow = [[FSplashWindow alloc] initWithContentRect: ContentRect styleMask: 0 backing: NSBackingStoreBuffered defer: NO];
		[GSplashWindow setContentView: [[UE4SplashView alloc] initWithFrame: ContentRect]];

		if( GSplashWindow )
		{
			// Show window
			[GSplashWindow setHasShadow:YES];
			[GSplashWindow center];
			[GSplashWindow orderFront: nil];
			[NSApp activateIgnoringOtherApps:YES];
		}
		else
		{
			[GSplashScreenImage release];
			GSplashScreenImage = NULL;
		}

		FPlatformMisc::PumpMessages(true);
	}
}

void FMacPlatformSplash::Hide()
{
	if (GSplashWindow)
	{
		SCOPED_AUTORELEASE_POOL;

		[GSplashWindow close];
		GSplashWindow = NULL;

		[GSplashScreenImage release];
		GSplashScreenImage = NULL;

		FPlatformMisc::PumpMessages(true);
	}
}

void FMacPlatformSplash::SetSplashText(const SplashTextType::Type InType, const TCHAR* InText)
{
	// We only want to bother drawing startup progress in the editor, since this information is
	// not interesting to an end-user (also, it's not usually localized properly.)
	if( GSplashWindow )
	{
		// Only allow copyright text displayed while loading the game.  Editor displays all.
		if( InType == SplashTextType::CopyrightInfo || GIsEditor )
		{
			bool bWasUpdated = false;
			{
				// Update splash text
				if( FCString::Strcmp( InText, *GSplashScreenText[ InType ].ToString() ) != 0 )
				{
					GSplashScreenText[ InType ] = FText::FromString( InText );
					bWasUpdated = true;
				}
			}

			if( bWasUpdated )
			{
				SCOPED_AUTORELEASE_POOL;

				// Repaint the window
				[[GSplashWindow contentView] setNeedsDisplayInRect: GSplashScreenTextRects[InType]];

				FPlatformMisc::PumpMessages(true);
			}
		}
	}
}
