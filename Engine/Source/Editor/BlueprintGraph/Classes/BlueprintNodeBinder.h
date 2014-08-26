// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class UEdGraphNode;

/**
 * 
 */
class FBlueprintNodeBinder
{
public:
	virtual ~FBlueprintNodeBinder() {}

	/**
	 * 
	 * 
	 * @param  BindingCandidate	
	 * @return 
	 */
	virtual bool CanBind(UObject const* BindingCandidate) const = 0;

	/**
	 * 
	 * 
	 * @return 
	 */
	virtual bool CanBindMultipleObjects() const
	{
		return false;
	}

	/**
	 * 
	 * 
	 * @return 
	 */
	bool IsBindingSet() const
	{
		return (BoundObjects.Num() > 0);
	}

	/**
	 * 
	 * 
	 * @param  BindingCandidate	
	 * @return 
	 */
	bool AddBinding(UObject const* BindingCandidate)
	{
		bool const bAddBinding = CanBind(BindingCandidate) && (!IsBindingSet() || CanBindMultipleObjects());
		if (bAddBinding)
		{
			BoundObjects.Add(BindingCandidate);
		}
		return bAddBinding;
	}

	/**
	 * 
	 * 
	 * @param  Node	
	 * @return 
	 */
	bool Bind(UEdGraphNode* Node) const
	{
		bool bFullSuccess = true;
		for (TWeakObjectPtr<UObject> Instance : BoundObjects)
		{
			if (Instance.IsValid())
			{
				bFullSuccess &= BindToNode(Node, Instance.Get());
			}
		}
		return bFullSuccess;
	}

	/**
	 * 
	 * 
	 * @param  Binding	
	 * @return 
	 */
	void RemoveBinding(UObject const* Binding)
	{
		BoundObjects.Remove(Binding);
	}

	/**
	 * 
	 * 
	 * @return 
	 */
	void ClearBindings()
	{
		BoundObjects.Empty();
	}

	/**
	 * 
	 * 
	 * @return 
	 */
	TSet< TWeakObjectPtr<UObject> > const& GetBindings() const
	{
		return BoundObjects;
	}

protected:
	/**
	 * 
	 * 
	 * @param  Node	
	 * @param  Binding	
	 * @return 
	 */
	virtual bool BindToNode(UEdGraphNode* Node, UObject* Binding) const = 0;

protected:
	/** */
	TSet< TWeakObjectPtr<UObject> > BoundObjects;
};
