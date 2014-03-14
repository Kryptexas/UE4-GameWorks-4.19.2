// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// StaticMeshFactoryT3D
//=============================================================================

#pragma once
#include "StaticMeshFactoryT3D.generated.h"

UCLASS(hidecategories=Object)
class UStaticMeshFactoryT3D : public UFactory
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	int32 Pitch;

	UPROPERTY()
	int32 Roll;

	UPROPERTY()
	int32 Yaw;


	// Begin UFactory Interface
	virtual FText GetDisplayName() const OVERRIDE;
	virtual UObject* FactoryCreateText( UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn ) OVERRIDE;
	// End UFactory Interface
};



