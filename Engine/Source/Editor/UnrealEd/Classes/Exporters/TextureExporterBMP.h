// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// TextureExporterBMP
//=============================================================================

#pragma once
#include "TextureExporterBMP.generated.h"

UCLASS()
class UTextureExporterBMP : public UExporter
{
	GENERATED_BODY()
public:
	UTextureExporterBMP(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	bool ExportBinary( UObject* Object, const TCHAR* Type, FArchive& Ar, FFeedbackContext* Warn, int32 FileIndex = 0, uint32 PortFlags=0 );
};



