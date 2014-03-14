// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

/**
 * Helper class for restoring default values on UDistribution objects in particle modules.
 */
struct FDistributionHelpers
{
private:
	/**
	 * Restores UDistribution default value after moving them to PostInitProperties.
	 *
	 * @param Name UDistribution parameter name.
	 * @param Value Default value.
	 */
	template <class DistributionType, typename ValueType>
	static bool RestoreDefaultConstant(UDistribution* Distribution, const TCHAR* Name, ValueType Value = ValueType(0.0f))
	{
		DistributionType* ConstantDistribution = Cast<DistributionType>(Distribution);
		if (ConstantDistribution && ConstantDistribution->Constant == ValueType(DefaultValue) && ConstantDistribution->GetName() == Name )
		{
			ConstantDistribution->Constant = Value;
		}
		return !!ConstantDistribution;
	}

	/**
	 * Restores UDistribution default value after moving them to PostInitProperties.
	 *
	 * @param Name UDistribution parameter name.
	 * @param Value Default value.
	 */
	template <class DistributionType, typename ValueType>
	static bool RestoreDefaultUniform(UDistribution* Distribution, const TCHAR* Name, ValueType Min = ValueType(0.0f), ValueType Max = ValueType(0.0f))
	{
		DistributionType* UniformDistribution = Cast<DistributionType>(Distribution);
		if (UniformDistribution && UniformDistribution->GetName() == Name)
		{
			if (UniformDistribution->Min == ValueType(DefaultValue))
			{
				UniformDistribution->Min = Min;
			}
			if (UniformDistribution->Max == ValueType(DefaultValue))
			{
				UniformDistribution->Max = Max;
			}
		}
		return !!UniformDistribution;
	}

public:

	/** Default value for initializing and checking correct values on UDistributions. */
	static const float DefaultValue;

	/**
	 * Restores UDistribution default value after moving them to PostInitProperties.
	 * Looks for all known distribution types if the default type is not found.
	 *
	 * @param Distribution UDistribution object to restore default value for.
	 * @param Name Name of the default distribution object.
	 * @param Value Default value.
	 */
	static bool RestoreDefaultConstant(UDistribution* Distribution, const TCHAR* Name, float Value)
	{
		if (RestoreDefaultConstant<UDistributionFloatConstant>(Distribution, Name, Value))
		{
			return true;
		}
		// Fallback to other known distribution types.
		if (RestoreDefaultConstant<UDistributionVectorConstant>(Distribution, Name, FVector::ZeroVector))
		{
			return true;
		}
		if (RestoreDefaultUniform<UDistributionFloatUniform>(Distribution, Name, 0.0f, 0.0f))
		{
			return true;
		}
		if (RestoreDefaultUniform<UDistributionVectorUniform>(Distribution, Name, FVector::ZeroVector, FVector::ZeroVector))
		{
			return true;
		}
		return false;
	}

	/**
	 * Restores UDistribution default value after moving them to PostInitProperties.
	 * Looks for all known distribution types if the default type is not found.
	 *
	 * @param Distribution UDistribution object to restore default value for.
	 * @param Name Name of the default distribution object.
	 * @param Value Default value.
	 */
	static bool RestoreDefaultConstant(UDistribution* Distribution, const TCHAR* Name, FVector Value)
	{
		if (RestoreDefaultConstant<UDistributionVectorConstant>(Distribution, Name, Value))
		{
			return true;
		}
		// Fallback to other known distribution types.
		if (RestoreDefaultConstant<UDistributionFloatConstant>(Distribution, Name, 0.0f))
		{
			return true;
		}
		if (RestoreDefaultUniform<UDistributionFloatUniform>(Distribution, Name, 0.0f, 0.0f))
		{
			return true;
		}
		if (RestoreDefaultUniform<UDistributionVectorUniform>(Distribution, Name, FVector::ZeroVector, FVector::ZeroVector))
		{
			return true;
		}
		return false;
	}

	/**
	 * Restores UDistribution default value after moving them to PostInitProperties.
	 * Looks for all known distribution types if the default type is not found.
	 *
	 * @param Distribution UDistribution object to restore default value for.
	 * @param Name Name of the default distribution object.
	 * @param Value Default value.
	 */
	static bool RestoreDefaultUniform(UDistribution* Distribution, const TCHAR* Name, float Min, float Max)
	{
		if (RestoreDefaultUniform<UDistributionFloatUniform>(Distribution, Name, Min, Max))
		{
			return true;
		}
		// Fallback to other known distribution types.
		if (RestoreDefaultUniform<UDistributionVectorUniform>(Distribution, Name, FVector::ZeroVector, FVector::ZeroVector))
		{
			return true;
		}
		if (RestoreDefaultConstant<UDistributionFloatConstant>(Distribution, Name, 0.0f))
		{
			return true;
		}
		if (RestoreDefaultConstant<UDistributionVectorConstant>(Distribution, Name, FVector::ZeroVector))
		{
			return true;
		}
		return false;
	}

	/**
	 * Restores UDistribution default value after moving them to PostInitProperties.
	 * Looks for all known distribution types if the default type is not found.
	 *
	 * @param Distribution UDistribution object to restore default value for.
	 * @param Name Name of the default distribution object.
	 * @param Value Default value.
	 */
	static bool RestoreDefaultUniform(UDistribution* Distribution, const TCHAR* Name, FVector Min, FVector Max)
	{
		if (RestoreDefaultUniform<UDistributionVectorUniform>(Distribution, Name, Min, Max))
		{
			return true;
		}
		// Fallback to other known distribution types.
		if (RestoreDefaultUniform<UDistributionFloatUniform>(Distribution, Name, 0.0f, 0.0f))
		{
			return true;
		}
		if (RestoreDefaultConstant<UDistributionFloatConstant>(Distribution, Name, 0.0f))
		{
			return true;
		}
		if (RestoreDefaultConstant<UDistributionVectorConstant>(Distribution, Name, FVector::ZeroVector))
		{
			return true;
		}
		return false;
	}
};

