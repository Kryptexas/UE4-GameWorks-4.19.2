//  Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
//
//  FlipsideViewController.h
//  UDKRemote
//
//  Created by jadams on 7/28/10.
//

#import <UIKit/UIKit.h>

@protocol FlipsideViewControllerDelegate;
@class UDKRemoteAppDelegate;

@interface FlipsideViewController : UINavigationController <UITextFieldDelegate, UIPopoverControllerDelegate>
{
	id <FlipsideViewControllerDelegate> Delegate;
	
	/** Cached app delegate */
	UDKRemoteAppDelegate* AppDelegate;
}

@property (nonatomic, assign) IBOutlet id <FlipsideViewControllerDelegate> Delegate;

/** Table cell types */
@property (nonatomic, retain) IBOutlet UITableViewCell* DestIPCell;
@property (nonatomic, retain) IBOutlet UITableViewCell* PortCell;
@property (nonatomic, retain) IBOutlet UITableViewCell* TiltCell;
@property (nonatomic, retain) IBOutlet UITableViewCell* TouchCell;
@property (nonatomic, retain) IBOutlet UITableViewCell* LockCell;

/** Useful objects inside the table view cells */
@property (nonatomic, assign) IBOutlet UITextField* SubIPTextField;
@property (nonatomic, assign) IBOutlet UITextField* PortTextField;
@property (nonatomic, assign) IBOutlet UISwitch* SubTiltSwitch;
@property (nonatomic, assign) IBOutlet UISwitch* SubTouchSwitch;
@property (nonatomic, assign) IBOutlet UISwitch* SubLockSwitch;

/** The table views in the hierarchy */
@property (nonatomic, retain) IBOutlet UITableView* MainSettingsTable;

/** Management of the computer address to broadcast to */
@property (nonatomic, retain) IBOutlet UITableView* RecentComputersTable;
@property (nonatomic, retain) IBOutlet UIViewController* RecentComputersController;
@property (nonatomic, assign) IBOutlet UIBarButtonItem* ComputerListEdit;
@property (nonatomic, retain) UIAlertView* ComputerTextAlert;

/** Management of the ports to broadcast to */
@property (nonatomic, retain) IBOutlet UITableView* RecentPortsTable;
@property (nonatomic, retain) IBOutlet UIViewController* RecentPortsController;
@property (nonatomic, assign) IBOutlet UIBarButtonItem* PortListEdit;
@property (nonatomic, retain) UIAlertView* PortTextAlert;

- (IBAction)done;
- (IBAction)AddComputer;
- (IBAction)AddPort;
- (IBAction)EditList;
- (IBAction)CalibrateTilt;

@end


@protocol FlipsideViewControllerDelegate
- (void)flipsideViewControllerDidFinish:(FlipsideViewController *)controller;
@end

