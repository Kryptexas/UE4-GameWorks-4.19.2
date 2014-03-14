// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#import "UE4EditorServicesAppDelegate.h"

@implementation FUE4EditorServicesAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
	// Insert code here to initialize your application
	[NSApp setServicesProvider:self];
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
		
		NSURL* BundleURL = [[NSWorkspace sharedWorkspace] URLForApplicationToOpenURL:[NSURL fileURLWithPath: FilePath]];
		
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
		
		NSURL* BundleURL = [[NSWorkspace sharedWorkspace] URLForApplicationToOpenURL:[NSURL fileURLWithPath: FilePath]];
		
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
