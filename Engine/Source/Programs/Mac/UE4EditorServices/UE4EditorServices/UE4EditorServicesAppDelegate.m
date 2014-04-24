// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#import "UE4EditorServicesAppDelegate.h"

@implementation FUE4EditorServicesAppDelegate

@synthesize Window;
@synthesize EditorMenu;
@synthesize OKButton;
@synthesize CancelButton;
@synthesize DontShow;
@synthesize DefaultApps;

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
	// Insert code here to initialize your application
	[NSApp setServicesProvider:self];
	DefaultApps = [NSMutableDictionary new];
	NSDictionary* Current = (NSDictionary*)[[NSUserDefaults standardUserDefaults] objectForKey:@"DefaultApps"];
	if(Current)
	{
		[DefaultApps addEntriesFromDictionary:Current];
	}
}

/// construct menu item for app
- (NSMenuItem *)menuItemForOpenWithForApplication:(NSString *)AppName URL:(NSURL *)AppURL
{
    NSMenuItem* AppItem = [NSMenuItem new];
    [AppItem setTitle:AppName];
    [AppItem setRepresentedObject:AppURL];
	[AppItem setToolTip:[AppURL path]];
	NSImage* Img = [[NSWorkspace sharedWorkspace] iconForFile:[AppURL path]];
	[Img setSize:NSMakeSize(16, 16)];
    [AppItem setImage:Img];
    return AppItem;
}

- (void)openWithApplicationOtherSelected:(id)Sender
{
    NSOpenPanel* Panel = [NSOpenPanel openPanel];
	
    [Panel setAllowsMultipleSelection:NO];
    [Panel setCanChooseFiles:YES];
    [Panel setCanChooseDirectories:NO];
	[Panel beginSheetModalForWindow: Window completionHandler:^(NSInteger result){
		if (result == NSOKButton)
		{
			NSMenuItem* SelectedItem = nil;
			bool AlreadyAddded = false;
			for (NSMenuItem* Item in [[EditorMenu menu] itemArray])
			{
				if(Item && [Item representedObject] && [[[Item representedObject] path] compare:[[Panel URL] path] options:NSCaseInsensitiveSearch] == NSOrderedSame)
				{
					SelectedItem = Item;
					AlreadyAddded = true;
					break;
				}
			}
			if(!AlreadyAddded)
			{
				NSString *appName = [[[[Panel URL] path] lastPathComponent] stringByDeletingPathExtension];
				NSMenuItem *newAppItem = [self menuItemForOpenWithForApplication:appName URL:[Panel URL]];
				[[EditorMenu menu] insertItem:newAppItem atIndex:2];
				SelectedItem = newAppItem;
			}
			[Panel close];
			[EditorMenu selectItem:SelectedItem];
		}
	}];
}

