// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"
#include "UMGDetailKeyframeHandler.h"


FUMGDetailKeyframeHandler::FUMGDetailKeyframeHandler(TSharedPtr<FWidgetBlueprintEditor> InBlueprintEditor)
	: BlueprintEditor( InBlueprintEditor )
{}

bool FUMGDetailKeyframeHandler::IsPropertyKeyable(const UClass& InObjectClass, const IPropertyHandle& InPropertyHandle) const
{
	return BlueprintEditor.Pin()->GetSequencer()->CanKeyProperty( InObjectClass, InPropertyHandle );
}

void FUMGDetailKeyframeHandler::OnKeyPropertyClicked(const IPropertyHandle& KeyedPropertyHandle)
{
	TArray<UObject*> Objects;
	KeyedPropertyHandle.GetOuterObjects( Objects );

	BlueprintEditor.Pin()->GetSequencer()->KeyProperty( Objects, KeyedPropertyHandle );
}
