/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#if sConfigShell
#include <time.h>
#else
#import <Cocoa/Cocoa.h>
#include <QuartzCore/CABase.h>
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>
#endif

#if !sConfigShell && !sConfigNoSetup
#import <OpenGL/OpenGL.h>
#import <QuartzCore/CVDisplayLink.h>
#import <Carbon/Carbon.h>
#endif

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Base/SystemPrivate.hpp"
#include "Altona2/Libs/Base/SystemOsx.hpp"

namespace Altona2 {

namespace Setup {

extern uint sKeyQualifier;
extern int sMousePosX;
extern int sMousePosY;
extern int sMouseButton;
}
    
using namespace Private;
   
 
/****************************************************************************/
 
#if sConfigShell

int sStartAudioOut(int freq,const sDelegate2<void,float *,int> &handler,int flags)
{ 
    return freq;
}

void sUpdateAudioOut(int flags)
{
}

void sStopAudioOut()
{
}

int sGetSamplesQueuedOut()
{
    return 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   Time and Date                                                      ***/
/***                                                                      ***/
/****************************************************************************/
   
static clock_t GetTimeBase = -1;
uint sGetTimeMS()
{
	if (GetTimeBase==-1)
	    GetTimeBase = clock();
	return uint((clock()-GetTimeBase)/(CLOCKS_PER_SEC)/1000);
}
#else
static double GetTimeBase = -1;
uint sGetTimeMS()
{
	if(GetTimeBase==-1)
    	GetTimeBase = CACurrentMediaTime();
	return uint((CACurrentMediaTime()-GetTimeBase)*1000.0f);
}
#endif
    
uint64 sGetTimeUS()
{
    return sGetTimeMS()*1000;
}
    
uint64 sGetTimeHR()
{
    return sGetTimeMS();
}

uint64 sGetHRFrequency()
{
    return 1000;
}

double sGetTimeSince2001()
{
	return [NSDate timeIntervalSinceReferenceDate];
}

void sGetTimeAndDate(sTimeAndDate &date)
{
  sFatal("not implemented");
}
  
bool sExecuteOpen(const char *file)
{
  sString<sFormatBuffer> loadmsg;
  
  const char *s = file;
  if(sCmpStringILen(s,"http://",7)==0) s+=7;
  //  if(sCmpStringILen(s,"www.",4)==0) s+=4;
  //  if(sCmpStringILen(s,"en.",3)==0) s+=3;
  const char *t = s;
  while(*s!=0 && *s!='/')
    s++;
  
  
  loadmsg.Add("<html><body><center><h2><br><br><br><br>Redirecting to<br>");
  loadmsg.Add(t,s-t);
  loadmsg.Add("</h2><br><h3>");
  
  while(*s!=0 && *s!='/')
    s++;
  if(*s=='/')
    s++;
  if(sCmpStringILen(s,"wiki/",5)==0) s+=5;
  t = s;
  while(*s!=0 && *s!='/' && *s!='#')
    s++;
  
  loadmsg.Add(t,s-t);
  loadmsg.Add("</h3></center></body></html>");
  
  sLogF("sys","load url %q",file);
  sLogF("sys","message %q",loadmsg);
  //sStartBrowser(file,loadmsg);
  
  NSString *nsstr = [NSString stringWithUTF8String:file];
  
  [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:nsstr]];
  [nsstr release];
  return 1;
}
  
const char *sGetUserName()
{
  return "dummy";
}

float sGetDisplayScaleFactor()
{
  return 1.0f;
}
    
/****************************************************************************/
/***                                                                      ***/
/***   Input                                                              ***/
/***                                                                      ***/
/****************************************************************************/

uint sGetKeyQualifier()
{
//    sFatal("sGetKeyQualifier()");
#if !sConfigShell
	return Altona2::Setup::sKeyQualifier;
#else
	return 0;
#endif
}

} //namespace Altona2


/****************************************************************************/
/***                                                                      ***/
/***   System depending code                                              ***/
/***                                                                      ***/
/****************************************************************************/

#if !sConfigShell

