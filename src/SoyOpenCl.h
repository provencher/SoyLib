#pragma once

#include "ofxSoylent.h"
#include "SoyThread.h"
#include <MSAOpenCL.h>
#include "SortArray.h"

class SoyOpenClManager;
class SoyOpenClShader;


namespace SoyOpenCl
{
	extern bool						DefaultReadBlocking;
	extern bool						DefaultWriteBlocking;
	extern bool						DefaultExecuteBlocking;
	extern msa::OpenClDevice::Type	DefaultDeviceType;
}

class SoyFileChangeDetector
{
public:
	SoyFileChangeDetector(const char* Filename);

	bool						HasChanged();
	Poco::Timestamp				GetCurrentTimestamp();	//	get the file's current timestamp
	void						SetLastModified(Poco::Timestamp Timestamp);
	const BufferString<200>&	GetFilename() const	{	return mFilename;	}

private:
	Poco::Timestamp			mLastModified;
	BufferString<200>		mFilename;
};


class SoyOpenClKernelRef
{
public:
	SoyOpenClKernelRef()
	{
	}
	SoyOpenClKernelRef(SoyRef Shader,const char* Kernel) :
		mShader	( Shader ),
		mKernel	( Kernel )
	{
	}

	BufferString<200>	Debug_GetName() const	{	return BufferString<200>() << mShader << "[" << mKernel << "]";	}

public:
	SoyRef				mShader;
	BufferString<100>	mKernel;
};



template<int DIMENSIONS>
class SoyOpenclKernelIteration
{
public:
	SoyOpenclKernelIteration() :
		mFirst		( DIMENSIONS ),
		mCount		( DIMENSIONS ),
		mBlocking	( true )
	{
		mFirst.SetAll(0);
		mCount.SetAll(0);
	}
	explicit SoyOpenclKernelIteration(int Exec1,bool Blocking) :
		mFirst		( DIMENSIONS ),
		mCount		( DIMENSIONS ),
		mBlocking	( Blocking )
	{
		mFirst.SetAll(0);
		assert( DIMENSIONS == 1 );
		mCount[0] = Exec1;
	}
	explicit SoyOpenclKernelIteration(int Exec1,int Exec2,bool Blocking) :
		mFirst		( DIMENSIONS ),
		mCount		( DIMENSIONS ),
		mBlocking	( Blocking )
	{
		mFirst.SetAll(0);
		assert( DIMENSIONS == 2 );
		mCount[0] = Exec1;
		mCount[1] = Exec2;
	}
	explicit SoyOpenclKernelIteration(int Exec1,int Exec2,int Exec3,bool Blocking) :
		mFirst		( DIMENSIONS ),
		mCount		( DIMENSIONS ),
		mBlocking	( Blocking )
	{
		mFirst.SetAll(0);
		assert( DIMENSIONS == 3 );
		mCount[0] = Exec1;
		mCount[1] = Exec2;
		mCount[2] = Exec3;
	}
public:
	BufferArray<int,DIMENSIONS>		mFirst;
	BufferArray<int,DIMENSIONS>		mCount;
	bool							mBlocking;
};
DECLARE_NONCOMPLEX_TYPE( SoyOpenclKernelIteration<1> );
DECLARE_NONCOMPLEX_TYPE( SoyOpenclKernelIteration<2> );
DECLARE_NONCOMPLEX_TYPE( SoyOpenclKernelIteration<3> );



class SoyOpenClKernel
{
public:
	SoyOpenClKernel(const char* Name,SoyOpenClShader& Parent);
	~SoyOpenClKernel()
	{
		DeleteKernel();
	}

