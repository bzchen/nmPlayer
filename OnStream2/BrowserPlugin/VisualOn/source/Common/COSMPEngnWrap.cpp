#include <afxwin.h>
#include <shlwapi.h>
#include <WinInet.h>
#include <shlobj.h>
#include <fstream>
#include <string>

#include "vompType.h"
#include "voOSFunc.h"
#include "voSource2.h"
#include "COSMPEngnWrap.h"
#include "voThread.h"
#include "voAudio.h"
#include "voVideo.h"
#include "voJSON.h"
#include "voSubTitleFormatSetting.h"
#include "voLog.h"
#include "voOnStreamType.h"
#include "voPluginCBType.h"
#include "CMsgQueue.h"
#include "voWinHttpsCertVerify.h"

#define MAX_AUDIO_VOLUME 100
#define MIN_AUDIO_VOLUME 0

#define RETRY_TIME                      3
#define FAILED_CHUNK_COUNT              10
#define LIVE_WAIT_TIME                  120000

#define USER_AGNET_NAME "User-Agent"
#define SET_COOKIE_NAME "Set-Cookie"
#define VO_VERIMATRIX_DRM_LIBNAME "voDRM_Verimatrix_AES128"
#define VOPERF_MONITOR 1

#define NO_SET_RETRY_TIME -2

//#define MAX_DRMSHAKINGHANDHREAD_COUNT 100

TCHAR COSMPEngnWrap::m_szVersion[MAX_PATH];

COSMPEngnWrap::COSMPEngnWrap():m_pOSMPPlayer(NULL),m_hWndView(NULL)
{
	InitInterMemberValue();

	m_pOverlayUI = NULL;

  m_bMute = FALSE;
  m_nAudioVolume = MAX_AUDIO_VOLUME;

	memset(m_szAnalyticsInfo, 0, 1024);
	_tcscpy(m_szJSONString,_T(""));

	memset(&m_Listener, 0, sizeof(VOOSMP_LISTENERINFO));
	memset(&m_paramUI, 0, sizeof(VOPUI_INIT_PARAM));
	memset(&m_voPlugInitParam, 0, sizeof(VO_PLUGINWRAP_INIT_PARAM));

	_tcscpy(m_szLicensePath,_T(""));
	memset(m_szLicenseString, 0, sizeof(m_szLicenseString));
	strcpy(m_szUserAgentHeaderValue,"VisualOn OSMP+ Player(Windows)\r\n");

	m_bProxySetup = FALSE;
	memset(&m_proxy, 0, sizeof(m_proxy));

	m_nBrowserType = 0;
	memset(&m_ffProxy, 0, sizeof(m_ffProxy));

	char module[1024];
	::GetModuleFileNameA(NULL, module, sizeof (module));
	if (strstr(module, "iexplore.exe"))
		m_nBrowserType = 0;
	else if (strstr(module, "chrome.exe"))
		m_nBrowserType = 1;
	else if (strstr(module, "Firefox"))
		m_nBrowserType = 2;

	VOLOGI("the browser is: %d", m_nBrowserType);
	// if firefox, detect its configure file firstly
	if (m_nBrowserType == 2)
		detectFFConfigureFile();

	m_hVODRM = NULL;
	strcpy(m_szDRMVerificationInfo,"");

	m_strDRMLibName = "";
	m_strDRMApiName = "";

	m_pDrmAdapter = NULL;
	memset((void*)&m_apiDRM, 0, sizeof(m_apiDRM));
	//m_drmThreadQueue.setCapability(MAX_DRMSHAKINGHANDHREAD_COUNT);
	m_nDrmShakingHand = 0;

	m_hwndProcessMsg = NULL;
	m_hPowerNotify = NULL;
	m_bSemaphoreGotten = FALSE;
	m_hSemaSingleInstanceCtrl = NULL;
    m_hSemaVerimatrixDrmInitCrl = NULL;
    m_hSemaVerimatrixDrmShakeCrl = NULL;
    m_iHttpRetryTime = NO_SET_RETRY_TIME;

    strcpy(m_szDefAudioLan,"");
    strcpy(m_szDefSubLan,"");
}

COSMPEngnWrap::~COSMPEngnWrap()
{

	Uninit();

	VOLOGI("uinited over");
}

void  COSMPEngnWrap::InitInterMemberValue()
{
	VOLOGI("Begin");

  m_bStopped = FALSE;
	m_OpenFlag = VO_OSMP_FLAG_SRC_OPEN_ASYNC;
  m_OpenType = VO_OSMP_SRC_AUTO_DETECT;
	_tcscpy(m_szOpenUrl,_T(""));

	m_typeRender = VOOSMP_RENDER_TYPE_DDRAW;

	m_nTrackDisabled = 0;

	VOLOGI("End");
}

BOOL	COSMPEngnWrap::Init (VO_PLUGINWRAP_INIT_PARAM * pParam)
{  
	VOLOGI("Begin");

    voCAutoLock lock (&m_Mutex);

	if (pParam != NULL) {
		memcpy(&m_voPlugInitParam, pParam, sizeof(VO_PLUGINWRAP_INIT_PARAM));
		int len = _tcslen(pParam->szWorkPath);
		if (len) {
			_tcsncpy(m_voPlugInitParam.szWorkPath, pParam->szWorkPath, len);
			TCHAR szDepPath[MAX_PATH];
			memset(szDepPath, 0, sizeof(TCHAR) * MAX_PATH);
			_tcscpy(szDepPath, m_voPlugInitParam.szWorkPath);
			_tcscat(szDepPath, _T("ThirdPartyLibs"));
			SetDllDirectory(szDepPath);
		}
	}

	TCHAR szLicensePath[MAX_PATH] = _T("");
	_tcscpy(szLicensePath,m_voPlugInitParam.szWorkPath);
	_tcscat(szLicensePath,_T("\\voVidDec.dat"));
	if(PathFileExists(szLicensePath))
	{
		_tcscpy(m_szLicensePath,szLicensePath);
	}

	m_hWndView = (HWND)m_voPlugInitParam.hView;

	BOOL bRet = InitOSMPPlayer();
	if (!m_voPlugInitParam.bWindowless) {

		InitPluginUI();
		if (m_pOverlayUI)
			m_hWndView = (HWND)m_pOverlayUI->GetView();
		m_pOSMPPlayer->SetView(m_hWndView);
	}

	CreateProcessMsgWnd();

	getSingleInstanceController();

	VOLOGI("End");
	return bRet;
}

void	COSMPEngnWrap::Uninit ()
{
	VOLOGI("Begin");

    voCAutoLock lock (&m_Mutex);

	if (m_bSemaphoreGotten) {
		::ReleaseSemaphore(m_hSemaSingleInstanceCtrl, 1, NULL);
		CloseHandle(m_hSemaSingleInstanceCtrl);
		m_bSemaphoreGotten = FALSE;
		m_hSemaSingleInstanceCtrl = NULL;
		VOLOGI("release m_hSemaSingleInstanceCtrl, this is %d", voThreadGetCurrentID());
	}

	if (m_hPowerNotify != NULL) {
		::UnregisterPowerSettingNotification(m_hPowerNotify);
		m_hPowerNotify = NULL;
	}

	if (m_hwndProcessMsg != NULL) {
		SendMessage(m_hwndProcessMsg, WM_CLOSE, 0,0);
		::DestroyWindow(m_hwndProcessMsg);
		m_hwndProcessMsg = NULL;
	}

	while (m_nDrmShakingHand) {
		VOLOGI("current m_nDrmShakingHand: %d", m_nDrmShakingHand);
		voOS_Sleep(100);
	}

	InitInterMemberValue();
	UnInitPluginUI();
	UnInitOSMPPlayer();
	UninitVerimatrixDrmEngn();

	if (m_bProxySetup) {
		if (m_proxy.pszProxyHost) {
			free(m_proxy.pszProxyHost);
			m_proxy.pszProxyHost = NULL;
		}

		if (m_proxy.pFlagData) {
			free(m_proxy.pFlagData);
			m_proxy.pFlagData = NULL;
		}

		m_proxy.nProxyPort = 0;
		m_proxy.uFlag = 0;
	}

	if (m_nBrowserType == 2 && m_ffProxy.autoconfig_url) {
		free(m_ffProxy.autoconfig_url);
		m_ffProxy.autoconfig_url = NULL;
	}

	// Revert to the standard search path used by LoadLibrary and LoadLibraryEx
	::SetDllDirectory(NULL);
	VOLOGI("End");
}

BOOL COSMPEngnWrap::InitPluginUI()
{
	VOLOGI("Begin");

	if (m_voPlugInitParam.hView==NULL)
	{
    VOLOGE("browser window is null!");
		assert(0);
		return FALSE;
	}

    VOLOGI("browser window is OK!");

	m_paramUI.hInst = NULL;
	if (m_paramUI.hView != m_voPlugInitParam.hView) {
		VOLOGI("main view changed!!!!!!!!!!!!!!!!!!!!!!!");
		if (m_pOverlayUI) {
			delete m_pOverlayUI;
			m_pOverlayUI = NULL;
		}
	} else {
		
		if (m_pOverlayUI && GetPlayerStatus() == VO_OSMP_STATUS_INITIALIZING)
			return TRUE;
	}
	m_paramUI.hView = m_voPlugInitParam.hView;
	m_paramUI.hUserData = this;
	m_paramUI.hHandle = this;
	m_paramUI.pOSMPPlayer = m_pOSMPPlayer;
	m_paramUI.nBrowserType = m_nBrowserType;
	m_paramUI.NotifyCommand = OnNotifyUICommand;
	_tcscpy(m_paramUI.szWorkingPath, m_voPlugInitParam.szWorkPath);

	if (m_pOverlayUI == NULL)
		m_pOverlayUI = new COverlayUI ();

	if(m_pOverlayUI)
		m_pOverlayUI->Init(m_voPlugInitParam.szWorkPath, &m_paramUI);

	VOLOGI("End");

	return m_pOverlayUI!=NULL;
}

void COSMPEngnWrap::UnInitPluginUI()
{
	VOLOGI("Begin");

	if (m_pOSMPPlayer)
	{
		//fix crash: refresh when playing in fullscreen model. must stop before setview null.(need to call m_pVideoRender->Stop)
		m_pOSMPPlayer->Stop();
		m_pOSMPPlayer->SetView(NULL);
	}

	if(m_pOverlayUI)
	{
		m_pOverlayUI->SetParam(VOOSMP_PLUGIN_UNIT_OSMP_PLAYER,NULL);
		delete m_pOverlayUI;
	}
	m_pOverlayUI = NULL;

	VOLOGI("End");
}

BOOL COSMPEngnWrap::InitOSMPPlayer()
{
	VOLOGI("Begin");

	if(m_pOSMPPlayer == NULL)
	{
		m_pOSMPPlayer = new CvoOnStreamMP(m_voPlugInitParam.szWorkPath);
	}
	else
	{
		VOLOGI("Stop");
		m_pOSMPPlayer->Stop();
		VOLOGI("Close");
		EraseBackGround();//before click link, clear the image of last stream
		m_pOSMPPlayer->Close();
		m_pOSMPPlayer->Uninit();
	}

	if (m_pOSMPPlayer == NULL)
		return FALSE;	

	VOLOGI("Init");
	LONG ret = m_pOSMPPlayer->Init();
	if (ret != VO_OSMP_ERR_NONE)
		return FALSE;

	VOLOGI("SetView: %p", m_hWndView);
	int nRC = m_pOSMPPlayer->SetView(m_hWndView); 
	if (m_voPlugInitParam.bWindowless) 
	{
		updateViewRegion(m_voPlugInitParam.rcDraw);
	}

	VOOSMP_RENDER_TYPE typeRender = (FALSE==m_voPlugInitParam.bWindowless) ? VOOSMP_RENDER_TYPE_DDRAW : VOOSMP_RENDER_TYPE_DC;
	nRC = m_pOSMPPlayer->SetParam(VOOSMP_PID_VIDEO_RENDER_TYPE, (void*)&typeRender); 
	m_typeRender = typeRender;

	m_pOSMPPlayer->SetParam(VOOSMP_PID_DRAW_VIDEO_DIRECTLY, &m_voPlugInitParam.bWindowless);

	if (m_nTrackDisabled & 0x001) // subtitle disabled
		EnableSubtitle(false);
	if (m_nTrackDisabled & 0x010) // audio disabled
		EnableAudioStream(FALSE);
	if (m_nTrackDisabled & 0x1000)
		EnableAudioEffect(FALSE);

  int nVolume = m_nAudioVolume;
  if (m_bMute)
  {
    nVolume = 0;
  }
  m_pOSMPPlayer->SetParam(VOOSMP_PID_AUDIO_VOLUME, &nVolume);

	m_Listener.pUserData = this;
	m_Listener.pListener = OnOSMPListener;
	m_pOSMPPlayer->SetParam(VOOSMP_PID_LISTENER , &m_Listener);

	SetLicenseFilePath(m_szLicensePath);
	//SetPreAgreedLicense(m_szLicenseString);
	//internalSetPreAgreedLicense();

	if(strlen(m_szUserAgentHeaderValue)>0)
		SetHTTPHeader(USER_AGNET_NAME,m_szUserAgentHeaderValue);

	if (m_strDRMLibName.CompareNoCase("voDRM_VisualOn_AES128") == 0)
		SetDRMLibrary(m_strDRMLibName, m_strDRMApiName, FALSE);

	if (m_hVODRM != NULL)
		m_pOSMPPlayer->SetParam(VOOSMP_SRC_PID_DRM_ADAPTER_OBJECT, (void*)m_pDrmAdapter);	

	VOLOGI("End");

	return m_pOSMPPlayer!=NULL;
}

void COSMPEngnWrap::UnInitOSMPPlayer()
{
  VOLOGI("Begin");
  
  if (m_pOSMPPlayer != NULL)
  {
    m_pOSMPPlayer->Stop();
    m_pOSMPPlayer->Close();
    m_pOSMPPlayer->Uninit();

    delete m_pOSMPPlayer;
    m_pOSMPPlayer = NULL;
  }
  
    VOLOGI("End");
}

int COSMPEngnWrap::OnNotifyUICommand (void * pUserData, int nID, void * pValue1, void * pValue2)
{
  return 0;//now, Ericssion requirement: web page do not process any UI event.

	if (pUserData == NULL)
		return VOOSMP_ERR_Implement;

  COSMPEngnWrap*eng = (COSMPEngnWrap *)pUserData;

  if(eng->m_voPlugInitParam.bWindowless)
    return 0;

  return eng->notifyWebPageCommand (nID, pValue1, pValue2);
}

int COSMPEngnWrap::notifyWebPageCommand (int nID, void * pParam1, void * pParam2)
{
  if(m_voPlugInitParam.pListener != NULL)
  {
	 return m_voPlugInitParam.pListener(m_voPlugInitParam.pUserData, nID, pParam1, pParam2);
  }

  return VOOSMP_ERR_Implement;
}

/*
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::FullScreen()
{
	voCAutoLock lock (&m_Mutex);
	if (m_pOSMPPlayer==NULL)
	{
		return VO_OSMP_ERR_POINTER;
	}

	if(m_pOverlayUI)
		m_pOverlayUI->ShowFullScreen();

	BOOL bFullScreen = IsFullScreen();


	if(m_voPlugInitParam.bWindowless)
		OSMPWndlessHandleEvent (VO_OSMP_CB_FULLSCREEN_INDICATOR, &bFullScreen, NULL);
	else
		OSMPWndHandleEvent(VO_OSMP_CB_FULLSCREEN_INDICATOR, &bFullScreen, NULL);

	if (m_nBrowserType == 2 ) {
		if (m_voPlugInitParam.bWindowless)
		{
			::PostMessage((HWND)m_voPlugInitParam.hView, WM_KEYDOWN, VK_F11, 0X00570001);
			::PostMessage((HWND)m_voPlugInitParam.hView, WM_KEYUP, VK_F11, 0XC0570001);
		}
	}

	return VO_OSMP_ERR_NONE;
}

/**
	 * Open media source.
	 *
	 * @param url  [string] Source file description (e.g. a URL or a file descriptor, etc.).
	 *
	 * @param flag [in]     Flag for opening media source. Default is zero.
	 *
	 * @param sourceType      [in] Indicates the source format. Refer to {@link VO_OSMP_SRC_FORMAT}. Default value is {@link VO_OSMP_SRC_AUTO_DETECT}
	 *
 	 * @param openParam       [in] Open parameters. Refer to {@link VOOSMPOpenParam}. Valid fields depend on the value.
	 *
 	 * @return 0 if successful.
	 */

