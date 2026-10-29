# Engine seam map

Read-only analysis for phase 0, step 4. For `OnGraphNotify`, `CloseMediaInternal` and the
`OpenMedia` path in `src/mpc-hc/MainFrm.cpp`: every `CMainFrame` member each function touches,
grouped as **playback state** or **UI state**, and the hazards that matter when the engine is
extracted behind a C ABI.

Mapped at commit `9d5edb8f7`, i.e. after the out-of-scope removals (#6, #8, #10, #12, #14), so the
capture, BDA, web server, LCD and crash-reporter branches are already gone. Line numbers refer to
that commit.

## Method

The member lists are extracted mechanically, then classified by hand:

- The `CMainFrame` declaration in `MainFrm.h` is parsed into 265 fields and 630 methods.
- Each function body is taken from `MainFrm.cpp` with comments and string literals blanked out.
- A member is **touched** when its name appears unqualified in the body (or as `this->name`).
  Names reached through another object (`x.name`, `p->name`, `T::name`) are not counted, so
  `m_wndPlaylistBar.m_pl` counts as `m_wndPlaylistBar` only.
- Members inherited from `CFrameWnd`/`CWnd` (`m_hWnd`, `PostMessage`, ...) are out of scope.
- Only direct touches are listed. Calls between `CMainFrame` methods are listed per function, so
  the transitive picture can be followed through the tables.
- The extraction was cross-checked per function against a plain grep for `m_` names; the only
  differences were names inside a `/* */` block of commented-out code and the inherited `m_hWnd`.

## Call flow

```text
Open (UI thread)                        Graph thread (when used)
OpenCurPlaylistItem
  OpenMedia            [defers if nested]
    OpenMediaInternal  [MLS::LOADING] -- TM_OPEN --> OpenMediaPrivate
                                                      OpenCreateGraphObject
                                                      OpenFile | OpenDVD
                                                      OpenCustomizeGraph, OpenSetupVideo, OpenSetupAudio, ...
OnFilePostOpenmedia  <-- WM_POSTOPEN ----------------- (success)
OnOpenMediaFailed    <-- WM_OPENFAILED --------------- (failure)

Close (UI thread)
CloseMedia             [defers if nested]
  CloseMediaInternal   [MLS::ABORTING | MLS::CLOSING] -- TM_CLOSE --> CloseMediaPrivate
  OnFilePostClosemedia (called directly once the graph is gone)

Events (UI thread)
OnGraphNotify  (WM_GRAPHNOTIFY, drains IMediaEventEx)
  GraphEventComplete  (EC_COMPLETE)
```

The graph thread is used when `fEnableWorkerThreadForOpening` is on and the media is a
DirectShow file or a DVD; otherwise `OpenMediaPrivate` and `CloseMediaPrivate` run on the UI
thread. Functions reachable from them within this map: `CloseMediaPrivate`, `OpenCreateGraphObject`, `OpenCustomizeGraph`, `OpenDVD`, `OpenFile`, `OpenMediaPrivate`, `OpenSetupAudio`, `OpenSetupGetVideoSize`, `OpenSetupVideo`, `OpenSetupWindowTitle`.
`OpenMediaPrivate` calls `OnFilePostOpenmedia`/`OnOpenMediaFailed` directly only on the
no-worker path; with the worker it posts `WM_POSTOPEN`/`WM_OPENFAILED`, so those two always run on
the UI thread.

`OnGraphNotify` handles: `EC_PAUSED`, `EC_COMPLETE`, `EC_ERRORABORT`, `EC_BUFFERING_DATA`, `EC_STEP_COMPLETE`, `EC_DEVICE_LOST`, `EC_STREAM_ERROR_*`, `EC_VIDEO_SIZE_CHANGED`, `EC_LENGTH_CHANGED`, `EC_BG_AUDIO_CHANGED`, `EC_BG_ERROR`, `EC_CLOCK_CHANGED`, `0xfa17`, `EC_DVD_*`.

## Summary

| Function | Line | Lines | Playback fields | UI fields | Calls (CMainFrame) |
|---|---:|---:|---:|---:|---:|
| `OnGraphNotify` | 3239 | 433 | 21 | 6 | 20 |
| `GraphEventComplete` | 3173 | 60 | 11 | 1 | 5 |
| `OpenCurPlaylistItem` | 19495 | 38 | 2 | 1 | 6 |
| `OpenMedia` | 19535 | 19 | 2 | 0 | 2 |
| `OpenMediaInternal` | 19556 | 172 | 11 | 10 | 6 |
| `OpenMediaPrivate` (graph thread) | 16296 | 398 | 49 | 7 | 22 |
| `OpenCreateGraphObject` (graph thread) | 14117 | 132 | 18 | 2 | 1 |
| `OpenFile` (graph thread) | 14555 | 375 | 26 | 1 | 6 |
| `OpenDVD` (graph thread) | 15270 | 127 | 14 | 0 | 2 |
| `OpenCustomizeGraph` (graph thread) | 15400 | 48 | 6 | 0 | 3 |
| `OpenSetupGetVideoSize` (graph thread) | 15450 | 33 | 5 | 0 | 0 |
| `OpenSetupVideo` (graph thread) | 15486 | 37 | 6 | 4 | 3 |
| `OpenSetupAudio` (graph thread) | 15526 | 15 | 1 | 1 | 0 |
| `OpenSetupInfoBar` | 15543 | 61 | 1 | 1 | 3 |
| `OpenSetupStatsBar` | 15633 | 48 | 3 | 1 | 1 |
| `OpenSetupStatusBar` | 15841 | 6 | 0 | 1 | 2 |
| `OpenSetupWindowTitle` (graph thread) | 15850 | 50 | 1 | 1 | 3 |
| `OnFilePostOpenmedia` | 4294 | 189 | 17 | 15 | 26 |
| `OnOpenMediaFailed` | 4485 | 108 | 12 | 2 | 7 |
| `CloseMedia` | 19805 | 19 | 2 | 0 | 2 |
| `CloseMediaInternal` | 19826 | 541 | 24 | 10 | 17 |
| `CloseMediaPrivate` (graph thread) | 16696 | 125 | 63 | 1 | 4 |
| `OnFilePostClosemedia` | 4595 | 76 | 8 | 20 | 8 |

Across these 23 functions: 169 distinct fields (132 playback, 37 UI).

## Event handling

### `OnGraphNotify`

`MainFrm.cpp:3239`, 433 lines. Drains IMediaEventEx on the UI thread (WM_GRAPHNOTIFY) and dispatches graph events: completion, errors, buffering, frame step, video size and length changes, and the DVD navigator events.

**Playback state**

- *Filter graph and DirectShow interfaces*: `m_ActiveGraphNotifyEvCode`, `m_fCustomGraph`, `m_pME`, `m_pMS`
- *Seek-preview graph*: `m_bUseSeekPreview`
- *Media and stream state*: `m_audioTrackCount`, `m_bBuffering`, `m_fAudioOnly`, `m_fFrameSteppingActive`, `m_lastOMD`, `m_nStepForwardCount`
- *DVD and Blu-ray state*: `m_bDVDStillOn`, `m_fValidDVDOpen`, `m_iDVDDomain`, `m_iDVDTitle`, `m_pDVDC`, `m_pDVDI`
- *Open/close lifecycle, threading and locks*: `lockGraphAccess`, `m_eMediaLoadState`, `m_fOpeningAborted`
- *Reload, resume and A-B repeat*: `abRepeat`

**UI state**

- *Windows, bars and panels*: `m_wndInfoBar`, `m_wndPlaylistBar`, `m_wndSeekBar`, `m_wndStatusBar`
- *OSD and status text*: `m_OSD`, `m_statusbarVideoSize`

**Calls**: `AutoChangeMonitorMode`, `CheckSelectedAudioStream`, `CheckSelectedVideoStream`, `GetPlaybackMode`, `GraphEventComplete`, `IsAeroSnapped`, `IsFullScreenMode`, `LoadKeyFrames`, `MoveVideoWindow`, `OnTimer`, `OnVideoSizeChanged`, `SetClosingError`, `SetupAudioSubMenu`, `SetupChapters`, `SetupDVDChapters`, `SetupSubtitlesSubMenu`, `SyncPreviewEdition`, `UpdateCachedMediaState`, `UpdateDXVAStatus`, `ZoomVideoWindow`

### `GraphEventComplete`

`MainFrm.cpp:3173`, 60 lines. End-of-stream handling for EC_COMPLETE: loop count, A-B repeat and the after-playback action (DoAfterPlaybackEvent).

**Playback state**

- *Filter graph and DirectShow interfaces*: `m_pBA`, `m_pMS`
- *Media and stream state*: `lastOpenFile`, `m_bKeepLoopCountOnStop`, `m_fEndOfStream`, `m_fFrameSteppingActive`, `m_nLoops`, `m_nStepForwardCount`, `m_nVolumeBeforeFrameStepping`
- *Reload, resume and A-B repeat*: `abRepeat`, `m_bRememberFilePos`

**UI state**

- *Windows, bars and panels*: `m_wndPlaylistBar`

**Calls**: `DoAfterPlaybackEvent`, `GetMediaState`, `IsImageFile`, `MediaControlPause`, `PerformABRepeat`

## Open path

### `OpenCurPlaylistItem`

`MainFrm.cpp:19495`, 38 lines. Entry point from the playlist: builds the OpenMediaData for the current item (CPlayerPlaylistBar::GetCurOMD) and calls OpenMedia; deferred when nested.

**Playback state**

- *Media and stream state*: `m_sydlLastProcessURL`
- *Reload, resume and A-B repeat*: `abRepeat`

**UI state**

- *Windows, bars and panels*: `m_wndPlaylistBar`

**Calls**: `CloseMediaBeforeOpen`, `DeferIfNested`, `IsPlaylistEmpty`, `OpenCurPlaylistItem`, `OpenMedia`, `ProcessYoutubeDLURL`

### `OpenMedia`

`MainFrm.cpp:19535`, 19 lines. Public wrapper: defers the open while a CDeferredActionScope holder is on the stack, otherwise takes the scope and calls OpenMediaInternal.

**Playback state**

- *Open/close lifecycle, threading and locks*: `m_deferredActions`, `m_nDeferredActionDepth`

**UI state**: none

**Calls**: `OpenMedia`, `OpenMediaInternal`

### `OpenMediaInternal`

`MainFrm.cpp:19556`, 172 lines. UI-thread half of the open: state checks, closes the previous media, creates the dedicated fullscreen window when needed, sets MLS::LOADING, then either posts TM_OPEN to the graph thread or calls OpenMediaPrivate directly.

**Playback state**

- *Filter graph and DirectShow interfaces*: `m_ActiveGraphNotifyEvCode`
- *DVD and Blu-ray state*: `m_LastOpenBDPath`, `m_MPLSPlaylist`, `m_bIsBDPlay`
- *Open/close lifecycle, threading and locks*: `m_OnClose_called`, `m_OnClose_queued`, `m_bOpenMediaActive`, `m_bOpenedThroughThread`, `m_eMediaLoadState`, `m_evOpenPrivateFinished`, `m_pGraphThread`

**UI state**

- *Windows, bars and panels*: `m_pDedicatedFSVideoWnd`, `m_pVideoWnd`, `m_wndPlaylistBar`, `m_wndStatusBar`, `m_wndView`
- *OSD and status text*: `m_bKeepTempStatusBarVisibleOnMediaLoad`
- *View geometry, zoom and fullscreen*: `m_bDelaySetOutputRect`, `m_bNeedZoomAfterFullscreenExit`, `m_fStartInD3DFullscreen`, `m_fStartInFullscreenSeparate`

**Calls**: `CloseMediaInternal`, `CreateFullScreenWindow`, `IsFullScreenMode`, `OpenMediaPrivate`, `RestoreStatusBarMessageHold`, `SetLoadState`

### `OpenMediaPrivate`

`MainFrm.cpp:16296`, 398 lines, graph thread when used. Builds the graph (graph thread when used): OpenCreateGraphObject, OpenFile or OpenDVD, OpenCustomizeGraph, interface queries, renderer, subtitle and OSD setup, then posts WM_POSTOPEN or WM_OPENFAILED.

**Playback state**

- *Filter graph and DirectShow interfaces*: `m_ActiveGraphNotifyEvCode`, `m_pAudioSwitcherSS`, `m_pFSF`, `m_pGB`, `m_pLN21`, `m_pMS`
- *Seek-preview graph*: `m_bUseSeekPreview`, `m_pCAP2_preview`, `m_pGB_preview`, `m_pMC_preview`, `m_pMFVDC_preview`, `m_pMFVP_preview`, `m_pVMR9C_preview`
- *Video renderer and presenter interfaces*: `m_iDefRotation`, `m_pCAP`, `m_pCAP2`, `m_pCAP3`, `m_pD3DFSC`, `m_pMFVDC`, `m_pMFVMB`, `m_pMFVP`, `m_pMPCVRSR`, `m_pMVRC`, `m_pMVRFG`, `m_pMVRI`, `m_pMVRS`, `m_pMVRSR`, `m_pMVTO`, `m_pVMB`, `m_pVMRMC`, `m_pVMRWC`
- *Media and stream state*: `defaultVideoAngle`, `m_fAudioOnly`, `m_loadedAudioTrackIndex`, `m_loadedSubtitleTrackIndex`, `m_replayGain`
- *Subtitles engine*: `m_pSubStreams`, `m_posFirstExtSub`
- *DVD and Blu-ray state*: `m_fValidDVDOpen`
- *Open/close lifecycle, threading and locks*: `m_OnClose_called`, `m_OnClose_queued`, `m_bOpenMediaActive`, `m_bOpenedThroughThread`, `m_closingmsg`, `m_fOpeningAborted`
- *Reload, resume and A-B repeat*: `abRepeat`, `m_bRememberFilePos`, `m_rtReloadPos`, `reloadABRepeat`

**UI state**

- *Windows, bars and panels*: `m_pOSDWnd`, `m_pVideoWnd`, `m_wndPlaylistBar`, `m_wndPreView`, `m_wndSeekBar`
- *OSD and status text*: `m_OSD`
- *View geometry, zoom and fullscreen*: `m_fFullScreen`

**Calls**: `ApplyReplayGain`, `GetLoadState`, `GetPlaybackMode`, `IsD3DFullScreenMode`, `IsValidSubtitleStream`, `LoadSubtitle`, `OnFilePostOpenmedia`, `OnOpenMediaFailed`, `OnPlayAudio`, `OpenCreateGraphObject`, `OpenCustomizeGraph`, `OpenDVD`, `OpenFile`, `OpenSetupAudio`, `OpenSetupVideo`, `OpenSetupWindowTitle`, `ParseReplayGainValue`, `SetAudioDelay`, `SetSubtitle`, `SetupAudioStreams`, `SetupSubtitleStreams`, `SetupVMR9ColorControl`

### `OpenCreateGraphObject`

`MainFrm.cpp:14117`, 132 lines, graph thread when used. Creates m_pGB (CFGManagerPlayer, CFGManagerDVD or the Shockwave graph) and the seek-preview graph, and queries the basic interfaces.

**Playback state**

- *Filter graph and DirectShow interfaces*: `m_fCustomGraph`, `m_fShockwaveGraph`, `m_pBA`, `m_pBV`, `m_pCB`, `m_pFS`, `m_pGB`, `m_pMC`, `m_pME`, `m_pMS`, `m_pProv`, `m_pVW`
- *Seek-preview graph*: `m_bUseSeekPreview`, `m_pBV_preview`, `m_pGB_preview`, `m_pMC_preview`, `m_pMS_preview`, `m_pVW_preview`

**UI state**

- *Windows, bars and panels*: `m_pVideoWnd`, `m_wndPreView`

**Calls**: `IsAudioFileExt`

### `OpenFile`

`MainFrm.cpp:14555`, 375 lines, graph thread when used. Renders the file(s) into the graph, handles youtube-dl and Blu-ray playlist inputs, chapters and audio switcher setup.

**Playback state**

- *Filter graph and DirectShow interfaces*: `m_fCustomGraph`, `m_pAMMC`, `m_pAMNS`, `m_pAMOP`, `m_pAudioSwitcherSS`, `m_pFSF`, `m_pGB`, `m_pKFI`, `m_pLN21`, `m_pME`, `m_pOtherSS`, `m_pSplitterDubSS`, `m_pSplitterSS`
- *Seek-preview graph*: `m_bUseSeekPreview`, `m_pGB_preview`
- *Media and stream state*: `lastOpenFile`
- *Subtitles engine*: `m_pDVS`, `m_pDVS2`
- *DVD and Blu-ray state*: `m_BDMeta`, `m_bHasBDMeta`
- *Open/close lifecycle, threading and locks*: `m_fOpeningAborted`
- *Reload, resume and A-B repeat*: `m_iReloadAudioIdx`, `m_iReloadSubIdx`, `m_reloadFilename`, `m_rtReloadPos`, `reloadABRepeat`

**UI state**

- *Windows, bars and panels*: `m_wndPlaylistBar`

**Calls**: `HandleMultipleEntryRar`, `IsImageFile`, `ReleasePreviewGraph`, `SetPlaybackMode`, `SetupChapters`, `ShowMediaTypesDialog`

### `OpenDVD`

`MainFrm.cpp:15270`, 127 lines, graph thread when used. Renders a DVD through the DVD graph manager, takes IDvdControl2/IDvdInfo2 from the navigator (also for the preview graph) and maps navigator pin errors to messages.

**Playback state**

- *Filter graph and DirectShow interfaces*: `m_pAudioSwitcherSS`, `m_pGB`, `m_pLN21`, `m_pOtherSS`
- *Seek-preview graph*: `m_bUseSeekPreview`, `m_pDVDC_preview`, `m_pDVDI_preview`, `m_pGB_preview`
- *Media and stream state*: `lastOpenFile`
- *Subtitles engine*: `m_pDVS`, `m_pDVS2`
- *DVD and Blu-ray state*: `m_iDVDDomain`, `m_pDVDC`, `m_pDVDI`

**UI state**: none

**Calls**: `SetPlaybackMode`, `ShowMediaTypesDialog`

### `OpenCustomizeGraph`

`MainFrm.cpp:15400`, 48 lines, graph thread when used. Post-build graph tweaks: CleanGraph, the text pass-through filter for internal subtitles, the sync-renderer reference clock, and the DVD subtitle clock.

**Playback state**

- *Filter graph and DirectShow interfaces*: `m_pGB`, `m_pRefClock`, `m_pSyncClock`
- *Video renderer and presenter interfaces*: `m_pCAP`
- *Subtitles engine*: `m_pDVS2`, `m_pSubClock`

**UI state**: none

**Calls**: `AddTextPassThruFilter`, `CleanGraph`, `GetPlaybackMode`

### `OpenSetupGetVideoSize`

`MainFrm.cpp:15450`, 33 lines, graph thread when used. Native video size from the renderer, used to size the window.

**Playback state**

- *Filter graph and DirectShow interfaces*: `m_pGB`, `m_pVW`
- *Video renderer and presenter interfaces*: `m_pCAP`, `m_pMFVDC`
- *Media and stream state*: `m_fAudioOnly`

**UI state**: none

**Calls**: none

### `OpenSetupVideo`

`MainFrm.cpp:15486`, 37 lines, graph thread when used. Initial video window, renderer target and audio-only detection.

**Playback state**

- *Filter graph and DirectShow interfaces*: `m_fShockwaveGraph`, `m_pVW`
- *Seek-preview graph*: `m_bUseSeekPreview`, `m_pVW_preview`
- *Media and stream state*: `m_fAudioOnly`
- *Open/close lifecycle, threading and locks*: `lockGraphAccess`

**UI state**

- *Windows, bars and panels*: `m_pDedicatedFSVideoWnd`, `m_pVideoWnd`, `m_wndPreView`
- *OSD and status text*: `m_statusbarVideoSize`

**Calls**: `HasDedicatedFSVideoWindow`, `OpenSetupGetVideoSize`, `UpdateDXVAStatus`

### `OpenSetupAudio`

`MainFrm.cpp:15526`, 15 lines, graph thread when used. Applies volume and balance to IBasicAudio.

**Playback state**

- *Filter graph and DirectShow interfaces*: `m_pBA`

**UI state**

- *Windows, bars and panels*: `m_wndToolBar`

**Calls**: none

### `OpenSetupInfoBar`

`MainFrm.cpp:15543`, 61 lines. Fills the info bar from media metadata.

**Playback state**

- *Filter graph and DirectShow interfaces*: `m_pAMMC`

**UI state**

- *Windows, bars and panels*: `m_wndInfoBar`

**Calls**: `GetPlaybackMode`, `RecalcLayout`, `UpdateChapterInInfoBar`

### `OpenSetupStatsBar`

`MainFrm.cpp:15633`, 48 lines. Creates the statistics bar lines.

**Playback state**

- *Filter graph and DirectShow interfaces*: `m_pBI`, `m_pGB`, `m_pQP`

**UI state**

- *Windows, bars and panels*: `m_wndStatsBar`

**Calls**: `GetLoadState`

### `OpenSetupStatusBar`

`MainFrm.cpp:15841`, 6 lines. Status bar setup for the new media.

**Playback state**: none

**UI state**

- *Windows, bars and panels*: `m_wndStatusBar`

**Calls**: `CheckSelectedAudioStream`, `CheckSelectedVideoStream`

### `OpenSetupWindowTitle`

`MainFrm.cpp:15850`, 50 lines, graph thread when used. Window title from file name or metadata.

**Playback state**

- *DVD and Blu-ray state*: `m_pDVDI`

**UI state**

- *Windows, bars and panels*: `m_wndPlaylistBar`

**Calls**: `GetFileName`, `GetPlaybackMode`, `getBestTitle`

### `OnFilePostOpenmedia`

`MainFrm.cpp:4294`, 189 lines. UI-thread completion of a successful open (WM_POSTOPEN): MLS::LOADED, playlist validity and duration, bars, menus and timers.

**Playback state**

- *Filter graph and DirectShow interfaces*: `m_pGB`, `m_pMS`, `m_pVW`
- *Video renderer and presenter interfaces*: `m_bToggleShader`, `m_bToggleShaderScreenSpace`, `m_pCAP`, `m_pCAP2`, `m_pMFVDC`, `m_pVMRWC`
- *Media and stream state*: `m_bFirstPlay`, `m_fAudioOnly`, `m_lastOMD`, `m_nLastSkipDirection`
- *Open/close lifecycle, threading and locks*: `m_OpenMediaFailedCount`, `m_bOpenMediaActive`, `m_timerOneTime`
- *Reload, resume and A-B repeat*: `m_bRememberFilePos`

**UI state**

- *Windows, bars and panels*: `m_pDedicatedFSVideoWnd`, `m_wndEditListEditor`, `m_wndPlaylistBar`, `m_wndSeekBar`
- *View geometry, zoom and fullscreen*: `m_AngleX`, `m_AngleY`, `m_AngleZ`, `m_bAllowWindowZoom`, `m_bDelaySetOutputRect`, `m_bNeedZoomAfterFullscreenExit`, `m_bOpeningInAutochangedMonitorMode`, `m_fFullScreen`, `m_fStartInD3DFullscreen`, `m_fStartInFullscreenSeparate`
- *Dialogs and menus*: `m_bSettingUpMenus`

**Calls**: `ApplyPanNScanPresetString`, `AutoChangeMonitorMode`, `CanPreviewUse`, `GetLoadState`, `GetMediaStateDirect`, `HasDedicatedFSVideoWindow`, `IsAeroSnapped`, `IsFullScreenMode`, `LoadDynamicMenus`, `LoadKeyFrames`, `MediaControlPause`, `MediaTransportControlSetMedia`, `MoveVideoWindow`, `OnPlayPlay`, `OnTimer`, `OnViewFullscreen`, `OpenSetupInfoBar`, `OpenSetupStatsBar`, `OpenSetupStatusBar`, `PerformFlipRotate`, `RecalcLayout`, `SendNowPlayingToApi`, `SetLoadState`, `SetShaders`, `UpdateControlState`, `ZoomVideoWindow`

### `OnOpenMediaFailed`

`MainFrm.cpp:4485`, 108 lines. UI-thread handling of a failed open (WM_OPENFAILED): error reporting, playlist validity, skip to next, command-line exits.

**Playback state**

- *Media and stream state*: `m_lastOMD`, `m_nLastSkipDirection`, `m_nLoops`, `m_sydlLastProcessURL`
- *Open/close lifecycle, threading and locks*: `m_OpenMediaFailedCount`, `m_bOpenMediaActive`, `m_closingmsg`
- *Reload, resume and A-B repeat*: `m_iReloadAudioIdx`, `m_iReloadSubIdx`, `m_reloadFilename`, `m_rtReloadPos`, `reloadABRepeat`

**UI state**

- *Windows, bars and panels*: `m_wndPlaylistBar`
- *OSD and status text*: `m_OSD`

**Calls**: `CloseMedia`, `DoAfterPlaybackEvent`, `GetLoadState`, `OpenCurPlaylistItem`, `SearchInDir`, `SetLoadState`, `ShowStatusBarForMessage`

## Close path

### `CloseMedia`

`MainFrm.cpp:19805`, 19 lines. Public wrapper: defers the close while a CDeferredActionScope holder is on the stack, otherwise takes the scope and calls CloseMediaInternal.

**Playback state**

- *Open/close lifecycle, threading and locks*: `m_deferredActions`, `m_nDeferredActionDepth`

**UI state**: none

**Calls**: `CloseMedia`, `CloseMediaInternal`

### `CloseMediaInternal`

`MainFrm.cpp:19826`, 541 lines. UI-thread half of the close: aborts an open in progress (MLS::ABORTING), drops pending open messages, sets MLS::CLOSING, posts TM_CLOSE or calls CloseMediaPrivate, and pumps messages with timeouts and deadlock prompts while it waits.

**Playback state**

- *Filter graph and DirectShow interfaces*: `m_ActiveGraphNotifyEvCode`, `m_pAMOP`, `m_pGB`, `m_pME`, `m_pMS`
- *Seek-preview graph*: `m_bUseSeekPreview`, `m_pGB_preview`
- *Media and stream state*: `lastOpenFile`, `m_dwLastPause`, `m_fEndOfStream`, `m_media_trans_control`
- *Subtitles engine*: `m_pCurrentSubInput`
- *DVD and Blu-ray state*: `m_bDVDStillOn`, `m_pDVDI`
- *Open/close lifecycle, threading and locks*: `lockGraphAccess`, `m_OnClose_queued`, `m_bOpenedThroughThread`, `m_closingmsg`, `m_eMediaLoadState`, `m_evClosePrivateFinished`, `m_evOpenPrivateFinished`, `m_fOpeningAborted`, `m_pGraphThread`
- *Reload, resume and A-B repeat*: `m_bRememberFilePos`

**UI state**

- *Windows, bars and panels*: `m_controls`, `m_wndPreView`, `m_wndStatusBar`
- *View geometry, zoom and fullscreen*: `m_fFullScreen`
- *Dialogs and menus*: `lockModalDialog`, `m_bSettingUpMenus`, `m_pSubtitlesProviders`, `m_wndSubtitlesDownloadDialog`, `mediaTypesErrorDlg`, `rarEntrySelectorDlg`

**Calls**: `CloseMediaPrivate`, `ForceCloseProcess`, `GetLoadState`, `GetMediaState`, `GetPlaybackMode`, `MediaControlPause`, `OnFilePostClosemedia`, `OnPlayStop`, `PreviewWindowHide`, `SetLoadState`, `SetupAudioSubMenu`, `SetupFiltersSubMenu`, `SetupJumpToSubMenus`, `SetupSubtitlesSubMenu`, `SetupVideoStreamsSubMenu`, `SubtitlesSave`, `ThrowAndForceClose`

### `CloseMediaPrivate`

`MainFrm.cpp:16696`, 125 lines, graph thread when used. Tears the graph down (graph thread when used): stops it and releases every interface and renderer object.

**Playback state**

- *Filter graph and DirectShow interfaces*: `m_CachedFilterState`, `m_fCustomGraph`, `m_fLiveWM`, `m_fShockwaveGraph`, `m_pAMMC`, `m_pAMNS`, `m_pAMOP`, `m_pAudioSwitcherSS`, `m_pBA`, `m_pBI`, `m_pBV`, `m_pCB`, `m_pFS`, `m_pFSF`, `m_pGB`, `m_pKFI`, `m_pLN21`, `m_pMC`, `m_pME`, `m_pMS`, `m_pOtherSS`, `m_pProv`, `m_pQP`, `m_pSplitterDubSS`, `m_pSplitterSS`, `m_pSyncClock`, `m_pVW`
- *Seek-preview graph*: `m_pGB_preview`, `m_pVW_preview`
- *Video renderer and presenter interfaces*: `m_bIsMPCVRExclusiveMode`, `m_bUsingDXVA`, `m_pCAP`, `m_pCAP2`, `m_pCAP3`, `m_pD3DFSC`, `m_pMFVDC`, `m_pMFVMB`, `m_pMFVP`, `m_pMPCVRSR`, `m_pMVRC`, `m_pMVRFG`, `m_pMVRI`, `m_pMVRS`, `m_pMVRSR`, `m_pMVTO`, `m_pVMB`, `m_pVMRMC`, `m_pVMRWC`
- *Media and stream state*: `m_audioTrackCount`, `m_bBuffering`, `m_fEndOfStream`, `m_lastOMD`
- *Subtitles engine*: `m_ExternalSubstreams`, `m_FontInstaller`, `m_csSubLock`, `m_pCurrentSubInput`, `m_pDVS`, `m_pDVS2`, `m_pSecondarySubInput`, `m_pSubClock`, `m_pSubStreams`
- *DVD and Blu-ray state*: `m_pDVDC`, `m_pDVDI`

**UI state**

- *OSD and status text*: `m_OSD`

**Calls**: `GetLoadState`, `MediaControlStop`, `ReleasePreviewGraph`, `ResetAutoCopySubtitle`

### `OnFilePostClosemedia`

`MainFrm.cpp:4595`, 76 lines. UI-thread reset after close: bars, OSD, title, panels and layout.

**Playback state**

- *Video renderer and presenter interfaces*: `m_HWAccelType`
- *Media and stream state*: `currentAudioLang`, `currentSubLang`, `m_kfs`
- *Subtitles engine*: `m_lSubtitleShift`, `m_nCurSubtitle`
- *Open/close lifecycle, threading and locks*: `m_closingmsg`
- *Reload, resume and A-B repeat*: `abRepeat`

**UI state**

- *Windows, bars and panels*: `m_controls`, `m_pDedicatedFSVideoWnd`, `m_wndEditListEditor`, `m_wndInfoBar`, `m_wndSeekBar`, `m_wndStatsBar`, `m_wndStatusBar`, `m_wndView`
- *OSD and status text*: `m_OSD`, `m_statusbarVideoFormat`, `m_statusbarVideoSize`
- *View geometry, zoom and fullscreen*: `m_AngleX`, `m_AngleY`, `m_AngleZ`, `m_PosX`, `m_PosY`, `m_ZoomX`, `m_ZoomY`, `m_fStartInD3DFullscreen`, `m_fStartInFullscreenSeparate`

**Calls**: `HasDedicatedFSVideoWindow`, `IsD3DFullScreenMode`, `OpenSetupWindowTitle`, `RecalcLayout`, `SetAlwaysOnTop`, `SetLoadState`, `SetPlaybackMode`, `UpdateControlState`

## Hazards for extraction

### UI objects that carry playback state

These are UI members, but the engine path reads or writes playback data through them. They
need an engine-side owner (or an explicit callback) before the open and close paths can move.

- `m_fFullScreen`: OpenMediaPrivate uses it to pick how the OSD attaches to the renderer (MPCVR bitmap path). Touched by `CloseMediaInternal`, `OpenMediaPrivate`, `OnFilePostOpenmedia`.
- `m_fStartInD3DFullscreen`: OpenMediaInternal creates the dedicated fullscreen window and repoints m_pVideoWnd at it before the graph is built. Touched by `OnFilePostClosemedia`, `OpenMediaInternal`, `OnFilePostOpenmedia`.
- `m_pVideoWnd`: its HWND is passed to CFGManagerPlayer/CFGManagerDVD/CShockwaveGraph in OpenCreateGraphObject and to the EVR in OpenMediaPrivate. Touched by `OpenMediaInternal`, `OpenMediaPrivate`, `OpenCreateGraphObject`, `OpenSetupVideo`.
- `m_wndPlaylistBar`: owns the playlist model (m_pl): the open path reads the next item and writes validity and duration back into it. Touched by `OnGraphNotify`, `GraphEventComplete`, `OpenCurPlaylistItem`, `OpenMediaInternal`, `OpenMediaPrivate`, `OpenFile`, `OpenSetupWindowTitle`, `OnFilePostOpenmedia`, `OnOpenMediaFailed`.
- `m_wndPreView`: its video HWND hosts the seek-preview graph. Touched by `CloseMediaInternal`, `OpenMediaPrivate`, `OpenCreateGraphObject`, `OpenSetupVideo`.
- `m_wndSeekBar`: holds position and duration: DoSeekTo clamps against its range, and OnUpdatePlayerStatus derives m_fLiveWM from it, instead of asking the graph. Touched by `OnGraphNotify`, `OnFilePostClosemedia`, `OpenMediaPrivate`, `OnFilePostOpenmedia`.
- `m_wndToolBar`: OpenSetupAudio reads the volume from the toolbar slider (m_wndToolBar.Volume) and applies it to IBasicAudio. Touched by `OpenSetupAudio`.

Playback members that reach outward:

- `m_media_trans_control`: Windows SMTC integration: reflects playback state outward. Touched by `CloseMediaInternal`.
- `m_timerOneTime`: shared timer pool: also drives UI timers. Touched by `OnFilePostOpenmedia`.

### UI state touched on the graph thread

When the graph thread is used, these UI fields are read or written from `OpenMediaPrivate`, `CloseMediaPrivate` (directly, per the
extraction). A core library has to receive them as inputs or report through callbacks instead:

- `CloseMediaPrivate`: `m_OSD`
- `OpenCreateGraphObject`: `m_pVideoWnd`, `m_wndPreView`
- `OpenFile`: `m_wndPlaylistBar`
- `OpenMediaPrivate`: `m_OSD`, `m_fFullScreen`, `m_pOSDWnd`, `m_pVideoWnd`, `m_wndPlaylistBar`, `m_wndPreView`, `m_wndSeekBar`
- `OpenSetupAudio`: `m_wndToolBar`
- `OpenSetupVideo`: `m_pDedicatedFSVideoWnd`, `m_pVideoWnd`, `m_statusbarVideoSize`, `m_wndPreView`
- `OpenSetupWindowTitle`: `m_wndPlaylistBar`

Notably, `OpenMediaPrivate` calls `OpenSetupWindowTitle()` unguarded (`MainFrm.cpp:16613`), so on
the worker it reads `m_wndPlaylistBar` and calls `SetWindowText` cross-thread (a synchronous
`WM_SETTEXT` to the UI thread). That only completes while the UI thread keeps processing
messages, and it is the kind of call the core/shell split has to turn into a notification.

### Coordination state shared by both halves

The *Open/close lifecycle, threading and locks* group is shared between the UI-thread functions and the graph-thread ones: for
example `CloseMediaInternal` sets `m_fOpeningAborted` and `OpenMediaPrivate` polls it.
`m_eMediaLoadState`, `m_bOpenMediaActive`, `m_fOpeningAborted`, `m_OnClose_called` and
`m_OnClose_queued` are declared `volatile`; the deferred-action queue (`m_deferredActions`,
`m_nDeferredActionDepth`) is how nested open and close requests are serialised on the UI thread.
This is the natural boundary of a core state machine: the extracted core should own load state and
the open/close sequencing, and the shell should only request transitions and observe them.

## Member index

Every field touched by the functions above, with its classification.

### Playback state

**Filter graph and DirectShow interfaces**

| Member | Declared as | Touched by |
|---|---|---|
| `m_ActiveGraphNotifyEvCode` | `volatile LONG m_ActiveGraphNotifyEvCode = 0` | OnGraphNotify, CloseMediaInternal, OpenMediaInternal, OpenMediaPrivate |
| `m_CachedFilterState` | `OAFilterState m_CachedFilterState` | CloseMediaPrivate |
| `m_fCustomGraph` | `bool m_fCustomGraph` | OnGraphNotify, CloseMediaPrivate, OpenCreateGraphObject, OpenFile |
| `m_fLiveWM` | `bool m_fLiveWM` | CloseMediaPrivate |
| `m_fShockwaveGraph` | `bool m_fShockwaveGraph` | CloseMediaPrivate, OpenCreateGraphObject, OpenSetupVideo |
| `m_pAMMC` | `CComQIPtr<IAMMediaContent, &IID_IAMMediaContent> m_pAMMC[2]` | CloseMediaPrivate, OpenFile, OpenSetupInfoBar |
| `m_pAMNS` | `CComQIPtr<IAMNetworkStatus, &IID_IAMNetworkStatus> m_pAMNS` | CloseMediaPrivate, OpenFile |
| `m_pAMOP` | `CComQIPtr<IAMOpenProgress> m_pAMOP` | CloseMediaInternal, CloseMediaPrivate, OpenFile |
| `m_pAudioSwitcherSS` | `CComQIPtr<IAMStreamSelect> m_pAudioSwitcherSS` | CloseMediaPrivate, OpenMediaPrivate, OpenFile, OpenDVD |
| `m_pBA` | `CComQIPtr<IBasicAudio> m_pBA` | GraphEventComplete, CloseMediaPrivate, OpenCreateGraphObject, OpenSetupAudio |
| `m_pBI` | `CComQIPtr<IBufferInfo> m_pBI` | CloseMediaPrivate, OpenSetupStatsBar |
| `m_pBV` | `CComQIPtr<IBasicVideo> m_pBV` | CloseMediaPrivate, OpenCreateGraphObject |
| `m_pCB` | `CComPtr<IDSMChapterBag> m_pCB` | CloseMediaPrivate, OpenCreateGraphObject |
| `m_pFS` | `CComQIPtr<IVideoFrameStep> m_pFS` | CloseMediaPrivate, OpenCreateGraphObject |
| `m_pFSF` | `CComQIPtr<IFileSourceFilter> m_pFSF` | CloseMediaPrivate, OpenMediaPrivate, OpenFile |
| `m_pGB` | `CComPtr<IGraphBuilder2> m_pGB` | CloseMediaInternal, CloseMediaPrivate, OpenMediaPrivate, OpenCreateGraphObject, OpenFile, OpenDVD, OpenCustomizeGraph, OpenSetupGetVideoSize, OpenSetupStatsBar, OnFilePostOpenmedia |
| `m_pKFI` | `CComQIPtr<IKeyFrameInfo> m_pKFI` | CloseMediaPrivate, OpenFile |
| `m_pLN21` | `CComPtr<IAMLine21Decoder_2> m_pLN21` | CloseMediaPrivate, OpenMediaPrivate, OpenFile, OpenDVD |
| `m_pMC` | `CComQIPtr<IMediaControl> m_pMC` | CloseMediaPrivate, OpenCreateGraphObject |
| `m_pME` | `CComQIPtr<IMediaEventEx> m_pME` | OnGraphNotify, CloseMediaInternal, CloseMediaPrivate, OpenCreateGraphObject, OpenFile |
| `m_pMS` | `CComQIPtr<IMediaSeeking> m_pMS` | OnGraphNotify, GraphEventComplete, CloseMediaInternal, CloseMediaPrivate, OpenMediaPrivate, OpenCreateGraphObject, OnFilePostOpenmedia |
| `m_pOtherSS` | `CComQIPtr<IAMStreamSelect> m_pOtherSS[2]` | CloseMediaPrivate, OpenFile, OpenDVD |
| `m_pProv` | `CComPtr<IUnknown> m_pProv` | CloseMediaPrivate, OpenCreateGraphObject |
| `m_pQP` | `CComQIPtr<IQualProp, &IID_IQualProp> m_pQP` | CloseMediaPrivate, OpenSetupStatsBar |
| `m_pRefClock` | `CComPtr<IBaseFilter> m_pRefClock` | OpenCustomizeGraph |
| `m_pSplitterDubSS` | `CComQIPtr<IAMStreamSelect> m_pSplitterDubSS` | CloseMediaPrivate, OpenFile |
| `m_pSplitterSS` | `CComQIPtr<IAMStreamSelect> m_pSplitterSS` | CloseMediaPrivate, OpenFile |
| `m_pSyncClock` | `CComPtr<ISyncClock> m_pSyncClock` | CloseMediaPrivate, OpenCustomizeGraph |
| `m_pVW` | `CComQIPtr<IVideoWindow> m_pVW` | CloseMediaPrivate, OpenCreateGraphObject, OpenSetupGetVideoSize, OpenSetupVideo, OnFilePostOpenmedia |

**Seek-preview graph**

| Member | Declared as | Touched by |
|---|---|---|
| `m_bUseSeekPreview` | `bool m_bUseSeekPreview` | OnGraphNotify, CloseMediaInternal, OpenMediaPrivate, OpenCreateGraphObject, OpenFile, OpenDVD, OpenSetupVideo |
| `m_pBV_preview` | `CComQIPtr<IBasicVideo> m_pBV_preview` | OpenCreateGraphObject |
| `m_pCAP2_preview` | `CComPtr<ISubPicAllocatorPresenter2> m_pCAP2_preview` | OpenMediaPrivate |
| `m_pDVDC_preview` | `CComQIPtr<IDvdControl2> m_pDVDC_preview` | OpenDVD |
| `m_pDVDI_preview` | `CComQIPtr<IDvdInfo2> m_pDVDI_preview` | OpenDVD |
| `m_pGB_preview` | `CComPtr<IGraphBuilder2> m_pGB_preview` | CloseMediaInternal, CloseMediaPrivate, OpenMediaPrivate, OpenCreateGraphObject, OpenFile, OpenDVD |
| `m_pMC_preview` | `CComQIPtr<IMediaControl> m_pMC_preview` | OpenMediaPrivate, OpenCreateGraphObject |
| `m_pMFVDC_preview` | `CComPtr<IMFVideoDisplayControl> m_pMFVDC_preview` | OpenMediaPrivate |
| `m_pMFVP_preview` | `CComPtr<IMFVideoProcessor> m_pMFVP_preview` | OpenMediaPrivate |
| `m_pMS_preview` | `CComQIPtr<IMediaSeeking> m_pMS_preview` | OpenCreateGraphObject |
| `m_pVMR9C_preview` | `CComPtr<IVMRWindowlessControl9> m_pVMR9C_preview` | OpenMediaPrivate |
| `m_pVW_preview` | `CComQIPtr<IVideoWindow> m_pVW_preview` | CloseMediaPrivate, OpenCreateGraphObject, OpenSetupVideo |

**Video renderer and presenter interfaces**

| Member | Declared as | Touched by |
|---|---|---|
| `m_HWAccelType` | `LPCTSTR m_HWAccelType` | OnFilePostClosemedia |
| `m_bIsMPCVRExclusiveMode` | `bool m_bIsMPCVRExclusiveMode = false` | CloseMediaPrivate |
| `m_bToggleShader` | `bool m_bToggleShader` | OnFilePostOpenmedia |
| `m_bToggleShaderScreenSpace` | `bool m_bToggleShaderScreenSpace` | OnFilePostOpenmedia |
| `m_bUsingDXVA` | `bool m_bUsingDXVA` | CloseMediaPrivate |
| `m_iDefRotation` | `int m_iDefRotation` | OpenMediaPrivate |
| `m_pCAP` | `CComPtr<ISubPicAllocatorPresenter> m_pCAP` | CloseMediaPrivate, OpenMediaPrivate, OpenCustomizeGraph, OpenSetupGetVideoSize, OnFilePostOpenmedia |
| `m_pCAP2` | `CComPtr<ISubPicAllocatorPresenter2> m_pCAP2` | CloseMediaPrivate, OpenMediaPrivate, OnFilePostOpenmedia |
| `m_pCAP3` | `CComPtr<ISubPicAllocatorPresenter3> m_pCAP3` | CloseMediaPrivate, OpenMediaPrivate |
| `m_pD3DFSC` | `CComPtr<ID3DFullscreenControl> m_pD3DFSC` | CloseMediaPrivate, OpenMediaPrivate |
| `m_pMFVDC` | `CComPtr<IMFVideoDisplayControl> m_pMFVDC` | CloseMediaPrivate, OpenMediaPrivate, OpenSetupGetVideoSize, OnFilePostOpenmedia |
| `m_pMFVMB` | `CComPtr<IMFVideoMixerBitmap> m_pMFVMB` | CloseMediaPrivate, OpenMediaPrivate |
| `m_pMFVP` | `CComPtr<IMFVideoProcessor> m_pMFVP` | CloseMediaPrivate, OpenMediaPrivate |
| `m_pMPCVRSR` | `CComPtr<IMPCVRSubclassReplacement> m_pMPCVRSR` | CloseMediaPrivate, OpenMediaPrivate |
| `m_pMVRC` | `CComPtr<IMadVRCommand> m_pMVRC` | CloseMediaPrivate, OpenMediaPrivate |
| `m_pMVRFG` | `CComPtr<IMadVRFrameGrabber> m_pMVRFG` | CloseMediaPrivate, OpenMediaPrivate |
| `m_pMVRI` | `CComPtr<IMadVRInfo> m_pMVRI` | CloseMediaPrivate, OpenMediaPrivate |
| `m_pMVRS` | `CComPtr<IMadVRSettings> m_pMVRS` | CloseMediaPrivate, OpenMediaPrivate |
| `m_pMVRSR` | `CComPtr<IMadVRSubclassReplacement> m_pMVRSR` | CloseMediaPrivate, OpenMediaPrivate |
| `m_pMVTO` | `CComPtr<IMadVRTextOsd> m_pMVTO` | CloseMediaPrivate, OpenMediaPrivate |
| `m_pVMB` | `CComPtr<IVMRMixerBitmap9> m_pVMB` | CloseMediaPrivate, OpenMediaPrivate |
| `m_pVMRMC` | `CComPtr<IVMRMixerControl9> m_pVMRMC` | CloseMediaPrivate, OpenMediaPrivate |
| `m_pVMRWC` | `CComPtr<IVMRWindowlessControl9> m_pVMRWC` | CloseMediaPrivate, OpenMediaPrivate, OnFilePostOpenmedia |

**Media and stream state**

| Member | Declared as | Touched by |
|---|---|---|
| `currentAudioLang` | `CString currentAudioLang` | OnFilePostClosemedia |
| `currentSubLang` | `CString currentSubLang` | OnFilePostClosemedia |
| `defaultVideoAngle` | `int defaultVideoAngle` | OpenMediaPrivate |
| `lastOpenFile` | `CString lastOpenFile` | GraphEventComplete, CloseMediaInternal, OpenFile, OpenDVD |
| `m_audioTrackCount` | `int m_audioTrackCount = 0` | OnGraphNotify, CloseMediaPrivate |
| `m_bBuffering` | `bool m_bBuffering` | OnGraphNotify, CloseMediaPrivate |
| `m_bFirstPlay` | `bool m_bFirstPlay` | OnFilePostOpenmedia |
| `m_bKeepLoopCountOnStop` | `bool m_bKeepLoopCountOnStop = false` | GraphEventComplete |
| `m_dwLastPause` | `ULONGLONG m_dwLastPause` | CloseMediaInternal |
| `m_fAudioOnly` | `bool m_fAudioOnly` | OnGraphNotify, OpenMediaPrivate, OpenSetupGetVideoSize, OpenSetupVideo, OnFilePostOpenmedia |
| `m_fEndOfStream` | `bool m_fEndOfStream` | GraphEventComplete, CloseMediaInternal, CloseMediaPrivate |
| `m_fFrameSteppingActive` | `bool m_fFrameSteppingActive` | OnGraphNotify, GraphEventComplete |
| `m_kfs` | `std::vector<REFERENCE_TIME> m_kfs` | OnFilePostClosemedia |
| `m_lastOMD` | `CAutoPtr<OpenMediaData> m_lastOMD` | OnGraphNotify, CloseMediaPrivate, OnFilePostOpenmedia, OnOpenMediaFailed |
| `m_loadedAudioTrackIndex` | `int m_loadedAudioTrackIndex = -1` | OpenMediaPrivate |
| `m_loadedSubtitleTrackIndex` | `int m_loadedSubtitleTrackIndex = -1` | OpenMediaPrivate |
| `m_media_trans_control` | `MediaTransControls m_media_trans_control` | CloseMediaInternal |
| `m_nLastSkipDirection` | `UINT m_nLastSkipDirection` | OnFilePostOpenmedia, OnOpenMediaFailed |
| `m_nLoops` | `int m_nLoops` | GraphEventComplete, OnOpenMediaFailed |
| `m_nStepForwardCount` | `int m_nStepForwardCount` | OnGraphNotify, GraphEventComplete |
| `m_nVolumeBeforeFrameStepping` | `int m_nVolumeBeforeFrameStepping` | GraphEventComplete |
| `m_replayGain` | `struct ReplayGainInfo m_replayGain` | OpenMediaPrivate |
| `m_sydlLastProcessURL` | `CString m_sydlLastProcessURL` | OpenCurPlaylistItem, OnOpenMediaFailed |

**Subtitles engine**

| Member | Declared as | Touched by |
|---|---|---|
| `m_ExternalSubstreams` | `std::list<ISubStream*> m_ExternalSubstreams` | CloseMediaPrivate |
| `m_FontInstaller` | `CFontInstaller m_FontInstaller` | CloseMediaPrivate |
| `m_csSubLock` | `CCritSec m_csSubLock` | CloseMediaPrivate |
| `m_lSubtitleShift` | `long m_lSubtitleShift` | OnFilePostClosemedia |
| `m_nCurSubtitle` | `int m_nCurSubtitle` | OnFilePostClosemedia |
| `m_pCurrentSubInput` | `SubtitleInput m_pCurrentSubInput` | CloseMediaInternal, CloseMediaPrivate |
| `m_pDVS` | `CComQIPtr<IDirectVobSub> m_pDVS` | CloseMediaPrivate, OpenFile, OpenDVD |
| `m_pDVS2` | `CComQIPtr<IDirectVobSub2> m_pDVS2` | CloseMediaPrivate, OpenFile, OpenDVD, OpenCustomizeGraph |
| `m_pSecondarySubInput` | `SubtitleInput m_pSecondarySubInput` | CloseMediaPrivate |
| `m_pSubClock` | `CComPtr<ISubClock> m_pSubClock` | CloseMediaPrivate, OpenCustomizeGraph |
| `m_pSubStreams` | `CList<SubtitleInput> m_pSubStreams` | CloseMediaPrivate, OpenMediaPrivate |
| `m_posFirstExtSub` | `POSITION m_posFirstExtSub` | OpenMediaPrivate |

**DVD and Blu-ray state**

| Member | Declared as | Touched by |
|---|---|---|
| `m_BDMeta` | `CAtlList<CHdmvClipInfo::BDMVMeta> m_BDMeta` | OpenFile |
| `m_LastOpenBDPath` | `CString m_LastOpenBDPath` | OpenMediaInternal |
| `m_MPLSPlaylist` | `CHdmvClipInfo::HdmvPlaylist m_MPLSPlaylist` | OpenMediaInternal |
| `m_bDVDStillOn` | `bool m_bDVDStillOn` | OnGraphNotify, CloseMediaInternal |
| `m_bHasBDMeta` | `bool m_bHasBDMeta` | OpenFile |
| `m_bIsBDPlay` | `bool m_bIsBDPlay` | OpenMediaInternal |
| `m_fValidDVDOpen` | `bool m_fValidDVDOpen` | OnGraphNotify, OpenMediaPrivate |
| `m_iDVDDomain` | `DVD_DOMAIN m_iDVDDomain` | OnGraphNotify, OpenDVD |
| `m_iDVDTitle` | `DWORD m_iDVDTitle` | OnGraphNotify |
| `m_pDVDC` | `CComQIPtr<IDvdControl2> m_pDVDC` | OnGraphNotify, CloseMediaPrivate, OpenDVD |
| `m_pDVDI` | `CComQIPtr<IDvdInfo2> m_pDVDI` | OnGraphNotify, CloseMediaInternal, CloseMediaPrivate, OpenDVD, OpenSetupWindowTitle |

**Open/close lifecycle, threading and locks**

| Member | Declared as | Touched by |
|---|---|---|
| `lockGraphAccess` | `CCritSec lockGraphAccess` | OnGraphNotify, CloseMediaInternal, OpenSetupVideo |
| `m_OnClose_called` | `volatile bool m_OnClose_called = false` | OpenMediaInternal, OpenMediaPrivate |
| `m_OnClose_queued` | `volatile bool m_OnClose_queued = false` | CloseMediaInternal, OpenMediaInternal, OpenMediaPrivate |
| `m_OpenMediaFailedCount` | `int m_OpenMediaFailedCount` | OnFilePostOpenmedia, OnOpenMediaFailed |
| `m_bOpenMediaActive` | `volatile bool m_bOpenMediaActive` | OpenMediaInternal, OpenMediaPrivate, OnFilePostOpenmedia, OnOpenMediaFailed |
| `m_bOpenedThroughThread` | `bool m_bOpenedThroughThread` | CloseMediaInternal, OpenMediaInternal, OpenMediaPrivate |
| `m_closingmsg` | `CString m_tempstatus_msg, m_closingmsg` | CloseMediaInternal, OnFilePostClosemedia, OpenMediaPrivate, OnOpenMediaFailed |
| `m_deferredActions` | `std::vector<DeferredAction> m_deferredActions` | CloseMedia, OpenMedia |
| `m_eMediaLoadState` | `volatile MLS m_eMediaLoadState` | OnGraphNotify, CloseMediaInternal, OpenMediaInternal |
| `m_evClosePrivateFinished` | `ATL::CEvent m_evClosePrivateFinished` | CloseMediaInternal |
| `m_evOpenPrivateFinished` | `ATL::CEvent m_evOpenPrivateFinished` | CloseMediaInternal, OpenMediaInternal |
| `m_fOpeningAborted` | `volatile bool m_fOpeningAborted` | OnGraphNotify, CloseMediaInternal, OpenMediaPrivate, OpenFile |
| `m_nDeferredActionDepth` | `int m_nDeferredActionDepth = 0` | CloseMedia, OpenMedia |
| `m_pGraphThread` | `CGraphThread* m_pGraphThread` | CloseMediaInternal, OpenMediaInternal |
| `m_timerOneTime` | `OneTimeTimerPool<TimerOneTimeSubscriber> m_timerOneTime` | OnFilePostOpenmedia |

**Reload, resume and A-B repeat**

| Member | Declared as | Touched by |
|---|---|---|
| `abRepeat` | `ABRepeat abRepeat, reloadABRepeat` | OnGraphNotify, GraphEventComplete, OnFilePostClosemedia, OpenCurPlaylistItem, OpenMediaPrivate |
| `m_bRememberFilePos` | `bool m_bRememberFilePos` | GraphEventComplete, CloseMediaInternal, OpenMediaPrivate, OnFilePostOpenmedia |
| `m_iReloadAudioIdx` | `int m_iReloadAudioIdx` | OpenFile, OnOpenMediaFailed |
| `m_iReloadSubIdx` | `int m_iReloadSubIdx` | OpenFile, OnOpenMediaFailed |
| `m_reloadFilename` | `CString m_reloadFilename` | OpenFile, OnOpenMediaFailed |
| `m_rtReloadPos` | `REFERENCE_TIME m_rtReloadPos` | OpenMediaPrivate, OpenFile, OnOpenMediaFailed |
| `reloadABRepeat` | `ABRepeat abRepeat, reloadABRepeat` | OpenMediaPrivate, OpenFile, OnOpenMediaFailed |

### UI state

**Windows, bars and panels**

| Member | Declared as | Touched by |
|---|---|---|
| `m_controls` | `CMainFrameControls m_controls` | CloseMediaInternal, OnFilePostClosemedia |
| `m_pDedicatedFSVideoWnd` | `CFullscreenWnd* m_pDedicatedFSVideoWnd` | OnFilePostClosemedia, OpenMediaInternal, OpenSetupVideo, OnFilePostOpenmedia |
| `m_pOSDWnd` | `CWnd* m_pOSDWnd` | OpenMediaPrivate |
| `m_pVideoWnd` | `CMouseWndWithArtView* m_pVideoWnd` | OpenMediaInternal, OpenMediaPrivate, OpenCreateGraphObject, OpenSetupVideo |
| `m_wndEditListEditor` | `CEditListEditor m_wndEditListEditor` | OnFilePostClosemedia, OnFilePostOpenmedia |
| `m_wndInfoBar` | `CPlayerInfoBar m_wndInfoBar` | OnGraphNotify, OnFilePostClosemedia, OpenSetupInfoBar |
| `m_wndPlaylistBar` | `CPlayerPlaylistBar m_wndPlaylistBar` | OnGraphNotify, GraphEventComplete, OpenCurPlaylistItem, OpenMediaInternal, OpenMediaPrivate, OpenFile, OpenSetupWindowTitle, OnFilePostOpenmedia, OnOpenMediaFailed |
| `m_wndPreView` | `CPreView m_wndPreView` | CloseMediaInternal, OpenMediaPrivate, OpenCreateGraphObject, OpenSetupVideo |
| `m_wndSeekBar` | `CPlayerSeekBar m_wndSeekBar` | OnGraphNotify, OnFilePostClosemedia, OpenMediaPrivate, OnFilePostOpenmedia |
| `m_wndStatsBar` | `CPlayerInfoBar m_wndStatsBar` | OnFilePostClosemedia, OpenSetupStatsBar |
| `m_wndStatusBar` | `CPlayerStatusBar m_wndStatusBar` | OnGraphNotify, CloseMediaInternal, OnFilePostClosemedia, OpenMediaInternal, OpenSetupStatusBar |
| `m_wndToolBar` | `CPlayerToolBar m_wndToolBar` | OpenSetupAudio |
| `m_wndView` | `CChildView m_wndView` | OnFilePostClosemedia, OpenMediaInternal |

**OSD and status text**

| Member | Declared as | Touched by |
|---|---|---|
| `m_OSD` | `COSD m_OSD` | OnGraphNotify, CloseMediaPrivate, OnFilePostClosemedia, OpenMediaPrivate, OnOpenMediaFailed |
| `m_bKeepTempStatusBarVisibleOnMediaLoad` | `bool m_bKeepTempStatusBarVisibleOnMediaLoad = false` | OpenMediaInternal |
| `m_statusbarVideoFormat` | `CString m_statusbarVideoFormat` | OnFilePostClosemedia |
| `m_statusbarVideoSize` | `CString m_statusbarVideoSize` | OnGraphNotify, OnFilePostClosemedia, OpenSetupVideo |

**View geometry, zoom and fullscreen**

| Member | Declared as | Touched by |
|---|---|---|
| `m_AngleX` | `int m_AngleX, m_AngleY, m_AngleZ` | OnFilePostClosemedia, OnFilePostOpenmedia |
| `m_AngleY` | `int m_AngleX, m_AngleY, m_AngleZ` | OnFilePostClosemedia, OnFilePostOpenmedia |
| `m_AngleZ` | `int m_AngleX, m_AngleY, m_AngleZ` | OnFilePostClosemedia, OnFilePostOpenmedia |
| `m_PosX` | `double m_ZoomX, m_ZoomY, m_PosX, m_PosY` | OnFilePostClosemedia |
| `m_PosY` | `double m_ZoomX, m_ZoomY, m_PosX, m_PosY` | OnFilePostClosemedia |
| `m_ZoomX` | `double m_ZoomX, m_ZoomY, m_PosX, m_PosY` | OnFilePostClosemedia |
| `m_ZoomY` | `double m_ZoomX, m_ZoomY, m_PosX, m_PosY` | OnFilePostClosemedia |
| `m_bAllowWindowZoom` | `bool m_bAllowWindowZoom` | OnFilePostOpenmedia |
| `m_bDelaySetOutputRect` | `bool m_bDelaySetOutputRect` | OpenMediaInternal, OnFilePostOpenmedia |
| `m_bNeedZoomAfterFullscreenExit` | `bool m_bNeedZoomAfterFullscreenExit` | OpenMediaInternal, OnFilePostOpenmedia |
| `m_bOpeningInAutochangedMonitorMode` | `bool m_bOpeningInAutochangedMonitorMode` | OnFilePostOpenmedia |
| `m_fFullScreen` | `bool m_fFullScreen` | CloseMediaInternal, OpenMediaPrivate, OnFilePostOpenmedia |
| `m_fStartInD3DFullscreen` | `bool m_fStartInD3DFullscreen` | OnFilePostClosemedia, OpenMediaInternal, OnFilePostOpenmedia |
| `m_fStartInFullscreenSeparate` | `bool m_fStartInFullscreenSeparate` | OnFilePostClosemedia, OpenMediaInternal, OnFilePostOpenmedia |

**Dialogs and menus**

| Member | Declared as | Touched by |
|---|---|---|
| `lockModalDialog` | `CCritSec lockModalDialog` | CloseMediaInternal |
| `m_bSettingUpMenus` | `bool m_bSettingUpMenus` | CloseMediaInternal, OnFilePostOpenmedia |
| `m_pSubtitlesProviders` | `std::unique_ptr<SubtitlesProviders> m_pSubtitlesProviders` | CloseMediaInternal |
| `m_wndSubtitlesDownloadDialog` | `CSubtitleDlDlg m_wndSubtitlesDownloadDialog` | CloseMediaInternal |
| `mediaTypesErrorDlg` | `CMediaTypesDlg* mediaTypesErrorDlg` | CloseMediaInternal |
| `rarEntrySelectorDlg` | `RarEntrySelectorDialog* rarEntrySelectorDlg` | CloseMediaInternal |

