#ifndef PLIST_ASYNC_LOADER_H
#define PLIST_ASYNC_LOADER_H

#include <cstring>
#include <memory>
#include <thread>
#include <condition_variable>

#include "cocos2d.h"

#include "ThreadSafeQueue.hpp"

USING_NS_CC;

class PlistAsyncLoader : public CCObject
{
    using FinishCallBack  = std::function<void (std::string name)>;

    struct _BaseInfo
    {
        std::string     name;
        FinishCallBack  callback;
    };

    using BaseInfoPtr       = _BaseInfo *;

    struct _ImageInfo
    {
        BaseInfoPtr     baseinfo;
        CCImage *       image;
    };

    using ImageInfoPtr      = _ImageInfo *;

    struct _TextureInfo
    {
        BaseInfoPtr     baseinfo;
        CCTexture2D *   texture;
    };

    using TextureInfoPtr    = _TextureInfo *;

    
    using BaseInfoToWorkerQueue     = ThreadSafeQueue<BaseInfoPtr>;
    using ImageInfoToMainQueue      = ThreadSafeQueue<ImageInfoPtr>;
    using TextureInfoToWorkerQueue  = ThreadSafeQueue<TextureInfoPtr>;
    using CallBackInfoQueue         = ThreadSafeQueue<BaseInfoPtr>;
    
    using UniqueMutexLock           = std::unique_lock<std::mutex>;

private:
    PlistAsyncLoader();

    ~PlistAsyncLoader();

public:
    static PlistAsyncLoader * GetInstance();

    static void DestoryInstance();

public:
    void LoadPlistResourceAsync(const std::string &name, FinishCallBack callback);

private:
    void LoadImageFinishCallBack(float);

    void AddToCacheFinishCallBack(float);
    
    void LoadImageWorker();

    void AddToCacheWorker();

private:
    BaseInfoToWorkerQueue       _btw_q;    // baseinfo to worker
    ImageInfoToMainQueue        _itm_q;    // imageinfo to main
    TextureInfoToWorkerQueue    _ttw_q;    // textureinfo to worker
    CallBackInfoQueue           _cbi_q;    // callback name queue
    
    long long                   _image_load_ref;
    long long                   _add_to_cache_ref;
    
    std::mutex                  _image_load_sleep_mutex;
    std::mutex                  _add_to_cache_sleep_mutex;
    
    std::condition_variable     _image_load_sleep_cond;
    std::condition_variable     _add_to_cache_sleep_cond;

    bool                        _quit_worker;

    static PlistAsyncLoader *   _instance;

    std::thread                 _t1;
    std::thread                 _t2;
};

#endif // !PLIST_ASYNC_LOADER_H
