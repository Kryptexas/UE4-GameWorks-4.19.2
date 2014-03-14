// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	RenderingCompositionGraph.cpp: Scene pass order and dependency system.
=============================================================================*/

#include "RendererPrivate.h"
#include "RenderingCompositionGraph.h"
#include "HighresScreenshot.h"

// render thread, 0:off, >0 next n frames should be debugged
uint32 GDebugCompositionGraphFrames = 0;

class FGMLFileWriter
{
public:
	// constructor
	FGMLFileWriter()
		: GMLFile(0)
	{		
	}

	void OpenGMLFile(const TCHAR* Name)
	{
#if !UE_BUILD_SHIPPING
		// todo: do we need to create the directory?
		FString FilePath = FPaths::ScreenShotDir() + TEXT("/") + Name + TEXT(".gml");
		GMLFile = IFileManager::Get().CreateDebugFileWriter(*FilePath);
#endif
	}

	void CloseGMLFile()
	{
#if !UE_BUILD_SHIPPING
		if(GMLFile)
		{
			delete GMLFile;
			GMLFile = 0;
		}
#endif
	}

	// .GML file is to visualize the post processing graph as a 2d graph
	void WriteLine(const char* Line)
	{
#if !UE_BUILD_SHIPPING
		if(GMLFile)
		{
			GMLFile->Serialize((void*)Line, FCStringAnsi::Strlen(Line));
			GMLFile->Serialize((void*)"\r\n", 2);
		}
#endif
	}

private:
	FArchive* GMLFile;
};

FGMLFileWriter GGMLFileWriter;


bool ShouldDebugCompositionGraph()
{
#if !UE_BUILD_SHIPPING
	return GDebugCompositionGraphFrames > 0;
#else 
	return false;
#endif
}

void ExecuteCompositionGraphDebug()
{
	ENQUEUE_UNIQUE_RENDER_COMMAND(
		StartDebugCompositionGraph,
	{
		GDebugCompositionGraphFrames = 1;
	}
	);
}

// main thread
void CompositionGraph_OnStartFrame()
{
#if !UE_BUILD_SHIPPING
	ENQUEUE_UNIQUE_RENDER_COMMAND(
		DebugCompositionGraphDec,
	{
		if(GDebugCompositionGraphFrames)
		{
			--GDebugCompositionGraphFrames;
		}
	}
	);		
#endif
}


#if !UE_BUILD_SHIPPING
FAutoConsoleCommand CmdCompositionGraphDebug(
	TEXT("r.CompositionGraphDebug"),
	TEXT("Execute to get a single frame dump of the composition graph of one frame (post processing and lighting)."),
	FConsoleCommandDelegate::CreateStatic(ExecuteCompositionGraphDebug)
	);
#endif

FRenderingCompositePassContext::FRenderingCompositePassContext(const FViewInfo& InView/*, const FSceneRenderTargetItem& InRenderTargetItem*/)
	: View(InView)
	//		, CompositingOutputRTItem(InRenderTargetItem)
	, Pass(0)
	, ViewPortRect(0, 0, 0 ,0)
{
	check(!IsViewportValid());

	Root = Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessRoot());

	Graph.ResetDependencies();
}

FRenderingCompositePassContext::~FRenderingCompositePassContext()
{
	Graph.Free();
}

void FRenderingCompositePassContext::Process(const TCHAR *GraphDebugName)
{
	if(Root)
	{
		if(ShouldDebugCompositionGraph())
		{
			UE_LOG(LogConsoleResponse,Log, TEXT(""));
			UE_LOG(LogConsoleResponse,Log, TEXT("FRenderingCompositePassContext:Debug '%s' ---------"), GraphDebugName);
			UE_LOG(LogConsoleResponse,Log, TEXT(""));

			GGMLFileWriter.OpenGMLFile(GraphDebugName);
			GGMLFileWriter.WriteLine("Creator \"UnrealEngine4\"");
			GGMLFileWriter.WriteLine("Version \"2.10\"");
			GGMLFileWriter.WriteLine("graph");
			GGMLFileWriter.WriteLine("[");
			GGMLFileWriter.WriteLine("\tcomment\t\"This file can be viewed with yEd from yWorks. Run Layout/Hierarchical after loading.\"");
			GGMLFileWriter.WriteLine("\thierarchic\t1");
			GGMLFileWriter.WriteLine("\tdirected\t1");
		}

		FRenderingCompositeOutputRef Output(Root);

		Graph.RecursivelyGatherDependencies(Output);
		Graph.RecursivelyProcess(Root, *this);

		if(ShouldDebugCompositionGraph())
		{
			UE_LOG(LogConsoleResponse,Log, TEXT(""));

			GGMLFileWriter.WriteLine("]");
			GGMLFileWriter.CloseGMLFile();
		}
	}
}

