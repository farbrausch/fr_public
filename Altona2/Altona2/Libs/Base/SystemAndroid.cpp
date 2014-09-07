/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>


#if sConfigNoSetup
#include <jni.h>
#include <android/asset_manager_jni.h>
#endif


#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Base/SystemPrivate.hpp"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>


#include <android/asset_manager.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include <android_native_app_glue.h>
#include <android/log.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "native-activity", __VA_ARGS__))


extern "C"
{
void Android_DebugBreak()
{
    LOGE("DebugBreak");
}

char *DefaultEngine2Source = 0;
//char *SimpleEngineSource = 0;
}

namespace Altona2 {

namespace Private {
void sUpdateScreenSize(int sx,int sy);
}

using namespace Private;


static SLObjectItf engineObject;
static SLEngineItf engineEngine;
static SLObjectItf outputMixObject;
 
// buffer queue player interfaces
static SLObjectItf bqPlayerObject = NULL;
static SLPlayItf bqPlayerPlay;
static SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;
static SLMuteSoloItf bqPlayerMuteSolo;
static SLVolumeItf bqPlayerVolume;
 
#define BUFFER_SIZE 512
#define BUFFER_SIZE_IN_SAMPLES (BUFFER_SIZE / 2)
 
// Double buffering.
static short buffer[2][BUFFER_SIZE];
static int curBuffer = 0;
 
typedef void (*AndroidAudioCallback)(short *buffer, int num_samples);

static AndroidAudioCallback audioCallback;
 
static void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) 
{
    sASSERT(bq == bqPlayerBufferQueue);
    sASSERT(NULL == context);

    short *nextBuffer = buffer[curBuffer];
    int nextSize = sizeof(buffer[0]);

    SLresult result;
    result = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, nextBuffer, nextSize);

    sASSERT(SL_RESULT_SUCCESS == result);
    curBuffer ^= 1;

    audioCallback(buffer[curBuffer], BUFFER_SIZE_IN_SAMPLES);
}
 
bool OpenSLWrap_Init(AndroidAudioCallback cb) 
{
    audioCallback = cb;
    SLresult result;
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    sASSERT(SL_RESULT_SUCCESS == result);
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    sASSERT(SL_RESULT_SUCCESS == result);
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    sASSERT(SL_RESULT_SUCCESS == result);
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 0, 0, 0);
    sASSERT(SL_RESULT_SUCCESS == result);
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    sASSERT(SL_RESULT_SUCCESS == result);

    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataFormat_PCM format_pcm = {
        SL_DATAFORMAT_PCM,
        2,
        SL_SAMPLINGRATE_44_1,
        SL_PCMSAMPLEFORMAT_FIXED_16,
        SL_PCMSAMPLEFORMAT_FIXED_16,
        SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
        SL_BYTEORDER_LITTLEENDIAN
    };

    SLDataSource audioSrc = {&loc_bufq, &format_pcm};

    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&loc_outmix, NULL};

    const SLInterfaceID ids[2] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME};
    const SLboolean req[2] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
    result = (*engineEngine)->CreateAudioPlayer(engineEngine, &bqPlayerObject, &audioSrc, &audioSnk, 2, ids, req);
    sASSERT(SL_RESULT_SUCCESS == result);

    result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
    sASSERT(SL_RESULT_SUCCESS == result);
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
    sASSERT(SL_RESULT_SUCCESS == result);
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
        &bqPlayerBufferQueue);
    sASSERT(SL_RESULT_SUCCESS == result);
    result = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, NULL);
    sASSERT(SL_RESULT_SUCCESS == result);
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_VOLUME, &bqPlayerVolume);
    sASSERT(SL_RESULT_SUCCESS == result);
    result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
    sASSERT(SL_RESULT_SUCCESS == result);

    curBuffer = 0;
    audioCallback(buffer[curBuffer], BUFFER_SIZE_IN_SAMPLES);

    result = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, buffer[curBuffer], sizeof(buffer[curBuffer]));
    if (SL_RESULT_SUCCESS != result) 
    {
        return false;
    }
    curBuffer ^= 1;
    return true;
}
 
