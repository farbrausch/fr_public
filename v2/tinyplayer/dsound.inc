; generated automatically from dsound.h by h2inc. do not edit.

%define DIRECTSOUND_VERSION                             0x0800
%define _FACDS                                          0x878
%define DSFX_LOCHARDWARE                                0x00000001
%define DSFX_LOCSOFTWARE                                0x00000002
%define DSCFX_LOCHARDWARE                               0x00000001
%define DSCFX_LOCSOFTWARE                               0x00000002
%define DSCFXR_LOCHARDWARE                              0x00000010
%define DSCFXR_LOCSOFTWARE                              0x00000020
%define DSCFXR_UNALLOCATED                              0x00000040
%define DSCFXR_FAILED                                   0x00000080
%define DSCFXR_UNKNOWN                                  0x00000100

%define ReferenceClock.QueryInterface                   0x00000000
%define ReferenceClock.AddRef                           0x00000004
%define ReferenceClock.Release                          0x00000008
%define ReferenceClock.GetTime                          0x0000000c
%define ReferenceClock.Unadvise                         0x00000010

%define DirectSound8.QueryInterface                     0x00000000
%define DirectSound8.AddRef                             0x00000004
%define DirectSound8.Release                            0x00000008
%define DirectSound8.CreateSoundBuffer                  0x0000000c
%define DirectSound8.GetCaps                            0x00000010
%define DirectSound8.DuplicateSoundBuffer               0x00000014
%define DirectSound8.SetCooperativeLevel                0x00000018
%define DirectSound8.Compact                            0x0000001c
%define DirectSound8.GetSpeakerConfig                   0x00000020
%define DirectSound8.SetSpeakerConfig                   0x00000024
%define DirectSound8.Initialize                         0x00000028
%define DirectSound8.VerifyCertification                0x0000002c

%define DirectSoundBuffer8.QueryInterface               0x00000000
%define DirectSoundBuffer8.AddRef                       0x00000004
%define DirectSoundBuffer8.Release                      0x00000008
%define DirectSoundBuffer8.GetCaps                      0x0000000c
%define DirectSoundBuffer8.GetCurrentPosition           0x00000010
%define DirectSoundBuffer8.GetFormat                    0x00000014
%define DirectSoundBuffer8.GetVolume                    0x00000018
%define DirectSoundBuffer8.GetPan                       0x0000001c
%define DirectSoundBuffer8.GetFrequency                 0x00000020
%define DirectSoundBuffer8.GetStatus                    0x00000024
%define DirectSoundBuffer8.Initialize                   0x00000028
%define DirectSoundBuffer8.Lock				                  0x0000002c
%define DirectSoundBuffer8.Play                         0x00000030
%define DirectSoundBuffer8.SetCurrentPosition           0x00000034
%define DirectSoundBuffer8.SetFormat                    0x00000038
%define DirectSoundBuffer8.SetVolume                    0x0000003c
%define DirectSoundBuffer8.SetPan                       0x00000040
%define DirectSoundBuffer8.SetFrequency                 0x00000044
%define DirectSoundBuffer8.Stop                         0x00000048
%define DirectSoundBuffer8.Unlock                       0x0000004c
%define DirectSoundBuffer8.Restore                      0x00000050
%define DirectSoundBuffer8.SetFX                        0x00000054
%define DirectSoundBuffer8.AcquireResources             0x00000058
%define DirectSoundBuffer8.GetObjectInPath              0x0000005c

%define DirectSound3DListener.QueryInterface            0x00000000
%define DirectSound3DListener.AddRef                    0x00000004
%define DirectSound3DListener.Release                   0x00000008
%define DirectSound3DListener.GetAllParameters          0x0000000c
%define DirectSound3DListener.GetDistanceFactor         0x00000010
%define DirectSound3DListener.GetDopplerFactor          0x00000014
%define DirectSound3DListener.GetOrientation            0x00000018
%define DirectSound3DListener.GetPosition               0x0000001c
%define DirectSound3DListener.GetRolloffFactor          0x00000020
%define DirectSound3DListener.GetVelocity               0x00000024
%define DirectSound3DListener.SetAllParameters          0x00000028
%define DirectSound3DListener.SetDistanceFactor         0x0000002c
%define DirectSound3DListener.SetDopplerFactor          0x00000030
%define DirectSound3DListener.SetPosition               0x00000034
%define DirectSound3DListener.SetRolloffFactor          0x00000038
%define DirectSound3DListener.SetVelocity               0x0000003c
%define DirectSound3DListener.CommitDeferredSettings    0x00000040

