//  Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
//
//  FlipsideViewController.m
//  UDKRemote
//
//  Created by jadams on 7/28/10.
//

#import "FlipsideViewController.h"
#import "UDKRemoteAppDelegate.h"
#import "MainViewController.h"


@implementation FlipsideViewController

@synthesize Delegate;
@synthesize DestIPCell;
@synthesize PortCell;
@synthesize TiltCell;
@synthesize TouchCell;
@synthesize LockCell;
@synthesize SubIPTextField;
@synthesize PortTextField;
@synthesize SubTiltSwitch;
@synthesize SubTouchSwitch;
@synthesize SubLockSwitch;
@synthesize MainSettingsTable;
@synthesize RecentComputersTable;
@synthesize RecentComputersController;
@synthesize ComputerListEdit;
@synthesize ComputerTextAlert;
@synthesize RecentPortsTable;
@synthesize RecentPortsController;
@synthesize PortListEdit;
@synthesize PortTextAlert;

#define MAX_NUMBER_PORTS 5

/** 
 * Enum to notify of which view mode we are in when manipulating the table data 
 */
enum EAlertViewMode
{
	SECTION_AddComputer = 0,
	SECTION_AddPort,
};
EAlertViewMode CurrentViewMode;


/**
 * Enum to describe each and how many sections are in the table
 */
enum ESections
{
	SECTION_Settings = 0,
	SECTION_MAX
};

/**
 * Enum to describe each and how many rows in section 0
 */
enum ESection0Rows
{
	SECTION0_IPAddr = 0,
	SECTION0_Port,
	SECTION0_Tilt,
	SECTION0_Touch,
	SECTION0_MAX_IPad,
	SECTION0_Lock=SECTION0_MAX_IPad,
	SECTION0_MAX
};


- (void)viewDidLoad
{
    [super viewDidLoad];
	
	AppDelegate = ((UDKRemoteAppDelegate*)[UIApplication sharedApplication].delegate);
}


- (void)SetPortString
{
	self.PortTextField.text = @"";
	
	for( int i = 0; i < [AppDelegate.Ports count]; i++ )
	{
		self.PortTextField.text = [NSString stringWithFormat:@"%@%@%@", self.PortTextField.text, [AppDelegate.Ports objectAtIndex:i], i < [AppDelegate.Ports count]-1 ? @", " : @""];
	}
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
	
	// get settings from app
	self.SubIPTextField.text = AppDelegate.PCAddress;
	[self SetPortString];
	self.SubTiltSwitch.on = !AppDelegate.bShouldIgnoreTilt;
	self.SubTouchSwitch.on = !AppDelegate.bShouldIgnoreTouch;
	self.SubLockSwitch.on = AppDelegate.bLockOrientation;
	
	// make sure toolbar from adding comptuer page is hidden
	[self setToolbarHidden:YES];
}


- (IBAction)done 
{
	// write settings back to app
	AppDelegate.bShouldIgnoreTilt = !self.SubTiltSwitch.on;
	AppDelegate.bShouldIgnoreTouch = !self.SubTouchSwitch.on;
	AppDelegate.bLockOrientation = self.SubLockSwitch.on;
	if (AppDelegate.bLockOrientation)
	{
		AppDelegate.LockedOrientation = [UIApplication sharedApplication].statusBarOrientation;
	}

	// always save the tilt setting
	[[NSUserDefaults standardUserDefaults] setObject:AppDelegate.PCAddress forKey:@"PCAddress"];
	[[NSUserDefaults standardUserDefaults] setBool:AppDelegate.bShouldIgnoreTilt forKey:@"bShouldIgnoreTilt"];
	[[NSUserDefaults standardUserDefaults] setBool:AppDelegate.bShouldIgnoreTouch forKey:@"bShouldIgnoreTouch"];
	[[NSUserDefaults standardUserDefaults] setBool:AppDelegate.bLockOrientation forKey:@"bLockOrientation"];
	[[NSUserDefaults standardUserDefaults] setInteger:(int)AppDelegate.LockedOrientation forKey:@"LockedOrientation"];
	[[NSUserDefaults standardUserDefaults] setObject:AppDelegate.RecentComputers forKey:@"RecentComputers"];
	[[NSUserDefaults standardUserDefaults] setObject:AppDelegate.Ports forKey:@"Ports"];
	[[NSUserDefaults standardUserDefaults] synchronize];

	[self.Delegate flipsideViewControllerDidFinish:self];	
}

