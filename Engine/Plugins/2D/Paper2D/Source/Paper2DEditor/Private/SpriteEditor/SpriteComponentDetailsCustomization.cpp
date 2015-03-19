// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "SpriteComponentDetailsCustomization.h"

#define LOCTEXT_NAMESPACE "SpriteEditor"

//////////////////////////////////////////////////////////////////////////
// FSpriteComponentDetailsCustomization

TSharedRef<IDetailCustomization> FSpriteComponentDetailsCustomization::MakeInstance()
{
	return MakeShareable(new FSpriteComponentDetailsCustomization);
}

void FSpriteComponentDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	// Create a category so this is displayed early in the properties
	DetailBuilder.EditCategory("Sprite", FText::GetEmpty(), ECategoryPriority::Important);
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