void OpenSLWrap_Shutdown() 
{
    if (bqPlayerObject != NULL) 
    {
        (*bqPlayerObject)->Destroy(bqPlayerObject);
        bqPlayerObject = NULL;
        bqPlayerPlay = NULL;
        bqPlayerBufferQueue = NULL;
        bqPlayerMuteSolo = NULL;
        bqPlayerVolume = NULL;
    }
    if (outputMixObject != NULL) 
    {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = NULL;
    }
    if (engineObject != NULL) 
    {
        (*engineObject)->Destroy(engineObject);
        engineObject = NULL;
        engineEngine = NULL;
    }
}


namespace Private
{
    sDelegate2<void,float *,int> AOHandler;        
    bool AOEnable = 0;
}


bool ExitFlag;

/****************************************************************************/
/***                                                                      ***/
/***   NotSetup                                                           ***/
/***                                                                      ***/
/****************************************************************************/


#if sConfigNoSetup

#define JNI_ALTONA2_FUNC_HEADER(x) JNICALL Java_com_farbrausch_altona2_Native_##x

extern "C" 
{
    JNIEXPORT void JNI_ALTONA2_FUNC_HEADER(sMain)(JNIEnv * env, jobject obj);
    JNIEXPORT jint JNI_ALTONA2_FUNC_HEADER(sInitSystem)(JNIEnv * env, jobject obj, jobject assetmanager);
    JNIEXPORT void JNI_ALTONA2_FUNC_HEADER(sExitSystem)(JNIEnv * env, jobject obj);    
    JNIEXPORT void JNI_ALTONA2_FUNC_HEADER(sOnInitApp)(JNIEnv * env, jobject obj);
    JNIEXPORT void JNI_ALTONA2_FUNC_HEADER(sOnStartApp)(JNIEnv * env, jobject obj);
    JNIEXPORT void JNI_ALTONA2_FUNC_HEADER(sOnPauseApp)(JNIEnv * env, jobject obj);
    JNIEXPORT void JNI_ALTONA2_FUNC_HEADER(sOnExitApp)(JNIEnv * env, jobject obj);
    JNIEXPORT void JNI_ALTONA2_FUNC_HEADER(sOnPaintApp)(JNIEnv * env, jobject obj);
    JNIEXPORT void JNI_ALTONA2_FUNC_HEADER(sOnFrameApp)(JNIEnv * env, jobject obj);
    JNIEXPORT void JNI_ALTONA2_FUNC_HEADER(sSetScreenSize)(JNIEnv * env, jobject obj,  jint sizex, jint sizey);
    JNIEXPORT void JNI_ALTONA2_FUNC_HEADER(sSetGLFrameBuffer)(JNIEnv * env, jobject obj,  jint glname);
    JNIEXPORT void JNI_ALTONA2_FUNC_HEADER(sOnTouch)(JNIEnv * env, jobject obj, jint pid, jint mode, jfloat x, jfloat y);
    JNIEXPORT void JNI_ALTONA2_FUNC_HEADER(sOnTouchFlush)(JNIEnv * env, jobject obj);
}


void sAltonaInit();
void sAltonaExit();
void sRenderGL();
void sUpdateGfxStats();

namespace Private 
{
void sSetSysFramebuffer(unsigned int name);
void sUpdateScreenSize(int sx,int sy);
extern sScreenMode CurrentMode;
}

AAssetManager* gAssetManager = 0;

static bool gFirst = true;

extern "C"
{
AAssetManager* AAssetManager_fromJava(JNIEnv* env, jobject assetManager);
JNIEnv * jniEnv = 0;
jobject jniObj = 0;
}




JNIEXPORT void JNI_ALTONA2_FUNC_HEADER(sMain)(JNIEnv * env, jobject obj)
{
    jniEnv = env;
    jniObj = obj;
    
    sSubsystem::SetRunlevel(0xff);
    Main();
}


JNIEXPORT jint JNI_ALTONA2_FUNC_HEADER(sInitSystem)(JNIEnv * env, jobject obj, jobject assetmanager)
{
    LOGI("sInitSystem - Init");

    jniEnv = env;
    jniObj = obj;

    gAssetManager = AAssetManager_fromJava(env, assetmanager);
    sAltonaInit();
    LOGI("sInitSystem - Exit");
    return 0;
}

JNIEXPORT void JNI_ALTONA2_FUNC_HEADER(sExitSystem)(JNIEnv * env, jobject obj)
{
    LOGI("sExitSystem");
    sAltonaExit();
}