// --------------------------------------------------------------------------

FRenderingCompositionGraph::FRenderingCompositionGraph()
{
}

FRenderingCompositionGraph::~FRenderingCompositionGraph()
{
	Free();
}

void FRenderingCompositionGraph::Free()
{
	for(uint32 i = 0; i < (uint32)Nodes.Num(); ++i)
	{
		FRenderingCompositePass *Element = Nodes[i];
		if (FMemStack::Get().ContainsPointer(Element))
		{
			Element->~FRenderingCompositePass();
		}
		else
		{
			// Call release on non-stack allocated elements
			Element->Release();
		}
	}

	Nodes.Empty();
}

void FRenderingCompositionGraph::RecursivelyGatherDependencies(const FRenderingCompositeOutputRef& InOutputRef)
{
	FRenderingCompositePass *Pass = InOutputRef.GetPass();

	if(Pass->bGraphMarker)
	{
		// already processed
		return;
	}
	Pass->bGraphMarker = true;

	// iterate through all inputs and additional dependencies of this pass
	uint32 Index = 0;
	while(const FRenderingCompositeOutputRef* OutputRefIt = Pass->GetDependency(Index++))
	{
		FRenderingCompositeOutput* InputOutput = OutputRefIt->GetOutput();
				
		if(InputOutput)
		{
			// add a dependency to this output as we are referencing to it
			InputOutput->AddDependency();
		}
		
		if(OutputRefIt->GetPass())
		{
			// recursively process all inputs of this Pass
			RecursivelyGatherDependencies(*OutputRefIt);
		}
	}

	// the pass is asked what the intermediate surface/texture format needs to be for all its outputs.
	for(uint32 OutputId = 0; ; ++OutputId)
	{
		EPassOutputId PassOutputId = (EPassOutputId)(OutputId);
		FRenderingCompositeOutput* Output = Pass->GetOutput(PassOutputId);

		if(!Output)
		{
			break;
		}

		Output->RenderTargetDesc = Pass->ComputeOutputDesc(PassOutputId);
	}
}

void FRenderingCompositionGraph::DumpOutputToFile(FRenderingCompositePassContext& Context, const FString& Filename, FRenderingCompositeOutput* Output) const 
{
	FReadSurfaceDataFlags ReadDataFlags;
	ReadDataFlags.SetLinearToGamma(false);

	FSceneRenderTargetItem& RenderTargetItem = Output->PooledRenderTarget->GetRenderTargetItem();
	FTextureRHIRef Texture = RenderTargetItem.TargetableTexture ? RenderTargetItem.TargetableTexture : RenderTargetItem.ShaderResourceTexture;

	check(Texture);
	check(Texture->GetTexture2D());

	TArray<FColor> Bitmap;

	FIntPoint Extent = Context.View.ViewRect.Size();
	RHIReadSurfaceData(Texture, FIntRect(0, 0, Extent.X, Extent.Y), Bitmap, ReadDataFlags);

	static TCHAR File[MAX_SPRINTF];
	FCString::Sprintf( File, TEXT("%s.bmp"), *Filename);

	uint32 ExtendXWithMSAA = Bitmap.Num() / Extent.Y;
	uint32 CaptureRegionScaleX = 1.0f;
	if (ExtendXWithMSAA > (uint32)Extent.X)
	{
		CaptureRegionScaleX = ExtendXWithMSAA / (uint32)Extent.X;
	}

	// Save the contents of the array to a bitmap file. (24bit only so alpha channel is dropped)
	FIntRect SourceRect;
	
	if (GIsHighResScreenshot)
	{
		SourceRect = GetHighResScreenshotConfig().CaptureRegion;
		SourceRect.Min.X *= CaptureRegionScaleX;
		SourceRect.Max.X *= CaptureRegionScaleX;
	}
	
	FFileHelper::CreateBitmap(File, ExtendXWithMSAA, Extent.Y, Bitmap.GetTypedData(), GIsHighResScreenshot ? &SourceRect : NULL);	

	UE_LOG(LogConsoleResponse, Display, TEXT("Content was saved to \"%s\""), *FPaths::ScreenShotDir());
}

