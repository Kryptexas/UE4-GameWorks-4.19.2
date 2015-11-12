// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once


struct UMG_API FSlateVectorArtInstanceData
{
public:
	const FVector4& GetData() const { return Data; }
	FVector4& GetData() { return Data; }

	/**
	 * A Float4 can pack 12 bytes worth of data (we cannot safely use the exponent bits)
	 */
	template<int32 Component, int32 ByteIndex>
	void PackFloatIntoByte(float InValue)
	{
		PackByteIntoByte<Component, ByteIndex>(static_cast<uint8>(FMath::RoundToInt(InValue * 0xFFu)));
	}
	
	template<int32 Component, int32 ByteIndex>
	void PackByteIntoByte(uint8 InValue)
	{
		// Each float has 24 usable bits of mantissa, but we cannot access the bits directly.
		// We will not respect the IEEE "normalized mantissa" rules, so let 
		// the compiler/FPU do conversions from byte to float and vice versa for us.

		//Produces a value like 0xFFFF00FF; where the 00s are shifted by `Position` bytes.
		static const uint32 Mask = ~(0xFF << ByteIndex * 8);

		// Clear out a byte to OR with while maintaining the rest of the data intact.
		uint32 CurrentData = static_cast<uint32>(Data[Component]) & Mask;
		// OR in the value
		Data[Component] = CurrentData | (InValue << ByteIndex*8);
	}

	static float AdjustScaleInput(float UnadjustedScaleInput)
	{
		// We only support scaling UP (initial "vector" art is at 1:1 resolution)
		// We also don't want to waste any bits on supporting fractional scale values.
		// So store a value between 0 and 3, with an assumed base scale of 1.0f
		static const float MAXIMUM_SCALE = 4.0f;
		return FMath::Clamp(UnadjustedScaleInput, 0.0f, MAXIMUM_SCALE) / MAXIMUM_SCALE;
	}

	template<int32 Component, int32 ByteIndex>
	void PackByteIntoByte(float InValue)
	{
		checkf(false, TEXT("PackByteIntoByte() being used with a float value. This is almost definitely an error. Use PackFloatIntoByte()."));
	}

	template<int32 Component, int32 ByteIndex>
	float UnpackValue() const
	{
		// See PackFloatIntoByte
		static const float Denominator = static_cast<float>(0x1 << (ByteIndex+1)*8);
		return Data[Component] / Denominator;
	}

	void SetPositionFixedPoint16(FVector2D Position);

	void SetPosition(FVector2D Position);
	void SetScale(float Scale);

protected:
	FVector4 Data;
};