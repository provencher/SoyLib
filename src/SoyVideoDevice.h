#pragma once

#include "Array.hpp"
#include "HeapArray.hpp"
#include "SoyEvent.h"
#include "SoyPixels.h"
#include "SoyTime.h"
#include "SoyMemFile.h"


class TVideoDeviceMeta
{
public:
	TVideoDeviceMeta(const std::string& Serial="",const std::string& Name="") :
	mSerial			( Serial ),
	mName			( Name ),
	mVideo			( false ),
	mAudio			( false ),
	mText			( false ),
	mClosedCaption	( false ),
	mSubtitle		( false ),
	mTimecode		( false ),
	mTimedMetadata	( false ),
	mMetadata		( false ),
	mMuxed			( false ),
	mDepth			( false )
	{
		//Soy::Assert( !IsValid(), "expected invalid" );
	}
	
	const std::string&	GetName() const	{	return mName.empty() ? mSerial : mName;	}
	bool		IsValid() const		{	return !mSerial.empty();	}
	bool		operator==(const std::string& SerialOrName) const;
	
public:
	std::string	mName;
	std::string	mModel;
	std::string	mVendor;
	std::string	mSerial;
	bool		mConnected;
	bool		mVideo;
	bool		mAudio;
	bool		mText;
	bool		mClosedCaption;
	bool		mSubtitle;
	bool		mTimecode;
	bool		mTimedMetadata;
	bool		mMetadata;
	bool		mMuxed;
	bool		mDepth;
};

std::ostream& operator<< (std::ostream &out,const TVideoDeviceMeta &in);


namespace TVideoQuality
{
	enum Type
	{
		Low,
		Medium,
		High,
	};
}
namespace TVideoOption
{
	enum Type
	{
		LockedFocus,
		LockedWhiteBalance,
		LockedExposure,
	};
}


class TVideoDeviceParams
{
public:
	TVideoDeviceParams() :
	mDiscardOldFrames	( true ),
	mQuality			( TVideoQuality::Low )
	{
	}
	
	bool				mDiscardOldFrames;
	TVideoQuality::Type	mQuality;	//	replce with resolution!
};


//	seperate type for meta so we can have timecode
//	gr: change this so we store directly to a memfile so we're constantly updating a shared membuffer
class TVideoFrameImpl
{
public:
	TVideoFrameImpl(const std::string& Serial) :
		mSerial	( Serial )
	{
	}
	
	SoyPixelsImpl&							GetPixels();
	virtual std::shared_ptr<SoyPixelsImpl>	GetPixelsShared()=0;
	const SoyTime&							GetTime() const		{	return mTimecode;	}

public:
	std::string			mSerial;
	SoyTime				mTimecode;
};

class TVideoFrame : public TVideoFrameImpl
{
public:
	TVideoFrame(const std::string& Serial) :
		TVideoFrameImpl	( Serial ),
		mPixels			( new SoyPixels )
	{
	}

	virtual std::shared_ptr<SoyPixelsImpl>	GetPixelsShared()	{	return mPixels;	}

public:
	std::shared_ptr<SoyPixels>	mPixels;
};

class SoyPixelsMemFile : public SoyPixelsImpl
{
public:
	SoyPixelsMemFile(std::string Filename,bool AllowOtherFilenames) :
	mMemFileArray		( Filename, AllowOtherFilenames ),
	mPixelBufferBridge	( mPixelBuffer )
	{
	}
	
	virtual void		Copy(const SoyPixelsImpl& that,const TSoyPixelsCopyParams& Params) override
	{
		SoyPixelsImpl::Copy( that, Params );
		
		//	update memfile
		GetRawSoyPixels( GetArrayBridge( mMemFileArray ) );
	}
	
	virtual SoyPixelsMeta&					GetMeta() override				{	return mMeta;	}
	virtual const SoyPixelsMeta&			GetMeta() const override		{	return mMeta;	}
	virtual ArrayInterface<uint8>&			GetPixelsArray() override		{	return mPixelBufferBridge;	}
	virtual const ArrayInterface<uint8>&	GetPixelsArray() const override	{	return mPixelBufferBridge;	}
	
public:
	SoyPixelsMeta		mMeta;
	Array<uint8>		mPixelBuffer;
	ArrayBridgeDef<Array<uint8>>	mPixelBufferBridge;
	