namespace Altona2 {
    
using namespace Private;

/****************************************************************************/
/***                                                                      ***/
/***   General                                                            ***/
/***                                                                      ***/
/****************************************************************************/
    
namespace Private
{
    sThreadLock *AudioLock;
    sDelegate2<void,float *,int> AOHandler;
        
    AudioComponentInstance AOUnit;
    bool AOEnable = 0;
    int AOFlags;
    bool ExitFlag = false;
}
    
/****************************************************************************/
/***                                                                      ***/
/***   Audio                                                              ***/
/***                                                                      ***/
/****************************************************************************/

// Room for improvements
// * do not always render to all available space to reduce latency
// * use the WASAPI event system

OSStatus AOCallback
(
 void *ref,
 AudioUnitRenderActionFlags *flags,
 const AudioTimeStamp *timestamp,
 UInt32 bus,
 UInt32 count,
 AudioBufferList *data
 )
{
    sASSERT(data->mNumberBuffers==1);
    sASSERT(data->mBuffers[0].mNumberChannels==2);
    sASSERT(data->mBuffers[0].mDataByteSize>=count*sizeof(float)*2);
    float *ptr = (float *)data->mBuffers[0].mData;
    
    AudioLock->Lock();
    if(AOEnable)
        AOHandler.Call(ptr,count);
    AudioLock->Unlock();
    
    return noErr;
}

int sStartAudioOut(int freq,const sDelegate2<void,float *,int> &handler,int flags)
{
    sASSERT(AOEnable==0)
    AOEnable = 1;
    OSErr err;
    
    AudioLock = new sThreadLock();
    AOHandler = handler;
    
    //AudioSessionInitialize(0,0,0,0);
    AOFlags = flags^1;
    sUpdateAudioOut(flags);
    
    AudioComponentDescription defaultOutputDescription;
    defaultOutputDescription.componentType = kAudioUnitType_Output;
    defaultOutputDescription.componentSubType = kAudioUnitSubType_DefaultOutput;
    defaultOutputDescription.componentManufacturer = kAudioUnitManufacturer_Apple;
    defaultOutputDescription.componentFlags = 0;
    defaultOutputDescription.componentFlagsMask = 0;
    
    AudioComponent defaultOutput = AudioComponentFindNext(NULL, &defaultOutputDescription);
    sASSERT(defaultOutput)
    
    AudioComponentInstanceNew(defaultOutput, &AOUnit);
    sASSERT(AOUnit);
    
    AURenderCallbackStruct input;
    input.inputProc = AOCallback;
    input.inputProcRefCon = 0;
    err = AudioUnitSetProperty(AOUnit,
                               kAudioUnitProperty_SetRenderCallback,
                               kAudioUnitScope_Input,
                               0,
                               &input,
                               sizeof(input));
    sASSERT(err==noErr);
    
    AudioStreamBasicDescription streamFormat;
    streamFormat.mSampleRate = freq;
    streamFormat.mFormatID = kAudioFormatLinearPCM;
    streamFormat.mFormatFlags = kAudioFormatFlagIsFloat;
    streamFormat.mFramesPerPacket = 1;
    streamFormat.mBytesPerPacket = 4*2;
    streamFormat.mBytesPerFrame = 4*2;
    streamFormat.mChannelsPerFrame = 2;
    streamFormat.mBitsPerChannel = 32;
    err = AudioUnitSetProperty (AOUnit,
                                kAudioUnitProperty_StreamFormat,
                                kAudioUnitScope_Input,
                                0,
                                &streamFormat,
                                sizeof(AudioStreamBasicDescription));
    sASSERT(err == noErr);
    
    // Initialize unit
    err = AudioUnitInitialize(AOUnit);
    if (err) { sDPrintF ("AudioUnitInitialize=%ld\n", err); }
    
    
    err = AudioOutputUnitStart(AOUnit);
    sASSERT(err == noErr);
    return freq;
}

void sUpdateAudioOut(int flags)
{
}

void sStopAudioOut()
{
    sASSERT(AOEnable==1)
    AudioLock->Lock();
    AOEnable = 0;
    AudioOutputUnitStop(AOUnit);
    AudioUnitUninitialize(AOUnit);
    AudioComponentInstanceDispose(AOUnit);
    //AudioSessionSetActive(0);
    AudioLock->Unlock();
    sDelete(AudioLock);
    AOUnit = 0;
}

int sGetSamplesQueuedOut()
{
    return 0;
}


} //namespace Altona2