VO_OSMP_RETURN_CODE COSMPEngnWrap::Open(LPCTSTR cstrUrl, VO_OSMP_SRC_FLAG voSourceFlag,VO_OSMP_SRC_FORMAT voSourceType, void* openParam)
{
	VOLOGI("Begin");

	voCAutoLock lockReadSrc (&m_Mutex);

#if VOPERF_MONITOR
	VOLOGI("[Open] @ %d", voOS_GetSysTime());
#endif

	// to hide full-btn
	if (m_pOverlayUI) {
        VOOSMP_STATUS status = VOOSMP_STATUS_INIT;
        m_pOverlayUI->SetParam(VOOSMP_PLUGIN_PLAYER_STATUS, &status);
	}

	VOLOGI("m_hSemaSingleInstanceCtrl: %p, m_bSemaphoreGotten: %d", m_hSemaSingleInstanceCtrl, m_bSemaphoreGotten);
	if (!m_bSemaphoreGotten) {
		if (m_hSemaSingleInstanceCtrl) {
			DWORD dwRtn = WaitForSingleObject(m_hSemaSingleInstanceCtrl, 1);
            VOLOGI("WaitForSingleObject(m_hSemaSingleInstanceCtrl, 1) return 0x%08x. this is %d", dwRtn, voThreadGetCurrentID());

			if (dwRtn != WAIT_OBJECT_0) {
				if (m_pOverlayUI)
				{
					m_pOverlayUI->SetParam(VOOSMP_PLUGIN_ENALBE_THIS_INSTANCE,NULL);
				}
				return VO_OSMP_ERR_MULTIPLE_INSTANCES_NOT_SUPPORTED;
			} else {
				m_bSemaphoreGotten = TRUE;
				if (m_pOverlayUI)
				{
					BOOL bEnable = TRUE;
					m_pOverlayUI->SetParam(VOOSMP_PLUGIN_ENALBE_THIS_INSTANCE,&bEnable);
				}
			}
		}
	}

	InitOSMPPlayer();
	if (m_pOSMPPlayer == NULL)
		return VO_OSMP_ERR_POINTER;

	{
		// here to do CA verify
		// David @ 10/17/2013
		TCHAR pswzUrl[1024];
		memset(pswzUrl, 0, sizeof(pswzUrl));
		int len = _tcslen((LPCTSTR)cstrUrl);
		if (len > 1024)
			len = 1024;

		_tcsncpy(pswzUrl, (LPCTSTR)cstrUrl, len);
		_tcsupr(pswzUrl);
		LPCWSTR https = _tcsstr(pswzUrl, L"HTTPS://");
		if (https != NULL) {
			int ret = voWinHttpsCertVerify((void*)cstrUrl);
			if (ret !=  VOCERT_ERROR_NONE)
				return VO_OSMP_ERR_HTTPS_CA_FAIL;
		}
	}

	if(FALSE ==isUrlInExceptions(cstrUrl))
		internalSetHttpProxy();

    if(m_iHttpRetryTime != NO_SET_RETRY_TIME)
    {
        SetHTTPRetryTimeout(m_iHttpRetryTime);
    }

    if(strlen(m_szDefAudioLan)>0)
    {
        SetDefaultAudioLanguage(m_szDefAudioLan,FALSE);
    }
    if (strlen(m_szDefSubLan)>0)
    {
        SetDefaultSubtitleLanguage(m_szDefSubLan,FALSE);
    }

	m_bStopped = FALSE;
	m_OpenFlag = voSourceFlag;
	m_OpenType = voSourceType;
	int nRC = m_pOSMPPlayer->Open((void *)cstrUrl ,voSourceFlag|VOOSMP_FLAG_SOURCE_URL , voSourceType);

	if (nRC == VO_OSMP_ERR_NONE)
	{
		if(_tcslen(cstrUrl)<2048)
			_tcscpy(m_szOpenUrl,cstrUrl);
	}
	else
	{
		VOLOGE("Open failed!");
		_tcscpy(m_szOpenUrl,_T(""));
	}

	if(FALSE == m_voPlugInitParam.bWindowless)
	{
		if(voSourceFlag==VO_OSMP_FLAG_SRC_OPEN_SYNC)
			notifyUIEventInfo(VOOSMP_SRC_CB_Open_Finished,NULL,NULL);//sync: tell UI a new url start to play

		if(!IsFullScreen())//for the first open url, do not move the window, the video can not show. because the window size is not correct. 
		{
			RECT rcView;
			GetClientRect ((HWND)m_voPlugInitParam.hView, &rcView);
			SetWindowPos (m_hWndView, 0, rcView.left, rcView.top, rcView.right - rcView.left, rcView.bottom - rcView.top, 0);
		}
	}

	VOLOGI("End");

	return (VO_OSMP_RETURN_CODE)nRC;
}

/*
Description
Close media data source

Parameter

Returned Value
Return VO_OSMP_ERR_NONE if successful
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::Close()
{
  VOLOGI("Begin");

  voCAutoLock lockReadSrc (&m_Mutex); 

  if (m_pOSMPPlayer==NULL)
  {
    return VO_OSMP_ERR_POINTER;
  }

  InitInterMemberValue();

  EndBuffering();

  int nRC = m_pOSMPPlayer->Stop();
  nRC = m_pOSMPPlayer->Close();
  nRC = m_pOSMPPlayer->Uninit();

  if (m_pOverlayUI) 
  {
      VOOSMP_STATUS status = VOOSMP_STATUS_STOPPED;
      m_pOverlayUI->SetParam(VOOSMP_PLUGIN_PLAYER_STATUS, &status);
  }

  EraseBackGround();

  VOLOGI("End");

  return (VO_OSMP_RETURN_CODE)nRC;
}

/*
Description
Start playback

Parameter

Returned Value
Return VO_OSMP_ERR_NONE if successful
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::Start()
{
  VOLOGI("Begin");

  voCAutoLock lockReadSrc (&m_Mutex);

  if (m_pOSMPPlayer==NULL)
  {
    return VO_OSMP_ERR_POINTER;
  }

  //after stop, can play at start.
  if (m_bStopped)
  {
    m_bStopped = FALSE;

    ULONG nRC = Open(m_szOpenUrl ,m_OpenFlag, m_OpenType,0);
    if( (m_OpenFlag & VO_OSMP_FLAG_SRC_OPEN_ASYNC) > 0)
      return (VO_OSMP_RETURN_CODE)nRC;
  }

  if (GetPlayerStatus() == VO_OSMP_STATUS_PLAYING)
    return VO_OSMP_ERR_NONE;

  int nRC = m_pOSMPPlayer->Run();

  if (m_pOverlayUI) 
  {
      VOOSMP_STATUS status = VOOSMP_STATUS_RUNNING;
      m_pOverlayUI->SetParam(VOOSMP_PLUGIN_PLAYER_STATUS, &status);
  }

  if(m_bMute)
  {
    int nAudioVolume = 0;
    m_pOSMPPlayer->SetParam(VOOSMP_PID_AUDIO_VOLUME, &nAudioVolume);
  }

  VOLOGI("End");

  return (VO_OSMP_RETURN_CODE)nRC;
}

/*
Description
Pause playback

Parameter

Returned Value
Return VO_OSMP_ERR_NONE if successful
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::Pause()
{
  VOLOGI("Begin");

  voCAutoLock lockReadSrc (&m_Mutex);
  if (m_pOSMPPlayer==NULL)
  {
    return VO_OSMP_ERR_POINTER;
  }

  int nRC = m_pOSMPPlayer->Pause();

  if (m_pOverlayUI) 
  {
      VOOSMP_STATUS status = VOOSMP_STATUS_PAUSED;
      m_pOverlayUI->SetParam(VOOSMP_PLUGIN_PLAYER_STATUS, &status);
  }

  VOLOGI("End");

  return (VO_OSMP_RETURN_CODE)nRC;
}

/*
Description
Stop playback

Parameter

Returned Value
Return VO_OSMP_ERR_NONE if successful
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::Stop()
{
  VOLOGI("Begin");

  voCAutoLock lockReadSrc (&m_Mutex);
  if (m_pOSMPPlayer==NULL)
  {
    return VO_OSMP_ERR_POINTER;
  }

  EndBuffering();

  m_bStopped = TRUE;
  int nRC = m_pOSMPPlayer->Stop();

  if (m_pOverlayUI) 
  {
      VOOSMP_STATUS status = VOOSMP_STATUS_STOPPED;
      m_pOverlayUI->SetParam(VOOSMP_PLUGIN_PLAYER_STATUS, &status);
  }

  VOLOGI("End");

  return (VO_OSMP_RETURN_CODE)nRC;
}

/*
Description
Get duration of the current media source

Parameter

Returned Value
Return the duration of the current media source if successful; 0 if the current media source is a live stream.
*/
LONG COSMPEngnWrap::GetDuration()
{
  if (m_pOSMPPlayer==NULL)
  {
    return 0;
  }

  LONG lDura = m_pOSMPPlayer->GetDuration();
  return lDura;
}

/*
Description
Set playback position to "position" (seek)

Parameter
[in]	position	the position to seek to in millisecond

Returned Value
Return the position after seek operation if successful, -1 if failed.
*/
LONG COSMPEngnWrap::SetPosition(LONG lMSec)
{
  voCAutoLock lock (&m_Mutex);
  if (m_pOSMPPlayer==NULL)
  {
    return -1;
  }

#if VOPERF_MONITOR
  VOLOGI("[SEEK] @ %d to pos: %d", voOS_GetSysTime(), lMSec);
#endif

  int iCurPos = m_pOSMPPlayer->SetPos(lMSec);
  return iCurPos;
}

/*
Description
Get current playback position

Parameter

Returned Value
Current playback position
*/
LONG COSMPEngnWrap::GetPosition()
{
  if (m_pOSMPPlayer==NULL)
  {
    return 0;
  }

  LONG lPos = m_pOSMPPlayer->GetPos();  
  return lPos;
}

/*
Description
Set playback volume

Parameter
[in]	left	left audio channel volume. A value of 0.0f indicates silence; Value 1.0 indicates no attenuation.
[in]	right	right audio channel volume

Returned Value
Return VO_OSMP_ERR_NONE if successful
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::SetVolume(FLOAT fLeftVol, FLOAT fRightVol)//only call it by webpage, the plugin do not call it self
{
  voCAutoLock lock (&m_Mutex);
  if (m_pOSMPPlayer==NULL)
  {
    return VO_OSMP_ERR_POINTER;
  }

  m_bMute = FALSE;

  fLeftVol = (fLeftVol < MIN_AUDIO_VOLUME) ? MIN_AUDIO_VOLUME : fLeftVol;
  fLeftVol = (fLeftVol > MAX_AUDIO_VOLUME) ? MAX_AUDIO_VOLUME : fLeftVol;

  m_nAudioVolume = (INT)fLeftVol;
  int nRC = m_pOSMPPlayer->SetParam(VOOSMP_PID_AUDIO_VOLUME, &m_nAudioVolume);  

  return (VO_OSMP_RETURN_CODE)nRC;
}

LONG COSMPEngnWrap::GetVolume()
{
  if (m_pOSMPPlayer==NULL)
  {
    return 0;
  }

  if (m_bMute)
  {
    return 0;
  }

  int nRC = m_pOSMPPlayer->GetParam(VOOSMP_PID_AUDIO_VOLUME, &m_nAudioVolume);

  return m_nAudioVolume;
}

/*
Description
Mute the audio

Parameter

Returned Value
Return VO_OSMP_ERR_NONE if successful
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::Mute()
{
  voCAutoLock lock (&m_Mutex);
  if (m_pOSMPPlayer==NULL)
  {
    return VO_OSMP_ERR_POINTER;
  }

  m_bMute = TRUE;

  int nAudioVolume = 0;
  int nRC = m_pOSMPPlayer->SetParam(VOOSMP_PID_AUDIO_VOLUME, &nAudioVolume);
  return (VO_OSMP_RETURN_CODE)nRC;
}

/*
Description
Unmute the audio and restore the previous volume settings

Parameter

Returned Value
Return VO_OSMP_ERR_NONE if successful
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::UnMute()
{
  voCAutoLock lock (&m_Mutex);
  if (m_pOSMPPlayer==NULL)
  {
    return VO_OSMP_ERR_POINTER;
  }

  m_bMute = FALSE;

  int nRC = m_pOSMPPlayer->SetParam(VOOSMP_PID_AUDIO_VOLUME, &m_nAudioVolume);
  return (VO_OSMP_RETURN_CODE)nRC;
}

/*
Description
Set video aspect ratio

Parameter
[in]	ar	aspect ratio of the video (refers to VO_OSMP_ASPECT_RATIO)

Returned Value
Return VO_OSMP_ERR_NONE if successful
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::SetVideoAspectRatio(VO_OSMP_ASPECT_RATIO ar)
{
  voCAutoLock lock (&m_Mutex);
  if (m_pOSMPPlayer==NULL)
  {
    return VO_OSMP_ERR_POINTER;
  }

  int nRC = m_pOSMPPlayer->SetParam(VOOSMP_PID_VIDEO_ASPECT_RATIO, &ar);

  //fix bug: https://sh.visualon.com/node/28592 Video remains if change aspectRatio.
  if(GetPlayerStatus() == VO_OSMP_STATUS_PAUSED)
  {
    if (!m_voPlugInitParam.bWindowless)
    {
      Redraw();
    }
    else
    {
      notifyWebPageCommand(VOMP_CB_VideoReadyToRender,NULL,NULL);
    }
  }
  return (VO_OSMP_RETURN_CODE)nRC;
}

/*
Description
Get player status

Parameter

Returned Value
Player status (refers to VO_OSMP_STATUS)
*/
VO_OSMP_STATUS COSMPEngnWrap::GetPlayerStatus()
{
  if (m_pOSMPPlayer==NULL)
  {
    return VO_OSMP_STATUS_MAX;
  }

  VO_OSMP_STATUS nStatus = VO_OSMP_STATUS_MAX;
  int nRC = m_pOSMPPlayer->GetParam(VOOSMP_PID_STATUS,&nStatus);

  return nStatus;
}

/*
Description
Check if the playback can be paused

Parameter

Returned Value
Return TRUE if the playback can be paused
*/
BOOL COSMPEngnWrap::CanBePaused()
{
  if (m_pOSMPPlayer==NULL)
  {
    return FALSE;
  }

  return !IsLiveStreaming();
}

/*
Description
Set content of license file

Parameter
[in]	data	Content of the license file in a char* array

Returned Value
Return VO_OSMP_ERR_NONE if successful
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::SetLicenseContent(LPCTSTR data)
{
  voCAutoLock lock (&m_Mutex);
  if (m_pOSMPPlayer==NULL)
  {
    return VO_OSMP_ERR_POINTER;
  }

  int nRC = m_pOSMPPlayer->SetParam(VOOSMP_PID_LICENSE_CONTENT,(void *)data);
  return (VO_OSMP_RETURN_CODE)nRC;
}

/*
Description
Set pre-agreed license string

Parameter
[in]	str	pre-agreed license string

Returned Value
Return VO_OSMP_ERR_NONE if successful
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::SetPreAgreedLicense(LPCSTR str)
{
  voCAutoLock lock (&m_Mutex);
  if (m_pOSMPPlayer==NULL || NULL == str)
  {
    return VO_OSMP_ERR_POINTER;
  }

  int len = strlen(str);
  if (len > MAX_PATH)
	  len = MAX_PATH;

  strncpy(m_szLicenseString,str, len);
  int nRC = m_pOSMPPlayer->SetParam(VOOSMP_PID_LICENSE_TEXT,(void *)m_szLicenseString);
  return (VO_OSMP_RETURN_CODE)nRC;
}

/*
Description
Set the location of the license file

Parameter
[in]	path	location of license file

Returned Value
Return VO_OSMP_ERR_NONE if successful
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::SetLicenseFilePath(LPCTSTR path)
{
  voCAutoLock lock (&m_Mutex);
  
  if (m_pOSMPPlayer==NULL || NULL == path)
  {
    return VO_OSMP_ERR_POINTER;
  }

  _tcscpy(m_szLicensePath,path);
  int nRC = m_pOSMPPlayer->SetParam(VOOSMP_PID_LICENSE_FILE_PATH, (void*)m_szLicensePath);

  return (VO_OSMP_RETURN_CODE)nRC;
}

/*
Description
Enable/Disable deblock. This is valid for H.264 and Real video. The default is enabled.

Parameter
[in]	value	TRUE to enable; FALSE to disable

Returned Value
Return VO_OSMP_ERR_NONE if successful
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::EnableDeblock(BOOL bEnable)
{
  voCAutoLock lock (&m_Mutex);
  if (m_pOSMPPlayer==NULL)
  {
    return VO_OSMP_ERR_POINTER;
  }

  int nRC = m_pOSMPPlayer->SetParam(VOOSMP_PID_DEBLOCK_ONOFF,&bEnable);
  return (VO_OSMP_RETURN_CODE)nRC;
}

/*
Description
Set the location of device capability file.

Parameter
[in]	filePath	location of the device capability file

Returned Value
Return VO_OSMP_ERR_NONE if successful
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::SetDeviceCapabilityByFile(LPCTSTR filePath)
{
  if (m_pOSMPPlayer==NULL)
  {
    return VO_OSMP_ERR_POINTER;
  }

  //int nRC = m_pOSMPPlayer->SetParam(VOOSMP_PID_CAP_TABLE_PATH,&filePath);
  //return (VO_OSMP_RETURN_CODE)nRC;

  MessageBox(NULL,_T("NO IMPLEMENT"), _T("SetDeviceCapabilityByFile"),MB_OK);
  return VO_OSMP_ERR_IMPLEMENT;
}

/*
Description
Set the initial bitrate

Parameter
[in]	bitrate	Bitrate

Returned Value
Return VO_OSMP_ERR_NONE if successful
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::SetInitialBitrate(int bitrate)
{
  if (m_pOSMPPlayer==NULL)
  {
    return VO_OSMP_ERR_POINTER;
  }

  MessageBox(NULL,_T("NO IMPLEMENT"), _T("SetInitialBitrate"),MB_OK);
  return VO_OSMP_ERR_IMPLEMENT;
}

/**
* Set the buffering time for continue playback when need to buffer again
*
* @param time [in] buffer time (milliseconds)
*
* @return {@link VO_OSMP_ERR_NONE} if successful.
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::SetAnewBufferingTime(int buftime)
{
	if (m_pOSMPPlayer==NULL)
	{
		return VO_OSMP_ERR_POINTER;
	}

	int bt = buftime;
	if (bt < 0)
		bt = 0;

	return (VO_OSMP_RETURN_CODE)m_pOSMPPlayer->SetParam( VOOSMP_SRC_PID_BUFFER_BUFFERING_TIME, &bt);
}


/*
DDescription
Get the starting position of the media data source

Parameter

Returned Value
Return 0 for VOD; TBD for a live stream (0 currently)
*/
LONG COSMPEngnWrap::GetMinPosition()
{
  if (m_pOSMPPlayer==NULL)
  {
    return 0;
  }

  return m_pOSMPPlayer->GetMinPosition();
}

