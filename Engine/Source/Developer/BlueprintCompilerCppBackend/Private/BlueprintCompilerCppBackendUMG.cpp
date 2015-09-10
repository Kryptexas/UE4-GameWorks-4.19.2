// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "BlueprintCompilerCppBackendModulePrivatePCH.h"
#include "BlueprintCompilerCppBackendUtils.h"
#include "WidgetBlueprintGeneratedClass.h"
#include "WidgetAnimation.h"
#include "WidgetTree.h"

FString FBackendHelperUMG::WidgetFunctionsInHeader(UClass* SourceClass)
{
	if (Cast<UWidgetBlueprintGeneratedClass>(SourceClass))
	{
		return FString(TEXT("public:\n\tvirtual void GetSlotNames(TArray<FName>& SlotNames) const override;\n\tvirtual void PreSave() override;\n\tvirtual void CustomNativeInitilize() override;\n"));
	}
	return FString();
}

FString FBackendHelperUMG::AdditionalHeaderIncludeForWidget(UClass* SourceClass)
{
	if (Cast<UWidgetBlueprintGeneratedClass>(SourceClass))
	{
		return FString(TEXT("\n#include \"Runtime/UMG/Public/UMG.h\"\n"));
	}
	return FString();
}

void FBackendHelperUMG::CreateClassSubobjects(FEmitterLocalContext& Context)
{
	if (auto WidgetClass = Cast<UWidgetBlueprintGeneratedClass>(Context.GetCurrentlyGeneratedClass()))
	{
		if (WidgetClass->WidgetTree)
		{
			ensure(WidgetClass->WidgetTree->GetOuter() == Context.GetCurrentlyGeneratedClass());
			FEmitDefaultValueHelper::HandleClassSubobject(Context, WidgetClass->WidgetTree, FEmitterLocalContext::EClassSubobjectList::MiscConvertedSubobjects);
		}
		for (auto Anim : WidgetClass->Animations)
		{
			ensure(Anim->GetOuter() == Context.GetCurrentlyGeneratedClass());
			FEmitDefaultValueHelper::HandleClassSubobject(Context, Anim, FEmitterLocalContext::EClassSubobjectList::MiscConvertedSubobjects);
		}
	}
}

void FBackendHelperUMG::EmitWidgetInitializationFunctions(FEmitterLocalContext& Context)
{
	if (auto WidgetClass = Cast<UWidgetBlueprintGeneratedClass>(Context.GetCurrentlyGeneratedClass()))
	{
		const FString CppClassName = FEmitHelper::GetCppName(WidgetClass);

		auto GenerateLocalProperty = [](FEmitterLocalContext& InContext, UProperty* InProperty, const uint8* DataPtr) -> FString
		{
			check(InProperty && DataPtr);
			const FString NativeName = InContext.GenerateUniqueLocalName();
			FStringOutputDevice Target;
			InProperty->ExportCppDeclaration(Target, EExportedDeclaration::Local, nullptr
				, EPropertyExportCPPFlags::CPPF_CustomTypeName | EPropertyExportCPPFlags::CPPF_BlueprintCppBackend | EPropertyExportCPPFlags::CPPF_NoConst | EPropertyExportCPPFlags::CPPF_NoRef
				, true);
			InContext.AddLine(FString::Printf(TEXT("%s %s;"), *Target, *NativeName));
			FEmitDefaultValueHelper::InnerGenerate(InContext, InProperty, NativeName, DataPtr, nullptr, true);
			return NativeName;
		};

		{	// GetSlotNames
			Context.AddLine(FString::Printf(TEXT("void %s::GetSlotNames(TArray<FName>& SlotNames) const"), *CppClassName));
			Context.AddLine(TEXT("{"));
			Context.IncreaseIndent();

			const FString LocalNativeName = GenerateLocalProperty(Context, FindFieldChecked<UArrayProperty>(UWidgetBlueprintGeneratedClass::StaticClass(), TEXT("NamedSlots")), reinterpret_cast<const uint8*>(&WidgetClass->NamedSlots));
			Context.AddLine(FString::Printf(TEXT("SlotNames.Append(%s);"), *LocalNativeName));

			Context.DecreaseIndent();
			Context.AddLine(TEXT("}"));
		}

		{	// CustomNativeInitilize
			Context.AddLine(FString::Printf(TEXT("void %s::CustomNativeInitilize()"), *CppClassName));
			Context.AddLine(TEXT("{"));
			Context.IncreaseIndent();

			const FString WidgetTreeStr = Context.FindGloballyMappedObject(WidgetClass->WidgetTree, true);
			ensure(!WidgetTreeStr.IsEmpty());
			const FString AnimationsArrayNativeName = GenerateLocalProperty(Context, FindFieldChecked<UArrayProperty>(UWidgetBlueprintGeneratedClass::StaticClass(), TEXT("Animations")), reinterpret_cast<const uint8*>(&WidgetClass->Animations));
			const FString BindingsArrayNativeName = GenerateLocalProperty(Context, FindFieldChecked<UArrayProperty>(UWidgetBlueprintGeneratedClass::StaticClass(), TEXT("Bindings")), reinterpret_cast<const uint8*>(&WidgetClass->Bindings));
					
			Context.AddLine(FString::Printf(TEXT("UWidgetBlueprintGeneratedClass::InitializeWidgetStatic(this, GetClass(), %s, %s, %s, %s, %s, nullptr);")
				, WidgetClass->bCanEverTick ? TEXT("true") : TEXT("false")
				, WidgetClass->bCanEverPaint ? TEXT("true") : TEXT("false")
				, *WidgetTreeStr
				, *AnimationsArrayNativeName
				, *BindingsArrayNativeName));

			Context.DecreaseIndent();
			Context.AddLine(TEXT("}"));
		}

		// PreSave
		Context.AddLine(FString::Printf(TEXT("void %s::PreSave()"), *CppClassName));
		Context.AddLine(TEXT("{"));
		Context.IncreaseIndent();
		Context.AddLine(TEXT("Super::PreSave();"));
		Context.AddLine(TEXT("TArray<FName> LocalNamedSlots;"));
		Context.AddLine(TEXT("GetSlotNames(LocalNamedSlots);"));
		Context.AddLine(TEXT("RemoveObsoleteBindings(LocalNamedSlots);"));
		Context.DecreaseIndent();
		Context.AddLine(TEXT("}"));
	}
}

