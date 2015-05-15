// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"
#include "GroupedSpriteSceneProxy.h"
#include "PaperGroupedSpriteComponent.h"

//////////////////////////////////////////////////////////////////////////
// FGroupedSpriteSceneProxy

FSpriteRenderSection& FGroupedSpriteSceneProxy::FindOrAddSection(FSpriteDrawCallRecord& InBatch, UMaterialInterface* InMaterial)
{
	// Check the existing sections, starting with the most recent
	for (int32 SectionIndex = BatchedSections.Num() - 1; SectionIndex >= 0; --SectionIndex)
	{
		FSpriteRenderSection& TestSection = BatchedSections[SectionIndex];

		if (TestSection.Material == InMaterial)
		{
			if (TestSection.BaseTexture == InBatch.BaseTexture)
			{
				if (TestSection.AdditionalTextures == InBatch.AdditionalTextures)
				{
					return TestSection;
				}
			}
		}
	}

	// Didn't find a matching section, create one
	FSpriteRenderSection& NewSection = *(new (BatchedSections) FSpriteRenderSection());
	NewSection.Material = InMaterial;
	NewSection.BaseTexture = InBatch.BaseTexture;
	NewSection.AdditionalTextures = InBatch.AdditionalTextures;
	NewSection.VertexOffset = VertexBuffer.Vertices.Num();

	return NewSection;
}

FGroupedSpriteSceneProxy::FGroupedSpriteSceneProxy(UPaperGroupedSpriteComponent* InComponent)
	: FPaperRenderSceneProxy(InComponent)
	, MyComponent(InComponent)
{
	MaterialRelevance = InComponent->GetMaterialRelevance(GetScene().GetFeatureLevel());

	NumInstances = InComponent->PerInstanceSpriteData.Num();

	for (const FSpriteInstanceData InstanceData : InComponent->PerInstanceSpriteData)
	{
		if (InstanceData.SourceSprite != nullptr)
		{
			FSpriteDrawCallRecord Record;
			Record.BuildFromSprite(InstanceData.SourceSprite);

			UMaterialInterface* SpriteMaterial = InstanceData.SourceSprite->GetMaterial(0); //@TODO: Should query from the component due to overrides, but that requires an index per item, etc...

			FSpriteRenderSection& Section = FindOrAddSection(Record, SpriteMaterial);
			
			
			const int32 NumNewVerts = Record.RenderVerts.Num();
			Section.NumVertices += NumNewVerts;

			const FColor VertColor(InstanceData.VertexColor);
			for (const FVector4& SourceVert : Record.RenderVerts)
			{
				const FVector LocalPos((PaperAxisX * SourceVert.X) + (PaperAxisY * SourceVert.Y));
				const FVector ComponentSpacePos = InstanceData.Transform.TransformPosition(LocalPos);
				const FVector2D UV(SourceVert.Z, SourceVert.W);

				new (VertexBuffer.Vertices) FPaperSpriteVertex(ComponentSpacePos, UV, VertColor);
			}
		}
	}
	
	if (VertexBuffer.Vertices.Num() > 0)
	{
		// Init the vertex factory
		MyVertexFactory.Init(&VertexBuffer);

		// Enqueue initialization of render resources
		BeginInitResource(&VertexBuffer);
		BeginInitResource(&MyVertexFactory);
	}
}