	void			OnLoaded();				//	kernel will be null if it failed
	void			OnUnloaded();
	const char*		GetName() const					{	return mName.c_str();	}
	bool			IsValid() const					{	return mKernel!=NULL;	}
	bool			IsValidExecCount(int ExecCount)	{	return IsValidGlobalExecCount( ExecCount );	}
	bool			IsValidGlobalExecCount(int ExecCount)	{	return (GetMaxWorkGroupSize()==-1) ? true : (ExecCount<=GetMaxWorkGroupSize());	}
	bool			IsValidLocalExecCount(int ExecCount)	{	return (GetMaxWorkGroupSize()==-1) ? true : (ExecCount<=GetMaxWorkGroupSize());	}
	int				GetMaxWorkGroupSize() const				{	return GetMaxGlobalWorkGroupSize();	}
	int				GetMaxGlobalWorkGroupSize() const;
	int				GetMaxLocalWorkGroupSize() const;
	int				GetMaxBufferSize() const		{	return mDeviceInfo.maxMemAllocSize;	}
	cl_command_queue	GetQueue()					{	return mKernel ? mKernel->getQueue() : NULL;	}
	msa::OpenCL&	GetOpenCL() const;
	msa::OpenClDevice::Type	GetDevice() const		{	return static_cast<msa::OpenClDevice::Type>( mDeviceInfo.type );	}

	void			DeleteKernel();
	inline bool		operator==(const char* Name) const	{	return mName == Name;	}
	bool			Begin();
	bool			End(bool Blocking,const ArrayBridge<int>& GlobalExec,const ArrayBridge<int>& LocalExec);
	template<int DIMENSIONS>
	bool			End(const SoyOpenclKernelIteration<DIMENSIONS>& Iteration,const ArrayBridge<int>& LocalIterations)	{	return End( Iteration.mBlocking, GetArrayBridge(Iteration.mCount), LocalIterations );	}
	template<int DIMENSIONS>
	bool			End(const SoyOpenclKernelIteration<DIMENSIONS>& Iteration)	{	Array<int> LocalCount;	return End( Iteration, GetArrayBridge( LocalCount ) );	}
	
	bool			End1D(int Exec1)							{	return End1D( SoyOpenCl::DefaultExecuteBlocking, Exec1 );	}
	bool			End2D(int Exec1,int Exec2)					{	return End2D( SoyOpenCl::DefaultExecuteBlocking, Exec1, Exec2 );	}
	bool			End3D(int Exec1,int Exec2,int Exec3)		{	return End3D( SoyOpenCl::DefaultExecuteBlocking, Exec1, Exec2, Exec3 );	}
	bool			End1D(bool Blocking,int Exec1)						{	SoyOpenclKernelIteration<1> It( Exec1, Blocking );	return End(It);	}
	bool			End2D(bool Blocking,int Exec1,int Exec2)			{	SoyOpenclKernelIteration<2> It( Exec1, Exec2, Blocking );	return End(It);	}
	bool			End3D(bool Blocking,int Exec1,int Exec2,int Exec3)	{	SoyOpenclKernelIteration<3> It( Exec1, Exec2, Exec3, Blocking );	return End(It);	}
	
	void			GetIterations(Array<SoyOpenclKernelIteration<1>>& Iterations,int Exec1,bool BlockLast=SoyOpenCl::DefaultExecuteBlocking);
	void			GetIterations(Array<SoyOpenclKernelIteration<2>>& Iterations,int Exec1,int Exec2,bool BlockLast=SoyOpenCl::DefaultExecuteBlocking);
	void			GetIterations(Array<SoyOpenclKernelIteration<3>>& Iterations,int Exec1,int Exec2,int Exec3,bool BlockLast=SoyOpenCl::DefaultExecuteBlocking);

	void			OnBufferPendingWrite(cl_event PendingWriteEvent);