/// this method return open with menu for specified file
- (NSMenu *)openWithMenuItemForURL:(NSURL *)FileURL
{
    NSMenu* SubMenu = [NSMenu new];
    CFArrayRef AllApps = LSCopyApplicationURLsForURL((__bridge CFURLRef)FileURL, kLSRolesEditor);
    NSMutableSet* AlreadyAdded = [NSMutableSet new];
	if (AllApps != nil)
	{
		CFIndex count = CFArrayGetCount(AllApps);
        //get and add default app
        CFURLRef DefaultApp;
        LSGetApplicationForURL((__bridge CFURLRef)FileURL, kLSRolesEditor, NULL, &DefaultApp);
        if (DefaultApp)
		{
			NSString* DefaultAppPath = [(__bridge NSURL *)DefaultApp path];
            NSString* DefaultAppName = [[[DefaultAppPath lastPathComponent] stringByDeletingPathExtension] stringByAppendingString:@" (default)"];
            NSMenuItem* AppItem = [self menuItemForOpenWithForApplication:DefaultAppName URL:(__bridge NSURL *)DefaultApp];
			[AlreadyAdded addObject:DefaultAppName];
			[SubMenu addItem:AppItem];
			[SubMenu addItem:[NSMenuItem separatorItem]];
		}
		else
		{
            NSLog(@"There is no default UE4 Editor for %@", FileURL);
            NSMenuItem* NoneItem = [NSMenuItem new];
            [NoneItem setTitle:@"<None>"];
            [NoneItem setEnabled:NO];
            [SubMenu addItem:NoneItem];
            [SubMenu addItem:[NSMenuItem separatorItem]];
        }
        
		if (count != 0)
		{
			for (int index = 0; index < count; ++index)
			{
				NSURL *appURL = (NSURL *)CFArrayGetValueAtIndex(AllApps, index);
				if ([appURL isFileURL])
				{
					NSString* AppName = [[[appURL path] lastPathComponent] stringByDeletingPathExtension];
					NSDictionary* Dictionary = [[NSBundle bundleWithPath:[appURL path]] infoDictionary];
					if(Dictionary)
					{
						NSString* BundleId = (NSString*)[Dictionary valueForKey:@"CFBundleIdentifier"];
						if (BundleId && ([BundleId compare:@"com.epicgames.UE4Editor" options:NSCaseInsensitiveSearch] == NSOrderedSame
										 || [BundleId compare:@"com.epicgames.RocketEditor" options:NSCaseInsensitiveSearch] == NSOrderedSame))
						{
							if ([AlreadyAdded containsObject:AppName])
							{
								AppName = [AppName stringByAppendingFormat:@" (%@)", [Dictionary valueForKey:@"CFBundleVersion"]];
							}
							NSMenuItem* AppItem = [self menuItemForOpenWithForApplication:AppName URL:appURL];
							[AlreadyAdded addObject:AppName];
							[SubMenu addItem:AppItem];
						}
					}
				}
			}
			[SubMenu addItem:[NSMenuItem separatorItem]];
		}
		
        NSMenuItem* OtherAppItem = [NSMenuItem new];
        [OtherAppItem setTitle:@"Other…"];
        [OtherAppItem setTarget:self];
        [OtherAppItem setAction:@selector(openWithApplicationOtherSelected:)];
        [SubMenu addItem:OtherAppItem];
		
		CFRelease(AllApps);
    }
    return SubMenu;
}

- (IBAction)onCancelButtonPressed:(id)sender
{
	[NSApp stopModalWithCode:NSCancelButton];
	[Window orderOut:self];
}

- (IBAction)onOKButtonPressed:(id)sender
{
	[NSApp stopModalWithCode:NSOKButton];
	[Window orderOut:self];
}

