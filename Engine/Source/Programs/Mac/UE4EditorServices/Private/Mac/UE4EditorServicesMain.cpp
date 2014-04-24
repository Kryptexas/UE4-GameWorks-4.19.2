// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Core.h"
#include "RequiredProgramMainCPPInclude.h"
#include "ModuleManager.h"

#import <Cocoa/Cocoa.h>

IMPLEMENT_APPLICATION(UE4EditorServices, "UE4EditorServices");

int main(int argc, char *argv[])
{
	FCommandLine::Set(TEXT(""));
	FPlatformProcess::SetCurrentWorkingDirectoryToBaseDir();
	return NSApplicationMain(argc, (const char**)argv);
}
