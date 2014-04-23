// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#import <Cocoa/Cocoa.h>

@interface FUE4EditorServicesAppDelegate : NSObject <NSApplicationDelegate>

@property (assign) IBOutlet NSWindow* Window;
@property (assign) IBOutlet NSPopUpButton* EditorMenu;
@property (assign) IBOutlet NSButton* CancelButton;
@property (assign) IBOutlet NSButton* OKButton;
@property (assign) IBOutlet NSButton* DontShow;
@property (strong) NSMutableDictionary* DefaultApps;

- (IBAction)onCancelButtonPressed:(id)sender;
- (IBAction)onOKButtonPressed:(id)sender;

- (void)launchGameService:(NSPasteboard *)pboard userData:(NSString *)userData error:(NSString **)error;
- (void)generateXcodeFilesService:(NSPasteboard *)pboard userData:(NSString *)userData error:(NSString **)error;

@end
