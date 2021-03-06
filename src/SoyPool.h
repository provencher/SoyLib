#pragma once

#include "HeapArray.hpp"



//	todo; make threadsafe and make the destructor wait for all elements to be released
template<typename TYPE>
class TPool
{
public:
	TPool(size_t ItemLimit=100) :
		mLimit		( ItemLimit )
	{
	}

	template<typename MATCHTYPE>
	std::shared_ptr<TYPE>			AllocPtr(const MATCHTYPE& Meta,const std::function<std::shared_ptr<TYPE>()>& RealAlloc);
	std::shared_ptr<TYPE>			AllocPtr(const std::function<std::shared_ptr<TYPE>()>& RealAlloc);
	template<typename MATCHTYPE>
	TYPE&							Alloc(const MATCHTYPE& Meta,const std::function<std::shared_ptr<TYPE>()>& RealAlloc);
	TYPE&							Alloc(const std::function<std::shared_ptr<TYPE>()>& RealAlloc);
	
	void							Release(const std::function<bool(const TYPE&)>& Compare);
	void							Release(TYPE* Object);
	void							Release(std::shared_ptr<TYPE>& Object)	{	Release( Object.get() );	}
	void							Release(TYPE& Object)					{	Release( &Object );	}
	
	//	gr: didn't I remove this to stop anyone using it?
	void							ReleaseAll();
	
	size_t							GetAllocCount() const		{	return mPool.GetSize() + mUsed.GetSize();	}
	bool							IsFull() const				{	return GetAllocCount() >= mLimit;	}

public:
	size_t							mLimit;

	//	gr: make shared ptr optional and change to a ring buffer for performance? (lockless etc)
	std::mutex						mPoolUsedLock;
	Array<std::shared_ptr<TYPE>>	mPool;
	Array<std::shared_ptr<TYPE>>	mUsed;
};




template<typename TYPE>
template<typename MATCHTYPE>
inline std::shared_ptr<TYPE> TPool<TYPE>::AllocPtr(const MATCHTYPE& Meta,const std::function<std::shared_ptr<TYPE>()>& RealAlloc)
{
	std::lock_guard<std::mutex> Lock( mPoolUsedLock );

	//	find free one
	for ( int i=0;	i<mPool.GetSize();	i++ )
	{
		auto pFree = mPool[i];
		if ( !(*pFree == Meta ) )
			continue;
		
		//	match!
		mUsed.PushBack( pFree );
		mPool.RemoveBlock( i, 1 );
		return pFree;
	}
	
	//	check if full
	if ( IsFull() )
	{
		std::stringstream Error;
		Error << "Pool<" << Soy::GetTypeName<TYPE>() << "> full; " << GetAllocCount() << "/" << mLimit;
		throw Soy::AssertException(Error.str());
	}

	//	no appropriate match, alloc
	auto pNew = RealAlloc();
	if ( !pNew )
	{
		std::stringstream Error;
		Error << "Pool<" << Soy::GetTypeName<TYPE>() << " failed to allocate new item";
		throw Soy::AssertException( Error.str() );
	}
	
	//	double check the meta == works
	//	maybe this should be a warning rather than throwing, but we'll probably make a giant pool and never reuse
	if ( !( *pNew == Meta ) )
	{
		std::stringstream Error;
		Error << "Pool<" << Soy::GetTypeName<TYPE>() << " allocated new item, but didn't match original meta (" << Meta << ")";
		throw Soy::AssertException( Error.str() );
	}
	
	mUsed.PushBack( pNew );
	return pNew;
}


template<typename TYPE>
inline std::shared_ptr<TYPE> TPool<TYPE>::AllocPtr(const std::function<std::shared_ptr<TYPE>()>& RealAlloc)
{
	//	gr: re-use above code!
	std::lock_guard<std::mutex> Lock( mPoolUsedLock );

	//	find free one
	for ( int i=0;	i<mPool.GetSize();	i++ )
	{
		auto pFree = mPool[i];
		//if ( !(*pFree == Meta ) )
		//	continue;
		
		//	match!
		mUsed.PushBack( pFree );
		mPool.RemoveBlock( i, 1 );
		return pFree;
	}
	
	//	check if full
	if ( IsFull() )
	{
		std::stringstream Error;
		Error << "Pool<" << Soy::GetTypeName<TYPE>() << "> full; " << GetAllocCount() << "/" << mLimit;
		throw Soy::AssertException(Error.str());
	}
	
	//	no appropriate match, alloc
	auto pNew = RealAlloc();
	if ( !pNew )
	{
		std::stringstream Error;
		Error << "Pool<" << Soy::GetTypeName<TYPE>() << " failed to allocate new item";
		throw Soy::AssertException( Error.str() );
	}
	
	/*
	//	double check the meta == works
	//	maybe this should be a warning rather than throwing, but we'll probably make a giant pool and never reuse
	if ( !( *pNew == Meta ) )
	{
		std::stringstream Error;
		Error << "Pool<" << Soy::GetTypeName<TYPE>() << " allocated new item, but didn't match original meta (" << Meta << ")";
		throw Soy::AssertException( Error.str() );
	}
	*/
	mUsed.PushBack( pNew );
	return pNew;
}


template<typename TYPE>
template<typename MATCHTYPE>
inline TYPE& TPool<TYPE>::Alloc(const MATCHTYPE& Meta,const std::function<std::shared_ptr<TYPE>()>& RealAlloc)
{
	return *AllocPtr( Meta, RealAlloc );
}

template<typename TYPE>
void TPool<TYPE>::Release(const std::function<bool(const TYPE&)>& Compare)
{
	std::lock_guard<std::mutex> Lock( mPoolUsedLock );

	for ( int i=0;	i<mUsed.GetSize();	i++ )
	{
		auto* pUsed = mUsed[i].get();
		if ( !Compare( *pUsed ) )
			continue;
		
		mPool.PushBack( mUsed[i] );
		mUsed.RemoveBlock( i, 1 );
		return;
	}
	
	//	check in case it's already released?
	bool IsInPool = false;
	for ( int i=0;	i<mPool.GetSize();	i++ )
	{
		auto* pUsed = mUsed[i].get();
		if ( !Compare( *pUsed ) )
			continue;
		
		IsInPool = true;
	}
	
	//	for safety
	std::stringstream Error;
	Error << "Tried to release object back to pool<" << Soy::GetTypeName<TYPE>() << " that wasn't used (in pool=" << IsInPool << ")";
	throw Soy::AssertException( Error.str() );
}

template<typename TYPE>
void TPool<TYPE>::Release(TYPE* Object)
{
	if ( !Object )
		return;

	auto Compare = [=](const TYPE& Match)
	{
		if ( &Match == Object )
			return true;
		if ( Match == *Object )
			return true;
		return false;
	};

	Release( Compare );
}


template<typename TYPE>
void TPool<TYPE>::ReleaseAll()
{
	std::lock_guard<std::mutex> Lock( mPoolUsedLock );

	mPool.PushBackArray( mUsed );
	mUsed.Clear();
}


template<typename TYPE>
TYPE& TPool<TYPE>::Alloc(const std::function<std::shared_ptr<TYPE>()>& RealAlloc)
{
	return *AllocPtr( RealAlloc );
}
