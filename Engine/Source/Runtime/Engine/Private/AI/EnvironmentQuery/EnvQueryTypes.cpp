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
		ItemType->IsChildOf(UEnvQueryItemType_VectorBase::StaticClass()))
	{
		UEnvQueryItemType_VectorBase* DefTypeOb = (UEnvQueryItemType_VectorBase*)ItemType->GetDefaultObject();
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

FString FEnvDirection::ToString() const
{
	return (DirMode == EEnvDirection::TwoPoints) ?
		FString::Printf(TEXT("[%s - %s]"), *UEnvQueryTypes::DescribeContext(LineFrom), *UEnvQueryTypes::DescribeContext(LineTo)) :
		FString::Printf(TEXT("[%s rotation]"), *UEnvQueryTypes::DescribeContext(Rotation));
}

FString FEnvTraceData::ToString(FEnvTraceData::EDescriptionMode DescMode) const
{
	FString Desc;

	if (TraceMode == EEnvQueryTrace::Geometry)
	{
		Desc = (TraceShape == EEnvTraceShape::Line) ? TEXT("line") :
			(TraceShape == EEnvTraceShape::Sphere) ? FString::Printf(TEXT("sphere (radius: %.2f)"), ExtentX) :
			(TraceShape == EEnvTraceShape::Capsule) ? FString::Printf(TEXT("capsule (radius: %.2f, half height: %.2f)"), ExtentX, ExtentZ) :
			(TraceShape == EEnvTraceShape::Box) ? FString::Printf(TEXT("box (extent: %.2f %.2f %.2f)"), ExtentX, ExtentY, ExtentZ) :
			TEXT("unknown");

		if (DescMode == FEnvTraceData::Brief)
		{
			static UEnum* ChannelEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("ETraceTypeQuery"), true);
			Desc += FString::Printf(TEXT(" %s on %s"), bCanProjectDown ? TEXT("projection") : TEXT("trace"), *ChannelEnum->GetEnumString(TraceChannel));
		}
		else
		{
			if (bTraceComplex)
			{
				Desc += TEXT(", complex collision");
			}

			if (!bOnlyBlockingHits)
			{
				Desc += TEXT(", accept non blocking");
			}
		}
	}
	else if (TraceMode == EEnvQueryTrace::Navigation)
	{
		Desc = TEXT("navmesh ");
		if (bCanProjectDown)
		{
			Desc += FString::Printf(TEXT("projection (%s: %.0f"),
				(ProjectDown == ProjectUp) ? TEXT("height") : (ProjectDown > ProjectUp) ? TEXT("down") : TEXT("up"),
				FMath::Max(ProjectDown, ProjectUp));

			if (ExtentX > 1.0f)
			{
				Desc += FString::Printf(TEXT(", radius: %.2f"), ExtentX);
			}
			Desc += TEXT(")");
		}
		else
		{
			Desc += TEXT("trace");
			if (NavigationFilter)
			{
				Desc += TEXT(" (filter: ");
				Desc += NavigationFilter->GetName();
				Desc += TEXT(")");
			}
		}
	}

	return Desc;
}

void FEnvTraceData::SetGeometryOnly()
{
	TraceMode = EEnvQueryTrace::Geometry;
	bCanTraceOnGeometry = true;
	bCanTraceOnNavMesh = false;
	bCanDisableTrace = false;
}

void FEnvTraceData::SetNavmeshOnly()
{
	TraceMode = EEnvQueryTrace::Navigation;
	bCanTraceOnGeometry = false;
	bCanTraceOnNavMesh = true;
	bCanDisableTrace = false;
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