/*
Description
Get ending position

Parameter

Returned Value
Return the duration for VOD; TBD for a live stream (0 currently)
*/
LONG COSMPEngnWrap::GetMaxPosition()
{
	if (m_pOSMPPlayer==NULL)
	{
		return 0;
	}

	return m_pOSMPPlayer->GetMaxPosition();
}


VO_OSMP_RETURN_CODE COSMPEngnWrap::EnableAudioEffect(BOOL bEnable)
{
	voCAutoLock lock (&m_Mutex);
	if (m_pOSMPPlayer==NULL)
	{
		return VO_OSMP_ERR_POINTER;
	}

	if (bEnable)
		m_nTrackDisabled &= 0x0111;
	else 
		m_nTrackDisabled |= 0x1000;

	BOOL be = bEnable;
	int nRC = m_pOSMPPlayer->SetParam(VOOSMP_PID_AUDIO_EFFECT_ENABLE, &be);
	return (VO_OSMP_RETURN_CODE)nRC;
}

VO_OSMP_RETURN_CODE COSMPEngnWrap::EnableAudioStream(BOOL bEnable)
{
	voCAutoLock lock (&m_Mutex);
	if (m_pOSMPPlayer==NULL)
	{
		return VO_OSMP_ERR_POINTER;
	}

	if (bEnable)
		m_nTrackDisabled &= 0x101;
	else
		m_nTrackDisabled |= 0x010;

	BOOL be = bEnable;
	int nRC = m_pOSMPPlayer->SetParam(VOOSMP_PID_AUDIO_STREAM_ONOFF, &be);
	return (VO_OSMP_RETURN_CODE)nRC;
}

VO_OSMP_RETURN_CODE	COSMPEngnWrap::SetDRMVerificationInfo(char* drmvi, int len,BOOL bSetByWebPage) {

	if (m_pOSMPPlayer == NULL)
		return VO_OSMP_ERR_UNINITIALIZE;

	if (drmvi==NULL)
		return VO_OSMP_ERR_POINTER;

	memset(m_szDRMVerificationInfo, 0, MAX_PATH);
	strncpy(m_szDRMVerificationInfo, drmvi, len);
	VOLOGI("drm verification server: %s", m_szDRMVerificationInfo);
	
	if (m_strDRMLibName.CompareNoCase(VO_VERIMATRIX_DRM_LIBNAME) == 0) {

		VO_U32 tID = 0;
		voThreadHandle hDrmThrd = NULL;
		int ret = voThreadCreate(&hDrmThrd, &tID, (voThreadProc)shakeHandWithDrmServerProc, this, 0);
		//if (ret == VO_ERR_NONE) {

		//	m_drmThreadQueue.enqueue(tID);
		//}
		return VO_OSMP_ERR_NONE;
	}

	//when web page set value to plugin, just need remember the values.
	if (bSetByWebPage)
		return VO_OSMP_ERR_NONE;

	VOOSMP_SRC_VERIFICATIONINFO vi;
	memset(&vi, 0, sizeof(vi));
	if (len) {
		vi.pData = (void *)drmvi;
		vi.nDataSize = len;
	}
	int nRC = m_pOSMPPlayer->SetParam(VOOSMP_SRC_PID_DODRMVERIFICATION, &vi);

	return (VO_OSMP_RETURN_CODE)nRC;
}

char* COSMPEngnWrap::GetDRMUniqueIndentifier() {

  VOLOGI("Tick: %d, Begin", voOS_GetSysTime());

	if (m_pOSMPPlayer == NULL)
		return NULL;

	char* pid = NULL; int ret = 0;
	if (m_pDrmAdapter && m_apiDRM.GetParameter)
		ret = m_apiDRM.GetParameter(m_pDrmAdapter, VO_PID_DRM2_UNIQUE_IDENTIFIER, (void*)&pid);

  VOLOGI("Tick: %d, End", voOS_GetSysTime());

	if ((VO_OSMP_ERR_NONE == ret) && (pid != NULL))
		return pid;

	return NULL;
}

VO_OSMP_RETURN_CODE	COSMPEngnWrap::SetDRMUniqueIndentifier(char* pid) {

	if (m_pOSMPPlayer == NULL)
		return VO_OSMP_ERR_UNINITIALIZE;

	if (pid == NULL)
		return VO_OSMP_ERR_POINTER;

	int ret = m_pOSMPPlayer->SetParam(VOOSMP_SRC_PID_DRM_UNIQUE_IDENTIFIER, (void*)pid);

	return (VO_OSMP_RETURN_CODE)ret;
}

/*
Description
Get the number of available video streams

Parameter

Returned Value
Return the number of video streams; -1 if failed.
*/

INT COSMPEngnWrap::GetVideoCount()
{
  if (m_pOSMPPlayer==NULL)
  {
    return 0;
  }

  INT lCount = m_pOSMPPlayer->GetVideoCount();

  return lCount;
}

/*
Description
Get the number of available audio streams

Parameter

Returned Value
Return the number of audio streams; -1 if failed.
*/
INT COSMPEngnWrap::GetAudioCount()
{
  if (m_pOSMPPlayer==NULL)
  {
    return 0;
  }

  INT lCount = m_pOSMPPlayer->GetAudioCount();

  return lCount;
}

/*
Description
Get the number of available subtilte streams

Parameter

Returned Value
Return the number of subtitle streams; -1 if failed.
*/
INT COSMPEngnWrap::GetSubtitleCount()
{
  if (m_pOSMPPlayer==NULL)
  {
    return 0;
  }

  INT lCount = m_pOSMPPlayer->GetSubtitleCount();

  return lCount;
}

/*
Description
Select a video stream by index

Parameter
[in]	index	index of video stream, valid from 0.

Returned Value
Return VO_OSMP_ERR_NONE if successful
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::SelectVideo(INT index)
{
  if (m_pOSMPPlayer==NULL)
  {
    return VO_OSMP_ERR_POINTER;
  }

  int nRC = m_pOSMPPlayer->SelectVideo(index);
  return (VO_OSMP_RETURN_CODE)nRC;
}

/*
Description
Select an audio stream using index

Parameter
[in]	index	index of audio stream, valid from 0.

Returned Value
Return VO_OSMP_ERR_NONE if successful
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::SelectAudio(INT index)
{
  voCAutoLock lock (&m_Mutex);
  if (m_pOSMPPlayer==NULL)
  {
    return VO_OSMP_ERR_POINTER;
  }

  int nRC = m_pOSMPPlayer->SelectAudio(index);
  return (VO_OSMP_RETURN_CODE)nRC;
}

/*
Description
Select a subtitle stream by index

Parameter
[in]	index	index of subtitle stream, valid from 0.

Returned Value
Return VO_OSMP_ERR_NONE if successful
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::SelectSubtitle(INT index)
{
  voCAutoLock lock (&m_Mutex);
  if (m_pOSMPPlayer==NULL)
  {
    return VO_OSMP_ERR_POINTER;
  }

  int nRC = m_pOSMPPlayer->SelectSubtitle(index);
  return (VO_OSMP_RETURN_CODE)nRC;
}

/*
Description
Check if specified video stream is available for selection

Parameter
[in]	index	index of video stream, valid from 0.

Returned Value
Return TRUE if the specified stream is available for selection; FALSE if not.
*/
BOOL COSMPEngnWrap::IsVideoAvailable(INT index)
{
  if (m_pOSMPPlayer==NULL)
  {
    return FALSE;
  }

  BOOL bEnable = m_pOSMPPlayer->IsVideoAvailable(index);

  return bEnable;
}

/*
Description
Check if specified audio stream is available for selection

Parameter
[in]	index	index of audio stream, valid from 0.

Returned Value
Return TRUE if the specified stream is available for selection; FALSE if not.
*/
BOOL COSMPEngnWrap::IsAudioAvailable(INT index)
{
  if (m_pOSMPPlayer==NULL)
  {
    return FALSE;
  }

  BOOL bEnable = m_pOSMPPlayer->IsAudioAvailable(index);

  return bEnable;
}

/*
Description
Check if specified subtitle stream is available for selection

Parameter
[in]	index	index of subtitle stream, valid from 0.

Returned Value
Return TRUE if the specified stream is available for selection; FALSE if not.
*/
BOOL COSMPEngnWrap::IsSubtitleAvailable(INT index)
{
  if (m_pOSMPPlayer==NULL)
  {
    return FALSE;
  }

  BOOL bEnable = m_pOSMPPlayer->IsSubtitleAvailable(index);

  return bEnable;
}

/*
Description
Commit all current asset selections. If any asset type is not selected, current playing asset of that type is used. This operation will remove all current selections after commit.

Parameter

Returned Value
Return VO_OSMP_ERR_NONE if successful
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::CommitSelection()
{
	voCAutoLock lock (&m_Mutex);
	if (NULL == m_pOSMPPlayer)
	{
		return VO_OSMP_ERR_POINTER;
	}

	VOOSMP_SRC_CURR_TRACK_INDEX curIndex;;
	m_pOSMPPlayer->GetCurrSelectedTrackIndex(&curIndex);

#if VOPERF_MONITOR
	if (curIndex.nCurrAudioIdx >= 0) {
		VOLOGI("[Commit selected audio %d ] @ %d", curIndex.nCurrAudioIdx, voOS_GetSysTime());
	}
	
	if (curIndex.nCurrSubtitleIdx >= 0) {
		VOLOGI("[Commit selected subtitle %d ] @ %d", curIndex.nCurrSubtitleIdx, voOS_GetSysTime());
	}

	if (curIndex.nCurrVideoIdx >= 0) {
		VOLOGI("[Commit selected video %d ] @ %d", curIndex.nCurrVideoIdx, voOS_GetSysTime());
	}
#endif

	int nRC = m_pOSMPPlayer->CommitSelection();
	return (VO_OSMP_RETURN_CODE)nRC;
}

/*
Description
Remove all current uncommitted selections

Parameter

Returned Value
Return VO_OSMP_ERR_NONE if successful
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::ClearSelection()
{
  voCAutoLock lock (&m_Mutex);
  if (m_pOSMPPlayer==NULL)
  {
    return VO_OSMP_ERR_POINTER;
  }

  int nRC = m_pOSMPPlayer->ClearSelection();

   return (VO_OSMP_RETURN_CODE)nRC;
}

/*
Description
Return the properties of specified video stream in JSON object [8.2] format. Supported keys are: ¡°description¡±, ¡°codec¡±, ¡°bitrate¡±, ¡°width¡±, ¡°height¡±

Parameter
[in]	index	index of video stream, valid from 0.

Returned Value
Return video property in JSON object format: {key£ºvalue£¬key£ºvalue,...}
*/
JSON COSMPEngnWrap::GetVideoProperty(INT index)
{
  if (m_pOSMPPlayer==NULL)
  {
    return _T("");
  }

  voJSON *pvi = voJSON_CreateObject();
  if (pvi == NULL)
    return _T("");

  VOOSMP_SRC_TRACK_PROPERTY *pProperty = NULL;
  int nRC = m_pOSMPPlayer->GetVideoProperty(index,&pProperty);
  if (pProperty == NULL)
	  return _T("");

  if (nRC != VO_OSMP_ERR_NONE)
  {
    if (index<m_pOSMPPlayer->GetVideoCount())
    {
      CHAR szProperty[MAX_PATH] = "Video property error";
      CHAR szTemp[10] = "";
      itoa(index,szTemp,10);
      strcat(szProperty,szTemp);

      voJSON_AddStringToObject(pvi, "description", szProperty);
    }
  }
  else
  {
    for (int i=0; i<pProperty->nPropertyCount; ++i)
    {
      VOOSMP_SRC_TRACK_ITEM_PROPERTY* pItemProperties = pProperty->ppItemProperties[i];

      char szProperty[MAX_PATH] = "";                    
      if (strstr(pItemProperties->szKey, ("language")))
      {
        char* szTemp = strstr(pItemProperties->pszProperty, "-");
        strncpy(szProperty, pItemProperties->pszProperty, szTemp-pItemProperties->pszProperty);
      }
      else
      {
        strcpy(szProperty,pItemProperties->pszProperty);
      }

      voJSON_AddStringToObject(pvi, pItemProperties->szKey, szProperty);
    }
  } 

  _tcscpy(m_szJSONString, _T(""));
  char* szval = voJSON_Print(pvi);
  ::MultiByteToWideChar(CP_ACP, 0, szval, -1, m_szJSONString, sizeof(m_szJSONString));
  free(szval);
  voJSON_Delete(pvi);
  return m_szJSONString;
}

/*
Description
Return the properties of specified audio stream in JSON object [8.2] format. Supported keys are: "description","language", "codec"

Parameter
[in]	index	index of audio stream, valid from 0.

Returned Value
Return audio property in JSON object format: {key£ºvalue£¬key£ºvalue,...}
*/
JSON COSMPEngnWrap::GetAudioProperty(INT index)
{
  if (m_pOSMPPlayer==NULL)
  {
    return _T("");
  }

  voJSON *pvi = voJSON_CreateObject();
  if (pvi == NULL)
    return _T("");

  VOOSMP_SRC_TRACK_PROPERTY *pProperty = NULL;
  int nRC = m_pOSMPPlayer->GetAudioProperty(index,&pProperty);
  if (pProperty == NULL)
	  return _T("");

  if (nRC != VO_OSMP_ERR_NONE)
  {
    if (index<m_pOSMPPlayer->GetAudioCount())
    {
      CHAR szProperty[MAX_PATH] = "Audio  property error";
      CHAR szTemp[10] = "";
      itoa(index,szTemp,10);
      strcat(szProperty,szTemp);

      voJSON_AddStringToObject(pvi, "description", szProperty);
    }
  }
  else
  {
    for (int i=0; i<pProperty->nPropertyCount; ++i)
    {
      VOOSMP_SRC_TRACK_ITEM_PROPERTY* pItemProperties = pProperty->ppItemProperties[i];

      char szProperty[MAX_PATH] = "";
      strcpy(szProperty,pItemProperties->pszProperty);
      if (strstr(pItemProperties->szKey, ("language")))
      {
        for (int m=0; m < (int)strlen(pItemProperties->pszProperty); ++m)
        {
          if (pItemProperties->pszProperty[m] == '-')
          {
            szProperty[m] = '\0';
            break;
          }

          szProperty[m] = pItemProperties->pszProperty[m];
        }
      }

      voJSON_AddStringToObject(pvi, pItemProperties->szKey, szProperty);
    }
  } 

  _tcscpy(m_szJSONString, _T(""));
  char* szval = voJSON_Print(pvi);
  ::MultiByteToWideChar(CP_ACP, 0, szval, -1, m_szJSONString, sizeof(m_szJSONString));
  free(szval);
  voJSON_Delete(pvi);
  return m_szJSONString;
}

/*
Description
Return the properties of specified subtitle stream in JSON object [8.2] format. Supported keys are: ¡°description¡±, ¡°language¡±, ¡°codec¡±

Parameter
[in]	index	index of subtitle stream, valid from 0.

Returned Value
Return subtitle property in JSON object format: {key£ºvalue£¬key£ºvalue,...}
*/
JSON COSMPEngnWrap::GetSubtitleProperty(INT index)
{
  if (m_pOSMPPlayer==NULL)
  {
    return _T("");
  }

  voJSON *pvi = voJSON_CreateObject();
  if (pvi == NULL)
    return _T("");

  VOOSMP_SRC_TRACK_PROPERTY *pProperty = NULL;
  int nRC = m_pOSMPPlayer->GetSubtitleProperty(index,&pProperty);
  if (pProperty == NULL)
	  return _T("");

  if (nRC != VO_OSMP_ERR_NONE)
  {
    if (index<m_pOSMPPlayer->GetSubtitleCount())
    {
      CHAR szProperty[MAX_PATH] = "Subtitle property error";
      CHAR szTemp[10] = "";
      itoa(index,szTemp,10);
      strcat(szProperty,szTemp);

      voJSON_AddStringToObject(pvi, "description", szProperty);
    }
  }
  else
  {
    for (int i=0; i<pProperty->nPropertyCount; ++i)
    {
      VOOSMP_SRC_TRACK_ITEM_PROPERTY* pItemProperties = pProperty->ppItemProperties[i];

      char szProperty[MAX_PATH] = "";
      strcpy(szProperty,pItemProperties->pszProperty);
      if (strstr(pItemProperties->szKey, ("language")))
      {
        for (int m=0; m < (int)strlen(pItemProperties->pszProperty); ++m)
        {
          if (pItemProperties->pszProperty[m] == '-')
          {
            szProperty[m] = '\0';
            break;
          }

          szProperty[m] = pItemProperties->pszProperty[m];
        }
      }

      voJSON_AddStringToObject(pvi, pItemProperties->szKey, szProperty);
    }
  } 

  _tcscpy(m_szJSONString, _T(""));
  char* szval = voJSON_Print(pvi);
  ::MultiByteToWideChar(CP_ACP, 0, szval, -1, m_szJSONString, sizeof(m_szJSONString));
  free(szval);
  voJSON_Delete(pvi);
  return m_szJSONString;
}

