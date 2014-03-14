// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	RenderingCompositionGraph.h: Scene pass order and dependency system.
=============================================================================*/

#pragma once

struct FRenderingCompositePass;
struct FRenderingCompositeOutputRef;
struct FRenderingCompositeOutput;
class FRenderingCompositionGraph;
class FViewInfo;
class FShaderParameterMap;

// This is the index for the texture input of this pass.
// More that that should not be needed.
// Could be an uint32 but for better readability and type safety it's an enum.
// Counting starts from 0 in consecutive order.
enum EPassInputId
{
	ePId_Input0,
	ePId_Input1,
	ePId_Input2,
	ePId_Input3,
	ePId_Input4,
	ePId_Input5,
	ePId_Input6,
/*	ePId_Input7,
	ePId_Input8,
	ePId_Input9,
	ePId_Input10,
	ePId_Input11,
	ePId_Input12,
	ePId_Input13,
	ePId_Input14,
	ePId_Input15*/

	// to get the total count of inputs
	ePId_Input_MAX
};

// Usually the same as the MRT number but it doesn't have to be implemented as MRT.
// More that that should not be needed.
// Could be an uint32 but for better readability and type safety it's an enum.
// Counting starts from 0 in consecutive order.
enum EPassOutputId
{
	ePId_Output0,
	ePId_Output1,
	ePId_Output2,
	ePId_Output3,
	ePId_Output4,
	ePId_Output5,
	ePId_Output6,
	ePId_Output7
};

struct FRenderingCompositeOutputRef;
struct FRenderingCompositePassContext;

class FRenderingCompositionGraph
{
public:
	FRenderingCompositionGraph();
	~FRenderingCompositionGraph();

	/**
	 * Returns the input pointer as output to allow this:
	 * Example:  SceneColor = Graph.RegisterPass(new FRCPassPostProcessInput(GSceneRenderTargets.SceneColor));
	 * @param InPass - must not be 0
	 */
	template<class T>
	T* RegisterPass(T* InPass)
	{
		check(InPass);
		Nodes.Add(InPass);

		return InPass;
	}

	friend struct FRenderingCompositePassContext;

private:
	/** */
	TArray<FRenderingCompositePass*> Nodes;

	/** */
	void Free();

	/** calls ResetDependency() in OutputId in each pass */
	void ResetDependencies();

	/** */
	void ProcessGatherDependency(const FRenderingCompositeOutputRef* OutputRefIt);

	/** should only be called by GatherDependencies(), can also be implemented without recursion */
	static void RecursivelyGatherDependencies(const FRenderingCompositeOutputRef& InOutputRef);

	/** can also be implemented without recursion */
	void RecursivelyProcess(const FRenderingCompositeOutputRef& InOutputRef, FRenderingCompositePassContext& Context) const;

	/** Write the contents of the specified output to a file */
	void DumpOutputToFile(FRenderingCompositePassContext& Context, const FString& Filename, FRenderingCompositeOutput* Output) const;

	/**
	 * for debugging purpose O(n)
	 * @return -1 if not found
	 */
	int32 ComputeUniquePassId(FRenderingCompositePass* Pass) const;

	/**
	 * for debugging purpose O(n), unique and not overlapping with the PassId
	 * @return -1 if not found
	 */
	int32 ComputeUniqueOutputId(FRenderingCompositePass* Pass, EPassOutputId OutputId) const;
};


struct FRenderingCompositePassContext
{
	FRenderingCompositePassContext(const FViewInfo& InView);

	~FRenderingCompositePassContext();

	// @param GraphDebugName must not be 0
	void Process(const TCHAR *GraphDebugName);

	//
	const FViewInfo& View;
	// is updated before each Pass->Process() call
	FRenderingCompositePass* Pass;

