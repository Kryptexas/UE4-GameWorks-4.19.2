// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#import <Cocoa/Cocoa.h>

@interface FUE4EditorServicesAppDelegate : NSObject <NSApplicationDelegate>

@property (assign) IBOutlet NSWindow* Window;
@property (assign) IBOutlet NSPopUpButton* EditorMenu;
@property (assign) IBOutlet NSButton* CancelButton;
@property (assign) IBOutlet NSButton* OKButton;

- (IBAction)onCancelButtonPressed:(id)Sender;
- (IBAction)onOKButtonPressed:(id)Sender;

- (void)launchGameService:(NSPasteboard *)PBoard userData:(NSString *)UserData error:(NSString **)Error;
- (void)generateXcodeProjectService:(NSPasteboard *)PBoard userData:(NSString *)UserData error:(NSString **)Error;
- (void)switchUnrealEngineVersionService:(NSPasteboard *)PBoard userData:(NSString *)UserData error:(NSString **)Error;

@end
