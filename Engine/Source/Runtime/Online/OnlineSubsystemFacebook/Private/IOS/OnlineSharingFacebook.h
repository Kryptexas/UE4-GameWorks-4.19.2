// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
 

// Module includes
#include "OnlineSharingInterface.h"


/**
 * Facebook implementation of the Online Sharing Interface
 */
class FOnlineSharingFacebook : public IOnlineSharing
{

public:

	// Begin IOnlineSharing Interface
	virtual bool ReadNewsFeed(int32 LocalUserNum, int32 NumPostsToRead) OVERRIDE;
	virtual bool RequestNewReadPermissions(int32 LocalUserNum, EOnlineSharingReadCategory::Type NewPermissions) OVERRIDE;
	virtual bool ShareStatusUpdate(int32 LocalUserNum, const FOnlineStatusUpdate& StatusUpdate) OVERRIDE;
	virtual bool RequestNewPublishPermissions(int32 LocalUserNum, EOnlineSharingPublishingCategory::Type NewPermissions, EOnlineStatusUpdatePrivacy::Type Privacy) OVERRIDE;
	// End IOnlineSharing Interface

	
public:

	/**
	 * Constructor used to indicate which OSS we are a part of
	 */
	FOnlineSharingFacebook(class FOnlineSubsystemFacebook* InSubsystem);
	
	/**
	 * Default destructor
	 */
	virtual ~FOnlineSharingFacebook();


private:
	
	/**
	 * Setup the permission categories with the relevant Facebook entries
	 */
	void SetupPermissionMaps();

	// The read permissions map which sets up the facebook permissions in their correct category
	typedef TMap< EOnlineSharingReadCategory::Type, NSArray* > FReadPermissionsMap;
	FReadPermissionsMap ReadPermissionsMap;

	// The publish permissions map which sets up the facebook permissions in their correct category
	typedef TMap< EOnlineSharingPublishingCategory::Type, NSArray* > FPublishPermissionsMap;
	FPublishPermissionsMap PublishPermissionsMap;


private:

	/** Reference to the main Facebook identity */
	IOnlineIdentityPtr IdentityInterface;
};


typedef TSharedPtr<FOnlineSharingFacebook, ESPMode::ThreadSafe> FOnlineSharingFacebookPtr;