// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "MessageLog.h"
#include "UObjectToken.h"


//////////////////////////////////////////////////////////////////////////
// UKismetInputLibrary

#define LOCTEXT_NAMESPACE "KismetInputLibrary"


UKismetInputLibrary::UKismetInputLibrary(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}


void UKismetInputLibrary::CalibrateTilt()
{
	GEngine->Exec(NULL, TEXT("CALIBRATEMOTION"));
}

#undef LOCTEXT_NAMESPACE