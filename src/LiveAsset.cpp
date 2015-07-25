#include "LiveAsset.h"
#include "cinder/app/App.h"

using namespace ci;
using namespace std;

namespace reza { namespace live {

void LiveAsset::unwatch()
{
	LiveAssetManager::instance()->unwatch( shared_from_this() );
}

void LiveAssetSingle::reload()
{
	mCallback( loadFile( mFilePath ) );
}

bool LiveAssetSingle::checkCurrent()
{
	try {
		time_t timeLastWrite = fs::last_write_time( mFilePath );
		if( mTimeLastWrite < timeLastWrite )
        {
			mTimeLastWrite = timeLastWrite;
			return false;
		}
	}
	catch( std::exception &exc )
    {
        app::console() << "could not read file's last write time" << exc.what() << endl;
	}
	return true;
}

void LiveAssetDouble::reload()
{
	mCallback( loadFile( mFilePath1 ), loadFile( mFilePath2 ) );
}

bool LiveAssetDouble::checkCurrent()
{
	bool isCurrent = true;
	try {
		time_t timeLastWrite = ci::fs::last_write_time( mFilePath1 );
		if( mTimeLastWrite1 < timeLastWrite )
        {
			mTimeLastWrite1 = timeLastWrite;
			isCurrent = false;
		}
		timeLastWrite = ci::fs::last_write_time( mFilePath2 );
		if( mTimeLastWrite2 < timeLastWrite )
        {
			mTimeLastWrite2 = timeLastWrite;
			isCurrent = false;
		}
	}
	catch( std::exception &exc )
    {
        app::console() << "could not read file's last write time: " << exc.what() << endl;
	}
    
	return isCurrent;
}
    
void LiveAssetVector::reload()
{
    std::vector<ci::DataSourceRef> dataSources;
    for( auto &it : mFilePaths ) { dataSources.push_back( loadFile( it ) ); }
    mCallback( dataSources );
}

bool LiveAssetVector::checkCurrent()
{
    bool isCurrent = true;
    try {
        int totalFiles = mFilePaths.size();
        for( int i = 0; i < totalFiles; i++ )
        {
            time_t timeLastWrite = ci::fs::last_write_time( mFilePaths[i] );
            
            if( mTimeLastWrites[i] < timeLastWrite )
            {
                mTimeLastWrites[i] = timeLastWrite;
                isCurrent = false;
            }
        }
    }
    catch( std::exception &exc )
    {
        app::console() << "could not read file's last write time: " << exc.what() << endl;
    }
    
    return isCurrent;
}
    
LiveAssetManager::LiveAssetManager()
{
	app::App::get()->getSignalUpdate().connect( bind( &LiveAssetManager::update, this ) );
}

namespace {

void checkAssetPath( const fs::path &fullPath )
{
	if( fullPath.empty() )
    {
		throw LiveAssetException( "empty path" );
    }

	if( ! fs::exists( fullPath ) )
    {
		throw LiveAssetException( "no file at path: " + fullPath.string() );
    }
}

}

LiveAssetRef LiveAssetManager::load( const fs::path &fullPath, const function<void( DataSourceRef )> &callback )
{
	checkAssetPath( fullPath );
	LiveAssetRef asset( new LiveAssetSingle( fullPath, callback ) );
	instance()->watch( asset );
	asset->reload();
	return asset;
}

LiveAssetRef LiveAssetManager::load( const fs::path &fullPath1, const fs::path &fullPath2, const function<void ( DataSourceRef, DataSourceRef )> &callback )
{
	checkAssetPath( fullPath1 );
	checkAssetPath( fullPath2 );
	LiveAssetRef asset( new LiveAssetDouble( fullPath1, fullPath2, callback ) );
	instance()->watch( asset );
	asset->reload();
	return asset;
}
    
LiveAssetRef LiveAssetManager::load( std::vector<const ci::fs::path> paths, const std::function<void ( std::vector<ci::DataSourceRef> )> &callback )
{
    for( auto &it : paths )
    {
        checkAssetPath( it );
    }
    LiveAssetRef asset( new LiveAssetVector( paths, callback ) );
    instance()->watch( asset );
    asset->reload();
    return asset;
}

void LiveAssetManager::update()
{
	for( const auto &asset : mWatchList )
    {
		if( ! asset->checkCurrent() )
        {
			asset->reload();
        }
	}
}

LiveAssetManager* LiveAssetManager::instance()
{
	static LiveAssetManager sInstance;
	return &sInstance;
}

} } // namespace reza::live