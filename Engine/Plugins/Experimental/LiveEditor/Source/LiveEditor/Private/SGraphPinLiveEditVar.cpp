
#include "LiveEditorPrivatePCH.h"

void SGraphPinLiveEditVar::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj, UClass *LiveEditObjectClass)
{
	NodeClass = LiveEditObjectClass;
	CurrentValue = InGraphPinObj->DefaultValue;

	SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
}

TSharedRef<SWidget>	SGraphPinLiveEditVar::GetDefaultValueWidget()
{
	//Get list of enum indexes
	TArray< TSharedPtr<int32> > ComboItems;
	GenerateComboBoxIndexes( ComboItems );

	//Create widget
	return SAssignNew(ComboBox, SPinComboBox)
		.ComboItemList( ComboItems )
		.VisibleText( this, &SGraphPinLiveEditVar::OnGetText )
		.OnSelectionChanged( this, &SGraphPinLiveEditVar::ComboBoxSelectionChanged )
		.Visibility( this, &SGraphPin::GetDefaultValueVisibility )
		.OnGetDisplayName(this, &SGraphPinLiveEditVar::OnGetFriendlyName);
}

FString SGraphPinLiveEditVar::OnGetFriendlyName(int32 EnumIndex) const
{
	if ( EnumIndex >= 0 && EnumIndex < VariableNameList.Num() )
	{
		return VariableNameList[EnumIndex];
	}
	else
	{
		return FString::Printf( TEXT("BOOM - should never happen"), EnumIndex );
	}
}

void SGraphPinLiveEditVar::ComboBoxSelectionChanged( TSharedPtr<int32> NewSelection, ESelectInfo::Type /*SelectInfo*/ )
{
	//Set new selection
	CurrentValue = OnGetFriendlyName( *NewSelection.Get() );
	GraphPinObj->GetSchema()->TrySetDefaultValue(*GraphPinObj, CurrentValue);
}

FString SGraphPinLiveEditVar::OnGetText() const 
{
	//if ( ComboBox.IsValid() && ComboBox->GetSelectedItem().IsValid() )
	if ( CurrentValue != FString(TEXT("")) )
	{
		return CurrentValue;
	}
	else
	{
		return FString( TEXT("Choose Variable") );
	}
}

void SGraphPinLiveEditVar::GenerateComboBoxIndexes( TArray< TSharedPtr<int32> >& OutComboBoxIndexes )
{
	if ( !NodeClass.IsValid() )
		return;

	UClass *InClass = Cast<UClass>(NodeClass.Get());
	if ( InClass == NULL )
		return;

	GenerateComboBoxIndexesRecurse( InClass, FString(TEXT("")), OutComboBoxIndexes );

	AActor *AsActor = Cast<AActor>(InClass->ClassDefaultObject);
	if ( AsActor != NULL )
	{
		TArray<UActorComponent*> ActorComponents;
		AsActor->GetComponents(ActorComponents);
		for ( TArray<UActorComponent*>::TIterator ComponentIt(ActorComponents); ComponentIt; ++ComponentIt )
		{
			UActorComponent *Component = *ComponentIt;
			check( Component != NULL );
			FString ComponentName = Component->GetName() + FString(TEXT("."));
			GenerateComboBoxIndexesRecurse( Component->GetClass(), ComponentName, OutComboBoxIndexes );
		}
	}

	for (int32 i = 0; i < VariableNameList.Num(); ++i)
	{
		TSharedPtr<int32> EnumIdxPtr(new int32(i));
		OutComboBoxIndexes.Add(EnumIdxPtr);
	}
}

bool SGraphPinLiveEditVar::IsPropertyPermitedForLiveEditor( UProperty &Property )
{
	const bool bIsDelegate = Property.IsA(UMulticastDelegateProperty::StaticClass());
		
	//YES (must have all)
	uint64 MustHaveFlags = CPF_Edit;

	//NO (must not have any)
	uint64 MustNotHaveFlags = CPF_BlueprintReadOnly | CPF_DisableEditOnInstance | CPF_EditConst | CPF_Parm;

	return ( Property.HasAllPropertyFlags(MustHaveFlags) &&
			 !Property.HasAnyPropertyFlags(MustNotHaveFlags) && 
			 !bIsDelegate );
}

void SGraphPinLiveEditVar::GenerateComboBoxIndexesRecurse( UStruct *InStruct, FString PropertyPrefix, TArray< TSharedPtr<int32> >& OutComboBoxIndexes )
{
	check( InStruct != NULL );

	for (TFieldIterator<UProperty> PropertyIt(InStruct, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
	{
		UProperty* Property = *PropertyIt;
		if ( !Property->IsA(UNumericProperty::StaticClass()) )
		{
			UStructProperty *StructProp = Cast<UStructProperty>(Property);
			if ( StructProp != NULL && IsPropertyPermitedForLiveEditor(*StructProp) )
			{
				FString NewPropertyPrefix = FString::Printf( TEXT("%s%s."), *PropertyPrefix, *Property->GetName() );
				GenerateComboBoxIndexesRecurse( StructProp->Struct, NewPropertyPrefix, OutComboBoxIndexes );
			}
			continue;
		}

		if ( IsPropertyPermitedForLiveEditor(*Property) )
		{
			FString Name = PropertyPrefix + Property->GetName();
			VariableNameList.Add( Name );
		}
	}
}