	// call this method instead of RHISetViewport() so we can cache the values and use them to map beteen ScreenPos and pixels
	void SetViewportAndCallRHI(FIntRect InViewPortRect, float InMinZ = 0.0f, float InMaxZ = 1.0f)
	{
		ViewPortRect = InViewPortRect;

		RHISetViewport(ViewPortRect.Min.X, ViewPortRect.Min.Y, InMinZ, ViewPortRect.Max.X, ViewPortRect.Max.Y, InMaxZ);
	}

	// call this method instead of RHISetViewport() so we can cache the values and use them to map beteen ScreenPos and pixels
	void SetViewportAndCallRHI(uint32 InMinX, uint32 InMinY, float InMinZ, uint32 InMaxX, uint32 InMaxY, float InMaxZ)
	{
		SetViewportAndCallRHI(FIntRect(InMinX, InMinY, InMaxX, InMaxY), InMinZ, InMaxZ);

		// otherwise the input parameters are bad
		check(IsViewportValid());
	}

	// should be called before each pass so we don't get state from the pass before
	void SetViewportInvalid()
	{
		ViewPortRect = FIntRect(0, 0, 0, 0);

		check(!IsViewportValid());
	}
	
	//
	FIntRect GetViewport() const
	{
		// need to call SetViewportAndCallRHI() before
		check(IsViewportValid());

		return ViewPortRect;
	}

	bool IsViewportValid() const
	{
		return ViewPortRect.Min != ViewPortRect.Max;
	}

	FRenderingCompositePass* Root;

	FRenderingCompositionGraph Graph;

private:
	// cached state to map between ScreenPos and pixels
	FIntRect ViewPortRect;
};

// ---------------------------------------------------------------------------

struct FRenderingCompositePass
{
	/** constructor */
	FRenderingCompositePass() : bGraphMarker(false)
	{
	}

	virtual ~FRenderingCompositePass() {}

	/** @return 0 if outside the range */
	virtual FRenderingCompositeOutputRef* GetInput(EPassInputId InPassInputId) = 0;

	/**
	 * Each input is a dependency and will be processed before the node itself (don't generate cyles)
	 * The index allows to access the input in Process() and on the shader side
	 * @param InInputIndex silently ignores calls outside the range
	 */
	virtual void SetInput(EPassInputId InPassInputId, const FRenderingCompositeOutputRef& InOutputRef) = 0;

	/**
	 * Allows to add additional dependencies (cannot be accessed by the node but need to be processed before the node)
	 */
	virtual void AddDependency(const FRenderingCompositeOutputRef& InOutputRef) = 0;

	/** @param Parent the one that was pointing to *this */
	virtual void Process(FRenderingCompositePassContext& Context) = 0;

	// @return true: ePId_Input0 is used as output, cannot make texture lookups, does not support MRT yet
	virtual bool FrameBufferBlendingWithInput0() const { return false; }

	/** @return 0 if outside the range */
	virtual FRenderingCompositeOutput* GetOutput(EPassOutputId InPassOutputId) = 0;

	/**
	 * Allows to iterate through all dependencies (inputs and additional dependency)
	 * @return 0 if outside the range
	 */
	virtual FRenderingCompositeOutputRef* GetDependency(uint32 Index) = 0;

	/**
	 * Allows to iterate through all additional dependencies
	 * @return 0 if outside the range
	 */
	virtual FRenderingCompositeOutputRef* GetAdditionalDependency(uint32 Index) = 0;

	/**
	 * Allows access to dump filename for a given output
	 * @return Filename for output dump
	 */
	virtual const TCHAR* GetOutputDumpFilename(EPassOutputId OutputId) const = 0;

	/**
	 * Allows setting of a dump filename for a given output
	 * @param Index - Output index
	 * @param Filename - Output dump filename
	 */
	virtual void SetOutputDumpFilename(EPassOutputId OutputId, const TCHAR* Filename) = 0;

	/**
	 * Allows access to an optional TArray of colors in which to capture the pass output
	 * @return Filename for output dump
	 */
	virtual TArray<FColor>* GetOutputColorArray(EPassOutputId OutputId) const = 0;