#if !sConfigNoSetup

@interface Altona2GLView : NSView
{
	NSOpenGLContext 	*OpenGLContext;
	NSOpenGLPixelFormat *PixelFormat;
}
- (id) initWithFrame:(NSRect)frameRect;
@end

@implementation Altona2GLView

- (id) initWithFrame:(NSRect)frameRect
{
    NSOpenGLPixelFormatAttribute attribs[] = {kCGLPFAAccelerated, kCGLPFANoRecovery, kCGLPFADoubleBuffer, kCGLPFAColorSize, 24, kCGLPFADepthSize, 24, 0};
    NSOpenGLPixelFormatAttribute attribs2[] = {kCGLPFAAccelerated, kCGLPFANoRecovery, kCGLPFADoubleBuffer, kCGLPFAColorSize, 24, kCGLPFADepthSize, 16, 0};
    
    PixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];
	
    if (!PixelFormat)
        PixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs2];
    
    OpenGLContext = [[NSOpenGLContext alloc] initWithFormat:PixelFormat shareContext:nil];
    
    if (self = [super initWithFrame:frameRect])
    {
    	//[OpenGLContext makeCurrentContext];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(reshape) name:NSViewGlobalFrameDidChangeNotification object:self];
    }
	return self;
}

- (void) lockFocus
{
	[super lockFocus];
	if ([OpenGLContext view] != self)
		[OpenGLContext setView:self];
}

- (void) reshape
{
	[OpenGLContext update];
    NSRect rec = [self bounds];
    Altona2::Setup::sSetScreenSize(rec.size.width, rec.size.height);
    [self drawView];
}

- (void) drawView
{
	if (!Altona2::Private::ExitFlag)
   	{
    	Altona2::Setup::sOnFrameApp();
    
	    //if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE)
    		Altona2::Setup::sOnPaintApp();
    
		[OpenGLContext flushBuffer];
    }
}

- (void) SetOpenGLSwapVSync:(int )on
{
        GLint swapInt = on;
        [OpenGLContext setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];
}

- (void) LockOpenGL
{
	CGLLockContext((CGLContextObj)[OpenGLContext CGLContextObj]);
	[OpenGLContext makeCurrentContext];
}

- (void) UnlockOpenGL
{
	CGLUnlockContext((CGLContextObj)[OpenGLContext CGLContextObj]);
}

- (BOOL) acceptsFirstResponder
{
	return YES;
}

- (void) keyDown:(NSEvent *)event
{

}

- (void) keyUp:(NSEvent *)event
{

}

- (void) dealloc
{
	[OpenGLContext release];
	[PixelFormat release];
	[[NSNotificationCenter defaultCenter] removeObserver:self name:NSViewGlobalFrameDidChangeNotification object:self];
	[super dealloc];
}

@end

@interface Altona2AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate>
{
	@public NSWindow * Window;
    @public Altona2GLView *GlView;
}
@end

@implementation Altona2AppDelegate : NSObject
- (id)init
{
	if (self = [super init])
	{
        NSRect viewRect = NSMakeRect(0, 0, 800, 600);
		GlView = [[Altona2GLView alloc] initWithFrame:viewRect];
        Window = [[[NSWindow alloc] initWithContentRect:viewRect styleMask:NSTitledWindowMask|NSResizableWindowMask backing:NSBackingStoreBuffered defer:NO] autorelease];
        [Window setContentView:GlView];
        [Window setDelegate:self];
        [Window setCollectionBehavior: NSWindowCollectionBehaviorFullScreenPrimary];
        [Window setAcceptsMouseMovedEvents:YES];
        [Window makeKeyAndOrderFront: NSApp];
	}
	return self;
}

- (void)windowWillClose:(NSNotification *)notification
{
	Altona2::Private::ExitFlag = true;
}

