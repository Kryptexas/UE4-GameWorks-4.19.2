// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NiagaraConstantSet.h"

#include "NiagaraScriptConstantData.generated.h"

USTRUCT()
struct FNiagaraScriptConstantData
{
	GENERATED_USTRUCT_BODY()

	/** The set of external constants for this script. */
	UPROPERTY()
	FNiagaraConstants ExternalConstants;

	/** The set of internal constants for this script. */
	UPROPERTY()
	FNiagaraConstants InternalConstants;

	/** Fill a constants table ready for use in an update or spawn. */
	void FillConstantTable(const FNiagaraConstantMap& ExternalConstantsMap, TArray<FVector4>& OutConstantTable, TArray<FNiagaraDataObject *> &DataObjTable) const
	{
		//First up in the table comes the External constants in scalar->vector->matrix order.
		//Only fills the constants actually used by the script.
		OutConstantTable.Empty();
		ExternalConstants.AppendToConstantsTable(OutConstantTable, ExternalConstantsMap);
		ExternalConstants.AppendExternalBufferConstants(DataObjTable, ExternalConstantsMap);
		//Next up add all the internal constants from the script.
		InternalConstants.AppendToConstantsTable(OutConstantTable);
		InternalConstants.AppendBufferConstants(DataObjTable);
	}

	void SetOrAddInternal(FName Name, float Sc)
	{
		InternalConstants.SetOrAdd(Name, Sc);
	}

	void SetOrAddInternal(FName Name, const FVector4& Vc)
	{
		InternalConstants.SetOrAdd(Name, Vc);
	}

	void SetOrAddInternal(FName Name, const FMatrix& Mc)
	{
		InternalConstants.SetOrAdd(Name, Mc);
	}

	void SetOrAddInternal(FName Name, FNiagaraDataObject* Curve)
	{
		InternalConstants.SetOrAdd(Name, Curve);
	}

	void SetOrAddExternal(FName Name, float Sc)
	{
		ExternalConstants.SetOrAdd(Name, Sc);
	}

	void SetOrAddExternal(FName Name, const FVector4& Vc)
	{
		ExternalConstants.SetOrAdd(Name, Vc);
	}

	void SetOrAddExternal(FName Name, const FMatrix& Mc)
	{
		ExternalConstants.SetOrAdd(Name, Mc);
	}

	void SetOrAddExternal(FName Name, FNiagaraDataObject* Curve)
	{
		ExternalConstants.SetOrAdd(Name, Curve);
	}


	/**
	Calculates the index into a a constant table created from this constant data for the given constant name.
	Obviously, assumes the constant data is complete. Any additions etc will invalidate previously calculated indexes.
	*/
	void GetTableIndex(FName Name, bool bInternal, int32& OutConstantIdx, int32& OutComponentIndex, ENiagaraDataType& OutType)const
	{
		int32 Base = bInternal ? ExternalConstants.GetTableSize() : 0;
		int32 ConstIdx = INDEX_NONE;
		int32 CompIdx = INDEX_NONE;
		ENiagaraDataType Type;
		const FNiagaraConstants& Consts = bInternal ? InternalConstants : ExternalConstants;

		ConstIdx = Consts.GetAbsoluteIndex_Scalar(Name);
		Type = ENiagaraDataType::Scalar;
		if (ConstIdx == INDEX_NONE)
		{
			ConstIdx = Consts.GetTableIndex_Vector(Name);
			Type = ENiagaraDataType::Vector;
			if (ConstIdx == INDEX_NONE)
			{
				ConstIdx = Consts.GetTableIndex_Matrix(Name);
				Type = ENiagaraDataType::Matrix;

				if (ConstIdx == INDEX_NONE)	// curves/buffers are in a separate table, so set base to 0
				{
					ConstIdx = Consts.GetTableIndex_DataObj(Name);
					Type = ENiagaraDataType::Curve;
					Base = 0;
				}
			}
		}
		else
		{
			CompIdx = ConstIdx % 4;
			ConstIdx /= 4;
		}

		OutConstantIdx = Base + ConstIdx;
		OutComponentIndex = CompIdx;
		OutType = Type;
	}
};