JNIEXPORT void JNI_ALTONA2_FUNC_HEADER(sOnInitApp)(JNIEnv * env, jobject obj)
{
    LOGI("sOnInitApp - Init");
    if (CurrentApp) CurrentApp->OnInit();
    LOGI("sOnInitApp - Exit");
}

JNIEXPORT void JNI_ALTONA2_FUNC_HEADER(sOnStartApp)(JNIEnv * env, jobject obj)
{
}

JNIEXPORT void JNI_ALTONA2_FUNC_HEADER(sOnPauseApp)(JNIEnv * env, jobject obj)
{
}

JNIEXPORT void JNI_ALTONA2_FUNC_HEADER(sOnExitApp)(JNIEnv * env, jobject obj)
{
    if (CurrentApp) CurrentApp->OnExit();
    CurrentApp = 0;
}

JNIEXPORT void JNI_ALTONA2_FUNC_HEADER(sOnPaintApp)(JNIEnv * env, jobject obj)
{
    //LOGI("sOnPaintApp - Init");
    if (CurrentApp) sRenderGL();
    //LOGI("sOnPaintApp - Exit");
}

JNIEXPORT void JNI_ALTONA2_FUNC_HEADER(sOnFrameApp)(JNIEnv * env, jobject obj)
{
    //LOGI("sOnFrameApp - Init");
    if (CurrentApp) CurrentApp->OnFrame();
    //LOGI("sOnFrameApp - Exit");
}

JNIEXPORT void JNI_ALTONA2_FUNC_HEADER(sSetScreenSize)(JNIEnv * env, jobject obj,  jint sizex, jint sizey)
{
    LOGI("sSetScreenSize - Init %d %d",sizex,sizey);
    sUpdateScreenSize(sizex, sizey);
    LOGI("sSetScreenSize - Exit");
}

JNIEXPORT void JNI_ALTONA2_FUNC_HEADER(sSetGLFrameBuffer)(JNIEnv * env, jobject obj,  jint glname)
{
    sSetSysFramebuffer((unsigned int)glname);
}

static bool gNewTouches = false;

JNIEXPORT void JNI_ALTONA2_FUNC_HEADER(sOnTouch)(JNIEnv * env, jobject obj, jint pid, jint mode, jfloat x, jfloat y)
{
    if (gFirst)
    {
        DragData.Mode = sDM_Hover;
        DragData.Buttons = 0;
        DragData.PosX = 0;
        DragData.PosY = 0;
        DragData.StartX = 0;
        DragData.StartY = 0;
        DragData.DeltaX = 0;
        DragData.DeltaY = 0;
        DragData.TouchCount = 4;
        for (int i=0;i<10;i++)
        {            
            DragData.Touches[i].Mode = sDM_Hover;
            DragData.Touches[i].PosX = 0;
            DragData.Touches[i].PosY = 0;
            DragData.Touches[i].StartX = 0;
            DragData.Touches[i].StartY = 0;     
        }
        gFirst = false;
    }

    if (pid<10)
    {
        DragData.Touches[pid].Mode = (sDragMode)mode;
        DragData.Touches[pid].PosX = (int)x;
        DragData.Touches[pid].PosY = (int)y;
        
        if (mode==sDM_Start)
        {
            DragData.Touches[pid].StartX = DragData.Touches[pid].PosX;
            DragData.Touches[pid].StartY = DragData.Touches[pid].PosY;     
        }

        if (pid==0) //special mb
        {
            DragData.Mode =  (sDragMode)mode;
            DragData.Buttons = sMB_Left;
            DragData.PosX = DragData.Touches[pid].PosX;
            DragData.PosY = DragData.Touches[pid].PosY;
            DragData.StartX = DragData.Touches[pid].StartX;
            DragData.StartY = DragData.Touches[pid].StartY;
            DragData.DeltaX = DragData.PosX - DragData.StartX;
            DragData.DeltaY = DragData.PosY - DragData.StartY;
        }
    }
    gNewTouches = true;
}

JNIEXPORT void JNI_ALTONA2_FUNC_HEADER(sOnTouchFlush)(JNIEnv * env, jobject obj)
{
    if (gNewTouches)
    {
        DragData.Screen = sGetScreen();
        DragData.Timestamp = sGetTimeMS();
        DragData.TouchCount = 4;    
        gNewTouches = false;
        if (CurrentApp) CurrentApp->OnDrag(DragData);

     //   LOGI("%d %d",DragData.PosX,DragData.PosY);
    }

    if (DragData.Mode==sDM_Stop)
    {
        DragData.Mode = sDM_Hover;
    }

    for (int i=0;i<10;i++)
    {            
        if (DragData.Touches[i].Mode==sDM_Stop)        
                DragData.Touches[i].Mode = sDM_Hover;
    }
}



