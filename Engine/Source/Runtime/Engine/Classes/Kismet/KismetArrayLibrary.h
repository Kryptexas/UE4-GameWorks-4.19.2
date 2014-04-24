// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "KismetArrayLibrary.generated.h"

UCLASS(HeaderGroup=KismetLibrary,MinimalAPI)
class UKismetArrayLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()
	/** 
	 *Add item to array
	 *
	 *@param	Array	The array to add item to
	 *@param	Item	The item to add to the array
	*/
	UFUNCTION(BlueprintCallable, CustomThunk, meta=(FriendlyName = "Add", CompactNodeTitle = "ADD", ArrayParm = "TargetArray|ArrayProperty", ArrayTypeDependentParams = "NewItem", AutoCreateRefTerm = "NewItem"), Category="Utilities|Array")
	static int32 Array_Add(const TArray<int32>& TargetArray, const UArrayProperty* ArrayProperty, const int32& NewItem);

	/** 
	 *Append an array to another array
	 *
	 *@param	TargetArray		The array to add the source array to
	 *@param	SourceArray		The array to add to the target array
	*/
	UFUNCTION(BlueprintCallable, CustomThunk, meta=(FriendlyName = "Append Array", CompactNodeTitle = "APPEND", ArrayParm = "TargetArray|TargetArrayProperty,SourceArray|SourceArrayProperty", ArrayTypeDependentParams = "SourceArray"), Category="Utilities|Array")
	static void Array_Append(const TArray<int32>& TargetArray, const UArrayProperty* TargetArrayProperty, const TArray<int32>& SourceArray, const UArrayProperty* SourceArrayProperty);

	/* 
	 *Insert item at the given index into the array.
	 *	
	 *@param	Array	The array to insert into
	 *@param	Item	The item to insert into the array
	 *@param	Int		The index at which to insert the item into the array
	*/
	UFUNCTION(BlueprintCallable, CustomThunk, meta=(FriendlyName = "Insert", CompactNodeTitle = "INSERT", ArrayParm = "TargetArray|ArrayProperty", ArrayTypeDependentParams = "NewItem", AutoCreateRefTerm = "NewItem"), Category="Utilities|Array")
	static void Array_Insert(const TArray<int32>& TargetArray, const UArrayProperty* ArrayProperty, const int32& NewItem, int32 Index);


	/* 
	 *Remove item at the given index from the array.
	 *
	 *@param	Array	The array to remove from
	 *@param	Item	The index into the array to remove from
	*/
	UFUNCTION(BlueprintCallable, CustomThunk, meta=(FriendlyName = "Remove Index", CompactNodeTitle = "REMOVE INDEX", ArrayParm = "TargetArray|ArrayProperty"), Category="Utilities|Array")
	static void Array_Remove(const TArray<int32>& TargetArray, const UArrayProperty* ArrayProperty, int32 IndexToRemove);

	/* 
	 *Remove all instances of item from array.
	 *
	 *@param	Array	The array to remove from
	 *@param	Item	The item to remove from the array
	 *@return bool	Returns True if one or more items were removed
	*/
	UFUNCTION(BlueprintCallable, CustomThunk, meta=(FriendlyName = "Remove Item", CompactNodeTitle = "REMOVE", ArrayParm = "TargetArray|ArrayProperty", ArrayTypeDependentParams = "Item", AutoCreateRefTerm = "Item"), Category="Utilities|Array")
	static bool Array_RemoveItem(const TArray<int32>& TargetArray, const UArrayProperty* ArrayProperty, const int32 &Item);

	/* 
	 *Clear an array, removes all content
	 *
	 *@param	Pin	The array to clear
	*/
	UFUNCTION(BlueprintCallable, CustomThunk, meta=(FriendlyName = "Clear", CompactNodeTitle = "CLEAR", ArrayParm = "TargetArray|ArrayProperty"), Category="Utilities|Array")
	static void Array_Clear(const TArray<int32>& TargetArray, const UArrayProperty* ArrayProperty);

	/* 
	 *Resize Array to specified size. 
	 *	
	 *@param	Array	The array to resize
	 *@param	Int		The new size of the array
	*/
	UFUNCTION(BlueprintCallable, CustomThunk, meta=(FriendlyName = "Resize", CompactNodeTitle = "RESIZE", ArrayParm = "TargetArray|ArrayProperty"), Category="Utilities|Array")
	static void Array_Resize(const TArray<int32>& TargetArray, const UArrayProperty* ArrayProperty, int32 Size);

	/* 
	 *Get the number of items in an array
	 *
	 *@param	Array	The array to get the length of
	 *@return int		Returns the length of the array
	*/
	UFUNCTION(BlueprintPure, CustomThunk, meta=(FriendlyName = "Length", CompactNodeTitle = "LENGTH", ArrayParm = "TargetArray|ArrayProperty", Keywords = "num size count"), Category="Utilities|Array")
	static int32 Array_Length(const TArray<int32>& TargetArray, const UArrayProperty* ArrayProperty);


	/* 
	 *Get the last valid index into an array
	 *	
	 *@param	Array	The array to perform the operation on
	 *@return int		Returns the last valid index of the array
	*/
	UFUNCTION(BlueprintPure, CustomThunk, meta=(FriendlyName = "Last Index", CompactNodeTitle = "LAST INDEX", ArrayParm = "TargetArray|ArrayProperty"), Category="Utilities|Array")
	static int32 Array_LastIndex(const TArray<int32>& TargetArray, const UArrayProperty* ArrayProperty);

	/* 
	 *Given an array and an index, returns the item found at that index
	 *
	 *@param	Array	The array 
	 *@param	int		The index
	 *@return int		Returns the item stored at the index
	*/
	UFUNCTION(BlueprintPure, CustomThunk, meta=(FriendlyName = "Get", CompactNodeTitle = "GET", ArrayParm = "TargetArray|ArrayProperty", ArrayTypeDependentParams = "Item"), Category="Utilities|Array")
	static void Array_Get(const TArray<int32>& TargetArray, const UArrayProperty* ArrayProperty, int32 Index, int32& Item);

	/* 
	 *Given an array and an index, assigns the item to that array element
	 *
	 *@param	Array	The array to perform the operation on
	 *@param	int		The index
	 *@param	Item	The item to assign to the index of the array
	 *@param	bSizeToFit	If true, the array will expand if Index is greater than the current size of the array
	*/
	UFUNCTION(BlueprintCallable, CustomThunk, meta=(FriendlyName = "Set Array Elem", ArrayParm = "TargetArray|ArrayProperty", ArrayTypeDependentParams = "Item", AutoCreateRefTerm = "Item"), Category="Utilities|Array")
	static void Array_Set(const TArray<int32>& TargetArray, const UArrayProperty* ArrayProperty, int32 Index, const int32& Item, bool bSizeToFit);

	/*  
	 *Finds the index of the first instance of the item within the array
	 *	
	 *@param	Array	The array to search for the item
	 *@param	int		The item to look for
	 *@return int		The index the item was found at, or INDEX_NONE if not found
	*/
	UFUNCTION(BlueprintPure, CustomThunk, meta=(FriendlyName = "Find Item", CompactNodeTitle = "FIND", ArrayParm = "TargetArray|ArrayProperty", ArrayTypeDependentParams = "ItemToFind", AutoCreateRefTerm = "ItemToFind"), Category="Utilities|Array")
	static int32 Array_Find(const TArray<int32>& TargetArray, const UArrayProperty* ArrayProperty, const int32& ItemToFind);

	/*  
	 *Returns true if the array contains the given item
	 *
	 *@param	Array	The array to search for the item
	 *@param	item	The item to look for
	 *@return bool	Returns True if the item was found within the array
	*/
	UFUNCTION(BlueprintPure, CustomThunk, meta=(FriendlyName = "Contains Item", CompactNodeTitle = "CONTAINS", ArrayParm = "TargetArray|ArrayProperty", ArrayTypeDependentParams = "ItemToFind", AutoCreateRefTerm = "ItemToFind"), Category="Utilities|Array")
	static bool Array_Contains(const TArray<int32>& TargetArray, const UArrayProperty* ArrayProperty, const int32& ItemToFind);

	/*  
	 *Filter an array based on a Class derived from Actor.  
	 *	
	 *@param	Array	The array to filter from
	 *@param	Actor	The Actor sub-class type that acts as the filter, only objects derived from it will be returned.
	 *@return Array	Returns array containing only those objects which are derived from the class type specified.
	*/
	UFUNCTION(BlueprintCallable, meta=(FriendlyName = "Filter Array"), Category="Utilities|Array")
	static void FilterArray(const TArray<AActor*>& TargetArray, TSubclassOf<class AActor> FilterClass, TArray<AActor*>& FilteredArray);

	/** 
	 *
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, meta=(BlueprintInternalUseOnly = "true", ArrayParm = "Value", ArrayTypeDependentParams="Value"))
	static void SetArrayPropertyByName(UObject* Object, FName PropertyName, const TArray<int32>& Value);

	// Native functions that will be called by the below custom thunk layers, which read off the property address, and call the appropriate native handler
	int32 GenericArray_Add(void* TargetArray, const UArrayProperty* ArrayProp, const int32& NewItem);
	void GenericArray_Append(void* TargetArray, const UArrayProperty* TargetArrayProp, void* SourceArray, const UArrayProperty* SourceArrayProperty);
	void GenericArray_Insert(void* TargetArray, const UArrayProperty* ArrayProp, const int32& NewItem, int32 Index);
	void GenericArray_Remove(void* TargetArray, const UArrayProperty* ArrayProp, int32 IndexToRemove);
	bool GenericArray_RemoveItem(void* TargetArray, const UArrayProperty* ArrayProp,const int32& Item);
	void GenericArray_Clear(void* TargetArray, const UArrayProperty* ArrayProp);
	void GenericArray_Resize(void* TargetArray, const UArrayProperty* ArrayProp, int32 Size);
	int32 GenericArray_Length(void* TargetArray, const UArrayProperty* ArrayProp);
	int32 GenericArray_LastIndex(void* TargetArray, const UArrayProperty* ArrayProp);
	void GenericArray_Get(void* TargetArray, const UArrayProperty* ArrayProp, int32 Index, int32& Item);
	void GenericArray_Set(void* TargetArray, const UArrayProperty* ArrayProp, int32 Index, const int32& NewItem, bool bSizeToFit);
	int32 GenericArray_Find(void* TargetArray, const UArrayProperty* ArrayProperty, const int32& ItemToFind);

	// Helper function to get the last valid index of the array for error reporting, or 0 if the array is empty
	static int32 GetLastIndex(const FScriptArrayHelper& ArrayHelper)
	{
		const int32 ArraySize = ArrayHelper.Num();
		return (ArraySize > 0) ? ArraySize-1 : 0;
	}

	DECLARE_FUNCTION(execArray_Add)
	{
 		Stack.StepCompiledIn<UArrayProperty>(NULL);
		void* ArrayAddr = Stack.MostRecentPropertyAddress;
 		
		P_GET_OBJECT(UArrayProperty, ArrayProperty);
 
 		// Since NewItem isn't really an int, step the stack manually
 		const UProperty* InnerProp = ArrayProperty->Inner;
 		const int32 PropertySize = InnerProp->ElementSize * InnerProp->ArrayDim;
 		void* StorageSpace = FMemory_Alloca(PropertySize);
 		InnerProp->InitializeValue(StorageSpace);
 
 		Stack.MostRecentPropertyAddress = NULL;
 		Stack.StepCompiledIn<UProperty>(StorageSpace);
 		int32& NewItem = (Stack.MostRecentPropertyAddress != NULL) ? *(int32*)(Stack.MostRecentPropertyAddress) : (*(int32*)StorageSpace);
 
 		P_FINISH;
 
 		*(int32*)Result = GenericArray_Add(ArrayAddr, ArrayProperty, NewItem);
 
		InnerProp->DestroyValue(StorageSpace);
	}

	DECLARE_FUNCTION(execArray_Append)
	{
		// Retrieve the target array
		Stack.StepCompiledIn<UArrayProperty>(NULL);
		void* TargetArrayAddr = Stack.MostRecentPropertyAddress;

		P_GET_OBJECT(UArrayProperty, TargetArrayProperty);

		// Retrieve the source array
		Stack.StepCompiledIn<UArrayProperty>(NULL);
		void* SourceArrayAddr = Stack.MostRecentPropertyAddress;

		P_GET_OBJECT(UArrayProperty, SourceArrayProperty);

		P_FINISH;

		GenericArray_Append(TargetArrayAddr, TargetArrayProperty, SourceArrayAddr, SourceArrayProperty);
	}

	DECLARE_FUNCTION(execArray_Insert)
	{
		Stack.StepCompiledIn<UArrayProperty>(NULL);
		void* ArrayAddr = Stack.MostRecentPropertyAddress;

		P_GET_OBJECT(UArrayProperty, ArrayProperty);

		// Since NewItem isn't really an int, step the stack manually
		const UProperty* InnerProp = ArrayProperty->Inner;
		const int32 PropertySize = InnerProp->ElementSize * InnerProp->ArrayDim;
		void* StorageSpace = FMemory_Alloca(PropertySize);
		InnerProp->InitializeValue(StorageSpace);

		Stack.MostRecentPropertyAddress = NULL;
		Stack.StepCompiledIn<UProperty>(StorageSpace);
		int32& NewItem = (Stack.MostRecentPropertyAddress != NULL) ? *(int32*)(Stack.MostRecentPropertyAddress) : (*(int32*)StorageSpace);

		P_GET_PROPERTY(UIntProperty, Index);
		P_FINISH;

		GenericArray_Insert(ArrayAddr, ArrayProperty, NewItem, Index);

		InnerProp->DestroyValue(StorageSpace);
	}

	DECLARE_FUNCTION(execArray_Remove)
	{
		Stack.StepCompiledIn<UArrayProperty>(NULL);
		void* ArrayAddr = Stack.MostRecentPropertyAddress;

		P_GET_OBJECT(UArrayProperty, ArrayProperty);
		P_GET_PROPERTY(UIntProperty, Index);
		P_FINISH;

		GenericArray_Remove(ArrayAddr, ArrayProperty, Index);
	}

	DECLARE_FUNCTION(execArray_RemoveItem)
	{
		Stack.StepCompiledIn<UArrayProperty>(NULL);
		void* ArrayAddr = Stack.MostRecentPropertyAddress;

		P_GET_OBJECT(UArrayProperty, ArrayProperty);

		// Since Item isn't really an int, step the stack manually
		const UProperty* InnerProp = ArrayProperty->Inner;
		const int32 PropertySize = InnerProp->ElementSize * InnerProp->ArrayDim;
		void* StorageSpace = FMemory_Alloca(PropertySize);
		InnerProp->InitializeValue(StorageSpace);

		Stack.MostRecentPropertyAddress = NULL;
		Stack.StepCompiledIn<UProperty>(StorageSpace);
		int32& Item = (Stack.MostRecentPropertyAddress != NULL) ? *(int32*)(Stack.MostRecentPropertyAddress) : (*(int32*)StorageSpace);

		P_FINISH;

		// Bools need to be processed internally by the property so that C++ bool value is properly set.
		const UBoolProperty* BoolProperty = Cast<const UBoolProperty>(InnerProp);
		if (BoolProperty != NULL)
		{
			BoolProperty->SetPropertyValue(&Item, !!Item);
		}
		  
		bool WasRemoved = GenericArray_RemoveItem(ArrayAddr, ArrayProperty, Item);
		*(bool*)Result = WasRemoved; 

		InnerProp->DestroyValue(StorageSpace);
	}

	
	DECLARE_FUNCTION(execArray_Clear)
	{
		Stack.StepCompiledIn<UArrayProperty>(NULL);
		void* ArrayAddr = Stack.MostRecentPropertyAddress;

		P_GET_OBJECT(UArrayProperty, ArrayProperty);
		P_FINISH;

		GenericArray_Clear(ArrayAddr, ArrayProperty);
	}

	DECLARE_FUNCTION(execArray_Resize)
	{
		Stack.StepCompiledIn<UArrayProperty>(NULL);
		void* ArrayAddr = Stack.MostRecentPropertyAddress;

		P_GET_OBJECT(UArrayProperty, ArrayProperty);
		P_GET_PROPERTY(UIntProperty, Size);
		P_FINISH;

		GenericArray_Resize(ArrayAddr, ArrayProperty, Size);
	}

	DECLARE_FUNCTION(execArray_Length)
	{
		Stack.StepCompiledIn<UArrayProperty>(NULL);
		void* ArrayAddr = Stack.MostRecentPropertyAddress;

		P_GET_OBJECT(UArrayProperty, ArrayProperty);
		P_FINISH;

		*(int32*)Result = GenericArray_Length(ArrayAddr, ArrayProperty);
	}

	DECLARE_FUNCTION(execArray_LastIndex)
	{
		Stack.StepCompiledIn<UArrayProperty>(NULL);
		void* ArrayAddr = Stack.MostRecentPropertyAddress;

		P_GET_OBJECT(UArrayProperty, ArrayProperty);
		P_FINISH;

		*(int32*)Result = GenericArray_LastIndex(ArrayAddr, ArrayProperty);
	}

	DECLARE_FUNCTION(execArray_Get)
	{
		Stack.StepCompiledIn<UArrayProperty>(NULL);
		void* ArrayAddr = Stack.MostRecentPropertyAddress;

		P_GET_OBJECT(UArrayProperty, ArrayProperty);
		P_GET_PROPERTY(UIntProperty, Index);

		// Since Item isn't really an int, step the stack manually
		const UProperty* InnerProp = ArrayProperty->Inner;
		const int32 PropertySize = InnerProp->ElementSize * InnerProp->ArrayDim;
		void* StorageSpace = FMemory_Alloca(PropertySize);
		InnerProp->InitializeValue(StorageSpace);

		Stack.MostRecentPropertyAddress = NULL;
		Stack.StepCompiledIn<UProperty>(StorageSpace);
		int32& Item = (Stack.MostRecentPropertyAddress != NULL) ? *(int32*)(Stack.MostRecentPropertyAddress) : (*(int32*)StorageSpace);

		P_FINISH;

		GenericArray_Get(ArrayAddr, ArrayProperty, Index, Item);

		InnerProp->DestroyValue(StorageSpace);
	}

	DECLARE_FUNCTION(execArray_Set)
	{
		Stack.StepCompiledIn<UArrayProperty>(NULL);
		void* ArrayAddr = Stack.MostRecentPropertyAddress;

		P_GET_OBJECT(UArrayProperty, ArrayProperty);
		P_GET_PROPERTY(UIntProperty, Index);

		// Since NewItem isn't really an int, step the stack manually
		const UProperty* InnerProp = ArrayProperty->Inner;
		const int32 PropertySize = InnerProp->ElementSize * InnerProp->ArrayDim;
		void* StorageSpace = FMemory_Alloca(PropertySize);
		InnerProp->InitializeValue(StorageSpace);

		Stack.MostRecentPropertyAddress = NULL;
		Stack.StepCompiledIn<UProperty>(StorageSpace);
		int32& NewItem = (Stack.MostRecentPropertyAddress != NULL) ? *(int32*)(Stack.MostRecentPropertyAddress) : (*(int32*)StorageSpace);

		P_GET_UBOOL(bSizeToFit);

		P_FINISH;

		GenericArray_Set(ArrayAddr, ArrayProperty, Index, NewItem, bSizeToFit);

		InnerProp->DestroyValue(StorageSpace);
	}

	DECLARE_FUNCTION(execArray_Find)
	{
		Stack.StepCompiledIn<UArrayProperty>(NULL);
		void* ArrayAddr = Stack.MostRecentPropertyAddress;

		P_GET_OBJECT(UArrayProperty, ArrayProperty);

		// Since ItemToFind isn't really an int, step the stack manually
		const UProperty* InnerProp = ArrayProperty->Inner;
		const int32 PropertySize = InnerProp->ElementSize * InnerProp->ArrayDim;
		void* StorageSpace = FMemory_Alloca(PropertySize);
		InnerProp->InitializeValue(StorageSpace);

		Stack.MostRecentPropertyAddress = NULL;
		Stack.StepCompiledIn<UProperty>(StorageSpace);
		int32& ItemToFind = (Stack.MostRecentPropertyAddress != NULL) ? *(int32*)(Stack.MostRecentPropertyAddress) : (*(int32*)StorageSpace);

		P_FINISH;

		// Bools need to be processed internally by the property so that C++ bool value is properly set.
		const UBoolProperty* BoolProperty = Cast<const UBoolProperty>(InnerProp);
		if (BoolProperty != NULL)
		{
			BoolProperty->SetPropertyValue(&ItemToFind, !!ItemToFind);
		}

		// Perform the search
		int32 FoundIndex = GenericArray_Find(ArrayAddr, ArrayProperty, ItemToFind);
		*(int32*)Result = FoundIndex;

		InnerProp->DestroyValue(StorageSpace);
	}
	
	DECLARE_FUNCTION(execArray_Contains)
	{
		Stack.StepCompiledIn<UArrayProperty>(NULL);
		void* ArrayAddr = Stack.MostRecentPropertyAddress;

		P_GET_OBJECT(UArrayProperty, ArrayProperty);

		// Since ItemToFind isn't really an int, step the stack manually
		const UProperty* InnerProp = ArrayProperty->Inner;
		const int32 PropertySize = InnerProp->ElementSize * InnerProp->ArrayDim;
		void* StorageSpace = FMemory_Alloca(PropertySize);
		InnerProp->InitializeValue(StorageSpace);

		Stack.MostRecentPropertyAddress = NULL;
		Stack.StepCompiledIn<UProperty>(StorageSpace);
		int32& ItemToFind = (Stack.MostRecentPropertyAddress != NULL) ? *(int32*)(Stack.MostRecentPropertyAddress) : (*(int32*)StorageSpace);

		P_FINISH;

		// Bools need to be processed internally by the property so that C++ bool value is properly set.
		const UBoolProperty* BoolProperty = Cast<const UBoolProperty>(InnerProp);
		if (BoolProperty != NULL)
		{
			BoolProperty->SetPropertyValue(&ItemToFind, !!ItemToFind);
		}

		// Perform the search
		int32 FoundIndex = GenericArray_Find(ArrayAddr, ArrayProperty, ItemToFind);
		*(bool*)Result = (FoundIndex >= 0);

		InnerProp->DestroyValue(StorageSpace);
	}

	DECLARE_FUNCTION(execSetArrayPropertyByName)
	{
		P_GET_OBJECT(UObject, OwnerObject);
		P_GET_PROPERTY(UNameProperty, ArrayPropertyName);

		Stack.StepCompiledIn<UArrayProperty>(NULL);
		void* SrcArrayAddr = Stack.MostRecentPropertyAddress;

		P_FINISH;

		if(OwnerObject != NULL)
		{
			UArrayProperty* ArrayProp = FindField<UArrayProperty>(OwnerObject->GetClass(), ArrayPropertyName);
			if(ArrayProp != NULL)
			{
				void* Dest = ArrayProp->ContainerPtrToValuePtr<void>(OwnerObject);
				ArrayProp->CopyValuesInternal(Dest, SrcArrayAddr, 1);
			}
		}
	}
};
