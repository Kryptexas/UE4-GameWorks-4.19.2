// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved. 
#include "ScriptPluginPrivatePCH.h"

//////////////////////////////////////////////////////////////////////////

UScriptAsset::UScriptAsset(const FPostConstructInitializeProperties& PCIP)
	: Super( PCIP )
{
}

void UScriptAsset::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	if (!SourceCode.IsEmpty() && !ByteCode.Num())
	{
		Compile();
	}
#endif
}

#if WITH_EDITOR
void UScriptAsset::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	Compile();
}

void UScriptAsset::Compile()
{
	if (GIsEditor)
	{
		MarkPackageDirty();
	}	
}
#endif