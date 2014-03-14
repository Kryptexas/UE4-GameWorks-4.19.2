// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlankProgram.h"

#include "RequiredProgramMainCPPInclude.h"

DEFINE_LOG_CATEGORY_STATIC(LogBlankProgram, Log, All);

IMPLEMENT_APPLICATION(BlankProgram, "BlankProgram");
int32 main(int32 ArgC, char* ArgV[])
{
	GEngineLoop.PreInit(ArgC, ArgV);
	UE_LOG(LogBlankProgram, Display, TEXT("Hello World"));
	return 0;
}
