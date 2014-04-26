// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UMG.h: UMG module public header file.
=============================================================================*/

#pragma once


/* Dependencies
 *****************************************************************************/

#include "Core.h"
#include "CoreUObject.h"
#include "Engine.h"
#include "InputCore.h"
#include "Slate.h"
#include "SlateCore.h"


/* Components
 *****************************************************************************/

#include "SlateWrapperTypes.h"

#include "PanelSlot.h"

#include "SlateWrapperComponent.h"

#include "SlateLeafWidgetComponent.h"
#include "SlateNonLeafWidgetComponent.h"

#include "ContentWidget.h"

#include "BoxPanelComponent.h"

#include "CanvasPanelSlot.h"
#include "CanvasPanelComponent.h"

#include "HorizontalBoxSlot.h"
#include "HorizontalBoxComponent.h"

#include "VerticalBoxSlot.h"
#include "VerticalBoxComponent.h"

#include "BorderComponent.h"
#include "ButtonComponent.h"
#include "CheckBoxComponent.h"
#include "CircularThrobberComponent.h"
#include "EditableText.h"
#include "EditableTextBlockComponent.h"
#include "GridPanelComponent.h"
#include "ImageComponent.h"
#include "ScrollBoxComponent.h"
#include "SpacerComponent.h"
#include "SpinningImageComponent.h"
#include "TextBlockComponent.h"
#include "ThrobberComponent.h"


/* Blueprint
 *****************************************************************************/

#include "WidgetTree.h"
#include "UserWidget.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintGeneratedClass.h"
 

/* Interfaces
 *****************************************************************************/

#include "IUMGModule.h"
