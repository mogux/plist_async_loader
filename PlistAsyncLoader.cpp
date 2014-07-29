#include "PlistAsyncLoader.h"

PlistAsyncLoader * PlistAsyncLoader::_instance = nullptr;

PlistAsyncLoader::PlistAsyncLoader() :
    _quit_worker(false),
    _image_load_ref(0),
    _add_to_cache_ref(0)
{
    _t1 = std::thread(std::bind(&PlistAsyncLoader::LoadImageWorker,  this));
    _t2 = std::thread(std::bind(&PlistAsyncLoader::AddToCacheWorker, this));
}

PlistAsyncLoader::~PlistAsyncLoader()
{
    _quit_worker = true;

    _image_load_sleep_cond.notify_all();
    _add_to_cache_sleep_cond.notify_all();
    
    _t1.join();
    _t2.join();
}

PlistAsyncLoader * PlistAsyncLoader::GetInstance()
{
    if (_instance == nullptr)
        _instance = new PlistAsyncLoader();

    return _instance;
}

void PlistAsyncLoader::DestoryInstance()
{
    delete _instance;
    _instance = nullptr;
}

void PlistAsyncLoader::LoadPlistResourceAsync(const std::string &name, FinishCallBack callback)
{
    std::string pngpath = name;
    pngpath.replace(pngpath.find_last_of('.'), sizeof(".plist"), ".png");

    auto texture = CCTextureCache::sharedTextureCache()->textureForKey(pngpath.c_str());

    if (texture != nullptr)
    {
        TextureInfoPtr ttw      = new _TextureInfo;
        ttw->baseinfo           = new _BaseInfo;
        ttw->baseinfo->callback = std::move(callback);
        ttw->baseinfo->name     = name;
        ttw->texture            = texture;

        _ttw_q.Push(ttw);
    }

    if (0 == _image_load_ref)
    {
        CCDirector::sharedDirector()->getScheduler()
            ->scheduleSelector(schedule_selector(PlistAsyncLoader::LoadImageFinishCallBack), this, 0, false);
    }

    ++_image_load_ref;

    BaseInfoPtr info = new _BaseInfo;
    info->callback   = std::move(callback);
    info->name       = name;

    _btw_q.Push(info);

    _image_load_sleep_cond.notify_all();

    CCLOG("### LoadPlistResourceAsync: name = %s", name.c_str());
}

void PlistAsyncLoader::LoadImageFinishCallBack(float)
{
    if (_itm_q.Empty())
        return;

    auto item = _itm_q.GetTop();

    auto binfo  = item->baseinfo;
    auto pImage = item->image;

    auto pngname = binfo->name;

    pngname.replace(pngname.find_last_of('.'), sizeof(".plist"), ".png");

    auto texture = CCTextureCache::sharedTextureCache()->addUIImage(pImage, pngname.c_str());

    pImage->release();

    auto info = new _TextureInfo();

    info->baseinfo = binfo;
    info->texture  = texture;

    _ttw_q.Push(info);
    
    CCLOG("### LoadImageFinishCallBack: texture = %#x", texture);

    delete item;

    --_image_load_ref;

    if (0 == _image_load_ref)
    {
        CCDirector::sharedDirector()->getScheduler()
            ->unscheduleSelector(schedule_selector(PlistAsyncLoader::LoadImageFinishCallBack), this);
    }

    if (0 == _add_to_cache_ref)
    {
        CCDirector::sharedDirector()->getScheduler()
            ->scheduleSelector(schedule_selector(PlistAsyncLoader::AddToCacheFinishCallBack), this, 0, false);
    }

    ++_add_to_cache_ref;

    _add_to_cache_sleep_cond.notify_all();
}

void PlistAsyncLoader::AddToCacheFinishCallBack(float)
{
    if (_cbi_q.Empty())
        return;

    auto item = _cbi_q.GetTop();

    CCLOG("### AddToCacheFinishCallBack: name = %s", item->name.c_str());

    item->callback(item->name);

    --_add_to_cache_ref;

    if (0 == _add_to_cache_ref)
    {
        CCDirector::sharedDirector()->getScheduler()
            ->unscheduleSelector(schedule_selector(PlistAsyncLoader::AddToCacheFinishCallBack), this);
    }
}

void PlistAsyncLoader::LoadImageWorker()
{
    UniqueMutexLock lock(_image_load_sleep_mutex);
    while (true)
    {
        if (_btw_q.Empty())
        {
            if (_quit_worker)
                break;
            else
                _image_load_sleep_cond.wait(lock);

            continue;
        }

        auto item    = _btw_q.GetTop();
        auto pngname = item->name;

        pngname.replace(pngname.find_last_of('.'), sizeof(".plist"), ".png");
          
        CCImage *pImage = new CCImage();
        if (pImage && !pImage->initWithImageFileThreadSafe(pngname.c_str(), CCImage::kFmtPng))
        {
            CC_SAFE_RELEASE(pImage);
            CCLOG("FATAL: Can not load %s", pngname.c_str());
            delete item;
            return;
        }

        ImageInfoPtr info = new _ImageInfo();
        info->baseinfo    = item;
        info->image       = pImage;

        CCLOG("### LoadImageWorker: pngname = %s, pImage = %#x", pngname.c_str(), pImage);

        _itm_q.Push(info);
    }
}

void PlistAsyncLoader::AddToCacheWorker()
{
    UniqueMutexLock lock(_add_to_cache_sleep_mutex);
    while (true)
    {
        if (_ttw_q.Empty())
        {
            if (_quit_worker)
                break;
            else
                _add_to_cache_sleep_cond.wait(lock);

            continue;
        }
        
        auto item    = _ttw_q.GetTop();
        CCSpriteFrameCache::sharedSpriteFrameCache()->addSpriteFramesWithFile(item->baseinfo->name.c_str(), item->texture);

        _cbi_q.Push(item->baseinfo);
    
        CCLOG("### AddToCacheWorker: name = %s", item->baseinfo->name.c_str());
        
        delete item;
    }
}
