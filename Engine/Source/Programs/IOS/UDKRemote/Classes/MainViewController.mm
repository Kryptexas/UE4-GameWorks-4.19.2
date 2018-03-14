//  Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
//
//  MainViewController.m
//  UDKRemote
//
//  Created by jadams on 7/28/10.
//

#import "MainViewController.h"
#import "UDKRemoteAppDelegate.h"
//#import <MediaPlayer/MediaPlayer.h>
#include <netdb.h>          // getaddrinfo, struct addrinfo, AI_NUMERICHOST



/** Size of the ring buffer, MUST BE POWER OF TWO */
#define RING_BUFFER_NUM_ELEMENTS 1024

/** These data types must match up on the other side! */
enum EDataType
{
	DT_TouchBegan=0,
	DT_TouchMoved=1,
	DT_TouchEnded=2,
	DT_Tilt=3,
	DT_Motion=4,
	DT_Gyro=5,
	DT_Ping=6,
};

// start all messages with this, the other side can use to help verify the data
#define MESSAGE_MAGIC_ID 0xAB

// version of messages being sent
#define MESSAGE_VERSION 1

/** A single event message to send over the network */
struct FMessage
{
	unsigned char MagicTag; // verification byte
	unsigned char MessageVersion; // version of the message, for compatibility going forward
	unsigned short MessageID;
	unsigned char DataType;
	unsigned char Handle;
	unsigned char DeviceOrientation;
	unsigned char UIOrientation;
	float Data[12]; // enough space for 4 vectors for CMMotion
};


struct FMessageQueue
{
	/** The finger this queue is for */
	unsigned char FinderIndex;
	
	/** A queue of messages (0 is always a TouchBegan, queue moves up to a TouchEnd) */
	FMessage Queue[64];
};


@interface MainViewController()
- (void)OnMotionTimer:(NSTimer*)Timer;
- (void)OnPingTimer:(NSTimer*)Timer;
- (void)PushData:(EDataType)DataType Data:(float*)Data DataSize:(int)DataSize UniqueTouchHandle:(unsigned char)Handle;
@end

@implementation MainViewController

@synthesize HostNameLabel;
@synthesize ResolvedNameLabel;
@synthesize HelpLabel;
@synthesize NavController;
@synthesize InfoButton;
@synthesize MotionManager;
@synthesize ReferenceAttitude;
@synthesize MotionTimer;
@synthesize PingTimer;
@synthesize ResolvedAddrString;
@synthesize ReceiveData;
@synthesize Background;
@synthesize FlipsidePopoverController;



/** Ring buffer of elements so that we can go back in time to resend events if needed */
static FMessage GRingBuffer[RING_BUFFER_NUM_ELEMENTS];

static void SocketDataCallback(CFSocketRef Socket, CFSocketCallBackType CallbackType, CFDataRef Addr, const void* Data, void* UserInfo);

/**
 * Helper function to get the formatted port string
 */
-(NSString*) GetPortsString
{
	NSString* OutPortsString = [[NSString alloc]initWithString:@""];
	for( int i = 0; i < [AppDelegate.Ports count]; i++ )
	{
		OutPortsString = [NSString stringWithFormat:@"%@%@", OutPortsString, [AppDelegate.Ports objectAtIndex:i]];
		if( i < [AppDelegate.Ports count]-1 )
		{
			OutPortsString = [NSString stringWithFormat:@"%@%@", OutPortsString, @", "];
		}
	}
	return OutPortsString;
}

