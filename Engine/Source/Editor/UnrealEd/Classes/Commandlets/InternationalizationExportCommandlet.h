// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "Commandlets/Commandlet.h"
#include "InternationalizationExportCommandlet.generated.h"

/**
 *	UInternationalizationExportCommandlet: Commandlet used to export internationalization data to various standard formats. 
 */
UCLASS()
class UInternationalizationExportCommandlet : public UGatherTextCommandletBase
{
    GENERATED_UCLASS_BODY()

public:
	// Begin UCommandlet Interface
	virtual int32 Main(const FString& Params) override;
	// End UCommandlet Interface
private:
	bool DoExport(const FString& SourcePath, const FString& DestinationPath, const FString& Filename);
	bool DoImport(const FString& SourcePath, const FString& DestinationPath, const FString& Filename);

	TArray<FString> CulturesToGenerate;
	FString ConfigPath;
	FString SectionName;

};