void sRunApp(sApp *app,const sScreenMode &sm)
{
  CurrentApp = app;
  CurrentMode = sm;
}


#else	//sConfigNoSetup

/****************************************************************************/
/***                                                                      ***/
/***   Helpers                                                            ***/
/***                                                                      ***/
/****************************************************************************/


/****************************************************************************/
/***                                                                      ***/
/***   Initialization and message loop and stuff                          ***/
/***                                                                      ***/
/****************************************************************************/

extern "C"
{
  extern struct android_app* gState;
  extern struct AAssetManager* gAssetManager;
}

struct saved_state 
{
    float angle;
    int32_t x;
    int32_t y;
};

/**
 * Shared state for our app.
 */
struct engine 
{
    struct android_app* app;    
    int animating;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    int32_t width;
    int32_t height;
//    struct saved_state state;
};

static void CheckEGL()
{    
    switch (eglGetError())
    {
    case EGL_NOT_INITIALIZED :
        LOGW("EGL is not initialized, or could not be initialized, for the specified EGL display connection.");
        break;
    case EGL_BAD_ACCESS :
        LOGW("EGL cannot access a requested resource (for example a context is bound in another thread).");
        break;
    case EGL_BAD_ALLOC :
        LOGW("EGL failed to allocate resources for the requested operation.");
        break;
    case EGL_BAD_ATTRIBUTE :
        LOGW("An unrecognized attribute or attribute value was passed in the attribute list.");
        break;
    case EGL_BAD_CONTEXT :
        LOGW("An EGLContext argument does not name a valid EGL rendering context.");
        break;
    case EGL_BAD_CONFIG :
        LOGW("An EGLConfig argument does not name a valid EGL frame buffer configuration.");
        break;
    case EGL_BAD_CURRENT_SURFACE :
        LOGW("The current surface of the calling thread is a window, pixel buffer or pixmap that is no longer valid.");
        break;
    case EGL_BAD_DISPLAY :
        LOGW("An EGLDisplay argument does not name a valid EGL display connection.");
        break;
    case EGL_BAD_SURFACE :
        LOGW("An EGLSurface argument does not name a valid surface (window, pixel buffer or pixmap) configured for GL rendering.");
        break;
    case EGL_BAD_MATCH :
        LOGW("Arguments are inconsistent (for example, a valid context requires buffers not supplied by a valid surface).");
        break;
    case EGL_BAD_PARAMETER :
        LOGW("One or more argument values are invalid.");
        break;
    case EGL_BAD_NATIVE_PIXMAP :
        LOGW("A NativePixmapType argument does not refer to a valid native pixmap.");
        break;
    case EGL_BAD_NATIVE_WINDOW :
        LOGW("A NativeWindowType argument does not refer to a valid native window.");
        break;
    case EGL_CONTEXT_LOST :
        LOGW("A power management event has occurred. The application must destroy all contexts and reinitialise OpenGL ES state and objects to continue rendering. ");
        break;
    }
}

