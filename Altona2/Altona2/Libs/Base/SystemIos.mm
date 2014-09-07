/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012 Dierk Ohlerich, all rights reserved                       ***/
/***                                                                      ***/
/****************************************************************************/

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <AudioToolbox/AudioToolbox.h>

#ifdef MOTIONMANAGER
#import <CoreMotion/CMError.h>
#import <CoreMotion/CMErrorDomain.h>
#import <CoreMotion/CMMotionManager.h>
#endif

#if !sConfigNoSetup
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#endif

#include "Altona2/Libs/Base/Base.hpp"
#include "Altona2/Libs/Base/SystemPrivate.hpp"
#include "Altona2/Libs/Base/SystemIos.hpp"


//#define MSAAON

namespace Altona2 {
namespace Setup {
static void sTouch(int mode,Altona2::Setup::sPrivateTouch *td,int count,int timestamp);
static bool sIosRetina = false;
}
}

namespace Altona2 {
    
using namespace Private;
   
 
/****************************************************************************/
 
static double GetTimeBase = -1;
uint sGetTimeMS()
{
	if(GetTimeBase==-1)
    	GetTimeBase = CACurrentMediaTime();
	return uint((CACurrentMediaTime()-GetTimeBase)*1000.0f);
}
    
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
 
void sGetAppId(sString<sMaxPath> &appid)
{
    const char *str = [[[NSBundle mainBundle] bundleIdentifier] UTF8String];
    strcpy(appid.GetBuffer(), str);    
}
    
/****************************************************************************/
/***                                                                      ***/
/***   Input                                                              ***/
/***                                                                      ***/
/****************************************************************************/

uint sGetKeyQualifier()
{
    sFatal("sGetKeyQualifier()");
    return 0;
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
    
	int sArgC;
    char **sArgV;
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
 
  AudioSessionInitialize(0,0,0,0);
  AOFlags = flags^1;
  sUpdateAudioOut(flags);
  
	AudioComponentDescription defaultOutputDescription;
	defaultOutputDescription.componentType = kAudioUnitType_Output;
	defaultOutputDescription.componentSubType = kAudioUnitSubType_RemoteIO;
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
  err = AudioOutputUnitStart(AOUnit);
	sASSERT(err == noErr);
  return freq;
}

void sUpdateAudioOut(int flags)
{
  if(AOFlags!=flags)
  {
    uint value;
    AOFlags = flags;
  
    value = (flags & sAOF_AllowBackgroundMusic) ? kAudioSessionCategory_AmbientSound : kAudioSessionCategory_SoloAmbientSound;
    AudioSessionSetProperty(kAudioSessionProperty_AudioCategory,sizeof(value),&value);
  
//    value = (flags & sAOF_AllowBackgroundMusic) ? 1 : 0;
//    AudioSessionSetProperty(kAudioSessionProperty_OtherMixableAudioShouldDuck,sizeof(value),&value);

    AudioSessionSetActive(0);
    AudioSessionSetActive(1);
  }
}

void sStopAudioOut()
{
  sASSERT(AOEnable==1)
  AudioLock->Lock();
  AOEnable = 0;
  AudioOutputUnitStop(AOUnit);
  AudioUnitUninitialize(AOUnit);
  AudioComponentInstanceDispose(AOUnit);
  AudioSessionSetActive(0);
  AudioLock->Unlock();
  sDelete(AudioLock);
  AOUnit = 0;
}

int sGetSamplesQueuedOut()
{
    return 0;
} //namespace Altona2


}



#if !sConfigNoSetup

@class Altona2ViewController;
@interface Altona2AppDelegate : NSObject <UIApplicationDelegate>
{
    UIWindow *window;
    Altona2ViewController *viewController;    
}
@end

@interface Altona2GLView : UIView
{
    GLint framebufferWidth;
    GLint framebufferHeight;
	EAGLContext *context;
	GLuint defaultFramebuffer;
    GLuint colorRenderbuffer;
    GLuint depthRenderbuffer;
    
	GLuint msaaFramebuffer, msaaRenderBuffer, msaaDepthBuffer;        
}

- (void)setFramebuffer;
- (BOOL)presentFramebuffer;
- (void)createFramebuffer;
- (void)deleteFramebuffer;
@end

