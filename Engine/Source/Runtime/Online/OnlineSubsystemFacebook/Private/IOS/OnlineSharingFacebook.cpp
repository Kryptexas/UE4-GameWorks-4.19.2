// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


// Module includes
#include "OnlineSubsystemFacebookPrivatePCH.h"
#include "OnlineSharingFacebook.h"
#include "ImageCore.h"


FOnlineSharingFacebook::FOnlineSharingFacebook(class FOnlineSubsystemFacebook* InSubsystem)
{
	// Get our handle to the identity interface
	IdentityInterface = InSubsystem->GetIdentityInterface();
	SetupPermissionMaps();
}


FOnlineSharingFacebook::~FOnlineSharingFacebook()
{
	IdentityInterface = NULL;
}


void FOnlineSharingFacebook::SetupPermissionMaps()
{
	///////////////////////////////////////////////
	// Read Permissions

	ReadPermissionsMap.Add( EOnlineSharingReadCategory::Posts, [[NSArray alloc] 
		initWithObjects:@"read_stream", nil] );

	ReadPermissionsMap.Add( EOnlineSharingReadCategory::Friends, [[NSArray alloc] 
		initWithObjects:@"friends_about_me", @"friends_activities", @"friends_birthday", @"friends_checkins", 
						@"friends_education_history", @"friends_groups", @"friends_hometown", @"friends_interests", @"friends_likes",
						@"friends_notes", @"friends_interests", @"friends_likes", @"friends_notes", @"friends_online_presence", 
						@"friends_religion_politics", @"friends_videos", @"friends_status", @"friends_subscriptions", @"friends_online_presence",
						@"friends_events", @"friends_website", @"friends_work_history", @"read_friendlists", nil] );

	ReadPermissionsMap.Add( EOnlineSharingReadCategory::Mailbox, [[NSArray alloc] 
		initWithObjects:@"read_mailbox", nil] );

	ReadPermissionsMap.Add( EOnlineSharingReadCategory::OnlineStatus, [[NSArray alloc] 
		initWithObjects:@"user_status", @"user_online_presence", nil] );

	ReadPermissionsMap.Add( EOnlineSharingReadCategory::ProfileInfo, [[NSArray alloc] 
		initWithObjects:@"user_about_me", @"user_birthday", @"user_education_history", @"user_likes", @"user_work_history", 
						@"user_activities", @"user_interests", @"user_notes", @"user_videos", @"user_website", @"user_religion_politics",
						@"user_subscriptions", @"user_events", @"user_groups", @"read_requests", nil] );

	ReadPermissionsMap.Add( EOnlineSharingReadCategory::LocationInfo, [[NSArray alloc] 
		initWithObjects:@"user_checkins", @"user_hometown", nil] );


	///////////////////////////////////////////////
	// Publish Permissions
	PublishPermissionsMap.Add( EOnlineSharingPublishingCategory::Posts, [[NSArray alloc] 
		initWithObjects:@"publish_actions", @"publish_stream", nil] );
	
	PublishPermissionsMap.Add( EOnlineSharingPublishingCategory::Friends, [[NSArray alloc] 
		initWithObjects:@"manage_friendlists", nil] );
	
	PublishPermissionsMap.Add( EOnlineSharingPublishingCategory::AccountAdmin, [[NSArray alloc] 
		initWithObjects:@"manage_notifications", @"manage_pages", nil] );
	
	PublishPermissionsMap.Add( EOnlineSharingPublishingCategory::Events, [[NSArray alloc] 
		initWithObjects:@"create_event", @"rsvp_event", nil ] );
}


