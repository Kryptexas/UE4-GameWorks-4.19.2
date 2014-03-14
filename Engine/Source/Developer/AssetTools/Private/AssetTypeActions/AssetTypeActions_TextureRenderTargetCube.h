// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_TextureRenderTargetCube : public FAssetTypeActions_TextureRenderTarget
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_TextureRenderTargetCube", "Cube Render Target"); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return UTextureRenderTargetCube::StaticClass(); }
	virtual bool CanFilter() OVERRIDE { return true; }
};