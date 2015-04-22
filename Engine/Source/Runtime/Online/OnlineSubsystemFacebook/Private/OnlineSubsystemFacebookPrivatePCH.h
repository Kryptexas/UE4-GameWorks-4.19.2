// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Core.h"
#include "ModuleManager.h"

#include "Http.h"
#include "Json.h"

#include "OnlineSubsystemFacebook.h"
#include "OnlineSubsystemFacebookModule.h"

#include "OnlineIdentityFacebook.h"
#include "OnlineFriendsFacebook.h"


// Begin contents of Facebook core kit
#import <FBSDKCoreKit/FBSDKCoreKit.h>
/*
#include "FBSDKAccessToken.h"
#include "FBSDKAppEvents.h"
#include "FBSDKAppLinkUtility.h"
#include "FBSDKApplicationDelegate.h"
#include "FBSDKConstants.h"
#include "FBSDKCopying.h"
#include "FBSDKGraphRequest.h"
#include "FBSDKGraphRequestConnection.h"
#include "FBSDKMacros.h"
#include "FBSDKMutableCopying.h"
#include "FBSDKProfile.h"
#include "FBSDKProfilePictureView.h"
#include "FBSDKSettings.h"
#include "FBSDKUtility.h"

#define FBSDK_VERSION_STRING @"4.0.1"
#define FBSDK_TARGET_PLATFORM_VERSION @"v2.3"
*/
// End contents of Facebook core kit


// Begin contents of Facebook login kit
#import <FBSDKLoginKit/FBSDKLoginKit.h>
/*
#include "FBSDKLoginButton.h"
#include "FBSDKLoginConstants.h"
#include "FBSDKLoginManager.h"
#include "FBSDKLoginManagerLoginResult.h"
#include "FBSDKLoginTooltipView.h"
*/
// End contents of Facebook login kit

/** FName declaration of Facebook subsystem */
#define FACEBOOK_SUBSYSTEM FName(TEXT("Facebook"))

/** pre-pended to all Facebook logging */
#undef ONLINE_LOG_PREFIX
#define ONLINE_LOG_PREFIX TEXT("Facebook: ")