@interface Altona2ViewController : UIViewController<UIWebViewDelegate>
{
	Altona2GLView *glView;
    BOOL animating;
    CADisplayLink *displayLink;
    EAGLContext *context;
#ifdef MOTIONMANAGER
    CMMotionManager *motionManager;
	CMAttitude *referenceAttitude;
#endif
}

// Initialization and teardown
- (id)init;
- (void)startAnimation;
- (void)stopAnimation;
- (void)enableGyro;
- (float)getDeviceGLRotationMatrix;
@end

GLfloat rotMatrix[16];

Altona2ViewController *ViewController = 0;

@implementation Altona2AppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions 
{	
	window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
	if (!window) 
	{
		[self release];
		return NO;
	}
	window.autoresizesSubviews = YES;
	window.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
	
    viewController = [[Altona2ViewController alloc] init];
    ViewController = viewController;
    
	//[window addSubview:viewController.view];
    [window setRootViewController:viewController];
	[[UIApplication sharedApplication] setStatusBarHidden:YES];
    
    [window makeKeyAndVisible];
	[window layoutSubviews];

	while (glGetError());

    Altona2::sSubsystem::SetRunlevel(0xff);
	if (Altona2::CurrentApp)
	    Altona2::CurrentApp->OnInit();
    
	return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application {
    /*
     Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
     Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
     */
     [viewController stopAnimation];
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
    /*
     Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later. 
     If your application supports background execution, called instead of applicationWillTerminate: when the user quits.
     */
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
    /*
     Called as part of  transition from the background to the inactive state: here you can undo many of the changes made on entering the background.
     */
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
    /*
     Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
     */
	[viewController startAnimation];
}

- (void)applicationWillTerminate:(UIApplication *)application {
    /*
     Called when the application is about to terminate.
     See also applicationDidEnterBackground:.
     */
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application {
    /*
     Free up as much memory as possible by purging cached data objects that can be recreated (or reloaded from disk) later.
     */
}
- (void)dealloc
{
    [viewController release];
    [window release];
    [super dealloc];
}
@end


@implementation Altona2GLView
// Override the class method to return the OpenGL layer, as opposed to the normal CALayer
+ (Class) layerClass 
{
	return [CAEAGLLayer class];
}

- (id)initWithFrame:(CGRect)frame 
{
    if ((self = [super initWithFrame:frame])) 
	{
	    CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;
        
    	eaglLayer.opaque = TRUE;
	    eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                   [NSNumber numberWithBool:FALSE], kEAGLDrawablePropertyRetainedBacking,
                                   kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat,
                                   nil];
                                   
    	if([[UIScreen mainScreen] respondsToSelector:@selector(scale)]==YES && [[UIScreen mainScreen] scale] == 2.00)
	    {
        	[self setContentScaleFactor:2.0f];
            Altona2::Setup::sIosRetina = true;
    	}
    	[self setMultipleTouchEnabled:true];
    }
    return self;
}

- (void)dealloc
{
    [self deleteFramebuffer];    
    [context release];
    
    [super dealloc];
}

- (void)setContext:(EAGLContext *)newContext
{
    if (context != newContext)
    {
        [self deleteFramebuffer];
        
        [context release];
        context = [newContext retain];
        
        [EAGLContext setCurrentContext:nil];
    }
}

- (void)createFramebuffer
{
    if (context && !defaultFramebuffer)
    {
        [EAGLContext setCurrentContext:context];
        
        // Create default framebuffer object.
        glGenFramebuffers(1, &defaultFramebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebuffer);
        
        // Create color render buffer and allocate backing store.
        glGenRenderbuffers(1, &colorRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
        [context renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer *)self.layer];                
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &framebufferWidth);
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &framebufferHeight);
        
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorRenderbuffer);

        glGenRenderbuffers(1, &depthRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, framebufferWidth, framebufferHeight);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
      
