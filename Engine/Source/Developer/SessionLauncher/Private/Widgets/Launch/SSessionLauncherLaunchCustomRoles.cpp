// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherLaunchCustomRoles.cpp: Implements the SSessionLauncherLaunchCustomRoles class.
=============================================================================*/

#include "SessionLauncherPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SSessionLauncherLaunchCustomRoles"


/* SSessionLauncherLaunchCustomRoles structors
 *****************************************************************************/

SSessionLauncherLaunchCustomRoles::~SSessionLauncherLaunchCustomRoles( )
{
	if (Model.IsValid())
	{
		Model->OnProfileSelected().RemoveAll(this);
	}
}


/* SSessionLauncherLaunchCustomRoles interface
 *****************************************************************************/

void SSessionLauncherLaunchCustomRoles::Construct( const FArguments& InArgs, const FSessionLauncherModelRef& InModel )
{
	Model = InModel;

	ChildSlot
	[
		SNullWidget::NullWidget
	];
}


#undef LOCTEXT_NAMESPACE