	/**
	 * Allows setting of a pointer to a color array, into which the specified pass output will be copied
	 * @param Index - Output index
	 * @param OutputBuffer - Output array pointer
	 */
	virtual void SetOutputColorArray(EPassOutputId OutputId, TArray<FColor>* OutputBuffer) = 0;

	/** */
	virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const = 0;

	/** Convenience method as this could have been done with GetInput() alone, performance: O(n) */
	uint32 ComputeInputCount();

	/** Convenience method as this could have been done with GetOutput() alone, performance: O(n) */
	uint32 ComputeOutputCount();

	FString ConstructDebugName();

	/**
	 * Convenience method, is using other virtual methods.
	 * @return 0 if there is an error
	 */
	const FPooledRenderTargetDesc* GetInputDesc(EPassInputId InPassInputId) const;

	/** */
	virtual void Release() = 0;

private:

	/** to allow the graph to mark already processed nodes */
	bool bGraphMarker;

	friend class FRenderingCompositionGraph;
};

struct FRenderingCompositeOutputRef
{
	FRenderingCompositeOutputRef(FRenderingCompositePass* InSource = 0, EPassOutputId InPassOutputId = ePId_Output0)
		:Source(InSource), PassOutputId(InPassOutputId)
	{
	}

	FRenderingCompositePass* GetPass() const; 

	/** @return can be 0 */
	FRenderingCompositeOutput* GetOutput() const;

	EPassOutputId GetOutputId() const { return PassOutputId; }

	bool IsValid() const
	{
		return Source != 0;
	}

private:
	/** can be 0 */
	FRenderingCompositePass* Source;
	/** to call Source->GetInput(SourceSubIndex) */
	EPassOutputId PassOutputId;

	friend class FRenderingCompositionGraph;
};

struct FRenderingCompositeOutput
{
	FRenderingCompositeOutput()
		:Dependencies(0)
	{
	}

	void ResetDependency()
	{
		Dependencies = 0;
	}

	void AddDependency()
	{
		++Dependencies;
	}

	uint32 GetDependencyCount() const
	{
		return Dependencies;
	}

	void ResolveDependencies()
	{
		if(Dependencies > 0)
		{
			--Dependencies;

			if(!Dependencies)
			{
				// the internal reference is released
				PooledRenderTarget.SafeRelease();
			}
		}
	}

	/** Get the texture to read from */
	TRefCountPtr<IPooledRenderTarget> RequestInput()
	{
//		check(PooledRenderTarget);
		check(Dependencies > 0);

		return PooledRenderTarget;
	}

	/**
	 * get the surface to write to
	 * @param DebugName must not be 0
	 */
	const FSceneRenderTargetItem& RequestSurface(const FRenderingCompositePassContext& Context);

	// private:
	FPooledRenderTargetDesc RenderTargetDesc; 
	TRefCountPtr<IPooledRenderTarget> PooledRenderTarget;


private:

	uint32 Dependencies;
};

//
template <uint32 InputCount, uint32 OutputCount>
struct TRenderingCompositePassBase :public FRenderingCompositePass
{
	TRenderingCompositePassBase()
	{
		for (uint32 i = 0; i < OutputCount; ++i)
		{
			PassOutputColorArrays[i] = nullptr;
		}
	}

	virtual ~TRenderingCompositePassBase()
	{
	}

	// interface FRenderingCompositePass

	virtual FRenderingCompositeOutputRef* GetInput(EPassInputId InPassInputId)
	{
		if((int32)InPassInputId < InputCount)
		{
			return &PassInputs[InPassInputId];
		}

		return 0;
	}
	
	virtual void SetInput(EPassInputId InPassInputId, const FRenderingCompositeOutputRef& VirtualBuffer)
	{
		if((int32)InPassInputId < InputCount)
		{
			PassInputs[InPassInputId] = VirtualBuffer;
		}
		else
		{
			// this node doesn't have this input
			check(0);
		}
	}

	void AddDependency(const FRenderingCompositeOutputRef& InOutputRef)
	{
		AdditionalDependencies.Add(InOutputRef);
	}

