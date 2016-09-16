// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	D3D12RootSignature.cpp: D3D12 Root Signatures
=============================================================================*/

#include "D3D12RHIPrivate.h"

FORCEINLINE D3D12_SHADER_VISIBILITY GetD3D12ShaderVisibility(EShaderVisibility Visibility)
{
	switch (Visibility)
	{
	case SV_Vertex:
		return D3D12_SHADER_VISIBILITY_VERTEX;
	case SV_Hull:
		return D3D12_SHADER_VISIBILITY_HULL;
	case SV_Domain:
		return D3D12_SHADER_VISIBILITY_DOMAIN;
	case SV_Geometry:
return D3D12_SHADER_VISIBILITY_GEOMETRY;
	case SV_Pixel:
		return D3D12_SHADER_VISIBILITY_PIXEL;
	case SV_All:
		return D3D12_SHADER_VISIBILITY_ALL;

	default:
		check(false);
		return static_cast<D3D12_SHADER_VISIBILITY>(-1);
	};
}

FORCEINLINE D3D12_ROOT_SIGNATURE_FLAGS GetD3D12RootSignatureDenyFlag(EShaderVisibility Visibility)
{
	switch (Visibility)
	{
	case SV_Vertex:
		return D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
	case SV_Hull:
		return D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
	case SV_Domain:
		return D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
	case SV_Geometry:
		return D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
	case SV_Pixel:
		return D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
	case SV_All:
		return D3D12_ROOT_SIGNATURE_FLAG_NONE;

	default:
		check(false);
		return static_cast<D3D12_ROOT_SIGNATURE_FLAGS>(-1);
	};
}