#ifdef MSAAON
      
      
		//Generate our MSAA Frame and Render buffers
    	glGenFramebuffers(1, &msaaFramebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, msaaFramebuffer);

        glGenRenderbuffers(1, &msaaRenderBuffer);
        //Bind our MSAA buffers
        glBindRenderbuffer(GL_RENDERBUFFER, msaaRenderBuffer);
        // Generate the msaaDepthBuffer.
        // 4 will be the number of pixels that the MSAA buffer will use in order to make one pixel on the render buffer.
        glRenderbufferStorageMultisampleAPPLE(GL_RENDERBUFFER, 4, GL_RGBA8_OES, framebufferWidth, framebufferHeight);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, msaaRenderBuffer);

        glGenRenderbuffers(1, &msaaDepthBuffer);
        //Bind the msaa depth buffer.
        glBindRenderbuffer(GL_RENDERBUFFER, msaaDepthBuffer);
        glRenderbufferStorageMultisampleAPPLE(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT16, framebufferWidth , framebufferHeight);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, msaaDepthBuffer);
#endif
      
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            NSLog(@"Failed to make complete framebuffer object %x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
        

#ifdef MSAAON
        Altona2::Setup::sSetGLFrameBuffer(msaaFramebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, msaaFramebuffer);
#else
        Altona2::Setup::sSetGLFrameBuffer(defaultFramebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebuffer);        
#endif
      	Altona2::Setup::sSetScreenSize(framebufferWidth,framebufferHeight);

    }
}

- (void)deleteFramebuffer
{
    if (context)
    {
        [EAGLContext setCurrentContext:context];
        
        if (defaultFramebuffer)
        {
            glDeleteFramebuffers(1, &defaultFramebuffer);
            defaultFramebuffer = 0;
        }
        
        if (colorRenderbuffer)
        {
            glDeleteRenderbuffers(1, &colorRenderbuffer);
            colorRenderbuffer = 0;
        }
      
      	if (depthRenderbuffer)
        {
        	glDeleteRenderbuffers(1, &depthRenderbuffer);
        	depthRenderbuffer = 0;
      	}
        
#ifdef MSAAON        
        if (msaaFramebuffer)
        {
            glDeleteFramebuffers(1, &msaaFramebuffer);
            msaaFramebuffer = 0;
        }
        
        if (msaaRenderBuffer)
        {
            glDeleteRenderbuffers(1, &msaaRenderBuffer);
            msaaRenderBuffer = 0;
        }
      
      	if (msaaDepthBuffer)
        {
        	glDeleteRenderbuffers(1, &msaaDepthBuffer);
        	msaaDepthBuffer = 0;
      	} 
#endif
    }
}

- (void)setFramebuffer
{
    if (context)
    {
    	[EAGLContext setCurrentContext:context];
        if (!defaultFramebuffer)
            [self createFramebuffer];
#ifdef MSAAON
        glBindFramebuffer(GL_FRAMEBUFFER, msaaFramebuffer);
#else
        glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebuffer);        
#endif
        glViewport(0, 0, framebufferWidth, framebufferHeight);
    }
}

- (BOOL)presentFramebuffer
{
    BOOL success = FALSE;
    if (context)
    {
    	[EAGLContext setCurrentContext:context];
#ifdef MSAAON
        glBindFramebuffer(GL_READ_FRAMEBUFFER_APPLE, msaaFramebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER_APPLE, defaultFramebuffer);
        glResolveMultisampleFramebufferAPPLE();
        
        const GLenum discards[]  = {GL_COLOR_ATTACHMENT0,GL_DEPTH_ATTACHMENT};
		glDiscardFramebufferEXT(GL_READ_FRAMEBUFFER_APPLE,2,discards);
  
        // Present final image to screen
        glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
#else
        glBindFramebuffer(GL_READ_FRAMEBUFFER_APPLE, defaultFramebuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
#endif
        success = [context presentRenderbuffer:GL_RENDERBUFFER];
    }
    return success;
}

- (void)layoutSubviews
{
    [self deleteFramebuffer];
}

- (void)touchesPrivate:(NSSet *)touches withEvent:(UIEvent *)event Mode:(int)mode
{
	Altona2::Setup::sPrivateTouch pt[10];
  	int n = 0;
  
	//  for(UITouch *t in [event allTouches])
	for(UITouch *t in touches)
  	{
    	if(n<10)
    	{
      		CGPoint loc = [t locationInView: self];
      		pt[n].x = loc.x;
      		pt[n].y = loc.y;
      		pt[n].id_ = (unsigned int)t;
      		pt[n].count = [t tapCount];
      		n++;
    	}
  	}
  	sTouch(mode,pt,n,(int)([event timestamp]*1000));
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
  [self touchesPrivate: touches withEvent: event Mode: 1];
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
  [self touchesPrivate: touches withEvent: event Mode: 2];
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
  [self touchesPrivate: touches withEvent: event Mode: 3];
}

- (void) touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
  [self touchesPrivate: touches withEvent: event Mode: 4];
}