/*
Description
Retrieve the assets which are currently being played; or default assets before playback

Parameter

Returned Value
Return indices of currently playing/default assets in JSON object format
*/
JSON COSMPEngnWrap::GetPlayingAsset()
{
	if (m_pOSMPPlayer==NULL)
		return _T("");

	voJSON *root = voJSON_CreateObject();
	if (root == NULL)
		return _T("");

	VOOSMP_SRC_CURR_TRACK_INDEX curIndex;
	curIndex.nCurrAudioIdx = -1;
	curIndex.nCurrVideoIdx = -1;
	curIndex.nCurrSubtitleIdx = -1;
	int nRC = m_pOSMPPlayer->GetCurrPlayingTrackIndex(&curIndex);

	if (nRC == VO_OSMP_ERR_NONE) 
	{
		if (m_nTrackDisabled & 0x001)
			curIndex.nCurrSubtitleIdx = -1;

		if (m_nTrackDisabled & 0x010)
			curIndex.nCurrAudioIdx = -1;

		voJSON_AddNumberToObject(root, "CurAudioIndex", curIndex.nCurrAudioIdx);
		voJSON_AddNumberToObject(root, "CurVideoIndex", curIndex.nCurrVideoIdx);
		voJSON_AddNumberToObject(root, "CurSubtitleIndex", curIndex.nCurrSubtitleIdx);
	} 
	else 
	{
		voJSON_AddNumberToObject(root, "CurAudioIndex", -1);
		voJSON_AddNumberToObject(root, "CurVideoIndex", -1);
		voJSON_AddNumberToObject(root, "CurSubtitleIndex", -1);
	}

	_tcscpy(m_szJSONString, _T(""));
	char* szval = voJSON_Print(root);
	::MultiByteToWideChar(CP_ACP, 0, szval, -1, m_szJSONString, sizeof(m_szJSONString));
	free(szval);
	voJSON_Delete(root);
	return m_szJSONString;
}

/*
Description
Retrieve the current track selection of each type.

Parameter

Returned Value
Return index of currently playing/default assets in JSON object format
*/

JSON COSMPEngnWrap::GetCurrentSelection()
{
	if (m_pOSMPPlayer==NULL)
		return _T("");

	voJSON *root = voJSON_CreateObject();
	if (root == NULL)
		return _T("");

	VOOSMP_SRC_CURR_TRACK_INDEX curIndex;
	curIndex.nCurrAudioIdx = -1;
	curIndex.nCurrVideoIdx = -1;
	curIndex.nCurrSubtitleIdx = -1;
	int nRC = m_pOSMPPlayer->GetCurrSelectedTrackIndex(&curIndex);

	if (nRC == VO_OSMP_ERR_NONE) 
	{
		voJSON_AddNumberToObject(root, "CurAudioIndex", curIndex.nCurrAudioIdx);
		voJSON_AddNumberToObject(root, "CurVideoIndex", curIndex.nCurrVideoIdx);
		voJSON_AddNumberToObject(root, "CurSubtitleIndex", curIndex.nCurrSubtitleIdx);
	} 
	else 
	{
		voJSON_AddNumberToObject(root, "CurAudioIndex", -1);
		voJSON_AddNumberToObject(root, "CurVideoIndex", -1);
		voJSON_AddNumberToObject(root, "CurSubtitleIndex", -1);
	}

	_tcscpy(m_szJSONString, _T(""));
	char* szval = voJSON_Print(root);
	::MultiByteToWideChar(CP_ACP, 0, szval, -1, m_szJSONString, sizeof(m_szJSONString));
	free(szval);
	voJSON_Delete(root);
	return m_szJSONString;
}

/**
* Set Subtitle file full path for external subtitle (e.g. smi, srt files, etc.)
*
* @param  filePath location of the subtitle file
* 
* @return 0 if successful
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::SetSubtitlePath(LPCTSTR filePath)
{
  voCAutoLock lock (&m_Mutex);
  if (m_pOSMPPlayer==NULL)
    return VO_OSMP_ERR_NONE;

  int nRC = m_pOSMPPlayer->SetParam(VOOSMP_PID_SUBTITLE_FILE_NAME,(void*)filePath);
  return (VO_OSMP_RETURN_CODE)nRC;
}

/**                                                             
* To display/hide subtitle, default is false                   
*                                                              
* @param  val	true to display subtitle, false to hide subtitle
*                                                              
* @return 0 if successful                                      
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::EnableSubtitle(bool value)
{
  voCAutoLock lock (&m_Mutex);
  if (m_pOSMPPlayer==NULL)
    return VO_OSMP_ERR_POINTER;

  if (value)
	  m_nTrackDisabled &= 0x110;
  else
	  m_nTrackDisabled |= 0x001;

  int nRC = m_pOSMPPlayer->SetParam(VOOSMP_PID_COMMON_CCPARSER,&value);
  return (VO_OSMP_RETURN_CODE)nRC;
}

/**
* Set subtitle font color
*
* @param color the font color (such as 0x00RRGGBB) of subtitle text 
* 
* @return 0 if successful
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::SetSubtitleFontColor(COLORREF color)
{
  voCAutoLock lock (&m_Mutex);
  if (m_pOSMPPlayer==NULL)
    return VO_OSMP_ERR_POINTER;

  voSubTitleFormatSetting* pSett = NULL;
  int nRC = m_pOSMPPlayer->GetParam(VOOSMP_PID_CLOSED_CAPTION_SETTINGS, & pSett);

  if (pSett)
  {
    pSett->setFontColor(color);
    return VO_OSMP_ERR_NONE;
  }

  return (VO_OSMP_RETURN_CODE)nRC;
}

/**
* Set subtitle font color opacity rate
*
* @param alpha the font color opacity rate. The valid range is 0 to 100, from transparent to opaque
* 
* @return 0 if successful
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::SetSubtitleFontOpacity(int alpha)
{
  voCAutoLock lock (&m_Mutex);
  if (m_pOSMPPlayer==NULL)
    return VO_OSMP_ERR_POINTER;

  voSubTitleFormatSetting* pSett = NULL;
  int nRC = m_pOSMPPlayer->GetParam(VOOSMP_PID_CLOSED_CAPTION_SETTINGS, & pSett);

  if (pSett)
  {
    //pSett->setFontOpacity(alpha);
    MessageBox(NULL,_T("NO IMPLEMENT"), _T("SetSubtitleFontOpacity"),MB_OK);
    return VO_OSMP_ERR_IMPLEMENT;
  }

  return (VO_OSMP_RETURN_CODE)nRC;
}

/**
* Set subtitle font size scale
*
* @param scale the font size scale for subtitle text. The valid range is 50 to 200, 
* where 50 is the smallest and is half of the default size, and 200 is the largest and is twice the default size
* 
* @return 0 if successful
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::SetSubtitleFontSizeScale(int scale)
{
  voCAutoLock lock (&m_Mutex);
  if (m_pOSMPPlayer==NULL)
    return VO_OSMP_ERR_POINTER;

  voSubTitleFormatSetting* pSett = NULL;
  int nRC = m_pOSMPPlayer->GetParam(VOOSMP_PID_CLOSED_CAPTION_SETTINGS, & pSett);

  if (pSett)
  {
    pSett->setFontSizeScale(scale);
    return VO_OSMP_ERR_NONE;
  }

  return (VO_OSMP_RETURN_CODE)nRC;
}

/**
* Set subtitle font background color
*
* @param color the subtitle font background color (such as 0x00RRGGBB)
* 
* @return 0 if successful
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::SetSubtitleFontBackgroundColor(COLORREF color)
{
  voCAutoLock lock (&m_Mutex);
  if (m_pOSMPPlayer==NULL)
    return VO_OSMP_ERR_POINTER;

  voSubTitleFormatSetting* pSett = NULL;
  int nRC = m_pOSMPPlayer->GetParam(VOOSMP_PID_CLOSED_CAPTION_SETTINGS, & pSett);

  if (pSett)
  {
    pSett->setBackgroundColor(color);
    return VO_OSMP_ERR_NONE;
  }

  return (VO_OSMP_RETURN_CODE)nRC;
}

/**
* Set subtitile font background color opacity rate
*
* @param alpha the subtitle font background color opacity rate. The valid range is 0 to 100, from transparent to opaque
* 
* @return 0 if successful
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::SetSubtitleFontBackgroundOpacity(int alpha)
{
  voCAutoLock lock (&m_Mutex);
  if (m_pOSMPPlayer==NULL)
    return VO_OSMP_ERR_POINTER;

  voSubTitleFormatSetting* pSett = NULL;
  int nRC = m_pOSMPPlayer->GetParam(VOOSMP_PID_CLOSED_CAPTION_SETTINGS, & pSett);

  if (pSett)
  {
    //pSett->setBackgroundOpacity(alpha);
    return VO_OSMP_ERR_NONE;
  }

  return (VO_OSMP_RETURN_CODE)nRC;
}

/**
* Set window background color 
*
* @param color the subtitle window background color (such as 0x00RRGGBB)
* 
* @return 0 if successful
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::SetSubtitleWindowBackgroundColor(COLORREF color)
{
  voCAutoLock lock (&m_Mutex);
  if (m_pOSMPPlayer==NULL)
    return VO_OSMP_ERR_POINTER;

  voSubTitleFormatSetting* pSett = NULL;
  int nRC = m_pOSMPPlayer->GetParam(VOOSMP_PID_CLOSED_CAPTION_SETTINGS, & pSett);

  if (pSett)
  {
    pSett->setWindowColor(color);
    return VO_OSMP_ERR_NONE;
  }

  return (VO_OSMP_RETURN_CODE)nRC;
}

/**
* Set window background color opacity rate
*
* @param alpha the subtitle window background color opacity rate. The valid range is 0 to 100, from transparent to opaque
* 
* @return 0 if successful
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::SetSubtitleWindowBackgroundOpacity(int alpha)
{
  voCAutoLock lock (&m_Mutex);
  if (m_pOSMPPlayer==NULL)
    return VO_OSMP_ERR_POINTER;

  voSubTitleFormatSetting* pSett = NULL;
  int nRC = m_pOSMPPlayer->GetParam(VOOSMP_PID_CLOSED_CAPTION_SETTINGS, & pSett);

  if (pSett)
  {
    pSett->setWindowOpacity(alpha);
    return VO_OSMP_ERR_NONE;
  }

  return (VO_OSMP_RETURN_CODE)nRC;
}

/**
* Set subtitle font italic
*
* @param   enable true to set subtitle font italic
* 
* @return 0 if successful
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::SetSubtitleFontItalic(bool enable)
{
  voCAutoLock lock (&m_Mutex);
  if (m_pOSMPPlayer==NULL)
    return VO_OSMP_ERR_POINTER;

  voSubTitleFormatSetting* pSett = NULL;
  int nRC = m_pOSMPPlayer->GetParam(VOOSMP_PID_CLOSED_CAPTION_SETTINGS, & pSett);

  if (pSett)
  {
    pSett->setFontItalic(enable);
    return VO_OSMP_ERR_NONE;
  }

  return (VO_OSMP_RETURN_CODE)nRC;
}

/**
* Set subtitle font bold
*
* @param   enable true to set subtitle font bold
* 
* @return 0 if successful
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::SetSubtitleFontBold(bool enable)
{
  voCAutoLock lock (&m_Mutex);
  if (m_pOSMPPlayer==NULL)
    return VO_OSMP_ERR_POINTER;

  voSubTitleFormatSetting* pSett = NULL;
  int nRC = m_pOSMPPlayer->GetParam(VOOSMP_PID_CLOSED_CAPTION_SETTINGS, & pSett);

  if (pSett)
  {
    pSett->setFontBold(enable);
    return VO_OSMP_ERR_NONE;
  }

  return (VO_OSMP_RETURN_CODE)nRC;
}

/**
* Set subtitle font underlined 
*
* @param   enable true to set subtitle font underlined
* 
* @return 0 if successful
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::SetSubtitleFontUnderline(bool enable)
{
  voCAutoLock lock (&m_Mutex);
  if (m_pOSMPPlayer==NULL)
    return VO_OSMP_ERR_POINTER;

  voSubTitleFormatSetting* pSett = NULL;
  int nRC = m_pOSMPPlayer->GetParam(VOOSMP_PID_CLOSED_CAPTION_SETTINGS, & pSett);

  if (pSett)
  {
    pSett->setFontUnderline(enable);
    return VO_OSMP_ERR_NONE;
  }

  return (VO_OSMP_RETURN_CODE)nRC;
}

/**
* Set subtitle font name
*
* @param fontname 		the font name for subtitle text
* 
* @return 0 if successful
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::SetSubtitleFontName(LPCTSTR name)
{
  voCAutoLock lock (&m_Mutex);

  if (m_pOSMPPlayer==NULL)
    return VO_OSMP_ERR_POINTER;

  voSubTitleFormatSetting* pSett = NULL;
  int nRC = m_pOSMPPlayer->GetParam(VOOSMP_PID_CLOSED_CAPTION_SETTINGS, & pSett);

  if (pSett)
  {
    pSett->setFontName((LPTSTR)name);
    return VO_OSMP_ERR_NONE;
  }

  return (VO_OSMP_RETURN_CODE)nRC;
}

/**
* Set subtitle font edge type
*
* @param edgetype the edge type of subtitle font, for details, please refer the relative values defined in VOOSMPConstant.js
* 
* @return 0 if successful
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::SetSubtitleFontEdgeType(int type)
{
  voCAutoLock lock (&m_Mutex);

  if (m_pOSMPPlayer==NULL)
    return VO_OSMP_ERR_POINTER;

  voSubTitleFormatSetting* pSett = NULL;
  int nRC = m_pOSMPPlayer->GetParam(VOOSMP_PID_CLOSED_CAPTION_SETTINGS, & pSett);

  if (pSett)
  {
    pSett->setEdgeType(type);
    return VO_OSMP_ERR_NONE;
  }

  return (VO_OSMP_RETURN_CODE)nRC;
}

/**
* Set subtitle font edge color
*
* @param color 	the font edge color (such as 0x00RRGGBB) of subtitle text 
* 
* @return 0 if successful
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::SetSubtitleFontEdgeColor(COLORREF color)
{
  voCAutoLock lock (&m_Mutex);
  if (m_pOSMPPlayer==NULL)
    return VO_OSMP_ERR_POINTER;

  voSubTitleFormatSetting* pSett = NULL;
  int nRC = m_pOSMPPlayer->GetParam(VOOSMP_PID_CLOSED_CAPTION_SETTINGS, & pSett);

  if (pSett)
  {
    pSett->setEdgeColor(color);
    return VO_OSMP_ERR_NONE;
  }

  return (VO_OSMP_RETURN_CODE)nRC;
}

/**
* Set subtitle font edge color opacity rate
*
* @param alpha the edge color opacity rate of subtitle font. The valid range is 0 to 100, from transparent to opaque
* 
* @return 0 if successful
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::SetSubtitleFontEdgeOpacity(int type)
{
  voCAutoLock lock (&m_Mutex);
  if (m_pOSMPPlayer==NULL)
    return VO_OSMP_ERR_POINTER;

  voSubTitleFormatSetting* pSett = NULL;
  int nRC = m_pOSMPPlayer->GetParam(VOOSMP_PID_CLOSED_CAPTION_SETTINGS, & pSett);

  if (pSett)
  {
    //   pSett->setEdgeOpacity(type);
    MessageBox(NULL,_T("NO IMPLEMENT"), _T("SetSubtitleFontEdgeOpacity"),MB_OK);
    return VO_OSMP_ERR_IMPLEMENT;
  }

  return (VO_OSMP_RETURN_CODE)nRC;
}

/**
* Reset all parameters to their default values. Subtitles will be presented as specified in the subtitle stream 
* 
* @return 0 if successful
*/
VO_OSMP_RETURN_CODE COSMPEngnWrap::ResetSubtitleParameter()
{
  voCAutoLock lock (&m_Mutex);
  if (m_pOSMPPlayer==NULL)
    return VO_OSMP_ERR_POINTER;

  voSubTitleFormatSetting* pSett = NULL;
  int nRC = m_pOSMPPlayer->GetParam(VOOSMP_PID_CLOSED_CAPTION_SETTINGS, & pSett);

  if (pSett)
  {
    pSett->reset();
    return VO_OSMP_ERR_NONE;
  }

  return (VO_OSMP_RETURN_CODE)nRC;
}

