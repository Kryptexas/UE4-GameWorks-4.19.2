// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Toolkits/IToolkit.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "TranslationEditorModule.h"


/** Translation Editor public interface */
class ITranslationEditor : public FAssetEditorToolkit
{

public:

	static void OpenTranslationEditor(FName ProjectName, FName TranslationTargetLanguage)
	{
		FTranslationEditorModule& TranslationEditorModule = FModuleManager::LoadModuleChecked<FTranslationEditorModule>( "TranslationEditor" );
		TSharedRef< FTranslationEditor > NewTranslationEditor = TranslationEditorModule.CreateTranslationEditor(ProjectName, TranslationTargetLanguage);
	}

};