FD3D12RootSignatureDesc::FD3D12RootSignatureDesc(const FD3D12QuantizedBoundShaderState& QBSS)
	: RootParametersSize(0)
{
	const EShaderVisibility ShaderVisibilityPriorityOrder[] = { SV_Pixel, SV_Vertex, SV_Geometry, SV_Hull, SV_Domain, SV_All };
	const D3D12_ROOT_PARAMETER_TYPE RootParameterTypePriorityOrder[] = { D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, D3D12_ROOT_PARAMETER_TYPE_CBV };
	uint32 RootParameterCount = 0;

	// For each root parameter type...
	for (uint32 RootParameterTypeIndex = 0; RootParameterTypeIndex < _countof(RootParameterTypePriorityOrder); RootParameterTypeIndex++)
	{
		const D3D12_ROOT_PARAMETER_TYPE& RootParameterType = RootParameterTypePriorityOrder[RootParameterTypeIndex];

		// ... and each shader stage visibility ...
		for (uint32 ShaderVisibilityIndex = 0; ShaderVisibilityIndex < _countof(ShaderVisibilityPriorityOrder); ShaderVisibilityIndex++)
		{
			const EShaderVisibility& Visibility = ShaderVisibilityPriorityOrder[ShaderVisibilityIndex];
			const FShaderRegisterCounts& Shader = QBSS.RegisterCounts[Visibility];

			switch (RootParameterType)
			{
			case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
			{
				static const uint32 DescriptorTableCost = 1; // Descriptor tables cost 1 DWORD
				if (Shader.ShaderResourceCount > 0)
				{
					check(RootParameterCount < MaxRootParameters);
					DescriptorRanges[RootParameterCount].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, Shader.ShaderResourceCount, 0u);
					TableSlots[RootParameterCount].InitAsDescriptorTable(1, &DescriptorRanges[RootParameterCount], GetD3D12ShaderVisibility(Visibility));
					RootParameterCount++;
					RootParametersSize += DescriptorTableCost;
				}

				if (Shader.ConstantBufferCount > MAX_ROOT_CBVS)
				{
					// Use a descriptor table for the 'excess' CBVs
					check(RootParameterCount < MaxRootParameters);
					DescriptorRanges[RootParameterCount].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, Shader.ConstantBufferCount - MAX_ROOT_CBVS, MAX_ROOT_CBVS);
					TableSlots[RootParameterCount].InitAsDescriptorTable(1, &DescriptorRanges[RootParameterCount], GetD3D12ShaderVisibility(Visibility));
					RootParameterCount++;
					RootParametersSize += DescriptorTableCost;
				}

				if (Shader.SamplerCount > 0)
				{
					check(RootParameterCount < MaxRootParameters);
					DescriptorRanges[RootParameterCount].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, Shader.SamplerCount, 0u);
					TableSlots[RootParameterCount].InitAsDescriptorTable(1, &DescriptorRanges[RootParameterCount], GetD3D12ShaderVisibility(Visibility));
					RootParameterCount++;
					RootParametersSize += DescriptorTableCost;
				}

				if (Shader.UnorderedAccessCount > 0)
				{
					check(RootParameterCount < MaxRootParameters);
					DescriptorRanges[RootParameterCount].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, Shader.UnorderedAccessCount, 0u);
					TableSlots[RootParameterCount].InitAsDescriptorTable(1, &DescriptorRanges[RootParameterCount], GetD3D12ShaderVisibility(Visibility));
					RootParameterCount++;
					RootParametersSize += DescriptorTableCost;
				}
				break;
			}

			case D3D12_ROOT_PARAMETER_TYPE_CBV:
			{
				static const uint32 RootCBVCost = 2; // Root CBVs cost 2 DWORDs
				for (uint32 ShaderRegister = 0; (ShaderRegister < Shader.ConstantBufferCount) && (ShaderRegister < MAX_ROOT_CBVS); ShaderRegister++)
				{
					check(RootParameterCount < MaxRootParameters);
					TableSlots[RootParameterCount].InitAsConstantBufferView(ShaderRegister, 0u, GetD3D12ShaderVisibility(Visibility));
					RootParameterCount++;
					RootParametersSize += RootCBVCost;
				}
				break;
			}

			default:
				check(false);
				break;
			}
		}
	}

	// Determine what shader stages need access in the root signature.
	D3D12_ROOT_SIGNATURE_FLAGS Flags = QBSS.bAllowIAInputLayout ? D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT : D3D12_ROOT_SIGNATURE_FLAG_NONE;
	for (uint32 ShaderVisibilityIndex = 0; ShaderVisibilityIndex < _countof(ShaderVisibilityPriorityOrder); ShaderVisibilityIndex++)
	{
		const EShaderVisibility& Visibility = ShaderVisibilityPriorityOrder[ShaderVisibilityIndex];
		const FShaderRegisterCounts& Shader = QBSS.RegisterCounts[Visibility];
		if ((Shader.ShaderResourceCount == 0) &&
			(Shader.ConstantBufferCount == 0) &&
			(Shader.UnorderedAccessCount == 0) &&
			(Shader.SamplerCount == 0))
		{
			// This shader stage doesn't use any descriptors, deny access to the shader stage in the root signature.
			Flags = (Flags | GetD3D12RootSignatureDenyFlag(Visibility));
		}
	}

	// Init the desc (warn about the size if necessary).
	const uint32 SizeWarningThreshold = 12;
	if (RootParametersSize > SizeWarningThreshold)
	{
		UE_LOG(LogD3D12RHI, Warning, TEXT("Root signature created where the root parameters take up %u DWORDS. Using more than %u DWORDs can negatively impact performance depending on the hardware and root parameter usage."), RootParametersSize, SizeWarningThreshold);
	}
	RootDesc.Init(RootParameterCount, TableSlots, 0, nullptr, Flags);
}