int COSMPEngnWrap::OnOSMPListener (void * pUserData, int nID, void * pParam1, void * pParam2)
{
	if (pUserData == NULL)
		return VO_OSMP_ERR_POINTER;

  COSMPEngnWrap * pPlayer = (COSMPEngnWrap *)pUserData;
  if(FALSE == pPlayer->m_voPlugInitParam.bWindowless)
    return pPlayer->OSMPWndHandleEvent (nID, pParam1, pParam2);
  else
    return pPlayer->OSMPWndlessHandleEvent (nID, pParam1, pParam2);
}


//return VOOSMP_ERR_Implement will notify this event to webpage, rerurn 0 will do not notify it.
int COSMPEngnWrap::OSMPCommonHandleEvent(int nID, void * pParam1, void * pParam2)
{
	if (m_pOSMPPlayer)
	{
		m_pOSMPPlayer->OnListener((void *)m_pOSMPPlayer,nID,pParam1,pParam2);
	}

	switch(nID)
	{
	case VOOSMP_CB_MediaTypeChanged:
		{
			cbMediaTypeChanged(pParam1, pParam2);
			break;;//need notify webpage
		}
	case VOOSMP_SRC_CB_Authentication_Request:
		{
			cbSrcAuthenticationRequest(pParam1, pParam2);
			break;;//need notify webpage
		}
  case VOOSMP_SRC_CB_IO_HTTP_Start_Download:
    {
      if(strlen(m_szUserAgentHeaderValue)>0)
        SetHTTPHeader(USER_AGNET_NAME,m_szUserAgentHeaderValue);
      break;
    }
  case VOOSMP_SRC_CB_IO_HTTP_Download_Failed:
  {
    if (pParam2 == NULL){
      break;
    }
    VOOSMP_SRC_IO_FAILED_REASON_DESCRIPTION *description = (VOOSMP_SRC_IO_FAILED_REASON_DESCRIPTION *)pParam2;
    if (description->reason == VOOSMP_IO_HTTP_CLIENT_ERROR) {
      if (pParam1 != NULL)
      {
        VOLOGI("[APP]HTTP error: reason %d, response %s, url %s", description->reason, description->pszResponse, pParam1);
      }
    }
  }
  break;

#if VOPERF_MONITOR
	case VOOSMP_CB_SeekComplete:
		{
			VOLOGI("[Seek] completed @ %d", voOS_GetSysTime());
		}
		break;
#endif

	default:
		break;
	}

	return VOOSMP_ERR_Implement;
}

int COSMPEngnWrap::OSMPWndlessHandleEvent (int nID, void * pParam1, void * pParam2)
{
	if (NULL == m_pOSMPPlayer)
	{
		return VO_OSMP_ERR_POINTER;
	}

	if(FALSE == m_voPlugInitParam.bWindowless)
	{
		VOLOGE("Error function callback!");
		return 0;
	}

	int nRC = OSMPCommonHandleEvent(nID, pParam1, pParam2);
	if(VOOSMP_ERR_Implement != nRC)
		return nRC;

	switch(nID)
	{
	case VOOSMP_CB_Metadata_Arrive: 
		{//https://sh.visualon.com/node/28737
			notifyWebPageCommand(VOMP_CB_VideoReadyToRender,NULL,NULL);
			return 0;
		}

	default:
		break;

	}

	notifyWebPageCommand(nID, pParam1, pParam2);
	return 0;
}

int COSMPEngnWrap::OSMPWndHandleEvent (int nID, void * pParam1, void * pParam2)
{
  if (NULL == m_pOSMPPlayer)
  {
    return VO_OSMP_ERR_POINTER;
  }

  if(TRUE == m_voPlugInitParam.bWindowless)
  {
    VOLOGE("Error Listerner");
    return 0;
  }

  int nRC = OSMPCommonHandleEvent(nID, pParam1, pParam2);
  if(VOOSMP_ERR_Implement != nRC)
    return nRC;

  switch(nID)
  {
  case VOOSMP_CB_VideoStartBuff:
  case VOOSMP_CB_AudioStartBuff:
    {
       BeginBuffering(nID);
       break;//need notify webpage
    }
  case VOOSMP_CB_AudioStopBuff:
  case VOOSMP_CB_VideoStopBuff:
    {
      EndBuffering(nID);
      break;//need notify webpage
    }
  case VOOSMP_CB_VR_USERCALLBACK://this event should not notify webpage.
      {
          notifyUIEventInfo(nID,pParam1,pParam2);
          return VO_OSMP_ERR_NONE;
      }
  case VOOSMP_CB_PlayComplete:
  case VOOSMP_CB_CodecNotSupport:
  case VOOSMP_CB_LicenseFailed:
  case VOOSMP_CB_Video_Render_Complete:
  case VOOSMP_SRC_CB_Adaptive_Streaming_Error:
  case VOOSMP_SRC_CB_Open_Finished:
  case VOOSMP_CB_VideoRenderStart:
  case VOOSMP_CB_AudioRenderStart:
  case VOOSMP_CB_SubtitleRenderStart:
    {
      notifyUIEventInfo(nID,pParam1,pParam2);
    }
    break;
  case VOOSMP_SRC_CB_Adaptive_Streaming_Info:
      {
          if (NULL == pParam1)
              break;

          int nMessage = *(int *)pParam1;
          if (nMessage == VOOSMP_SRC_ADAPTIVE_STREAMING_INFO_EVENT_LIVESEEKABLE)
          {
              VOLOGI("Receive seeking available event.");
              notifyUIEventInfo(VOOSMP_SRC_CB_Adaptive_Streaming_Info,pParam1,pParam2);
          }
      }
      break;
  default:
    break;
  }

  //now, notify all the enent to webpage
  notifyWebPageCommand(nID, pParam1, pParam2);
  return VO_OSMP_ERR_NONE;
}

int COSMPEngnWrap::cbMediaTypeChanged(void * pParam1, void * pParam2)
{
  if (pParam1 == NULL) 
    return VO_OSMP_ERR_NONE;

  VOOSMP_AVAILABLE_TRACK_TYPE trackType = (VOOSMP_AVAILABLE_TRACK_TYPE)(*(int *)pParam1);
  if (trackType == VOOSMP_AVAILABLE_PUREAUDIO)
  {
    if(!m_voPlugInitParam.bWindowless)
    {
      notifyUIEventInfo(VOOSMP_CB_MediaTypeChanged,pParam1,pParam2);
    }
    else
    {
      VOLOGI("pure audio!");
      notifyWebPageCommand(VOMP_CB_VideoReadyToRender,NULL,NULL);//if not do this, will have this bug: https://sh.visualon.com/node/31043
    }
  }
  else
  {

  }

  return VO_OSMP_ERR_NONE;
}

VOOSMP_SRC_PROGRAM_TYPE COSMPEngnWrap::GetProgramType()
{
  if (m_pOSMPPlayer==NULL)
    return VOOSMP_SRC_PROGRAM_TYPE_UNKNOWN;

  VOOSMP_SRC_PROGRAM_INFO *pProgramInfo = NULL;
  int nRC = m_pOSMPPlayer->GetProgramInfo(0, &pProgramInfo);

  if(pProgramInfo)
  {
    return pProgramInfo->nProgramType;
  }

  return VOOSMP_SRC_PROGRAM_TYPE_UNKNOWN;
}

void COSMPEngnWrap::notifyUIEventInfo(int nID, void * pParam1, void * pParam2)
{
  if(!m_voPlugInitParam.bWindowless)
  {
    VOCBMSG cbMsg;
    cbMsg.nID = nID;
	if(nID == VOOSMP_CB_Video_Render_Complete || nID == VOOSMP_CB_VR_USERCALLBACK)
	{
		cbMsg.nValue1 = pParam1==NULL ? 0 : (int )pParam1;
		cbMsg.nValue2 = pParam2==NULL ? 0 : (int )pParam2;
	}
	else
	{
		cbMsg.nValue1 = pParam1==NULL ? 0 : *(int *)pParam1;
		cbMsg.nValue2 = pParam2==NULL ? 0 : *(int *)pParam2;

        VOLOGI("nID 0x%08x param1 %d param2 %d", nID, cbMsg.nValue1, cbMsg.nValue2);
	}

    if(m_pOverlayUI)
        m_pOverlayUI->SetParam(VOOSMP_PLUGIN_CB_EVENT_INFO,&cbMsg);
    else
        VOLOGE("UI window is null!");
  }
}

void COSMPEngnWrap::EraseBackGround()
{
	if (!m_voPlugInitParam.bWindowless) {
		HDC hdc = GetDC(m_hWndView);
		RECT	lRect;
		GetClientRect(m_hWndView,&lRect);
		HBRUSH brhBackground = (HBRUSH)::CreateSolidBrush (RGB_DEFAULT_BKG);
		::FillRect(hdc,&lRect,brhBackground);
		::DeleteObject(brhBackground);
	}
}

BOOL CALLBACK EnumChildProc(HWND hwndChild, LPARAM lParam)
{
  if(lParam==NULL)
    return FALSE;

  HWND hStausBarWnd = FindWindowEx(hwndChild,NULL,_T("msctls_statusbar32"),NULL);
  if(IsWindow(hStausBarWnd))
  {
    if(IsWindowVisible(hStausBarWnd))
    {
      RECT rcBar;
      GetClientRect (hStausBarWnd, &rcBar);

      RECT * prcView = (RECT*)lParam;
      prcView->bottom += (rcBar.bottom-rcBar.top);
    }

    return FALSE; 
  }

  return TRUE;
}

BOOL COSMPEngnWrap::IsFullScreen ()
{
  RECT rcView;
  if (m_voPlugInitParam.bWindowless)
  {
    rcView = m_voPlugInitParam.rcDraw;

    BOOL bIE = m_nBrowserType ? false : true;
    if (bIE)//maybe the staus bar will be shown when fullscreen
    {
      HWND hIEFrame = FindWindow(_T("IEFrame"),NULL);
      EnumChildWindows(hIEFrame, EnumChildProc, (LPARAM)(&rcView));
    }
  }
  else
  {
    GetClientRect (m_hWndView, &rcView);
  }

  int nvw = rcView.right - rcView.left;
  int nvh = rcView.bottom - rcView.top;

  //maybe there are more than one monitors (extend monitor)
  HMONITOR hMonitor = MonitorFromWindow(m_hWndView,MONITOR_DEFAULTTONULL);
  MONITORINFO monitorInfo;
  memset(&monitorInfo, 0,sizeof(monitorInfo));
  monitorInfo.cbSize = sizeof(MONITORINFO);
  BOOL bRet = GetMonitorInfo(hMonitor,&monitorInfo); 
  int nScreenWidth = bRet ? (monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left) : GetSystemMetrics (SM_CXSCREEN);
  int nScreenHeight = bRet ? (monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top) : GetSystemMetrics (SM_CYSCREEN);
  if (abs(nScreenWidth-nvw)<5 && abs(nvh - nScreenHeight)<5)//windowless,the rcView sometimes has a little error, about 1 pix
    return TRUE;
  
  return FALSE;
}

VO_OSMP_RETURN_CODE COSMPEngnWrap::EnableAnalytics(const int cacheTime)
{
	if (m_pOSMPPlayer==NULL)
		return VO_OSMP_ERR_UNINITIALIZE;

	int ret = 0;

	VOMP_PERFORMANCEDATA pd;
	memset(&pd, 0, sizeof(VOMP_PERFORMANCEDATA));
	pd.nLastTime = cacheTime;
	ret = m_pOSMPPlayer->SetParam(VOOSMP_PID_VIDEO_PERFORMANCE_OPTION, &pd);

	VOLOGI("set cache time return: 0x%08x", ret)
	int nEnable = 1;
	ret |= m_pOSMPPlayer->SetParam(VOOSMP_PID_VIDEO_PERFORMANCE_ONOFF, &nEnable);
	VOLOGI("enable analytics return: 0x%08x", ret)
	return (VO_OSMP_RETURN_CODE)ret;
}

VO_OSMP_RETURN_CODE COSMPEngnWrap::StartAnalyticsNotification(const int interval, const char* filter)
{
	if (m_pOSMPPlayer==NULL || filter == NULL )
		return VO_OSMP_ERR_UNINITIALIZE;

	VOMP_PERFORMANCEDATA pd;
	memset(&pd, 0, sizeof(VOMP_PERFORMANCEDATA));
	pd.nLastTime = interval;

	char pbuf[MAX_PATH];
	memset(pbuf, 0, sizeof(char)*MAX_PATH);
	strcpy(pbuf, filter);

	voJSON * pjf = voJSON_Parse(pbuf);
	int iSize = voJSON_GetArraySize(pjf);
	for(int i = 0; i<iSize; ++i)
	{
		voJSON * pj = voJSON_GetArrayItem(pjf, i);
		if (pj == NULL || pj->string == NULL || strlen(pj->string) <= 0)
			break;
		
		if (stricmp(pj->string, "lastTime") == 0)
		{
			pd.nLastTime = atoi(pj->valuestring);
		}
		else if (stricmp(pj->string, "sourceTime") == 0)
		{
			pd.nSourceTimeNum = atoi(pj->valuestring);
		}
		else if (stricmp(pj->string,"codecTime") == 0)
		{
			pd.nCodecTimeNum = atoi(pj->valuestring);
		}
		else if (stricmp(pj->string,"renderTime") == 0)
		{
			pd.nRenderTimeNum = atoi(pj->valuestring);
		} 
		else if (stricmp(pj->string,"jitter") == 0)
		{
			pd.nJitterNum = atoi(pj->valuestring);
		}  
	}

	int ret = m_pOSMPPlayer->SetParam(VOOSMP_PID_VIDEO_PERFORMANCE_OPTION, &pd);
	VOLOGI("start analytics return: 0x%08x", ret)
	return (VO_OSMP_RETURN_CODE)ret;
}

VO_OSMP_RETURN_CODE COSMPEngnWrap::StopAnalyticsNotification()
{
	if (m_pOSMPPlayer==NULL)
		return VO_OSMP_ERR_UNINITIALIZE;

	int nEnable = 0;
	int ret = m_pOSMPPlayer->SetParam(VOOSMP_PID_VIDEO_PERFORMANCE_ONOFF, &nEnable);
	VOLOGI("stop analytics return: 0x%08x", ret)
	return (VO_OSMP_RETURN_CODE)ret;
}

const char* COSMPEngnWrap::GetAnalytics(const char* filter)
{
	if (m_pOSMPPlayer == NULL)
		return "";

	int ret = 0;
	VOMP_PERFORMANCEDATA pd;
	memset(&pd, 0, sizeof(VOMP_PERFORMANCEDATA));

	if (filter) {
		char pbuf[MAX_PATH];
		memset(pbuf, 0, sizeof(char)*MAX_PATH);
		strcpy(pbuf, filter);

		voJSON * pjf = voJSON_Parse(pbuf);
		int iSize = voJSON_GetArraySize(pjf);
		for(int i = 0; i<iSize; ++i)
		{
			voJSON * pj = voJSON_GetArrayItem(pjf, i);
			if (pj == NULL || pj->string == NULL || strlen(pj->string) <= 0)
				break;

			if (stricmp(pj->string, "lastTime") == 0)
			{
				pd.nLastTime = atoi(pj->valuestring);
			}
			else if (stricmp(pj->string, "sourceTime") == 0)
			{
				pd.nSourceTimeNum = atoi(pj->valuestring);
			}
			else if (stricmp(pj->string,"codecTime") == 0)
			{
				pd.nCodecTimeNum = atoi(pj->valuestring);
			}
			else if (stricmp(pj->string,"renderTime") == 0)
			{
				pd.nRenderTimeNum = atoi(pj->valuestring);
			} 
			else if (stricmp(pj->string,"jitter") == 0)
			{
				pd.nJitterNum = atoi(pj->valuestring);
			}  
		}

		ret = m_pOSMPPlayer->SetParam(VOOSMP_PID_VIDEO_PERFORMANCE_OPTION, &pd);
	}

	ret |= m_pOSMPPlayer->GetParam(VOOSMP_PID_VIDEO_PERFORMANCE_OPTION, &pd);
	if (ret != VO_OSMP_ERR_NONE)
		return "";

	voJSON *pana = voJSON_CreateObject();
	if (pana == NULL)
		return NULL;

	voJSON_AddNumberToObject(pana, "lastTime", pd.nLastTime);
	voJSON_AddNumberToObject(pana, "sourceDropNum", pd.nSourceDropNum);
	voJSON_AddNumberToObject(pana, "codecDropNum", pd.nCodecDropNum);
	voJSON_AddNumberToObject(pana, "renderDropNum", pd.nRenderDropNum);
	voJSON_AddNumberToObject(pana, "decodedNum", pd.nDecodedNum);
	voJSON_AddNumberToObject(pana, "renderNum", pd.nRenderNum);
	voJSON_AddNumberToObject(pana, "sourceTimeNum", pd.nSourceTimeNum);
	voJSON_AddNumberToObject(pana, "codecTimeNum", pd.nCodecTimeNum);
	voJSON_AddNumberToObject(pana, "renderTimeNum", pd.nRenderTimeNum);
	voJSON_AddNumberToObject(pana, "jitterNum", pd.nJitterNum);
	voJSON_AddNumberToObject(pana, "codecErrorsNum", pd.nCodecErrorsNum);
	if (pd.nCodecErrorsNum > 0)
		voJSON_AddItemToObject(pana, "codecErrors", voJSON_CreateIntArray(pd.nCodecErrors, pd.nCodecErrorsNum));
	else
		voJSON_AddStringToObject(pana, "codecErrors", "none errors");
	voJSON_AddNumberToObject(pana, "CPULoad", pd.nCPULoad);
	voJSON_AddNumberToObject(pana, "frequency", pd.nFrequency);
	voJSON_AddNumberToObject(pana, "maxFrequency", pd.nMaxFrequency);
	voJSON_AddNumberToObject(pana, "worstDecodeTime", pd.nWorstDecodeTime);
	voJSON_AddNumberToObject(pana, "worstRenderTime", pd.nWorstRenderTime);
	voJSON_AddNumberToObject(pana, "averageDecodeTime", pd.nAverageDecodeTime);
	voJSON_AddNumberToObject(pana, "averageRenderTime", pd.nAverageRenderTime);
	voJSON_AddNumberToObject(pana, "totalCPULoad", pd.nTotalCPULoad);
	voJSON_AddNumberToObject(pana, "playbackDuration", pd.nTotalPlaybackDuration);
    voJSON_AddNumberToObject(pana, "totalSourceDropNum", pd.nTotalSourceDropNum);
    voJSON_AddNumberToObject(pana, "totalCodecDropNum", pd.nTotalCodecDropNum);
    voJSON_AddNumberToObject(pana, "totalRenderDropNum", pd.nTotalRenderDropNum);
    voJSON_AddNumberToObject(pana, "totalDecodedNum", pd.nTotalDecodedNum);
    voJSON_AddNumberToObject(pana, "totalRenderedNum", pd.nTotalRenderNum);

	char* szval = voJSON_Print(pana);
	strcpy(m_szAnalyticsInfo, szval);
	VOLOGI("got analytics info: %s", m_szAnalyticsInfo);
	free(szval);
	voJSON_Delete(pana);
	return m_szAnalyticsInfo;
}

