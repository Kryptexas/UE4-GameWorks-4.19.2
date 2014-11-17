// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "PaperJsonImporterPrivatePCH.h"


//////////////////////////////////////////////////////////////////////////
// FPaperJsonImporterModule

class FPaperJsonImporterModule : public FDefaultModuleImpl
{
public:
	virtual void StartupModule() OVERRIDE
	{
	}

	virtual void ShutdownModule() OVERRIDE
	{
	}
};

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_MODULE(FPaperJsonImporterModule, PaperJsonImporter);
DEFINE_LOG_CATEGORY(LogPaperJsonImporter);
