#include "SoyFilesystem.h"
#include "SoyAvf.h"
#include <CoreFoundation/CoreFoundation.h>
#include "SoyString.h"

#if defined(TARGET_OSX)
#include <AppKit/AppKit.h>
#include <AppKit/NSWorkspace.h>
#endif


namespace Platform
{
	bool	EnumDirectoryUnsafe(const std::string& Directory,std::function<bool(const std::string&,SoyPathType::Type)> OnPathFound);
}

bool GetUrlKeyBool(NSURL* Path,NSString* Key)
{
	NSNumber* KeyValue = nil;
	NSError* Error = nil;
	[Path getResourceValue:&KeyValue forKey:Key error:&Error];
	
	if ( !KeyValue )
	{
		std::stringstream Error;
		Error << "Key not found " << Soy::NSStringToString(Key);
		throw Soy::AssertException( Error.str() );
	}

	auto Value = [KeyValue boolValue];
	return Value;
}

std::string UrlGetFilename(NSURL* Path)
{
	auto* AbsolutePathNs = [Path absoluteString];
	auto AbsolutePath = Soy::NSStringToString( AbsolutePathNs );

	//	gr: this is just to make it pretty and remove the protocol really...
	Soy::StringTrimLeft( AbsolutePath, "file://localhost", true );

	return AbsolutePath;
	/*
	NSString* LocalizedName = nil;
	NSError* Error = nil;
	[Path getResourceValue:&LocalizedName forKey:NSURLNameKey error:&Error];

	return Soy::NSStringToString( LocalizedName );
	 */
}

SoyPathType::Type GetPathType(NSURL* Path)
{
	try
	{
		if ( GetUrlKeyBool( Path, NSURLIsDirectoryKey ) )
			return SoyPathType::Directory;
	}
	catch(...)
	{
	}
	
	
	try
	{
		if ( GetUrlKeyBool( Path, NSURLIsRegularFileKey ) )
			return SoyPathType::File;
	}
	catch(...)
	{
	}

	return SoyPathType::Unknown;
}



bool Platform::EnumDirectoryUnsafe(const std::string& Directory,std::function<bool(const std::string&,SoyPathType::Type)> OnPathFound)
{
	auto directoryURL = Platform::GetUrl( Directory );
	
	//	dir doesn't exist
	if ( directoryURL == nullptr )
		return true;
 
	NSArray* Keys = [NSArray arrayWithObjects:NSURLIsDirectoryKey, NSURLIsPackageKey, NSURLLocalizedNameKey, nil];
	
	auto ErrorHandler = ^(NSURL *url, NSError *error) {
		// Handle the error.
		// Return YES if the enumeration should continue after the error.
		return YES;
	};
	
	NSDirectoryEnumerator *enumerator = [[NSFileManager defaultManager]
										 enumeratorAtURL:directoryURL
										 includingPropertiesForKeys:Keys
										 options:(NSDirectoryEnumerationSkipsPackageDescendants |
												  NSDirectoryEnumerationSkipsHiddenFiles)
										 errorHandler:ErrorHandler];
 
	for (NSURL* url in enumerator)
	{
		try
		{
			auto UrlType = GetPathType( url );
			auto Filename = UrlGetFilename( url );
			
			//	if this returns false, bail
			if ( !OnPathFound( Filename, UrlType ) )
				return false;
		}
		catch(std::exception& e)
		{
			std::Debug << "Error extracting path meta; " << e.what() << std::endl;
		}
	}
	
	return true;
}


bool Platform::EnumDirectory(const std::string& Directory,std::function<bool(const std::string&,SoyPathType::Type)> OnPathFound)
{
	//	catch obj-c exceptions
	@try
	{
		return EnumDirectoryUnsafe( Directory, OnPathFound );
	}
	@catch (NSException* e)
	{
		std::stringstream Error;
		Error << "Platform::EnumDirectory NSException " << Soy::NSErrorToString( e );
		throw Soy::AssertException( Error.str() );
	}
	@catch (...)
	{
		std::stringstream Error;
		Error << "Platform::EnumDirectory Unknown exception";
		throw Soy::AssertException( Error.str() );
	}
}

