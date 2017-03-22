//
// Copyright (C) Impulsonic, Inc. All rights reserved.
//

#pragma once

#include "PhononMaterial.h"
#include "PhononCommon.h"
#include "PhononSettings.generated.h"

UCLASS(config = Engine, defaultconfig)
class PHONON_API UPhononSettings : public UObject
{
	GENERATED_BODY()

public:

	UPhononSettings();

	IPLMaterial GetDefaultStaticMeshMaterial() const;
	IPLMaterial GetDefaultBSPMaterial() const;
	IPLMaterial GetDefaultLandscapeMaterial() const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual bool CanEditChange(const UProperty* InProperty) const override;
#endif

	UPROPERTY(GlobalConfig, EditAnywhere, Category = General)
	EIplAudioEngine AudioEngine;

	UPROPERTY(GlobalConfig, EditAnywhere, Category = "Scene Export", meta = (DisplayName = "Export BSP Geometry"))
	bool ExportBSPGeometry;

	UPROPERTY(GlobalConfig, EditAnywhere, Category = "Scene Export", meta = (DisplayName = "Export Landscape Geometry"))
	bool ExportLandscapeGeometry;

	///////

	UPROPERTY(GlobalConfig, EditAnywhere, Category = "Scene Export|Default Static Mesh Material", meta = (DisplayName = "Material Preset"))
	EPhononMaterial StaticMeshMaterialPreset;

