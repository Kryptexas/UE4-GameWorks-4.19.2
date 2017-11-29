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

/* Register an external texture, its sampler state and coordinate scale/bias against a GUID */
void FExternalTextureRegistry::RegisterExternalTexture(const FGuid& InGuid, FTextureRHIRef& InTextureRHI, FSamplerStateRHIRef& InSamplerStateRHI, const FLinearColor& InCoordinateScaleRotation, const FLinearColor& InCoordinateOffset)
{
	TextureEntries.Add(InGuid, FExternalTextureEntry(InTextureRHI, InSamplerStateRHI, InCoordinateScaleRotation, InCoordinateOffset));
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

/* Removes the specified MaterialRenderProxy from the list of those using an external texture. */
void FExternalTextureRegistry::RemoveMaterialRenderProxyReference(const FMaterialRenderProxy* MaterialRenderProxy)
{
	ReferencingMaterialRenderProxies.Remove(MaterialRenderProxy);
}

/* Registers the MaterialRenderProxy as using an external texture and looks up an external texture for given the specified GUID provided it is valid.
 * @return false if the texture is not registered or the GUID is invalid. 
 */
bool FExternalTextureRegistry::GetExternalTexture(const FMaterialRenderProxy* MaterialRenderProxy, const FGuid& InGuid, FTextureRHIRef& OutTextureRHI, FSamplerStateRHIRef& OutSamplerStateRHI)
{
//	FPlatformMisc::LowLevelOutputDebugStringf(TEXT("GetExternalTexture: Guid = %s"), *InGuid.ToString());

	// Only cache render proxies that have been initialized, since FMaterialRenderProxy::ReleaseDynamicRHI() is responsible for removing them
	if (MaterialRenderProxy && MaterialRenderProxy->IsInitialized())
	{
		ReferencingMaterialRenderProxies.Add(MaterialRenderProxy);
	}

	// If the GUID was not valid, we still need to register the MaterialRenderProxy so the uniform expressions can be recached when the external texture gets assigned a valid GUID.

	if (InGuid.IsValid())
	{
		FExternalTextureEntry* Entry = TextureEntries.Find(InGuid);
		if (Entry)
		{
//			FPlatformMisc::LowLevelOutputDebugStringf(TEXT("GetExternalTexture: Found"));
			OutTextureRHI = Entry->TextureRHI;
			OutSamplerStateRHI = Entry->SamplerStateRHI;
			return true;
		}
		else
		{
//			FPlatformMisc::LowLevelOutputDebugStringf(TEXT("GetExternalTexture: NOT FOUND!"));
		}
	}

	return false;
}

/* Looks up an texture coordinate scale rotation for given a given GUID
 * @return false if the texture is not registered
 */
bool FExternalTextureRegistry::GetExternalTextureCoordinateScaleRotation(const FGuid& InGuid, FLinearColor& OutCoordinateScaleRotation)
{
	FExternalTextureEntry* Entry = TextureEntries.Find(InGuid);
	if (Entry)
	{
		OutCoordinateScaleRotation = Entry->CoordinateScaleRotation;
		return true;
	}
	else
	{
		return false;
	}
}

/* Looks up an texture coordinate offset for given a given GUID
* @return false if the texture is not registered
*/
bool FExternalTextureRegistry::GetExternalTextureCoordinateOffset(const FGuid& InGuid, FLinearColor& OutCoordinateOffset)
{
	FExternalTextureEntry* Entry = TextureEntries.Find(InGuid);
	if (Entry)
	{
		OutCoordinateOffset = Entry->CoordinateOffset;
		return true;
	}
	else
	{
		return false;
	}
}
