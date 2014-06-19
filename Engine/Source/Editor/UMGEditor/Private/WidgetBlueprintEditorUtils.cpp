// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "WidgetBlueprintEditorUtils.h"
#include "Kismet2NameValidators.h"
#include "BlueprintEditorUtils.h"
#include "K2Node_Variable.h"

bool FWidgetBlueprintEditorUtils::RenameWidget(UWidgetBlueprint* Blueprint, const FName& OldName, const FName& NewName)
{
	check(Blueprint);

	bool bRenamed = false;

	TSharedPtr<INameValidatorInterface> NameValidator = MakeShareable(new FKismetNameValidator(Blueprint));

	// NewName should be already validated. But one must make sure that NewTemplateName is also unique.
	const bool bUniqueNameForTemplate = ( EValidatorResult::Ok == NameValidator->IsValid(NewName) );

	const FString NewNameStr = NewName.ToString();
	const FString OldNameStr = OldName.ToString();

	UWidget* Widget = Blueprint->WidgetTree->FindWidget(OldNameStr);
	check(Widget);
	Widget->Rename(*NewNameStr);

	UTimelineTemplate* Template = Blueprint->FindTimelineTemplateByVariableName(NewName);
	if ((Template == NULL) && bUniqueNameForTemplate)
	{
		Template = Blueprint->FindTimelineTemplateByVariableName(OldName);
		if (Template)
		{
			Blueprint->Modify();
			Template->Modify();

			TArray<UK2Node_Variable*> TimelineVarNodes;
			FBlueprintEditorUtils::GetAllNodesOfClass<UK2Node_Variable>(Blueprint, TimelineVarNodes);
			for(int32 It = 0; It < TimelineVarNodes.Num(); It++)
			{
				UK2Node_Variable* TestNode = TimelineVarNodes[It];
				if(TestNode && (OldName == TestNode->GetVarName()))
				{
					UEdGraphPin* TestPin = TestNode->FindPin(OldNameStr);
					if(TestPin && (UTimelineComponent::StaticClass() == TestPin->PinType.PinSubCategoryObject.Get()))
					{
						TestNode->Modify();
						if(TestNode->VariableReference.IsSelfContext())
						{
							TestNode->VariableReference.SetSelfMember(NewName);
						}
						else
						{
							//TODO:
							UClass* ParentClass = TestNode->VariableReference.GetMemberParentClass((UClass*)NULL);
							TestNode->VariableReference.SetExternalMember(NewName, ParentClass);
						}
						TestPin->Modify();
						TestPin->PinName = NewNameStr;
					}
				}
			}

			Blueprint->Timelines.Remove(Template);
			
			UObject* ExistingObject = StaticFindObject(NULL, Template->GetOuter(), *NewName.ToString(), true);
			if (ExistingObject != Template && ExistingObject != NULL)
			{
				ExistingObject->Rename(*MakeUniqueObjectName(ExistingObject->GetOuter(), ExistingObject->GetClass(), ExistingObject->GetFName()).ToString());
			}
			Template->Rename(*NewName.ToString(), Template->GetOuter(), ( Blueprint->bIsRegeneratingOnLoad ? REN_ForceNoResetLoaders : REN_None ));
			Blueprint->Timelines.Add(Template);

			// Validate child blueprints and adjust variable names to avoid a potential name collision
			FBlueprintEditorUtils::ValidateBlueprintChildVariables(Blueprint, NewName);

			// Refresh references and flush editors
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
			bRenamed = true;
		}
	}
	return bRenamed;
}