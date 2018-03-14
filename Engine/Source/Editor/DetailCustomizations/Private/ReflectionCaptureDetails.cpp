// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "ReflectionCaptureDetails.h"
#include "Engine/ReflectionCapture.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Editor.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "DetailCategoryBuilder.h"
#include "IDetailsView.h"

#define LOCTEXT_NAMESPACE "ReflectionCaptureDetails"



TSharedRef<IDetailCustomization> FReflectionCaptureDetails::MakeInstance()
{
	return MakeShareable( new FReflectionCaptureDetails );
}

void FReflectionCaptureDetails::CustomizeDetails( IDetailLayoutBuilder& DetailLayout )
{
	
}

#undef LOCTEXT_NAMESPACE
