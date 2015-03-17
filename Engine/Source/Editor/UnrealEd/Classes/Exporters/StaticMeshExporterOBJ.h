// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// StaticMeshExporterOBJ
//=============================================================================

#pragma once
#include "StaticMeshExporterOBJ.generated.h"

UCLASS()
class UStaticMeshExporterOBJ : public UExporter
{
	GENERATED_BODY()
public:
	UStaticMeshExporterOBJ(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	// Begin UExporter Interface
	virtual bool ExportText( const FExportObjectInnerContext* Context, UObject* Object, const TCHAR* Type, FOutputDevice& Ar, FFeedbackContext* Warn, uint32 PortFlags=0 ) override;
	// End UExporter Interface
};



