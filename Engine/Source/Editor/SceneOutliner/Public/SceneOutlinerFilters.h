// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IFilter.h"
#include "FilterCollection.h"
#include "DelegateFilter.h"

namespace SceneOutliner
{
	/** Helper typedefs to aid in creation of filter predicates */
	typedef TDelegateFilter< const AActor* const >::FPredicate FActorFilterPredicate;
	typedef TDelegateFilter< FName >::FPredicate FFolderFilterPredicate;

	/** Proxy type that is passed to the filters to check whether something should be shown or not */
	struct FOutlinerFilterProxy
	{
		/** The actor to filter, null for folders */
		const AActor* Actor;

		/** The folder to filter */
		FName FolderPath;

		/** Constructor for filtering the specified actor */
		FOutlinerFilterProxy(const AActor* InActor)
			: Actor(InActor)
			, FolderPath(Actor->GetFolderPath())
		{}

		/** Constructor for filtering the specified folder path */
		FOutlinerFilterProxy(FName InFolder)
			: FolderPath(InFolder), Actor(nullptr)
		{}
	};

	/** Scene outliner filter class. This class abstracts the filtering of both actors and folders and allows for filtering on both types */
	struct FOutlinerFilters : public TFilterCollection<const FOutlinerFilterProxy&>
	{
		/** Check if the specified object matches the specified folder filter */
		static bool ProxyPassesFilter(const FOutlinerFilterProxy& Proxy, TSharedPtr<IFilter<FName>> Filter)
		{
			return Filter->PassesFilter(Proxy.FolderPath);
		}

		/** Check if the specified object matches the specified actor filter */
		static bool ProxyPassesFilter(const FOutlinerFilterProxy& Proxy, TSharedPtr<IFilter<const AActor* const>> Filter)
		{
			return !Proxy.Actor || Filter->PassesFilter(Proxy.Actor);
		}

		/** Base class for proxy delegates */
		struct ProxyBase : IFilter<const FOutlinerFilterProxy&>
		{
			/** Virtual functions to test whether the wrapped filter matches the specified filter. Used for removal */
			virtual bool MatchesWrappedFilter(TSharedPtr<IFilter<FName>>) const					{ return false; }
			virtual bool MatchesWrappedFilter(TSharedPtr<IFilter<const AActor* const>>) const	{ return false; }
		};

		/** Derived proxy wrapper. This proxy executes the wrapped delegate with the correct type.
			Currently supported TFilterTypes are FName and const AActor* const.
		 */
		template<class TFilterType>
		struct ProxyFilter : ProxyBase
		{
			typedef TSharedPtr<IFilter<TFilterType>> TFilterPtr;

			/** Constructor */
			ProxyFilter(TFilterPtr InFilter) : Filter(InFilter) {}

			/** Returns whether the specified Item passes the Filter's restrictions */
			virtual bool PassesFilter(const FOutlinerFilterProxy& InItem) const override
			{
				return ProxyPassesFilter(InItem, Filter);
			}

			/** Get the on changed event - does nothing for this wrapper */
			virtual FChangedEvent& OnChanged() override
			{
				return ChangedEvent;
			}

			/** Override the relevant function that matches our type */
			virtual bool MatchesWrappedFilter(TFilterPtr WrappedFilter) const override
			{
				return WrappedFilter == Filter;
			}

			TFilterPtr Filter;
			FChangedEvent ChangedEvent;
		};

		/** Meta template used to extract the filter type from a pointer or raw ptr proxy (to support MakeShareable) */
		template <class> struct TGetFilterType;
		template <class X, template<class, ESPMode::Type> class TPtr, ESPMode::Type E> struct TGetFilterType<TPtr<X, E>> { typedef typename X::ItemType Type; };
		template <class X, template<class> class TPtr> struct TGetFilterType<TPtr<X>> { typedef typename X::ItemType Type; };

		/** Add a custom filter to this collection.
			Example usage:
				struct FMyFilterType : public IFilter<const AActor* const> {...}
				Filters.Add(MakeSareable(new FMyFilterType));
		*/
		template<class T>
		void Add(T InFilter)
		{
			typedef ProxyFilter<typename TGetFilterType<T>::Type> TProxyType;

			TSharedPtr<TProxyType> NewProxy(new TProxyType(InFilter));
			NewProxy->Filter->OnChanged().AddSP(this, &FOutlinerFilters::OnChildFilterChanged);
			TFilterCollection::Add(NewProxy);
		}
		
		/** Add an actor filter predicate to this filter collection */
		void AddFilterPredicate(SceneOutliner::FActorFilterPredicate Predicate)
		{
			Add(MakeShareable(new TDelegateFilter<const AActor* const>(Predicate)));
		}

		/** Add a folder filter predicate to this filter collection */
		void AddFilterPredicate(SceneOutliner::FFolderFilterPredicate Predicate)
		{
			Add(MakeShareable(new TDelegateFilter<FName>(Predicate)));
		}

		/** Remove the specified wrapped filter from this collection */
		template<class T>
		void Remove(TSharedPtr<T> InFilter)
		{
			TSharedPtr<IFilter<typename T::ItemType>> InBaseFilter = InFilter;

			InFilter->OnChanged().RemoveAll(this);

			TArray<TSharedPtr<ProxyBase>> FiltersToRemove;
			for (const auto& Child : ChildFilters)
			{
				const TSharedPtr<ProxyBase> ProxyChild = StaticCastSharedPtr<ProxyBase>(Child);
				if (ProxyChild->MatchesWrappedFilter(InBaseFilter))
				{
					FiltersToRemove.Add(ProxyChild);
				}
			}
			if (FiltersToRemove.Num())
			{
				for (const auto& Filter : FiltersToRemove)
				{
					TFilterCollection::Remove(Filter);
				}
			}
		}
	};
}