// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

AActor* FEnvQueryResult::GetItemAsActor(int32 Index) const
{
	if (Items.IsValidIndex(Index) &&
		ItemType->IsChildOf(UEnvQueryItemType_ActorBase::StaticClass()))
	{
		UEnvQueryItemType_ActorBase* DefTypeOb = (UEnvQueryItemType_ActorBase*)ItemType->GetDefaultObject();
		return DefTypeOb->GetActor(RawData.GetTypedData() + Items[Index].DataOffset);
	}

	return NULL;
}

FVector FEnvQueryResult::GetItemAsLocation(int32 Index) const
{
	if (Items.IsValidIndex(Index) &&
		ItemType->IsChildOf(UEnvQueryItemType_LocationBase::StaticClass()))
	{
		UEnvQueryItemType_LocationBase* DefTypeOb = (UEnvQueryItemType_LocationBase*)ItemType->GetDefaultObject();
		return DefTypeOb->GetLocation(RawData.GetTypedData() + Items[Index].DataOffset);
	}

	return FVector::ZeroVector;
}

FString UEnvQueryTypes::GetShortTypeName(const UObject* Ob)
{
	if (Ob == NULL)
	{
		return TEXT("unknown");
	}

	const UClass* ObClass = Ob->IsA(UClass::StaticClass()) ? (const UClass*)Ob : Ob->GetClass();
	if (ObClass->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
	{
		return ObClass->GetName().LeftChop(2);
	}

	FString TypeDesc = ObClass->GetName();
	const int32 ShortNameIdx = TypeDesc.Find(TEXT("_"));
	if (ShortNameIdx != INDEX_NONE)
	{
		TypeDesc = TypeDesc.Mid(ShortNameIdx + 1);
	}

	return TypeDesc;
}

FString UEnvQueryTypes::DescribeContext(TSubclassOf<class UEnvQueryContext> ContextClass)
{
	return GetShortTypeName(ContextClass);
}

FString UEnvQueryTypes::DescribeIntParam(const FEnvIntParam& Param)
{
	if (Param.IsNamedParam())
	{
		return Param.ParamName.ToString();
	}

	return FString::Printf(TEXT("%d"), Param.Value);
}

FString UEnvQueryTypes::DescribeFloatParam(const FEnvFloatParam& Param)
{
	if (Param.IsNamedParam())
	{
		return Param.ParamName.ToString();
	}

	return FString::Printf(TEXT("%.2f"), Param.Value);
}

FString UEnvQueryTypes::DescribeBoolParam(const FEnvBoolParam& Param)
{
	if (Param.IsNamedParam())
	{
		return Param.ParamName.ToString();
	}

	return Param.Value ? TEXT("true") : TEXT("false");
}

//----------------------------------------------------------------------//
// namespace FEQSHelpers
//----------------------------------------------------------------------//
namespace FEQSHelpers
{
	const ARecastNavMesh* FindNavMeshForQuery(FEnvQueryInstance& QueryInstance)
	{
		const UNavigationSystem* NavSys = QueryInstance.World->GetNavigationSystem();

		// try to match navigation agent for querier
		INavAgentInterface* NavAgent = QueryInstance.Owner.IsValid() ? InterfaceCast<INavAgentInterface>(QueryInstance.Owner.Get()) : NULL;
		if (NavAgent)
		{
			const FNavAgentProperties* NavAgentProps = NavAgent ? NavAgent->GetNavAgentProperties() : NULL;
			if (NavAgentProps != NULL)
			{
				return Cast<const ARecastNavMesh>(NavSys->GetNavDataForProps(*NavAgentProps));
			}
		}

		return Cast<const ARecastNavMesh>(NavSys->GetMainNavData());
	}
}