@end

@implementation Altona2ViewController

- (id)init
{
#ifdef MOTIONMANAGER
	motionManager = [[CMMotionManager alloc] init];
	referenceAttitude = nil;
#endif

	[self enableGyro];

	context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    
  	if (!context)
    	NSLog(@"Failed to create ES context");
  	else if (![EAGLContext setCurrentContext:context])
    	NSLog(@"Failed to set ES context current");
        
    if ((self = [super initWithNibName:nil bundle:nil])) 
	{
		animating = FALSE;
  	    displayLink = nil;
    }
    return self;
}

-(void) enableGyro
{
#ifdef MOTIONMANAGER
	if (motionManager.gyroAvailable)
    {
		CMDeviceMotion *deviceMotion = motionManager.deviceMotion;
	    CMAttitude *attitude = deviceMotion.attitude;
	    referenceAttitude = [attitude retain];
    	[motionManager startGyroUpdates];
        [motionManager startDeviceMotionUpdates];
    }
#endif
}
-(void) getDeviceGLRotationMatrix
{
#ifdef MOTIONMANAGER

        CMDeviceMotion *deviceMotion = motionManager.deviceMotion;
        CMAttitude *attitude = deviceMotion.attitude;

        if (referenceAttitude != nil) [attitude multiplyByInverseOfAttitude:referenceAttitude];
        CMRotationMatrix rot=attitude.rotationMatrix;
        rotMatrix[0]=rot.m11; rotMatrix[1]=rot.m21; rotMatrix[2]=rot.m31;  rotMatrix[3]=0;
        rotMatrix[4]=rot.m12; rotMatrix[5]=rot.m22; rotMatrix[6]=rot.m32;  rotMatrix[7]=0;
        rotMatrix[8]=rot.m13; rotMatrix[9]=rot.m23; rotMatrix[10]=rot.m33; rotMatrix[11]=0;
        rotMatrix[12]=0;      rotMatrix[13]=0;      rotMatrix[14]=0;       rotMatrix[15]=1;
#endif
}

- (void)loadView 
{
	CGRect applicationFrame = [[UIScreen mainScreen] applicationFrame];
	CGRect mainScreenFrame = [[UIScreen mainScreen] applicationFrame];	
	UIView *primaryView = [[UIView alloc] initWithFrame:mainScreenFrame];
	self.view = primaryView;
	[primaryView release];

	glView = [[Altona2GLView alloc] initWithFrame:CGRectMake(0.0f, 0.0f, applicationFrame.size.width, applicationFrame.size.height)];
    
    [glView setContext:context];
  	[glView setFramebuffer];

	glView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    [self.view addSubview:glView];
	[glView release];
}

- (void)didReceiveMemoryWarning 
{
//    [super didReceiveMemoryWarning];
}

- (void)startAnimation
{
  	if (!animating)
  	{
    	displayLink = [[UIScreen mainScreen] displayLinkWithTarget:self selector:@selector(drawFrame)];
    	[displayLink setFrameInterval:1];
    	[displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
        animating = TRUE;
  	}
}

- (void)stopAnimation
{
  	if (animating)
  	{
    	[displayLink invalidate];
    	displayLink = nil;
    	animating = FALSE;
  	}
}

- (void)dealloc 
{
   [super dealloc];
}

- (void)drawFrame
{
	[glView setFramebuffer];
   	Altona2::Setup::sOnFrameApp();
   	Altona2::Setup::sOnPaintApp();    
    [glView presentFramebuffer];
}

- (BOOL) shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation) io
{
/*
	if (io==UIInterfaceOrientationLandscapeLeft || io==UIInterfaceOrientationLandscapeRight)
		return YES;
    return NO;
*/
	return YES;
}

