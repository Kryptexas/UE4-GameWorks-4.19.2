// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CoreUObjectPrivate.h"
#include "ArchiveDescribeReference.h"


FArchiveDescribeReference::FArchiveDescribeReference( UObject* Src, UObject* InTarget, FOutputDevice& InOutput ) : Source(Src)
	, Target(InTarget)
	, Output(InOutput)
{
	// use the optimized RefLink to skip over properties which don't contain object references
	ArIsObjectReferenceCollector = true;

	ArIgnoreArchetypeRef				= false;
	ArIgnoreOuterRef					= true;
	ArIgnoreClassRef					= false;

	GSerializedProperty = NULL;
	Source->Serialize( *this );
}

FArchive& FArchiveDescribeReference::operator<<( class UObject*& Obj )
{
	if (Obj == Target)
	{
		if (GSerializedProperty)
		{
			Output.Logf(TEXT("        [%s]"), *GSerializedProperty->GetFullName());
		}
		else
		{
			Output.Logf(TEXT("        [native]"));
		}
		PTRINT BigOffset = ((uint8*)&Obj) - (uint8*)Source;
		if (BigOffset > 0 && BigOffset < Source->GetClass()->GetPropertiesSize())
		{
			int32 Offset = int32(BigOffset);
			UClass* UseClass = Source->GetClass();
			UClass* SuperClass = UseClass->GetSuperClass();
			while (1)
			{
				if (!SuperClass || Offset >= SuperClass->GetPropertiesSize())
				{
					break;
				}
				UseClass = SuperClass;
				SuperClass = UseClass->GetSuperClass();
			}
			Output.Logf(TEXT("            class %s offset %d, offset from UObject %d "), *UseClass->GetName(), SuperClass ? Offset - SuperClass->GetPropertiesSize() : Offset, Offset);
		}
	}
	return *this;
}