VO_OSMP_RETURN_CODE	COSMPEngnWrap::Redraw(BOOL bDraw) {

	voCAutoLock lock (&m_Mutex);
  if(m_pOSMPPlayer ==NULL)
    return VO_OSMP_ERR_POINTER;

	int ret = m_pOSMPPlayer->Redraw(bDraw);
	return (VO_OSMP_RETURN_CODE)ret;
}

VO_OSMP_RETURN_CODE	COSMPEngnWrap::updateViewRegion(RECT& r) 
{
  voCAutoLock lock (&m_Mutex);
  if(m_pOSMPPlayer ==NULL)
    return VO_OSMP_ERR_POINTER;

	int ret = 0;
	if (m_pOSMPPlayer) {
		m_voPlugInitParam.rcDraw = r;
		int w = r.right - r.left;
		w = w / 16 * 16;
		m_voPlugInitParam.rcDraw.right = r.left + w;
		ret = m_pOSMPPlayer->updateViewRegion(m_voPlugInitParam.rcDraw);
	}

	return (VO_OSMP_RETURN_CODE)ret;
}

VO_OSMP_RETURN_CODE COSMPEngnWrap::SetHTTPHeader(CHAR* headerName, CHAR* headerValue)
{
  //https://sh.visualon.com/node/29830 [Window]ANR if input a link and tap "Commit" several times(about 5)
//  voCAutoLock lock (&m_Mutex);

  //VOLOGI("begin");

  if (headerName==NULL || headerValue==NULL || (stricmp(headerName,USER_AGNET_NAME)!=0 && stricmp(headerName,SET_COOKIE_NAME)!=0))
  {
    return VO_OSMP_ERR_UNKNOWN;
  }

  if(stricmp(headerName, USER_AGNET_NAME) == 0)
  {
    strcpy(m_szUserAgentHeaderValue, headerValue);
  }

  int nRC = VO_OSMP_ERR_UNKNOWN;
  
  if(m_pOSMPPlayer)
  {
    nRC = m_pOSMPPlayer->SetHTTPHeader(headerName,headerValue);
  }

  //VOLOGI("end");
  return (VO_OSMP_RETURN_CODE)nRC;
}

//eg:  {\"pszProxyHost\":\"httphost\", \"nProxyPort\":\"80\",\"uFlag\":\"1\",\"pFlagData\":\"123\"}
VO_OSMP_RETURN_CODE COSMPEngnWrap::SetHTTPProxy(JSON proxy)
{
	voCAutoLock lock (&m_Mutex);

	if (proxy==NULL || _tcslen(proxy)<=0)
	{
		return VO_OSMP_ERR_UNKNOWN;
	}

	CHAR szProxy[MAX_PATH] = "";
	WideCharToMultiByte(CP_ACP,NULL,proxy,-1,szProxy,MAX_PATH,NULL,FALSE);

	voJSON * voJsonProxy = voJSON_Parse(szProxy);
	memset(&m_proxy, 0, sizeof(VOOSMP_SRC_HTTP_PROXY));

	int iSize = voJSON_GetArraySize(voJsonProxy);
	for(int i = 0; i<iSize; ++i)
	{
		voJSON * temp = voJSON_GetArrayItem(voJsonProxy, i);
		int size = strlen(temp->string);
		if (temp==NULL || temp->string == NULL || size <=0)
		{
			break;
		}

		size = strlen(temp->valuestring);
		if (stricmp(temp->string,"pszProxyHost") == 0)
		{
			m_proxy.pszProxyHost = (unsigned char*)malloc(size + 1);
			memset(m_proxy.pszProxyHost, 0, size + 1);
			memcpy(m_proxy.pszProxyHost, temp->valuestring, size);
		}
		else if (stricmp(temp->string,"nProxyPort") == 0)
		{
			m_proxy.nProxyPort = atoi(temp->valuestring);
		}
		else if (stricmp(temp->string,"uFlag") == 0)
		{
			m_proxy.uFlag = atoi(temp->valuestring);
		}
		else if (stricmp(temp->string,"pFlagData") == 0)
		{
			m_proxy.pFlagData = malloc(size);
			memcpy(m_proxy.pFlagData, temp->valuestring, size);
		}     
	}

	int nRC = internalSetHttpProxy();

	m_bProxySetup = TRUE;
	voJSON_Delete(voJsonProxy);
	return (VO_OSMP_RETURN_CODE)nRC;
}

void COSMPEngnWrap::BeginBuffering(int nID)
{
  if(nID!= VOOSMP_CB_VideoStartBuff && nID!=VOOSMP_CB_AudioStartBuff)
    return;

  VOLOGI("Begin");

  notifyUIEventInfo(nID,NULL,NULL);
}

void COSMPEngnWrap::EndBuffering(int nID)
{
  if(nID!= VOOSMP_CB_VideoStopBuff && nID!=VOOSMP_CB_AudioStopBuff)
    return;

  VOLOGI("End");
  notifyUIEventInfo(nID,NULL,NULL);
}

void COSMPEngnWrap::detectFFConfigureFile() {

	TCHAR   szPath[1024];
	SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, szPath);

	CString strFFPref(szPath);
	if (strFFPref.Right(1) != '\\')
		strFFPref += _T("\\");
	strFFPref += _T("Mozilla\\Firefox\\");

	CString strFFIni = strFFPref;
	strFFIni += _T("profiles.ini");
	GetPrivateProfileString(_T("Profile0"), _T("Path"), _T(""), szPath, sizeof(szPath), strFFIni.GetBuffer(strFFIni.GetLength()));

	strFFPref += szPath;
	if (strFFPref.Right(1) != '\\')
		strFFPref+=_T("\\");
	
	strFFPref += _T("prefs.js");
	m_cfgFirefox = strFFPref;
}

void COSMPEngnWrap::detectProxy() {

	// firstly detect system proxy setting
	DWORD ipiSize = 0;
	InternetQueryOption(NULL, INTERNET_OPTION_PROXY, NULL, &ipiSize);	
	char ipiBuf[1024];
	INTERNET_PROXY_INFO* ipi = (INTERNET_PROXY_INFO*)ipiBuf;

	if (!InternetQueryOption(NULL, INTERNET_OPTION_PROXY, ipiBuf, &ipiSize)) 
		return;

	if (ipi->dwAccessType == INTERNET_OPEN_TYPE_PROXY) {

		char* host = strtok((char*)ipi->lpszProxy, ":");
		if (host) {
			int size = strlen(host);
			m_proxy.pszProxyHost = (unsigned char*)malloc(size + 1);
			m_proxy.pszProxyHost[size] = 0;
			memcpy(m_proxy.pszProxyHost, host, size);
		}

		char* port = strtok(NULL, ":");
		if (port)
			m_proxy.nProxyPort = atoi(port);
		
		//VOLOGI("[Proxy] host: %s, port: %d", m_proxy.pszProxyHost, m_proxy.nProxyPort);
		m_bProxySetup = TRUE;
	} else {
		m_bProxySetup = FALSE;
		//VOLOGI("[proxy type: %d ] -- { 0: registry config; 1: direct to net; 3: via named proxy; 4: prevent using java/script/INS", ipi->dwAccessType);
	}

	// if firefox
	if (m_nBrowserType == 2) {

		std::ifstream in_stream;
		in_stream.open(m_cfgFirefox);
		if (in_stream.fail())
			return;

		std::string item;
		bool bTypeFound = false;
		while (std::getline(in_stream, item)) {

			if (std::string::npos != item.find("user_pref(\"network.proxy.type\",")) {

				std::string subitem = item.substr(item.find(", ") + 2, 1);
				bTypeFound = true;
				m_ffProxy.type = atoi(subitem.c_str());
				//VOLOGI("this: %p, Firefox proxy settype: %d", this, m_ffProxy.type);
				if (m_ffProxy.type == 0)
					m_bProxySetup = false;
			
			} else if (std::string::npos != item.find("user_pref(\"network.proxy.share_proxy_settings\",")) {

				int start = item.find(" ", strlen("user_pref(\"network.proxy.share_proxy_settings\",")) + 1;
				int len = item.find(")", start) - start;
				std::string subitem = item.substr(start, len);
				if (subitem.compare("true"))
					m_ffProxy.share_proxy_setting = true;
				else
					m_ffProxy.share_proxy_setting = false;
			} else if (std::string::npos != item.find("user_pref(\"network.proxy.autoconfig_url\",")) {
			
				int start = item.find("\"", strlen("user_pref(\"network.proxy.autoconfig_url\",")) + 1;
				int len = item.find("\"", start) - start;
				std::string subitem = item.substr(start, len);
				m_ffProxy.autoconfig_url = (char*)malloc(len + 1);
				memset(m_ffProxy.autoconfig_url, 0, len + 1);
				if (m_ffProxy.autoconfig_url)
					strncpy(m_ffProxy.autoconfig_url, subitem.c_str(), len);

			} else if (std::string::npos != item.find("user_pref(\"network.proxy.ftp\",")) {
				
				int start = item.find("\"", strlen("user_pref(\"network.proxy.ftp\",")) + 1;
				int len = item.find("\"", start) - start;
				std::string subitem = item.substr(start, len);

				strcpy(m_ffProxy.ftp, subitem.c_str());
			} else if (std::string::npos != item.find("user_pref(\"network.proxy.ftp_port\",")) {

				int start = item.find(" ", strlen("user_pref(\"network.proxy.ftp_port\",")) + 1;
				int len = item.find(")", start) - start;
				std::string subitem = item.substr(start, len);
				m_ffProxy.ftp_port = atoi(subitem.c_str());
			} else if (std::string::npos != item.find("user_pref(\"network.proxy.http\",")) {

				int start = item.find("\"", strlen("user_pref(\"network.proxy.http\",")) + 1;
				int len = item.find("\"", start) - start;
				std::string subitem = item.substr(start, len);

				strcpy(m_ffProxy.http, subitem.c_str());
			} else if (std::string::npos != item.find("user_pref(\"network.proxy.http_port\",")) {

				int start = item.find(" ", strlen("user_pref(\"network.proxy.http_port\",")) + 1;
				int len = item.find(")", start) - start;
				std::string subitem = item.substr(start, len);
				m_ffProxy.http_port = atoi(subitem.c_str());
			} else if (std::string::npos != item.find("user_pref(\"network.proxy.socks\",")) {

				int start = item.find("\"", strlen("user_pref(\"network.proxy.socks\",")) + 1;
				int len = item.find("\"", start) - start;
				std::string subitem = item.substr(start, len);
				strcpy(m_ffProxy.socks, subitem.c_str());
			} else if (std::string::npos != item.find("user_pref(\"network.proxy.socks_port\",")) {
				
				int start = item.find(" ", strlen("user_pref(\"network.proxy.socks_port\",")) + 1;
				int len = item.find(")", start) - start;
				std::string subitem = item.substr(start, len);
				m_ffProxy.socks_port = atoi(subitem.c_str());
			} else if (std::string::npos != item.find("user_pref(\"network.proxy.ssl\",")) {

				int start = item.find("\"", strlen("user_pref(\"network.proxy.ssl\",")) + 1;
				int len = item.find("\"", start) - start;
				std::string subitem = item.substr(start, len);
				strcpy(m_ffProxy.ssl, subitem.c_str());
			} else if (std::string::npos != item.find("user_pref(\"network.proxy.ssl_port\",")) {

				int start = item.find(" ", strlen("user_pref(\"network.proxy.ssl_port\",")) + 1;
				int len = item.find(")", start) - start;
				std::string subitem = item.substr(start, len);
				m_ffProxy.ssl_port = atoi(subitem.c_str());
			}
		}
		in_stream.close();

		if (!bTypeFound) {

			// note: if not found the 'type' item, it should be 3 - use system setting;
			// and if system(ie/chrome) set proxy, then just use it, else it just should be direct to net
			if (m_bProxySetup)
				m_ffProxy.type = 3;
			else 
				m_ffProxy.type = 0;
		}

		if (m_ffProxy.type == 0)
			memset(m_ffProxy.http, 0, 16);
		//VOLOGI("this: %p, FF proxy type: %d", this, m_ffProxy.type);
	}
}


void COSMPEngnWrap::ActiveBrowserWindow() {

	if (m_voPlugInitParam.bWindowless)
		return;

	HWND pp = NULL;
	HWND aw = ::GetForegroundWindow();
	char winName[256];
	winName[255] = 0;
	::GetClassNameA(aw, winName, 256);
	
	if (strcmp(winName, "voPlugInControlWidnow") == 0 || strcmp(winName, "voPlugInViewWidnow") == 0) {
		
		if (IsFullScreen()) return;

		if (m_nBrowserType == 1)
			pp = ::GetParent(::GetParent((HWND)m_voPlugInitParam.hView));
		else if (m_nBrowserType == 2)
			pp = ::GetParent((HWND)m_voPlugInitParam.hView);

		if (pp)
			::PostMessage(pp, WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(1, 1));
	} else if (IsFullScreen()) {

		if ((m_nBrowserType == 1 && strcmp(winName, "Chrome_WidgetWin_1")) || (m_nBrowserType == 2 && strcmp(winName, "MozillaWindowClass")))
			::ShowWindow(::FindWindowA("Shell_TrayWnd", ""), SW_SHOW);
	}
}

int COSMPEngnWrap::internalSetHttpProxy() {

	if (m_pOSMPPlayer == NULL)
		return VO_OSMP_ERR_UNINITIALIZE;

	int ret = 0;
	if (m_nBrowserType < 2) {
		
		if (m_bProxySetup) {
			ret = m_pOSMPPlayer->SetParam(VOOSMP_SRC_PID_HTTP_PROXY_INFO, &m_proxy);

			VOLOGI("browser: %d, proxy: %s, port: %d", m_nBrowserType, m_proxy.pszProxyHost, m_proxy.nProxyPort);
		} else {

			// cancel proxy setting with this null proxy inf
			ret = m_pOSMPPlayer->SetParam(VOOSMP_SRC_PID_HTTP_PROXY_INFO, NULL);
			VOLOGI("NO PROXY, DIRECT TO NET.");
		}

	} else if (m_nBrowserType == 2) {

		VOOSMP_SRC_HTTP_PROXY proxy;
		memset(&proxy, 0, sizeof(proxy));
		if (m_ffProxy.type == 0) {

			ret = m_pOSMPPlayer->SetParam(VOOSMP_SRC_PID_HTTP_PROXY_INFO, NULL);
			VOLOGI("DIRECT CONNECTING, NO NEED TO SET PROXY.");
		} else if (m_ffProxy.type == 1) { // manual setting

			proxy.pszProxyHost = (unsigned char*)m_ffProxy.http;
			proxy.nProxyPort = m_ffProxy.http_port;
			ret = m_pOSMPPlayer->SetParam(VOOSMP_SRC_PID_HTTP_PROXY_INFO, &proxy);


			VOLOGI("browser: %d, MANUALLY SET proxy: %s, port: %d", m_nBrowserType, proxy.pszProxyHost, proxy.nProxyPort);
		} else if (m_ffProxy.type == 2) { //automatic configure url

			ret = m_pOSMPPlayer->SetParam(VOOSMP_SRC_PID_HTTP_PROXY_INFO, NULL);
			VOLOGI("It's automatic-configure-url type, so far not supported yet. so make it direct to net.");
		} else if (m_ffProxy.type == 3) { // use system setting

			ret = m_pOSMPPlayer->SetParam(VOOSMP_SRC_PID_HTTP_PROXY_INFO, &m_proxy);

			VOLOGI("browser: %d, USE SYSTEM proxy: %s, port: %d", m_nBrowserType, m_proxy.pszProxyHost, m_proxy.nProxyPort);
		} else if (m_ffProxy.type == 4) { // auto detect

			ret = m_pOSMPPlayer->SetParam(VOOSMP_SRC_PID_HTTP_PROXY_INFO, NULL);
			VOLOGI("It's auto-detecting type, so far not supported yet. so make it direct to net.");
		}
	}

	return ret;
}

