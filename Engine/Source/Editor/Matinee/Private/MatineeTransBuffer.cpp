// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MatineeModule.h"
#include "MatineeTransaction.h"
#include "MatineeTransBuffer.h"

/*-----------------------------------------------------------------------------
	UMatineeTransBuffer / FMatineeTransaction
-----------------------------------------------------------------------------*/
UMatineeTransBuffer::UMatineeTransBuffer(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UMatineeTransBuffer::BeginSpecial(const FText& Description)
{
	BeginInternal<FMatineeTransaction>(TEXT("Matinee"), Description);
}

void UMatineeTransBuffer::EndSpecial()
{
	UTransBuffer::End();
}