	bool									CheckPaddingChecksum(const int* Padding,int Length);
	template<typename TYPE> bool			CheckPaddingChecksum(const TYPE& Object)						{	return CheckPaddingChecksum( Object.mPadding, sizeofarray(Object.mPadding) );	}
	template<typename ARRAYTYPE> bool		CheckPaddingChecksum(const ArrayBridgeDef<ARRAYTYPE>& ObjectArray);
	template<typename TYPE> bool			CheckPaddingChecksum(const Array<TYPE>& ObjectArray)			{	return CheckPaddingChecksum( GetArrayBridge( ObjectArray ) );	}
	template<typename TYPE,int SIZE> bool	CheckPaddingChecksum(const BufferArray<TYPE,SIZE>& ObjectArray)	{	return CheckPaddingChecksum( GetArrayBridge( ObjectArray ) );	}
	template<typename TYPE> bool			CheckPaddingChecksum(const RemoteArray<TYPE>& ObjectArray)		{	return CheckPaddingChecksum( GetArrayBridge( ObjectArray ) );	}
	template<typename TYPE,class SORT> bool	CheckPaddingChecksum(const SortArray<TYPE,SORT>& ObjectArray)	{	return CheckPaddingChecksum( GetArrayBridge( ObjectArray.mArray ) );	}
	
protected:
	bool			WaitForPendingWrites();

private:
	msa::clDeviceInfo	mDeviceInfo;
	BufferString<100>	mName;
	Array<cl_event>		mPendingBufferWrites;	//	wait for all these to finish before executing

public:
	SoyOpenClManager&	mManager;
	SoyOpenClShader&	mShader;				//	parent
	msa::OpenCLKernel*	mKernel;
	SoyOpenClKernelRef	mKernelRef;
};


template<typename ARRAYTYPE>
inline bool SoyOpenClKernel::CheckPaddingChecksum(const ArrayBridgeDef<ARRAYTYPE>& ObjectArray)	
{	
	for ( int i=0;	i<ObjectArray.GetSize();	i++ )
		if ( !CheckPaddingChecksum( ObjectArray[i] ) )
			return false;
	return true;
}


//	soy-cl-program which auto-reloads if the file changes
class SoyOpenClShader : public SoyFileChangeDetector
{
public:
	SoyOpenClShader(SoyRef Ref,const char* Filename,const char* BuildOptions,SoyOpenClManager& Manager) :
		SoyFileChangeDetector	( Filename ),
		mRef					( Ref ),
		mManager				( Manager ),
		mProgram				( NULL ),
		mBuildOptions			( BuildOptions )
	{
	}
	~SoyOpenClShader()	{	UnloadShader();	}

	bool				IsLoading();
	bool				LoadShader();
	void				UnloadShader();
	SoyOpenClKernel*	GetKernel(const char* Name,cl_command_queue Queue);			//	load and return a kernel
	SoyRef				GetRef() const							{	return mRef;	}
	const char*			GetBuildOptions() const					{	return mBuildOptions;	}
	inline bool			operator==(const char* Filename) const	{	return GetFilename().StartsWith( Filename, false );	}
	inline bool			operator==(const SoyRef& Ref) const		{	return GetRef() == Ref;	}

private:
	ofMutex					mLock;			//	lock whilst in use so we don't reload whilst loading new file
	msa::OpenCLProgram*		mProgram;		//	shader/file
	SoyRef					mRef;
	BufferString<100>		mBuildOptions;

public:
	SoyOpenClManager&		mManager;
};

class SoyThreadQueue
{
public:
	SoyThreadQueue() :
		mQueue		( NULL ),
		mDeviceType	( msa::OpenClDevice::Invalid )
	{
	}

public:
	SoyThreadId				mThreadId;
	cl_command_queue		mQueue;
	msa::OpenClDevice::Type	mDeviceType;
};

class SoyOpenClManager : public SoyThread
{
public:
	SoyOpenClManager(const char* PlatformName,prmem::Heap& Heap);
	~SoyOpenClManager();

	virtual void			threadedFunction();

	SoyOpenClKernel*		GetKernel(SoyOpenClKernelRef KernelRef,cl_command_queue Queue);
	void					DeleteKernel(SoyOpenClKernel* Kernel);
	bool					IsValid();			//	if not, cannot use opencl
	void					PreLoadShader(const char* Filename,const char* BuildOptions);	//	load in the background
	SoyOpenClShader*		LoadShader(const char* Filename,const char* BuildOptions);	//	returns invalid ref if it couldn't be created
	SoyOpenClShader*		GetShader(SoyRef ShaderRef);
	SoyOpenClShader*		GetShader(const char* Filename);
	SoyRef					GetUniqueRef(SoyRef BaseRef=SoyRef("Shader"));
	prmem::Heap&			GetHeap()		{	return mHeap;	}
	msa::OpenCL&			GetOpenCL()		{	return mOpencl;	}

