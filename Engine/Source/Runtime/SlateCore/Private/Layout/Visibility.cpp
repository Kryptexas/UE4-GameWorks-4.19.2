// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Visibility.cpp: Implements the FVisibility class.
=============================================================================*/

#include "SlateCorePrivatePCH.h"


/* Static initialization
 *****************************************************************************/

const EVisibility EVisibility::Visible(EVisibility::VIS_Visible);
const EVisibility EVisibility::Collapsed(EVisibility::VIS_Collapsed);
const EVisibility EVisibility::Hidden(EVisibility::VIS_Hidden);
const EVisibility EVisibility::HitTestInvisible(EVisibility::VIS_HitTestInvisible);
const EVisibility EVisibility::SelfHitTestInvisible(EVisibility::VIS_SelfHitTestInvisible);
const EVisibility EVisibility::All(EVisibility::VIS_All);


/* FVisibility interface
 *****************************************************************************/

FString EVisibility::ToString( ) const
{
	static const FString VisibleString("Visible");
	static const FString HiddenString("Hidden");
	static const FString CollapsedString("Collapsed");
	static const FString HitTestInvisibleString("HitTestInvisible");
	static const FString SelfHitTestInvisibleString("SelfHitTestInvisible");

	if ((Value & VISPRIVATE_ChildrenHitTestVisible) == 0)
	{
		return HitTestInvisibleString;
	}

	if ((Value & VISPRIVATE_SelfHitTestVisible) == 0)
	{
		return SelfHitTestInvisibleString;
	}

	if ((Value & VISPRIVATE_Visible) != 0)
	{
		return VisibleString;
	}

	if ((Value & VISPRIVATE_Hidden) != 0)
	{
		return HiddenString;
	}

	if ((Value & VISPRIVATE_Collapsed) != 0)
	{
		return CollapsedString;
	}

	return FString();
}