- (void)popoverControllerDidDismissPopover:(UIPopoverController *)popoverController
{
	[self done];
}

- (IBAction)AddComputer
{
	// make the dialog
	self.ComputerTextAlert = [[UIAlertView alloc] initWithTitle:@"Add New Computer"
														message:@""
													   delegate:self
											  cancelButtonTitle:NSLocalizedString(@"Cancel", nil)
											  otherButtonTitles:NSLocalizedString(@"OK", nil), nil];
	self.ComputerTextAlert.alertViewStyle = UIAlertViewStylePlainTextInput;

	// modify the text box
	[self.ComputerTextAlert textFieldAtIndex:0].keyboardType = UIKeyboardTypeURL;
	[self.ComputerTextAlert textFieldAtIndex:0].placeholder = @"IP address or name";

	// remember our mode
	CurrentViewMode = SECTION_AddComputer;

	[self.ComputerTextAlert show];
	[self.ComputerTextAlert release];
}


- (IBAction)AddPort
{
	if( [AppDelegate.Ports count] == MAX_NUMBER_PORTS )
	{
		UIAlertView* TooManyPortsAlert = [[UIAlertView alloc] initWithTitle:@"Alert" message:@"You have reached the maximum number of Ports" delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil];
		[TooManyPortsAlert show];
		[TooManyPortsAlert release];
		return;
	}

	// make the dialog
	self.PortTextAlert = [[UIAlertView alloc] initWithTitle:@"Add New Port"
														message:@""
													   delegate:self
											  cancelButtonTitle:NSLocalizedString(@"Cancel", nil)
											  otherButtonTitles:NSLocalizedString(@"OK", nil), nil];
	self.PortTextAlert.alertViewStyle = UIAlertViewStylePlainTextInput;
	
	// modify the text box
	[self.PortTextAlert textFieldAtIndex:0].keyboardType = UIKeyboardTypeNumberPad;
	[self.PortTextAlert textFieldAtIndex:0].placeholder = @"41765,41766 are game/editor defaults";
	
	// remember our mode
	CurrentViewMode = SECTION_AddPort;
	
	[self.PortTextAlert show];
	[self.PortTextAlert release];
}


/**
 * Handle pressing OK or Return on the keyboard
 */
- (void)ProcessOK:(UITextField*)TextField
{
	NSString* EnteredText;
	
	bool AlreadyAnEntry = NO;
	
	switch ( CurrentViewMode )
	{
		case SECTION_AddComputer:
			
			// get the text the user entered
			EnteredText = TextField.text;
			
			// check to make sure it's not already there
			for (NSString* Existing in AppDelegate.RecentComputers)
			{
				if ([Existing compare:EnteredText] == NSOrderedSame)
				{
					AlreadyAnEntry = YES;
					break;
				}
			}
			
			// add it if it wasn't already there
			if (!AlreadyAnEntry)
			{
				[AppDelegate.RecentComputers addObject:EnteredText];
			}
			
			// even if it was already there, use it as the current address
			AppDelegate.PCAddress = EnteredText;
			self.SubIPTextField.text = AppDelegate.PCAddress;
			[self.RecentComputersTable reloadData];	
			break;
		case SECTION_AddPort:
			
			// get the text the user entered
			NSString* EnteredPortText = TextField.text;
			
			// check to make sure it's not already there
			for (NSString* Existing in AppDelegate.Ports)
			{
				if ([Existing compare:EnteredPortText] == NSOrderedSame)
				{
					AlreadyAnEntry = YES;
					break;
				}
			}
			
			// add it if it wasn't already there
			if (!AlreadyAnEntry)
			{
				[AppDelegate.Ports addObject:EnteredPortText];
			}
			
			// even if it was already there, use it as the current address
			[self SetPortString];
			[self.RecentPortsTable reloadData];
			break;
	}
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
	[self.ComputerTextAlert dismissWithClickedButtonIndex:1 animated:YES];
	[self.PortTextAlert dismissWithClickedButtonIndex:1 animated:YES];
	return NO;
}