bool FOnlineSharingFacebook::RequestNewReadPermissions(int32 LocalUserNum, EOnlineSharingReadCategory::Type NewPermissions)
{
	bool bTriggeredRequest = false;
	if( IdentityInterface->GetLoginStatus(LocalUserNum) == ELoginStatus::LoggedIn )
	{
		bTriggeredRequest = true;

		dispatch_async(dispatch_get_main_queue(),^ 
			{
				// Fill in an nsarray with the permissions which match those that the user has set,
				// Here we iterate over each category, adding each individual permission linked with it in the ::SetupPermissionMaps
				NSMutableArray* Permissions = [[NSMutableArray alloc] init];

				for(FReadPermissionsMap::TIterator It(ReadPermissionsMap); It; ++It)
				{
					if((NewPermissions & It.Key()) != 0)
					{
						UE_LOG(LogOnline, Display, TEXT("ReadPermissionsMap[%i] - [%i]"), (int32)It.Key(), [It.Value() count]);
						for( NSString* RequestPermission in It.Value() )
						{
							[Permissions addObject:RequestPermission];
						}
					}
				}

				// Kick off the server request
				[FBSession.activeSession requestNewReadPermissions:Permissions
					completionHandler:^(FBSession *session, NSError *error)
					{
						TriggerOnRequestNewReadPermissionsCompleteDelegates(LocalUserNum, error==nil);
					}
				];
			}
		);
	}
	else
	{
		// If we weren't logged into Facebook we cannot do this action
		TriggerOnRequestNewReadPermissionsCompleteDelegates( LocalUserNum, false );
	}

	// We did kick off a request
	return bTriggeredRequest;
}


bool FOnlineSharingFacebook::RequestNewPublishPermissions(int32 LocalUserNum, EOnlineSharingPublishingCategory::Type NewPermissions, EOnlineStatusUpdatePrivacy::Type Privacy)
{
	bool bTriggeredRequest = false;
	if( IdentityInterface->GetLoginStatus(LocalUserNum) == ELoginStatus::LoggedIn )
	{
		bTriggeredRequest = true;

		dispatch_async(dispatch_get_main_queue(),^ 
			{
				// Fill in an nsarray with the permissions which match those that the user has set,
				// Here we iterate over each category, adding each individual permission linked with it in the ::SetupPermissionMaps
				NSMutableArray* Permissions = [[NSMutableArray alloc] init];

				for(FPublishPermissionsMap::TIterator It(PublishPermissionsMap); It; ++It)
				{
					if((NewPermissions & It.Key()) != 0)
					{
						UE_LOG(LogOnline, Display, TEXT("PublishPermissionsMap[%i] - [%i]"), (int32)It.Key(), [It.Value() count]);
						for( NSString* RequestPermission in It.Value() )
						{
							[Permissions addObject:RequestPermission];
						}
					}
				}

				
				FBSessionDefaultAudience DefaultAudience = FBSessionDefaultAudienceNone;
				switch (Privacy)
				{
				case EOnlineStatusUpdatePrivacy::OnlyMe:
					DefaultAudience = FBSessionDefaultAudienceOnlyMe;
					break;
				case EOnlineStatusUpdatePrivacy::OnlyFriends:
					DefaultAudience = FBSessionDefaultAudienceFriends;
					break;
				case EOnlineStatusUpdatePrivacy::Everyone:
					DefaultAudience = FBSessionDefaultAudienceEveryone;
					break;
				}

				// Kick off the server request
				[FBSession.activeSession requestNewPublishPermissions:Permissions defaultAudience:FBSessionDefaultAudienceOnlyMe 
					completionHandler:^(FBSession *session, NSError *error)
					{
						TriggerOnRequestNewPublishPermissionsCompleteDelegates(LocalUserNum, error==nil);
					}
				];
			}
		);
	}
	else
	{
		// If we weren't logged into Facebook we cannot do this action
		TriggerOnRequestNewPublishPermissionsCompleteDelegates( LocalUserNum, false );
	}

	// We did kick off a request
	return bTriggeredRequest;
}