%define DirectSound3DBuffer.QueryInterface              0x00000000
%define DirectSound3DBuffer.AddRef                      0x00000004
%define DirectSound3DBuffer.Release                     0x00000008
%define DirectSound3DBuffer.GetAllParameters            0x0000000c
%define DirectSound3DBuffer.GetConeAngles               0x00000010
%define DirectSound3DBuffer.GetConeOrientation          0x00000014
%define DirectSound3DBuffer.GetConeOutsideVolume        0x00000018
%define DirectSound3DBuffer.GetMaxDistance              0x0000001c
%define DirectSound3DBuffer.GetMinDistance              0x00000020
%define DirectSound3DBuffer.GetMode                     0x00000024
%define DirectSound3DBuffer.GetPosition                 0x00000028
%define DirectSound3DBuffer.GetVelocity                 0x0000002c
%define DirectSound3DBuffer.SetAllParameters            0x00000030
%define DirectSound3DBuffer.SetConeAngles               0x00000034
%define DirectSound3DBuffer.SetConeOrientation          0x00000038
%define DirectSound3DBuffer.SetConeOutsideVolume        0x0000003c
%define DirectSound3DBuffer.SetMaxDistance              0x00000040
%define DirectSound3DBuffer.SetMinDistance              0x00000044
%define DirectSound3DBuffer.SetMode                     0x00000048
%define DirectSound3DBuffer.SetPosition                 0x0000004c
%define DirectSound3DBuffer.SetVelocity                 0x00000050

%define DirectSoundCapture.QueryInterface               0x00000000
%define DirectSoundCapture.AddRef                       0x00000004
%define DirectSoundCapture.Release                      0x00000008
%define DirectSoundCapture.CreateCaptureBuffer          0x0000000c
%define DirectSoundCapture.GetCaps                      0x00000010
%define DirectSoundCapture.Initialize                   0x00000014

%define DirectSoundCaptureBuffer.QueryInterface         0x00000000
%define DirectSoundCaptureBuffer.AddRef                 0x00000004
%define DirectSoundCaptureBuffer.Release                0x00000008
%define DirectSoundCaptureBuffer.GetCaps                0x0000000c
%define DirectSoundCaptureBuffer.GetCurrentPosition     0x00000010
%define DirectSoundCaptureBuffer.GetFormat              0x00000014
%define DirectSoundCaptureBuffer.GetStatus              0x00000018
%define DirectSoundCaptureBuffer.Initialize             0x0000001c
%define DirectSoundCaptureBuffer.Start                  0x00000020
%define DirectSoundCaptureBuffer.Stop                   0x00000024
%define DirectSoundCaptureBuffer.Unlock                 0x00000028

%define DirectSoundCaptureBuffer8.QueryInterface        0x00000000
%define DirectSoundCaptureBuffer8.AddRef                0x00000004
%define DirectSoundCaptureBuffer8.Release               0x00000008
%define DirectSoundCaptureBuffer8.GetCaps               0x0000000c
%define DirectSoundCaptureBuffer8.GetCurrentPosition    0x00000010
%define DirectSoundCaptureBuffer8.GetFormat             0x00000014
%define DirectSoundCaptureBuffer8.GetStatus             0x00000018
%define DirectSoundCaptureBuffer8.Initialize            0x0000001c
%define DirectSoundCaptureBuffer8.Start                 0x00000020
%define DirectSoundCaptureBuffer8.Stop                  0x00000024
%define DirectSoundCaptureBuffer8.Unlock                0x00000028
%define DirectSoundCaptureBuffer8.GetObjectInPath       0x0000002c
%define DirectSoundCaptureBuffer8.GetFXStatus           0x00000030

%define DirectSoundNotify.QueryInterface                0x00000000
%define DirectSoundNotify.AddRef                        0x00000004
%define DirectSoundNotify.Release                       0x00000008
%define DirectSoundNotify.SetNotificationPositions      0x0000000c

