// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "Internationalization/InternationalizationArchive.h"
#include "Internationalization/InternationalizationManifest.h"
#include "GenerateGatherArchiveCommandlet.generated.h"


/**
 *	UGenerateGatherArchiveCommandlet: Generates a localisation archive; generally used as a gather step.
 */
UCLASS()
class UGenerateGatherArchiveCommandlet : public UGatherTextCommandletBase
{
    GENERATED_UCLASS_BODY()
#if CPP || UE_BUILD_DOCS
public:
	// Begin UCommandlet Interface
	virtual int32 Main( const FString& Params ) OVERRIDE;
	// End UCommandlet Interface
	
	bool WriteArchiveToFile( TSharedRef< FJsonObject> ManifestObject, const FString& OutputDirectoryPath, const TCHAR* Culture, const FString& FileName );
	void BuildArchiveFromManifest( TSharedRef< const FInternationalizationManifest > InManifest, TSharedRef< FInternationalizationArchive > Archive, const FString& SourceCulture, const FString& TargetCulture  );
	void AppendArchiveData( TSharedRef< const FInternationalizationArchive > InArchiveToAppend, TSharedRef< FInternationalizationArchive > ArchiveCombined );
	void ConditionTranslation( FLocItem& LocItem );
	void ConditionSource( FLocItem& LocItem );

#endif
};