	cl_command_queue		GetQueueForThread(msa::OpenClDevice::Type DeviceType);				//	get/alloc a specific queue for the current thread
	cl_command_queue		GetQueueForThread(SoyThreadId ThreadId,msa::OpenClDevice::Type DeviceType);	//	get/alloc a specific queue for a thread

private:
	SoyOpenClShader*		AllocShader(const char* Filename,const char* BuildOptions);

private:
	prmem::Heap&			mHeap;
	ofMutex					mShaderLock;
	Array<SoyOpenClShader*>	mShaders;
	ofMutexT<Array<SoyThreadQueue>>	mThreadQueues;

	ofMutexT<Array<SoyPair<BufferString<MAX_PATH>,BufferString<MAX_PATH>>>>	mPreloadShaders;	//	filename,buildoptions

public:
	msa::OpenCL				mOpencl;
	ofEvent<SoyOpenClShader*>	mOnShaderLoaded;	//	or [successfully] reloaded - after kernels have loaded
};



class SoyClShaderRunner
{
public:
	SoyClShaderRunner(const char* Shader,const char* Kernel,SoyOpenClManager& Manager,msa::OpenClDevice::Type Device=SoyOpenCl::DefaultDeviceType,const char* BuildOptions=NULL);
	SoyClShaderRunner(const char* Shader,const char* Kernel,bool UseThreadQueue,SoyOpenClManager& Manager,msa::OpenClDevice::Type Device=SoyOpenCl::DefaultDeviceType,const char* BuildOptions=NULL);
	virtual ~SoyClShaderRunner()				{	DeleteKernel();	}

	bool				IsValid()		
	{
		auto* pKernel = GetKernel();	
		return pKernel ? pKernel->IsValid() : false;
	}
	SoyOpenClKernel*	GetKernel();
	void				DeleteKernel()			{	return mManager.DeleteKernel( mKernel );	}
	BufferString<200>	Debug_GetName() const	{	return BufferString<200>() << mKernelRef.Debug_GetName() << '[' << msa::OpenClDevice::ToString( mKernel ? mKernel->GetDevice() : msa::OpenClDevice::Invalid ) << ']';	}

public:
	bool				mUseThreadQueue;
	msa::OpenClDevice::Type	mRequestDevice;	//	device we requested at construction if not default/any. actual device used is stored in kernel
	SoyOpenClKernel*	mKernel;
	SoyOpenClKernelRef	mKernelRef;
	SoyOpenClManager&	mManager;
	prmem::Heap&		mHeap;
};




