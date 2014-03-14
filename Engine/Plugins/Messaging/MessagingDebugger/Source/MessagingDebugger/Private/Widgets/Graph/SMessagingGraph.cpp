// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SMessagingGraph.cpp: Implements the SMessagingGraph class.
=============================================================================*/

#include "MessagingDebuggerPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SMessagingGraph"


/* SMessagingGraph interface
 *****************************************************************************/

void SMessagingGraph::Construct( const FArguments& InArgs, const TSharedRef<ISlateStyle>& InStyle )
{
	ChildSlot
	[
		SNullWidget::NullWidget
		//SAssignNew(GraphEditor, SGraphEditor)
	];
}


#undef LOCTEXT_NAMESPACE