%define KSPROPERTY_SUPPORT_GET                          0x00000001
%define KSPROPERTY_SUPPORT_SET                          0x00000002

%define KsPropertySet.QueryInterface                    0x00000000
%define KsPropertySet.AddRef                            0x00000004
%define KsPropertySet.Release                           0x00000008
%define KsPropertySet.QuerySupport                      0x0000000c

%define DSFXGARGLE_WAVE_TRIANGLE                        0
%define DSFXGARGLE_WAVE_SQUARE                          1
%define DSFXGARGLE_RATEHZ_MIN                           1
%define DSFXGARGLE_RATEHZ_MAX                           1000

%define DirectSoundFXGargle.QueryInterface              0x00000000
%define DirectSoundFXGargle.AddRef                      0x00000004
%define DirectSoundFXGargle.Release                     0x00000008
%define DirectSoundFXGargle.SetAllParameters            0x0000000c
%define DirectSoundFXGargle.GetAllParameters            0x00000010

%define DSFXCHORUS_WAVE_TRIANGLE                        0
%define DSFXCHORUS_WAVE_SIN                             1
%define DSFXCHORUS_PHASE_MIN                            0
%define DSFXCHORUS_PHASE_MAX                            4
%define DSFXCHORUS_PHASE_NEG_180                        0
%define DSFXCHORUS_PHASE_NEG_90                         1
%define DSFXCHORUS_PHASE_ZERO                           2
%define DSFXCHORUS_PHASE_90                             3
%define DSFXCHORUS_PHASE_180                            4

%define DirectSoundFXChorus.QueryInterface              0x00000000
%define DirectSoundFXChorus.AddRef                      0x00000004
%define DirectSoundFXChorus.Release                     0x00000008
%define DirectSoundFXChorus.SetAllParameters            0x0000000c
%define DirectSoundFXChorus.GetAllParameters            0x00000010

%define DSFXFLANGER_WAVE_TRIANGLE                       0
%define DSFXFLANGER_WAVE_SIN                            1
%define DSFXFLANGER_PHASE_MIN                           0
%define DSFXFLANGER_PHASE_MAX                           4
%define DSFXFLANGER_PHASE_NEG_180                       0
%define DSFXFLANGER_PHASE_NEG_90                        1
%define DSFXFLANGER_PHASE_ZERO                          2
%define DSFXFLANGER_PHASE_90                            3
%define DSFXFLANGER_PHASE_180                           4

%define DirectSoundFXFlanger.QueryInterface             0x00000000
%define DirectSoundFXFlanger.AddRef                     0x00000004
%define DirectSoundFXFlanger.Release                    0x00000008
%define DirectSoundFXFlanger.SetAllParameters           0x0000000c
%define DirectSoundFXFlanger.GetAllParameters           0x00000010

%define DSFXECHO_PANDELAY_MIN                           0
%define DSFXECHO_PANDELAY_MAX                           1

%define DirectSoundFXEcho.QueryInterface                0x00000000
%define DirectSoundFXEcho.AddRef                        0x00000004
%define DirectSoundFXEcho.Release                       0x00000008
%define DirectSoundFXEcho.SetAllParameters              0x0000000c
%define DirectSoundFXEcho.GetAllParameters              0x00000010

%define DirectSoundFXDistortion.QueryInterface          0x00000000
%define DirectSoundFXDistortion.AddRef                  0x00000004
%define DirectSoundFXDistortion.Release                 0x00000008
%define DirectSoundFXDistortion.SetAllParameters        0x0000000c
%define DirectSoundFXDistortion.GetAllParameters        0x00000010

%define DirectSoundFXCompressor.QueryInterface          0x00000000
%define DirectSoundFXCompressor.AddRef                  0x00000004
%define DirectSoundFXCompressor.Release                 0x00000008
%define DirectSoundFXCompressor.SetAllParameters        0x0000000c
%define DirectSoundFXCompressor.GetAllParameters        0x00000010

%define DirectSoundFXParamEq.QueryInterface             0x00000000
%define DirectSoundFXParamEq.AddRef                     0x00000004
%define DirectSoundFXParamEq.Release                    0x00000008
%define DirectSoundFXParamEq.SetAllParameters           0x0000000c
%define DirectSoundFXParamEq.GetAllParameters           0x00000010