void FRenderingCompositionGraph::RecursivelyProcess(const FRenderingCompositeOutputRef& InOutputRef, FRenderingCompositePassContext& Context) const
{
	FRenderingCompositePass *Pass = InOutputRef.GetPass();
	FRenderingCompositeOutput *Output = InOutputRef.GetOutput();

#if !UE_BUILD_SHIPPING
	if(!Pass || !Output)
	{
		// to track down a crash bug
		if(Context.Pass)
		{
			UE_LOG(LogRenderer,Fatal, TEXT("FRenderingCompositionGraph::RecursivelyProcess %s"), *Context.Pass->ConstructDebugName());
		}
	}
#endif

	check(Pass);
	check(Output);

	if(!Pass->bGraphMarker)
	{
		// already processed
		return;
	}
	Pass->bGraphMarker = false;

	// iterate through all inputs and additional dependencies of this pass
	{
		uint32 Index = 0;

		while(const FRenderingCompositeOutputRef* OutputRefIt = Pass->GetDependency(Index++))
		{
			if(OutputRefIt->GetPass())
			{
				if(!OutputRefIt)
				{
					// Pass doesn't have more inputs
					break;
				}

				FRenderingCompositeOutput* Input = OutputRefIt->GetOutput();
				
				// to track down an issue, should never happen
				check(OutputRefIt->GetPass());

				Context.Pass = Pass;
				RecursivelyProcess(*OutputRefIt, Context);
			}
		}
	}

	if(ShouldDebugCompositionGraph())
	{
		GGMLFileWriter.WriteLine("\tnode");
		GGMLFileWriter.WriteLine("\t[");

		int32 PassId = ComputeUniquePassId(Pass);
		FString PassDebugName = Pass->ConstructDebugName();

		ANSICHAR Line[MAX_SPRINTF];

		{
			GGMLFileWriter.WriteLine("\t\tgraphics");
			GGMLFileWriter.WriteLine("\t\t[");
			FCStringAnsi::Sprintf(Line, "\t\t\tw\t%d", 200);
			GGMLFileWriter.WriteLine(Line);
			FCStringAnsi::Sprintf(Line, "\t\t\th\t%d", 80);
			GGMLFileWriter.WriteLine(Line);
			GGMLFileWriter.WriteLine("\t\t\tfill\t\"#FFCCCC\"");
			GGMLFileWriter.WriteLine("\t\t]");
		}

		{
			FCStringAnsi::Sprintf(Line, "\t\tid\t%d", PassId);
			GGMLFileWriter.WriteLine(Line);
			GGMLFileWriter.WriteLine("\t\tLabelGraphics");
			GGMLFileWriter.WriteLine("\t\t[");
			FCStringAnsi::Sprintf(Line, "\t\t\ttext\t\"#%d\r%s\"", PassId, (const char *)TCHAR_TO_ANSI(*PassDebugName));
			GGMLFileWriter.WriteLine(Line);
			GGMLFileWriter.WriteLine("\t\t\tanchor\t\"t\"");	// put label internally on top
			GGMLFileWriter.WriteLine("\t\t\tfontSize\t14");
			GGMLFileWriter.WriteLine("\t\t\tfontStyle\t\"bold\"");
			GGMLFileWriter.WriteLine("\t\t]");
		}

		UE_LOG(LogConsoleResponse,Log, TEXT("Node#%d '%s'"), PassId, *PassDebugName);
	
		GGMLFileWriter.WriteLine("\t\tisGroup\t1");
		GGMLFileWriter.WriteLine("\t]");

		uint32 InputId = 0;
		while(FRenderingCompositeOutputRef* OutputRefIt = Pass->GetInput((EPassInputId)(InputId++)))
		{
			if(OutputRefIt->Source)
			{
				// source is hooked up 
				FString InputName = OutputRefIt->Source->ConstructDebugName();

				int32 TargetPassId = ComputeUniquePassId(OutputRefIt->Source);

				UE_LOG(LogConsoleResponse,Log, TEXT("  ePId_Input%d: Node#%d @ ePId_Output%d '%s'"), InputId - 1, TargetPassId, (uint32)OutputRefIt->PassOutputId, *InputName);

				// input connection to another node
				{
					GGMLFileWriter.WriteLine("\tedge");
					GGMLFileWriter.WriteLine("\t[");
					{
						ANSICHAR Line[MAX_SPRINTF];

						FCStringAnsi::Sprintf(Line, "\t\tsource\t%d", ComputeUniqueOutputId(OutputRefIt->Source, OutputRefIt->PassOutputId));
						GGMLFileWriter.WriteLine(Line);
						FCStringAnsi::Sprintf(Line, "\t\ttarget\t%d", PassId);
						GGMLFileWriter.WriteLine(Line);
					}
					{
						FString EdgeName = FString::Printf(TEXT("ePId_Input%d"), InputId - 1);

						GGMLFileWriter.WriteLine("\t\tLabelGraphics");
						GGMLFileWriter.WriteLine("\t\t[");
						FCStringAnsi::Sprintf(Line, "\t\t\ttext\t\"%s\"", (const char *)TCHAR_TO_ANSI(*EdgeName));
						GGMLFileWriter.WriteLine(Line);
						GGMLFileWriter.WriteLine("\t\t\tmodel\t\"three_center\"");
						GGMLFileWriter.WriteLine("\t\t\tposition\t\"tcentr\"");
						GGMLFileWriter.WriteLine("\t\t]");
					}
					GGMLFileWriter.WriteLine("\t]");
				}
			}
			else
			{
				// source is not hooked up 
				UE_LOG(LogConsoleResponse,Log, TEXT("  ePId_Input%d:"), InputId - 1);
			}
		}

		uint32 DepId = 0;
		while(FRenderingCompositeOutputRef* OutputRefIt = Pass->GetAdditionalDependency(DepId++))
		{
			check(OutputRefIt->Source);

			FString InputName = OutputRefIt->Source->ConstructDebugName();
			int32 TargetPassId = ComputeUniquePassId(OutputRefIt->Source);

			UE_LOG(LogConsoleResponse,Log, TEXT("  Dependency: Node#%d @ ePId_Output%d '%s'"), TargetPassId, (uint32)OutputRefIt->PassOutputId, *InputName);

			// dependency connection to another node
			{
				GGMLFileWriter.WriteLine("\tedge");
				GGMLFileWriter.WriteLine("\t[");
				{
					ANSICHAR Line[MAX_SPRINTF];

					FCStringAnsi::Sprintf(Line, "\t\tsource\t%d", ComputeUniqueOutputId(OutputRefIt->Source, OutputRefIt->PassOutputId));
					GGMLFileWriter.WriteLine(Line);
					FCStringAnsi::Sprintf(Line, "\t\ttarget\t%d", PassId);
					GGMLFileWriter.WriteLine(Line);
				}
				// dashed line
				{
					GGMLFileWriter.WriteLine("\t\tgraphics");
					GGMLFileWriter.WriteLine("\t\t[");
					GGMLFileWriter.WriteLine("\t\t\tstyle\t\"dashed\"");
					GGMLFileWriter.WriteLine("\t\t]");
				}
				{
					FString EdgeName = TEXT("Dependency");

					GGMLFileWriter.WriteLine("\t\tLabelGraphics");
					GGMLFileWriter.WriteLine("\t\t[");
					FCStringAnsi::Sprintf(Line, "\t\t\ttext\t\"%s\"", (const char *)TCHAR_TO_ANSI(*EdgeName));
					GGMLFileWriter.WriteLine(Line);
					GGMLFileWriter.WriteLine("\t\t\tmodel\t\"three_center\"");
					GGMLFileWriter.WriteLine("\t\t\tposition\t\"tcentr\"");
					GGMLFileWriter.WriteLine("\t\t]");
				}
				GGMLFileWriter.WriteLine("\t]");
			}
		}

		uint32 OutputId = 0;
		while(FRenderingCompositeOutput* Output = Pass->GetOutput((EPassOutputId)(OutputId)))
		{
			UE_LOG(LogConsoleResponse,Log, TEXT("  ePId_Output%d %s %s Dep: %d"), OutputId, *Output->RenderTargetDesc.GenerateInfoString(), Output->RenderTargetDesc.DebugName, Output->GetDependencyCount());

			GGMLFileWriter.WriteLine("\tnode");
			GGMLFileWriter.WriteLine("\t[");

			ANSICHAR Line[MAX_SPRINTF];

			{
				GGMLFileWriter.WriteLine("\t\tgraphics");
				GGMLFileWriter.WriteLine("\t\t[");
				FCStringAnsi::Sprintf(Line, "\t\t\tw\t%d", 220);
				GGMLFileWriter.WriteLine(Line);
				FCStringAnsi::Sprintf(Line, "\t\t\th\t%d", 40);
				GGMLFileWriter.WriteLine(Line);
				GGMLFileWriter.WriteLine("\t\t]");
			}

			{
				FCStringAnsi::Sprintf(Line, "\t\tid\t%d", ComputeUniqueOutputId(Pass, (EPassOutputId)(OutputId)));
				GGMLFileWriter.WriteLine(Line);
				GGMLFileWriter.WriteLine("\t\tLabelGraphics");
				GGMLFileWriter.WriteLine("\t\t[");
				FCStringAnsi::Sprintf(Line, "\t\t\ttext\t\"ePId_Output%d '%s'\r%s\"", 
					OutputId, 
					(const char *)TCHAR_TO_ANSI(Output->RenderTargetDesc.DebugName),
					(const char *)TCHAR_TO_ANSI(*Output->RenderTargetDesc.GenerateInfoString()));
				GGMLFileWriter.WriteLine(Line);
				GGMLFileWriter.WriteLine("\t\t]");
			}


			{
				FCStringAnsi::Sprintf(Line, "\t\tgid\t%d", PassId);
				GGMLFileWriter.WriteLine(Line);
			}

			GGMLFileWriter.WriteLine("\t]");

			++OutputId;
		}

		UE_LOG(LogConsoleResponse,Log, TEXT(""));
	}

	Context.Pass = Pass;
	Context.SetViewportInvalid();

	// then process the pass itself
	Pass->Process(Context);

	// for VisualizeTexture and output buffer dumping
	{
		uint32 OutputId = 0;

		while(FRenderingCompositeOutput* PassOutput = Pass->GetOutput((EPassOutputId)OutputId))
		{
			// use intermediate texture unless it's the last one where we render to the final output
			if(PassOutput->PooledRenderTarget)
			{
				GRenderTargetPool.VisualizeTexture.SetCheckPoint(PassOutput->PooledRenderTarget);

				// If this buffer was given a dump filename, write it out
				const FString& Filename = Pass->GetOutputDumpFilename((EPassOutputId)OutputId);
				if (!Filename.IsEmpty())
				{
					DumpOutputToFile(Context, Filename, PassOutput);
				}

				// If we've been asked to write out the pixel data for this pass to an external array, do it now
				TArray<FColor>* OutputColorArray = Pass->GetOutputColorArray((EPassOutputId)OutputId);
				if (OutputColorArray)
				{
					RHIReadSurfaceData(
						PassOutput->PooledRenderTarget->GetRenderTargetItem().TargetableTexture,
						Context.View.ViewRect,
						*OutputColorArray,
						FReadSurfaceDataFlags()
						);
				}
			}

			OutputId++;
		}
	}

	// iterate through all inputs of this pass and decrement the references for it's inputs
	// this can release some intermediate RT so they can be reused
	{
		uint32 InputId = 0;

		while(const FRenderingCompositeOutputRef* OutputRefIt = Pass->GetDependency(InputId++))
		{
			FRenderingCompositeOutput* Input = OutputRefIt->GetOutput();

			if(Input)
			{
				Input->ResolveDependencies();
			}
		}
	}
}

