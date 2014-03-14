// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SystemTextures.cpp: System textures implementation.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "AtmosphereTextures.h"

void FAtmosphereTextures::InitDynamicRHI()
{
	check(PrecomputeParams != NULL);
	// Allocate atmosphere precompute textures...
	{
		// todo: Expose
		// Transmittance
		FIntPoint GTransmittanceTexSize(PrecomputeParams->TransmittanceTexWidth, PrecomputeParams->TransmittanceTexHeight);
		FPooledRenderTargetDesc TransmittanceDesc(FPooledRenderTargetDesc::Create2DDesc(GTransmittanceTexSize, PF_FloatRGBA, TexCreate_None, TexCreate_RenderTargetable, false));
		GRenderTargetPool.FindFreeElement(TransmittanceDesc, AtmosphereTransmittance, TEXT("AtmosphereTransmittance"));

		RHISetRenderTarget(AtmosphereTransmittance->GetRenderTargetItem().TargetableTexture, FTextureRHIRef());
		RHIClear(true, FLinearColor(0, 0, 0, 0), false, 0, false, 0, FIntRect());
		RHICopyToResolveTarget(AtmosphereTransmittance->GetRenderTargetItem().TargetableTexture, AtmosphereTransmittance->GetRenderTargetItem().ShaderResourceTexture, true, FResolveParams());

		// Irradiance
		FIntPoint GIrradianceTexSize(PrecomputeParams->IrradianceTexWidth, PrecomputeParams->IrradianceTexHeight);
		FPooledRenderTargetDesc IrradianceDesc(FPooledRenderTargetDesc::Create2DDesc(GIrradianceTexSize, PF_FloatRGBA, TexCreate_None, TexCreate_RenderTargetable, false));
		GRenderTargetPool.FindFreeElement(IrradianceDesc, AtmosphereIrradiance, TEXT("AtmosphereIrradiance"));

		RHISetRenderTarget(AtmosphereIrradiance->GetRenderTargetItem().TargetableTexture, FTextureRHIRef());
		RHIClear(true, FLinearColor(0, 0, 0, 0), false, 0, false, 0, FIntRect());
		RHICopyToResolveTarget(AtmosphereIrradiance->GetRenderTargetItem().TargetableTexture, AtmosphereIrradiance->GetRenderTargetItem().ShaderResourceTexture, true, FResolveParams());

		// DeltaE
		GRenderTargetPool.FindFreeElement(IrradianceDesc, AtmosphereDeltaE, TEXT("AtmosphereDeltaE"));

		// 3D Texture
		// Inscatter
		FPooledRenderTargetDesc InscatterDesc(FPooledRenderTargetDesc::CreateVolumeDesc(PrecomputeParams->InscatterMuSNum * PrecomputeParams->InscatterNuNum, PrecomputeParams->InscatterMuNum, PrecomputeParams->InscatterAltitudeSampleNum, PF_FloatRGBA, TexCreate_None, TexCreate_ShaderResource | TexCreate_RenderTargetable, false));
		GRenderTargetPool.FindFreeElement(InscatterDesc, AtmosphereInscatter, TEXT("AtmosphereInscatter"));

		// DeltaSR
		GRenderTargetPool.FindFreeElement(InscatterDesc, AtmosphereDeltaSR, TEXT("AtmosphereDeltaSR"));

		// DeltaSM
		GRenderTargetPool.FindFreeElement(InscatterDesc, AtmosphereDeltaSM, TEXT("AtmosphereDeltaSM"));

		// DeltaJ
		GRenderTargetPool.FindFreeElement(InscatterDesc, AtmosphereDeltaJ, TEXT("AtmosphereDeltaJ"));
	}
}

void FAtmosphereTextures::ReleaseDynamicRHI()
{
	AtmosphereTransmittance.SafeRelease();
	AtmosphereIrradiance.SafeRelease();
	AtmosphereDeltaE.SafeRelease();

	AtmosphereInscatter.SafeRelease();
	AtmosphereDeltaSR.SafeRelease();
	AtmosphereDeltaSM.SafeRelease();
	AtmosphereDeltaJ.SafeRelease();

	GRenderTargetPool.FreeUnusedResources();
}

FArchive& operator<<(FArchive& Ar,FAtmosphereShaderTextureParameters& Parameters)
{
	Ar << Parameters.TransmittanceTexture;
	Ar << Parameters.TransmittanceTextureSampler;
	Ar << Parameters.IrradianceTexture;
	Ar << Parameters.IrradianceTextureSampler;
	Ar << Parameters.InscatterTexture;
	Ar << Parameters.InscatterTextureSampler;
	return Ar;
}
