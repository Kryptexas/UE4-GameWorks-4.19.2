// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

//////////////////////////////////////////////////////////////////////////
// FMaterialEditorUtilities

class MATERIALEDITOR_API FMaterialEditorUtilities
{
public:
	/**
	 * Creates a new material expression of the specified class on the material represented by a graph.
	 *
	 * @param	Graph				Graph representing a material.
	 * @param	NewExpressionClass	The type of material expression to add.  Must be a child of UMaterialExpression.
	 * @param	NodePos				Position of the new node.
	 * @param	bAutoSelect			If true, deselect all expressions and select the newly created one.
	 * @param	bAutoAssignResource	If true, assign resources to new expression.
	 */
	static UMaterialExpression* CreateNewMaterialExpression(const class UEdGraph* Graph, UClass* NewExpressionClass, const FVector2D& NodePos, bool bAutoSelect, bool bAutoAssignResource);

	/**
	 * Creates a new material expression comment on the material represented by a graph.
	 *
	 * @param	Graph	Graph to add comment to
	 * @param	NodePos	Position to place new comment at
	 *
	 * @return	UMaterialExpressionComment*	Newly created comment
	 */
	static UMaterialExpressionComment* CreateNewMaterialExpressionComment(const class UEdGraph* Graph, const FVector2D& NodePos);

	/**
	 * Refreshes all material expression previews, regardless of whether or not realtime previews are enabled.
	 *
	 * @param	Graph	Graph representing a material.
	 */
	static void ForceRefreshExpressionPreviews(const class UEdGraph* Graph);

	/**
	 * Add the specified object to the list of selected objects
	 *
	 * @param	Graph	Graph representing a material.
	 * @param	Obj		Object to add to selection.
	 */
	static void AddToSelection(const class UEdGraph* Graph, UMaterialExpression* Expression);

	/**
	 * Disconnects and removes the selected material graph nodes.
	 *
	 * @param	Graph	Graph representing a material.
	 */
	static void DeleteSelectedNodes(const class UEdGraph* Graph);

	/**
	 * Gets the name of the material or material function that we are editing
	 *
	 * @param	Graph	Graph representing a material or material function.
	 */
	static FString GetOriginalObjectName(const class UEdGraph* Graph);

	/**
	 * Re-links the material and updates its representation in the editor
	 *
	 * @param	Graph	Graph representing a material or material function.
	 */
	static void UpdateMaterialAfterGraphChange(const class UEdGraph* Graph);

	/** Can we paste to this graph? */
	static bool CanPasteNodes(const class UEdGraph* Graph);

	/** Perform paste on graph, at location */
	static void  PasteNodesHere(class UEdGraph* Graph, const FVector2D& Location);

	/** Gets the number of selected nodes on this graph */
	static int32 GetNumberOfSelectedNodes(const class UEdGraph* Graph);

	/**
	 * Get all actions for placing material expressions
	 *
	 * @param [in,out]	ActionMenuBuilder	The output menu builder.
	 * @param bMaterialFunction				Whether we're dealing with a Material Function
	 */
	static void GetMaterialExpressionActions(FGraphActionMenuBuilder& ActionMenuBuilder, bool bMaterialFunction);

	/**
	 * Check whether an expression is in the favourites list
	 *
	 * @param	InExpression	The Expression we are checking
	 */
	static bool IsMaterialExpressionInFavorites(UMaterialExpression* InExpression);

	/**
	 * Get a preview material render proxy for an expression
	 *
	 * @param	Graph			Graph representing material
	 * @param	InExpression	Expression we want preview for
	 *
	 * @return	FMaterialRenderProxy*	Preview for this expression or NULL
	 */
	static FMaterialRenderProxy* GetExpressionPreview(const class UEdGraph* Graph, UMaterialExpression* InExpression);

	/**
	 * Updates the material editor search results
	 *
	 * @param	Graph			Graph representing material
	 */
	static void UpdateSearchResults(const class UEdGraph* Graph);

	/////////////////////////////////////////////////////
	// Static functions moved from SMaterialEditorCanvas

