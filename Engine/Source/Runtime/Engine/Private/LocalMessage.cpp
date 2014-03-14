// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "EngineClasses.h"

FClientReceiveData::FClientReceiveData()
	: LocalPC(NULL)
	, MessageType(FName(TEXT("None")))
	, MessageIndex(-1)
	, MessageString(TEXT(""))
	, RelatedPlayerState_1(NULL)
	, RelatedPlayerState_2(NULL)
	, OptionalObject(NULL)
{
	
}

ULocalMessage::ULocalMessage(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}
