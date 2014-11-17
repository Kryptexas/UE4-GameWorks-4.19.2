// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlateRHIRendererPrivatePCH.h"
#include "SlateMaterialResource.h"


FSlateMaterialResource::FSlateMaterialResource(UMaterialInterface& InMaterial, const FVector2D& InImageSize)
	: MaterialObject( &InMaterial )
	, RenderProxy( InMaterial.GetRenderProxy(false) )
	, SlateProxy( new FSlateShaderResourceProxy )
	, Width(FMath::RoundToInt(InImageSize.X))
	, Height(FMath::RoundToInt(InImageSize.Y))
{
	SlateProxy->ActualSize = InImageSize.IntPoint();
	SlateProxy->Resource = this;
}

FSlateMaterialResource::~FSlateMaterialResource()
{
	if(SlateProxy)
	{
		delete SlateProxy;
	}
}

