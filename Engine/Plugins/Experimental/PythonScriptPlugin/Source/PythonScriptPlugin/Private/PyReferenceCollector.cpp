// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "PyReferenceCollector.h"
#include "PyWrapperBase.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectHash.h"

#if WITH_PYTHON

/** Reference collector that will purge (null) any references to the given set of objects (as if they'd bee marked PendingKill) */
class FPyPurgingReferenceCollector : public FReferenceCollector
{
public:
	bool HasObjectToPurge() const
	{
		return ObjectsToPurge.Num() > 0;
	}

	void AddObjectToPurge(const UObject* Object)
	{
		ObjectsToPurge.Add(Object);
	}

	virtual bool IsIgnoringArchetypeRef() const override
	{
		return false;
	}

	virtual bool IsIgnoringTransient() const override
	{
		return false;
	}

protected:
	virtual void HandleObjectReference(UObject*& Object, const UObject* ReferencingObject, const UProperty* ReferencingProperty) override
	{
		ConditionalPurgeObject(Object);
	}

	virtual void HandleObjectReferences(UObject** InObjects, const int32 ObjectNum, const UObject* InReferencingObject, const UProperty* InReferencingProperty) override
	{
		for (int32 ObjectIndex = 0; ObjectIndex < ObjectNum; ++ObjectIndex)
		{
			UObject*& Object = InObjects[ObjectIndex];
			ConditionalPurgeObject(Object);
		}
	}

	void ConditionalPurgeObject(UObject*& Object)
	{
		if (ObjectsToPurge.Contains(Object))
		{
			Object = nullptr;
		}
	}

	TSet<const UObject*> ObjectsToPurge;
};

FPyReferenceCollector& FPyReferenceCollector::Get()
{
	static FPyReferenceCollector Instance;
	return Instance;
}

void FPyReferenceCollector::AddWrappedInstance(FPyWrapperBase* InInstance)
{
	PythonWrappedInstances.Add(InInstance);
}

void FPyReferenceCollector::RemoveWrappedInstance(FPyWrapperBase* InInstance)
{
	PythonWrappedInstances.Remove(InInstance);
}

void FPyReferenceCollector::AddReferencedObjects(FReferenceCollector& InCollector)
{
	for (FPyWrapperBase* PythonWrappedInstance : PythonWrappedInstances)
	{
		FPyWrapperBaseMetaData* PythonWrappedInstanceMetaData = FPyWrapperBaseMetaData::GetMetaData(PythonWrappedInstance);
		if (PythonWrappedInstanceMetaData && PythonWrappedInstanceMetaData->AddReferencedObjects)
		{
			PythonWrappedInstanceMetaData->AddReferencedObjects(PythonWrappedInstance, InCollector);
		}
	}
}

void FPyReferenceCollector::PurgeUnrealObjectReferences(const UObject* InObject, const bool bIncludeInnerObjects)
{
	PurgeUnrealObjectReferences(TArrayView<const UObject*>(&InObject, 1), bIncludeInnerObjects);
}

void FPyReferenceCollector::PurgeUnrealObjectReferences(const TArrayView<const UObject*>& InObjects, const bool bIncludeInnerObjects)
{
	FPyPurgingReferenceCollector PurgingReferenceCollector;

	for (const UObject* Object : InObjects)
	{
		PurgingReferenceCollector.AddObjectToPurge(Object);

		if (bIncludeInnerObjects)
		{
			TArray<UObject*> InnerObjects;
			GetObjectsWithOuter(Object, InnerObjects, true);

			for (const UObject* InnerObject : InnerObjects)
			{
				PurgingReferenceCollector.AddObjectToPurge(InnerObject);
			}
		}
	}

	if (PurgingReferenceCollector.HasObjectToPurge())
	{
		AddReferencedObjects(PurgingReferenceCollector);
	}
}

void FPyReferenceCollector::AddReferencedObjectsFromStruct(FReferenceCollector& InCollector, const UStruct* InStruct, void* InStructAddr)
{
	bool bUnused = false;
	AddReferencedObjectsFromStructInternal(InCollector, InStruct, InStructAddr, bUnused);
}

void FPyReferenceCollector::AddReferencedObjectsFromProperty(FReferenceCollector& InCollector, const UProperty* InProp, void* InBaseAddr)
{
	bool bUnused = false;
	AddReferencedObjectsFromPropertyInternal(InCollector, InProp, InBaseAddr, bUnused);
}

void FPyReferenceCollector::AddReferencedObjectsFromStructInternal(FReferenceCollector& InCollector, const UStruct* InStruct, void* InStructAddr, bool& OutValueChanged)
{
	for (TFieldIterator<const UProperty> PropIt(InStruct); PropIt; ++PropIt)
	{
		AddReferencedObjectsFromPropertyInternal(InCollector, *PropIt, InStructAddr, OutValueChanged);
	}
}

