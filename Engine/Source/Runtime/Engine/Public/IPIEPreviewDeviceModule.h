// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "IDeviceProfileSelectorModule.h"
#include "Widgets/SWindow.h"
#include "PIEPreviewDeviceEnumeration.h"

class IPIEPreviewDeviceModule : public IDeviceProfileSelectorModule
{
	public:
		//~ Begin IDeviceProfileSelectorModule Interface
		virtual const FString GetRuntimeDeviceProfileName() override;
		//~ End IDeviceProfileSelectorModule Interface


		//~ Begin IModuleInterface Interface
		virtual void StartupModule() override;
		virtual void ShutdownModule() override;
		//~ End IModuleInterface Interface

		/**
		* Virtual destructor.
		*/
		virtual ~IPIEPreviewDeviceModule()
		{
		}

		/**
		* Create PieWindow Ref
		*/
		virtual  TSharedRef<SWindow>   CreatePIEPreviewDeviceWindow(FVector2D ClientSize, FText WindowTitle, EAutoCenter AutoCenterType, FVector2D ScreenPosition, TOptional<float> MaxWindowWidth, TOptional<float> MaxWindowHeight) = 0;

		/**
		* Apply PieWindow device parameters
		*/
		virtual void ApplyPreviewDeviceState() = 0;
};