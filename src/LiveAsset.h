#pragma once

#include "cinder/gl/GlslProg.h"
#include "cinder/CinderAssert.h"
#include "cinder/Exception.h"

#include <list>

namespace reza { namespace live {

typedef std::shared_ptr<class LiveAsset> LiveAssetRef;
class LiveAsset : public std::enable_shared_from_this<LiveAsset>
{
public:
	virtual ~LiveAsset() { }
	virtual void reload() = 0;
	virtual bool checkCurrent() = 0;
	void unwatch();
};

class LiveAssetSingle : public LiveAsset
{
public:
	LiveAssetSingle( const ci::fs::path &filePath, const std::function<void ( ci::DataSourceRef )> &callback )
		: mFilePath( filePath ), mCallback( callback ), mTimeLastWrite( ci::fs::last_write_time( filePath ) ) { }

	void reload() override;
	bool checkCurrent() override;

  protected:
	std::function<void ( ci::DataSourceRef )> mCallback;
	ci::fs::path mFilePath;
	std::time_t	mTimeLastWrite;
};

class LiveAssetDouble : public LiveAsset
{
public:
	LiveAssetDouble( const ci::fs::path &filePath1, const ci::fs::path &filePath2, const std::function<void ( ci::DataSourceRef, ci::DataSourceRef )> &callback )
		: mFilePath1( filePath1 ), mFilePath2( filePath2 ), mCallback( callback ),
			mTimeLastWrite1( ci::fs::last_write_time( filePath1 ) ), mTimeLastWrite2( ci::fs::last_write_time( filePath2 ) ) { }

	void reload() override;
	bool checkCurrent() override;

protected:
	std::function<void ( ci::DataSourceRef, ci::DataSourceRef )> mCallback;
	ci::fs::path mFilePath1, mFilePath2;
	std::time_t	mTimeLastWrite1, mTimeLastWrite2;
};
    
class LiveAssetVector : public LiveAsset
{
public:
    LiveAssetVector( const std::vector<const ci::fs::path> &filePaths, const std::function<void ( std::vector<ci::DataSourceRef> )> &callback )
    : mCallback( callback )
    {
        for( auto &it : filePaths )
        {
            mFilePaths.push_back( it );
            mTimeLastWrites.push_back( ci::fs::last_write_time( it ) );
        }
    }
    
    void reload() override;
    bool checkCurrent() override;
    
protected:
    std::function<void ( std::vector<ci::DataSourceRef> )> mCallback;
    std::vector<ci::fs::path> mFilePaths;
    std::vector<std::time_t> mTimeLastWrites;
};

class LiveAssetManager
{
public:
	static LiveAssetManager* instance();
	static LiveAssetRef load( const ci::fs::path &fullPath, const std::function<void ( ci::DataSourceRef )> &callback );
	static LiveAssetRef load( const ci::fs::path &fullPath1, const ci::fs::path &fullPath2, const std::function<void ( ci::DataSourceRef, ci::DataSourceRef )> &callback );
    static LiveAssetRef load( std::vector<const ci::fs::path> paths, const std::function<void ( std::vector<ci::DataSourceRef> )> &callback );

    
    
	void update();
	void watch( const LiveAssetRef &asset ) { mWatchList.push_back( asset ); }
	void unwatch( const LiveAssetRef &asset ) { mWatchList.remove( asset ); }
	const size_t getNumLiveAssets() const { return mWatchList.size(); }

  private:
	LiveAssetManager();
	std::list<LiveAssetRef> mWatchList;
};

//! Calls LiveAsset::unwatch() when goes out of scope.
struct ScopedLiveAsset
{
	ScopedLiveAsset() { }
	ScopedLiveAsset( const LiveAssetRef &liveAsset ) : mLiveAsset( liveAsset ) { }
	~ScopedLiveAsset()
	{
		if( mLiveAsset )
        {
			mLiveAsset->unwatch();
        }
	}

	ScopedLiveAsset& operator=( const LiveAssetRef &liveAsset )
	{
		if( mLiveAsset )
        {
			mLiveAsset->unwatch();
        }
		mLiveAsset = liveAsset;
		return *this;
	}

  private:
	LiveAssetRef mLiveAsset;
};

class LiveAssetException : public ci::Exception
{
public:
	LiveAssetException( const std::string &description ) : Exception( description )	{ }
};

} } // namespace reza::live