static int engine_init_display(struct engine* engine) 
{
    const EGLint attribs[] = 
    {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 4,
            EGL_GREEN_SIZE, 4,
            EGL_RED_SIZE, 4,
            EGL_DEPTH_SIZE, 8,
            EGL_NONE
    };

    const EGLint attribs2[] = 
    {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
    };


    EGLint w, h, dummy, format;
    EGLint numConfigs;
    EGLConfig config;
    EGLSurface surface;
    EGLContext context;

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    CheckEGL();
    if (display==EGL_NO_DISPLAY)
    {
        LOGI("Error eglGetDisplay");
        exit(-1);
    }
    
    eglInitialize(display, 0, 0);
    CheckEGL();

    if (eglChooseConfig(display, attribs, &config, 1, &numConfigs)==EGL_FALSE)
    {
        CheckEGL();
        LOGW("Unable to eglChooseConfig");
        exit(-1);
    }
    
    LOGI("Number of Configs: %d",numConfigs);



    /* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
     * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
     * As soon as we picked a EGLConfig, we can safely reconfigure the
     * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
    if (eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format)==EGL_FALSE)
    {
        CheckEGL();
        LOGW("Unable to eglGetConfigAttrib");
        exit(-1);
    }


    ANativeWindow_setBuffersGeometry(engine->app->window, 0, 0, format);

    surface = eglCreateWindowSurface(display, config, engine->app->window, NULL);    
    CheckEGL();
    if (surface==EGL_NO_SURFACE)
    {
        LOGW("Unable to eglCreateWindowSurface");
    }
    

    context = eglGetCurrentContext();
    CheckEGL();
    if (context==EGL_NO_CONTEXT)
    {
      LOGI("eglCreateContext\n");
      context = eglCreateContext(display, config, NULL, attribs2);
      if (context==EGL_NO_CONTEXT)
      {
          CheckEGL();
          LOGW("Unable to eglCreateContext");
          exit(-1);
      }
    }

    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) 
    {
        CheckEGL();
        LOGW("Unable to eglMakeCurrent");
        return -1;
    }

    eglQuerySurface(display, surface, EGL_WIDTH, &w);
    eglQuerySurface(display, surface, EGL_HEIGHT, &h);

    engine->animating = 0;
    engine->display = display;
    engine->context = context;
    engine->surface = surface;
    engine->width = w;
    engine->height = h;

    //engine->state.angle = 0;

    // Initialize GL state.
    //glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    //glEnable(GL_CULL_FACE);
    //glShadeModel(GL_SMOOTH);
    //glDisable(GL_DEPTH_TEST);

    return 0;
}

/**
 * Just the current frame in the display.
 */
static void engine_draw_frame(struct engine* engine) 
{
    if (engine->display == NULL) 
    {
        // No display.
        return;
    }

    eglSwapBuffers(engine->display, engine->surface);
}

/**
 * Tear down the EGL context currently associated with the display.
 */
static void engine_term_display(struct engine* engine) 
{
    if (engine->display != EGL_NO_DISPLAY) 
    {
        eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (engine->context != EGL_NO_CONTEXT) {
            eglDestroyContext(engine->display, engine->context);
        }
        if (engine->surface != EGL_NO_SURFACE) {
            eglDestroySurface(engine->display, engine->surface);
        }
        eglTerminate(engine->display);
    }
    engine->animating = 0;
    engine->display = EGL_NO_DISPLAY;
    engine->context = EGL_NO_CONTEXT;
    engine->surface = EGL_NO_SURFACE;
}

/**
 * Process the next input event.
 */

static bool first = true;

static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) 
{
    if (first)
    {
        DragData.Mode = sDM_Hover;
        DragData.Buttons = 0;
        DragData.PosX = 0;
        DragData.PosY = 0;
        DragData.StartX = 0;
        DragData.StartY = 0;
        DragData.DeltaX = 0;
        DragData.DeltaY = 0;
        DragData.TouchCount = 0;
        for (int i=0;i<10;i++)
        {            
            DragData.Touches[i].Mode = sDM_Hover;
            DragData.Touches[i].PosX = 0;
            DragData.Touches[i].PosY = 0;
            DragData.Touches[i].StartX = 0;
            DragData.Touches[i].StartY = 0;     
        }
        first = false;
    }


    struct engine* engine = (struct engine*)app->userData;
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) 
    {
        int pc = AMotionEvent_getPointerCount(event);
        int time = (int)(AMotionEvent_getEventTime(event)/1000000);
        int action = AMotionEvent_getAction(event);
        int id = (action&AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)>>AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        action = action&AMOTION_EVENT_ACTION_MASK;
        int pid;
        sTouchData *e;

        pc = pc < 4 ? pc : 4;

        //LOGI("Event - time:%d , pc:%d, action:%d, id:%d",time, pc, action, id);        

        if (action==5)
        {
            pid = AMotionEvent_getPointerId(event, id);          
            e = &DragData.Touches[pid];
            e->Mode = sDM_Start;
        }
        else if (action==6)
        {
            pid = AMotionEvent_getPointerId(event, id);
            e = &DragData.Touches[pid];
            if (e!=0)
            {
                e->Mode = sDM_Stop;
            }
        }
        
        DragData.Screen = sGetScreen();
        DragData.Timestamp = time;
        DragData.TouchCount = 4;
        DragData.Buttons = 0;

        for (int i=0;i<pc;i++)
        {            
            float x = AMotionEvent_getX(event, i);
            float y = AMotionEvent_getY(event, i);
            
            pid = AMotionEvent_getPointerId(event, i);            
            e = &DragData.Touches[pid];

            if (e==0)
            {
                LOGI("Error e==0");
                return 1;
            }
                       
            if (i==0)
            {
                switch(action)
                {
                case AMOTION_EVENT_ACTION_DOWN :
                    e->Mode = sDM_Start;
                    break;
                case AMOTION_EVENT_ACTION_UP :
                    e->Mode = sDM_Stop;
                    break;
                case AMOTION_EVENT_ACTION_MOVE :
                    e->Mode = sDM_Drag;
                    break;
                case AMOTION_EVENT_ACTION_CANCEL :
                    e->Mode = sDM_Stop;
                    break;
                default :
                    break;
                }                
            }
            
            if (e->Mode==sDM_Start)
            {
                e->StartX = (int)x;
                e->StartY = (int)y;
            }

            e->PosX = (int)x;
            e->PosY = (int)y;
                     
            if (pid==0)
            {
                DragData.DeltaX = e->PosX - e->StartX;
                DragData.DeltaY = e->PosY - e->StartY;
                DragData.PosX = e->PosX;
                DragData.PosY = e->PosY;
                DragData.StartX = e->StartX;
                DragData.StartY = e->StartY;
                DragData.Mode = e->Mode;
                DragData.Buttons = sMB_Left;
            }
            //LOGI("%d %5.3f %5.3f",pid,x,y);
        }

        CurrentApp->OnDrag(DragData);


        for (int i=0;i<10;i++)
        {
            if (DragData.Touches[i].Mode==sDM_Start)
                DragData.Touches[i].Mode = sDM_Drag;

            if (DragData.Touches[i].Mode==sDM_Stop)
                DragData.Touches[i].Mode = sDM_Hover;
        }


        /*
        if (AMotionEvent_getPointerCount(event)>1)
        {
            LOGI("Pressed more then one");
        }
        */
    //    engine->animating = 1;
    //    engine->state.x = AMotionEvent_getX(event, 0);
    //    engine->state.y = AMotionEvent_getY(event, 0);
        return 1;
    }
    return 0;
}


