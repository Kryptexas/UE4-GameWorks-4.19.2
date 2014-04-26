// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SlateWidgetStyleContainerBase.cpp: Implements the USlateWidgetStyleContainerBase class.
=============================================================================*/

#include "SlateCorePrivatePCH.h"
#include "SlateWidgetStyleContainerBase.h"


DEFINE_LOG_CATEGORY(LogSlateStyle);


USlateWidgetStyleContainerBase::USlateWidgetStyleContainerBase( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{ }


const struct FSlateWidgetStyle* const USlateWidgetStyleContainerBase::GetStyle() const
{
	return nullptr;
}