	virtual FRenderingCompositeOutput* GetOutput(EPassOutputId InPassOutputId)
	{
		if((int32)InPassOutputId < OutputCount)
		{
			return &PassOutputs[InPassOutputId];
		}

		return 0;
	}

	/** can be overloaded for more control */
/*	virtual FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const
	{
		FPooledRenderTargetDesc Ret = PassInputs[0].GetOutput()->RenderTargetDesc;

		Ret.Reset();

		return Ret;
	}
*/	
	virtual FRenderingCompositeOutputRef* GetDependency(uint32 Index)
	{
		// first through all inputs
		FRenderingCompositeOutputRef* Ret = GetInput((EPassInputId)Index);

		if(!Ret)
		{
			// then all additional dependencies
			Ret = GetAdditionalDependency(Index - InputCount);
		}

		return Ret;
	}

	virtual FRenderingCompositeOutputRef* GetAdditionalDependency(uint32 Index)
	{
		uint32 AdditionalDependenciesCount = AdditionalDependencies.Num();

		if(Index < AdditionalDependenciesCount)
		{
			return &AdditionalDependencies[Index];
		}

		return 0;
	}

	virtual const TCHAR* GetOutputDumpFilename(EPassOutputId OutputId) const
	{
		check (OutputId < OutputCount);
		return *PassOutputDumpFilenames[OutputId];
	}

	virtual void SetOutputDumpFilename(EPassOutputId OutputId, const TCHAR* Filename)
	{
		check (OutputId < OutputCount);
		PassOutputDumpFilenames[OutputId] = Filename;
	}

	virtual TArray<FColor>* GetOutputColorArray(EPassOutputId OutputId) const
	{
		check (OutputId < OutputCount);
		return PassOutputColorArrays[OutputId];
	}

	virtual void SetOutputColorArray(EPassOutputId OutputId, TArray<FColor>* OutputBuffer)
	{
		check (OutputId < OutputCount);
		PassOutputColorArrays[OutputId] = OutputBuffer;
	}

protected:
	/** hack to allow 0 inputs */
	FRenderingCompositeOutputRef PassInputs[InputCount == 0 ? 1 : InputCount];
	/** */
	FRenderingCompositeOutput PassOutputs[OutputCount];
	/** Filenames that the outputs can be written to after being processed */
	FString PassOutputDumpFilenames[OutputCount];
	/** Color arrays for saving off a copy of the pixel data from this pass output */
	TArray<FColor>* PassOutputColorArrays[OutputCount];
	/** All dependencies: PassInputs and all objects in this container */
	TArray<FRenderingCompositeOutputRef> AdditionalDependencies;
};


// derives from TRenderingCompositePassBase<InputCount, OutputCount>
class FRCPassPostProcessRoot : public TRenderingCompositePassBase<0, 1>
{
public:
	// interface FRenderingCompositePass ---------
	virtual void Process(FRenderingCompositePassContext& Context) {}
	virtual void Release() OVERRIDE { delete this; }
	FPooledRenderTargetDesc ComputeOutputDesc(EPassOutputId InPassOutputId) const { FPooledRenderTargetDesc Desc; Desc.DebugName = TEXT("Root"); return Desc; }
};

// currently hard coded to 4 input textures
// convenient but not the most optimized solution
struct FPostProcessPassParameters 
{
	/** Initialization constructor. */
	void Bind(const FShaderParameterMap& ParameterMap);

	/** Set the pixel shader parameter values. */
	void SetPS(const FPixelShaderRHIParamRef& ShaderRHI, const FRenderingCompositePassContext& Context, FSamplerStateRHIParamRef Filter = TStaticSamplerState<>::GetRHI(), bool bWhiteIfNoTexture = false, FSamplerStateRHIParamRef* FilterOverrideArray = 0);

