// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GLTFAsset.h"
#include "Dom/JsonObject.h"
#include "Misc/FeedbackContext.h"

// approach: get away from JSON and OpenGL-specific values ASAP

// interface between JSON and in-memory data
// (the only class that needs to know about JSON)
class FGLTFReader
{
	// How many of each structure?
	uint32 BufferCount { 0 };
	uint32 BufferViewCount { 0 };
	uint32 AccessorCount { 0 };
	uint32 MeshCount { 0 };

	uint32 ImageCount { 0 };
	uint32 SamplerCount { 0 };
	uint32 TextureCount { 0 };
	uint32 MaterialCount { 0 };

	// Each mesh has its own primitives array.
	// Each primitive refers to the asset's global accessors array.

	GLTF::FAsset& Asset;

	// Binary glTF files can have embedded data after JSON.
	// This will be empty when reading from a text glTF (common) or a binary glTF with no BIN chunk (rare).
	const TArray<uint8>& BinChunk;

	const FString& Path;
	FFeedbackContext& Warn;

	void SetupBuffer(const FJsonObject&);
	void SetupBufferView(const FJsonObject&);
	void SetupAccessor(const FJsonObject&);
	void SetupPrimitive(const FJsonObject&, GLTF::FMesh&);
	void SetupMesh(const FJsonObject&);

	void SetupImage(const FJsonObject&);
	void SetupSampler(const FJsonObject&);
	void SetupTexture(const FJsonObject&);
	void SetupMaterial(const FJsonObject&);

	// Returns scale factor if JSON has it, 1.0 by default.
	float SetupMaterialTexture(GLTF::FMatTex&, const FJsonObject&, const char* TexName, const char* ScaleName);

public:
	FGLTFReader(GLTF::FAsset&, const FJsonObject& Root, const FString& Path, FFeedbackContext&, const TArray<uint8>& BinChunk);
};
