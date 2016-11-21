// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AudioEditorPrivatePCH.h"
#include "AudioEditorModule.h"
#include "SoundClassEditorUtilities.h"
#include "Toolkits/ToolkitManager.h"
#include "SoundDefinitions.h"
#include "Classes/SoundClassGraph/SoundClassGraph.h"

void FSoundClassEditorUtilities::CreateSoundClass(const class UEdGraph* Graph, class UEdGraphPin* FromPin, const FVector2D& Location, FString Name)
{
	check(Graph);

	// Cast outer to SoundClass
	USoundClass* SoundClass = CastChecked<USoundClass>(Graph->GetOuter());

	if (SoundClass != NULL)
	{
		TSharedPtr<ISoundClassEditor> SoundClassEditor;
		TSharedPtr< IToolkit > FoundAssetEditor = FToolkitManager::Get().FindEditorForAsset(SoundClass);
		if (FoundAssetEditor.IsValid())
		{
			SoundClassEditor = StaticCastSharedPtr<ISoundClassEditor>(FoundAssetEditor);
			SoundClassEditor->CreateSoundClass(FromPin, Location, Name);
		}
	}
}