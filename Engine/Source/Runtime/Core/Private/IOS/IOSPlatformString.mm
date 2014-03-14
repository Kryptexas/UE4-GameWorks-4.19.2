// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	IOSPlatformString.mm: Additional NSString methods
=============================================================================*/
#include "CorePrivate.h"

@implementation NSString (FString_Extensions)

+ (NSString*) stringWithFString:(const FString&)InFString
{
	return [NSString stringWithCString:TCHAR_TO_UTF8(*InFString) encoding:NSUTF8StringEncoding];
}

@end