D3D12_ROOT_SIGNATURE_DESC& FD3D12RootSignatureDesc::GetStaticGraphicsRootSignatureDesc()
{
	static const uint32 DescriptorTableCount = 16;
	static struct
	{
		D3D12_SHADER_VISIBILITY Vis;
		D3D12_DESCRIPTOR_RANGE_TYPE Type;
		uint32 Count;
		uint32 BaseShaderReg;
	} RangeDesc[DescriptorTableCount] =
	{
		{ D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, MAX_SRVS, 0 },
		{ D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, MAX_CBS, 0 },
		{ D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, D3D12_COMMONSHADER_SAMPLER_SLOT_COUNT, 0 },

		{ D3D12_SHADER_VISIBILITY_VERTEX, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, MAX_SRVS, 0 },
		{ D3D12_SHADER_VISIBILITY_VERTEX, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, MAX_CBS, 0 },
		{ D3D12_SHADER_VISIBILITY_VERTEX, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, D3D12_COMMONSHADER_SAMPLER_SLOT_COUNT, 0 },

		{ D3D12_SHADER_VISIBILITY_GEOMETRY, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, MAX_SRVS, 0 },
		{ D3D12_SHADER_VISIBILITY_GEOMETRY, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, MAX_CBS, 0 },
		{ D3D12_SHADER_VISIBILITY_GEOMETRY, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, D3D12_COMMONSHADER_SAMPLER_SLOT_COUNT, 0 },

		{ D3D12_SHADER_VISIBILITY_HULL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, MAX_SRVS, 0 },
		{ D3D12_SHADER_VISIBILITY_HULL, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, MAX_CBS, 0 },
		{ D3D12_SHADER_VISIBILITY_HULL, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, D3D12_COMMONSHADER_SAMPLER_SLOT_COUNT, 0 },

		{ D3D12_SHADER_VISIBILITY_DOMAIN, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, MAX_SRVS, 0 },
		{ D3D12_SHADER_VISIBILITY_DOMAIN, D3D12_DESCRIPTOR_RANGE_TYPE_CBV, MAX_CBS, 0 },
		{ D3D12_SHADER_VISIBILITY_DOMAIN, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, D3D12_COMMONSHADER_SAMPLER_SLOT_COUNT, 0 },

		{ D3D12_SHADER_VISIBILITY_ALL, D3D12_DESCRIPTOR_RANGE_TYPE_UAV, D3D12_PS_CS_UAV_REGISTER_COUNT, 0 },
	};

	static CD3DX12_ROOT_PARAMETER TableSlots[DescriptorTableCount];
	static CD3DX12_DESCRIPTOR_RANGE DescriptorRanges[DescriptorTableCount];

	for (uint32 i = 0; i < DescriptorTableCount; i++)
	{
		DescriptorRanges[i].Init(
			RangeDesc[i].Type,
			RangeDesc[i].Count,
			RangeDesc[i].BaseShaderReg
			);

		TableSlots[i].InitAsDescriptorTable(1, &DescriptorRanges[i], RangeDesc[i].Vis);
	}

	static CD3DX12_ROOT_SIGNATURE_DESC RootDesc;
	RootDesc.Init(DescriptorTableCount, TableSlots, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	return RootDesc;
}

D3D12_ROOT_SIGNATURE_DESC& FD3D12RootSignatureDesc::GetStaticComputeRootSignatureDesc()
{
	static const uint32 DescriptorTableCount = 4;
	static CD3DX12_ROOT_PARAMETER TableSlots[DescriptorTableCount];
	static CD3DX12_DESCRIPTOR_RANGE DescriptorRanges[DescriptorTableCount];

	uint32 RangeIndex = 0;
	DescriptorRanges[RangeIndex].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, MAX_SRVS, 0);
	TableSlots[RangeIndex].InitAsDescriptorTable(1, &DescriptorRanges[RangeIndex], D3D12_SHADER_VISIBILITY_ALL);
	++RangeIndex;
	DescriptorRanges[RangeIndex].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, MAX_CBS, 0);
	TableSlots[RangeIndex].InitAsDescriptorTable(1, &DescriptorRanges[RangeIndex], D3D12_SHADER_VISIBILITY_ALL);
	++RangeIndex;
	DescriptorRanges[RangeIndex].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, D3D12_COMMONSHADER_SAMPLER_SLOT_COUNT, 0);
	TableSlots[RangeIndex].InitAsDescriptorTable(1, &DescriptorRanges[RangeIndex], D3D12_SHADER_VISIBILITY_ALL);
	++RangeIndex;
	DescriptorRanges[RangeIndex].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, D3D12_PS_CS_UAV_REGISTER_COUNT, 0);
	TableSlots[RangeIndex].InitAsDescriptorTable(1, &DescriptorRanges[RangeIndex], D3D12_SHADER_VISIBILITY_ALL);
	++RangeIndex;

	static CD3DX12_ROOT_SIGNATURE_DESC RootDesc;
	RootDesc.Init(RangeIndex, TableSlots, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

	return RootDesc;
}

void FD3D12RootSignature::Init(const FD3D12QuantizedBoundShaderState& InQBSS)
{
	// Create a root signature desc from the quantized bound shader state.
	FD3D12RootSignatureDesc Desc(InQBSS);
	Init(Desc.GetDesc());
}

void FD3D12RootSignature::Init(const D3D12_ROOT_SIGNATURE_DESC& InDesc)
{
	ID3D12Device* Device = GetParentAdapter()->GetD3DDevice();

	// Serialize the desc.
	TRefCountPtr<ID3DBlob> Error;
	const HRESULT SerializeHR = D3D12SerializeRootSignature(&InDesc, D3D_ROOT_SIGNATURE_VERSION_1, RootSignatureBlob.GetInitReference(), Error.GetInitReference());
	if (Error.GetReference())
	{
		UE_LOG(LogD3D12RHI, Fatal, TEXT("D3D12SerializeRootSignature failed with error %s"), ANSI_TO_TCHAR(Error->GetBufferPointer()));
	}
	VERIFYD3D12RESULT(SerializeHR);

	// Create and analyze the root signature.
	VERIFYD3D12RESULT(Device->CreateRootSignature(GetParentAdapter()->ActiveGPUMask(),
		RootSignatureBlob->GetBufferPointer(),
		RootSignatureBlob->GetBufferSize(),
		IID_PPV_ARGS(RootSignature.GetInitReference())));

	AnalyzeSignature(InDesc);
}

