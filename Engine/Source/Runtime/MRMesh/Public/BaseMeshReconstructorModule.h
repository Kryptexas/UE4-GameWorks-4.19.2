// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Features/IModularFeatures.h"
#include "Features/IModularFeature.h"

class IMRMesh;

class FBaseMeshReconstructorModule : public IModuleInterface, public IModularFeature
{
public:
	//
	// MESH RECONSTRUCTION API
	//

	/** Invoked when a new MRMesh instance wants to render and is requesting that data be sent to it. */
	virtual void PairWithComponent(IMRMesh& MRMeshComponent) = 0;

	/** A signal to stop any running threads. */
	virtual void Stop() = 0;

public:
	//
	// MODULAR FEATURE SUPPORT
	//

	/**
	 * Part of the pattern for supporting modular features.
	 *
	 * @return the name of the feature.
	 */
	static FName GetModularFeatureName()
	{
		static const FName ModularFeatureName = FName(TEXT("MeshReconstructor"));
		return ModularFeatureName;
	}
	
	/**
	 * Singleton-like access to a MeshReconstructorModule.
	 *
	 * @return Returns reference to the highest priority MeshReconstructorModule module
	 */
	static inline FBaseMeshReconstructorModule& Get()
	{
		//@todo implement priority for choosing the most appropriate implementation

		TArray<FBaseMeshReconstructorModule*> MeshReconstructors = IModularFeatures::Get().GetModularFeatureImplementations<FBaseMeshReconstructorModule>(GetModularFeatureName());
		check(MeshReconstructors.Num() > 0);
		return *MeshReconstructors[0];
	}

	/**
	 * Check to see that there is a MeshReconstructor module available.
	 *
	 * @return True if there exists a MeshReconstructor module registered.
	 */
	static inline bool IsAvailable()
	{
		return IModularFeatures::Get().IsModularFeatureAvailable(GetModularFeatureName());
	}

	/**
	 * Register module as a MeshReconstructor on startup.
	 */
	virtual void StartupModule() override
	{
		IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);
	}

};