// for debugging purpose O(n)
int32 FRenderingCompositionGraph::ComputeUniquePassId(FRenderingCompositePass* Pass) const
{
	for(uint32 i = 0; i < (uint32)Nodes.Num(); ++i)
	{
		FRenderingCompositePass *Element = Nodes[i];

		if(Element == Pass)
		{
			return i;
		}
	}

	return -1;
}

int32 FRenderingCompositionGraph::ComputeUniqueOutputId(FRenderingCompositePass* Pass, EPassOutputId OutputId) const
{
	uint32 Ret = Nodes.Num();

	for(uint32 i = 0; i < (uint32)Nodes.Num(); ++i)
	{
		FRenderingCompositePass *Element = Nodes[i];

		if(Element == Pass)
		{
			return (int32)(Ret + (uint32)OutputId);
		}

		uint32 OutputCount = 0;
		while(Pass->GetOutput((EPassOutputId)OutputCount))
		{
			++OutputCount;
		}

		Ret += OutputCount;
	}

	return -1;
}


void FRenderingCompositionGraph::ResetDependencies()
{
	for(uint32 i = 0; i < (uint32)Nodes.Num(); ++i)
	{
		FRenderingCompositePass *Element = Nodes[i];

		Element->bGraphMarker = false;

		uint32 OutputId = 0;

		while(FRenderingCompositeOutput* Output = Element->GetOutput((EPassOutputId)(OutputId++)))
		{
			Output->ResetDependency();
		}
	}
}

