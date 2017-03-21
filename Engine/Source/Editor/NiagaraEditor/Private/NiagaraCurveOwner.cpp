// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "NiagaraCurveOwner.h"

void FNiagaraCurveOwner::EmptyCurves()
{
	ConstCurves.Empty();
	Curves.Empty();
	EditInfoToOwnerMap.Empty();
	EditInfoToColorMap.Empty();
	EditInfoToNotifyCurveChangedMap.Empty();
}

void FNiagaraCurveOwner::AddCurve(FRichCurve& Curve, FName& Name, FLinearColor& Color, UObject& Owner, FNotifyCurveChanged CurveChangedHandler)
{
	FRichCurveEditInfo EditInfo(&Curve, Name);
	Curves.Add(EditInfo);
	ConstCurves.Add(FRichCurveEditInfoConst(&Curve, Name));
	EditInfoToOwnerMap.Add(EditInfo, &Owner);
	EditInfoToColorMap.Add(EditInfo, Color);
	EditInfoToNotifyCurveChangedMap.Add(EditInfo, CurveChangedHandler);
}

TArray<FRichCurveEditInfoConst> FNiagaraCurveOwner::GetCurves() const
{
	return ConstCurves;
}

TArray<FRichCurveEditInfo> FNiagaraCurveOwner::GetCurves()
{
	return Curves;
};

void FNiagaraCurveOwner::ModifyOwner()
{
	TArray<UObject*> Owners;
	EditInfoToOwnerMap.GenerateValueArray(Owners);
	for (UObject* Owner : Owners)
	{
		Owner->Modify();
	}
}

TArray<const UObject*> FNiagaraCurveOwner::GetOwners() const
{
	TArray<UObject*> Owners;
	EditInfoToOwnerMap.GenerateValueArray(Owners);
	TArray<const UObject*> ConstOwners;
	ConstOwners.Append(Owners);
	return ConstOwners;
}

void FNiagaraCurveOwner::MakeTransactional()
{
	TArray<UObject*> Owners;
	EditInfoToOwnerMap.GenerateValueArray(Owners);
	for (UObject* Owner : Owners)
	{
		Owner->SetFlags(RF_Transactional);
	}
}

void FNiagaraCurveOwner::OnCurveChanged(const TArray<FRichCurveEditInfo>& ChangedCurveEditInfos)
{
	for (const FRichCurveEditInfo& ChangedCurveEditInfo : ChangedCurveEditInfos)
	{
		FNotifyCurveChanged* CurveChanged = EditInfoToNotifyCurveChangedMap.Find(ChangedCurveEditInfo);
		if (CurveChanged != nullptr)
		{
			CurveChanged->Execute();
		}
	}
}

bool FNiagaraCurveOwner::IsValidCurve(FRichCurveEditInfo CurveInfo)
{
	return EditInfoToOwnerMap.Contains(CurveInfo);
}

FLinearColor FNiagaraCurveOwner::GetCurveColor(FRichCurveEditInfo CurveInfo) const
{
	const FLinearColor* Color = EditInfoToColorMap.Find(CurveInfo);
	if (Color != nullptr)
	{
		return *Color;
	}
	return FCurveOwnerInterface::GetCurveColor(CurveInfo);
}