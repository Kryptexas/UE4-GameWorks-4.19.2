//
// Copyright (C) Valve Corporation. All rights reserved.
//

#include "PhononMaterial.h"

static TMap<EPhononMaterial, IPLMaterial> GetMaterialPresets()
{
	TMap<EPhononMaterial, IPLMaterial> Presets;

	IPLMaterial mat;
	mat.scattering = 0.05f;

	mat.lowFreqAbsorption = 0.10f;
	mat.midFreqAbsorption = 0.20f;
	mat.highFreqAbsorption = 0.30f;
	Presets.Add(EPhononMaterial::GENERIC, mat);

	mat.lowFreqAbsorption = 0.03f;
	mat.midFreqAbsorption = 0.04f;
	mat.highFreqAbsorption = 0.07f;
	Presets.Add(EPhononMaterial::BRICK, mat);

	mat.lowFreqAbsorption = 0.05f;
	mat.midFreqAbsorption = 0.07f;
	mat.highFreqAbsorption = 0.08f;
	Presets.Add(EPhononMaterial::CONCRETE, mat);

	mat.lowFreqAbsorption = 0.01f;
	mat.midFreqAbsorption = 0.02f;
	mat.highFreqAbsorption = 0.02f;
	Presets.Add(EPhononMaterial::CERAMIC, mat);

	mat.lowFreqAbsorption = 0.60f;
	mat.midFreqAbsorption = 0.70f;
	mat.highFreqAbsorption = 0.80f;
	Presets.Add(EPhononMaterial::GRAVEL, mat);

	mat.lowFreqAbsorption = 0.24f;
	mat.midFreqAbsorption = 0.69f;
	mat.highFreqAbsorption = 0.73f;
	Presets.Add(EPhononMaterial::CARPET, mat);

	mat.lowFreqAbsorption = 0.06f;
	mat.midFreqAbsorption = 0.03f;
	mat.highFreqAbsorption = 0.02f;
	Presets.Add(EPhononMaterial::GLASS, mat);

	mat.lowFreqAbsorption = 0.12f;
	mat.midFreqAbsorption = 0.06f;
	mat.highFreqAbsorption = 0.04f;
	Presets.Add(EPhononMaterial::PLASTER, mat);

	mat.lowFreqAbsorption = 0.11f;
	mat.midFreqAbsorption = 0.07f;
	mat.highFreqAbsorption = 0.06f;
	Presets.Add(EPhononMaterial::WOOD, mat);

	mat.lowFreqAbsorption = 0.20f;
	mat.midFreqAbsorption = 0.07f;
	mat.highFreqAbsorption = 0.06f;
	Presets.Add(EPhononMaterial::METAL, mat);

	mat.lowFreqAbsorption = 0.13f;
	mat.midFreqAbsorption = 0.20f;
	mat.highFreqAbsorption = 0.24f;
	Presets.Add(EPhononMaterial::ROCK, mat);

	mat.lowFreqAbsorption = 0.00f;
	mat.midFreqAbsorption = 0.00f;
	mat.highFreqAbsorption = 0.00f;
	Presets.Add(EPhononMaterial::CUSTOM, mat);

	return Presets;
}

TMap<EPhononMaterial, IPLMaterial> SteamAudio::MaterialPresets = GetMaterialPresets();
