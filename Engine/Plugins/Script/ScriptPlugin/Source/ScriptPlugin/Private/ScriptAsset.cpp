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
	UpdateAndCompile();
#endif
}

#if WITH_EDITOR
void UScriptAsset::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UpdateAndCompile();
}

void UScriptAsset::UpdateAndCompile()
{
	FDateTime OldTimeStamp;
	bool bCodeDirty = !FDateTime::Parse(SourceFileTimestamp, OldTimeStamp);
	FDateTime TimeStamp = IFileManager::Get().GetTimeStamp(*SourceFilePath);
	if (bCodeDirty || TimeStamp > OldTimeStamp)
	{
		FString NewScript;
		if (FFileHelper::LoadFileToString(NewScript, *SourceFilePath))
		{
			SourceCode = NewScript;
			SourceFileTimestamp = TimeStamp.ToString();
			bCodeDirty = true;			
		}
	}

	if (bCodeDirty)
	{
		Compile();
		MarkPackageDirty();
	}
}

void UScriptAsset::Compile()
{
}

#endif