class SoyClDataBuffer
{
private:
	SoyClDataBuffer(const SoyClDataBuffer& That) :
		mManager	( *(SoyOpenClManager*)nullptr ),
		mKernel		( nullptr )
	{
	}
public:
	explicit SoyClDataBuffer(SoyOpenClManager& Manager,cl_command_queue Queue) :
		mBuffer		( nullptr ),
		mManager	( Manager ),
		mKernel		( nullptr ),
		mQueue		( Queue )
	{
	}
	template<typename TYPE>
	explicit SoyClDataBuffer(SoyOpenClKernel& Kernel,const TYPE& Data,cl_int ReadWriteMode=CL_MEM_READ_WRITE) :
		mBuffer		( nullptr ),
		mManager	( Kernel.mManager ),
		mKernel		( &Kernel ),
		mQueue		( nullptr )
	{
		Write( Data, ReadWriteMode );
	}
	template<typename TYPE>
	explicit SoyClDataBuffer(SoyOpenClKernel& Kernel,const Array<TYPE>& Array,cl_int ReadWriteMode=CL_MEM_READ_WRITE) :
		mBuffer		( nullptr ),
		mManager	( Kernel.mManager ),
		mKernel		( &Kernel ),
		mQueue		( nullptr )
	{
		Write( Array, ReadWriteMode );
	}
	template<typename TYPE,uint32 SIZE>
	explicit SoyClDataBuffer(SoyOpenClKernel& Kernel,const BufferArray<TYPE,SIZE>& Array,cl_int ReadWriteMode=CL_MEM_READ_WRITE) :
		mBuffer		( nullptr ),
		mManager	( Kernel.mManager ),
		mKernel		( &Kernel ),
		mQueue		( nullptr )
	{
		Write( Array, ReadWriteMode );
	}
	template<typename TYPE>
	explicit SoyClDataBuffer(SoyOpenClKernel& Kernel,const RemoteArray<TYPE>& Array,cl_int ReadWriteMode=CL_MEM_READ_WRITE) :
		mBuffer		( nullptr ),
		mManager	( Kernel.mManager ),
		mKernel		( &Kernel ),
		mQueue		( nullptr )
	{
		Write( Array, ReadWriteMode );
	}
	template<typename TYPE,class SORT>
	explicit SoyClDataBuffer(SoyOpenClKernel& Kernel,const SortArray<TYPE,SORT>& Array,cl_int ReadWriteMode=CL_MEM_READ_WRITE) :
		mBuffer		( nullptr ),
		mManager	( Kernel.mManager ),
		mKernel		( &Kernel ),
		mQueue		( nullptr )
	{
		Write( Array, ReadWriteMode );
	}
	template<typename TYPE>
	explicit SoyClDataBuffer(SoyOpenClKernel& Kernel,const ArrayBridge<TYPE>& Array,cl_int ReadWriteMode=CL_MEM_READ_WRITE) :
		mBuffer		( nullptr ),
		mManager	( Kernel.mManager ),
		mKernel		( &Kernel ),
		mQueue		( nullptr )
	{
		Write( Array, ReadWriteMode );
	}
	template<typename TYPE>
	explicit SoyClDataBuffer(SoyOpenClManager& OpenCl,cl_command_queue Queue,const TYPE& Data,cl_int ReadWriteMode=CL_MEM_READ_WRITE) :
		mBuffer		( nullptr ),
		mManager	( OpenCl ),
		mKernel		( nullptr ),
		mQueue		( Queue )
	{
		Write( Data, ReadWriteMode );
	}
	template<typename TYPE>
	explicit SoyClDataBuffer(SoyOpenClManager& OpenCl,cl_command_queue Queue,const Array<TYPE>& Array,cl_int ReadWriteMode=CL_MEM_READ_WRITE) :
		mBuffer		( nullptr ),
		mManager	( OpenCl ),
		mKernel		( nullptr ),
		mQueue		( Queue )
	{
		Write( Array, ReadWriteMode );
	}
	template<typename TYPE,uint32 SIZE>
	explicit SoyClDataBuffer(SoyOpenClManager& OpenCl,cl_command_queue Queue,const BufferArray<TYPE,SIZE>& Array,cl_int ReadWriteMode=CL_MEM_READ_WRITE) :
		mBuffer		( nullptr ),
		mManager	( OpenCl ),
		mKernel		( nullptr ),
		mQueue		( Queue )
	{
		Write( Array, ReadWriteMode );
	}
	template<typename TYPE>
	explicit SoyClDataBuffer(SoyOpenClManager& OpenCl,cl_command_queue Queue,const RemoteArray<TYPE>& Array,cl_int ReadWriteMode=CL_MEM_READ_WRITE) :
		mBuffer		( nullptr ),
		mManager	( OpenCl ),
		mKernel		( nullptr ),
		mQueue		( Queue )
	{
		Write( Array, ReadWriteMode );
	}
	template<typename TYPE,class SORT>
	explicit SoyClDataBuffer(SoyOpenClManager& OpenCl,cl_command_queue Queue,const SortArray<TYPE,SORT>& Array,cl_int ReadWriteMode=CL_MEM_READ_WRITE) :
		mBuffer		( nullptr ),
		mManager	( OpenCl ),
		mKernel		( nullptr ),
		mQueue		( Queue )
	{
		Write( Array, ReadWriteMode );
	}
	template<typename TYPE>
	explicit SoyClDataBuffer(SoyOpenClManager& OpenCl,cl_command_queue Queue,const ArrayBridge<TYPE>& Array,cl_int ReadWriteMode=CL_MEM_READ_WRITE) :
		mBuffer		( nullptr ),
		mManager	( OpenCl ),
		mKernel		( nullptr ),
		mQueue		( Queue )
	{
		Write( Array, ReadWriteMode );
	}

