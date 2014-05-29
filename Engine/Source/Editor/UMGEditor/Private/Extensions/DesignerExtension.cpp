// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

void FDesignerExtension::Initialize(UWidgetBlueprint* InBlueprint)
{
	Blueprint = InBlueprint;
}

FName FDesignerExtension::GetExtensionId() const
{
	return ExtensionId;
}

#undef LOCTEXT_NAMESPACE