- (void)launchGameService:(NSPasteboard *)pboard userData:(NSString *)userData error:(NSString **)error
{
	//reg add HKEY_CLASSES_ROOT\Unreal.ProjectFile\shell\run /d "Launch Game"
	//reg add HKEY_CLASSES_ROOT\Unreal.ProjectFile\shell\run\command /t REG_SZ /d "\"%UE_ROOT_DIR%\\Engine\\Binaries\\Win64\\UE4Editor.exe\" \"%%1\" -game"
	
	NSString* UnrealError = nil;
	
	if([[pboard types] containsObject:NSFilenamesPboardType])
    {
		NSArray* FileArray = [pboard propertyListForType:NSFilenamesPboardType];
		
		NSString* FilePath = [FileArray objectAtIndex:0];
		
		NSString* ProjectName = FilePath;
		
		NSURL* FileURL = [NSURL fileURLWithPath: FilePath];
		
		NSURL* BundleURL = nil;
		
		CFArrayRef AllAppURLs = LSCopyApplicationURLsForURL((__bridge CFURLRef)FileURL, kLSRolesAll);
		bool AltDown = ([NSEvent modifierFlags] & NSAlternateKeyMask);
		if([DefaultApps objectForKey:[FileURL absoluteString]] && !AltDown)
		{
			BundleURL = [NSURL URLWithString:(NSString*)[DefaultApps objectForKey:[FileURL absoluteString]]];
		}
		else if(CFArrayGetCount(AllAppURLs) > 1)
		{
			[EditorMenu setMenu:[self openWithMenuItemForURL:FileURL]];
			[DontShow setState:NSOnState];
			[Window setTitle:[NSString stringWithFormat:@"Launch %@ With...", [[FilePath lastPathComponent] stringByDeletingPathExtension]]];
			[Window setRepresentedURL:FileURL];
			NSInteger Code = [NSApp runModalForWindow:Window];
			if(Code == NSOKButton)
			{
				BundleURL = (NSURL*)[[EditorMenu selectedItem] representedObject];
				if([DontShow state] == NSOnState)
				{
					[DefaultApps setObject:[BundleURL absoluteString] forKey:[FileURL absoluteString]];
					[[NSUserDefaults standardUserDefaults] setObject:(NSDictionary*)DefaultApps forKey:@"DefaultApps"];
					[[NSUserDefaults standardUserDefaults] synchronize];
				}
			}
		}
		else
		{
			BundleURL = [[NSWorkspace sharedWorkspace] URLForApplicationToOpenURL:FileURL];
		}
		CFRelease(AllAppURLs);
		
		if(BundleURL)
		{
			NSDictionary* Configuration = [NSDictionary dictionaryWithObject: [NSArray arrayWithObjects: ProjectName, @"-game", nil] forKey: NSWorkspaceLaunchConfigurationArguments];
			
			NSError* Error = nil;
			
			NSRunningApplication* NewInstance = [[NSWorkspace sharedWorkspace] launchApplicationAtURL:BundleURL options:(NSWorkspaceLaunchOptions)(NSWorkspaceLaunchAsync|NSWorkspaceLaunchNewInstance) configuration:Configuration error:&Error];
			
			if(!NewInstance)
			{
				if(Error)
				{
					UnrealError = [Error localizedDescription];
				}
				else
				{
					UnrealError = @"Failed to run a copy of the game on this machine.";
				}
			}
		}
		else
		{
			UnrealError = @"No application to open the project file available.";
		}
    }
	else
	{
		UnrealError = @"No valid project file selected.";
	}
	
	if(error)
	{
		*error = UnrealError;
	}
}

