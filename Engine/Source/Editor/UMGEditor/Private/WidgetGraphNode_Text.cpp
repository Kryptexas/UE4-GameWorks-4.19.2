// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG_EDITOR"

UWidgetGraphNode_Text::UWidgetGraphNode_Text(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FString UWidgetGraphNode_Text::GetTooltip() const
{
	return LOCTEXT("TextBlockBindingTooltip", "Text Block (Binding)").ToString();
}

FText UWidgetGraphNode_Text::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("TextBlockBinding", "Text Block (Binding)");
}

#undef LOCTEXT_NAMESPACE