	/**
	 * Retrieves all visible parameters within the material.
	 *
	 * @param	Material			The material to retrieve the parameters from.
	 * @param	MaterialInstance	The material instance that contains all parameter overrides.
	 * @param	VisisbleExpressions	The array that will contain the id's of the visible parameter expressions.
	 */
	static void GetVisibleMaterialParameters(const UMaterial *Material, UMaterialInstance *MaterialInstance, TArray<FGuid> &VisibleExpressions);

	/** Returns true if the function or dependent functions contain a static switch expression */
	static bool IsFunctionContainingSwitchExpressions(UMaterialFunction* MaterialFunction);

	/** Finds an input in the passed in array with a matching Id. */
	static const FFunctionExpressionInput* FindInputById(const UMaterialExpressionFunctionInput* InputExpression, const TArray<FFunctionExpressionInput>& Inputs);

	/** 
	* Returns the value for a static switch material expression.
	*
	* @param	MaterialInstance		The material instance that contains all parameter overrides.
	* @param	SwitchValueExpression	The switch expression to find the value for.
	* @param	OutValue				The value for the switch expression.
	* @param	OutExpressionID			The Guid of the expression that is input as the switch value.
	* @param	FunctionInputs			optional An array of FFunctionExpressionInputs when parsing a switch within a function.
	* 
	* @return	Returns true if a value for the switch expression is found, otherwise returns false.
	*/
	static bool GetStaticSwitchExpressionValue(UMaterialInstance* MaterialInstance, UMaterialExpression *SwitchValueExpression, bool& OutValue, FGuid& OutExpressionID, const TArray<FFunctionExpressionInput>* FunctionInputs = NULL);

	/**
	 * Allows access to a material's input by index.
	 *
	 * @param	Material	The material to retrieve an input from.
	 * @param	Index		The index of the desired input.
	 *
	 * @return	Returns the requested input or NULL.
	 */
	static FExpressionInput* GetMaterialInput(UMaterial* Material, int32 Index);

	/**
	 * Checks whether a material's input is visible by index.
	 *
	 * @param	Material	The material to check inputs.
	 * @param	Index		The index of the input to check.
	 *
	 * @return	Returns whether requested input is visible.
	 */
	static bool IsInputVisible(UMaterial* Material, int32 Index);

	/**
	 * Populates the specified material's Expressions array (eg if cooked out or old content).
	 * Also ensures materials and expressions are RF_Transactional for undo/redo support.
	 *
	 * @param	Material	Material we are initializing
	 */
	static void InitExpressions(UMaterial* Material);

private:

	/**
	 * Recursively walks the expression tree and parses the visible expression branches.
	 *
	 * @param	MaterialExpression				The expression to parse.
	 * @param	MaterialInstance				The material instance that contains all parameter overrides.
	 * @param	VisisbleExpressions				The array that will contain the id's of the visible parameter expressions.
	 * @param	FunctionInputs					An array of FFunctionExpressionInput when parsing material functions calls
	 * @param	VisibleFunctionInputExpressions	The array that will contain the id's of visible function inputs when parsing function calls.
	 */
	static void GetVisibleMaterialParametersFromExpression(UMaterialExpression *MaterialExpression, UMaterialInstance *MaterialInstance, TArray<FGuid> &VisibleExpressions, TArray<UMaterialExpression*> &ProcessedExpressions, const TArray<FFunctionExpressionInput>* FunctionInputs = NULL, TArray<FGuid>* VisibleFunctionInputExpressions = NULL);

	/** Get IMaterialEditor for given object, if it exists */
	static TSharedPtr<class IMaterialEditor> GetIMaterialEditorForObject(const UObject* ObjectToFocusOn);

	static void AddMaterialExpressionCategory(FGraphActionMenuBuilder& ActionMenuBuilder, FString CategoryName, TArray<struct FMaterialExpression>* MaterialExpressions, bool bMaterialFunction);

	/** Constructor */
	FMaterialEditorUtilities() {}
};
