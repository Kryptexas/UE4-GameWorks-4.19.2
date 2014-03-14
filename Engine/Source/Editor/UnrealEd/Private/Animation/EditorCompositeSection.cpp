// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AnimMontage.cpp: Montage classes that contains slots
=============================================================================*/ 

#include "UnrealEd.h"



UEditorCompositeSection::UEditorCompositeSection(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

	SectionIndex = INDEX_NONE;
}

void UEditorCompositeSection::InitSection(int SectionIndexIn)
{
	SectionIndex = SectionIndexIn;
	if(UAnimMontage* Montage = Cast<UAnimMontage>(AnimObject))
	{
		if(Montage->CompositeSections.IsValidIndex(SectionIndex))
		{
			CompositeSection = Montage->CompositeSections[SectionIndex];
		}
	}
}
bool UEditorCompositeSection::ApplyChangesToMontage()
{
	if(UAnimMontage* Montage = Cast<UAnimMontage>(AnimObject))
	{
		if(Montage->CompositeSections.IsValidIndex(SectionIndex))
		{
			Montage->CompositeSections[SectionIndex] = CompositeSection;
			return true;
		}
	}

	return false;
}
