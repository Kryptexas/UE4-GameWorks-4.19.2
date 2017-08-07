// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ExternalTexture.h"

FExternalTextureRegistry* FExternalTextureRegistry::Singleton = nullptr;

FExternalTextureRegistry& FExternalTextureRegistry::Get()
{
	check(IsInRenderingThread());
	if (Singleton == nullptr)
	{
		Singleton = new FExternalTextureRegistry();
	}
	return *Singleton;
}

/* Register an external texture and its sampler state against a GUID */
void FExternalTextureRegistry::RegisterExternalTexture(const FGuid& InGuid, FTextureRHIRef& InTextureRHI, FSamplerStateRHIRef& InSamplerStateRHI)
{
	//FPlatformMisc::LowLevelOutputDebugStringf(TEXT("RegisterExternalTexture %s\n"), *InGuid.ToString());
	TextureEntries.Add(InGuid, FExternalTextureEntry(InTextureRHI, InSamplerStateRHI));
	for (const FMaterialRenderProxy* MaterialRenderProxy : ReferencingMaterialRenderProxies)
	{
		const_cast<FMaterialRenderProxy*>(MaterialRenderProxy)->CacheUniformExpressions();
	}
}

/* Removes an external texture given a GUID */
void FExternalTextureRegistry::UnregisterExternalTexture(const FGuid& InGuid)
{
	TextureEntries.Remove(InGuid);
	for (const FMaterialRenderProxy* MaterialRenderProxy : ReferencingMaterialRenderProxies)
	{
		const_cast<FMaterialRenderProxy*>(MaterialRenderProxy)->CacheUniformExpressions();
	}
}

void FExternalTextureRegistry::RemoveMaterialRenderProxyReference(const FMaterialRenderProxy* MaterialRenderProxy)
{
	ReferencingMaterialRenderProxies.Remove(MaterialRenderProxy);
}

/* Looks up an external texture for given a given GUID
 * @return false if the texture is not registered
 */
bool FExternalTextureRegistry::GetExternalTexture(const FMaterialRenderProxy* MaterialRenderProxy, const FGuid& InGuid, FTextureRHIRef& OutTextureRHI, FSamplerStateRHIRef& OutSamplerStateRHI)
{
	if (MaterialRenderProxy)
	{
		ReferencingMaterialRenderProxies.Add(MaterialRenderProxy);
	}

	FExternalTextureEntry* Entry = TextureEntries.Find(InGuid);
	if (Entry)
	{
		//FPlatformMisc::LowLevelOutputDebugStringf(TEXT("GetExternalTexture %s found\n"),*InGuid.ToString());
		OutTextureRHI = Entry->TextureRHI;
		OutSamplerStateRHI = Entry->SamplerStateRHI;
		return true;
	}
	else
	{
		//FPlatformMisc::LowLevelOutputDebugStringf(TEXT("GetExternalTexture %s not found\n"), *InGuid.ToString());
		return false;
	}
}
