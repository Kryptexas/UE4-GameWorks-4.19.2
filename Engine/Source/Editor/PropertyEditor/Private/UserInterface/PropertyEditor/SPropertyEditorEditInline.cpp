// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "PropertyEditorPrivatePCH.h"
#include "SPropertyEditorEditInline.h"
#include "PropertyEditorHelpers.h"
#include "PropertyNode.h"
#include "ObjectPropertyNode.h"
#include "PropertyEditor.h"
#include "PropertyHandleImpl.h"
#include "SPropertyComboBox.h"
#include "Editor/ClassViewer/Public/ClassViewerModule.h"
#include "Editor/ClassViewer/Public/ClassViewerFilter.h"

class FPropertyEditorInlineClassFilter : public IClassViewerFilter
{
public:
	/** The Object Property, classes are examined for a child-of relationship of the property's class. */
	UObjectPropertyBase* ObjProperty;

	/** The Interface Property, classes are examined for implementing the property's class. */
	UInterfaceProperty* IntProperty;

	/** Whether or not abstract classes are allowed. */
	bool bAllowAbstract;

	/** Limits the visible classes to meeting IsA relationships with all the objects. */
	TSet< const UObject* > LimitToIsAOfAllObjects;

	bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs )
	{
		const bool bChildOfObjectClass = ObjProperty && InClass->IsChildOf(ObjProperty->PropertyClass);
		const bool bDerivedInterfaceClass = IntProperty && InClass->ImplementsInterface(IntProperty->InterfaceClass);

		bool bMatchesFlags = InClass->HasAnyClassFlags(CLASS_EditInlineNew) && 
			!InClass->HasAnyClassFlags(CLASS_Hidden|CLASS_HideDropDown|CLASS_Deprecated) &&
			(bAllowAbstract || !InClass->HasAnyClassFlags(CLASS_Abstract));

		if( (bChildOfObjectClass || bDerivedInterfaceClass) && bMatchesFlags )
		{
			// The class must satisfy a Is-A relationship with all the objects in this set.
			return InFilterFuncs->IfMatchesAll_ObjectsSetIsAClass(LimitToIsAOfAllObjects, InClass->ClassWithin) != EFilterReturn::Failed;
		}

		return false;
	}

	virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const IUnloadedBlueprintData > InUnloadedClassData, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) OVERRIDE
	{
		const bool bChildOfObjectClass = InUnloadedClassData->IsChildOf(ObjProperty->PropertyClass);

		bool bMatchesFlags = InUnloadedClassData->HasAnyClassFlags(CLASS_EditInlineNew) && 
			!InUnloadedClassData->HasAnyClassFlags(CLASS_Hidden|CLASS_HideDropDown|CLASS_Deprecated) && 
			(bAllowAbstract || !InUnloadedClassData->HasAnyClassFlags((CLASS_Abstract)));

		if(bChildOfObjectClass && bMatchesFlags)
		{
			// The class must satisfy a Is-A relationship with all the objects in this set.
			return InFilterFuncs->IfMatchesAll_ObjectsSetIsAClass(LimitToIsAOfAllObjects, InUnloadedClassData) != EFilterReturn::Failed;
		}

		return false;
	}
};

void SPropertyEditorEditInline::Construct( const FArguments& InArgs, const TSharedRef< class FPropertyEditor >& InPropertyEditor )
{
	PropertyEditor = InPropertyEditor;

	ChildSlot
	[
		SAssignNew(ComboButton, SComboButton)
		.OnGetMenuContent(this, &SPropertyEditorEditInline::GenerateClassPicker)
		.ContentPadding(0)
		.ToolTipText(InPropertyEditor, &FPropertyEditor::GetValueAsString )
		.ButtonContent()
		[
			SNew( STextBlock )
			.Text( this, &SPropertyEditorEditInline::GetDisplayValueAsString )
		]
	];
}

FString SPropertyEditorEditInline::GetDisplayValueAsString() const
{
	UObject* CurrentValue = NULL;
	FPropertyAccess::Result Result = PropertyEditor->GetPropertyHandle()->GetValue( CurrentValue );
	if( Result == FPropertyAccess::Success && CurrentValue != NULL )
	{
		return CurrentValue->GetName();
	}
	else
	{
		return PropertyEditor->GetValueAsString();
	}
}

void SPropertyEditorEditInline::GetDesiredWidth( float& OutMinDesiredWidth, float& OutMaxDesiredWidth )
{
	OutMinDesiredWidth = 250.0f;
	OutMaxDesiredWidth = 600.0f;
}

