//
// Created by user on 2020/3/12.
//

#ifndef AVIDEO_MEDIAMETADATARETRIEVER_H
#define AVIDEO_MEDIAMETADATARETRIEVER_H

extern "C"{
#include <android/native_window_jni.h>
};

#include "ffmpeg_mediametadataretriever.h"
#include "Mutex.h"

class MediaMetadataRetriever
{
    State* state;
public:
    MediaMetadataRetriever();
    ~MediaMetadataRetriever();
    void disconnect();
    int setDataSource(const char* dataSourceUrl, const char* headers);
    int setDataSource(int fd, int64_t offset, int64_t length);
    int getFrameAtTime(int64_t timeUs, int option, AVPacket *pkt);
    int getScaledFrameAtTime(int64_t timeUs, int option, AVPacket *pkt, int width, int height);
    int extractAlbumArt(AVPacket *pkt);
    const char* extractMetadata(const char* key);
    const char* extractMetadataFromChapter(const char* key, int chapter);
    int getMetadata(bool update_only, bool apply_filter, AVDictionary **metadata);
    int setNativeWindow(ANativeWindow* native_window);

private:
    Mutex mLock;
};

#endif //AVIDEO_MEDIAMETADATARETRIEVER_H
