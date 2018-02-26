// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "PyGenUtil.h"
#include "PyUtil.h"
#include "PyWrapperBase.h"
#include "Internationalization/BreakIterator.h"
#include "UObject/UnrealType.h"
#include "UObject/Class.h"
#include "UObject/EnumProperty.h"
#include "UObject/Package.h"
#include "UObject/TextProperty.h"
#include "UObject/UnrealType.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/UserDefinedStruct.h"
#include "Engine/UserDefinedEnum.h"

#if WITH_PYTHON

namespace PyGenUtil
{

const FName ScriptNameMetaDataKey = TEXT("ScriptName");
const FName ScriptNoExportMetaDataKey = TEXT("ScriptNoExport");
const FName BlueprintTypeMetaDataKey = TEXT("BlueprintType");
const FName NotBlueprintTypeMetaDataKey = TEXT("NotBlueprintType");
const FName BlueprintSpawnableComponentMetaDataKey = TEXT("BlueprintSpawnableComponent");
const FName BlueprintGetterMetaDataKey = TEXT("BlueprintGetter");
const FName BlueprintSetterMetaDataKey = TEXT("BlueprintSetter");
const TCHAR* HiddenMetaDataKey = TEXT("Hidden");


TSharedPtr<IBreakIterator> NameBreakIterator;


void FGeneratedWrappedMethod::ToPython(FPyMethodWithClosureDef& OutPyMethod) const
{
	OutPyMethod.MethodName = MethodName.GetData();
	OutPyMethod.MethodDoc = MethodDoc.GetData();
	OutPyMethod.MethodCallback = MethodCallback;
	OutPyMethod.MethodFlags = MethodFlags;
	OutPyMethod.MethodClosure = (void*)this;
}


void FGeneratedWrappedGetSet::ToPython(PyGetSetDef& OutPyGetSet) const
{
	OutPyGetSet.name = (char*)GetSetName.GetData();
	OutPyGetSet.doc = (char*)GetSetDoc.GetData();
	OutPyGetSet.get = GetCallback;
	OutPyGetSet.set = SetCallback;
	OutPyGetSet.closure = (void*)this;
}


FGeneratedWrappedPropertyDoc::FGeneratedWrappedPropertyDoc(const UProperty* InProp)
{
	PythonPropName = PyGenUtil::GetPropertyPythonName(InProp);

	const FString PropTooltip = PyGenUtil::GetFieldTooltip(InProp);
	DocString = PyGenUtil::PythonizePropertyTooltip(PropTooltip, InProp);
	EditorDocString = PyGenUtil::PythonizePropertyTooltip(PropTooltip, InProp, CPF_EditConst);
}

bool FGeneratedWrappedPropertyDoc::SortPredicate(const FGeneratedWrappedPropertyDoc& InOne, const FGeneratedWrappedPropertyDoc& InTwo)
{
	return InOne.PythonPropName < InTwo.PythonPropName;
}

FString FGeneratedWrappedPropertyDoc::BuildDocString(const TArray<FGeneratedWrappedPropertyDoc>& InDocs, const bool bEditorVariant)
{
	FString Str;
	AppendDocString(InDocs, Str, bEditorVariant);
	return Str;
}

void FGeneratedWrappedPropertyDoc::AppendDocString(const TArray<FGeneratedWrappedPropertyDoc>& InDocs, FString& OutStr, const bool bEditorVariant)
{
	if (!InDocs.Num())
	{
		return;
	}

	if (!OutStr.IsEmpty())
	{
		if (OutStr[OutStr.Len() - 1] != TEXT('\n'))
		{
			OutStr += TEXT('\n');
		}
		OutStr += TEXT("\n----------------------------------------------------------------------\n");
	}

	OutStr += TEXT("Editor Properties: (see get_editor_property/set_editor_property)\n");
	for (const FGeneratedWrappedPropertyDoc& Doc : InDocs)
	{
		TArray<FString> DocStringLines;
		(bEditorVariant ? Doc.EditorDocString : Doc.DocString).ParseIntoArrayLines(DocStringLines, /*bCullEmpty*/false);

		OutStr += TEXT('\n');
		OutStr += Doc.PythonPropName;
		for (const FString& DocStringLine : DocStringLines)
		{
			OutStr += TEXT("\n    ");
			OutStr += DocStringLine;
		}
		OutStr += TEXT('\n');
	}
	OutStr += TEXT("\n----------------------------------------------------------------------");
}


bool FGeneratedWrappedType::Finalize()
{
	PyType.tp_name = TypeName.GetData();
	PyType.tp_doc = TypeDoc.GetData();

	PyMethods.Reserve(TypeMethods.Num() + 1);
	for (const FGeneratedWrappedMethod& TypeMethod : TypeMethods)
	{
		FPyMethodWithClosureDef& PyMethod = PyMethods[PyMethods.AddZeroed()];
		TypeMethod.ToPython(PyMethod);
	}
	PyMethods.AddZeroed(); // null terminator
	
	PyMethods.Reserve(PyGetSets.Num() + 1);
	for (const FGeneratedWrappedGetSet& TypeGetSet : TypeGetSets)
	{
		PyGetSetDef& PyGetSet = PyGetSets[PyGetSets.AddZeroed()];
		TypeGetSet.ToPython(PyGetSet);
	}
	PyGetSets.AddZeroed(); // null terminator
	PyType.tp_getset = PyGetSets.GetData();

	if (PyType_Ready(&PyType) == 0)
	{
		FPyMethodWithClosureDef::AddMethods(PyMethods.GetData(), &PyType);
		FPyWrapperBaseMetaData::SetMetaData(&PyType, MetaData.Get());
		return true;
	}

	return false;
}


FUTF8Buffer TCHARToUTF8Buffer(const TCHAR* InStr)
{
	auto ToUTF8Buffer = [](const char* InUTF8Str)
	{
		int32 UTF8StrLen = 0;
		while (InUTF8Str[UTF8StrLen++] != 0) {} // Count includes the null terminator

		FUTF8Buffer UTF8Buffer;
		UTF8Buffer.Append(InUTF8Str, UTF8StrLen);
		return UTF8Buffer;
	};

	return ToUTF8Buffer(TCHAR_TO_UTF8(InStr));
}

PyObject* GetPostInitFunc(PyTypeObject* InPyType)
{
	FPyObjectPtr PostInitFunc = FPyObjectPtr::StealReference(PyObject_GetAttrString((PyObject*)InPyType, PostInitFuncName));
	if (!PostInitFunc)
	{
		PyUtil::SetPythonError(PyExc_TypeError, InPyType, *FString::Printf(TEXT("Python type has no '%s' function"), UTF8_TO_TCHAR(PostInitFuncName)));
		return nullptr;
	}

	if (!PyCallable_Check(PostInitFunc))
	{
		PyUtil::SetPythonError(PyExc_TypeError, InPyType, *FString::Printf(TEXT("Python type attribute '%s' is not callable"), UTF8_TO_TCHAR(PostInitFuncName)));
		return nullptr;
	}

	// Only test arguments for actual functions and methods (the base type exposed from C will be a 'method_descriptor')
	if (PyFunction_Check(PostInitFunc) || PyMethod_Check(PostInitFunc))
	{
		TArray<FString> FuncArgNames;
		if (!PyUtil::InspectFunctionArgs(PostInitFunc, FuncArgNames))
		{
			PyUtil::SetPythonError(PyExc_Exception, InPyType, *FString::Printf(TEXT("Failed to inspect the arguments for '%s'"), UTF8_TO_TCHAR(PostInitFuncName)));
			return nullptr;
		}
		if (FuncArgNames.Num() != 1)
		{
			PyUtil::SetPythonError(PyExc_TypeError, InPyType, *FString::Printf(TEXT("'%s' must take a single parameter ('self')"), UTF8_TO_TCHAR(PostInitFuncName)));
			return nullptr;
		}
	}

	return PostInitFunc.Release();
}

void AddStructInitParam(const TCHAR* InUnrealPropName, const TCHAR* InPythonAttrName, TArray<FGeneratedWrappedMethodParameter>& OutInitParams)
{
	FGeneratedWrappedMethodParameter& InitParam = OutInitParams[OutInitParams.AddDefaulted()];
	InitParam.ParamName = TCHARToUTF8Buffer(InPythonAttrName);
	InitParam.ParamPropName = InUnrealPropName;
	InitParam.ParamDefaultValue = FString();
}

bool ParseMethodParameters(PyObject* InArgs, PyObject* InKwds, const TArray<FGeneratedWrappedMethodParameter>& InParamDef, const char* InPyMethodName, TArray<PyObject*>& OutPyParams)
{
	if (!InArgs || !PyTuple_Check(InArgs) || (InKwds && !PyDict_Check(InKwds)) || !InPyMethodName)
	{
		PyErr_BadInternalCall();
		return false;
	}

	const Py_ssize_t NumArgs = PyTuple_GET_SIZE(InArgs);
	const Py_ssize_t NumKeywords = InKwds ? PyDict_Size(InKwds) : 0;
	if (NumArgs + NumKeywords > InParamDef.Num())
	{
		PyErr_Format(PyExc_TypeError, "%s() takes at most %d argument%s (%d given)", InPyMethodName, InParamDef.Num(), (InParamDef.Num() == 1 ? "" : "s"), (int32)(NumArgs + NumKeywords));
		return false;
	}

	// Parse both keyword and index args in the same loop (favor keywords, fallback to index)
	Py_ssize_t RemainingKeywords = NumKeywords;
	for (int32 Index = 0; Index < InParamDef.Num(); ++Index)
	{
		const FGeneratedWrappedMethodParameter& ParamDef = InParamDef[Index];

		PyObject* ParsedArg = nullptr;
		if (RemainingKeywords > 0)
		{
			ParsedArg = PyDict_GetItemString(InKwds, ParamDef.ParamName.GetData());
		}

		if (ParsedArg)
		{
			--RemainingKeywords;
			if (Index < NumArgs)
			{
				PyErr_Format(PyExc_TypeError, "Argument given by name ('%s') and position (%d)", ParamDef.ParamName.GetData(), Index + 1);
				return false;
			}
		}
		else if (RemainingKeywords > 0 && PyErr_Occurred())
		{
			return false;
		}
		else if (Index < NumArgs)
		{
			ParsedArg = PyTuple_GET_ITEM(InArgs, Index);
		}

		if (ParsedArg || ParamDef.ParamDefaultValue.IsSet())
		{
			OutPyParams.Add(ParsedArg);
			continue;
		}

		PyErr_Format(PyExc_TypeError, "Required argument '%s' (pos %d) not found", ParamDef.ParamName.GetData(), Index + 1);
		return false;
	}

	// Report any extra keyword args
	if (RemainingKeywords > 0)
	{
		PyObject* Key = nullptr;
		PyObject* Value = nullptr;
		Py_ssize_t Index = 0;
		while (PyDict_Next(InKwds, &Index, &Key, &Value))
		{
			const FUTF8Buffer Keyword = TCHARToUTF8Buffer(*PyUtil::PyObjectToUEString(Key));
			const bool bIsExpected = InParamDef.ContainsByPredicate([&Keyword](const FGeneratedWrappedMethodParameter& ParamDef)
			{
				return FCStringAnsi::Strcmp(Keyword.GetData(), ParamDef.ParamName.GetData()) == 0;
			});

			if (!bIsExpected)
			{
				PyErr_Format(PyExc_TypeError, "'%s' is an invalid keyword argument for this function", Keyword.GetData());
				return false;
			}
		}
	}

	return true;
}

bool IsBlueprintExposedClass(const UClass* InClass)
{
	for (const UClass* ParentClass = InClass; ParentClass; ParentClass = ParentClass->GetSuperClass())
	{
		if (ParentClass->GetBoolMetaData(BlueprintTypeMetaDataKey) || ParentClass->HasMetaData(BlueprintSpawnableComponentMetaDataKey))
		{
			return true;
		}

		if (ParentClass->GetBoolMetaData(NotBlueprintTypeMetaDataKey))
		{
			return false;
		}
	}

	return false;
}

bool IsBlueprintExposedStruct(const UStruct* InStruct)
{
	for (const UStruct* ParentStruct = InStruct; ParentStruct; ParentStruct = ParentStruct->GetSuperStruct())
	{
		if (ParentStruct->GetBoolMetaData(BlueprintTypeMetaDataKey))
		{
			return true;
		}

		if (ParentStruct->GetBoolMetaData(NotBlueprintTypeMetaDataKey))
		{
			return false;
		}
	}

	return false;
}

bool IsBlueprintExposedEnum(const UEnum* InEnum)
{
	if (InEnum->GetBoolMetaData(BlueprintTypeMetaDataKey))
	{
		return true;
	}

	if (InEnum->GetBoolMetaData(NotBlueprintTypeMetaDataKey))
	{
		return false;
	}

	return false;
}

bool IsBlueprintExposedEnumEntry(const UEnum* InEnum, int32 InEnumEntryIndex)
{
	return !InEnum->HasMetaData(HiddenMetaDataKey, InEnumEntryIndex);
}

bool IsBlueprintExposedProperty(const UProperty* InProp)
{
	return InProp->HasAnyPropertyFlags(CPF_BlueprintVisible);
}

bool IsBlueprintExposedFunction(const UFunction* InFunc)
{
	return InFunc->HasAnyFunctionFlags(FUNC_BlueprintCallable | FUNC_BlueprintEvent) && !InFunc->HasMetaData(BlueprintGetterMetaDataKey) && !InFunc->HasMetaData(BlueprintSetterMetaDataKey);
}

bool IsBlueprintExposedField(const UField* InField)
{
	if (const UProperty* Prop = Cast<const UProperty>(InField))
	{
		return IsBlueprintExposedProperty(Prop);
	}

	if (const UFunction* Func = Cast<const UFunction>(InField))
	{
		return IsBlueprintExposedFunction(Func);
	}

	return false;
}

bool HasBlueprintExposedFields(const UStruct* InStruct)
{
	for (TFieldIterator<const UField> FieldIt(InStruct); FieldIt; ++FieldIt)
	{
		if (IsBlueprintExposedField(*FieldIt))
		{
			return true;
		}
	}

	return false;
}

bool IsBlueprintGeneratedClass(const UClass* InClass)
{
	// Need to use IsA rather than IsChildOf since we want to test the type of InClass itself *NOT* the class instance represented by InClass
	const UObject* ClassObject = InClass;
	return ClassObject->IsA<UBlueprintGeneratedClass>();
}

bool IsBlueprintGeneratedStruct(const UStruct* InStruct)
{
	return InStruct->IsA<UUserDefinedStruct>();
}

bool IsBlueprintGeneratedEnum(const UEnum* InEnum)
{
	return InEnum->IsA<UUserDefinedEnum>();
}

bool ShouldExportClass(const UClass* InClass)
{
	return IsBlueprintExposedClass(InClass) || HasBlueprintExposedFields(InClass);
}

bool ShouldExportStruct(const UStruct* InStruct)
{
	return IsBlueprintExposedStruct(InStruct) || HasBlueprintExposedFields(InStruct);
}

bool ShouldExportEnum(const UEnum* InEnum)
{
	return IsBlueprintExposedEnum(InEnum);
}

bool ShouldExportEnumEntry(const UEnum* InEnum, int32 InEnumEntryIndex)
{
	return IsBlueprintExposedEnumEntry(InEnum, InEnumEntryIndex);
}

bool ShouldExportProperty(const UProperty* InProp)
{
	const bool bCanScriptExport = !InProp->HasMetaData(ScriptNoExportMetaDataKey);
	return bCanScriptExport && IsBlueprintExposedProperty(InProp);
}

bool ShouldExportEditorOnlyProperty(const UProperty* InProp)
{
	const bool bCanScriptExport = !InProp->HasMetaData(ScriptNoExportMetaDataKey);
	return bCanScriptExport && GIsEditor && InProp->HasAnyPropertyFlags(CPF_Edit);
}

bool ShouldExportFunction(const UFunction* InFunc)
{
	const bool bCanScriptExport = !InFunc->HasMetaData(ScriptNoExportMetaDataKey);
	return bCanScriptExport && IsBlueprintExposedFunction(InFunc);
}

FString PythonizeName(const FString& InName, const EPythonizeNameCase InNameCase)
{
	FString PythonizedName;
	PythonizedName.Reserve(InName.Len() + 10);

	if (!NameBreakIterator.IsValid())
	{
		NameBreakIterator = FBreakIterator::CreateCamelCaseBreakIterator();
	}

	NameBreakIterator->SetString(InName);
	for (int32 PrevBreak = 0, NameBreak = NameBreakIterator->MoveToNext(); NameBreak != INDEX_NONE; NameBreak = NameBreakIterator->MoveToNext())
	{
		const int32 OrigPythonizedNameLen = PythonizedName.Len();

		// Append an underscore if this was a break between two parts of the identifier, *and* the previous character isn't already an underscore
		if (OrigPythonizedNameLen > 0 && PythonizedName[OrigPythonizedNameLen - 1] != TEXT('_'))
		{
			PythonizedName += TEXT('_');
		}

		// Append this part of the identifier
		PythonizedName.AppendChars(&InName[PrevBreak], NameBreak - PrevBreak);

		// Remove any trailing underscores in the last part of the identifier
		while (PythonizedName.Len() > OrigPythonizedNameLen)
		{
			const int32 CharIndex = PythonizedName.Len() - 1;
			if (PythonizedName[CharIndex] != TEXT('_'))
			{
				break;
			}
			PythonizedName.RemoveAt(CharIndex, 1, false);
		}

		PrevBreak = NameBreak;
	}
	NameBreakIterator->ClearString();

	if (InNameCase == EPythonizeNameCase::Lower)
	{
		PythonizedName.ToLowerInline();
	}
	else if (InNameCase == EPythonizeNameCase::Upper)
	{
		PythonizedName.ToUpperInline();
	}

	return PythonizedName;
}

FString PythonizePropertyName(const FString& InName, const EPythonizeNameCase InNameCase)
{
	int32 NameOffset = 0;

	for (;;)
	{
		// Strip the "b" prefix from bool names
		if (InName.Len() - NameOffset >= 2 && InName[NameOffset] == TEXT('b') && FChar::IsUpper(InName[NameOffset + 1]))
		{
			NameOffset += 1;
			continue;
		}

		// Strip the "In" prefix from names
		if (InName.Len() - NameOffset >= 3 && InName[NameOffset] == TEXT('I') && InName[NameOffset + 1] == TEXT('n') && FChar::IsUpper(InName[NameOffset + 2]))
		{
			NameOffset += 2;
			continue;
		}

		// Strip the "Out" prefix from names
		if (InName.Len() - NameOffset >= 4 && InName[NameOffset] == TEXT('O') && InName[NameOffset + 1] == TEXT('u') && InName[NameOffset + 2] == TEXT('t') && FChar::IsUpper(InName[NameOffset + 3]))
		{
			NameOffset += 3;
			continue;
		}

		// Nothing more to strip
		break;
	}

	return PythonizeName(NameOffset ? InName.RightChop(NameOffset) : InName, InNameCase);
}

FString PythonizePropertyTooltip(const FString& InTooltip, const UProperty* InProp, const uint64 InReadOnlyFlags)
{
	return PythonizeTooltip(InTooltip, FPythonizeTooltipContext(InProp, nullptr, InReadOnlyFlags));
}

FString PythonizeFunctionTooltip(const FString& InTooltip, const UStruct* InParamsStruct)
{
	return PythonizeTooltip(InTooltip, FPythonizeTooltipContext(nullptr, InParamsStruct));
}

FString PythonizeTooltip(const FString& InTooltip, const FPythonizeTooltipContext& InContext)
{
	FString PythonizedTooltip;
	PythonizedTooltip.Reserve(InTooltip.Len());

	int32 TooltipIndex = 0;
	const int32 TooltipLen = InTooltip.Len();

	TArray<TTuple<FString, FString>, TInlineAllocator<4>> ParsedMiscTokens;
	TArray<TTuple<FName, FString>, TInlineAllocator<8>> ParsedParamTokens;
	FString ReturnToken;

	auto SkipToNextToken = [&InTooltip, &TooltipIndex, &TooltipLen]()
	{
		while (TooltipIndex < TooltipLen && (FChar::IsWhitespace(InTooltip[TooltipIndex]) || InTooltip[TooltipIndex] == TEXT('-')))
		{
			++TooltipIndex;
		}
	};

	auto ParseSimpleToken = [&InTooltip, &TooltipIndex, &TooltipLen](FString& OutToken)
	{
		while (TooltipIndex < TooltipLen && !FChar::IsWhitespace(InTooltip[TooltipIndex]))
		{
			OutToken += InTooltip[TooltipIndex++];
		}
	};

	auto ParseComplexToken = [&InTooltip, &TooltipIndex, &TooltipLen](FString& OutToken)
	{
		while (TooltipIndex < TooltipLen && InTooltip[TooltipIndex] != TEXT('@'))
		{
			// Convert a new-line within a token to a space
			if (FChar::IsLinebreak(InTooltip[TooltipIndex]))
			{
				while (TooltipIndex < TooltipLen && FChar::IsLinebreak(InTooltip[TooltipIndex]))
				{
					++TooltipIndex;
				}

				while (TooltipIndex < TooltipLen && FChar::IsWhitespace(InTooltip[TooltipIndex]))
				{
					++TooltipIndex;
				}

				OutToken += TEXT(' ');
			}

			// Sanity check in case the first character after the new-line is @
			if (TooltipIndex < TooltipLen && InTooltip[TooltipIndex] != TEXT('@'))
			{
				OutToken += InTooltip[TooltipIndex++];
			}
		}
		OutToken.TrimEndInline();
	};

	// Append the property type (if given)
	if (InContext.Prop)
	{
		PythonizedTooltip += TEXT("type: ");
		AppendPropertyPythonType(InContext.Prop, PythonizedTooltip, /*bIncludeReadWriteState*/true, InContext.ReadOnlyFlags);
		PythonizedTooltip += TEXT('\n');
	}

	// Parse the tooltip for its tokens and values (basic content goes directly into PythonizedTooltip)
	for (; TooltipIndex < TooltipLen;)
	{
		if (InTooltip[TooltipIndex] == TEXT('@'))
		{
			++TooltipIndex; // Walk over the @
			if (InTooltip[TooltipIndex] == TEXT('@'))
			{
				// Literal @ character
				PythonizedTooltip += TEXT('@');
				continue;
			}

			// Parse out the token name
			FString TokenName;
			SkipToNextToken();
			ParseSimpleToken(TokenName);

			if (TokenName == TEXT("param"))
			{
				// Parse out the parameter name
				FString ParamName;
				SkipToNextToken();
				ParseSimpleToken(ParamName);

				// Parse out the parameter comment
				FString ParamComment;
				SkipToNextToken();
				ParseComplexToken(ParamComment);

				ParsedParamTokens.Add(MakeTuple(FName(*ParamName), MoveTemp(ParamComment)));
			}
			else if (TokenName == TEXT("return") || TokenName == TEXT("returns"))
			{
				// Parse out the return value token
				SkipToNextToken();
				ParseComplexToken(ReturnToken);
			}
			else
			{
				// Parse out the token value
				FString TokenValue;
				SkipToNextToken();
				ParseComplexToken(TokenValue);

				ParsedMiscTokens.Add(MakeTuple(MoveTemp(TokenName), MoveTemp(TokenValue)));
			}
		}
		else
		{
			// Convert duplicate new-lines to a single new-line
			if (FChar::IsLinebreak(InTooltip[TooltipIndex]))
			{
				while (TooltipIndex < TooltipLen && FChar::IsLinebreak(InTooltip[TooltipIndex]))
				{
					++TooltipIndex;
				}
				PythonizedTooltip += TEXT('\n');
			}
			else
			{
				// Normal character
				PythonizedTooltip += InTooltip[TooltipIndex++];
			}
		}
	}

	PythonizedTooltip.TrimEndInline();

	// Process the misc tokens into PythonizedTooltip
	for (const auto& MiscTokenPair : ParsedMiscTokens)
	{
		PythonizedTooltip += TEXT('\n');
		PythonizedTooltip += MiscTokenPair.Key;
		PythonizedTooltip += TEXT(": ");
		PythonizedTooltip += MiscTokenPair.Value;
	}

	// Process the param tokens into PythonizedTooltip
	auto AppendParamTypeDoc = [&PythonizedTooltip](const UProperty* InParamProp)
	{
		PythonizedTooltip += TEXT(" (");
		AppendPropertyPythonType(InParamProp, PythonizedTooltip);
		PythonizedTooltip += TEXT(')');
	};
	for (const auto& ParamTokenPair : ParsedParamTokens)
	{
		PythonizedTooltip += TEXT('\n');
		PythonizedTooltip += TEXT("param: ");
		PythonizedTooltip += PythonizePropertyName(ParamTokenPair.Key.ToString(), EPythonizeNameCase::Lower);

		if (InContext.ParamsStruct)
		{
			if (const UProperty* ParamProp = InContext.ParamsStruct->FindPropertyByName(ParamTokenPair.Key))
			{
				AppendParamTypeDoc(ParamProp);
			}
		}

		if (!ParamTokenPair.Value.IsEmpty())
		{
			PythonizedTooltip += TEXT(" -- ");
			PythonizedTooltip += ParamTokenPair.Value;
		}
	}
	if (InContext.ParamsStruct)
	{
		for (TFieldIterator<const UProperty> ParamIt(InContext.ParamsStruct); ParamIt; ++ParamIt)
		{
			const UProperty* ParamProp = *ParamIt;

			const bool bHasProcessedParamProp = ParsedParamTokens.ContainsByPredicate([&ParamProp](const TTuple<FName, FString>& ParamTokenPair)
			{
				return ParamTokenPair.Key == ParamProp->GetFName();
			});
			
			if (bHasProcessedParamProp)
			{
				continue;
			}

			PythonizedTooltip += TEXT('\n');
			PythonizedTooltip += TEXT("param: ");
			PythonizedTooltip += PythonizePropertyName(ParamProp->GetName(), EPythonizeNameCase::Lower);

			AppendParamTypeDoc(ParamProp);
		}
	}

	// Process the return token into PythonizedTooltip
	if (!ReturnToken.IsEmpty())
	{
		PythonizedTooltip += TEXT('\n');
		PythonizedTooltip += TEXT("return: ");
		PythonizedTooltip += ReturnToken;
	}

	PythonizedTooltip.TrimEndInline();

	return PythonizedTooltip;
}

FString GetFieldModule(const UField* InField)
{
	// todo: should have meta-data on the type that can override this for scripting
	UPackage* ScriptPackage = InField->GetOutermost();
	
	const FString PackageName = ScriptPackage->GetName();
	if (PackageName.StartsWith(TEXT("/Script/")))
	{
		return PackageName.RightChop(8); // Chop "/Script/" from the name
	}

	check(PackageName[0] == TEXT('/'));
	int32 RootNameEnd = 1;
	for (; PackageName[RootNameEnd] != TEXT('/'); ++RootNameEnd) {}
	return PackageName.Mid(1, RootNameEnd - 1);
}

FString GetModulePythonName(const FName InModuleName, const bool bIncludePrefix)
{
	// Some modules are mapped to others in Python
	static const FName PythonModuleMappings[][2] = {
		{ TEXT("CoreUObject"), TEXT("Core") },
		{ TEXT("SlateCore"), TEXT("Slate") },
		{ TEXT("UnrealEd"), TEXT("Editor") },
		{ TEXT("PythonScriptPlugin"), TEXT("Python") },
	};

	FName MappedModuleName = InModuleName;
	for (const auto& PythonModuleMapping : PythonModuleMappings)
	{
		if (InModuleName == PythonModuleMapping[0])
		{
			MappedModuleName = PythonModuleMapping[1];
			break;
		}
	}

	const FString ModulePythonName = MappedModuleName.ToString().ToLower();
	return bIncludePrefix ? FString::Printf(TEXT("_unreal_%s"), *ModulePythonName) : ModulePythonName;
}

FString GetClassPythonName(const UClass* InClass)
{
	FString ClassName = InClass->GetMetaData(ScriptNameMetaDataKey);
	if (ClassName.IsEmpty())
	{
		ClassName = InClass->GetName();
	}
	return ClassName;
}

FString GetStructPythonName(const UStruct* InStruct)
{
	FString StructName = InStruct->GetMetaData(ScriptNameMetaDataKey);
	if (StructName.IsEmpty())
	{
		StructName = InStruct->GetName();
	}
	return StructName;
}

FString GetEnumPythonName(const UEnum* InEnum)
{
	FString EnumName = InEnum->UField::GetMetaData(ScriptNameMetaDataKey);
	if (EnumName.IsEmpty())
	{
		EnumName = InEnum->GetName();

		// Strip the "E" prefix from enum names
		if (EnumName.Len() >= 2 && EnumName[0] == TEXT('E') && FChar::IsUpper(EnumName[1]))
		{
			EnumName.RemoveAt(0, 1, /*bAllowShrinking*/false);
		}
	}
	return EnumName;
}

FString GetDelegatePythonName(const UFunction* InDelegateSignature)
{
	return InDelegateSignature->GetName().LeftChop(19); // Trim the "__DelegateSignature" suffix from the name
}

FString GetFunctionPythonName(const UFunction* InFunc)
{
	FString FuncName = InFunc->GetMetaData(ScriptNameMetaDataKey);
	if (FuncName.IsEmpty())
	{
		FuncName = InFunc->GetName();
	}
	return PythonizeName(FuncName, EPythonizeNameCase::Lower);
}

FString GetPropertyPythonName(const UProperty* InProp)
{
	FString PropName = InProp->GetMetaData(ScriptNameMetaDataKey);
	if (PropName.IsEmpty())
	{
		PropName = InProp->GetName();
	}
	return PythonizePropertyName(PropName, EPythonizeNameCase::Lower);
}

FString GetPropertyTypePythonName(const UProperty* InProp)
{
#define GET_PROPERTY_TYPE(TYPE, VALUE)				\
		if (Cast<const TYPE>(InProp) != nullptr)	\
		{											\
			return VALUE;							\
		}

	GET_PROPERTY_TYPE(UBoolProperty, TEXT("bool"))
	GET_PROPERTY_TYPE(UInt8Property, TEXT("int8"))
	GET_PROPERTY_TYPE(UInt16Property, TEXT("int16"))
	GET_PROPERTY_TYPE(UUInt16Property, TEXT("uint16"))
	GET_PROPERTY_TYPE(UIntProperty, TEXT("int32"))
	GET_PROPERTY_TYPE(UUInt32Property, TEXT("uint32"))
	GET_PROPERTY_TYPE(UInt64Property, TEXT("int64"))
	GET_PROPERTY_TYPE(UUInt64Property, TEXT("uint64"))
	GET_PROPERTY_TYPE(UFloatProperty, TEXT("float"))
	GET_PROPERTY_TYPE(UDoubleProperty, TEXT("double"))
	GET_PROPERTY_TYPE(UStrProperty, TEXT("String"))
	GET_PROPERTY_TYPE(UNameProperty, TEXT("Name"))
	GET_PROPERTY_TYPE(UTextProperty, TEXT("Text"))
	if (const UByteProperty* ByteProp = Cast<const UByteProperty>(InProp))
	{
		if (ByteProp->Enum)
		{
			return GetEnumPythonName(ByteProp->Enum);
		}
		else
		{
			return TEXT("uint8");
		}
	}
	if (const UEnumProperty* EnumProp = Cast<const UEnumProperty>(InProp))
	{
		return GetEnumPythonName(EnumProp->GetEnum());
	}
	if (const UClassProperty* ClassProp = Cast<const UClassProperty>(InProp))
	{
		return FString::Printf(TEXT("type(%s)"), *GetClassPythonName(ClassProp->PropertyClass));
	}
	if (const UObjectPropertyBase* ObjProp = Cast<const UObjectPropertyBase>(InProp))
	{
		return GetClassPythonName(ObjProp->PropertyClass);
	}
	if (const UInterfaceProperty* InterfaceProp = Cast<const UInterfaceProperty>(InProp))
	{
		return GetClassPythonName(InterfaceProp->InterfaceClass);
	}
	if (const UStructProperty* StructProp = Cast<const UStructProperty>(InProp))
	{
		return GetStructPythonName(StructProp->Struct);
	}
	if (const UDelegateProperty* DelegateProp = Cast<const UDelegateProperty>(InProp))
	{
		return GetDelegatePythonName(DelegateProp->SignatureFunction);
	}
	if (const UMulticastDelegateProperty* MulticastDelegateProp = Cast<const UMulticastDelegateProperty>(InProp))
	{
		return GetDelegatePythonName(MulticastDelegateProp->SignatureFunction);
	}
	if (const UArrayProperty* ArrayProperty = Cast<const UArrayProperty>(InProp))
	{
		return FString::Printf(TEXT("Array(%s)"), *GetPropertyTypePythonName(ArrayProperty->Inner));
	}
	if (const USetProperty* SetProperty = Cast<const USetProperty>(InProp))
	{
		return FString::Printf(TEXT("Set(%s)"), *GetPropertyTypePythonName(SetProperty->ElementProp));
	}
	if (const UMapProperty* MapProperty = Cast<const UMapProperty>(InProp))
	{
		return FString::Printf(TEXT("Map(%s, %s)"), *GetPropertyTypePythonName(MapProperty->KeyProp), *GetPropertyTypePythonName(MapProperty->ValueProp));
	}

	return TEXT("'undefined'");

#undef GET_PROPERTY_TYPE
}

FString GetPropertyPythonType(const UProperty* InProp, const bool bIncludeReadWriteState, const uint64 InReadOnlyFlags)
{
	FString RetStr;
	AppendPropertyPythonType(InProp, RetStr, bIncludeReadWriteState, InReadOnlyFlags);
	return RetStr;
}

void AppendPropertyPythonType(const UProperty* InProp, FString& OutStr, const bool bIncludeReadWriteState, const uint64 InReadOnlyFlags)
{
	OutStr += GetPropertyTypePythonName(InProp);

	if (bIncludeReadWriteState)
	{
		OutStr += (InProp->HasAnyPropertyFlags(InReadOnlyFlags) ? TEXT(" [Read-Only]") : TEXT(" [Read-Write]"));
	}
}

FString GetFieldTooltip(const UField* InField)
{
	// We use the source string here as the culture may change while the editor is running, and also because some versions 
	// of Python (<3.4) can't override the default encoding to UTF-8 so produce errors when trying to print the help docs
	return *FTextInspector::GetSourceString(InField->GetToolTipText());
}

FString GetEnumEntryTooltip(const UEnum* InEnum, const int64 InEntryIndex)
{
	// We use the source string here as the culture may change while the editor is running, and also because some versions 
	// of Python (<3.4) can't override the default encoding to UTF-8 so produce errors when trying to print the help docs
	return *FTextInspector::GetSourceString(InEnum->GetToolTipTextByIndex(InEntryIndex));
}

}

#endif	// WITH_PYTHON