%define DSFX_I3DL2REVERB_ROOM_MAX                       0
%define DSFX_I3DL2REVERB_ROOMHF_MAX                     0
%define DSFX_I3DL2REVERB_REFLECTIONS_MAX                1000
%define DSFX_I3DL2REVERB_REVERB_MAX                     2000
%define DSFX_I3DL2REVERB_REVERB_DEFAULT                 200
%define DSFX_I3DL2REVERB_QUALITY_MIN                    0
%define DSFX_I3DL2REVERB_QUALITY_MAX                    3
%define DSFX_I3DL2REVERB_QUALITY_DEFAULT                2

%define DirectSoundFXI3DL2Reverb.QueryInterface         0x00000000
%define DirectSoundFXI3DL2Reverb.AddRef                 0x00000004
%define DirectSoundFXI3DL2Reverb.Release                0x00000008
%define DirectSoundFXI3DL2Reverb.SetAllParameters       0x0000000c
%define DirectSoundFXI3DL2Reverb.GetAllParameters       0x00000010
%define DirectSoundFXI3DL2Reverb.SetPreset              0x00000014
%define DirectSoundFXI3DL2Reverb.GetPreset              0x00000018
%define DirectSoundFXI3DL2Reverb.SetQuality             0x0000001c
%define DirectSoundFXI3DL2Reverb.GetQuality             0x00000020

%define DirectSoundFXWavesReverb.QueryInterface         0x00000000
%define DirectSoundFXWavesReverb.AddRef                 0x00000004
%define DirectSoundFXWavesReverb.Release                0x00000008
%define DirectSoundFXWavesReverb.SetAllParameters       0x0000000c
%define DirectSoundFXWavesReverb.GetAllParameters       0x00000010

%define DirectSoundCaptureFXAec.QueryInterface          0x00000000
%define DirectSoundCaptureFXAec.AddRef                  0x00000004
%define DirectSoundCaptureFXAec.Release                 0x00000008
%define DirectSoundCaptureFXAec.SetAllParameters        0x0000000c
%define DirectSoundCaptureFXAec.GetAllParameters        0x00000010

%define DirectSoundCaptureFXNoiseSuppress.QueryInterface 0x00000000
%define DirectSoundCaptureFXNoiseSuppress.AddRef        0x00000004
%define DirectSoundCaptureFXNoiseSuppress.Release       0x00000008
%define DirectSoundCaptureFXNoiseSuppress.SetAllParameters 0x0000000c
%define DirectSoundCaptureFXNoiseSuppress.GetAllParameters 0x00000010

%define DirectSoundFullDuplex.QueryInterface            0x00000000
%define DirectSoundFullDuplex.AddRef                    0x00000004
%define DirectSoundFullDuplex.Release                   0x00000008
%define DirectSoundFullDuplex.Initialize                0x0000000c

%define DSCAPS_PRIMARYMONO                              0x00000001
%define DSCAPS_PRIMARYSTEREO                            0x00000002
%define DSCAPS_PRIMARY8BIT                              0x00000004
%define DSCAPS_PRIMARY16BIT                             0x00000008
%define DSCAPS_CONTINUOUSRATE                           0x00000010
%define DSCAPS_EMULDRIVER                               0x00000020
%define DSCAPS_CERTIFIED                                0x00000040
%define DSCAPS_SECONDARYMONO                            0x00000100
%define DSCAPS_SECONDARYSTEREO                          0x00000200
%define DSCAPS_SECONDARY8BIT                            0x00000400
%define DSCAPS_SECONDARY16BIT                           0x00000800

%define DSSCL_NORMAL                                    0x00000001
%define DSSCL_PRIORITY                                  0x00000002
%define DSSCL_EXCLUSIVE                                 0x00000003
%define DSSCL_WRITEPRIMARY                              0x00000004

