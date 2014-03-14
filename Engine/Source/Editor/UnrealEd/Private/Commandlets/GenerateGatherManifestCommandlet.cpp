// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "ISourceControlModule.h"
#include "Json.h"
#include "Internationalization/InternationalizationManifest.h"
#include "InternationalizationManifestJsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogGenerateManifestCommandlet, Log, All);

/**
 *	UGenerateGatherManifestCommandlet
 */
UGenerateGatherManifestCommandlet::UGenerateGatherManifestCommandlet(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

int32 UGenerateGatherManifestCommandlet::Main( const FString& Params )
{
	// Parse command line - we're interested in the param vals
	TArray<FString> Tokens;
	TArray<FString> Switches;
	TMap<FString, FString> ParamVals;
	UCommandlet::ParseCommandLine( *Params, Tokens, Switches, ParamVals );

	// Set config file.
	const FString* ParamVal = ParamVals.Find( FString( TEXT("Config") ) );
	FString GatherTextConfigPath;

	if ( ParamVal )
	{
		GatherTextConfigPath = *ParamVal;
	}
	else
	{
		UE_LOG( LogGenerateManifestCommandlet, Error, TEXT("No config specified.") );
		return -1;
	}

	// Set config section.
	ParamVal = ParamVals.Find( FString( TEXT("Section") ) );
	FString SectionName;

	if ( ParamVal )
	{
		SectionName = *ParamVal;
	}
	else
	{
		UE_LOG( LogGenerateManifestCommandlet, Error, TEXT("No config section specified.") );
		return -1;
	}

	// Get destination path.
	FString DestinationPath;
	if( !GetConfigString( *SectionName, TEXT("DestinationPath"), DestinationPath, GatherTextConfigPath ) )
	{
		UE_LOG( LogGenerateManifestCommandlet, Error, TEXT("No destination path specified.") );
		return -1;
	}

	// Get manifest name.
	FString ManifestName;
	if( !GetConfigString( *SectionName, TEXT("ManifestName"), ManifestName, GatherTextConfigPath ) )
	{
		UE_LOG( LogGenerateManifestCommandlet, Error, TEXT("No manifest name specified.") );
		return -1;
	}

	//Grab any manifest dependencies
	TArray<FString> ManifestDependenciesList;
	GetConfigArray(*SectionName, TEXT("ManifestDependencies"), ManifestDependenciesList, GatherTextConfigPath);

	if( ManifestDependenciesList.Num() > 0 )
	{
		if( !ManifestInfo->AddManifestDependencies( ManifestDependenciesList ) )
		{
			UE_LOG(LogGenerateManifestCommandlet, Error, TEXT("The GenerateGatherManifest commandlet couldn't find all the specified manifest dependencies."));
			return -1;
		}
	
		ManifestInfo->ApplyManifestDependencies();
	}
	

	if( !WriteManifest( ManifestInfo->GetManifest(), DestinationPath / ManifestName ) )
	{
		UE_LOG( LogGenerateManifestCommandlet, Error,TEXT("Failed to write manifest to %s."), *DestinationPath );				
		return -1;
	}
	return 0;
}

bool UGenerateGatherManifestCommandlet::WriteManifest( const TSharedPtr<FInternationalizationManifest>& InManifest, const FString& OutputFilePath )
{
	// We can not continue if the provided manifest is not valid
	if( !InManifest.IsValid() )
	{
		return false;
	}
	FInternationalizationManifestJsonSerializer ManifestSerializer;
	TSharedRef<FJsonObject> JsonManifestObj = MakeShareable( new FJsonObject );
	
	bool bSuccess = ManifestSerializer.SerializeManifest( InManifest.ToSharedRef(), JsonManifestObj );
	
	if( bSuccess )
	{
		bSuccess = WriteJSONToTextFile( JsonManifestObj, OutputFilePath, SourceControlInfo );
	}
	return bSuccess;	
}
