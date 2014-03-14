// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessVisualizeBuffer.h: Post processing buffer visualization
=============================================================================*/

#pragma once

#include "RenderingCompositionGraph.h"

// derives from TRenderingCompositePassBase<InputCount, OutputCount>
// ePId_Input0: SceneColor
// ePId_Input1: SeparateTranslucency
class FRCPassPostProcessVisualizeBuffer : public TRenderingCompositePassBase<2, 1>
{
public:

	/** Data for a single buffer overview tile **/
	struct TileData
	{
		FRenderingCompositeOutputRef Source;
		FString Name;

		TileData(FRenderingCompositeOutputRef InSource, const FString& InName)
		: Source (InSource)
		, Name (InName)
		{

		}
	};

	// constructor
	void AddVisualizationBuffer(FRenderingCompositeOutputRef InSource, const FString& InName);

	// interface FRenderingCompositePass ---------

	virtual void Process(FRenderingCompositePassContext& Context);
	virtual void Release() OVERRIDE { delete this; }
	virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const;

private:

	template <bool bDrawTileTemplate>
	void SetShaderTempl(const FRenderingCompositePassContext& Context);

	TArray<TileData> Tiles;
};
