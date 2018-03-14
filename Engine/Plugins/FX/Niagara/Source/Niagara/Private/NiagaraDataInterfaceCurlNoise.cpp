// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraDataInterfaceCurlNoise.h"

UNiagaraDataInterfaceCurlNoise::UNiagaraDataInterfaceCurlNoise(FObjectInitializer const& ObjectInitializer)
	: Super(ObjectInitializer)
	, bGPUBufferDirty(true)
	, Seed(0)
{

}

void UNiagaraDataInterfaceCurlNoise::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		FNiagaraTypeRegistry::Register(FNiagaraTypeDefinition(GetClass()), true, false, false);
	}
}

void UNiagaraDataInterfaceCurlNoise::PostLoad()
{
	Super::PostLoad();
	InitNoiseLUT();
}

#if WITH_EDITOR

void UNiagaraDataInterfaceCurlNoise::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UNiagaraDataInterfaceCurlNoise, Seed))
	{
		InitNoiseLUT();
	}
}

#endif

bool UNiagaraDataInterfaceCurlNoise::CopyToInternal(UNiagaraDataInterface* Destination) const
{
	if (!Super::CopyToInternal(Destination))
	{
		return false;
	}
	UNiagaraDataInterfaceCurlNoise* DestinationCurlNoise = CastChecked<UNiagaraDataInterfaceCurlNoise>(Destination);
	DestinationCurlNoise->Seed = Seed;
	DestinationCurlNoise->InitNoiseLUT();

	return true;
}

bool UNiagaraDataInterfaceCurlNoise::Equals(const UNiagaraDataInterface* Other) const
{
	if (!Super::Equals(Other))
	{
		return false;
	}
	const UNiagaraDataInterfaceCurlNoise* OtherCurlNoise = CastChecked<const UNiagaraDataInterfaceCurlNoise>(Other);
	return OtherCurlNoise->Seed == Seed;
}

void UNiagaraDataInterfaceCurlNoise::GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions)
{
	FNiagaraFunctionSignature Sig;
	Sig.Name = TEXT("SampleNoiseField");
	Sig.bMemberFunction = true;
	Sig.bRequiresContext = false;
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("NoiseField")));
	Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("XYZ")));
	Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Value")));
	//Sig.Owner = *GetName();

	OutFunctions.Add(Sig);
}

DEFINE_NDI_FUNC_BINDER(UNiagaraDataInterfaceCurlNoise, SampleNoiseField);
FVMExternalFunction UNiagaraDataInterfaceCurlNoise::GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo, void* InstanceData)
{
	check(BindingInfo.Name == TEXT("SampleNoiseField"));
	check(BindingInfo.GetNumInputs() == 3 && BindingInfo.GetNumOutputs() == 3);
	return TNDIParamBinder<0, float, TNDIParamBinder<1, float, TNDIParamBinder<2, float, NDI_FUNC_BINDER(UNiagaraDataInterfaceCurlNoise, SampleNoiseField)>>>::Bind(this, BindingInfo, InstanceData);
}