FRenderingCompositeOutput *FRenderingCompositeOutputRef::GetOutput() const
{
	if(Source == 0)
	{
		return 0;
	}

	return Source->GetOutput(PassOutputId); 
}

FRenderingCompositePass* FRenderingCompositeOutputRef::GetPass() const
{
	return Source;
}

// -----------------------------------------------------------------

void FPostProcessPassParameters::Bind(const FShaderParameterMap& ParameterMap)
{
	BilinearTextureSampler0.Bind(ParameterMap,TEXT("BilinearTextureSampler0"));
	BilinearTextureSampler1.Bind(ParameterMap,TEXT("BilinearTextureSampler1"));
	ViewportSize.Bind(ParameterMap,TEXT("ViewportSize"));
	ViewportRect.Bind(ParameterMap,TEXT("ViewportRect"));
	ScreenPosToPixel.Bind(ParameterMap,TEXT("ScreenPosToPixel"));
	
	for(uint32 i = 0; i < ePId_Input_MAX; ++i)
	{
		PostprocessInputParameter[i].Bind(ParameterMap, *FString::Printf(TEXT("PostprocessInput%d"), i));
		PostprocessInputParameterSampler[i].Bind(ParameterMap, *FString::Printf(TEXT("PostprocessInput%dSampler"), i));
		PostprocessInputNew[i].Bind(ParameterMap, *FString::Printf(TEXT("InputNew%d"), i));
		PostprocessInputSizeParameter[i].Bind(ParameterMap, *FString::Printf(TEXT("PostprocessInput%dSize"), i));
		PostProcessInputMinMaxParameter[i].Bind(ParameterMap, *FString::Printf(TEXT("PostprocessInput%dMinMax"), i));
	}
}