%define DSSPEAKER_HEADPHONE                             0x00000001
%define DSSPEAKER_MONO                                  0x00000002
%define DSSPEAKER_QUAD                                  0x00000003
%define DSSPEAKER_STEREO                                0x00000004
%define DSSPEAKER_SURROUND                              0x00000005
%define DSSPEAKER_5POINT1                               0x00000006
%define DSSPEAKER_GEOMETRY_MIN                          0x00000005
%define DSSPEAKER_GEOMETRY_NARROW                       0x0000000A
%define DSSPEAKER_GEOMETRY_WIDE                         0x00000014
%define DSSPEAKER_GEOMETRY_MAX                          0x000000B4

%define DSBCAPS_PRIMARYBUFFER                           0x00000001
%define DSBCAPS_STATIC                                  0x00000002
%define DSBCAPS_LOCHARDWARE                             0x00000004
%define DSBCAPS_LOCSOFTWARE                             0x00000008
%define DSBCAPS_CTRL3D                                  0x00000010
%define DSBCAPS_CTRLFREQUENCY                           0x00000020
%define DSBCAPS_CTRLPAN                                 0x00000040
%define DSBCAPS_CTRLVOLUME                              0x00000080
%define DSBCAPS_CTRLPOSITIONNOTIFY                      0x00000100
%define DSBCAPS_CTRLFX                                  0x00000200
%define DSBCAPS_STICKYFOCUS                             0x00004000
%define DSBCAPS_GLOBALFOCUS                             0x00008000
%define DSBCAPS_GETCURRENTPOSITION2                     0x00010000
%define DSBCAPS_MUTE3DATMAXDISTANCE                     0x00020000
%define DSBCAPS_LOCDEFER                                0x00040000

%define DSBPLAY_LOOPING                                 0x00000001
%define DSBPLAY_LOCHARDWARE                             0x00000002
%define DSBPLAY_LOCSOFTWARE                             0x00000004
%define DSBPLAY_TERMINATEBY_TIME                        0x00000008
%define DSBPLAY_TERMINATEBY_DISTANCE                    0x000000010
%define DSBPLAY_TERMINATEBY_PRIORITY                    0x000000020

%define DSBSTATUS_PLAYING                               0x00000001
%define DSBSTATUS_BUFFERLOST                            0x00000002
%define DSBSTATUS_LOOPING                               0x00000004
%define DSBSTATUS_LOCHARDWARE                           0x00000008
%define DSBSTATUS_LOCSOFTWARE                           0x00000010
%define DSBSTATUS_TERMINATED                            0x00000020

%define DSBLOCK_FROMWRITECURSOR                         0x00000001
%define DSBLOCK_ENTIREBUFFER                            0x00000002

%define DSBFREQUENCY_MIN                                100
%define DSBFREQUENCY_MAX                                100000
%define DSBFREQUENCY_ORIGINAL                           0

%define DSBPAN_CENTER                                   0
%define DSBPAN_RIGHT                                    10000

%define DSBVOLUME_MAX                                   0

%define DSBSIZE_MIN                                     4
%define DSBSIZE_MAX                                     0x0FFFFFFF
%define DSBSIZE_FX_MIN                                  150

%define DS3DMODE_NORMAL                                 0x00000000
%define DS3DMODE_HEADRELATIVE                           0x00000001
%define DS3DMODE_DISABLE                                0x00000002

%define DS3D_IMMEDIATE                                  0x00000000
%define DS3D_DEFERRED                                   0x00000001
%define DS3D_MINCONEANGLE                               0
%define DS3D_MAXCONEANGLE                               360
%define DS3D_DEFAULTCONEANGLE                           360

%define DSCBCAPS_WAVEMAPPED                             0x80000000
%define DSCBCAPS_CTRLFX                                 0x00000200

%define DSCBLOCK_ENTIREBUFFER                           0x00000001

%define DSCBSTATUS_CAPTURING                            0x00000001
%define DSCBSTATUS_LOOPING                              0x00000002

%define DSCBSTART_LOOPING                               0x00000001

%define DSBPN_OFFSETSTOP                                0xFFFFFFFF

%define DS_CERTIFIED                                    0x00000000
%define DS_UNCERTIFIED                                  0x00000001
%define DS_SYSTEM_RESOURCES_NO_HOST_RESOURCES           0x00000000
%define DS_SYSTEM_RESOURCES_ALL_HOST_RESOURCES          0x7FFFFFFF
%define DS_SYSTEM_RESOURCES_UNDEFINED                   0x80000000
