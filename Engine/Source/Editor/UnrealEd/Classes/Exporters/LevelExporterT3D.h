// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// LevelExporterT3D
//=============================================================================

#pragma once
#include "LevelExporterT3D.generated.h"

UCLASS()
class ULevelExporterT3D : public UExporter
{
	GENERATED_UCLASS_BODY()


	// Begin UExporter Interface
	virtual bool ExportText( const FExportObjectInnerContext* Context, UObject* Object, const TCHAR* Type, FOutputDevice& Ar, FFeedbackContext* Warn, uint32 PortFlags=0 ) OVERRIDE;
	virtual void ExportPackageObject(FExportPackageParams& ExpPackageParams) OVERRIDE;
	virtual void ExportPackageInners(FExportPackageParams& ExpPackageParams) OVERRIDE;
	virtual void ExportComponentExtra( const FExportObjectInnerContext* Context, const TArray<UObject*>& Components, FOutputDevice& Ar, uint32 PortFlags) OVERRIDE;
	// End UExporter Interface
};