static bool sInitApp = false;

/**
 * Process the next main command.
 */
static void engine_handle_cmd(struct android_app* app, int32_t cmd) 
{
    struct engine* engine = (struct engine*)app->userData;
    switch (cmd) 
    {
        case APP_CMD_SAVE_STATE:
            // The system has asked us to save our current state.  Do so.
            //engine->app->savedState = malloc(sizeof(struct saved_state));
            //*((struct saved_state*)engine->app->savedState) = engine->state;
            //engine->app->savedStateSize = sizeof(struct saved_state);
            break;
        case APP_CMD_INIT_WINDOW:
            if (engine->app->window != NULL) 
            {
                engine_init_display(engine);
                if (!sInitApp)
                {
                  sSubsystem::SetRunlevel(0xff);
                  CurrentApp->OnInit();
                  sInitApp = true;
                }
                sUpdateScreenSize(engine->width, engine->height);

            }
            break;
        case APP_CMD_TERM_WINDOW:            
          LOGW("APP_CMD_TERM_WINDOW");
            CurrentApp->OnExit();
            sSubsystem::SetRunlevel(0x7f);
            ExitGfx();

            delete CurrentApp;
            CurrentApp = 0;

            engine_term_display(engine);
            _exit(0);
            break;
        case APP_CMD_GAINED_FOCUS:
          engine->animating = 1;
            break;
        case APP_CMD_LOST_FOCUS:
          engine->animating = 0;
            break;
    }
}

void sRenderGL();

void sRunApp(sApp *app,const sScreenMode &sm)
{
  struct engine engine;

  memset(&engine, 0, sizeof(engine));
  gState->userData = &engine;
  gState->onAppCmd = engine_handle_cmd;
  gState->onInputEvent = engine_handle_input;
  engine.app = gState;

  CurrentApp = app;
  CurrentMode = sm;
  
  while (1) 
  {
    int ident;
    int events;
    struct android_poll_source* source;

    while ((ident=ALooper_pollAll(0, NULL, &events, (void**)&source)) >= 0) 
    {
      if (source != NULL) 
      {
        source->process(gState, source);
      }

      if (gState->destroyRequested != 0) 
      {
        engine_term_display(&engine);
        break;
      }
    }

    if (engine.animating)
    {
      glDisable(GL_SCISSOR_TEST);
      glColorMask(1,1,1,1);    
      glViewport(0,0,engine.width,engine.height);
      glClearColor(0,0,0,0);
      glClear(GL_COLOR_BUFFER_BIT);

      sRenderGL();
      //CurrentApp->OnPaint();
      engine_draw_frame(&engine);
    }
  }
}