- (void)SpinUpThreads
{
	// create the send socket
	if (PushSocket != NULL)
	{
		CFSocketInvalidate(PushSocket);
		CFRelease(PushSocket);
	}
	PushSocket = CFSocketCreate(NULL, 0, SOCK_DGRAM, 0, kCFSocketNoCallBack, NULL, NULL);
	
	// make it so when a socket dies on device sleep, we get an error when using it instead of a SIGPIPE exception
	int NoSigPipe = 1;
	setsockopt(CFSocketGetNative(PushSocket), SOL_SOCKET, SO_NOSIGPIPE, &NoSigPipe, sizeof(NoSigPipe));
	
	// create the listen socket
	if (ReplySocket != NULL)
	{
		CFSocketInvalidate(ReplySocket);
		CFRelease(ReplySocket);
	}
	CFSocketContext Context = { 0, (__bridge void*)self, NULL, NULL, NULL };
	ReplySocket = CFSocketCreate(NULL, 0, SOCK_DGRAM, 0, kCFSocketDataCallBack, SocketDataCallback, &Context);
	
	
	// bind to the port
	sockaddr_in ServerAddress;
	memset(&ServerAddress, 0, sizeof(ServerAddress));
	ServerAddress.sin_len = sizeof(ServerAddress);
	ServerAddress.sin_family = AF_INET;
	// @todo: we should use port 0 and let it choose a port, then send it to the PC every second or something (using CFSocketCopyAddress to get the address that contains the port)
	ServerAddress.sin_port = htons(41764);
	ServerAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	NSData *AddressData = [NSData dataWithBytes:&ServerAddress length:sizeof(ServerAddress)];
	if (CFSocketSetAddress(ReplySocket, (__bridge CFDataRef)AddressData) != kCFSocketSuccess)
	{
		NSLog(@"Failed to set addr");
	}
	
	// hook uip RunLoop source for our callbacks, etc
	CFRunLoopRef RunLoop = CFRunLoopGetCurrent();
	if (ReplySource != NULL)
	{
		CFRunLoopRemoveSource(RunLoop, ReplySource, kCFRunLoopCommonModes);
		CFRelease(ReplySource);
	}
	ReplySource = CFSocketCreateRunLoopSource(kCFAllocatorDefault, ReplySocket, 0);
	CFRunLoopAddSource(RunLoop, ReplySource, kCFRunLoopCommonModes);
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    if ((self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil])) 
	{
		// initialization
		SocketAddrData[0] = SocketAddrData[1] = SocketAddrData[2] = SocketAddrData[3] = SocketAddrData[4] = NULL;
		AppDelegate = ((UDKRemoteAppDelegate*)[UIApplication sharedApplication].delegate);
		
		// make sure arrays are NULLed out
		bzero(AllTouches, sizeof(AllTouches));

		// load the images and make views
		for (int Index = 0; Index < ARRAY_COUNT(TouchImageViews); Index++)
		{
			// load the image
			UIImage* Image = [UIImage imageNamed:[NSString stringWithFormat:@"Finger%d.png", Index]];
			TouchImageViews[Index] = [[UIImageView alloc] initWithImage:Image];
			
			// add it to as a subview
			[self.view addSubview:TouchImageViews[Index]];
			TouchImageViews[Index].hidden = YES;
		}

		PushSocket = ReplySocket = NULL;
		ReplySource = NULL;
		[self SpinUpThreads];

		// start up a timer to ping other end
		self.PingTimer = [NSTimer scheduledTimerWithTimeInterval:3.0f target:self selector:@selector(OnPingTimer:) userInfo:nil repeats:YES];		

		// gyro is now a required capability, so just use it
//		assert(MotionManager.deviceMotionAvailable);
		MotionManager.deviceMotionUpdateInterval = 1.0 / 30.0;
		[MotionManager startDeviceMotionUpdates];

		// turn on the gyro if it's there
		if (MotionManager.gyroAvailable)
		{
			[MotionManager startGyroUpdates];
		}

		// start up a timer to pull CM data from
		self.MotionTimer = [NSTimer scheduledTimerWithTimeInterval:MotionManager.deviceMotionUpdateInterval target:self selector:@selector(OnMotionTimer:) userInfo:nil repeats:YES];
    }
	
    return self;
}


/*
// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
	[super viewDidLoad];
}
*/


- (void)flipsideViewControllerDidFinish:(FlipsideViewController *)controller 
{
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) 
	{
        [self dismissViewControllerAnimated:YES completion:nil];
    }
	else
	{
        [self.FlipsidePopoverController dismissPopoverAnimated:YES];
    }

	// resolve any new addresses
	[self UpdateSocketAddr];
}