template<typename XType, typename YType, typename ZType>
void UNiagaraDataInterfaceCurlNoise::SampleNoiseField(FVectorVMContext& Context)
{
	XType XParam(Context);
	YType YParam(Context);
	ZType ZParam(Context);
	FRegisterHandler<float> OutSampleX(Context);
	FRegisterHandler<float> OutSampleY(Context);
	FRegisterHandler<float> OutSampleZ(Context);
	
	for (int32 InstanceIdx = 0; InstanceIdx < Context.NumInstances; ++InstanceIdx)
	{
		VectorRegister InCoords = MakeVectorRegister(XParam.Get(), YParam.Get(), ZParam.Get(), 0.0f);

		const VectorRegister One = MakeVectorRegister(1.0f, 1.0f, 1.0f, 1.0f);
		const VectorRegister Zero = MakeVectorRegister(0.0f, 0.0f, 0.0f, 0.0f);
		// We use 15,15,15 here because the value will be between 0 and 1. 1 should indicate the maximal index of the 
		// real table and we pad that out to handle border work. So our values for the Cxyz below should not
		// ever be 17 in any dimension as that would overflow the array.
		const VectorRegister VecSize = MakeVectorRegister(15.0f, 15.0f, 15.0f, 15.0f);

		VectorRegister Dst = MakeVectorRegister(0.0f, 0.0f, 0.0f, 0.0f);

		float Di = 0.2f; // Hard-coded scale TODO remove!
		VectorRegister Div = MakeVectorRegister(Di, Di, Di, Di);
		VectorRegister Coords = VectorMod(VectorAbs(VectorMultiply(InCoords, Div)), VecSize);
		Coords = VectorMin(Coords, VecSize);
		Coords = VectorMax(Coords, Zero);
		const float *CoordPtr = reinterpret_cast<float const*>(&Coords);
		const int32 Cx = CoordPtr[0];
		const int32 Cy = CoordPtr[1];
		const int32 Cz = CoordPtr[2];

		VectorRegister Frac = VectorFractional(Coords);
		VectorRegister Alpha = VectorReplicate(Frac, 0);
		VectorRegister OneMinusAlpha = VectorSubtract(One, Alpha);

		// Trilinear interpolation, as defined by https://en.wikipedia.org/wiki/Trilinear_interpolation
		ensure(Cx + 1 < 17);
		ensure(Cy + 1 < 17);
		ensure(Cz + 1 < 17);
		VectorRegister C00 = VectorMultiplyAdd(NoiseTable[Cx][Cy][Cz], OneMinusAlpha, VectorMultiply(NoiseTable[Cx + 1][Cy][Cz], Alpha)); // (x0, y0, z0)(1-a) + (x1, y0, z0)a
		VectorRegister C01 = VectorMultiplyAdd(NoiseTable[Cx][Cy][Cz + 1], OneMinusAlpha, VectorMultiply(NoiseTable[Cx + 1][Cy][Cz + 1], Alpha)); // (x0, y0, z1)(1-a) + (x1, y0, z1)a
		VectorRegister C10 = VectorMultiplyAdd(NoiseTable[Cx][Cy + 1][Cz], OneMinusAlpha, VectorMultiply(NoiseTable[Cx + 1][Cy + 1][Cz], Alpha)); // (x0, y1, z0)(1-a) + (x1, y1, z0)a
		VectorRegister C11 = VectorMultiplyAdd(NoiseTable[Cx][Cy + 1][Cz + 1], OneMinusAlpha, VectorMultiply(NoiseTable[Cx + 1][Cy + 1][Cz + 1], Alpha)); // (x0, y1, z1)(1-a) + (x1, y1, z1)a

		Alpha = VectorReplicate(Frac, 1);
		OneMinusAlpha = VectorSubtract(One, Alpha);
		VectorRegister C0 = VectorMultiplyAdd(C00, OneMinusAlpha, VectorMultiply(C10, Alpha));
		VectorRegister C1 = VectorMultiplyAdd(C01, OneMinusAlpha, VectorMultiply(C11, Alpha));

		Alpha = VectorReplicate(Frac, 2);
		OneMinusAlpha = VectorSubtract(One, Alpha);
		VectorRegister ZV = VectorMultiplyAdd(C0, OneMinusAlpha, VectorMultiply(C1, Alpha));

		Dst = VectorAdd(Dst, ZV);

		float *RegPtr = reinterpret_cast<float*>(&Dst);
		*OutSampleX.GetDest() = RegPtr[0];
		*OutSampleY.GetDest() = RegPtr[1];
		*OutSampleZ.GetDest() = RegPtr[2];

		XParam.Advance();
		YParam.Advance();
		ZParam.Advance();
		OutSampleX.Advance();
		OutSampleY.Advance();
		OutSampleZ.Advance();
	}
}



