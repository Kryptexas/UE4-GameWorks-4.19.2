// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#import <Cocoa/Cocoa.h>

@interface FUE4EditorServicesAppDelegate : NSObject <NSApplicationDelegate>

- (void)launchGameService:(NSPasteboard *)pboard userData:(NSString *)userData error:(NSString **)error;
- (void)generateXcodeFilesService:(NSPasteboard *)pboard userData:(NSString *)userData error:(NSString **)error;
//- (void)generateAllXcodeFilesService:(NSPasteboard *)pboard userData:(NSString *)userData error:(NSString **)error;

@end
