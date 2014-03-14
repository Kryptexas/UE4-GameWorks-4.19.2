// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// SoundSurroundExporterWAV
//=============================================================================

#pragma once
#include "SoundSurroundExporterWAV.generated.h"

UCLASS()
class USoundSurroundExporterWAV : public UExporter
{
	GENERATED_UCLASS_BODY()


	// Begin UExporter Interface
	virtual int32 GetFileCount( void ) const OVERRIDE;
	virtual FString GetUniqueFilename( const TCHAR* Filename, int32 FileIndex ) OVERRIDE;
	virtual bool ExportBinary( UObject* Object, const TCHAR* Type, FArchive& Ar, FFeedbackContext* Warn, int32 FileIndex = 0, uint32 PortFlags=0 ) OVERRIDE;
	// End UExporter Interface
};