// build the shader function HLSL; function name is passed in, as it's defined per-DI; that way, configuration could change
// the HLSL in the spirit of a static switch
// TODO: need a way to identify each specific function here
// 
bool UNiagaraDataInterfaceCurlNoise::GetFunctionHLSL(const FName& DefinitionFunctionName, FString InstanceFunctionName, TArray<FDIGPUBufferParamDescriptor> &Descriptors, FString &HLSLInterfaceID, FString &OutHLSL)
{

	FString BufferName = Descriptors[0].BufferParamName;
	OutHLSL += TEXT("void ") + InstanceFunctionName + TEXT("(in float3 In_XYZ, out float3 Out_Value) \n{\n");
	OutHLSL += TEXT("\t float3 a = trunc((In_XYZ*0.2) / 15.0);\n");
	OutHLSL += TEXT("\t float3 ModXYZ = (In_XYZ*0.2) - a*15.0;\n");
	OutHLSL += TEXT("\t int3 IntCoord = int3(ModXYZ.x, ModXYZ.y, ModXYZ.z);\n");
	OutHLSL += TEXT("\t float3 frc = frac(ModXYZ);\n");
	// Trilinear interpolation, as defined by https://en.wikipedia.org/wiki/Trilinear_interpolation
	// HLSL lerp is defined as lerp(x, y, s) = x*(1-s) + y*s
	OutHLSL += TEXT("\t float3 V1 = ") + BufferName + TEXT("[IntCoord.x + IntCoord.y*17 + IntCoord.z*17*17].xyz;\n"); // x0,y0,z0
	OutHLSL += TEXT("\t float3 V2 = ") + BufferName + TEXT("[IntCoord.x+1 + IntCoord.y*17 + IntCoord.z*17*17].xyz;\n"); //  x1,y0,z0
	OutHLSL += TEXT("\t float3 C00 = lerp(V1, V2, frc.xxx);\n");
	OutHLSL += TEXT("\t V1 = ") + BufferName + TEXT("[IntCoord.x + IntCoord.y*17 + (IntCoord.z+1)*17*17].xyz;\n"); // x0, y0, z1
	OutHLSL += TEXT("\t V2 = ") + BufferName + TEXT("[IntCoord.x+1 + IntCoord.y*17 + (IntCoord.z+1)*17*17].xyz;\n"); //x1, y0, z1
	OutHLSL += TEXT("\t float3 C01 = lerp(V1, V2, frc.xxx);\n");
	OutHLSL += TEXT("\t V1 = ") + BufferName + TEXT("[IntCoord.x + (IntCoord.y+1)*17 + IntCoord.z*17*17].xyz;\n"); //x0, y1, z0
	OutHLSL += TEXT("\t V2 = ") + BufferName + TEXT("[IntCoord.x+1 + (IntCoord.y+1)*17 + IntCoord.z*17*17].xyz;\n"); // x1, y1, z0
	OutHLSL += TEXT("\t float3 C10 = lerp(V1, V2, frc.xxx);\n");
	OutHLSL += TEXT("\t V1 = ") + BufferName + TEXT("[IntCoord.x + (IntCoord.y+1)*17 + (IntCoord.z+1)*17*17].xyz;\n"); // x0, y1, z1
	OutHLSL += TEXT("\t V2 = ") + BufferName + TEXT("[IntCoord.x+1 + (IntCoord.y+1)*17 + (IntCoord.z+1)*17*17].xyz;\n"); //x1, y1, z1
	OutHLSL += TEXT("\t float3 C11 = lerp(V1, V2, frc.xxx);\n");
	OutHLSL += TEXT("\t float3 C0 = lerp(C00, C10, frc.yyy);\n");
	OutHLSL += TEXT("\t float3 C1 = lerp(C01, C11, frc.yyy);\n");
	OutHLSL += TEXT("\t Out_Value = lerp(C0, C1, frc.zzz);\n");
	OutHLSL += TEXT("\n}\n");
	return true;
}

