// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SubversionSourceControlPrivatePCH.h"
#include "SubversionSourceControlLabel.h"
#include "SubversionSourceControlModule.h"
#include "SubversionSourceControlProvider.h"
#include "SubversionSourceControlRevision.h"
#include "XmlParser.h"
#include "SubversionSourceControlUtils.h"

const FString& FSubversionSourceControlLabel::GetName() const
{
	return Name;
}

bool FSubversionSourceControlLabel::GetFileRevisions( const TArray<FString>& InFiles, TArray< TSharedRef<ISourceControlRevision, ESPMode::ThreadSafe> >& OutRevisions ) const
{
	SubversionSourceControlUtils::CheckFilenames(InFiles);
	TArray<FString> QuotedFiles = InFiles;
	SubversionSourceControlUtils::QuoteFilenames(QuotedFiles);

	FSubversionSourceControlModule& SubversionSourceControl = FModuleManager::LoadModuleChecked<FSubversionSourceControlModule>( "SubversionSourceControl" );
	FSubversionSourceControlProvider& Provider = SubversionSourceControl.GetProvider();

	bool bCommandOK = true;
	for(auto Iter(QuotedFiles.CreateConstIterator()); Iter; Iter++)
	{
		TArray<FString> Files;
		Files.Add(*Iter);

		TArray<FXmlFile> ResultsXml;
		TArray<FString> Parameters;
		TArray<FString> ErrorMessages;
		SubversionSourceControlUtils::FHistoryOutput History;

		//limit to last change
		Parameters.Add(TEXT("--limit 1"));
		// output all properties
		Parameters.Add(TEXT("--with-all-revprops"));
		// we want to view over merge boundaries
		Parameters.Add(TEXT("--use-merge-history"));
		// we want all the output!
		Parameters.Add(TEXT("--verbose"));
		// limit the range of revisions up to the one the tag specifies
		Parameters.Add(FString::Printf(TEXT("--revision %i:0"), Revision));
		
		bCommandOK &= SubversionSourceControlUtils::RunCommand(TEXT("log"), Files, Parameters, ResultsXml, ErrorMessages, Provider.GetUserName());
		SubversionSourceControlUtils::ParseLogResults(*Iter, ResultsXml, Provider.GetUserName(), History);

		// add the first (should be only) result
		if(History.Num() > 0)
		{
			check(History.Num() == 1);
			for(auto RevisionIter(History.CreateIterator().Value().CreateIterator()); RevisionIter; RevisionIter++)
			{
				OutRevisions.Add(*RevisionIter);
			}
		}
	}

	return bCommandOK;
}

bool FSubversionSourceControlLabel::Sync( const FString& InFilename ) const
{
	SubversionSourceControlUtils::CheckFilename(InFilename);
	FString QuotedFilename = InFilename;
	SubversionSourceControlUtils::QuoteFilename(QuotedFilename);

	FSubversionSourceControlModule& SubversionSourceControl = FModuleManager::LoadModuleChecked<FSubversionSourceControlModule>( "SubversionSourceControl" );
	FSubversionSourceControlProvider& Provider = SubversionSourceControl.GetProvider();

	TArray<FString> Files;
	Files.Add(QuotedFilename);

	TArray<FString> Results;
	TArray<FString> Parameters;
	TArray<FString> ErrorMessages;
	Parameters.Add(FString::Printf(TEXT("--revision %i"), Revision));

	bool bCommandOK = SubversionSourceControlUtils::RunCommand(TEXT("update"), Files, Parameters, Results, ErrorMessages, Provider.GetUserName());

	// also update cached state
	{
		TArray<FXmlFile> ResultsXml;
		TArray<FString> StatusParameters;
		TArray<FSubversionSourceControlState> OutStates;
		StatusParameters.Add(TEXT("--verbose"));
		StatusParameters.Add(TEXT("--show-updates"));

		bCommandOK &= SubversionSourceControlUtils::RunCommand(TEXT("status"), Files, StatusParameters, ResultsXml, ErrorMessages, Provider.GetUserName());
		SubversionSourceControlUtils::ParseStatusResults(ResultsXml, ErrorMessages, Provider.GetUserName(), OutStates);
		SubversionSourceControlUtils::UpdateCachedStates(OutStates);
	}

	return bCommandOK;
}