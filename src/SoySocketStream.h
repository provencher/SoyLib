#pragma once


#include "SoySocket.h"
#include "SoyStream.h"
#include "SoyHttp.h"



class TSocketReadThread : public TStreamReader
{
public:
	TSocketReadThread(std::shared_ptr<SoySocket>& Socket,SoyRef ConnectionRef);
	
	virtual bool					Read(TStreamBuffer& Buffer) override;	//	read next chunk of data into buffer
	
protected:
	virtual void					Shutdown() __noexcept override;
	
public:
	Array<char>						mRecvBuffer;		//	static buffer, just alloc once
	SoyRef							mConnectionRef;
	std::shared_ptr<SoySocket>		mSocket;			//	socket we're reading from
};


class TSocketWriteThread : public TStreamWriter
{
public:
	TSocketWriteThread(std::shared_ptr<SoySocket>& Socket,SoyRef ConnectionRef);
	~TSocketWriteThread();
	
protected:
	virtual void					Write(TStreamBuffer& Buffer,const std::function<bool()>& Block) override;
	SoySockAddr						GetSocketAddress() const;
	
private:
	SoyRef							mConnectionRef;
	std::shared_ptr<SoySocket>		mSocket;			//	socket we're writing to
};



template<class PROTOCOL>
class TSocketReadThread_Impl : public TSocketReadThread
{
public:
	TSocketReadThread_Impl(std::shared_ptr<SoySocket>& Socket,SoyRef ConnectionRef) :
		TSocketReadThread	( Socket, ConnectionRef )
	{
		mOnDataRecieved = [this](std::shared_ptr<Soy::TReadProtocol>& Data)
		{
			auto CastData = std::dynamic_pointer_cast<PROTOCOL>( Data );
			OnDataRecieved( CastData );
		};
	}
	
	virtual void OnDataRecieved(std::shared_ptr<PROTOCOL>& Data)=0;
	
	virtual std::shared_ptr<Soy::TReadProtocol>	AllocProtocol() override
	{
		return std::shared_ptr<Soy::TReadProtocol>( new PROTOCOL );
	}
};


typedef TSocketReadThread_Impl<Http::TResponseProtocol> THttpReadThread;

class THttpWriteThread : public TSocketWriteThread
{
public:
	THttpWriteThread(std::shared_ptr<SoySocket>& Socket,SoyRef ConnectionRef) :
		TSocketWriteThread	( Socket, ConnectionRef )
	{
	}
	
};