	template<typename TYPE>
	bool		Write(const ArrayBridge<TYPE>& Array,cl_int ReadWriteMode)
	{
		//	check for non 16-byte aligned structs (assume anything more than 4 bytes is a struct)
		if ( sizeof(TYPE) > 8 )
		{
			int Remainder = sizeof(TYPE) % 16;
			if ( Remainder != 0 )
			{
				BufferString<100> Debug;
				Debug << "Warning, type " << Soy::GetTypeName<TYPE>() << " not aligned to 16 bytes; " << sizeof(TYPE) << " (+" << Remainder << ")";
				ofLogWarning( Debug.c_str() );
			}
		}
		return Write( Array.GetArray(), Array.GetDataSize(), SoyOpenCl::DefaultWriteBlocking, ReadWriteMode );
	}
	template<typename ARRAYTYPE>
	bool		Write(const ArrayBridgeDef<ARRAYTYPE>& Array,cl_int ReadWriteMode)
	{
		auto& Bridge = static_cast<const ArrayBridge<ARRAYTYPE::TYPE>&>( Array );
		return Write( Bridge, ReadWriteMode );
	}
	template<typename TYPE>				bool	Write(const Array<TYPE>& Array,cl_int ReadWriteMode)			{	return Write( GetArrayBridge( Array ), ReadWriteMode );	}
	template<typename TYPE,int SIZE>	bool	Write(const BufferArray<TYPE,SIZE>& Array,cl_int ReadWriteMode)	{	return Write( GetArrayBridge( Array ), ReadWriteMode );	}
	template<typename TYPE>				bool	Write(const RemoteArray<TYPE>& Array,cl_int ReadWriteMode)		{	return Write( GetArrayBridge( Array ), ReadWriteMode );	}
	template<typename TYPE,class SORT>	bool	Write(const SortArray<TYPE,SORT>& Array,cl_int ReadWriteMode)	{	return Write( GetArrayBridge( Array.mArray ), ReadWriteMode );	}
	
	template<typename TYPE>
	bool		Write(const TYPE& Data,cl_int ReadWriteMode)
	{
		//	check for non 16-byte aligned structs (assume anything more than 4 bytes is a struct)
		if ( sizeof(TYPE) > 8 )
		{
			int Remainder = sizeof(TYPE) % 16;
			if ( Remainder != 0 )
			{
				BufferString<100> Debug;
				Debug << "Warning, type " << Soy::GetTypeName<TYPE>() << " not aligned to 16 bytes; " << sizeof(TYPE) << " (+" << Remainder << ")";
				ofLogWarning( Debug.c_str() );
			}
		}
		return Write( &Data, sizeof(TYPE), SoyOpenCl::DefaultWriteBlocking, ReadWriteMode );
	}

	~SoyClDataBuffer()
	{
		if ( mBuffer )
		{
			mManager.mOpencl.deleteBuffer( *mBuffer );
		}
	}