- (void)viewWillAppear:(BOOL)animated
{
	[super viewWillAppear:animated];
	
	// resolve any new addresses
	[self UpdateSocketAddr];
}


-(void)FlipController:(BOOL)bIsAnimated sender:(id)sender
{
	/*
	 NSURL* URL = [NSURL URLWithString:@"http://qthttp.akamai.com.edgesuite.net/iphone_demo/Video_Content/discovery/B02C36_12682301401197_Surrounded_By_Storms/hd/B02C36_12682301401197_Surrounded_By_Storms.mov"];
	 MPMoviePlayerController* MoviePlayer = [[MPMoviePlayerController alloc] initWithContentURL:URL];
	 MoviePlayer.fullscreen = YES;
	 [[MoviePlayer view] setFrame:self.view.frame];
	 [self.view addSubview:[MoviePlayer view]];
	 [MoviePlayer play];
	 */
	
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone)
	{
		[self presentViewController:self.NavController animated:bIsAnimated completion:nil];
		//        UIViewController *controller = [[UIViewController alloc] initWithNibName:@"UIViewController" bundle:nil];
		//        [self presentModalViewController:controller animated:YES];
    }
	else
	{
        if (!self.FlipsidePopoverController)
		{
			//            UIViewController *controller = [[UIViewController alloc] initWithNibName:@"Flipside" bundle:nil];
			//            self.flipsidePopoverController = [[UIPopoverController alloc] initWithContentViewController:controller];
            
            self.FlipsidePopoverController = [[UIPopoverController alloc] initWithContentViewController:self.NavController];
			self.FlipsidePopoverController.delegate = (FlipsideViewController*)self.NavController;
        }
		
        if ([self.FlipsidePopoverController isPopoverVisible])
		{
            [self.FlipsidePopoverController dismissPopoverAnimated:bIsAnimated];
        }
		else
		{
            [self.FlipsidePopoverController presentPopoverFromRect:[sender frame] inView:self.view permittedArrowDirections:UIPopoverArrowDirectionAny animated:bIsAnimated];
        }
    }
	
	
	//	[self presentModalViewController:self.NavController animated:YES];

}

- (IBAction)showInfo:(id)sender
{
	[self FlipController:YES sender:sender];
}
- (void)didReceiveMemoryWarning 
{
	// Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
	
	// Release any cached data, images, etc. that aren't in use.
}



// Override to allow orientations other than the default portrait orientation.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)InterfaceOrientation 
{
	// does user want to allow orientation changes?
	if (AppDelegate.bLockOrientation)
	{
		// if so, return yes for the current orientation (caller wants at least one to return YES)
		return InterfaceOrientation == AppDelegate.LockedOrientation; 
	}
	return YES;
}











