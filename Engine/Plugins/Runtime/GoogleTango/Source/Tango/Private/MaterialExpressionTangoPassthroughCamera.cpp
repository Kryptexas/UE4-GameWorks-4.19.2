// Copyright 2017 Google Inc.

#include "MaterialExpressionTangoPassthroughCamera.h"
#include "Engine/Engine.h"
#include "EngineGlobals.h"
#include "MaterialCompiler.h"
#include "TangoPassthroughCameraExternalTextureGuid.h"

UMaterialExpressionTangoPassthroughCamera::UMaterialExpressionTangoPassthroughCamera(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

#if WITH_EDITOR
int32 UMaterialExpressionTangoPassthroughCamera::Compile(class FMaterialCompiler* Compiler, int32 OutputIndex)
{
	return Compiler->TextureSample(
		Compiler->ExternalTexture(TangoPassthroughCameraExternalTextureGuid),
		Coordinates.GetTracedInput().Expression ? Coordinates.Compile(Compiler) : Compiler->TextureCoordinate(ConstCoordinate, false, false),
		EMaterialSamplerType::SAMPLERTYPE_Color);
}

int32 UMaterialExpressionTangoPassthroughCamera::CompilePreview(class FMaterialCompiler* Compiler, int32 OutputIndex)
{
	return INDEX_NONE;
}

void UMaterialExpressionTangoPassthroughCamera::GetCaption(TArray<FString>& OutCaptions) const
{
	OutCaptions.Add(TEXT("Tango Passthrough Camera"));
}
#endif