/*
- (NSUInteger)supportedInterfaceOrientations
{
//	return UIInterfaceOrientationMaskLandscape;
	return UIInterfaceOrientationMaskAll;
}

- (UIInterfaceOrientation)preferredInterfaceOrientationForPresentation
{
	return UIInterfaceOrientationLandscapeLeft;
//	return UIInterfaceOrientationPortrait;

}
*/

@end

int main(int argc, char * argv[])
{
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    
	Altona2::Private::sArgC = argc;
    Altona2::Private::sArgV = argv;
    
    Altona2::Setup::sInitSystem();
	Altona2::Main();
    
    [pool release];
    return 0;
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

#if 0
float *sGetGyroMatrix()
{	
	if (ViewController)
    {
    	[ViewController getDeviceGLRotationMatrix];
        return rotMatrix;
    }
    return 0;
}
#endif

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
    
        NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
        NSString *visDocDirectory = [[paths objectAtIndex:0] stringByAppendingPathComponent:@"visarity"];
 
//  		str = [[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,NSUserDomainMask,YES) lastObject] UTF8String];
        str =[visDocDirectory UTF8String];
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
    	sAddFileHandler(new sLinkFileHandler(sHomePath));
//    	sChangeDir(sHomePath);
  	}
} sFixPathSubsystem_;
 
#if !sConfigNoSetup
void sRunApp(sApp *currentapp,const sScreenMode &sm)
{
    CurrentApp = currentapp;
    Altona2::Private::ExitFlag = false;
    UIApplicationMain(Altona2::Private::sArgC, Altona2::Private::sArgV, nil, @"Altona2AppDelegate");
}
#endif

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
  x = 0;
  y = 0;
  b = 0;
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

const int MaxTouch=10;
uint TouchId[MaxTouch];
bool PrimaryFingerLeft;
bool Touching;

static void sMouseToKey(sApp *app,const sDragData &dd)
{
  if(dd.Mode==sDM_Start)
  {
    sKeyData kd;
    kd.Key = sKEY_LMB;
    kd.MouseX = dd.PosX;
    kd.MouseY = dd.PosY;
    kd.Timestamp = dd.Timestamp;
    app->OnKey(kd);
  }
  if(DragData.Mode==sDM_Stop)
  {
    sKeyData kd;
    kd.Key = sKEY_LMB|sKEYQ_Break;
    kd.MouseX = dd.PosX;
    kd.MouseY = dd.PosY;
    kd.Timestamp = dd.Timestamp;
    app->OnKey(kd);
  }
}