	template<typename TYPE>
	bool		Read(ArrayBridge<TYPE>& Array,int ElementCount=-1,bool Blocking=SoyOpenCl::DefaultReadBlocking)
	{
		if ( !mBuffer )
			return false;
		if ( ElementCount < 0 )
			ElementCount = Array.GetSize();
		Array.SetSize( ElementCount );
		if ( Array.IsEmpty() )
			return true;
		if ( !GetQueue() )
			return false;
		return mBuffer->read( Array.GetArray(), 0, Array.GetDataSize(), Blocking, GetQueue() );
	}
	template<typename ARRAYTYPE>
	bool		Read(ArrayBridgeDef<ARRAYTYPE>& Array,int ElementCount=-1,bool Blocking=SoyOpenCl::DefaultReadBlocking)
	{
		auto& Bridge = static_cast<ArrayBridge<ARRAYTYPE::TYPE>&>( Array );
		return Read( Bridge, ElementCount, Blocking );
	}
	template<typename TYPE>				bool	Read(Array<TYPE>& Array,int ElementCount=-1,bool Blocking=SoyOpenCl::DefaultReadBlocking)				{	return Read( GetArrayBridge( Array ), ElementCount, Blocking );	}
	template<typename TYPE,int SIZE>	bool	Read(BufferArray<TYPE,SIZE>& Array,int ElementCount=-1,bool Blocking=SoyOpenCl::DefaultReadBlocking)	{	return Read( GetArrayBridge( Array ), ElementCount, Blocking );	}
	template<typename TYPE>				bool	Read(RemoteArray<TYPE>& Array,int ElementCount=-1,bool Blocking=SoyOpenCl::DefaultReadBlocking)			{	return Read( GetArrayBridge( Array ), ElementCount, Blocking );	}
	template<typename TYPE,class SORT>	bool	Read(SortArray<TYPE,SORT>& Array,int ElementCount=-1,bool Blocking=SoyOpenCl::DefaultReadBlocking)		{	return Read( GetArrayBridge( Array.mArray ), ElementCount, Blocking );	}
		
	template<typename TYPE>
	bool		Read(TYPE& Data,bool Blocking=SoyOpenCl::DefaultReadBlocking)
	{
		if ( !mBuffer )
			return false;
		if ( !GetQueue() )
			return false;
		return mBuffer->read( &Data, 0, sizeof(TYPE), Blocking, GetQueue() );
	}

	bool		IsValid() const		{	return mBuffer!=nullptr;	}
	operator	bool()				{	return IsValid();	}
	cl_mem&		getCLMem()			{	static cl_mem Dummy;	assert( mBuffer );	return mBuffer ? mBuffer->getCLMem() : Dummy;	}

private:
	template<typename TYPE>
	bool	Write(const TYPE* pDataConst,int DataSize,bool Blocking,int ReadWriteMode)
	{
		assert( GetQueue() );
		if ( !GetQueue() )
			return false;
		assert( !mBuffer );
		mBuffer = mManager.mOpencl.createBuffer( GetQueue(), DataSize, ReadWriteMode, NULL, Blocking );
		if ( !mBuffer )
			return false;

		//	gr: only write data if we're NOT write-only memory
		bool WriteOnly = (ReadWriteMode & CL_MEM_WRITE_ONLY)!=0;
		if ( WriteOnly )
			return true;

		void* pData = const_cast<TYPE*>( pDataConst );
		if ( Blocking )
		{
			if ( !mBuffer->write( pData, 0, DataSize, Blocking, GetQueue() ) )
				return false;
		}
		else
		{
			//	write async and give an event to the kernel to wait for before executing
			cl_event Event;
			if ( !mBuffer->writeAsync( pData, 0, DataSize, &Event, GetQueue() ) )
				return false;
			if ( mKernel )
				mKernel->OnBufferPendingWrite( Event );
		}
		return true;
	}

	cl_command_queue	GetQueue()	{	return mKernel ? (mKernel->IsValid()?mKernel->GetQueue():nullptr) : mQueue;	}

public:
	msa::OpenCLBuffer*	mBuffer;
	SoyOpenClManager&	mManager;
	SoyOpenClKernel*	mKernel;
	cl_command_queue	mQueue;
};




