// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#if WITH_EDITOR
#include "MessageLog.h"
#include "UObjectToken.h"
#endif

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UPanelWidget

UPanelWidget::UPanelWidget(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