// build buffer definition hlsl
// 1. Choose a buffer name, add the data interface ID (important!)
// 2. add a DIGPUBufferParamDescriptor to the array argument; that'll be passed on to the FNiagaraShader for binding to a shader param, that can
// then later be found by name via FindDIBufferParam for setting; 
// 3. store buffer declaration hlsl in OutHLSL
// multiple buffers can be defined at once here
//
void UNiagaraDataInterfaceCurlNoise::GetBufferDefinitionHLSL(FString DataInterfaceID, TArray<FDIGPUBufferParamDescriptor> &BufferDescriptors, FString &OutHLSL)
{
	FString BufferName = "CurlNoiseLUT" + DataInterfaceID;
	OutHLSL += TEXT("Buffer<float4> ") + BufferName + TEXT(";\n");

	BufferDescriptors.Add(FDIGPUBufferParamDescriptor(BufferName, 0));		// add a descriptor for shader parameter binding

}

// called after translate, to setup buffers matching the buffer descriptors generated during hlsl translation
// need to do this because the script used during translate is a clone, including its DIs
//
void UNiagaraDataInterfaceCurlNoise::SetupBuffers(FDIBufferDescriptorStore &BufferDescriptors)
{
	GPUBuffers.Empty();
	for (FDIGPUBufferParamDescriptor &Desc : BufferDescriptors.Descriptors)
	{
		FNiagaraDataInterfaceBufferData BufferData(*Desc.BufferParamName);	// store off the data for later use
		GPUBuffers.Add(BufferData);
	}
	bGPUBufferDirty = true;
}

// return the GPU buffer array (called from NiagaraInstanceBatcher to get the buffers for setting to the shader)
// we lazily update the buffer with a new LUT here if necessary
//
TArray<FNiagaraDataInterfaceBufferData> &UNiagaraDataInterfaceCurlNoise::GetBufferDataArray()
{
	check(IsInRenderingThread());

	if (bGPUBufferDirty)
	{
		check(GPUBuffers.Num() > 0);
		FNiagaraDataInterfaceBufferData &GPUBuffer = GPUBuffers[0];
		GPUBuffer.Buffer.Release();
		uint32 BufferSize = 17 * 17 * 17 * sizeof(float) * 4;
		//int32 *BufferData = static_cast<int32*>(RHILockVertexBuffer(GPUBuffer.Buffer.Buffer, 0, BufferSize, EResourceLockMode::RLM_WriteOnly));
		TResourceArray<FVector4> TempTable;
		TempTable.AddUninitialized(17 * 17 * 17);
		for (int z = 0; z < 17; z++)
		{
			for (int y = 0; y < 17; y++)
			{
				for (int x = 0; x < 17; x++)
				{
					float *R = (float*)( &(NoiseTable[x][y][z]) );
					TempTable[x + y * 17 + z * 17 * 17] = FVector4(R[0], R[1], R[2], 0.0f);
				}
			}
		}
		GPUBuffer.Buffer.Initialize(sizeof(float) * 4, 17 * 17 * 17, EPixelFormat::PF_A32B32G32R32F, BUF_Static, TEXT("CurlnoiseTable"), &TempTable);
		//FPlatformMemory::Memcpy(BufferData, TempTable, BufferSize);
		//RHIUnlockVertexBuffer(GPUBuffer.Buffer.Buffer);
		bGPUBufferDirty = false;
	}


	return GPUBuffers;
}


// replicate a border for filtering
template<typename T>
void UNiagaraDataInterfaceCurlNoise::ReplicateBorder(T* DestBuffer)
{
	uint32 Extent = 17;
	uint32 Square = Extent*Extent;
	uint32 Last = Extent - 1;

	for (uint32 z = 0; z < Extent; ++z)
	{
		for (uint32 y = 0; y < Extent; ++y)
		{
			DestBuffer[Last + y * Extent + z * Square] = DestBuffer[0 + y * Extent + z * Square];
		}
	}
	for (uint32 z = 0; z < Extent; ++z)
	{
		for (uint32 x = 0; x < Extent; ++x)
		{
			DestBuffer[x + Last * Extent + z * Square] = DestBuffer[x + 0 * Extent + z * Square];
		}
	}
	for (uint32 y = 0; y < Extent; ++y)
	{
		for (uint32 x = 0; x < Extent; ++x)
		{
			DestBuffer[x + y * Extent + Last * Square] = DestBuffer[x + y * Extent + 0 * Square];
		}
	}
}

