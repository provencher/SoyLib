#pragma once

#include "SoyTypes.h"
#include "SoyTime.h"
#include <map>
#include <thread>
#include "SoyEvent.h"
#include "SoyScope.h"
#include "SoyString.h"

#if !defined(TARGET_ANDROID)
#define USE_HEAP_STRING
#endif

namespace std
{
	class DebugStream;
	
	//	cross platform debug output stream
	//	std::Debug << XXX << std::endl;
	extern DebugStream	Debug;
}


namespace Soy
{
	namespace Platform
	{
		bool	DebugBreak();
		bool	IsDebuggerAttached();
		void	DebugPrint(const std::string& String);
	}
	
};




namespace std
{
#if defined USE_HEAP_STRING
	typedef Soy::HeapString DebugBufferString;
#else
	typedef std::string DebugBufferString;
#endif

	class DebugStreamBuf : public streambuf
	{
	public:
		DebugStreamBuf() :
			mEnableStdOut	( true )
		{
		};
		~DebugStreamBuf()	
		{
			//	gr: currently WINDOWS on cleanup (OSX can't reach here atm but assume it suffers too)
			//		the debug heap gets destroyed (as its created later?) before the string that is flushed does
			//		so on flsh here, pointer still exists, but heap it was allocated from has gone.
			//		solution: subscribe to heap destruction?
			//		temp solution: no cleanup!
			//flush();	
		}

	protected:
		virtual int		overflow(int ch);
		void			flush(); 	

	private:
		DebugStreamBuf(DebugStreamBuf const &);		// disallow copy construction
		void operator= (DebugStreamBuf const &);	// disallow copy assignment

		DebugBufferString&	GetBuffer();
		
	public:
		bool			mEnableStdOut;
		SoyEvent<const std::string>	mOnFlush;		//	catch debug output
	};

	class DebugStream : public std::ostream
	{
	public:
		explicit DebugStream() : 
			std::basic_ostream<char,std::char_traits<char> > (&mBuffer) 
		{
		}

		SoyEvent<const std::string>&		GetOnFlushEvent()	{	return mBuffer.mOnFlush;	}
		
		//	toggle std output for this std debug stream
		void			EnableStdOut(bool Enable)	{	mBuffer.mEnableStdOut = Enable;	}
		
	private:
		DebugStreamBuf	mBuffer;
	};

};



//	gr: move this to... string?
namespace Soy
{
	std::string	FormatSizeBytes(uint64 bytes);
	
}

//#define ENABLE_SCOPE_TIMER


#if defined(ENABLE_SCOPE_TIMER)
class ofScopeTimerWarning
{
public:
	ofScopeTimerWarning(const char* Name,uint64 WarningTimeMs,bool AutoStart=true,std::ostream& Output=std::Debug) :
		mName				( Name ),
		mWarningTimeMs		( WarningTimeMs ),
		mStopped			( true ),
		mReportedOnLastStop	( false ),
		mAccumulatedTime	( 0 ),
		mOutputStream		( Output )
	{
		if ( AutoStart )
			Start( true );
	}
	~ofScopeTimerWarning()
	{
		if ( mStopped && !mReportedOnLastStop )
			Report();
		else
			Stop();
	}
	//	returns if a report was output
	bool				Stop(bool DoReport=true)
	{
		if ( mStopped )
		{
			mReportedOnLastStop = false;
			return false;
		}

		SoyTime Now(true);
		uint64 Delta = Now.GetTime() - mStartTime.GetTime();
		mAccumulatedTime += Delta;

		mReportedOnLastStop = DoReport;
		bool DidReport = false;
		if ( DoReport )
			DidReport = Report();
		
		mStopped = true;
		return DidReport;
	}
	bool				Report(bool Force=false)
	{
		if ( mAccumulatedTime >= mWarningTimeMs || Force )
		{
			mOutputStream << mName << " took " << mAccumulatedTime << "ms to execute" << std::endl;
			return true;
		}
		return false;
	}

	void				Start(bool Reset=false)
	{
		assert( mStopped );

		if ( Reset )
			mAccumulatedTime = 0;
			
		SoyTime Now(true);
		mStartTime = Now;
		mStopped = false;
	}

	SoyTime				mStartTime;
	uint64				mWarningTimeMs;
	std::string			mName;
	bool				mStopped;
	bool				mReportedOnLastStop;
	uint64				mAccumulatedTime;
	std::ostream&		mOutputStream;
};
#else
class ofScopeTimerWarning
{
public:
	ofScopeTimerWarning(const char* Name,uint64 WarningTimeMs,bool AutoStart=true,std::ostream& Output=std::Debug)
	{
	}

	bool				Stop(bool DoReport=true)
	{
		return false;
	}
	bool				Report(bool Force=false)
	{
		return false;
	}
	
	void				Start(bool Reset=false)
	{
	}
};
#endif


