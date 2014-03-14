// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

namespace MaterialExportUtils
{
	struct UNREALED_API FFlattenMaterial
	{
		FFlattenMaterial()
			: DiffuseSize(512, 512)
			, NormalSize(512, 512)
		{}
		
		FGuid			MaterialId;
		FIntPoint		DiffuseSize;
		FIntPoint		NormalSize;	
			
		TArray<FColor>	DiffuseSamples;
		TArray<FColor>	NormalSamples;
	};
	
	/**
	 * Renders specified material property into texture
	 *
	 * @param InMaterial			Target material
	 * @param InMaterialProperty	Material property to render
	 * @param InRenderTarget		Render target to render to
	 * @param OutBMP				Output array of rendered samples 
	 * @return						Whether operation was successful
	 */
	UNREALED_API bool ExportMaterialProperty(UMaterialInterface* InMaterial, 
												EMaterialProperty InMaterialProperty, 
												UTextureRenderTarget2D* InRenderTarget, 
												TArray<FColor>& OutBMP);
	/**
	 * Flattens specified material
	 *
	 * @param InMaterial			Target material
	 * @param OutFlattenMaterial	Output flattened material
	 * @return						Whether operation was successful
	 */
	UNREALED_API bool ExportMaterial(UMaterialInterface* InMaterial, FFlattenMaterial& OutFlattenMaterial);
	
	/**
	 * Flattens specified landscape material
	 *
	 * @param InLandscape			Target landscape
	 * @param OutFlattenMaterial	Output flattened material
	 * @return						Whether operation was successful
	 */
	UNREALED_API bool ExportMaterial(ALandscapeProxy* InLandscape, FFlattenMaterial& OutFlattenMaterial);

	/**
	 * Creates UMaterial object from a flatten material
	 *
	 * @param Outer			Outer for the material and texture objects.
	 * @param BaseName		BaseName for the material and texture objects.
	 * @param Flags			Object flags for the material and texture objects.
	 * @return				Returns a pointer to the constructed UMaterial object.
	 */
	UNREALED_API UMaterial* CreateMaterial(const FFlattenMaterial& InFlattenMaterial, UObject* Outer,const FString& BaseName, EObjectFlags Flags);
}