bool FOnlineSharingFacebook::ShareStatusUpdate(int32 LocalUserNum, const FOnlineStatusUpdate& StatusUpdate)
{
	bool bTriggeredRequest = false;
	if( IdentityInterface->GetLoginStatus(LocalUserNum) == ELoginStatus::LoggedIn )
	{
		bTriggeredRequest = true;

		dispatch_async(dispatch_get_main_queue(),^ 
			{
				// We need to determine where we are posting to.
				NSString* GraphPath;

				// The contents of the post
				NSMutableDictionary* Params = [[NSMutableDictionary alloc] init];

				NSString* ConvertedMessage = [NSString stringWithCString:TCHAR_TO_ANSI(*StatusUpdate.Message)  encoding:NSASCIIStringEncoding];
				[Params setObject:ConvertedMessage forKey:@"message"];

				// Setup the image if one was added to the post
				UIImage* SharingImage = nil;
				if( StatusUpdate.Image != NULL )
				{
					// We are posting this as a photo.
					GraphPath = @"me/photos";

					// Convert our FImage to a UIImage
					int32 Width = StatusUpdate.Image->SizeX;
					int32 Height = StatusUpdate.Image->SizeY;

					CGColorSpaceRef colorSpace=CGColorSpaceCreateDeviceRGB();
					CGContextRef bitmapContext=CGBitmapContextCreate(StatusUpdate.Image->RawData.GetData(), Width, Height, 8, 4*Width, colorSpace,  kCGImageAlphaNoneSkipLast);
					CFRelease(colorSpace);

					CGImageRef cgImage=CGBitmapContextCreateImage(bitmapContext);
					CGContextRelease(bitmapContext);

					SharingImage = [UIImage imageWithCGImage:cgImage];
					CGImageRelease(cgImage);

					[Params setObject:SharingImage forKey:@"picture"];
				}
				else
				{
					// if no image, we are posting this to the news feed.
					GraphPath = @"me/feed";
				}

				// get the formatted friends tags
				NSString* TaggedFriendIds = @"";

				for(int32 FriendIdx = 0; FriendIdx < StatusUpdate.TaggedFriends.Num(); FriendIdx++)
				{
					NSString* FriendId = [NSString stringWithCString:TCHAR_TO_ANSI(*StatusUpdate.TaggedFriends[FriendIdx]->ToString())  encoding:NSASCIIStringEncoding];
								
					TaggedFriendIds = [TaggedFriendIds stringByAppendingFormat:@"%@%@", 
						FriendId,
						(FriendIdx < StatusUpdate.TaggedFriends.Num() - 1) ? @"," : @""
					];
				}

				if( ![TaggedFriendIds isEqual:@""])
				{
					[Params setObject:TaggedFriendIds forKey:@"tags"];
				}

				// kick off a request to post the status
				[FBRequestConnection startWithGraphPath:GraphPath parameters:Params HTTPMethod:@"POST"
					completionHandler:^(FBRequestConnection *connection, id result, NSError *error) 
					{
						UE_LOG(LogOnline, Display, TEXT("startWithGraphPath : Success - %d"), error == nil);
						TriggerOnSharePostCompleteDelegates( LocalUserNum, error == nil );
					}
				];
			}
		);
	}
	else
	{
		// If we weren't logged into Facebook we cannot do this action
		TriggerOnSharePostCompleteDelegates( LocalUserNum, false );
	}

	return bTriggeredRequest;
}


bool FOnlineSharingFacebook::ReadNewsFeed(int32 LocalUserNum, int32 NumPostsToRead)
{
	bool bTriggeredRequest = false;
	if( IdentityInterface->GetLoginStatus(LocalUserNum) == ELoginStatus::LoggedIn )
	{
		bTriggeredRequest = true;
		
		dispatch_async(dispatch_get_main_queue(),^ 
			{
				// The current read permissions for this OSS.
				EOnlineSharingReadCategory::Type ReadPermissions;
				NSString *fqlQuery = 
					[NSString stringWithFormat:@"SELECT post_id, created_time, type, attachment \
												FROM stream WHERE filter_key in (SELECT filter_key \
												FROM stream_filter WHERE uid=me() AND type='newsfeed')AND is_hidden = 0 \
												LIMIT %d", NumPostsToRead];

				[FBRequestConnection
					startWithGraphPath:@"/fql"
					parameters:[NSDictionary dictionaryWithObjectsAndKeys: fqlQuery, @"q", nil]
					HTTPMethod:@"GET"
					completionHandler:^(FBRequestConnection *connection, id result, NSError *error) 
					{
						if( error )
						{
							UE_LOG(LogOnline, Display, TEXT("FTestSharingInterface::ReadStatusFeed - error[%s]"), ANSI_TO_TCHAR([[error localizedDescription] cStringUsingEncoding: NSASCIIStringEncoding]));
						}

						TriggerOnReadNewsFeedCompleteDelegates(LocalUserNum, error==nil);
					}
				];
			}
		);
	}
	else
	{
		// If we weren't logged into Facebook we cannot do this action
		TriggerOnReadNewsFeedCompleteDelegates( LocalUserNum, false );
	}

	return bTriggeredRequest;
}
