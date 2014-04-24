// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

template<typename TType>
class INotifyOnChanged
{
public:
	virtual void OnChanged(const TType* Changed) = 0;
};

template<typename TType>
class FListenerManager
{
public:
	typedef INotifyOnChanged<TType> ListenerType;

private:
	TSet<ListenerType*> Listeners;

public:
	void AddListener(ListenerType* Listener)
	{
		if (Listener)
		{
			Listeners.Add(Listener);
		}
	}

	void RemoveListener(ListenerType* Listener)
	{
		Listeners.Remove(Listener);
	}

	void OnChanged(const TType* Changed)
	{
		for (auto Listener : Listeners)
		{
			Listener->OnChanged(Changed);
		}
	}
};