- (void)generateXcodeFilesService:(NSPasteboard *)pboard userData:(NSString *)userData error:(NSString **)error
{
	// reg add HKEY_CLASSES_ROOT\Unreal.ProjectFile\shell\rungenproj /d "Generate Visual Studio projects"
	// reg add HKEY_CLASSES_ROOT\Unreal.ProjectFile\shell\rungenproj\command /t REG_EXPAND_SZ /d "%%comspec%% /c \"%UE_ROOT_DIR_SINGLE_SLASHES%\Engine\Build\BatchFiles\GenerateProjectFiles.bat -project=\"%%1\" -game\""
	
	NSString* UnrealError = nil;
	
	if([[pboard types] containsObject:NSFilenamesPboardType])
    {
		NSArray* FileArray = [pboard propertyListForType:NSFilenamesPboardType];
		
		NSString* FilePath = [FileArray objectAtIndex:0];
		
		NSURL* FileURL = [NSURL fileURLWithPath: FilePath];
		
		NSURL* BundleURL = nil;
		
		CFArrayRef AllAppURLs = LSCopyApplicationURLsForURL((__bridge CFURLRef)FileURL, kLSRolesAll);
		bool AltDown = ([NSEvent modifierFlags] & NSAlternateKeyMask);
		if([DefaultApps objectForKey:[FileURL absoluteString]] && !AltDown)
		{
			BundleURL = [NSURL URLWithString:(NSString*)[DefaultApps objectForKey:[FileURL absoluteString]]];
		}
		else if(CFArrayGetCount(AllAppURLs) > 1)
		{
			[EditorMenu setMenu:[self openWithMenuItemForURL:FileURL]];
			[DontShow setState:NSOnState];
			[Window setTitle:[NSString stringWithFormat:@"Generate %@ Xcode Project With...", [[FilePath lastPathComponent] stringByDeletingPathExtension]]];
			[Window setRepresentedURL:FileURL];
			NSInteger Code = [NSApp runModalForWindow:Window];
			if(Code == NSOKButton)
			{
				BundleURL = (NSURL*)[[EditorMenu selectedItem] representedObject];
				if([DontShow state] == NSOnState)
				{
					[DefaultApps setObject:[BundleURL absoluteString] forKey:[FileURL absoluteString]];
					[[NSUserDefaults standardUserDefaults] setObject:(NSDictionary*)DefaultApps forKey:@"DefaultApps"];
					[[NSUserDefaults standardUserDefaults] synchronize];
				}
			}
		}
		else
		{
			BundleURL = [[NSWorkspace sharedWorkspace] URLForApplicationToOpenURL:[NSURL fileURLWithPath: FilePath]];
		}
		CFRelease(AllAppURLs);
		
		NSString* FolderPath = nil;
		NSString* ScriptPath = nil;
		
		if(BundleURL)
		{
			NSString* CurrentPath = [[BundleURL path] stringByDeletingLastPathComponent];
			FolderPath = [CurrentPath stringByAppendingString:@"/../../Build/BatchFiles/Mac"];
			ScriptPath = [CurrentPath stringByAppendingString:@"/../../Build/BatchFiles/Mac/GenerateProjectFiles.sh"];
			if(![[NSFileManager defaultManager] fileExistsAtPath:ScriptPath])
			{
				NSString* RocketPath = [CurrentPath stringByAppendingString:@"/../../Build/BatchFiles/Mac/RocketGenerateProjectFiles.sh"];
				if([[NSFileManager defaultManager] fileExistsAtPath:RocketPath])
				{
					ScriptPath = RocketPath;
				}
			}
		}
		
		if(ScriptPath)
		{
			if([[NSWorkspace sharedWorkspace] launchAppWithBundleIdentifier:@"com.apple.terminal" options:0 additionalEventParamDescriptor:nil launchIdentifier:nil])
			{
				NSString* FullFolderPath = [FolderPath stringByResolvingSymlinksInPath];
				NSString* FullScriptPath = [ScriptPath stringByResolvingSymlinksInPath];
				NSString* Command = [NSString stringWithFormat:@"cd \"%@\" \n sh \"%@\" -project=\"%@\" -game\n logout\n", FullFolderPath, FullScriptPath, FilePath];
				
				char const* utf8Script = [Command UTF8String];
				
				char *bundleID = "com.apple.terminal";
				AppleEvent evt, res;
				OSStatus err;
				
				// Build event
				err = AEBuildAppleEvent(kAECoreSuite, kAEDoScript,
										typeApplicationBundleID,
										bundleID, strlen(bundleID),
										kAutoGenerateReturnID,
										kAnyTransactionID,
										&evt, NULL,
										"'----':utf8(@)", strlen(utf8Script),
										utf8Script);
				if(err == noErr)
				{
					// Send event and check for any Apple Event Manager errors
					err = AESendMessage(&evt, &res, kAENoReply, kAEDefaultTimeout);
					AEDisposeDesc(&evt);
					if(err)
					{
						UnrealError = @"Couldn't tell Terminal to generate project files.";
					}
				}
				else
				{
					UnrealError = @"Couldn't tell Terminal to generate project files.";
				}
			}
			else
			{
				UnrealError = @"Failed to open Terminal while trying to generate project files.";
			}
		}
		else
		{
			UnrealError = @"No application to generate project files available.";
		}
    }
	else
	{
		UnrealError = @"No valid project file selected.";
	}
	
	if(error)
	{
		*error = UnrealError;
	}
}

@end
