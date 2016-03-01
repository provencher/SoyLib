#include "SoyRuntimeLibrary.h"
#include "heaparray.hpp"
#include "SoyTypes.h"
#include "SoyDebug.h"


#if defined(TARGET_OSX)
#include <dlfcn.h>
#include <unistd.h>
#endif

std::string Soy::GetEnvVar(const char* Key)
{
#if defined(TARGET_OSX)
	const char* Value = getenv(Key);
	if ( !Value )
	{
		std::stringstream Error;
		Error << "Missing env var " << Key;
		throw Soy::AssertException( Error.str() );
	}
	return Value;
#elif defined(TARGET_WINDOWS)
	char* Buffer = nullptr;
	size_t BufferSize = 0;
	auto Result = _dupenv_s( &Buffer, &BufferSize, Key );

	if ( Result != S_OK || !Buffer )
	{
		std::stringstream Error;
		if ( Result == S_OK )
		{
			Error << "Null buffer returned for env var " << Key;
		}
		else
		{
			Error << "EnvVar " << Key << " error: " << ::Platform::GetErrorString(Result);
		}
		if ( Buffer )
		{
			free( Buffer );
			Buffer = nullptr;
		}
		throw Soy::AssertException( Error.str() );
	}

	std::string Value = Buffer;
	free( Buffer );
	Buffer = nullptr;
	return Value;
#else
	throw Soy::AssertException("GetEnvVar not implemented on this platform");
#endif
}


void DebugEnvVar(const char* Key)
{
	try
	{
		auto Value = Soy::GetEnvVar(Key);
		std::Debug << Key << "=" << Value << std::endl;
	}
	catch(std::exception& e)
	{
		std::Debug << Key << "=" << e.what() << std::endl;
	}
}

std::string Soy::GetCurrentWorkingDir()
{
	Array<char> Buffer;
	Buffer.SetSize(300);

#if defined(TARGET_WINDOWS)
	while ( !_getcwd( Buffer.GetArray(), Buffer.GetSize() ) )
#else
	while ( !getcwd( Buffer.GetArray(), Buffer.GetSize() ) )
#endif
	{
		//	check in case buffer isn't big enough
		auto LastError = ::Platform::GetLastError();
		if ( LastError == ERANGE )
		{
			Buffer.PushBlock(100);
			continue;
		}
		
		//	some other error
		std::stringstream Error;
		Error << "Failed to get current working directory: " << ::Platform::GetErrorString(LastError);
		throw Soy::AssertException( Error.str() );
	}
	
	return std::string( Buffer.GetArray() );
}



Soy::TRuntimeLibrary::TRuntimeLibrary(std::string Filename,std::function<bool(void)> LoadTest) :
#if defined(TARGET_OSX)||defined(TARGET_WINDOWS)
	mHandle			( nullptr ),
#endif
	mLibraryName	( Filename )
{
	if ( LoadTest && LoadTest() )
	{
		std::Debug << "Warning, load test succeeded before loading library..." << std::endl;
		return;
	}
	
#if defined(TARGET_OSX)
	//	link all symbols immediately
	int Mode = RTLD_LAZY | RTLD_GLOBAL;
	mHandle = dlopen( Filename.c_str(), Mode );
	
	if ( !mHandle )
	{
		DebugEnvVar("PATH");
		DebugEnvVar("@loader_path/");
		DebugEnvVar("@rpath/");
		DebugEnvVar("@executable_path/");
		DebugEnvVar("DYLD_LIBRARY_PATH");
		DebugEnvVar("DYLD_FRAMEWORK_PATH");
		std::Debug << "cwd: " << GetCurrentWorkingDir() << std::endl;
		
		const char* ErrorStr = dlerror();
		if ( !ErrorStr )
			ErrorStr = "<null error>";
		
		std::stringstream Error;
		Error << "dlopen failed for " << Filename << ": " << ErrorStr;
		throw Soy::AssertException( Error.str() );
	}
	
	//	run test
	if ( LoadTest && !LoadTest() )
	{
		Close();
		
		std::stringstream Error;
		Error << "Loaded library " << Filename << " but load-test failed.";
		
		throw Soy::AssertException( Error.str() );
	}
	
#elif defined(TARGET_WINDOWS)
	mHandle = LoadLibraryA( Filename.c_str() );
	if ( !mHandle )
	{
		std::stringstream Error;
		Error << "Failed to load library " << ::Platform::GetLastErrorString();
		throw Soy::AssertException(Error.str());
	}
 
#else
	throw Soy::AssertException("Soy::TRuntimeLibrary not implemented");
#endif
}

void Soy::TRuntimeLibrary::Close()
{
#if defined(TARGET_OSX)
	if ( mHandle )
	{
		dlclose( mHandle );
		mHandle = nullptr;
	}
#elif defined(TARGET_WINDOWS)
	if ( mHandle )
	{
		auto Result = FreeLibrary( mHandle );
		mHandle = nullptr;
		std::Debug << "Warning: FreeLibrary() failed for " << mLibraryName << std::endl;
	}
#endif
}

void* Soy::TRuntimeLibrary::GetSymbol(const char* Name)
{
#if defined(TARGET_OSX)
	Soy::Assert( mHandle!=nullptr, mLibraryName + " library not loaded");
	return dlsym( mHandle, Name );
#else
	Soy::Assert( mHandle!=nullptr, mLibraryName + " library not loaded");

	//	gr: this is for functions, not objects. May need to split this
	auto Address = GetProcAddress( mHandle, Name );
	if ( !Address )
		std::Debug << mLibraryName << "::GetProcAddress(" << Name << ") failed: " << ::Platform::GetLastErrorString() << std::endl;

	return Address;
#endif
}



