#pragma once

struct FFlexGPUParticleSimulationParameters
{
	FRHIShaderResourceView* ParticleIndexBufferSRV = nullptr;
	FRHIShaderResourceView* PositionBufferSRV = nullptr;
	FRHIShaderResourceView* VelocityBufferSRV = nullptr;
};

struct FFlexGPUParticleSimulationShaderParameters
{
	FShaderResourceParameter ParticleIndexBuffer;
	FShaderResourceParameter PositionBuffer;
	FShaderResourceParameter VelocityBuffer;

	void Bind(const FShaderParameterMap& ParameterMap)
	{
		ParticleIndexBuffer.Bind(ParameterMap, TEXT("FlexParticleIndexBuffer"));
		PositionBuffer.Bind(ParameterMap, TEXT("FlexPositionBuffer"));
		VelocityBuffer.Bind(ParameterMap, TEXT("FlexVelocityBuffer"));
	}

	template <typename ShaderRHIParamRef>
	void Set(FRHICommandList& RHICmdList, const ShaderRHIParamRef& ShaderRHI, const FFlexGPUParticleSimulationParameters& Parameters) const
	{
		if (ParticleIndexBuffer.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(ShaderRHI, ParticleIndexBuffer.GetBaseIndex(), Parameters.ParticleIndexBufferSRV);
		}
		if (PositionBuffer.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(ShaderRHI, PositionBuffer.GetBaseIndex(), Parameters.PositionBufferSRV);
		}
		if (VelocityBuffer.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(ShaderRHI, VelocityBuffer.GetBaseIndex(), Parameters.VelocityBufferSRV);
		}
	}

	template <typename ShaderRHIParamRef>
	void UnbindBuffers(FRHICommandList& RHICmdList, const ShaderRHIParamRef& ShaderRHI) const
	{
		const FShaderResourceViewRHIParamRef NullSRV = FShaderResourceViewRHIParamRef();
		if (ParticleIndexBuffer.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(ShaderRHI, ParticleIndexBuffer.GetBaseIndex(), NullSRV);
		}
		if (PositionBuffer.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(ShaderRHI, PositionBuffer.GetBaseIndex(), NullSRV);
		}
		if (VelocityBuffer.IsBound())
		{
			RHICmdList.SetShaderResourceViewParameter(ShaderRHI, VelocityBuffer.GetBaseIndex(), NullSRV);
		}
	}
};

FArchive& operator<<(FArchive& Ar, FFlexGPUParticleSimulationShaderParameters& Parameters)
{
	Ar << Parameters.ParticleIndexBuffer;
	Ar << Parameters.PositionBuffer;
	Ar << Parameters.VelocityBuffer;
	return Ar;
}