- (void)dealloc
{
    [GlView release];
	[Window release];
	[super dealloc];
}

- (void)test:(id)sender
{
	Altona2::Private::ExitFlag = true;
}

@end

static Altona2AppDelegate *appDelegate;
static NSApplication* app;

static void setApplicationMenu(void)
{
    NSMenu *appleMenu;
    NSMenuItem *menuItem;
    NSString *appName;
    
    appName = @"Yes";
    appleMenu = [[NSMenu alloc] initWithTitle:@""];
    
    [appleMenu addItem:[NSMenuItem separatorItem]];
    
    menuItem = [[NSMenuItem alloc] initWithTitle:@"Quit" action:@selector(test:) keyEquivalent:@"q"];
    [menuItem setTarget:appDelegate];
    [appleMenu addItem:menuItem];
        
    menuItem = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
    [menuItem setSubmenu:appleMenu];
    [[NSApp mainMenu] addItem:menuItem];

    [appleMenu release];
    [menuItem release];
}


extern "C"
{
extern const char *sCmdLine;
};

int main(int argc, char * argv[])
{
    Altona2::sTextLog log;
    for(int i=0;i<argc;i++)
    {
        if(i>0)
            log.Print(" ");
        log.Print(argv[i]);
    }
    sCmdLine = log.Get();

	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
	app = [NSApplication sharedApplication];
	appDelegate = [[[Altona2AppDelegate alloc] init] autorelease];
	[app setDelegate:appDelegate];    
	[app activateIgnoringOtherApps:YES];
    [app setMainMenu:[[NSMenu alloc] init]];
    setApplicationMenu();
    [app finishLaunching];
    [appDelegate->GlView LockOpenGL];
    Altona2::Setup::sInitSystem();
    Altona2::Main();
    Altona2::Setup::sExitSystem();
    [appDelegate->GlView UnlockOpenGL];
	[pool drain];
	return EXIT_SUCCESS;
}

#endif //!sConfigNoSetup

namespace Altona2 {
void sAltonaInit();
void sAltonaExit();
void sRenderGL();
void sUpdateGfxStats();
namespace Private {
void sSetSysFramebuffer(unsigned int name);
void sUpdateScreenSize(int sx,int sy);
extern sScreenMode CurrentMode;
}
}

namespace Altona2 {

class sFixPathSubsystem : public sSubsystem
{
public:
	sFixPathSubsystem() : sSubsystem("Fix App Path",0x12) {}
  
	char sBundlePath[2048];
  	char sHomePath[2048];

	void sGetAppPath()
	{
  		const char *str = [[[NSBundle mainBundle] resourcePath] UTF8String];
  		int len = (int)strlen(str);
  		if(len>2047) len = 2047;
  		memcpy(sBundlePath,str,len+1);
  		sBundlePath[2047] = 0;
    
  		str = [[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,NSUserDomainMask,YES) lastObject] UTF8String];
  		len = (int)strlen(str);
  		if(len>2047) len = 2047;
  		memcpy(sHomePath,str,len+1);
  		sHomePath[2047] = 0;
        
        NSLog (@"Bundle: %@\n",[[NSBundle mainBundle] bundleIdentifier] );
        
	}
  	void Init()
  	{
    	sGetAppPath();
    	sLogF("sys","bundle path <%s>",(char *)sBundlePath);
    	sLogF("sys","home path <%s>",(char *)sHomePath);
    	sAddFileHandler(new sLinkFileHandler(sBundlePath));
    	sChangeDir(sHomePath);
  	}
} sFixPathSubsystem_;
 

