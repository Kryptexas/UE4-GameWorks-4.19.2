// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AudioEditorPrivatePCH.h"

UClass* FAssetTypeActions_ReverbEffect::GetSupportedClass() const
{
	return UReverbEffect::StaticClass();
}