// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SuperSearchPrivatePCH.h"
#include "SSuperSearch.h"

IMPLEMENT_MODULE( FSuperSearchModule,SuperSearch );

namespace SuperSearchModule
{
	static const FName FNameSuperSearchApp = FName(TEXT("SuperSearchApp"));
}

FSearchEntry* FSearchEntry::MakeCategoryEntry(const FString & InTitle)
{
	FSearchEntry* SearchEntry = new FSearchEntry();
	SearchEntry->Title = InTitle;
	SearchEntry->bCategory = true;

	return SearchEntry;
}

FSuperSearchModule::FSuperSearchModule()
	: SearchEngine(ESearchEngine::Google)
{
	printf("ok");
}

void FSuperSearchModule::StartupModule()
{
	printf("ok");
}

void FSuperSearchModule::ShutdownModule()
{
}

void FSuperSearchModule::SetSearchEngine(ESearchEngine InSearchEngine)
{
	SearchEngine = InSearchEngine;

	for ( TWeakPtr<SSuperSearchBox>& SearchBoxPtr : SuperSearchBoxes )
	{
		TSharedPtr<SSuperSearchBox> SearchBox = SearchBoxPtr.Pin();
		if ( SearchBox.IsValid() )
		{
			SearchBox->SetSearchEngine(SearchEngine);
		}
	}
}

TSharedRef<SWidget> FSuperSearchModule::MakeSearchBox(TSharedPtr< SEditableTextBox >& OutExposedEditableTextBox, const FString& ConfigFilename, const TOptional<const FSearchBoxStyle*> InStyle, const TOptional<const FComboBoxStyle*> InSearchEngineStyle) const
{
	ESuperSearchConfig Config;
	Config.Style = InStyle;

	ESuperSearchOutput Output;
	TSharedRef< SWidget > SearchBox = MakeSearchBox(Config, Output);

	OutExposedEditableTextBox = Output.ExposedEditableTextBox;

	return SearchBox;
}

TSharedRef< SWidget > FSuperSearchModule::MakeSearchBox(const ESuperSearchConfig& Config, ESuperSearchOutput& Output) const
{
	// Remove any search box that has expired.
	for ( int32 i = SuperSearchBoxes.Num() - 1; i >= 0; i-- )
	{
		if ( !SuperSearchBoxes[i].IsValid() )
		{
			SuperSearchBoxes.RemoveAtSwap(i);
		}
	}

	TSharedRef< SSuperSearchBox > NewSearchBox =
		SNew(SSuperSearchBox)
		.Style(Config.Style)
		.SearchEngine(SearchEngine);

	Output.ExposedEditableTextBox = NewSearchBox->GetEditableTextBox();

	SuperSearchBoxes.Add(NewSearchBox);

	return NewSearchBox;
}
