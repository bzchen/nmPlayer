/* * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_voeditor_H
#define ANDROID_voeditor_H

#if defined __VONJ_CUPCAKE__ || defined __VONJ_DONUT__
#include <utils/IMemory.h>
#elif defined __VONJ_ECLAIR__
#include <binder/IMemory.h>
#endif
#include <ui/Surface.h>
#include <media/IMediaPlayerClient.h>
#include <media/IMediaPlayer.h>
#include <media/IMediaPlayerService.h>
#include <utils/SortedVector.h>



#include "CVOWEditor.h"
#define Cvoeditor CVOWEditor


#include "vonjar.h"

#define USESUR 1

#if defined __VOPRJ_MILESTONE__
#include "vomeZM2Overlay.h"
//#define voVideoRender vomeZM2Overlay
#endif


// pure software
#if USESUR
#include "njvrender/voVideoRender.h"
#elif defined __VOPRJ_MILESTONE__
#include "vomeZM2Overlay.h"
#define voVideoRender vomeZM2Overlay
#elif 1
#include "vomeRGB565VideoRender.h"
#define voVideoRender vomeRGB565VideoRender
#else
#include "vomeQCMVideoRender.h"
#define voVideoRender vomeQCMVideoRender
#endif


#include "voOMXThread.h"

namespace android {

enum vome_event_type {
    VOME_NOP               = 0, // interface test message
    VOME_PREPARED          = 1,
    VOME_PLAYBACK_COMPLETE = 2,
    VOME_BUFFERING_UPDATE  = 3,
    VOME_SEEK_COMPLETE     = 4,
    VOME_SET_VIDEO_SIZE    = 5,
    VOME_ERROR             = 100,
    VOME_INFO              = 200,
};

enum vome_error_type {
    // 0xx
    VOME_ERROR_UNKNOWN = 1,
    // 1xx
    VOME_ERROR_SERVER_DIED = 100,
    // 2xx
    VOME_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK = 200,
    // 3xx
};


enum vome_info_type {
    // 0xx
    VOME_INFO_UNKNOWN = 1,
    // 7xx
    // The video is too complex for the decoder: it can't decode frames fast
    // enough. Possibly only the audio plays fine at this stage.
    VOME_INFO_VIDEO_TRACK_LAGGING = 700,
    // 8xx
    // Bad interleaving means that a media has been improperly interleaved or not
    // interleaved at all, e.g has all the video samples first then all the audio
    // ones. Video is playing but a lot of disk seek may be happening.
    VOME_INFO_BAD_INTERLEAVING = 800,
    // The media is not seekable (e.g live stream).
    VOME_INFO_NOT_SEEKABLE = 801,
    // New media metadata is available.
    VOME_INFO_METADATA_UPDATE = 802,
};



enum vome_player_states {
    VOME_PLAYER_STATE_ERROR        = 0,
    VOME_PLAYER_IDLE               = 1 << 0,
    VOME_PLAYER_INITIALIZED        = 1 << 1,
    VOME_PLAYER_PREPARING          = 1 << 2,
    VOME_PLAYER_PREPARED           = 1 << 3,
    VOME_PLAYER_STARTED            = 1 << 4,
    VOME_PLAYER_PAUSED             = 1 << 5,
    VOME_PLAYER_STOPPED            = 1 << 6,
    VOME_PLAYER_PLAYBACK_COMPLETE  = 1 << 7
};


// ----------------------------------------------------------------------------
// ref-counted object for callbacks
class voeditorListener: virtual public RefBase
{
public:
    virtual void notify(int msg, int ext1, int ext2) = 0;
};

class voeditor : public BnMediaPlayerClient
{
public:
    voeditor();
    ~voeditor();
            void            onFirstRef();
            void            disconnect();
            status_t        setDataSource(const char *url);
            status_t        setDataSource(int fd, int64_t offset, int64_t length);
            status_t        setVideoSurface(const sp<Surface>& surface);
            status_t        setListener(const sp<voeditorListener>& listener);
            status_t        prepare();
            status_t        prepareAsync();
            status_t        start();
            status_t        stop();
            status_t        pause();
            bool            isPlaying();
            status_t        getVideoWidth(int *w);
            status_t        getVideoHeight(int *h);
            status_t        seekTo(int msec);
            status_t        getCurrentPosition(int *msec);
            status_t        getDuration(int *msec);
            status_t        reset();
            status_t        setAudioStreamType(int type);
            status_t        setLooping(int loop); bool            isLooping();
            status_t        setVolume(float leftVolume, float rightVolume);
            void            notify(int msg, int ext1, int ext2);
	    static  int     snoop(short *data, int len, int kind);
	    status_t        invoke(const Parcel& request, Parcel *reply);
            status_t        setMetadataFilter(const Parcel& filter);
            status_t        getMetadata(bool update_only, bool apply_filter, Parcel *metadata);
	    
	    status_t	    initAudioSink(void* pvoaparam);
	    status_t	    initVideoSink(); 
	    void *	    mkplayer();


	    status_t        ExportFile(const char *url);

private:
            void            clear_l();
            status_t        seekTo_l(int msec);
            status_t        prepareAsync_l();
            status_t        getDuration_l(int *msec);
            status_t        setDataSource(const sp<IMediaPlayer>& player);

    static const sp<IMediaPlayerService>& getMediaPlayerService();
    static void addObitRecipient(const wp<voeditor>& recipient);
    static void removeObitRecipient(const wp<voeditor>& recipient);
		static int vomeMsgHandler(void* pUserData, int nID, void* pParam1, void* pParam2);
		static void vomeSignalProc(void* pparam);

		inline void vomeMonitor();
		int msgHandler(int nID, void* pParam1, void* pParam2);

    class DeathNotifier: public IBinder::DeathRecipient
    {
    public:
                DeathNotifier() {}
        virtual ~DeathNotifier();

        virtual void binderDied(const wp<IBinder>& who);
    };

    sp<IMediaPlayer>            mPlayer;
    thread_id_t                 mLockThreadId;
    Mutex                       mLock;
    Mutex                       mEventLock;
    Mutex                       mNotifyLock;
    Condition                   mSignal;
    Condition                   mSignal2;
    sp<voeditorListener>      mListener;
    void*                       mCookie;
    vome_player_states          mCurrentState;
    int                         mDuration;
    int                         mCurrentPosition;
    int                         mSeekPosition;
    bool                        mPrepareSync;
    status_t                    mPrepareStatus;
    int                         mStreamType;
    bool                        mLoop;
    float                       mLeftVolume;
    float                       mRightVolume;
    int													mVideoWidth;
    int													mVideoHeight;

		bool												m_bAudioPaused4Seek;
		CVOWEditor*								m_pvoplayer;
		VOAudioOutput*							m_paudiosink;
		voVideoRender*							m_pvideosink;
		VOA_NOTIFYEVENT							m_event;

    friend class DeathNotifier;

    static  Mutex                           sServiceLock;
    static  sp<IMediaPlayerService>         sMediaPlayerService;
    static  sp<DeathNotifier>               sDeathNotifier;
    static  SortedVector< wp<voeditor> > sObitRecipients;
};

}; // namespace android

#endif // ANDROID_voeditor_H