void FPostProcessPassParameters::SetPS(const FPixelShaderRHIParamRef& ShaderRHI, const FRenderingCompositePassContext& Context, FSamplerStateRHIParamRef Filter, bool bWhiteIfNoTexture, FSamplerStateRHIParamRef* FilterOverrideArray)
{
	Set(ShaderRHI, Context, Filter, bWhiteIfNoTexture, FilterOverrideArray);
}

void FPostProcessPassParameters::SetCS(const FComputeShaderRHIParamRef& ShaderRHI, const FRenderingCompositePassContext& Context, FSamplerStateRHIParamRef Filter, bool bWhiteIfNoTexture, FSamplerStateRHIParamRef* FilterOverrideArray)
{
	Set(ShaderRHI, Context, Filter, bWhiteIfNoTexture, FilterOverrideArray);
}

void FPostProcessPassParameters::SetVS(const FVertexShaderRHIParamRef& ShaderRHI, const FRenderingCompositePassContext& Context, FSamplerStateRHIParamRef Filter, bool bWhiteIfNoTexture, FSamplerStateRHIParamRef* FilterOverrideArray)
{
	Set(ShaderRHI, Context, Filter, bWhiteIfNoTexture, FilterOverrideArray);
}

FArchive& operator<<(FArchive& Ar, FPostProcessPassParameters& P)
{
	Ar << P.BilinearTextureSampler0 << P.BilinearTextureSampler1 << P.ViewportSize << P.ScreenPosToPixel << P.ViewportRect;

	for(uint32 i = 0; i < ePId_Input_MAX; ++i)
	{
		Ar << P.PostprocessInputParameter[i];
		Ar << P.PostprocessInputParameterSampler[i];
		Ar << P.PostprocessInputNew[i];
		Ar << P.PostprocessInputSizeParameter[i];
		Ar << P.PostProcessInputMinMaxParameter[i];
	}

	return Ar;
}