	/** Set the compute shader parameter values. */
	void SetCS(const FComputeShaderRHIParamRef& ShaderRHI, const FRenderingCompositePassContext& Context, FSamplerStateRHIParamRef Filter = TStaticSamplerState<>::GetRHI(), bool bWhiteIfNoTexture = false, FSamplerStateRHIParamRef* FilterOverrideArray = 0);

	/** Set the vertex shader parameter values. */
	void SetVS(const FVertexShaderRHIParamRef& ShaderRHI, const FRenderingCompositePassContext& Context, FSamplerStateRHIParamRef Filter = TStaticSamplerState<>::GetRHI(), bool bWhiteIfNoTexture = false, FSamplerStateRHIParamRef* FilterOverrideArray = 0);

	/** Serializer. */
	friend FArchive& operator<<(FArchive& Ar,FPostProcessPassParameters& P);

private:

	FShaderParameter ViewportSize;
	FShaderParameter ViewportRect;
	FShaderResourceParameter PostprocessInputParameter[ePId_Input_MAX];
	FShaderResourceParameter PostprocessInputParameterSampler[ePId_Input_MAX];
	FShaderResourceParameter PostprocessInputNew[ePId_Input_MAX];
	FShaderParameter PostprocessInputSizeParameter[ePId_Input_MAX];
	FShaderParameter PostProcessInputMinMaxParameter[ePId_Input_MAX];
	FShaderParameter ScreenPosToPixel;
	FShaderResourceParameter BilinearTextureSampler0;
	FShaderResourceParameter BilinearTextureSampler1;

public:
	// using this template method in many places the executable size might go up
	// @param Filter can be 0 if FilterOverrideArray is used
	// @param FilterOverrideArray can be 0 if Filter is used
	template <class T>
	void Set(const T& ShaderRHI, const FRenderingCompositePassContext& Context, FSamplerStateRHIParamRef Filter, bool bWhiteIfNoTexture = false, FSamplerStateRHIParamRef* FilterOverrideArray = 0)
	{
		// assuming all outputs have the same size
		FRenderingCompositeOutput* Output = Context.Pass->GetOutput(ePId_Output0);

		// Output0 should always exist
		check(Output);

		// one should be on
		check(FilterOverrideArray || Filter);
		// but not both
		check(!FilterOverrideArray || !Filter);

		if(BilinearTextureSampler0.IsBound())
		{
			RHISetShaderSampler(
				ShaderRHI, 
				BilinearTextureSampler0.GetBaseIndex(), 
				TStaticSamplerState<SF_Bilinear>::GetRHI()
				);
		}

		if(BilinearTextureSampler1.IsBound())
		{
			RHISetShaderSampler(
				ShaderRHI, 
				BilinearTextureSampler1.GetBaseIndex(), 
				TStaticSamplerState<SF_Bilinear>::GetRHI()
				);
		}

		if(ViewportSize.IsBound() || ScreenPosToPixel.IsBound() || ViewportRect.IsBound())
		{
			FIntRect LocalViewport = Context.GetViewport();

			FIntPoint ViewportOffset = LocalViewport.Min;
			FIntPoint ViewportExtent = LocalViewport.Size();

			{
				FVector4 Value(ViewportExtent.X, ViewportExtent.Y, 1.0f / ViewportExtent.X, 1.0f / ViewportExtent.Y);

				SetShaderValue(ShaderRHI, ViewportSize, Value);
			}

			{
				FVector4 Value(LocalViewport.Min.X, LocalViewport.Min.Y, LocalViewport.Max.X, LocalViewport.Max.Y);

				SetShaderValue(ShaderRHI, ViewportRect, Value);
			}

			{
				FVector4 ScreenPosToPixelValue(
					ViewportExtent.X * 0.5f,
					-ViewportExtent.Y * 0.5f, 
					ViewportExtent.X * 0.5f - 0.5f + ViewportOffset.X,
					ViewportExtent.Y * 0.5f - 0.5f + ViewportOffset.Y);
				SetShaderValue(ShaderRHI, ScreenPosToPixel, ScreenPosToPixelValue);
			}
		}

		//Calculate a base scene texture min max which will be pulled in by a pixel for each PP input.
		FIntRect ContextViewportRect = Context.IsViewportValid() ? Context.GetViewport() : FIntRect(0,0,0,0);
		const FIntPoint SceneRTSize = GSceneRenderTargets.GetBufferSizeXY();
		FVector4 BaseSceneTexMinMax(	((float)ContextViewportRect.Min.X/SceneRTSize.X), 
										((float)ContextViewportRect.Min.Y/SceneRTSize.Y), 
										((float)ContextViewportRect.Max.X/SceneRTSize.X), 
										((float)ContextViewportRect.Max.Y/SceneRTSize.Y) );

		// ePId_Input0, ePId_Input1, ...
		for(uint32 Id = 0; Id < (uint32)ePId_Input_MAX; ++Id)
		{
			FRenderingCompositeOutputRef* OutputRef = Context.Pass->GetInput((EPassInputId)Id);

			if(!OutputRef)
			{
				// Pass doesn't have more inputs
				break;
			}

			FRenderingCompositeOutput* Input = OutputRef->GetOutput();

			TRefCountPtr<IPooledRenderTarget> InputPooledElement;

			if(Input)
			{
				InputPooledElement = Input->RequestInput();
			}

			FSamplerStateRHIParamRef LocalFilter = FilterOverrideArray ? FilterOverrideArray[Id] : Filter;

			if(InputPooledElement)
			{
				check(!InputPooledElement->IsFree());

				const FTextureRHIRef& SrcTexture = InputPooledElement->GetRenderTargetItem().ShaderResourceTexture;

				SetTextureParameter(ShaderRHI, PostprocessInputParameter[Id], PostprocessInputParameterSampler[Id], LocalFilter, SrcTexture);
				SetTextureParameter(ShaderRHI, PostprocessInputNew[Id], InputPooledElement->GetRenderTargetItem().TargetableTexture);

				{
					float Width = InputPooledElement->GetDesc().Extent.X;
					float Height = InputPooledElement->GetDesc().Extent.Y;
					FVector4 TextureSize(Width, Height, 1.0f / Width, 1.0f / Height);
					SetShaderValue(ShaderRHI, PostprocessInputSizeParameter[Id], TextureSize);
					
					//We could use the main scene min max here if it weren't that we need to pull the max in by a pixel on a per input basis.
					FVector2D OnePPInputPixelUVSize = FVector2D(1.0f / Width, 1.0f / Height);
					FVector4 PPInputMinMax = BaseSceneTexMinMax;
					PPInputMinMax.Z -= OnePPInputPixelUVSize.X;
					PPInputMinMax.W -= OnePPInputPixelUVSize.Y;
					SetShaderValue(ShaderRHI, PostProcessInputMinMaxParameter[Id], PPInputMinMax);
				}
			}
			else
			{
				IPooledRenderTarget* Texture = bWhiteIfNoTexture ? GSystemTextures.WhiteDummy : GSystemTextures.BlackDummy;

				// if the input is not there but the shader request it we give it at least some data to avoid d3ddebug errors and shader permutations
				// to make features optional we use default black for additive passes without shader permutations
				SetTextureParameter(ShaderRHI, PostprocessInputParameter[Id], PostprocessInputParameterSampler[Id], LocalFilter, Texture->GetRenderTargetItem().TargetableTexture);
				SetTextureParameter(ShaderRHI, PostprocessInputNew[Id], Texture->GetRenderTargetItem().TargetableTexture);

				FVector4 Dummy(1, 1, 1, 1);
				SetShaderValue(ShaderRHI, PostprocessInputSizeParameter[Id], Dummy);
				SetShaderValue(ShaderRHI, PostProcessInputMinMaxParameter[Id], Dummy);
			}
		}

		// todo warning if Input[] or InputSize[] is bound but not available, maybe set a specific input texture (blinking?)
	}
};

void CompositionGraph_OnStartFrame();