void sRunApp(sApp *currentapp,const sScreenMode &sm)
{
	CurrentMode = sm;
    NSString *str = [NSString stringWithUTF8String:sm.WindowTitle];
	[appDelegate->Window setTitle:str];
    NSSize size;
    size.width = sm.SizeX;
    size.height = sm.SizeY;
    if (sm.Flags&sSM_FullWindowed)
    {
        NSRect  rec;
        rec = [[NSScreen mainScreen] frame];
        size = rec.size;
    }
    [appDelegate->Window setContentSize:size];
    [appDelegate->Window center];
	
    if (sm.Flags&sSM_Fullscreen)
    {
    	[appDelegate->Window toggleFullScreen:nil];
    }
	[appDelegate->Window makeKeyAndOrderFront:appDelegate];
	
    if (sm.Flags&sSM_NoVSync)
		[appDelegate->GlView SetOpenGLSwapVSync:0];
	else
		[appDelegate->GlView SetOpenGLSwapVSync:1];
    
        
    CurrentApp = currentapp;
    Altona2::Private::ExitFlag = false;
    CurrentApp->OnInit();
	while (!Altona2::Private::ExitFlag)
    {
    	NSEvent * event;
   		do
    	{
	    	event = [app nextEventMatchingMask:NSAnyEventMask untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES];
            
            Altona2::Setup::sHandleEvent(event);
            if (event != nil)
	            [app sendEvent: event];
    	}
    	while(event != nil);        
        [appDelegate->GlView drawView];
    }
    delete CurrentApp;
    CurrentApp = 0;
}

void sExit()
{
	ExitFlag = true;
}

/****************************************************************************/
/***                                                                      ***/
/***   Hardware Mouse Cursor                                              ***/
/***                                                                      ***/
/****************************************************************************/

// mouse cursor on ipad ? :-)

void sSetHardwareCursor(sHardwareCursorImage img)
{
}

void sEnableHardwareCursor(bool enable)
{
}

void sGetMouse(int &x,int &y,int &b)
{
  x = Altona2::Setup::sMousePosX;
  y = Altona2::Setup::sMousePosY;
  b = Altona2::Setup::sMouseButton;
}

/****************************************************************************/
  
void sScreen::SetDragDropCallback(sDelegate1<void,const sDragDropInfo *> del)
{
}

void sScreen::ClearDragDropCallback()
{
}

void sEnableVirtualKeyboard(bool enable)
{
}

} //Altona2