#endif //!sConfigNoSetup

void sUpdateScreen(const sRect &r)
{
}

uint sGetKeyQualifier()
{
  return 0;//LastKey & 0xffff0000;
}

void sGetMouse(int &x,int &y,int &b)
{
}

void sSetHardwareCursor(sHardwareCursorImage img)
{  
}

void sEnableHardwareCursor(bool enable)
{

}

void sExit()
{
  ExitFlag = 1;
}

void sScreen::SetDragDropCallback(sDelegate1<void,const sDragDropInfo *> del)
{
}

void sScreen::ClearDragDropCallback()
{
}


/****************************************************************************/
/***                                                                      ***/
/***   dynamic link libraries                                             ***/
/***                                                                      ***/
/****************************************************************************/

sDynamicLibrary::sDynamicLibrary()
{
}

sDynamicLibrary::~sDynamicLibrary()
{
}

bool sDynamicLibrary::Load(const char *filename)
{
  return 0;
}

void *sDynamicLibrary::GetSymbol(const char *symbol)
{
  return 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   Ios Features                                                       ***/
/***                                                                      ***/
/****************************************************************************/

void sEnableVirtualKeyboard(bool enable)
{
}

float sGetDisplayScaleFactor()
{
  return 1.0f;
}

/****************************************************************************/
/***                                                                      ***/
/***   Audio                                                              ***/
/***                                                                      ***/
/****************************************************************************/

 void sAudioCallback(short *buffer, int num_samples)
 {  
     static float ptr[512];
     if(AOEnable)
        AOHandler.Call(ptr,num_samples);
     else
        sSetMem(ptr,0,sizeof(ptr));

     for (int i=0;i<num_samples*2;i++)
        buffer[i] = (short) (ptr[i] * 32767.0f);
 }


int sStartAudioOut(int freq,const sDelegate2<void,float *,int> &handler,int flags)
{    
    LOGI("sStartAudioOut - Init %d",freq);
    if (freq==44100)
    {
        sASSERT(AOEnable==0)
        OpenSLWrap_Init(sAudioCallback);
        LOGI("sStartAudioOut - 44100 Exit");        
        AOHandler = handler;                
        AOEnable = 1;
        return freq;
    }
    LOGI("sStartAudioOut - Exit");
    return 0;
}

void sUpdateAudioOut(int flags)
{

}

void sStopAudioOut()
{    
    AOEnable = 0;
    OpenSLWrap_Shutdown();
}

int sGetSamplesQueuedOut()
{
    return 0;
}


/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

const sSystemFontInfo *sGetSystemFont(sSystemFontId id)
{
  return 0;
}

sSystemFont *sLoadSystemFont(const sSystemFontInfo *fi,int sx,int sy,int flags)
{
  return 0;
}


int sSystemFont::GetAdvance()
{
  return 0;
}

int sSystemFont::GetBaseline()
{
  return 0;
}

int sSystemFont::GetCellHeight()
{
  return 0;
}

bool sSystemFont::GetInfo(int code,int *abc)
{
  return true;
}

sOutline *sSystemFont::GetOutline(const char *text)
{
  return 0;
}

void sSystemFont::BeginRender(int sx,int sy,uint8 *data)
{
}

void sSystemFont::RenderChar(int code,int px,int py)
{
}

void sSystemFont::EndRender()
{
}

bool sLoadSystemFontDir(sArray<const sSystemFontInfo *> &dir)
{
    return true;
}

bool sSystemFont::CodeExists(int code)
{
    return 0;
}


void sGetAppId(sString<sMaxPath> &udid)
{
    sCopyString(udid.GetBuffer(), "Android", sizeof("Android"));
}

/****************************************************************************/
/***                                                                      ***/
/***   dummies                                                            ***/
/***                                                                      ***/
/****************************************************************************/

uint64 sGetTimeHR()
{
  return sGetTimeMS();
}

uint64 sGetHRFrequency()
{
  return 1000;
}


/****************************************************************************/
/***                                                                      ***/
/*** Android Asset File Handling                                          ***/
/***                                                                      ***/
/****************************************************************************/

class sAndroidFile : public sFile
{
  sFileAccess Access;
  bool IsWrite;
  bool IsRead;

  AAsset *File;
  bool Ok;
  int64 Size;
  int64 Offset;
public:
  sAndroidFile(AAsset *file,sFileAccess access)
  {
    File = file;
    Access = access;
    Ok = 1;
    Offset = 0;
    IsWrite = Access==sFA_Write || Access==sFA_WriteAppend || Access==sFA_ReadWrite;
    IsRead = Access==sFA_Read || Access==sFA_ReadRandom || Access==sFA_ReadWrite;
    Size = AAsset_getLength(file);
//    sLogF("file","File size %d",Size);
  }

  ~sAndroidFile()
  {
    Close();
  }

  bool Close()
  {
    if(File)
    {
        AAsset_close(File);
        Ok = 0;
        File = 0;
    }
    return Ok;
  }

  bool SetSize(int64)
  {
    if(IsWrite)
    {
    }
    else
    {
      Ok = 0;
    }
    return Ok;
  }

  int64 GetSize()
  {
    return Size;
  }

  bool Read(void *data,uptr size)
  {
    if(IsRead)
    {      
      uptr read = AAsset_read (File,data,size);
  //    sLogF("file","Android asset read %d(%d)",read,size);      
      Offset+=read;
    }
    else 
    {
      Ok = 0;
    }
    return Ok;
  }

  bool Write(const void *data,uptr size)
  {
      Ok = 0;
      return Ok;  
  }

  bool SetOffset(int64 offset)
  {
    if(AAsset_seek(File,off_t(offset),SEEK_SET)==-1)
      Ok = 0;
//    sLogF("file","Android asset seek %d (%d)",offset,Ok);      
    return Ok;
  }

  int64 GetOffset()
  {
    return Offset;
  }

  uint8 *Map(int64 offset,uptr size)
  {
    return 0;
  }

  bool ReadAsync(int64 offset,uptr size,void *mem,sFileASyncHandle *hnd,sFilePriorityFlags pri)
  {
    *hnd = 0;
    return 0;
  }

  bool Write(int64 offset,uptr size,void *mem,sFileASyncHandle *hnd,sFilePriorityFlags pri)
  {
    return 0;
  }

  bool CheckAsync(sFileASyncHandle)
  {
    return 0;
  }

  bool EndAsync(sFileASyncHandle)
  {
    return 0;
  }
};

/****************************************************************************/

class sAndroidFileHandler : public sFileHandler
{
public:
  sAndroidFileHandler()
  {
  }

  ~sAndroidFileHandler()
  {
  }

  sFile *Create(const char *name,sFileAccess access)
  {
    static const char *mode[] =  {  0,"read","readrandom","write","writeappend","readwrite"  };
    sASSERT(access>=sFA_Read && access<=sFA_ReadWrite);
    
    AAsset* file = AAssetManager_open(gAssetManager, name, AASSET_MODE_UNKNOWN);
    
    if(file)
    {
      sLogF("file","open android asset <%s> for %s",name,mode[access]);
      sFile *f = new sAndroidFile(file,access);
      return f;
    }
    else 
    {
      sLogF("file","failed to open file <%s> for %s",name,mode[access]);
      return 0;
    }
  }

  bool Exists(const char *name)
  {
      AAssetManager* mgr = gAssetManager;
      AAsset* asset = AAssetManager_open(mgr, name, AASSET_MODE_UNKNOWN);
      if (asset!=0)
          AAsset_close(asset);
      return asset!=0;
  }
};

/****************************************************************************/

class sAndroidFileHandlerSubsystem : public sSubsystem
{
public:
  sAndroidFileHandlerSubsystem() : sSubsystem("Android Filehandler",0x11) {}

  void Init()
  {
    sAddFileHandler(new sAndroidFileHandler);
  }
} sAndroidFileHandlerSubsystem_;


class sFixPathSubsystem : public sSubsystem
{
public:
	sFixPathSubsystem() : sSubsystem("Fix App Path",0x12) {}
  
  	void Init()
  	{
    	sAddFileHandler(new sLinkFileHandler("/sdcard/Download/altona2"));    	
  	}
} sFixPathSubsystem_;
 

/****************************************************************************/

}