static void sTouch(int mode,sPrivateTouch *td,int count,int timestamp)
{
  if(0)
  {
    sDPrintF("%08x %d",timestamp,mode);
    for(int i=0;i<count;i++)
      sDPrintF(" : %08x %4d %4d",td[i].id_,td[i].x,td[i].y); 
    sDPrint("\n");
  }
  
  // a new start?
  
  DragData.Timestamp = timestamp;
  DragData.Buttons = 1;
  if(!Touching && mode==1)
  {
    sClear(TouchId);
    PrimaryFingerLeft = 0;
  }
  
  // retina
  
  int retina = sIosRetina ? 2 : 1;
  
  // update DragData
  
  for(int i=0;i<count;i++)
  {
    bool found = 0;
    for(int j=0;j<MaxTouch;j++)
    {
      if(TouchId[j]==td[i].id_)
      {
        found = 1;
        TouchId[j]= td[i].id_;
        DragData.Touches[j].PosX = td[i].x*retina;
        DragData.Touches[j].PosY = td[i].y*retina;
        DragData.Touches[j].Mode = mode==2 ? sDM_Drag : sDM_Stop;
        if(mode==1)
          sLog("sys","touches out of sequence (old touch)");
        break;
      }
    }
    if(!found)
    {
      for(int j=0;j<MaxTouch;j++)
      {
        if(TouchId[j]==0)
        {
          found = 1;
          TouchId[j]= td[i].id_;
          DragData.Touches[j].PosX = td[i].x*retina;
          DragData.Touches[j].PosY = td[i].y*retina;
          DragData.Touches[j].StartX = td[i].x*retina;
          DragData.Touches[j].StartY = td[i].y*retina;
          DragData.Touches[j].Mode = sDM_Start;
          if(mode!=1)
            sLog("sys","touches out of sequence (new touch)");
          break;
        }
      }
    }
    if(!found)
      sLog("sys","too many touches");
  }
  
  // make first drag the primary drag
  
  if(!PrimaryFingerLeft)
  {
    DragData.PosX = DragData.Touches[0].PosX;
    DragData.PosY = DragData.Touches[0].PosY;
    DragData.StartX = DragData.Touches[0].StartX;
    DragData.StartY = DragData.Touches[0].StartY;
    DragData.DeltaX = DragData.PosX - DragData.StartX;
    DragData.DeltaY = DragData.PosY - DragData.StartY;
    DragData.Mode = DragData.Touches[0].Mode;
  }
  for(int j=0;j<MaxTouch;j++)
    if(TouchId[j])
      DragData.TouchCount = j+1;
  
  // do start and drag events. there is no hover!
  
  if(mode==1 && !Touching)
  {
    Touching = 1;
    DragData.Mode = sDM_Start;
    CurrentApp->OnDrag(DragData);
    sMouseToKey(CurrentApp,DragData);
    for(int i=0;i<DragData.TouchCount;i++)
      if(DragData.Touches[i].Mode == sDM_Start)
        DragData.Touches[i].Mode = sDM_Drag;
  }
  if(DragData.Touches[0].Mode!=sDM_Drag)
    PrimaryFingerLeft=1;
  
  // is this the end?
  
  DragData.Mode = sDM_Drag;         // this is not the end
  if(mode==4)
  {
    sClear(TouchId);
    DragData.Mode = sDM_Stop;       // oh it is totally the end
    Touching = 0;
  }
  if(mode==3)
  {
    int left = 0;
    for(int j=0;j<MaxTouch;j++)
    {
      if(TouchId[j])
      {
        for(int i=0;i<count;i++)
        {
          if(TouchId[j]==td[i].id_)
          {
            TouchId[j] = 0;
            break;
          }
        }
      }
      if(TouchId[j])
        left++;
    }
    if(!left)
    {
      DragData.Mode = sDM_Stop;     // last finger left, this is the end
      Touching = 0;
    }
  }
  
  // send drag and stop events for individual touches
  
  CurrentApp->OnDrag(DragData);
  sMouseToKey(CurrentApp,DragData);
  for(int j=0;j<MaxTouch;j++)
    if(DragData.Touches[j].Mode==sDM_Stop)
      DragData.Touches[j].Mode = sDM_Hover;
}


static uint qmask = 0;
/*
static void HandlingFlags(NSEvent *event)
{
    unsigned long mf = [event modifierFlags];
    qmask = 0;
    if (mf&NSShiftKeyMask)
        qmask |= sKEYQ_Shift;
    if (mf&NSControlKeyMask)
        qmask |= sKEYQ_Ctrl;
    if (mf&NSCommandKeyMask)
        qmask |= sKEYQ_Alt;
}

static void HandleKey(NSEvent *event, uint mask)
{
    sKeyData kd;
    unichar c = 0;
    uint kc = [event keyCode];
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
	        c = [[event charactersIgnoringModifiers] characterAtIndex:0] & 0x7fff;
        	break;
    }
    if ([event isARepeat]) mask |= sKEYQ_Repeat;
    kd.Key = c | mask | qmask;
	CurrentApp->OnKey(kd);
}
*/

bool sCheckRetina()
{
    if ([[UIScreen mainScreen] respondsToSelector:@selector(scale)] && [[UIScreen mainScreen] scale] == 2.0)
    {
        sIosRetina = true;
    }
    return sIosRetina;
}

bool sInitSystem()
{
	Altona2::sAltonaInit();
#if sConfigNoSetup
    sSubsystem::SetRunlevel(0xff);
#endif
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

void sHandleKey(void *event, int type)
{
/*
	NSEvent *e = (NSEvent *)event;
    if (!Altona2::Private::ExitFlag && CurrentApp)
    {
    	if (type==sKT_Flags)
	        HandlingFlags(e);
        else
	        HandleKey(e, type==sKT_Up ? sKEYQ_Break:0);
    }
*/
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


#endif