namespace Altona2 {
namespace Setup {

using namespace Altona2;
using namespace Altona2::Private;


uint sKeyQualifier = 0;

static void HandlingFlags(NSEvent *event)
{
    unsigned long mf = [event modifierFlags];
    sKeyQualifier = 0;
    if (mf&NSShiftKeyMask)
        sKeyQualifier |= sKEYQ_Shift;
    if (mf&NSAlternateKeyMask)
        sKeyQualifier |= sKEYQ_Alt;
    if (mf&NSCommandKeyMask)
        sKeyQualifier |= sKEYQ_Ctrl;
}

int sMousePosX;
int sMousePosY;
int sMouseButton;

static void HandleEvent(NSEvent *event)
{
	static int sx;
    static int sy;
    
    int mb = 0;
    sDragMode dm = sDM_Hover;
    sKeyData kd;
    int mask = 0;
    bool isKey = 0;
    bool isMouse = 0;
    bool isReady = 0;
    
    NSPoint pnt = [event locationInWindow];
    int x,y;
    sGetScreen()->GetSize(x, y);
    pnt.y = y - pnt.y;
    
    
    switch ([event type])
    {
    	case NSKeyDown :
            isKey = 1;
        break;
		case NSKeyUp :
            mask |= sKEYQ_Break;
        	isKey = 1;
        break;
        case NSFlagsChanged :
	        HandlingFlags(event);
        break;
        case NSLeftMouseDown :
        	mask |= sKEY_LMB;
            dm = sDM_Start;
            mb = sMB_Left;
        	isMouse = 1;
            isReady = 1;
	        sx = pnt.x;
            sy = pnt.y;
        break;
        case NSLeftMouseUp :
        	mask |= sKEY_LMB|sKEYQ_Break;
            mb = sMB_Left;
            dm = sDM_Stop;
        	isMouse = 1;
            isReady = 1;
	        sx = pnt.x;
            sy = pnt.y;
        break;
        case NSLeftMouseDragged :
            dm = sDM_Drag;
            mb = sMB_Left;
        	isMouse = 1;
        break;
        case NSRightMouseDown :
        	mask |= sKEY_RMB;
            dm = sDM_Start;
            mb = sMB_Right;
        	isMouse = 1;
            isReady = 1;
	        sx = pnt.x;
            sy = pnt.y;
        break;
        case NSRightMouseUp :
        	mask |= sKEY_RMB|sKEYQ_Break;
            mb = sMB_Right;
            dm = sDM_Stop;
        	isMouse = 1;
            isReady = 1;
        break;
        case NSRightMouseDragged :
            dm = sDM_Drag;
            mb = sMB_Right;
        	isMouse = 1;
        break;
        case NSOtherMouseDown :
        	mask |= sKEY_MMB;
            dm = sDM_Start;
            mb = sMB_Middle;
        	isMouse = 1;
            isReady = 1;
	        sx = pnt.x;
            sy = pnt.y;
        break;
        case NSOtherMouseUp :
        	mask |= sKEY_MMB|sKEYQ_Break;
            mb = sMB_Middle;
            dm = sDM_Stop;
        	isMouse = 1;
            isReady = 1;
        break;
        case NSOtherMouseDragged :
            dm = sDM_Drag;
            mb = sMB_Middle;
        	isMouse = 1;
        break;
        case NSMouseMoved :
            dm = sDM_Hover;
        	isMouse = 1;
        break;
        case NSScrollWheel :
        {
        	float d = [event deltaY];
            
            if (sAbs(d)>3)
            {
                if (d>0.0f)
                    mask |= sKEY_WheelUp;
                else
                    mask |= sKEY_WheelDown;
                
                kd.Key = mask | sKeyQualifier;
                kd.MouseX = 0;
                kd.MouseY = 0;
                isReady = 1;
            }
        }
        break;
    }
    
    if (isMouse)
    {
		sDragData dd;

        if ([event clickCount]>1)
        {
        	mb |= sMB_DoubleClick;
            mask |= sKEYQ_Double;
        }
        
        dd.Screen = sGetScreen();
        dd.Buttons = mb;
        dd.Mode = dm;
        dd.DeltaX = pnt.x-sx;
        dd.DeltaY = pnt.y-sy;
        dd.PosX = pnt.x;
        dd.PosY = pnt.y;
        dd.StartX = sx;
        dd.StartY = sy;
        dd.Timestamp = sGetTimeMS();
        dd.TouchCount = 0;
        kd.MouseX = pnt.x;
        kd.MouseY = pnt.y;
        kd.Key = mask | sKeyQualifier;
        
        sMousePosX = pnt.x;
        sMousePosY = pnt.y;
        sMouseButton = mb;

        CurrentApp->OnDrag(dd);
        
        if (dm==sDM_Start)	//Hack! need to check with Dierk
        {
	        dd.Mode = sDM_Drag;
	        dd.DeltaX = 0;
    	    dd.DeltaY = 0;
	        CurrentApp->OnDrag(dd);
        }
        
    }
    
    if (isKey)
    {
        unichar c = 0;
        uint kc = [event keyCode];
        
        uint kq = sKeyQualifier;
        
        switch (kc)
        {
            case kVK_F1 : c = sKEY_F1; break;
            case kVK_F2 : c = sKEY_F2; break;
            case kVK_F3 : c = sKEY_F3; break;
            case kVK_F4 : c = sKEY_F4; break;
            case kVK_F5 : c = sKEY_F5; break;
            case kVK_F6 : c = sKEY_F6; break;
            case kVK_F7 : c = sKEY_F7; break;
            case kVK_F8 : c = sKEY_F8; break;
            case kVK_F9 : c = sKEY_F9; break;
            case kVK_F10 : c = sKEY_F10; break;
            case kVK_F11 : c = sKEY_F11; break;
            case kVK_F12 : c = sKEY_F12; break;
            case kVK_Delete : c = sKEY_Backspace; break;
            case kVK_Tab : c = sKEY_Tab; break;
            case kVK_Return : c = sKEY_Enter; break;
            case kVK_Escape : c = sKEY_Escape; break;
            case kVK_Space : c = sKEY_Space; break;
            case kVK_UpArrow : c = sKEY_Up; break;
            case kVK_DownArrow : c = sKEY_Down; break;
            case kVK_LeftArrow : c = sKEY_Left; break;
            case kVK_RightArrow : c = sKEY_Right; break;
            case kVK_PageUp : c = sKEY_PageUp; break;
            case kVK_PageDown : c = sKEY_PageDown; break;
            case kVK_Home : c = sKEY_Home; break;
            case kVK_End: c = sKEY_End; break;
            case kVK_ForwardDelete : c = sKEY_Delete; break;
            case kVK_Help : c = sKEY_Insert; break;
            default :
                c = [[event characters] characterAtIndex:0] & 0x7fff;
                kq = 0;
                break;
        }
        if ([event isARepeat]) mask |= sKEYQ_Repeat;
        
        kd.Key = c | mask | kq;
        kd.MouseX = sMousePosX;
        kd.MouseY = sMousePosY;
        isReady = 1;
    }
    
	if (isReady)
    {
    	kd.Timestamp = sGetTimeMS();
		CurrentApp->OnKey(kd);
    }
}

bool sInitSystem()
{
	Altona2::sAltonaInit();
    sSubsystem::SetRunlevel(0xff);
	return true;
}

void sExitSystem()
{
	Altona2::Private::ExitFlag = true;
	Altona2::sAltonaExit();
}

sApp *sGetApp()
{
	return CurrentApp;
}

void sSetApp(sApp *app)
{
	CurrentApp = app;
}

void sStartApp()
{
}

void sPauseApp()
{
}

void sHandleEvent(void *event)
{
	if (event!=0 && !Altona2::Private::ExitFlag && CurrentApp!=0)
    {
        HandleEvent((NSEvent *)event);
	}
}

void sSetScreenSize(int sizex, int sizey)
{
    sUpdateScreenSize(sizex, sizey);    
}

void sSetGLFrameBuffer(unsigned int glname)
{
	sSetSysFramebuffer(glname);
}

void sOnPaintApp()
{
	if (!Altona2::Private::ExitFlag && CurrentApp)
    {
		sRenderGL();
    }
}

void sOnFrameApp()
{
}

} //namespace Setup
} //namespace Altona2