VO_OSMP_RETURN_CODE COSMPEngnWrap::SetDC(HDC hdc)
{
  voCAutoLock lockReadSrc (&m_Mutex);
  if(m_pOSMPPlayer ==NULL)
    return VO_OSMP_ERR_POINTER;

  int nRC = m_pOSMPPlayer->SetParam(VOOSMP_PID_VIDEO_DC , hdc);
  return (VO_OSMP_RETURN_CODE)nRC;
}

VO_OSMP_RETURN_CODE COSMPEngnWrap::SetPresentationDelay(int time)
{
	voCAutoLock lock(&m_Mutex);
	if(m_pOSMPPlayer ==NULL)
		return VO_OSMP_ERR_POINTER;

	int nt = time;
	int nRC = m_pOSMPPlayer->SetParam(VOOSMP_SRC_PID_PRESENTATION_DELAY, &nt);
	return (VO_OSMP_RETURN_CODE)nRC;
}

VO_OSMP_RETURN_CODE COSMPEngnWrap::SetAudioPlaybackSpeed(FLOAT fSpeed)
{
  voCAutoLock lockReadSrc (&m_Mutex);
  if(m_pOSMPPlayer ==NULL)
    return VO_OSMP_ERR_POINTER;

  int nRC = m_pOSMPPlayer->SetParam(VOOSMP_PID_AUDIO_PLAYBACK_SPEED , &fSpeed);
  return (VO_OSMP_RETURN_CODE)nRC;
}

BOOL COSMPEngnWrap::IsLiveStreaming()
{
  if(m_pOSMPPlayer ==NULL)
    return FALSE;

  return m_pOSMPPlayer->IsLiveStreaming();
}

VO_OSMP_RETURN_CODE COSMPEngnWrap::GetVersion(VOOSMP_MODULE_VERSION* pVersion, TCHAR* pszWorkingPath)
{
	if(!pVersion || !pszWorkingPath)
		return VO_OSMP_ERR_POINTER;

	CvoOnStreamMP mp(pszWorkingPath);
	mp.Init();

	VO_OSMP_RETURN_CODE nRet = (VO_OSMP_RETURN_CODE)mp.GetParam(VOOSMP_PID_MODULE_VERSION, pVersion);

	if(pVersion->pszVersion)
	{
		memset(m_szVersion, 0, MAX_PATH);
		MultiByteToWideChar(CP_ACP, 0, pVersion->pszVersion, -1, m_szVersion, MAX_PATH); 
		pVersion->pszVersion = (char*)m_szVersion;
	}

	mp.Uninit();

	return nRet;
}

VO_OSMP_RETURN_CODE COSMPEngnWrap::ToggleOverlayUI(BOOL bShow)
{
  voCAutoLock lockReadSrc (&m_Mutex);

  if(FALSE == m_voPlugInitParam.bWindowless)
    return VO_OSMP_ERR_NONE;

  if(m_pOSMPPlayer ==NULL)
    return VO_OSMP_ERR_POINTER;

  //when paused, do not need to switch render type.
  if (GetPlayerStatus() ==  VO_OSMP_STATUS_PAUSED)
  {
    return VO_OSMP_ERR_NONE;
  }

  int nRC = VO_OSMP_ERR_NONE;
  VOOSMP_RENDER_TYPE typeRender = bShow ? VOOSMP_RENDER_TYPE_DC : VOOSMP_RENDER_TYPE_DDRAW;
  if(m_typeRender != typeRender)
  {
    nRC = m_pOSMPPlayer->SetParam(VOOSMP_PID_VIDEO_RENDER_TYPE, (void*)&typeRender);

    if (nRC!=0)
    {
      VOLOGE("Create DDdraw failed: type is %d, nRC=%d", typeRender, nRC);
      m_pOSMPPlayer->SetParam(VOOSMP_PID_VIDEO_RENDER_TYPE, (void*)&m_typeRender);
    }

    m_typeRender = typeRender;
  }

  return (VO_OSMP_RETURN_CODE)nRC;
}

void COSMPEngnWrap::postMessage2UI(UINT msg, WPARAM wParam, LPARAM lParam) {

	if (m_pOverlayUI) {
		VOLOGI("here to update video window...");
		::PostMessage((HWND)m_pOverlayUI->GetView(), msg, wParam, lParam);
	}
}

int COSMPEngnWrap::cbSrcAuthenticationRequest(void * pParam1, void * pParam2)
{
  return SetDRMVerificationInfo(m_szDRMVerificationInfo,strlen(m_szDRMVerificationInfo),FALSE);
}

VO_OSMP_RETURN_CODE COSMPEngnWrap::SetDRMLibrary(LPCSTR libName, LPCSTR libApiName,BOOL bSetByWebPage)
{
	if (NULL==libApiName || NULL==libName)
	{
		return VO_OSMP_ERR_POINTER;
	}

	VOLOGI("libname: %s, api name: %s", libName, libApiName);
	m_strDRMLibName = libName;
	m_strDRMApiName = libApiName;
	if (m_strDRMLibName.CompareNoCase(VO_VERIMATRIX_DRM_LIBNAME) == 0) {
		InitVerimatrixDrmEngn();
		return VO_OSMP_ERR_UNKNOWN;
	}

	//when web page set value to plugin, just need remember the values.
	if (bSetByWebPage)
		return VO_OSMP_ERR_NONE;
	
	int nRC = VO_OSMP_ERR_UNKNOWN;
	if(m_pOSMPPlayer)
	{
		nRC = m_pOSMPPlayer->SetParam(VOOSMP_SRC_PID_DRM_FILE_NAME, (void*)libName);
		nRC = m_pOSMPPlayer->SetParam(VOOSMP_SRC_PID_DRM_API_NAME,(void*)libApiName);
	}

  return (VO_OSMP_RETURN_CODE)nRC;
}


void COSMPEngnWrap::InitVerimatrixDrmEngn() {

	if (m_strDRMLibName.CompareNoCase(VO_VERIMATRIX_DRM_LIBNAME))
		return;

	getSingleInstanceController(2);
	if (m_hSemaVerimatrixDrmInitCrl) {
		DWORD dwRtn = WaitForSingleObject(m_hSemaVerimatrixDrmInitCrl, 6000);
        VOLOGI("WaitForSingleObject(m_hSemaVerimatrixDrmInitCrl, 6000) return 0x%08x. this is %d", dwRtn, voThreadGetCurrentID());
		if (dwRtn != WAIT_OBJECT_0) {
			return;
		}
    }

	if (m_hVODRM == NULL) {

		TCHAR szLibDRM[MAX_PATH];
		memset(szLibDRM, 0, sizeof(TCHAR)*MAX_PATH);
		_tcscpy(szLibDRM, m_voPlugInitParam.szWorkPath);
		_tcscat(szLibDRM, _T("voDRM_Verimatrix_AES128.dll"));
		m_hVODRM = ::LoadLibrary(szLibDRM);
		//VOLOGI("%d", GetLastError());
	}

	if (m_hVODRM == NULL)
		return;

	if (m_apiDRM.Init == NULL) {
		typedef VO_S32 (VO_API * VOGETDRM2API)(VO_DRM2_API * pDRMHandle, VO_U32 uFlag);
		VOGETDRM2API pGetDRM2API = (VOGETDRM2API)::GetProcAddress(m_hVODRM, "voGetDRMAPI");
		if (pGetDRM2API == NULL)
			return;

		pGetDRM2API(&m_apiDRM, 0);
		if (m_apiDRM.Init == NULL)
			return;
	}

	VOLOGI("m_apiDRM.Init: %p", m_apiDRM.Init);
	if (m_pDrmAdapter == NULL)
		m_apiDRM.Init((VO_PTR*)&m_pDrmAdapter, NULL);
	VOLOGI("m_pDrmAdapter: %p", m_pDrmAdapter);

	if (m_pDrmAdapter && m_apiDRM.SetParameter) {
		char szpath[MAX_PATH];
		memset(szpath, 0, MAX_PATH);
		WideCharToMultiByte(CP_UTF8, 0, m_voPlugInitParam.szWorkPath, _tcslen(m_voPlugInitParam.szWorkPath), szpath, MAX_PATH, NULL, NULL);
		m_apiDRM.SetParameter(m_pDrmAdapter, VO_PID_DRM2_PackagePath, szpath);
	}

	::ReleaseSemaphore(m_hSemaVerimatrixDrmInitCrl, 1, NULL);
    ::CloseHandle(m_hSemaVerimatrixDrmInitCrl);
    m_hSemaVerimatrixDrmInitCrl = NULL;
    VOLOGI("release m_hSemaVerimatrixDrmInitCrl, this is %d", voThreadGetCurrentID());
	
	internalSetDrmVerfication();
	internalSetPreAgreedLicense();
}

void COSMPEngnWrap::UninitVerimatrixDrmEngn() {

	if (m_apiDRM.Uninit) {
		m_apiDRM.Uninit((VO_PTR)m_pDrmAdapter);

		m_pDrmAdapter = NULL;
		memset(&m_apiDRM, 0, sizeof(m_apiDRM));
	}

	if (m_hVODRM) {
		::FreeLibrary(m_hVODRM);
		m_hVODRM = NULL;
	}
}

int COSMPEngnWrap::internalShakeHandWithDrmServer() {

	if (m_apiDRM.SetParameter == NULL)
		return -1;

	// waiting for 15s here due to Verimatrix DRM server does not support multiple instances
    VOLOGI("HERE TO wait another instance finishing shaking hand...");
    getSingleInstanceController(3);
	if (m_hSemaVerimatrixDrmShakeCrl) {
		DWORD dwRtn = WaitForSingleObject(m_hSemaVerimatrixDrmShakeCrl, 6000);
        VOLOGI("WaitForSingleObject(m_hSemaVerimatrixDrmShakeCrl, 6000) return 0x%08x. this is %d", dwRtn, voThreadGetCurrentID());
		if (dwRtn != WAIT_OBJECT_0) {
			return VO_OSMP_ERR_STATUS;
		}
	}


	m_nDrmShakingHand++;
	VOLOGI("m_nDrmShakingHand: %d", m_nDrmShakingHand);

	VOOSMP_SRC_VERIFICATIONINFO vi;
	memset(&vi, 0, sizeof(vi));
	int len = strlen(m_szDRMVerificationInfo);
	if (len) {
		vi.pData = (void *)m_szDRMVerificationInfo;
		vi.nDataSize = len;
	}
	m_apiDRM.SetParameter(m_pDrmAdapter, VO_PID_SOURCE2_DOHTTPVERIFICATION, &vi);

	m_nDrmShakingHand--;
	
    ::ReleaseSemaphore(m_hSemaVerimatrixDrmShakeCrl, 1, NULL);
    ::CloseHandle(m_hSemaVerimatrixDrmShakeCrl);
    m_hSemaVerimatrixDrmShakeCrl = NULL;
    VOLOGI("release m_hSemaVerimatrixDrmShakeCrl, this is %d", voThreadGetCurrentID());
	

	VOLOGI("m_nDrmShakingHand: %d", m_nDrmShakingHand);

	return 0;
}

int COSMPEngnWrap::shakeHandWithDrmServerProc(VO_PTR pParam) {

	if (pParam == NULL) return -1;

	COSMPEngnWrap* pengn = (COSMPEngnWrap*)pParam;
	pengn->internalShakeHandWithDrmServer();

	VOLOGI("DRM Engine loaded over!");
	return 0;
}

VO_OSMP_RETURN_CODE COSMPEngnWrap::EnableLiveStreamingDVRPosition(BOOL bEnable)
{
  if (m_pOSMPPlayer)
  {
    return (VO_OSMP_RETURN_CODE)(m_pOSMPPlayer->EnableLiveStreamingDVRPosition(bEnable));
  }

  return VO_OSMP_ERR_UNKNOWN;
}

const char*	COSMPEngnWrap::GetVideoDecodingBitrate() {

	if (m_pOSMPPlayer == NULL)
		return NULL;

	memset(m_szAnalyticsInfo, 0, 1024);
	
	int ret = m_pOSMPPlayer->GetVideoDecodingBitrate(m_szAnalyticsInfo);
	if (ret != VOOSMP_ERR_None)
		return NULL;

	return m_szAnalyticsInfo;
}

const char*	COSMPEngnWrap::GetAudioDecodingBitrate() {

	if (m_pOSMPPlayer == NULL)
		return NULL;

	memset(m_szAnalyticsInfo, 0, 1024);

	int ret = m_pOSMPPlayer->GetAudioDecodingBitrate(m_szAnalyticsInfo);
	if (ret != VOOSMP_ERR_None)
		return NULL;

	return m_szAnalyticsInfo;
}

void COSMPEngnWrap::detectExceptions()
{
  m_vecStrExceptions.clear();

  HKEY hKEY = NULL;
  LPCTSTR strKey = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\");

  long ret = ::RegOpenKeyEx(HKEY_CURRENT_USER,strKey,0,KEY_READ,&hKEY);
  if (ret != ERROR_SUCCESS)
  {
    return;
  }

  BYTE *lpExceptionSet = new BYTE[MAX_PATH];
  DWORD type = REG_SZ;
  DWORD cbData = MAX_PATH;

  ret = ::RegQueryValueEx(hKEY,_T("ProxyOverride"),NULL,&type,lpExceptionSet,&cbData);
  if (ret != ERROR_SUCCESS)
  {
	  ::RegCloseKey(hKEY);
	  if (lpExceptionSet)
		delete []lpExceptionSet;
    return;
  }

  TCHAR szException[MAX_PATH] = _T("");
  memcpy(szException,lpExceptionSet,cbData);

  CString str=szException;
  TCHAR seps[]   = _T(";");
  TCHAR *token;

  token = _tcstok( str.GetBuffer(str.GetLength()), seps );
  while( token != NULL )
  {
    CString strTemp = (TCHAR*)token;
    m_vecStrExceptions.push_back(strTemp);
    token = _tcstok( NULL, seps );
  }

  ::RegCloseKey(hKEY);
  if (lpExceptionSet)
	  delete []lpExceptionSet;
}

//str1 is the url to test is match, str2:match rule(*.1.2.*)
BOOL strMatch( const TCHAR *str1, const TCHAR *str2)   
{   
  int iLen1 = _tcslen(str1);   
  int iLen2 = _tcslen(str2); 
  int i=0, j=0, k=0, l , m , n;   

  //find the last character not '*'.
  for(l=0 , n=0; l<iLen2; l++) 
  { 
    if(str2[l]!='*')
      n=l;  
  } 

  while( i<iLen1 && j<iLen2 ) 
  {	 
    while(j<iLen2)	 
    {
      if(str2[j]=='*')	
      { 
        k=1;  
        j++;
      }
      else	
      { 
        l=j; 
        m=i; 
        break; 
      }
    }

    while( i<iLen1 && j<iLen2 )				 
    {	 
      if(str2[j]=='*')
        break; 
	 
      if(str1[i]==str2[j]||str2[j]=='?')
      {
        i++;
        j++;
      }
      else 
      { 
        if(k==1)
        {
          m++; 
          i=m; 
          j=l;
        }
        else 
          return 0; 
      }	 
    }		 

    if( i<iLen1 && j==iLen2 && str2[j-1]!='*' && k==1 )
    {
      m++; 
      i=m; 
      j=l;
    } 
  } 
  
  if( ((i==iLen1 && n+1==j) || (i<iLen1)&&j!=0&&str2[j-1]=='*') )
    return TRUE; 
  else 
    return FALSE; 
}

BOOL COSMPEngnWrap::isUrlInExceptions(LPCTSTR szUrl)
{
  //get the beginning text of the url
  CString strUrl = szUrl;
  CString strMainLink = szUrl;
  strMainLink.MakeLower();
  strMainLink.Replace(_T("http://"),_T(""));
  strMainLink.Replace(_T("https://"),_T(""));
  strMainLink.Replace(_T("ftp://"),_T(""));
  int nTemp = strMainLink.Find('/');
  if(nTemp>0)
    strMainLink = strMainLink.Left(nTemp);

  detectExceptions();
  for(std::vector<CString>::const_iterator it = m_vecStrExceptions.begin(); it < m_vecStrExceptions.end(); ++it)
  {
    CString strException = *it;
    strException.MakeLower();

    //maybe contain head "http://" "https://" "ftp://", like: http://1.1.1.1
    if ( strException.Find(_T("http://"))==0 )
    {
      if (strUrl.Find(_T("http://"))!=0)
        continue;
      strException.Replace(_T("http://"),_T(""));
    }
    else if ( strException.Find(_T("https://"))==0 )
    {
      if (strUrl.Find(_T("https://"))!=0)
        continue;
      strException.Replace(_T("https://"),_T(""));
    }
    else if ( strException.Find(_T("ftp://"))==0 )
    {
      if (strUrl.Find(_T("ftp://"))!=0)
        continue;
      strException.Replace(_T("ftp://"),_T(""));
    }

    CString strTempMainLink = strMainLink;
    //maybe contain head ":8082", exception like that: hls-iis.visualon.com:8082
    if ( strException.Find(_T(":"))>0 )
    {
      if (strTempMainLink.Find(_T(":"))<=0)//url like that: http://hls-iis.visualon.com:8082/hls/v8/bipbop_16x9_variant.m3u8
        continue;
    }
    else
    {
      nTemp = strTempMainLink.Find(':');//maybe the link like that: http://hls-iis.visualon.com:8082/hls/v8/bipbop_16x9_variant.m3u8
      if(nTemp>0)
        strTempMainLink = strTempMainLink.Left(nTemp);
    }

    if(TRUE == strMatch(strTempMainLink,strException))
      return TRUE;
    else
      continue;
  }

  return FALSE;
}