void UNiagaraDataInterfaceCurlNoise::InitNoiseLUT()
{
	// seed random stream
	FRandomStream RandStream(Seed);

	// random noise
	float TempTable[17][17][17];
	for (int z = 0; z < 17; z++)
	{
		for (int y = 0; y < 17; y++)
		{
			for (int x = 0; x < 17; x++)
			{
				float f1 = RandStream.FRandRange(-1.0f, 1.0f);
				TempTable[x][y][z] = f1;
			}
		}
	}

	// pad
	/*
	for (int i = 0; i < 17; i++)
	{
		for (int j = 0; j < 17; j++)
		{
			TempTable[i][j][16] = TempTable[i][j][0];
			TempTable[i][16][j] = TempTable[i][0][j];
			TempTable[16][j][i] = TempTable[0][j][i];
		}
	}
	*/
	ReplicateBorder(&TempTable[0][0][0]);

	// compute gradients
	FVector TempTable2[17][17][17];
	for (int z = 0; z < 16; z++)
	{
		for (int y = 0; y < 16; y++)
		{
			for (int x = 0; x < 16; x++)
			{
				FVector XGrad = FVector(1.0f, 0.0f, TempTable[x][y][z] - TempTable[x + 1][y][z]);
				FVector YGrad = FVector(0.0f, 1.0f, TempTable[x][y][z] - TempTable[x][y + 1][z]);
				FVector ZGrad = FVector(0.0f, 1.0f, TempTable[x][y][z] - TempTable[x][y][z + 1]);

				FVector Grad = FVector(XGrad.Z, YGrad.Z, ZGrad.Z);
				TempTable2[x][y][z] = Grad;
			}
		}
	}

	/*
	// pad
	for (int i = 0; i < 17; i++)
	{
		for (int j = 0; j < 17; j++)
		{
			TempTable2[i][j][16] = TempTable2[i][j][0];
			TempTable2[i][16][j] = TempTable2[i][0][j];
			TempTable2[16][j][i] = TempTable2[0][j][i];
		}
	}
	*/
	ReplicateBorder(&TempTable2[0][0][0]);

	// http://prideout.net/blog/?p=63           ???
	// compute curl of gradient field
	for (int z = 0; z < 16; z++)
	{
		for (int y = 0; y < 16; y++)
		{
			for (int x = 0; x < 16; x++)
			{
				FVector Dy = TempTable2[x][y][z] - TempTable2[x][y + 1][z];
				FVector Sy = TempTable2[x][y][z] + TempTable2[x][y + 1][z];
				FVector Dx = TempTable2[x][y][z] - TempTable2[x + 1][y][z];
				FVector Sx = TempTable2[x][y][z] + TempTable2[x + 1][y][z];
				FVector Dz = TempTable2[x][y][z] - TempTable2[x][y][z + 1];
				FVector Sz = TempTable2[x][y][z] + TempTable2[x][y][z + 1];
				FVector Dir = FVector(Dy.Z - Sz.Y, Dz.X - Sx.Z, Dx.Y - Sy.X);

				NoiseTable[x][y][z] = MakeVectorRegister(Dir.X, Dir.Y, Dir.Z, 0.0f);
			}
		}
	}

	// pad
	/*
	for (int i = 0; i < 17; i++)
	{
		for (int j = 0; j < 17; j++)
		{
			NoiseTable[i][j][16] = NoiseTable[i][j][0];
			NoiseTable[i][16][j] = NoiseTable[i][0][j];
			NoiseTable[16][j][i] = NoiseTable[0][j][i];
		}
	}*/
	ReplicateBorder(&NoiseTable[0][0][0]);

	bGPUBufferDirty = true;
}