void FD3D12RootSignature::Init(ID3DBlob* const InBlob)
{
	ID3D12Device* Device = GetParentAdapter()->GetD3DDevice();

	// Save the blob
	RootSignatureBlob = InBlob;

	// Deserialize to get the desc.
	TRefCountPtr<ID3D12RootSignatureDeserializer> Deserializer;
	VERIFYD3D12RESULT(D3D12CreateRootSignatureDeserializer(RootSignatureBlob->GetBufferPointer(), RootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(Deserializer.GetInitReference())));

	// Create and analyze the root signature.
	VERIFYD3D12RESULT(Device->CreateRootSignature(GetParentAdapter()->ActiveGPUMask(),
		RootSignatureBlob->GetBufferPointer(),
		RootSignatureBlob->GetBufferSize(),
		IID_PPV_ARGS(RootSignature.GetInitReference())));

	AnalyzeSignature(*Deserializer->GetRootSignatureDesc());
}

void FD3D12RootSignature::AnalyzeSignature(const D3D12_ROOT_SIGNATURE_DESC& Desc)
{
	// Reset members to default values.
	{
		FMemory::Memset(BindSlotMap, 0xFF, sizeof(BindSlotMap));
		bHasUAVs = false;
		bHasSRVs = false;
		bHasCBVs = false;
		bHasRDTCBVs = false;
		bHasRDCBVs = false;
		bHasSamplers = false;
	}

	const bool bDenyVS = (Desc.Flags & D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS) != 0;
	const bool bDenyHS = (Desc.Flags & D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS) != 0;
	const bool bDenyDS = (Desc.Flags & D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS) != 0;
	const bool bDenyGS = (Desc.Flags & D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS) != 0;
	const bool bDenyPS = (Desc.Flags & D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS) != 0;

	// Go through each root parameter.
	for (uint32 i = 0; i < Desc.NumParameters; i++)
	{
		const D3D12_ROOT_PARAMETER& CurrentParameter = Desc.pParameters[i];

		EShaderFrequency CurrentVisibleSF = SF_NumFrequencies;
		switch (CurrentParameter.ShaderVisibility)
		{
		case D3D12_SHADER_VISIBILITY_ALL:
			CurrentVisibleSF = SF_NumFrequencies;
			break;

		case D3D12_SHADER_VISIBILITY_VERTEX:
			CurrentVisibleSF = SF_Vertex;
			break;
		case D3D12_SHADER_VISIBILITY_HULL:
			CurrentVisibleSF = SF_Hull;
			break;
		case D3D12_SHADER_VISIBILITY_DOMAIN:
			CurrentVisibleSF = SF_Domain;
			break;
		case D3D12_SHADER_VISIBILITY_GEOMETRY:
			CurrentVisibleSF = SF_Geometry;
			break;
		case D3D12_SHADER_VISIBILITY_PIXEL:
			CurrentVisibleSF = SF_Pixel;
			break;

		default:
			check(false);
			break;
		}

		// Determine shader stage visibility.
		{
			Stage[SF_Vertex].bVisible = Stage[SF_Vertex].bVisible || (!bDenyVS && HasVisibility(CurrentParameter.ShaderVisibility, D3D12_SHADER_VISIBILITY_VERTEX));
			Stage[SF_Hull].bVisible = Stage[SF_Hull].bVisible || (!bDenyHS && HasVisibility(CurrentParameter.ShaderVisibility, D3D12_SHADER_VISIBILITY_HULL));
			Stage[SF_Domain].bVisible = Stage[SF_Domain].bVisible || (!bDenyDS && HasVisibility(CurrentParameter.ShaderVisibility, D3D12_SHADER_VISIBILITY_DOMAIN));
			Stage[SF_Geometry].bVisible = Stage[SF_Geometry].bVisible || (!bDenyGS && HasVisibility(CurrentParameter.ShaderVisibility, D3D12_SHADER_VISIBILITY_GEOMETRY));
			Stage[SF_Pixel].bVisible = Stage[SF_Pixel].bVisible || (!bDenyPS && HasVisibility(CurrentParameter.ShaderVisibility, D3D12_SHADER_VISIBILITY_PIXEL));

			// Compute is a special case, it must have visibility all.
			Stage[SF_Compute].bVisible = Stage[SF_Compute].bVisible || (CurrentParameter.ShaderVisibility == D3D12_SHADER_VISIBILITY_ALL);
		}

		// Determine shader resource counts.
		{
			switch (CurrentParameter.ParameterType)
			{
			case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
				check(CurrentParameter.DescriptorTable.NumDescriptorRanges == 1);	// Code currently assumes a single descriptor range.
				{
					const D3D12_DESCRIPTOR_RANGE& CurrentRange = CurrentParameter.DescriptorTable.pDescriptorRanges[0];
					check(CurrentRange.BaseShaderRegister == 0);	// Code currently assumes always starting at register 0.
					switch (CurrentRange.RangeType)
					{
					case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
						SetMaxSRVCount(CurrentVisibleSF, CurrentRange.NumDescriptors);
						SetSRVRDTBindSlot(CurrentVisibleSF, i);
						break;
					case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
						SetMaxUAVCount(CurrentVisibleSF, CurrentRange.NumDescriptors);
						SetUAVRDTBindSlot(CurrentVisibleSF, i);
						break;
					case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
						IncrementMaxCBVCount(CurrentVisibleSF, CurrentRange.NumDescriptors);
						SetCBVRDTBindSlot(CurrentVisibleSF, i);
						UpdateCBVRegisterMask(CurrentVisibleSF, CurrentRange);
						break;
					case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
						SetMaxSamplerCount(CurrentVisibleSF, CurrentRange.NumDescriptors);
						SetSamplersRDTBindSlot(CurrentVisibleSF, i);
						break;

					default: check(false); break;
					}
				}
				break;

			case D3D12_ROOT_PARAMETER_TYPE_CBV:
			{
				IncrementMaxCBVCount(CurrentVisibleSF, 1);
				if (CurrentParameter.Descriptor.ShaderRegister == 0)
				{
					// This is the first CBV for this stage, save it's root parameter index (other CBVs will be indexed using this base root parameter index).
					SetCBVRDBindSlot(CurrentVisibleSF, i);
				}

				UpdateCBVRegisterMask(CurrentVisibleSF, CurrentParameter.Descriptor);

				// The first CBV for this stage must come first in the root signature, and subsequent root CBVs for this stage must be contigous.
				check(0xFF != CBVRDBindSlot(CurrentVisibleSF, 0));
				check(i == CBVRDBindSlot(CurrentVisibleSF, 0) + CurrentParameter.Descriptor.ShaderRegister);
			}
			break;

			default:
				// Need to update this for the other types. Currently we only use descriptor tables in the root signature.
				check(false);
				break;
			}
		}
	}
}

