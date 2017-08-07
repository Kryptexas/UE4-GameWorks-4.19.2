// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RHI.h"
#include "RHIResources.h"
#include "MaterialShared.h"

/* FExternalTextureRegistry - stores a registry of external textures mapped to their GUIDs */

class ENGINE_API FExternalTextureRegistry
{
	static FExternalTextureRegistry* Singleton;

	FExternalTextureRegistry()
	{}

	struct FExternalTextureEntry
	{
		FExternalTextureEntry(FTextureRHIRef& InTextureRHI, FSamplerStateRHIRef& InSamplerStateRHI)
			: TextureRHI(InTextureRHI)
			, SamplerStateRHI(InSamplerStateRHI)
		{}

		const FTextureRHIRef TextureRHI;
		const FSamplerStateRHIRef SamplerStateRHI;
	};

	TMap<FGuid, FExternalTextureEntry> TextureEntries;
	TSet<const FMaterialRenderProxy*> ReferencingMaterialRenderProxies;

public:

	static FExternalTextureRegistry& Get();

	/* Register an external texture, its sampler state and coordinate scale/bias against a GUID */
	void RegisterExternalTexture(const FGuid& InGuid, FTextureRHIRef& InTextureRHI, FSamplerStateRHIRef& InSamplerStateRHI);

	/* Removes an external texture given a GUID */
	void UnregisterExternalTexture(const FGuid& InGuid);

	/* Removes reference to a MaterialRenderProxy which was referencing external textures */
	void RemoveMaterialRenderProxyReference(const FMaterialRenderProxy* MaterialRenderProxy);

	/* Looks up an external texture for given a given GUID
	 * @return false if the texture is not registered
	 */
	bool GetExternalTexture(const FMaterialRenderProxy* MaterialRenderProxy, const FGuid& InGuid, FTextureRHIRef& OutTextureRHI, FSamplerStateRHIRef& OutSamplerStateRHI);
};
