// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

struct BLUEPRINTGRAPH_API FBlueprintNodeSpawnerSignature
{
public:
	FBlueprintNodeSpawnerSignature() {}
	FBlueprintNodeSpawnerSignature(FString const& UserString);
	FBlueprintNodeSpawnerSignature(TSubclassOf<UEdGraphNode> NodeClass);

	/**
	 * 
	 * 
	 * @param  NodeClass	
	 * @return 
	 */
	void SetNodeClass(TSubclassOf<UEdGraphNode> NodeClass);

	/**
	 * 
	 * 
	 * @param  SignatureObj	
	 * @return 
	 */
	void AddSubObject(UObject const* SignatureObj);

	/**
	 * 
	 * 
	 * @param  SignatureKey	
	 * @param  Value	
	 * @return 
	 */
	void AddKeyValue(FName SignatureKey, FString const& Value);

	/**
	 * 
	 * 
	 * @return 
	 */
	bool IsValid() const;

	/**
	 * 
	 * 
	 * @return 
	 */
	FString const& ToString() const;

	/**
	*
	*
	* @return
	*/
	FGuid const& AsGuid() const;

private:
	/**
	 * 
	 * 
	 * @return 
	 */
	void MarkDirty();

private:
	typedef TMap<FName, FString> FSignatureSet;
	FSignatureSet SignatureSet;

	mutable FGuid   CachedSignatureGuid;
	mutable FString CachedSignatureString;
};