- (void)CleanupResolution
{
	// make sure the host isn't scheduled anymore
    CFHostUnscheduleFromRunLoop(ResolvingHost, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
	CFHostSetClient(ResolvingHost, NULL, NULL);
	
	CFRelease(ResolvingHost);
	ResolvingHost = NULL;
}

/**
 * Member function when resolution finishes
 */
- (void)OnResolveComplete:(const CFStreamError*) Error
{
	// did it succeed?
	if (Error->error == noErr)
	{
		// get the resolved addresses array
		Boolean bWasResolved = NO;
		CFArrayRef Addresses = CFHostGetAddressing(ResolvingHost, &bWasResolved);
		if (bWasResolved)
		{
			int NumAddresses = (int)CFArrayGetCount(Addresses);
			
			// look for the first IPv4 address
			for (int AddressIndex = 0; AddressIndex < NumAddresses; AddressIndex++)
			{
				
				// just use the last address (it should be IPv4)
				CFDataRef AddressData = (CFDataRef)CFArrayGetValueAtIndex(Addresses,  AddressIndex);
				
				// get the addr from the address list, and add a port number
				sockaddr* ResolvedSockAddr = (struct sockaddr *)CFDataGetBytePtr(AddressData);
				
				// is this IPv4?
				if (ResolvedSockAddr->sa_family == AF_INET)
				{
					sockaddr_in ResolvedAddrWithPort;
					memcpy(&ResolvedAddrWithPort, ResolvedSockAddr, ResolvedSockAddr->sa_len);
					
					UInt8 PortCount = [AppDelegate.Ports count];
					if( PortCount > 0 )
					{
						for( int i = 0; i < [AppDelegate.Ports count]; i++ )
						{
							short Port = [[AppDelegate.Ports objectAtIndex:i] intValue];
							ResolvedAddrWithPort.sin_port = htons(Port);
					
							// make a CFData object usable by CFSocket to send to
							SocketAddrData[i] = CFDataCreate(NULL, (UInt8*)&ResolvedAddrWithPort, ResolvedAddrWithPort.sin_len);
						}							
					}
					
					// turn it into a string
					char AddressBuffer[128];
					getnameinfo((sockaddr*)&ResolvedAddrWithPort, ResolvedAddrWithPort.sin_len, AddressBuffer, 128, NULL, 0, NI_NUMERICHOST);
					
					self.ResolvedAddrString = [NSString stringWithFormat:@"%s", AddressBuffer];

								
					// put string into the label
					self.ResolvedNameLabel.textColor = [UIColor yellowColor];
					
					NSString* PortsString = [self GetPortsString];//[[NSString alloc] initWithString: @""];
					

					self.ResolvedNameLabel.text = [NSString stringWithFormat:@"%@:%@ [Waiting for connection...]", self.ResolvedAddrString, PortsString];
					self.HelpLabel.textColor = [UIColor cyanColor];
					if (AppDelegate.bShouldIgnoreTilt && !AppDelegate.bShouldIgnoreTouch)
					{
						self.HelpLabel.text = @"Sending only TOUCH info...";
					}
					else if (!AppDelegate.bShouldIgnoreTilt && AppDelegate.bShouldIgnoreTouch)
					{
						self.HelpLabel.text = @"Sending only TILT info...";
					}
					else if (AppDelegate.bShouldIgnoreTilt && AppDelegate.bShouldIgnoreTouch)
					{
						self.HelpLabel.textColor = [UIColor yellowColor];
						self.HelpLabel.text = @"Sending nothing, change settings...";
					}
					else
					{
						self.HelpLabel.text = @"Sending TOUCH and TILT info...";
					}
					
					// send a ping right away to get a reply
					bIsConnected = NO;

					[self PushData:DT_Ping Data:NULL DataSize:0 UniqueTouchHandle:0];
					
					break;
				}
			}
		}
	}
	else 
	{
		// update label
		self.ResolvedNameLabel.text = @"Failed to resolve!";
		self.HelpLabel.text = @"Unable to send data...";
		self.HelpLabel.textColor = [UIColor redColor];
	}
	
	[self CleanupResolution];
}


/**
 * Callback function when resolution finishes
 */
static void MyCFHostClientCallBack(CFHostRef Host, CFHostInfoType TypeInfo, const CFStreamError *Error, void *UserInfo)
{
	MainViewController* Controller = (__bridge MainViewController*)UserInfo;
	[Controller OnResolveComplete:Error];
}

/**
 * Resolve a network name to IP address
 */
- (BOOL)UpdateSocketAddr
{
	// stop any inflight resolution
	if (ResolvingHost)
	{
		CFHostCancelInfoResolution(ResolvingHost, kCFHostAddresses);
		
		[self CleanupResolution];
	}
	
	// kill the old data
	for (int i = 0; i < ARRAY_COUNT( SocketAddrData ); i++ )
	{
		SocketAddrData[i] = NULL;
	}

	// update the ip addr from the other view
	NSString* HostName = AppDelegate.PCAddress;
	
	// if it's the same do nothing
	//	if ([self.HostNameLabel.text isEqualToString:HostName])
	//	{
	//		return YES;
	//	}
	
	if (!HostName)
	{
		// update the labels
		self.HostNameLabel.text = @"None entered!";
		self.ResolvedNameLabel.text = @"";
		self.HelpLabel.text = @"Unable to send data...";
		self.HelpLabel.textColor = [UIColor yellowColor];
		return NO;
	}
	
	// update the labels
	self.HostNameLabel.text = HostName;
	self.ResolvedNameLabel.text = @"Resolving...";
	self.HelpLabel.text = @"Unable to send data...";
	self.HelpLabel.textColor = [UIColor yellowColor];
	
	// make a new host object to resolve
	ResolvingHost = CFHostCreateWithName(NULL, (__bridge CFStringRef)HostName);
	
	// set up async resolution callback
    CFHostClientContext Context = { 0, (__bridge void*)self, CFRetain, CFRelease, NULL };
	if (!CFHostSetClient(ResolvingHost, MyCFHostClientCallBack, &Context))
	{
		self.ResolvedNameLabel.text = @"Failed to setup resolution...";
		self.HelpLabel.text = @"Unable to send data...";
		self.HelpLabel.textColor = [UIColor redColor];
		[self CleanupResolution];
		return NO;
	}
	
	// schedule the async operation
	CFHostScheduleWithRunLoop(ResolvingHost, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
	
	// perform asynchronous lookup
	CFStreamError Error;
	if (!CFHostStartInfoResolution(ResolvingHost, kCFHostAddresses, &Error))
	{
		self.ResolvedNameLabel.text = @"Failed to start resolution...";
		self.HelpLabel.text = @"Unable to send data...";
		self.HelpLabel.textColor = [UIColor redColor];
		[self CleanupResolution];
		return NO;
	}
	
	return YES;
}



/**
 * Process received data
 */
-(void)OnReceivedData:(NSData*)NetData FromAddr:(const NSData*)Addr
{
	// we have a connection if we got any data
	bIsConnected = YES;
	// reset how long it's been since we got a ping
	PingsWithoutReply = 0;

	if ([NetData length] == 5)
	{
		char AddressBuffer[128];
		getnameinfo((sockaddr*)[Addr bytes], (int)[Addr length], AddressBuffer, 128, NULL, 0, NI_NUMERICHOST);
		
		// put string into the label
		self.ResolvedNameLabel.textColor = [UIColor cyanColor];
		NSString* PortsString = [self GetPortsString];
		
		self.ResolvedNameLabel.text = [NSString stringWithFormat:@"Connected to %s:%@", AddressBuffer, PortsString];		
		
		return;
	}

	static int NumChunksWaiting = 0;
	int* IntData = (int*)[NetData bytes];
	if ([NetData length] > 4 && IntData[0] == 0xFFEEDDCC)
	{
		NumChunksWaiting = IntData[1];
		[self.ReceiveData setLength:0];
	}
	// get a chunk
	else if (NumChunksWaiting)
	{
		NumChunksWaiting--;
		
		if (self.ReceiveData == nil)
		{
			self.ReceiveData = [[NSMutableData alloc] init];
		}
		[self.ReceiveData appendData:NetData];
		
		// did we get em all?
		if (NumChunksWaiting == 0)
		{
			Background.image = [UIImage imageWithData:ReceiveData];
			Background.contentMode = UIViewContentModeScaleToFill;
//			Background.bounds = Background.superview.frame;
			Background.opaque = YES;
		}
	}
}

/**
 * Callback when data comes in from other end
 */
static void SocketDataCallback(CFSocketRef Socket, CFSocketCallBackType CallbackType, CFDataRef Addr, const void* Data, void* UserInfo)
{
	MainViewController* Controller = (__bridge MainViewController*)UserInfo;
	[Controller OnReceivedData:(__bridge NSData*)Data FromAddr:(__bridge NSData*)Addr];
}

/**
 * Called every once in awhile to handle connections
 */
- (void)OnPingTimer:(NSTimer*)Timer
{
	PingsWithoutReply++;
	
	if (bIsConnected && PingsWithoutReply == 3)
	{
		bIsConnected = NO;
		self.ResolvedNameLabel.textColor = [UIColor yellowColor];
		
		NSString* PortsString = [self GetPortsString];
		
		self.ResolvedNameLabel.text = [NSString stringWithFormat:@"%@:%@ [Waiting for connection...]", self.ResolvedAddrString, PortsString];
	}
	
	// send a ping to PC
	[self PushData:DT_Ping Data:NULL DataSize:0 UniqueTouchHandle:0]; 
}








/**
 * Send data over the network
 * 
 * @param DataType What kind of data to send
 * @param Data1 DataType-specific data
 * @param Data2 DataType-specific data
 */
- (void)PushData:(EDataType)DataType Data:(float*)Data DataSize:(int)DataSize UniqueTouchHandle:(unsigned char)Handle
{
	// can only send if we have a valid address
	if (!bIsConnected && DataType != DT_Ping)
	{
		return;
	}
	
	static int RingBufferIndex = 0;

	// next message to send
	FMessage& Message = GRingBuffer[RingBufferIndex];
	
	// move to next element for next time this function is called
	RingBufferIndex = (RingBufferIndex + 1) & (RING_BUFFER_NUM_ELEMENTS - 1);
	
	// setup the data to push
	Message.MagicTag = MESSAGE_MAGIC_ID;
	Message.MessageVersion = 1;
	Message.MessageID = MessageID++;
	Message.DataType = DataType;
	Message.Handle = Handle;
	Message.DeviceOrientation = (unsigned char)[[UIDevice currentDevice] orientation];
	Message.UIOrientation = (unsigned char)[UIApplication sharedApplication].statusBarOrientation;
	if (Data && DataSize)
	{
		memcpy(Message.Data, Data, DataSize);		
	}
	
	// create NSData if needed
	if (PushData == nil)
	{
		PushData = [[NSMutableData alloc] initWithBytes:&Message length:sizeof(Message)];
	}
	else 
	{
		// replace the whole range
		NSRange Range = { 0, sizeof(FMessage) };
		[PushData replaceBytesInRange:Range withBytes:&Message];
	}
	
	// send over the network
	int NumTimesToSend = 1;
	// certain messages are very important, so we send them a few times
	// and it's up to the listener to discard extra copies
	if (DataType == DT_TouchBegan || DataType == DT_TouchEnded)
	{
		NumTimesToSend = 10;
	}
	
	// send as many times as we want
	for (int SendIndex = 0; SendIndex < NumTimesToSend; SendIndex++)
	{
		for( int i = 0; i < ARRAY_COUNT( SocketAddrData ) && SocketAddrData[i] != NULL; i++ )
		{
			// in the case that the socket was killed (from sleep), this will return -1, so try to recreate it
			if (CFSocketSendData(PushSocket, (CFDataRef)SocketAddrData[i], (__bridge CFDataRef)PushData, 0) == -1)
			{
				[self SpinUpThreads];
			}
		}
	}
}


/**
 * Returns the unique ID for the given touch
 */
-(unsigned char) GetTouchIndex:(UITouch*)Touch
{
	// look for existing touch
	for (int Index = 0; Index < ARRAY_COUNT(AllTouches); Index++)
	{
		if (AllTouches[Index] == Touch)
		{
			return Index;
		}
	}
	
	// if we get here, it's a new touch, find a slot
	for (int Index = 0; Index < ARRAY_COUNT(AllTouches); Index++)
	{
		if (AllTouches[Index] == nil)
		{
			AllTouches[Index] = Touch;
			return Index;
		}
	}
	
	// if we get here, that means we are trying to use more than 5 touches at once, which is an error
	return 255;
}

/**
 * Process a set of touches generically
 */
- (void)ProcessTouches:(NSSet*)Touches OfType:(EDataType)DataType
{
	// send each touch
	if (!AppDelegate.bShouldIgnoreTouch)
	{
		for (UITouch* Touch in Touches)
		{
			// send the touch coords in 0..1 range
			CGPoint Loc = [Touch locationInView:self.view];
			float Data[2];
			Data[0] = (float)Loc.x / (float)self.view.bounds.size.width;
			Data[1] = (float)Loc.y / (float)self.view.bounds.size.height;
			unsigned char TouchIndex = [self GetTouchIndex:Touch];
			[self PushData:DataType Data:Data DataSize:sizeof(Data) UniqueTouchHandle:TouchIndex];

			// draw the "fingerprints"
			if (TouchIndex < ARRAY_COUNT(TouchImageViews))
			{
				UIImageView* TouchImageView = TouchImageViews[TouchIndex];
				
				// put the image under the touch
				TouchImageView.center = Loc;

				// hide when user lets go
				if (DataType == DT_TouchEnded)
				{
					TouchImageView.hidden = YES;
					AllTouches[TouchIndex] = nil;
				}
				else 
				{
					TouchImageView.hidden = NO;
				}

			}
		}
	}
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	[self ProcessTouches:touches OfType:DT_TouchBegan];
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
	[self ProcessTouches:touches OfType:DT_TouchMoved];
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	[self ProcessTouches:touches OfType:DT_TouchEnded];
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
	[self ProcessTouches:touches OfType:DT_TouchEnded];
}


/**
 * Set the current tilt to be the "zero" rotation
 */
- (void)CalibrateTilt
{
	if (self.MotionManager != nil)
	{
		self.ReferenceAttitude = MotionManager.deviceMotion.attitude;
	}
	else 
	{
		bRecenterPitchAndRoll = YES;
	}
}

- (void)OnMotionTimer:(NSTimer*)Timer
{
	if (!AppDelegate.bShouldIgnoreTilt)
	{
		// get the CMMotion data
		CMAttitude* CurrentAttitude = MotionManager.deviceMotion.attitude;
		CMRotationRate CurrentRotationRate = MotionManager.deviceMotion.rotationRate;
		CMAcceleration CurrentGravity = MotionManager.deviceMotion.gravity;
		CMAcceleration CurrentUserAcceleration = MotionManager.deviceMotion.userAcceleration;
		
		if (ReferenceAttitude)
		{
			[CurrentAttitude multiplyByInverseOfAttitude:ReferenceAttitude];
		}
		
		// setup the message
		float Data[12];
		Data[0 * 3 + 0] = (float)CurrentAttitude.pitch;
		Data[0 * 3 + 1] = (float)CurrentAttitude.yaw;
		Data[0 * 3 + 2] = (float)CurrentAttitude.roll;
		
		Data[1 * 3 + 0] = (float)CurrentRotationRate.x;
		Data[1 * 3 + 1] = (float)CurrentRotationRate.y;
		Data[1 * 3 + 2] = (float)CurrentRotationRate.z;
		
		Data[2 * 3 + 0] = (float)CurrentGravity.x;
		Data[2 * 3 + 1] = (float)CurrentGravity.y;
		Data[2 * 3 + 2] = (float)CurrentGravity.z;
		
		Data[3 * 3 + 0] = (float)CurrentUserAcceleration.x;
		Data[3 * 3 + 1] = (float)CurrentUserAcceleration.y;
		Data[3 * 3 + 2] = (float)CurrentUserAcceleration.z;

		// send it!
		[self PushData:DT_Motion Data:Data DataSize:sizeof(Data) UniqueTouchHandle:0];
		
		
		// setup gyro message
		if (MotionManager.gyroAvailable)
		{
			CMRotationRate GyroData = MotionManager.gyroData.rotationRate;
			float Gyro[3];
			Gyro[0] = (float)GyroData.x;
			Gyro[1] = (float)GyroData.y;
			Gyro[2] = (float)GyroData.z;
			
			// send it
			[self PushData:DT_Gyro Data:Gyro DataSize:sizeof(Gyro) UniqueTouchHandle:0];
		}
	}
}

- (void)viewDidUnload 
{
	// Release any retained subviews of the main view.
	self.HostNameLabel = nil;
	self.ResolvedNameLabel = nil;
	self.HelpLabel = nil;
	self.FlipsidePopoverController = nil;
}


- (void)dealloc 
{
    [super dealloc];
	
	for (int Index = 0; Index < ARRAY_COUNT(TouchImageViews); Index++)
	{
		[TouchImageViews[Index] release];
	}
	
	[PushData release];
	CFRelease(PushSocket);
}

- (BOOL)prefersStatusBarHidden
{
	return YES;
}

@end
