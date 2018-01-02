// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IXRSystemAssets.h"
#include "UObject/SoftObjectPtr.h"

/**
 *
 */
class FOculusAssetManager : public IXRSystemAssets
{
public:
	FOculusAssetManager();
	virtual ~FOculusAssetManager();

public:
	//~ IXRSystemAssets interface 

	virtual bool EnumerateRenderableDevices(TArray<int32>& DeviceListOut) override;
	virtual int32 GetDeviceId(EControllerHand ControllerHand) override;
	virtual UPrimitiveComponent* CreateRenderComponent(const int32 DeviceId, AActor* Owner, EObjectFlags Flags) override;
};