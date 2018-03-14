// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#if ARKIT_SUPPORT && __IPHONE_OS_VERSION_MAX_ALLOWED >= 110000

FORCEINLINE TArray<int32> To32BitIndexBuffer(const int16_t* Indices, const uint64 IndexCount)
{
	check(IndexCount % 3 == 0);

	TArray<int32> IndexBuffer;
	IndexBuffer.AddUninitialized(IndexCount);
	for (uint32 Index = 0; Index < IndexCount; Index += 3)
	{
		IndexBuffer[Index] = (int32)Indices[Index];
		// We need to reverse the winding order
		IndexBuffer[Index + 1] = (int32)Indices[Index + 2];
		IndexBuffer[Index + 2] = (int32)Indices[Index + 1];
	}
	return IndexBuffer;
}

// @todo JoeG - An option for which way to orient tris (down +X or -X)
FORCEINLINE TArray<FVector> ToVertexBuffer(const vector_float3* Vertices, const uint64 VertexCount)
{
	TArray<FVector> VertexBuffer;
	VertexBuffer.AddUninitialized(VertexCount);
	// @todo JoeG - make a fast routine for this
	for (int32 Index = 0; Index < VertexBuffer.Num(); Index++)
	{
		VertexBuffer[Index] = FVector(Vertices[Index].z, Vertices[Index].x, Vertices[Index].y);
	}
	return VertexBuffer;
}

#endif