	UPROPERTY(GlobalConfig, EditAnywhere, Category = "Scene Export|Default Static Mesh Material", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0",
		DisplayName = "Low Frequency Absorption"))
	float StaticMeshLowFreqAbsorption;

	UPROPERTY(GlobalConfig, EditAnywhere, Category = "Scene Export|Default Static Mesh Material", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0",
		DisplayName = "Mid Frequency Absorption"))
	float StaticMeshMidFreqAbsorption;

	UPROPERTY(GlobalConfig, EditAnywhere, Category = "Scene Export|Default Static Mesh Material", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0",
		DisplayName = "High Frequency Absorption"))
	float StaticMeshHighFreqAbsorption; 

	UPROPERTY(GlobalConfig, EditAnywhere, Category = "Scene Export|Default Static Mesh Material", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0",
		DisplayName = "Scattering"))
	float StaticMeshScattering;

	///////

	UPROPERTY(GlobalConfig, EditAnywhere, Category = "Scene Export|Default BSP Material", meta = (DisplayName = "Material Preset"))
	EPhononMaterial BSPMaterialPreset;

	UPROPERTY(GlobalConfig, EditAnywhere, Category = "Scene Export|Default BSP Material", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0",
		DisplayName = "Low Frequency Absorption"))
	float BSPLowFreqAbsorption;

	UPROPERTY(GlobalConfig, EditAnywhere, Category = "Scene Export|Default BSP Material", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0",
		DisplayName = "Mid Frequency Absorption"))
	float BSPMidFreqAbsorption;

	UPROPERTY(GlobalConfig, EditAnywhere, Category = "Scene Export|Default BSP Material", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0",
		DisplayName = "High Frequency Absorption"))
	float BSPHighFreqAbsorption;

	UPROPERTY(GlobalConfig, EditAnywhere, Category = "Scene Export|Default BSP Material", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0",
		DisplayName = "Scattering"))
	float BSPScattering;

	///////

	UPROPERTY(GlobalConfig, EditAnywhere, Category = "Scene Export|Default Landscape Material", meta = (DisplayName = "Material Preset"))
	EPhononMaterial LandscapeMaterialPreset;

	UPROPERTY(GlobalConfig, EditAnywhere, Category = "Scene Export|Default Landscape Material", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0",
		DisplayName = "Low Frequency Absorption"))
	float LandscapeLowFreqAbsorption;

	UPROPERTY(GlobalConfig, EditAnywhere, Category = "Scene Export|Default Landscape Material", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0",
		DisplayName = "Mid Frequency Absorption"))
	float LandscapeMidFreqAbsorption;

	UPROPERTY(GlobalConfig, EditAnywhere, Category = "Scene Export|Default Landscape Material", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0",
		DisplayName = "High Frequency Absorption"))
	float LandscapeHighFreqAbsorption;

	UPROPERTY(GlobalConfig, EditAnywhere, Category = "Scene Export|Default Landscape Material", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0",
		DisplayName = "Scattering"))
	float LandscapeScattering;

	///////

	// Scale factor to make the indirect contribution louder or softer.
	UPROPERTY(GlobalConfig, EditAnywhere, Category = IndirectSound, meta = (ClampMin = "0.0", ClampMax = "128.0", UIMin = "0.0", UIMax = "128.0"))
	float IndirectContribution;

	// The output of indirect propagation is stored in ambisonics of this order.
	UPROPERTY(GlobalConfig, EditAnywhere, Category = IndirectSound, meta = (ClampMin = "0", ClampMax = "8", UIMin = "0", UIMax = "8",
		DisplayName = "Impulse Response Order"))
	int32 IndirectImpulseResponseOrder;

	// The length of impulse response to compute for each sound source.
	UPROPERTY(GlobalConfig, EditAnywhere, Category = IndirectSound, meta = (ClampMin = "1.0", ClampMax = "8.0", UIMin = "1.0", UIMax = "8.0",
		DisplayName = "Impulse Response Duration"))
	float IndirectImpulseResponseDuration;

	UPROPERTY(GlobalConfig, EditAnywhere, Category = "IndirectSound")
	EIplSimulationType ReverbSimulationType;

	/*
	// Whether propagation should be applied per-source or on the mixed audio.
	UPROPERTY(GlobalConfig, EditAnywhere, Category = IndirectSound)
	bool EnableMixerOptimization;
	*/

	UPROPERTY(GlobalConfig, EditAnywhere, Category = "IndirectSound|Real-Time Quality Settings", meta = (DisplayName = "Quality Preset"))
	EQualitySettings RealtimeQualityPreset;

	// The number of times an indirect sound path bounces through the environment.
	UPROPERTY(GlobalConfig, EditAnywhere, Category = "IndirectSound|Real-Time Quality Settings", meta = (ClampMin = "0", ClampMax = "128", UIMin = "0", UIMax = "128",
		DisplayName = "Bounces"))
	int32 RealtimeBounces;

	// The number of primary rays to shoot from each sound source.
	UPROPERTY(GlobalConfig, EditAnywhere, Category = "IndirectSound|Real-Time Quality Settings", meta = (ClampMin = "0", ClampMax = "65536", UIMin = "0", UIMax = "65536",
		DisplayName = "Rays"))
	int32 RealtimeRays;

	UPROPERTY(GlobalConfig, EditAnywhere, Category = "IndirectSound|Real-Time Quality Settings", meta = (ClampMin = "0", ClampMax = "8192", UIMin = "0", UIMax = "8192",
		DisplayName = "Secondary Rays"))
	int32 RealtimeSecondaryRays;

	UPROPERTY(GlobalConfig, EditAnywhere, Category = "IndirectSound|Baked Quality Settings", meta = (DisplayName = "Quality Preset"))
	EQualitySettings BakedQualityPreset;

	// The number of times an indirect sound path bounces through the environment.
	UPROPERTY(GlobalConfig, EditAnywhere, Category = "IndirectSound|Baked Quality Settings", meta = (ClampMin = "0", ClampMax = "128", UIMin = "0", UIMax = "128",
		DisplayName = "Bounces"))
	int32 BakedBounces;

	// The number of primary rays to shoot from each sound source.
	UPROPERTY(GlobalConfig, EditAnywhere, Category = "IndirectSound|Baked Quality Settings", meta = (ClampMin = "0", ClampMax = "65536", UIMin = "0", UIMax = "65536",
		DisplayName = "Rays"))
	int32 BakedRays;

	UPROPERTY(GlobalConfig, EditAnywhere, Category = "IndirectSound|Baked Quality Settings", meta = (ClampMin = "0", ClampMax = "8192", UIMin = "0", UIMax = "8192",
		DisplayName = "Secondary Rays"))
	int32 BakedSecondaryRays;
};