bool SPropertyEditorEditInline::Supports( const FPropertyNode* InTreeNode, int32 InArrayIdx )
{
	if (InTreeNode->IsEditConst())
	{
		return false;
	}

	bool bCanEditInline = false;

	if( InTreeNode->HasNodeFlags(EPropertyNodeFlags::EditInline) != 0 )
	{
		// Assume objects with the editinline flag can be editinline.  If any of the parent object is a default subobject then we cannot allow editinline
		bCanEditInline = true;
		const FObjectPropertyNode* ObjectPropertyNode = InTreeNode->FindObjectItemParent();
		for( int32 ObjectIndex = 0; ObjectIndex < ObjectPropertyNode->GetNumObjects(); ++ObjectIndex )
		{
			const UObject* Object = ObjectPropertyNode->GetUObject(ObjectIndex);

			if( Object && Object->IsDefaultSubobject() && Object->IsTemplate() )
			{
				bCanEditInline = false;
				break;
			}
		}

	}

	return bCanEditInline;
}

bool SPropertyEditorEditInline::Supports( const TSharedRef< class FPropertyEditor >& InPropertyEditor )
{
	const TSharedRef< FPropertyNode > PropertyNode = InPropertyEditor->GetPropertyNode();
	return SPropertyEditorEditInline::Supports( &PropertyNode.Get(), PropertyNode->GetArrayIndex() );
}

bool SPropertyEditorEditInline::IsClassAllowed( UClass* CheckClass, bool bAllowAbstract ) const
{
	check(CheckClass);
	return PropertyEditorHelpers::IsEditInlineClassAllowed( CheckClass, bAllowAbstract ) &&  CheckClass->HasAnyClassFlags(CLASS_EditInlineNew);
}

TSharedRef<SWidget> SPropertyEditorEditInline::GenerateClassPicker()
{
	FClassViewerInitializationOptions Options;
	Options.bShowUnloadedBlueprints = true;

	TSharedPtr<FPropertyEditorInlineClassFilter> ClassFilter = MakeShareable( new FPropertyEditorInlineClassFilter );
	Options.ClassFilter = ClassFilter;
	ClassFilter->bAllowAbstract = false;

	const TSharedRef< FPropertyNode > PropertyNode = PropertyEditor->GetPropertyNode();
	UProperty* Property = PropertyNode->GetProperty();
	ClassFilter->ObjProperty = Cast<UObjectPropertyBase>( Property );
	ClassFilter->IntProperty = Cast<UInterfaceProperty>( Property );

	FObjectPropertyNode* ObjectPropertyNode = PropertyNode->FindObjectItemParent();
	if( ObjectPropertyNode )
	{
		for ( TPropObjectIterator Itor( ObjectPropertyNode->ObjectIterator() ); Itor; ++Itor )
		{
			UObject* OwnerObject = Itor->Get();
			ClassFilter->LimitToIsAOfAllObjects.Add( OwnerObject );
		}
	}

	Options.PropertyHandle = PropertyEditor->GetPropertyHandle();

	FOnClassPicked OnPicked( FOnClassPicked::CreateRaw( this, &SPropertyEditorEditInline::OnClassPicked ) );

	return FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer").CreateClassViewer(Options, OnPicked);
}

void SPropertyEditorEditInline::OnClassPicked(UClass* InClass)
{
	if( !InClass )
	{
		return;
	}

	TArray<FObjectBaseAddress> ObjectsToModify;
	TArray<FString> NewValues;

	const TSharedRef< FPropertyNode > PropertyNode = PropertyEditor->GetPropertyNode();
	FObjectPropertyNode* ObjectNode = PropertyNode->FindObjectItemParent();

	if( ObjectNode )
	{
		for ( TPropObjectIterator Itor( ObjectNode->ObjectIterator() ) ; Itor ; ++Itor )
		{
			UObject*		Object = Itor->Get();
			UObject*		UseOuter			= ( InClass->IsChildOf( UClass::StaticClass() ) ? Cast<UClass>(Object)->GetDefaultObject() : Object );
			EObjectFlags	MaskedOuterFlags	= UseOuter ? UseOuter->GetMaskedFlags(RF_PropagateToSubObjects) : RF_NoFlags;
			UObject*		NewObject			= StaticConstructObject( InClass, UseOuter, NAME_None, MaskedOuterFlags, NULL );

			NewValues.Add( NewObject->GetPathName() );
		}

		const TSharedRef< IPropertyHandle > PropertyHandle = PropertyEditor->GetPropertyHandle();
		PropertyHandle->SetPerObjectValues( NewValues );

		// Force a rebuild of the children when this node changes
		PropertyNode->RequestRebuildChildren();

		ComboButton->SetIsOpen(false);
	}
}