#endif //!sConfigShell





#if 0

/****************************************************************************/
/***                                                                      ***/
/***   Input                                                              ***/
/***                                                                      ***/
/****************************************************************************/

uint sGetKeyQualifier()
{
  return 0;
}

void sGetTimeAndDate(sTimeAndDate &date)
{
  sFatal("not implemented");
}
  
double sGetTimeSince2001()
{
  return sGetTimeAndDateIOSPrivate();
}
  
bool sExecuteOpen(const char *file)
{
  sString<sFormatBuffer> loadmsg;
  
  const char *s = file;
  if(sCmpStringILen(s,"http://",7)==0) s+=7;
  //  if(sCmpStringILen(s,"www.",4)==0) s+=4;
  //  if(sCmpStringILen(s,"en.",3)==0) s+=3;
  const char *t = s;
  while(*s!=0 && *s!='/')
    s++;
  
  
  loadmsg.Add("<html><body><center><h2><br><br><br><br>Redirecting to<br>");
  loadmsg.Add(t,s-t);
  loadmsg.Add("</h2><br><h3>");
  
  while(*s!=0 && *s!='/')
    s++;
  if(*s=='/')
    s++;
  if(sCmpStringILen(s,"wiki/",5)==0) s+=5;
  t = s;
  while(*s!=0 && *s!='/' && *s!='#')
    s++;
  
  loadmsg.Add(t,s-t);
  loadmsg.Add("</h3></center></body></html>");
  
  sLogF("sys","load url %q",file);
  sLogF("sys","message %q",loadmsg);
  //sStartBrowser(file,loadmsg);
  
  return 1;
}
  
const char *sGetUserName()
{
  return "dummy";
}
  
/****************************************************************************/
/***                                                                      ***/
/***   Ios Features                                                       ***/
/***                                                                      ***/
/****************************************************************************/



float sGetDisplayScaleFactor()
{
  return 1.0f;
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


  
}

#endif