NSURL* Platform::GetUrl(const std::string& Filename)
{
	NSString* UrlString = Soy::StringToNSString( Filename );
	NSError *err;
	
	//	try as file which we can test for immediate fail
	NSURL* Url = [[NSURL alloc]initFileURLWithPath:UrlString];
	//	resolve ../ ./ etc
	Url = [Url standardizedURL];
	
	if ([Url checkResourceIsReachableAndReturnError:&err] == NO)
	{
		//	FILE is not reachable.
		
		//	try as url
		Url = [[NSURL alloc]initWithString:UrlString];
		
		//	gr: throw this error IF we KNOW it's a file we're trying to reach and not an url.
		//		check for ANY scheme?
		std::stringstream Error;
		Error << "Failed to reach file from url: " << Filename << "; " << Soy::NSErrorToString(err);
		throw Soy::AssertException( Error.str() );
	}
	
	return Url;
}


bool Platform::FileExists(const std::string& Filename)
{
	NSString* UrlString = Soy::StringToNSString( Filename );
	NSError *err;
	
	//	try as file which we can test for immediate fail
	NSURL* Url = [[NSURL alloc]initFileURLWithPath:UrlString];
	if ([Url checkResourceIsReachableAndReturnError:&err] == NO)
		return false;
	
	return true;
}

void Platform::ShowFileExplorer(const std::string& Path)
{
#if defined(TARGET_OSX)
	auto PathUrl = GetUrl( Path );
	//	gr: this should have already thrown
	if ( !PathUrl )
		throw Soy::AssertException("Failed to get url");
	
	NSArray* FileURLs = [NSArray arrayWithObjects:PathUrl,nil];
	[[NSWorkspace sharedWorkspace] activateFileViewerSelectingURLs:FileURLs];

#else
	//	maybe this should popup the "share file" thing in ios
	throw Soy::AssertException("Platform::ShowFileExplorer not implemented");
#endif
}

void Platform::ShellExecute(const std::string& Path)
{
	//	https://gist.github.com/piaoapiao/4103404
	//	https://stackoverflow.com/questions/17497561/opening-web-url-with-nsbutton-mac-os
	auto* Url = GetUrl(Path);
	[[NSWorkspace sharedWorkspace] openURL:Url];

	/* ios
	auto *application = [NSApplication sharedApplication];
	[application openURL:Url];
	 */
}

void Platform::ShellOpenUrl(const std::string& UrlString)
{
	//	https://gist.github.com/piaoapiao/4103404
	//	https://stackoverflow.com/questions/17497561/opening-web-url-with-nsbutton-mac-os
	NSString* UrlStringN = Soy::StringToNSString( UrlString );
	auto* Url = [[NSURL alloc]initWithString:UrlStringN];
	[[NSWorkspace sharedWorkspace] openURL:Url];
	
	
	/* ios
	 auto *application = [NSApplication sharedApplication];
	 [application openURL:Url];
	 */
}

std::string Platform::GetAppResourcesDirectory()
{
	//	https://stackoverflow.com/a/24420220
	CFURLRef url = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());
	if ( !url )
		throw Soy::AssertException("Failed to get bundle resources url");

	try
	{
		char PathChars[PATH_MAX];
		auto* Path8 = reinterpret_cast<UInt8*>(PathChars);
		if ( !CFURLGetFileSystemRepresentation(url, true, Path8, sizeof(PathChars) ) )
			throw Soy::AssertException("Failed to get bundle resources directory name from url");
	
		CFRelease(url);
		std::string Path( PathChars );
		if ( !Soy::StringEndsWith( Path, "/", true ) )
			Path += "/";
		return Path;
	}
	catch(...)
	{
		CFRelease(url);
		throw;
	}
}

std::string Platform::GetComputerName()
{
	//	https://stackoverflow.com/questions/4063129/get-my-macs-computer-name
	//	this is blocking, so... might be good to promise() this on startup? and cache it? block when called...
	//	localizedName: Jonathan's MacBook
	//	name: "Jonathans-Macbook", or "jonathans-macbook.local"
 	auto* Name =  [[NSHost currentHost] localizedName];
	return Soy::NSStringToString(Name);
}