FD3D12RootSignatureManager::~FD3D12RootSignatureManager()
{
	for (auto Iter = RootSignatureMap.CreateIterator(); Iter; ++Iter)
	{
		FD3D12RootSignature* pRootSignature = Iter.Value();
		delete pRootSignature;
	}
}

FD3D12RootSignature* FD3D12RootSignatureManager::GetRootSignature(const FD3D12QuantizedBoundShaderState& QBSS)
{
	// Creating bound shader states happens in parallel, so this must be thread safe.
	FScopeLock Lock(&CS);

	FD3D12RootSignature** ppRootSignature = RootSignatureMap.Find(QBSS);
	if (ppRootSignature == nullptr)
	{
		// Create a new root signature and return it.
		return CreateRootSignature(QBSS);
	}

	check(*ppRootSignature);
	return *ppRootSignature;
}

FD3D12RootSignature* FD3D12RootSignatureManager::CreateRootSignature(const FD3D12QuantizedBoundShaderState& QBSS)
{
	// Create a desc and the root signature.
	FD3D12RootSignature* pNewRootSignature = new FD3D12RootSignature(GetParentAdapter(), QBSS);
	check(pNewRootSignature);

	// Add the index to the map.
	RootSignatureMap.Add(QBSS, pNewRootSignature);

	return pNewRootSignature;
}

FD3D12QuantizedBoundShaderState FD3D12RootSignatureManager::GetQuantizedBoundShaderState(const FD3D12RootSignature* const RootSignature)
{
	FScopeLock Lock(&CS);

	const FD3D12QuantizedBoundShaderState* QBSS = RootSignatureMap.FindKey(const_cast<FD3D12RootSignature*>(RootSignature));
	check(QBSS);

	return *QBSS;
}