void FPyReferenceCollector::AddReferencedObjectsFromPropertyInternal(FReferenceCollector& InCollector, const UProperty* InProp, void* InBaseAddr, bool& OutValueChanged)
{
	if (const UObjectPropertyBase* CastProp = Cast<UObjectPropertyBase>(InProp))
	{
		for (int32 ArrIndex = 0; ArrIndex < InProp->ArrayDim; ++ArrIndex)
		{
			void* ObjValuePtr = CastProp->ContainerPtrToValuePtr<void>(InBaseAddr, ArrIndex);
			UObject* CurObjVal = CastProp->GetObjectPropertyValue(ObjValuePtr);
			if (CurObjVal)
			{
				UObject* NewObjVal = CurObjVal;
				InCollector.AddReferencedObject(NewObjVal);

				if (NewObjVal != CurObjVal)
				{
					OutValueChanged = true;
					CastProp->SetObjectPropertyValue(ObjValuePtr, NewObjVal);
				}
			}
		}
		return;
	}

	if (const UInterfaceProperty* CastProp = Cast<UInterfaceProperty>(InProp))
	{
		for (int32 ArrIndex = 0; ArrIndex < InProp->ArrayDim; ++ArrIndex)
		{
			void* ValuePtr = CastProp->ContainerPtrToValuePtr<void>(InBaseAddr, ArrIndex);
			UObject* CurObjVal = CastProp->GetPropertyValue(ValuePtr).GetObject();
			if (CurObjVal)
			{
				UObject* NewObjVal = CurObjVal;
				InCollector.AddReferencedObject(NewObjVal);

				if (NewObjVal != CurObjVal)
				{
					OutValueChanged = true;
					CastProp->SetPropertyValue(ValuePtr, FScriptInterface(NewObjVal, NewObjVal ? NewObjVal->GetInterfaceAddress(CastProp->InterfaceClass) : nullptr));
				}
			}
		}
		return;
	}

	if (const UStructProperty* CastProp = Cast<UStructProperty>(InProp))
	{
		for (int32 ArrIndex = 0; ArrIndex < InProp->ArrayDim; ++ArrIndex)
		{
			AddReferencedObjectsFromStructInternal(InCollector, CastProp->Struct, CastProp->ContainerPtrToValuePtr<void>(InBaseAddr, ArrIndex), OutValueChanged);
		}
		return;
	}

	if (const UArrayProperty* CastProp = Cast<UArrayProperty>(InProp))
	{
		for (int32 ArrIndex = 0; ArrIndex < InProp->ArrayDim; ++ArrIndex)
		{
			FScriptArrayHelper_InContainer ScriptArrayHelper(CastProp, InBaseAddr, ArrIndex);

			const int32 ElementCount = ScriptArrayHelper.Num();
			for (int32 ElementIndex = 0; ElementIndex < ElementCount; ++ElementIndex)
			{
				AddReferencedObjectsFromPropertyInternal(InCollector, CastProp->Inner, ScriptArrayHelper.GetRawPtr(ElementIndex), OutValueChanged);
			}
		}
		return;
	}

	if (const USetProperty* CastProp = Cast<USetProperty>(InProp))
	{
		for (int32 ArrIndex = 0; ArrIndex < InProp->ArrayDim; ++ArrIndex)
		{
			bool bSetValuesChanged = false;
			FScriptSetHelper_InContainer ScriptSetHelper(CastProp, InBaseAddr, ArrIndex);

			for (int32 SparseElementIndex = 0; SparseElementIndex < ScriptSetHelper.GetMaxIndex(); ++SparseElementIndex)
			{
				if (ScriptSetHelper.IsValidIndex(SparseElementIndex))
				{
					AddReferencedObjectsFromPropertyInternal(InCollector, ScriptSetHelper.GetElementProperty(), ScriptSetHelper.GetElementPtr(SparseElementIndex), bSetValuesChanged);
				}
			}

			if (bSetValuesChanged)
			{
				OutValueChanged = true;
				ScriptSetHelper.Rehash();
			}
		}
		return;
	}

	if (const UMapProperty* CastProp = Cast<UMapProperty>(InProp))
	{
		for (int32 ArrIndex = 0; ArrIndex < InProp->ArrayDim; ++ArrIndex)
		{
			bool bMapKeysChanged = false;
			bool bMapValuesChanged = false;
			FScriptMapHelper_InContainer ScriptMapHelper(CastProp, InBaseAddr, ArrIndex);

			for (int32 SparseElementIndex = 0; SparseElementIndex < ScriptMapHelper.GetMaxIndex(); ++SparseElementIndex)
			{
				if (ScriptMapHelper.IsValidIndex(SparseElementIndex))
				{
					AddReferencedObjectsFromPropertyInternal(InCollector, ScriptMapHelper.GetKeyProperty(), ScriptMapHelper.GetKeyPtr(SparseElementIndex), bMapKeysChanged);
					AddReferencedObjectsFromPropertyInternal(InCollector, ScriptMapHelper.GetValueProperty(), ScriptMapHelper.GetValuePtr(SparseElementIndex), bMapValuesChanged);
				}
			}

			if (bMapKeysChanged || bMapValuesChanged)
			{
				OutValueChanged = true;
				if (bMapKeysChanged)
				{
					ScriptMapHelper.Rehash();
				}
			}
		}
		return;
	}
}

#endif	// WITH_PYTHON