void COSMPEngnWrap::internalSetDrmVerfication() {

	HKEY hKEY;
	LPCSTR lpszDrmKey = "SOFTWARE\\VisualOn\\BrowserPlugin\\DRMVerification\\Verimatrix";
	LSTATUS ret = ::RegOpenKeyExA(HKEY_LOCAL_MACHINE, lpszDrmKey, 0, KEY_READ | KEY_WOW64_32KEY, &hKEY);
	if (ret != ERROR_SUCCESS) {
		VOLOGI("Failed to open DRM verification key.");
		return;
	}

	BYTE server[128];
	DWORD type = REG_SZ;
	DWORD len = 128;

	memset(server, 0, 128);
	ret = ::RegQueryValueExA(hKEY, "server", NULL, &type, server, &len);
	::RegCloseKey(hKEY);
	if (ret != ERROR_SUCCESS) {
		VOLOGI("Failed to read DRM server information.");
		return;
	}
	
	VOLOGI("original server: %s", (char*)server);
	char buf[128];
	memset(buf, 0, 128);
	int j = 0;
	for (int i = 0; server[i] != '\0'; i++)
	{
		if (server[i] != ' ' && server[i] != '\t') 
			buf[j++] = server[i];
	} 
	buf[j]='\0';
	len = strlen(buf);
	VOLOGI("after trimmed, server: %s", (char*)buf);
	if (strcmp((const char*)buf, m_szDRMVerificationInfo))
		SetDRMVerificationInfo((char*)buf, len, FALSE);
}

void COSMPEngnWrap::internalSetPreAgreedLicense() {

	HKEY hKEY;
	LPCSTR lpszDrmKey = "SOFTWARE\\VisualOn\\BrowserPlugin\\PreAgreedLicense";
	LSTATUS ret = ::RegOpenKeyExA(HKEY_LOCAL_MACHINE, lpszDrmKey, 0, KEY_READ | KEY_WOW64_32KEY, &hKEY);
	if (ret != ERROR_SUCCESS) {
		VOLOGI("Failed to open DRM verification key.");
		return;
	}

	BYTE lickey[MAX_PATH];
	memset(lickey, 0, MAX_PATH);
	DWORD type = REG_SZ;
	DWORD len = MAX_PATH;

	ret = ::RegQueryValueExA(hKEY, "key", NULL, &type, lickey, &len);
	::RegCloseKey(hKEY);
	if (ret != ERROR_SUCCESS) {
		VOLOGI("Failed to read DRM server information.");
		return;
	}

	if (strcmp((const char*)lickey, (const char*)m_szLicenseString)) {
		memset(m_szLicenseString, 0, sizeof(m_szLicenseString));
		memcpy(m_szLicenseString, lickey, len);

		if (m_pOSMPPlayer)
			m_pOSMPPlayer->SetParam(VOOSMP_PID_LICENSE_TEXT,(void *)m_szLicenseString);
	}
}

/**
 * Set the initial buffering time of for playback start.
 *
 * @param   time [in] buffer time (seconds)
 *
 * @return  VO_OSMP_ERR_NONE if successful.
 */
VO_OSMP_RETURN_CODE COSMPEngnWrap::SetInitialBufferTime(int time)
{
  int nRC = VO_OSMP_ERR_UNKNOWN;
  if(m_pOSMPPlayer)
  {
    nRC = m_pOSMPPlayer->SetParam(VOOSMP_SRC_PID_BUFFER_START_BUFFERING_TIME, (void*)time);
  }

  return (VO_OSMP_RETURN_CODE)nRC;
}

/**
 * Enable/Disable processing SEI information.
 *
 * @param   flag [in] the flag {@link VO_OSMP_SEI_INFO_FLAG}. Set to VO_OSMP_SEI_INFO_NONE to disable processing SEI information or any other flags to enable
 *
 * @return  {@link VO_OSMP_ERR_NONE} if successful
 */
VO_OSMP_RETURN_CODE COSMPEngnWrap::EnableSEI(VO_OSMP_SEI_INFO_FLAG flag)
{
  if (NULL == m_pOSMPPlayer) {
    return VO_OSMP_ERR_UNINITIALIZE;
  }

  int nFlag = flag;
  if (VO_OSMP_FLAG_SEI_MAX == flag) {
    nFlag = VOOSMP_FLAG_SEI_MAX;
  }

  return (VO_OSMP_RETURN_CODE)(m_pOSMPPlayer->SetParam(VOOSMP_PID_RETRIEVE_SEI_INFO, &nFlag));
}

/**
 * Start periodic SEI data notifications.
 *
 * @param   interval [in] Time interval <ms> between two SEI data notifications.
 *
 * @return  {@link VO_OSMP_RETURN_CODE#VO_OSMP_ERR_NONE} if successful
 */
VO_OSMP_RETURN_CODE COSMPEngnWrap::StartSEINotification(int interval)
{
  if (NULL == m_pOSMPPlayer) {
    return VO_OSMP_ERR_UNINITIALIZE;
  }

  return (VO_OSMP_RETURN_CODE)(m_pOSMPPlayer->SetParam(VOOSMP_PID_SEI_EVENT_INTERVAL, &interval));
}

/**
 * Stop periodic SEI data notifications.
 *
 * @return  {@link VO_OSMP_ERR_NONE} if successful
 */
VO_OSMP_RETURN_CODE COSMPEngnWrap::StopSEINotification(void)
{
  if (NULL == m_pOSMPPlayer) {
    return VO_OSMP_ERR_UNINITIALIZE;
  }

  int interval = -1;
  return (VO_OSMP_RETURN_CODE)(m_pOSMPPlayer->SetParam(VOOSMP_PID_SEI_EVENT_INTERVAL, &interval));
}

/**
 * Get the SEI info.
 *
 * @param   time [in] the time stamp of SEI that want to get
 * @param   flag [in] the type flag of SEI info {@link VO_OSMP_SEI_INFO_FLAG}
 *
 * @return  the object according to flag if successful
 */
JSON COSMPEngnWrap::GetSEIInfo(LONG time, VO_OSMP_SEI_INFO_FLAG flag)
{
  if (NULL == m_pOSMPPlayer) {
    return _T("");
  }

  VOOSMP_SEI_INFO cInfo;
  memset(&cInfo, 0, sizeof(cInfo));
  cInfo.llTime = time;
  cInfo.nFlag = flag;
  if (VO_OSMP_FLAG_SEI_MAX == flag) {
    cInfo.nFlag = VOOSMP_FLAG_SEI_MAX;
  }

  int nRet = m_pOSMPPlayer->GetParam(VOOSMP_PID_RETRIEVE_SEI_INFO, &cInfo);
  if (VOOSMP_ERR_None != nRet) {
    VOLOGE("getSEIInfo Info Err:%d", nRet);
    return _T("");
  }

  voJSON *pvi = voJSON_CreateObject();
  if (pvi == NULL)
    return _T("");

  char szTemp[MAX_PATH] = "";
  _i64toa(cInfo.llTime,szTemp,10);
  voJSON_AddStringToObject(pvi, "llTime", szTemp);

  strcpy(szTemp, "");
  itoa(cInfo.nFlag,szTemp,16);
  voJSON_AddStringToObject(pvi, "nFlag", szTemp);

  strcpy(szTemp, "");
  _i64toa((LONG)(cInfo.pInfo),szTemp,16);//??
  voJSON_AddStringToObject(pvi, "pInfo", szTemp);

  _tcscpy(m_szJSONString, _T(""));
  char* szval = voJSON_Print(pvi);
  ::MultiByteToWideChar(CP_ACP, 0, szval, -1, m_szJSONString, sizeof(m_szJSONString));
  free(szval);
  voJSON_Delete(pvi);

  return m_szJSONString;
}

void COSMPEngnWrap::setSystemState(const int stat) {

	voCAutoLock lock (&m_Mutex);
	if (m_pOSMPPlayer==NULL)
		return;

	int ncurstat = stat;
	if (stat)
		m_pOSMPPlayer->SetParam(VOOSMP_SRC_PID_APPLICATION_RESUME, &ncurstat);
	else 
		m_pOSMPPlayer->SetParam(VOOSMP_SRC_PID_APPLICATION_SUSPEND, &ncurstat);

	VOLOGI("current system state: %d", stat);
}

VO_OSMP_RETURN_CODE COSMPEngnWrap::EnableCPUAdaptation(BOOL bEnable)
{
  voCAutoLock lock (&m_Mutex);
  if (m_pOSMPPlayer==NULL)
  {
    return VO_OSMP_ERR_POINTER;
  }

  bEnable = !bEnable;
  int nRC = m_pOSMPPlayer->SetParam(VOOSMP_SRC_PID_DISABLE_CPU_ADAPTION,&bEnable);
  return (VO_OSMP_RETURN_CODE)nRC;
}

VO_OSMP_RETURN_CODE COSMPEngnWrap::SetBitrateThreshold(int upper, int lower)
{
  voCAutoLock lock (&m_Mutex);
  if (m_pOSMPPlayer==NULL)
  {
    return VO_OSMP_ERR_POINTER;
  }

  VO_SOURCE2_BA_THRESHOLD baThreshold;
  baThreshold.nUpper = upper;
  baThreshold.nLower = lower;

  int nRC = m_pOSMPPlayer->SetParam(VOOSMP_SRC_PID_DISABLE_CPU_ADAPTION,&baThreshold);
  return (VO_OSMP_RETURN_CODE)nRC;
}

BOOL COSMPEngnWrap::CreateProcessMsgWnd()
{
  if (m_hwndProcessMsg)
  {
    return TRUE;
  }

  WNDCLASS wcex;
  wcex.style			= CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc	= (WNDPROC)msgWindowProc;
  wcex.cbClsExtra		= 0;
  wcex.cbWndExtra		= 0;
  wcex.hInstance		= AfxGetInstanceHandle();
  wcex.hIcon   = NULL;
  wcex.hCursor  = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
  wcex.lpszMenuName	= (LPCTSTR)NULL;
  wcex.lpszClassName	= _T("PluginProMsgWindow");

  int iRet = RegisterClass(&wcex);

  m_hwndProcessMsg = CreateWindow(_T("PluginProMsgWindow"), _T("PluginProMsgWindow"), WS_MINIMIZE,
    0, 0, 0, 0, NULL, NULL, AfxGetInstanceHandle(), NULL);
  if (m_hwndProcessMsg == NULL)
    return FALSE;

  ::SetWindowLong (m_hwndProcessMsg, GWL_USERDATA, (LONG)this);

  m_hPowerNotify = RegisterPowerSettingNotification(m_hwndProcessMsg,&GUID_LIDSWITCH_STATE_CHANGE,DEVICE_NOTIFY_WINDOW_HANDLE);

  return TRUE;
}

LRESULT CALLBACK COSMPEngnWrap::msgWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  COSMPEngnWrap * pWrap = (COSMPEngnWrap *)GetWindowLong (hwnd, GWL_USERDATA);
  if (pWrap == NULL)
    return(::DefWindowProc(hwnd, uMsg, wParam, lParam));
  else
    return pWrap->OnReceiveMessage(hwnd,uMsg,wParam,lParam);
}

LRESULT COSMPEngnWrap::OnReceiveMessage (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (hwnd != m_hwndProcessMsg)
		return 0;

  switch (uMsg)
  {
  case WM_POWERBROADCAST:
    {
      VOLOGI("WM_POWERBROADCAST");
      switch(wParam)
      {
      case PBT_APMRESUMESUSPEND:
        VOLOGI("PBT_APMRESUMESUSPEND");
        setSystemState(1);
        break;
      case PBT_APMSUSPEND:
        VOLOGI("PBT_APMSUSPEND");
        setSystemState(0);
        break;
      case PBT_APMRESUMEAUTOMATIC:
        VOLOGI("PBT_APMRESUMEAUTOMATIC");
        setSystemState(1);
        break;
      case PBT_POWERSETTINGCHANGE:
        VOLOGI("PBT_POWERSETTINGCHANGE");
        break;
      default:
        VOLOGI("WM_POWERBROADCAST %d",lParam);
        break;
      }

      break;
    }
  default:
    break;
  }

  return 0;
}

void COSMPEngnWrap::getSingleInstanceController(const int sema) {

    WCHAR* semaName = NULL;
    HANDLE* phsema = NULL;
    if (sema == 1) {
        phsema = &m_hSemaSingleInstanceCtrl;
        semaName = VOSEMA_MULINSTANCE_NAME;
        VOLOGI("get m_hSemaSingleInstanceCtrl, this is %d", voThreadGetCurrentID());
    } else if (sema == 2) {
        phsema = &m_hSemaVerimatrixDrmInitCrl;
        semaName = VOSEMA_VERIMATRIXDRMINIT_NAME;
        VOLOGI("get m_hSemaVerimatrixDrmInitCrl, this is %d", voThreadGetCurrentID());
    } else if (sema == 3) {
        phsema = &m_hSemaVerimatrixDrmShakeCrl;
        semaName = VOSEMA_VERIMATRIXDRMSHAKE_NAME;
        VOLOGI("get m_hSemaVerimatrixDrmShakeCrl, this is %d", voThreadGetCurrentID());
    }

    VOLOGI("sema index: %d, hsema: %p", sema, *phsema);
	if (NULL == *phsema) {
		
		*phsema = ::OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, semaName);
		if (*phsema == NULL) {
			*phsema = CreateSemaphore(NULL, 1, 1, semaName);
			if (*phsema == NULL) {
				VOLOGI("failed to create global semaphore for controlling multiple instance.");
			} else {
				VOLOGI("create global semaphore %p successfully.", *phsema);
			}
		} else {
			VOLOGI("open global semaphore %p successfully.", *phsema);
		}
	}
}

VO_OSMP_RETURN_CODE COSMPEngnWrap::UpdateSourceURL(LPCTSTR url)
{
    voCAutoLock lock (&m_Mutex);
    if (m_pOSMPPlayer==NULL)
        return VO_OSMP_ERR_UNINITIALIZE;

   return (VO_OSMP_RETURN_CODE)(m_pOSMPPlayer->SetParam(VOOSMP_SRC_PID_UPDATE_SOURCE_URL, (void*)url));
}

VO_OSMP_RETURN_CODE COSMPEngnWrap::EnableDolbyLibrary(BOOL bEnable)
{
    voCAutoLock lock (&m_Mutex);
    if (m_pOSMPPlayer==NULL)
        return VO_OSMP_ERR_UNINITIALIZE;

    int nValue = bEnable ? 1:0;
    m_pOSMPPlayer->SetParam(VOOSMP_PID_LOAD_DOLBY_DECODER_MODULE, &nValue);
    return (VO_OSMP_RETURN_CODE)(m_pOSMPPlayer->SetParam(VOOSMP_PID_LOAD_AUDIO_EFFECT_MODULE, &nValue));
}

VO_OSMP_RETURN_CODE COSMPEngnWrap::SetHTTPRetryTimeout(int iRetryTime)
{
    voCAutoLock lock (&m_Mutex);
    if (m_pOSMPPlayer==NULL)
        return VO_OSMP_ERR_UNINITIALIZE;

    m_iHttpRetryTime = iRetryTime;
    return (VO_OSMP_RETURN_CODE)(m_pOSMPPlayer->SetParam(VOOSMP_SRC_PID_HTTP_RETRY_TIMEOUT, &iRetryTime));
}

VO_OSMP_RETURN_CODE COSMPEngnWrap::SetDefaultAudioLanguage(CHAR* type,BOOL bSetByWebPage)
{
    if (type)
    {
        strcpy(m_szDefAudioLan, type);
    }
    else
    {
        strcpy(m_szDefAudioLan, "");
    }

    if(bSetByWebPage)
        return VO_OSMP_ERR_NONE;

    return (VO_OSMP_RETURN_CODE)(m_pOSMPPlayer->SetParam(VOOSMP_SRC_PID_DEFAULT_AUDIO_LANGUAGE, type));
}

VO_OSMP_RETURN_CODE COSMPEngnWrap::SetDefaultSubtitleLanguage(CHAR* type,BOOL bSetByWebPage)
{
    if (type)
    {
        strcpy(m_szDefSubLan, type);
    }
    else
    {
        strcpy(m_szDefSubLan, "");
    }

    if(bSetByWebPage)
        return VO_OSMP_ERR_NONE;

    return (VO_OSMP_RETURN_CODE)(m_pOSMPPlayer->SetParam(VOOSMP_SRC_PID_DEFAULT_SUBTITLE_LANGUAGE, type));
}