	//	gr: we need to have some sub-array type where Meta sits at the start of the memfile...
	MemFileArray		mMemFileArray;
};


class TVideoFrameMemFile : public TVideoFrameImpl
{
public:
	TVideoFrameMemFile(const std::string& Serial,std::string Filename,bool AllowOtherFilenames) :
		TVideoFrameImpl	( Serial ),
		mPixels			( new SoyPixelsMemFile(Filename, AllowOtherFilenames) )
	{
	}
	
	virtual std::shared_ptr<SoyPixelsImpl>	GetPixelsShared() override	{	return mPixels;	}
	
public:
	std::shared_ptr<SoyPixelsMemFile>	mPixels;
};

//	gr: currently RAII so no play/pause virtuals...
class TVideoDevice
{
public:
	TVideoDevice(const TVideoDeviceMeta& Meta);
	virtual ~TVideoDevice();
	
	virtual TVideoDeviceMeta	GetMeta() const=0;		//	gr: make this dynamic so other states might change
	std::string					GetSerial() const		{	return GetMeta().mSerial;	}
	TVideoFrame&				GetLastFrame();			//	throws if there's an error from the last frame
	float						GetFps() const;			//	how many frames per sec are we averaging?
	int							GetFrameMs() const;		//	how long does each frame take to recieve
	void						ResetFrameCounter();	//	reset the fps counter
	
	//	gr: might need to report if supported
	virtual bool				GetOption(TVideoOption::Type Option,bool Default=false)	{	return Default;	}
	virtual bool				SetOption(TVideoOption::Type Option,bool Enable)		{	return false;	}
	
	bool						operator==(const std::string& Serial) const				{	return GetMeta() == Serial;	}
	
protected:
	void						OnFailedFrame(const std::string& Error);
	void						OnNewFrame(const SoyPixelsImpl& Pixels,SoyTime Timecode);
	SoyPixelsImpl&				LockNewFrame();
	void						UnlockNewFrame(SoyTime Timecode);
	
public:
	SoyEvent<TVideoDevice>		mOnNewFrame;
	
private:
	//	gr: video frame can cope without a lock,(no realloc) but the string will probably crash
	std::string					mLastError;		//	should be empty if last frame was okay
	TVideoFrame					mLastFrame;
	
	//	fps counting
	SoyTime						mFirstFrameTime;	//	time we got first frame
	SoyTime						mLastFrameTime;
	uint64						mFrameCount;
};



//	need a better name, generic interface to specific implementations (AVCApture, kinect etc)
class SoyVideoContainer
{
public:
	bool									HasDevice(const std::string& Serial);
	inline void								GetDevices(ArrayBridge<TVideoDeviceMeta>&& Metas)	{	return GetDevices( Metas );	}

	virtual void							GetDevices(ArrayBridge<TVideoDeviceMeta>& Metas)=0;
	virtual std::shared_ptr<TVideoDevice>	AllocDevice(const TVideoDeviceMeta& Meta,std::stringstream& Error)=0;
};


class SoyVideoCapture
{
public:
	SoyVideoCapture();
	virtual ~SoyVideoCapture();
	
	std::shared_ptr<TVideoDevice>	GetDevice(std::string Serial,std::stringstream& Error);
	void							GetDevices(ArrayBridge<TVideoDeviceMeta>&& Metas);
	TVideoDeviceMeta				GetDeviceMeta(std::string Serial);
	void							CloseDevice(std::string Serial);
	
	static TVideoDeviceMeta			GetBestDeviceMeta(std::string Serial,ArrayBridge<TVideoDeviceMeta>&& Metas);	//	gr: abstracted to static so we can use it in a unit test
	
	void							AddContainer(std::shared_ptr<SoyVideoContainer> Container);
	
private:
	Array<std::shared_ptr<TVideoDevice>>		mDevices;
	Array<std::shared_ptr<SoyVideoContainer>>	mContainers;
};