// -----------------------------------------------------------------

const FSceneRenderTargetItem& FRenderingCompositeOutput::RequestSurface(const FRenderingCompositePassContext& Context)
{
	if(PooledRenderTarget)
	{
		return PooledRenderTarget->GetRenderTargetItem();
	}

	if(!RenderTargetDesc.IsValid())
	{
		// useful to use the CompositingGraph dependency resolve but pass the data between nodes differently
		static FSceneRenderTargetItem Null;

		return Null;
	}

	if(!PooledRenderTarget)
	{
		GRenderTargetPool.FindFreeElement(RenderTargetDesc, PooledRenderTarget, RenderTargetDesc.DebugName);
	}

	check(!PooledRenderTarget->IsFree());

	FSceneRenderTargetItem& RenderTargetItem = PooledRenderTarget->GetRenderTargetItem();

	return RenderTargetItem;
}


const FPooledRenderTargetDesc* FRenderingCompositePass::GetInputDesc(EPassInputId InPassInputId) const
{
	// to overcome const issues, this way it's kept local
	FRenderingCompositePass* This = (FRenderingCompositePass*)this;

	const FRenderingCompositeOutputRef* OutputRef = This->GetInput(InPassInputId);

	if(!OutputRef)
	{
		return 0;
	}

	FRenderingCompositeOutput* Input = OutputRef->GetOutput();

	if(!Input)
	{
		return 0;
	}

	return &Input->RenderTargetDesc;
}

uint32 FRenderingCompositePass::ComputeInputCount()
{
	for(uint32 i = 0; ; ++i)
	{
		if(!GetInput((EPassInputId)i))
		{
			return i;
		}
	}
}

uint32 FRenderingCompositePass::ComputeOutputCount()
{
	for(uint32 i = 0; ; ++i)
	{
		if(!GetOutput((EPassOutputId)i))
		{
			return i;
		}
	}
}

FString FRenderingCompositePass::ConstructDebugName()
{
	FString Name;

	uint32 OutputId = 0;
	while(FRenderingCompositeOutput* Output = GetOutput((EPassOutputId)OutputId))
	{
		Name += Output->RenderTargetDesc.DebugName;

		++OutputId;
	}

	if(Name.IsEmpty())
	{
		Name = TEXT("UnknownName");
	}

	return Name;
}