- (void)alertView:(UIAlertView*)AlertView willDismissWithButtonIndex:(NSInteger)ButtonIndex
{
//	// let go of the keyboard before dismissing (to avoid "wait_fences: failed to receive reply: 10004003")
//	[self.NewComputerTextField resignFirstResponder];
//	[self.NewPortTextField resignFirstResponder];
}

- (void)alertView:(UIAlertView*)AlertView didDismissWithButtonIndex:(NSInteger)ButtonIndex
{
	// on OK, do something (on cancel, do nothing)
	if (ButtonIndex == 1)
	{
		[self ProcessOK:[AlertView textFieldAtIndex:0]];
	}
	
	self.ComputerTextAlert = nil;
	self.PortTextAlert = nil;
}

- (IBAction)EditList
{
	if( CurrentViewMode == SECTION_AddComputer )
	{
		// toggle editing mode
		self.RecentComputersTable.editing = !self.RecentComputersTable.editing;
	
		if (self.RecentComputersTable.editing)
		{
			[self.ComputerListEdit setTitle:@"Done"];
		}
		else
		{
			[self.ComputerListEdit setTitle:@"Edit"];
			[RecentComputersTable reloadData];
		}
	}
	else if( CurrentViewMode == SECTION_AddPort )
	{
		// toggle editing mode
		self.RecentPortsTable.editing = !self.RecentPortsTable.editing;
		
		if (self.RecentPortsTable.editing)
		{
			[self.PortListEdit setTitle:@"Done"];
		}
		else
		{
			[self.PortListEdit setTitle:@"Edit"];
			[RecentPortsTable reloadData];
		}
	}
}

- (IBAction)CalibrateTilt
{
	[AppDelegate.mainViewController CalibrateTilt];
}


- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
	return SECTION_MAX;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
	if (tableView == MainSettingsTable)
	{
		// return how many rows for each section
		switch (section)
		{
			case SECTION_Settings:
				return (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad) ? SECTION0_MAX_IPad : SECTION0_MAX;
				
				// more sections here
		}		
	}
	else if (tableView == RecentComputersTable)
	{
		return [AppDelegate.RecentComputers count];
	}
	else if (tableView == RecentPortsTable)
	{
		return [AppDelegate.Ports count];
	}	
	return 0;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
	if (tableView == MainSettingsTable)
	{
		// return a unique cell for each row
		switch (indexPath.section)
		{
			case SECTION_Settings:
				switch (indexPath.row)
				{
					case SECTION0_IPAddr:
						return DestIPCell;
					case SECTION0_Port:
						return PortCell;
					case SECTION0_Tilt:
						return TiltCell;
					case SECTION0_Touch:
						return TouchCell;
					case SECTION0_Lock:
						return LockCell;
				}
				break;
				
			// more sections here
		}
	}
	else if (tableView == RecentComputersTable)
	{
		// get the computer name from our list of names
		NSString* ComputerName = [AppDelegate.RecentComputers objectAtIndex:indexPath.row];

		// reuse or create a cell for the name
		UITableViewCell* Cell = [RecentComputersTable dequeueReusableCellWithIdentifier:ComputerName];
		if (Cell == nil)
		{
			Cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:ComputerName] autorelease];
			Cell.textLabel.text = ComputerName;
		}
		Cell.accessoryType = ([AppDelegate.PCAddress compare:ComputerName] == NSOrderedSame) ? UITableViewCellAccessoryCheckmark : UITableViewCellAccessoryNone;
		
		return Cell;
	}
	else if (tableView == RecentPortsTable)
	{
		// get the computer name from our list of names
		NSString* PortName = [AppDelegate.Ports objectAtIndex:indexPath.row];
		
		// reuse or create a cell for the name
		UITableViewCell* Cell = [RecentPortsTable dequeueReusableCellWithIdentifier:PortName];
		if (Cell == nil)
		{
			Cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:PortName] autorelease];
			Cell.textLabel.text = PortName;
		}
		Cell.accessoryType = UITableViewCellAccessoryCheckmark;
		
		return Cell;
	}
	
	return nil;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath*)indexPath
{
	// if user clicked on the address cell, we need to go to a submenu to handle it
	if (tableView == MainSettingsTable && indexPath.section == SECTION_Settings)
	{
		CurrentViewMode = SECTION_AddComputer;
		[tableView deselectRowAtIndexPath:indexPath animated:YES];
		[self setToolbarHidden:NO animated:NO];
		
		if( indexPath.row == SECTION0_IPAddr )
		{
			[self pushViewController:RecentComputersController animated:YES];
		
			// auto prompt for new computer if none exist
			if ([AppDelegate.RecentComputers count] == 0)
			{
				[self AddComputer];
			}
		}
		else if( indexPath.row == SECTION0_Port )
		{
			CurrentViewMode = SECTION_AddPort;
			[self pushViewController:RecentPortsController animated:YES];
			
			// auto prompt for new port if none exist
			if ([AppDelegate.Ports count] == 0)
			{
				[self AddPort];
			}
		}
	}
	// if the user clicked on a cell in the computers list, then use that as the current address
	else if (tableView == RecentComputersTable)
	{
		AppDelegate.PCAddress = [AppDelegate.RecentComputers objectAtIndex:indexPath.row];
		self.SubIPTextField.text = AppDelegate.PCAddress;
		[self setToolbarHidden:YES animated:NO];
		[RecentComputersTable reloadData];

		// we can now close this list
		[self popViewControllerAnimated:YES];
	}
	// if the user clicked on a cell in the ports list, then use that as the current port
	else if (tableView == RecentPortsTable)
	{
		[self SetPortString];
		
		[self setToolbarHidden:YES animated:NO];
		[RecentPortsTable reloadData];
		
		// we can now close this list
		[self popViewControllerAnimated:YES];
	}
}

- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
	return tableView == RecentComputersTable || tableView == RecentPortsTable;
}

- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
{
	// delete a row
	if (editingStyle == UITableViewCellEditingStyleDelete)
	{
		if( tableView == RecentComputersTable )
		{
			// did we delete the selected computer
			NSString* Computer = [AppDelegate.RecentComputers objectAtIndex:indexPath.row];
			bool bRemovedSelected = [Computer compare:AppDelegate.PCAddress] == NSOrderedSame;
		
			// remove the computer from our known list
			[AppDelegate.RecentComputers removeObjectAtIndex:indexPath.row];
		
			// select a computer if we deleted the selected one
			if (bRemovedSelected)
			{
				if ([AppDelegate.RecentComputers count] > 0)
				{
					AppDelegate.PCAddress = [AppDelegate.RecentComputers objectAtIndex:0];
				}
				else 
				{
					AppDelegate.PCAddress = @"";
				}

			}
		}
		else if( tableView == RecentPortsTable )
		{
			// remove the computer from our known list
			[AppDelegate.Ports removeObjectAtIndex:indexPath.row];
			[self SetPortString];
		}
		
		NSArray* Rows = [NSArray arrayWithObject:indexPath];
		[tableView deleteRowsAtIndexPaths:Rows withRowAnimation:UITableViewRowAnimationBottom];
	}
	
}

- (void)didReceiveMemoryWarning {
	// Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
	
	// Release any cached data, images, etc that aren't in use.
}


- (void)viewDidUnload 
{
	// Release any retained subviews of the main view.
	self.DestIPCell = nil;
	self.TiltCell = nil;
	self.PortCell = nil;
	self.TouchCell = nil;
	self.LockCell = nil;
}



// Override to allow orientations other than the default portrait orientation.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation 
{
	// Return YES for supported orientations
	return YES;//(interfaceOrientation == UIInterfaceOrientationLandscapeRight);
}

- (void)dealloc 
{
    [super dealloc];
}


@end
