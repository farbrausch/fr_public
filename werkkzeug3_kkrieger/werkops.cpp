// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "werkkzeug.hpp"
#include "winpara.hpp"

#include "genbitmap.hpp"
#include "genscene.hpp"
#include "genmesh.hpp"
#include "genminmesh.hpp"
#include "genoverlay.hpp"
#include "genmaterial.hpp"
#include "geneffect.hpp"
#include "geneffectex.hpp"
#include "geneffectdebris.hpp"
#include "geneffectipp.hpp"
#include "genblobspline.hpp"

#define TEXTSIZE 9

static sChar *texnames[8] = {"Texture 0","Texture 1","Texture 2","Texture 3","Texure 4","Texture 5","Texture 6","Texture 7"};

/****************************************************************************/
/****************************************************************************/

#define COL_NEW   1
#define COL_MOD   2
#define COL_ADD   3
#define COL_XTR   4
#define COL_ALL   5
#define COL_HIDE  6

/****************************************************************************/
/****************************************************************************/

void Edit_Misc_Load(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditLink("Load",0);
}

void Edit_Misc_Store(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
}

void Edit_Misc_Nop(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
}

void Edit_Misc_Event(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("Default Duration" , 0,1,  0,   0, 64,0.01f);
}

void Edit_Misc_Trigger(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditGroup ("Condition");
  app->ParaWin->EditLink  ("Event",0);
  app->ParaWin->EditFloat ("*Trigger Value"   , 0,1,0.0,-256,256,0.01f);
  app->ParaWin->EditFloat ("Trigger Treshold" , 1,1,0.5,-256,256,0.01f);
  app->ParaWin->EditFloat ("Retrigger Delay"  , 2,1,  1,   0, 64,0.01f);
  app->ParaWin->EditFloat ("Event Duration"   , 3,1,  1,   0, 64,0.01f);
  app->ParaWin->EditFlags ("Flags"            , 4,0,"above|below:*1once|retrigger:*2|srt from scene");
  app->ParaWin->EditGroup ("Event Parameter");
  app->ParaWin->EditFloat ("*Velocity"        , 5,1,0,-256,256,0.01f);
  app->ParaWin->EditFloat ("*Modulation"      , 6,1,0,-256,256,0.01f);
  app->ParaWin->EditInt   ("*Select"          , 7,1,0,   0, 16,0.25f);
  app->ParaWin->EditFloat ("*1Scale"          , 8,3,0,-256,256,0.01f);
  app->ParaWin->EditFloat ("*2Rotate"         ,11,3,0, -64, 64,0.002f);
  app->ParaWin->EditFloat ("*3Translate"      ,14,3,0,-4096,4096,0.001f);
  app->ParaWin->EditRGBA  ("*4Color"          ,17,0xffffffff);
  app->ParaWin->EditSpline("Spline"           , 0);
}

void Edit_Misc_Delay(WerkkzeugApp *app,WerkOp *op)
{
  sInt i;
  i = 0xc0&*op->Op.GetAnimPtrU(0);
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFlags ("!Input"           , 0,0,"*6switch|velocity|modulation|select");
  app->ParaWin->EditFlags ("Output"           , 0,0,"*4time|velocity|modulation");
  app->ParaWin->EditFlags ("Animation"        , 0,0,"0 - 1 - 0|0 - 0.5 - 1|0 - 0.25 - 0.75 - 1");  
  app->ParaWin->EditSetGray(i!=0);
  app->ParaWin->EditInt   ("Switch"           , 1,1,0,   0,255,0.25f);
  app->ParaWin->EditSetGray(i==0);
  app->ParaWin->EditFloat ("Treshold"         , 2,1,0.5,-256,256,0.01f);
  app->ParaWin->EditFloat ("Hysteresis"       , 3,1,0.0,-256,256,0.01f);
  app->ParaWin->EditSetGray(0);
  app->ParaWin->EditFloat ("On-Time"          , 4,1,  1,   0,1024,0.01f);
  app->ParaWin->EditFloat ("Loop-Time"        , 5,1,  1,   0,1024,0.01f);
  app->ParaWin->EditFloat ("Off-Time"         , 6,1,  1,   0,1024,0.01f);
}

void Edit_Misc_PlaySample(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditInt   ("Switch #"         , 0,1,0,   0,255,0.25f);
  app->ParaWin->EditInt   ("Switch Value"     , 1,1,0,  -1,127,0.25f);
  app->ParaWin->EditFloat ("*Animate (>0)"    , 2,1,0.5,-1,1,0.01f);  
  app->ParaWin->EditInt   ("Sample"           , 3,1,0,   0,255,0.25f);
  app->ParaWin->EditFloat ("*Volume (-db)"    , 4,1,1.0, 0,100,1.0f);  
  app->ParaWin->EditFloat ("Retrigger (sec)"  , 5,1,0.0, 0,100,1.0f);  
  app->ParaWin->EditFloat ("Half-range"       , 6,1,0,   0,255,0.01f);
}

void Edit_Misc_If(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditInt   ("Switch"           , 0,1,0,0,255,0.25f);
  app->ParaWin->EditInt   ("Value"            , 1,1,1,0,255,0.25f);
}

void Edit_Misc_SetIf(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditInt   ("Switch"           , 0,1,0,0,255,0.25f);
  app->ParaWin->EditFlags ("Compare"          , 2,0,"*4equal|not equal|greater or equal|lesser");
  app->ParaWin->EditInt   ("Value"            , 1,1,1,0,255,0.25f);
  app->ParaWin->EditFlags ("!Variable"        , 2,0,"time|velocity|modulation|select|set switch|enable switch|disable switch");
  app->ParaWin->EditSetGray((*op->Op.GetEditPtrU(2)&15)<4);
  app->ParaWin->EditInt   ("Output Switch"    , 3,1,0,0,255,0.25f);
}

void Edit_Misc_State(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditInt   ("Switch"           , 0,1,16,0,255,0.25f);
  app->ParaWin->EditInt   ("Value"            , 1,1,1,0,255,0.25f);
  app->ParaWin->EditFlags ("Condition"        , 2,4,"always|space|enter|escape|up|down|left|right");
  app->ParaWin->EditInt   ("Cond. Switch"     , 3,1,0,0,255,0.25f);
  app->ParaWin->EditInt   ("Cond. Value"      , 4,1,1,0,255,0.25f);
}

void Edit_Misc_Menu(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditInt   ("Switch"           , 0,1,16,0,255,0.25f);
  app->ParaWin->EditInt   ("Count (0..n-1)"   , 1,1,1,0,255,0.25f);
  app->ParaWin->EditFlags ("Flags"            , 2,4,"left/right|up/down");
}

void Edit_Misc_Demo(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
}

void Edit_Misc_MasterCam(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("*2Rotate"     , 0,3, 0.0f,  -64,   64,0.002f);
  app->ParaWin->EditFloat ("*3Translate"  , 3,3, 0.0f,-4096, 4096,0.001f);
  app->ParaWin->EditFloat ("Far Clip"     , 6,1, 4096.0f, 0,65536,1.000f);
  app->ParaWin->EditFloat ("Near Clip"    , 7,1, 0.125f,   0,  128,0.002f);
  app->ParaWin->EditFloat ("*Center"      , 8,2, 0.0f,   -4,    4,0.002f);
  app->ParaWin->EditFloat ("*Zoom"        ,10,2, 1.0f,    0,  256,0.020f);
}

/****************************************************************************/

void Edit_KKrieger_Para(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditGroup ("Collectables Start/Max");
  app->ParaWin->EditInt   ("*Life"             , 0,2, 100,0,0x4000,10);
  app->ParaWin->EditInt   ("*Armor"            , 2,2,  10,0,0x4000,10);
  app->ParaWin->EditInt   ("*Alpha"            , 4,2,   0,0,0x4000,10);
  app->ParaWin->EditInt   ("*Beta"             , 6,2,   0,0,0x4000,10);
  app->ParaWin->EditInt   ("*Ammo 0"           , 8,2,  50,0,0x4000,10);
  app->ParaWin->EditInt   ("*Ammo 1"           ,10,2,  50,0,0x4000,10);
  app->ParaWin->EditInt   ("*Ammo 2"           ,12,2,  50,0,0x4000,10);
  app->ParaWin->EditInt   ("*Ammo 3"           ,14,2,  50,0,0x4000,10);
  app->ParaWin->EditFlags ("*Weapons"          ,16,7,"*0|0:*1|1:*2|2:*3|3");
  app->ParaWin->EditFlags ("*Weapons"          ,16,7,"*4|4:*5|5:*6|6:*7|7");
  app->ParaWin->EditGroup ("Player");
  app->ParaWin->EditFloat ("*Gravity Player"   ,17,1,0.01f, -1, 1,0.0001f);
  app->ParaWin->EditFloat ("*Damping Ground"   ,18,1,0.05f,  0, 1,0.0001f);
  app->ParaWin->EditFloat ("*Damping Air"      ,19,1,0.05f,  0, 1,0.0001f);
  app->ParaWin->EditFloat ("*Damping Height"   ,20,1,0.05f,  0, 1,0.0001f);
  app->ParaWin->EditFloat ("*Damping Cam"      ,56,1,0.05f,  0, 1,0.0001f);
  app->ParaWin->EditFloat ("*Stair Height"     ,21,1,0.55f,  0,64,0.01f);
  app->ParaWin->EditFloat ("*Eye Height"       ,22,1,2.00f,  0,64,0.01f);
  app->ParaWin->EditFloat ("*Mouse Turn Speed" ,23,1,0.01f, -1, 1,0.0001f);
  app->ParaWin->EditFloat ("*Mouse Look Speed" ,24,1,0.01f, -1, 1,0.0001f);
  app->ParaWin->EditFloat ("*Side Speed"       ,25,1,0.005f, 0, 1,0.0001f);
  app->ParaWin->EditFloat ("*Forward Speed"    ,26,1,0.005f, 0, 1,0.0001f);
  app->ParaWin->EditFloat ("*Backward Speed"   ,27,1,0.005f, 0, 1,0.0001f);
  app->ParaWin->EditFloat ("*Jump Force"       ,28,1,0.35f,  0,64,0.01f);
  app->ParaWin->EditFloat ("*Start Pos"        ,29,3,0 , -256,256,0.01f);
  app->ParaWin->EditGroup ("Weapon Flags / Hits / Reload-Time");
  app->ParaWin->EditFlags ("*Weapon 0"         ,48+0,0,"*0|Continuos:*1|Reflect");
  app->ParaWin->EditInt   (":"                 ,32+0,1,0,0,0x4000,10);
  app->ParaWin->EditFloat (":"                 ,40+0,1,0,0,64,0.01f);
  app->ParaWin->EditFlags ("*Weapon 1"         ,48+1,0,"*0|Continuos:*1|Reflect");
  app->ParaWin->EditInt   (":"                 ,32+1,1,0,0,0x4000,10);
  app->ParaWin->EditFloat (":"                 ,40+1,1,0,0,64,0.01f);
  app->ParaWin->EditFlags ("*Weapon 2"         ,48+2,0,"*0|Continuos:*1|Reflect");
  app->ParaWin->EditInt   (":"                 ,32+2,1,0,0,0x4000,10);
  app->ParaWin->EditFloat (":"                 ,40+2,1,0,0,64,0.01f);
  app->ParaWin->EditFlags ("*Weapon 3"         ,48+3,0,"*0|Continuos:*1|Reflect");
  app->ParaWin->EditInt   (":"                 ,32+3,1,0,0,0x4000,10);
  app->ParaWin->EditFloat (":"                 ,40+3,1,0,0,64,0.01f);
  app->ParaWin->EditFlags ("*Weapon 4"         ,48+4,0,"*0|Continuos:*1|Reflect");
  app->ParaWin->EditInt   (":"                 ,32+4,1,0,0,0x4000,10);
  app->ParaWin->EditFloat (":"                 ,40+4,1,0,0,64,0.01f);
  app->ParaWin->EditFlags ("*Weapon 5"         ,48+5,0,"*0|Continuos:*1|Reflect");
  app->ParaWin->EditInt   (":"                 ,32+5,1,0,0,0x4000,10);
  app->ParaWin->EditFloat (":"                 ,40+5,1,0,0,64,0.01f);
  app->ParaWin->EditFlags ("*Weapon 6"         ,48+6,0,"*0|Continuos:*1|Reflect");
  app->ParaWin->EditInt   (":"                 ,32+6,1,0,0,0x4000,10);
  app->ParaWin->EditFloat (":"                 ,40+6,1,0,0,64,0.01f);
  app->ParaWin->EditFlags ("*Weapon 7"         ,48+7,0,"*0|Continuos:*1|Reflect");
  app->ParaWin->EditInt   (":"                 ,32+7,1,0,0,0x4000,10);
  app->ParaWin->EditFloat (":"                 ,40+7,1,0,0,64,0.01f);
}

void Edit_KKrieger_Events(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditCycle ("What",0,0,"Weapon Shots|Weapon Optic|Weapon Miss|Weapon Hit");
  app->ParaWin->EditLink  ("Weapon 0"         , 0);
  app->ParaWin->EditLink  ("Weapon 1"         , 1);
  app->ParaWin->EditLink  ("Weapon 2"         , 2);
  app->ParaWin->EditLink  ("Weapon 3"         , 3);
  app->ParaWin->EditLink  ("Weapon 4"         , 4);
  app->ParaWin->EditLink  ("Weapon 5"         , 5);
  app->ParaWin->EditLink  ("Weapon 6"         , 6);
  app->ParaWin->EditLink  ("Weapon 7"         , 7);
}

void Edit_KKrieger_Monster(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditGroup ("Optic");
//  app->ParaWin->EditLink  ("Shot"             , 0);
//  app->ParaWin->EditLink  ("Death"            , 1);

  app->ParaWin->EditGroup ("Behavoir");
  app->ParaWin->EditFloat ("Rotate"           , 0,3,  0.0f,  -64,   64,0.002f);
  app->ParaWin->EditFloat ("Pos"              , 3,3,  0.0f, -256,  256,0.005f);
  app->ParaWin->EditFloat ("Speed"            , 6,1, 0.01f,    0,   16,0.0001f);
  app->ParaWin->EditFloat ("Notice-Radius"    , 7,1,    16,    0,  256,0.01f);
  app->ParaWin->EditFloat ("Wait-Radius"      ,18,1,     8,    0,  256,0.01f);
  app->ParaWin->EditFloat ("Charge-Radius"    , 8,1,     4,    0,  256,0.01f);
  app->ParaWin->EditFloat ("Min-Radius"       ,19,1,     1,    0,  256,0.01f);
  app->ParaWin->EditFlags ("Flags"            , 9,0,"*3|immobile");
  app->ParaWin->EditFloat ("Coll-Size"        ,20,3,   1.0, -256,  256,0.01f);
  app->ParaWin->EditFloat ("Coll-Pos"         ,23,3,   0.0, -256,  256,0.01f);
  app->ParaWin->EditInt   ("Kill Switch"      ,17,1,     0,    0,  255,0.25f);
  app->ParaWin->EditInt   ("Spawn Switch"     ,28,1,     0,    0,  255,0.25f);

  app->ParaWin->EditGroup ("Strength");
  app->ParaWin->EditInt   ("Life"             ,10,1,   100, 0,0x4000,10);
  app->ParaWin->EditInt   ("Armor"            ,11,1,     0, 0,0x4000,10);
  app->ParaWin->EditInt   ("Meelee Hits"      ,12,1,    10, 0,0x4000,10);
  app->ParaWin->EditFloat ("Meelee Time"      ,13,1, 3.00f, 0,    16,0.01f);
  app->ParaWin->EditInt   ("Ranged Hits"      ,14,1,     5, 0,0x4000,10);
  app->ParaWin->EditFloat ("Ranged Time"      ,15,1, 3.00f, 0,    16,0.01f);
  app->ParaWin->EditFloat ("Ranged Aim"       ,16,1, 0.50f, 0,     1,0.01f);

  app->ParaWin->EditGroup ("Other");
  app->ParaWin->EditInt   ("WalkStyle"        ,26,1,     0, 0,5,0.25f);
  app->ParaWin->EditSpline("Step-Spline" , 0);
  app->ParaWin->EditInt   ("Weapon Kind"      ,27,1,     0, -1,7,0.25f);
  app->ParaWin->EditFlags ("Spawning"         , 9,0,"*0|when prev. dies:*1|when machine lives:*2|this is machine");

  // charge-speed
}

void Edit_KKrieger_Respawn(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditInt   ("Zone"             ,3,1,   0,    0,   15,0.25f);
  app->ParaWin->EditFloat ("Posision"         ,0,3,   0,-4096, 4096,0.01f);
  app->ParaWin->EditFloat ("Direction"        ,4,1,   0,  -16,   16,0.01f);
}

void Edit_KKrieger_SplashDamage(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditInt   ("Hits"             ,0,1,   0,    0,  990,0.25f);
  app->ParaWin->EditFloat ("Range"            ,1,1,   0, -256,  256,0.01f);
  app->ParaWin->EditFloat ("*Enable if >= 0"  ,2,1,   0,  -16,   16,0.01f);
}

void Edit_KKrieger_MonsterFlags(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditInt   ("Kill Switch"      , 0,1,     0,    0,  255,0.25f);
  app->ParaWin->EditInt   ("Spawn Switch"     , 1,1,     0,    0,  255,0.25f);
  app->ParaWin->EditFlags ("Spawning"         , 2,0,"*0|when prev. dies:*1|when machine lives:*2|this is machine");

  // charge-speed
}

/****************************************************************************/

void Edit_Misc_Spline(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditBlobSpline();
}

void Edit_Misc_Shaker(WerkkzeugApp *app,WerkOp *op)
{
  sU32 flags = op->Op.GetEditPtrU(0)[0];
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFlags ("Mode"         , 0,0,"fadein|fadeout|smooth:*4around cam|around center:*5sin|perlin");
  app->ParaWin->EditFloat ("Translate Amp", 1,3, 0.0f,-256, 256,0.001f);
  app->ParaWin->EditFloat ("Rotate Amp"   , 4,3, 0.0f,-16, 16,0.001f);
  app->ParaWin->EditFloat ("Translate Freq",7,3, 0.0f,-4096,4096,0.25f);
  app->ParaWin->EditFloat ("Rotate Freq"  ,10,3, 0.0f,-4096,4096,0.25f);
  app->ParaWin->EditSetGray((flags&15)!=3);
  app->ParaWin->EditFloat ("*Animate"     ,13,1, 0.0f, 0, 1,0.001f);
  app->ParaWin->EditSetGray(!(flags&16));
  app->ParaWin->EditFloat ("Center"       ,14,3, 0.0f,-4096,4096,0.1f);
}

void Edit_Misc_PipeSpline(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditBlobPipe();
}

void Edit_Misc_MetaSpline(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("Time 0"       ,0,1, 0.0f,0,1,0.001f);
  app->ParaWin->EditFloat ("Time 1"       ,1,1, 0.0f,0,1,0.001f);
  app->ParaWin->EditFloat ("Time 2"       ,2,1, 0.0f,0,1,0.001f);
  app->ParaWin->EditFloat ("Time 3"       ,3,1, 0.0f,0,1,0.001f);
  app->ParaWin->EditFloat ("Time 4"       ,4,1, 0.0f,0,1,0.001f);
  app->ParaWin->EditFloat ("Time 5"       ,5,1, 0.0f,0,1,0.001f);
  app->ParaWin->EditFloat ("Time 6"       ,6,1, 0.0f,0,1,0.001f);
  app->ParaWin->EditFloat ("Time 7"       ,7,1, 0.0f,0,1,0.001f);
}

/****************************************************************************/

void Edit_Bitmap_Flat(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditCycle ("Width"        , 0,TEXTSIZE,"1|2|4|8|16|32|64|128|256|512|1024|2048");
  app->ParaWin->EditCycle ("Height"       , 1,TEXTSIZE,"1|2|4|8|16|32|64|128|256|512|1024|2048");
  app->ParaWin->EditRGBA  ("Color"        , 2,0xff000000);
}

void Edit_Bitmap_Perlin(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditCycle ("Width"        , 0,TEXTSIZE,"1|2|4|8|16|32|64|128|256|512|1024|2048");
  app->ParaWin->EditCycle ("Height"       , 1,TEXTSIZE,"1|2|4|8|16|32|64|128|256|512|1024|2048");
  app->ParaWin->EditInt   ("Frequency"    , 2,1,    1,    0,   16,0.25f);
  app->ParaWin->EditInt   ("Octaves"      , 3,1,    2,    0,   16,0.25f);
  app->ParaWin->EditFloat ("Fadeoff"      , 4,1, 1.0f,-8.0f, 8.0f,0.01f);
  app->ParaWin->EditInt   ("Seed"         , 5,1,    0,    0,  255,0.25f);
  app->ParaWin->EditCycle ("Mode"         , 6,0,"norm|abs|sin|abs+sin");
  app->ParaWin->EditFloat ("Amplfy"       , 7,1, 1.0f, 0.0f,16.0f,0.01f);
  app->ParaWin->EditFloat ("Gamma"        , 8,1, 1.0f, 0.0f,16.0f,0.01f);
  app->ParaWin->EditRGBA  ("Color 0"      , 9,0xff000000);
  app->ParaWin->EditRGBA  ("Color 1"      ,10,0xffffffff);
}

void Edit_Bitmap_Color(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditCycle ("Mode"         , 0,4,"mul|add|sub|gray|invert|scale");
  app->ParaWin->EditRGBA  ("Color"        , 1,0xff000000);
}

void Edit_Bitmap_Range(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFlags ("Flags"        , 0,0,"adjust|range:*1normal|invert");
  app->ParaWin->EditRGBA  ("Color0"       , 1,0xff000000);
  app->ParaWin->EditRGBA  ("Color1"       , 2,0xffffffff);
}

void Edit_Bitmap_Merge(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFlags ("Mode"         , 0,0,"add|sub|mul|diff|alpha|brightness|hardlight|over|addsmooth");
}

void Edit_Bitmap_Format(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFlags ("Format"       , 0,1,"illegal|A8R8G8B8|A8|R16F|A2R10G10B10|Q8W8V8U8 (bump)|A2W10V10U10");
  app->ParaWin->EditFlags ("Mipmap Count" , 1,0,"all|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15");
  app->ParaWin->EditFlags ("Mipmap Tresh" , 2,0,"1|2|3|4|5|6|7|8|9|10|11|12|13|14|15");
  app->ParaWin->EditFlags ("Treshold Dir" , 2,0,"*4first sharp then smooth|first smooth then sharp");
  app->ParaWin->EditFlags ("Alpha Channel", 2,0,"*5smooth|hard");
}

void Edit_Bitmap_RenderTarget(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditCycle ("Width"        , 0,TEXTSIZE,"0|1|2|4|8|16|32|64|128|256|512|1024|2048");
  app->ParaWin->EditCycle ("Height"       , 1,TEXTSIZE,"0|1|2|4|8|16|32|64|128|256|512|1024|2048");
  app->ParaWin->EditCycle ("Format"       , 2,1,"illegal|A8R8G8B8|A8|R16F|A2R10G10B10|Q8W8V8U8|A2W10V10U10");
}

void Edit_Bitmap_GlowRectOld(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("Center"       , 0,2,0.50f,-4.0f, 4.0f,0.001f);
  app->ParaWin->EditFloat ("Radius"       , 2,2,0.25f, 0.0f, 4.0f,0.001f);
  app->ParaWin->EditFloat ("Size"         , 4,2,0.00f, 0.0f, 4.0f,0.001f);
  app->ParaWin->EditRGBA  ("Color"        , 6,0xffffffff);
  app->ParaWin->EditFloat ("Blend"        , 7,1, 1.0f, 0.0f, 1.0f,0.01f);
  app->ParaWin->EditFloat ("Power"        , 8,1, 0.5f, 0.0f,16.0f,0.01f);
  app->ParaWin->EditFlags ("Wrap"         , 9,0,"off|on");
  app->ParaWin->EditFlags ("Buggy Gamma"  ,10,1,"correct gamma|buggy gamma");
}

void Edit_Bitmap_GlowRect(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("Center"       , 0,2,0.50f,-4.0f, 4.0f,0.001f);
  app->ParaWin->EditFloat ("Radius"       , 2,2,0.25f, 0.0f, 4.0f,0.001f);
  app->ParaWin->EditFloat ("Size"         , 4,2,0.00f, 0.0f, 4.0f,0.001f);
  app->ParaWin->EditRGBA  ("Color"        , 6,0xffffffff);
  app->ParaWin->EditFloat ("Blend"        , 7,1, 1.0f, 0.0f, 1.0f,0.01f);
  app->ParaWin->EditFloat ("Power"        , 8,1, 0.5f, 0.0f,16.0f,0.001f);
  app->ParaWin->EditFlags ("Wrap"         , 9,0,"off|on");
  app->ParaWin->EditFlags ("Buggy Gamma"  ,10,0,"correct gamma|buggy gamma");
}

void Edit_Bitmap_Dots(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditRGBA  ("Color 0"      , 0,0xff000000);
  app->ParaWin->EditRGBA  ("Color 1"      , 1,0xffffffff);
  app->ParaWin->EditInt   ("Count"        , 2,1,   16,    0,65536,0.25f);
  app->ParaWin->EditInt   ("Seed"         , 3,1,    0,    0,  255,0.25f);
}

void Edit_Bitmap_Blur(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFlags ("Mode"         , 0,0x12,"0|1|2|3|4|5:*4old paras|new paras");
  //app->ParaWin->EditFloat ("Blur"         , 1,2,0.015f, 0.0f, 4.0f,0.0001f);
  app->ParaWin->EditFloat ("Blur"         , 1,2,0.015f, 0.0f, 1.0f,0.0001f);
  app->ParaWin->EditFloat ("Amplify"      , 3,1,1.00f, 0.0f,16.0f,0.01f);
}

void Edit_Bitmap_Mask(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFlags ("Mode"         , 0,0,"mix|add|sub|mul");
}

void Edit_Bitmap_HSCB(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("Hue"          , 0,1,0.00f,-4.0f, 4.0f,0.001f);
  app->ParaWin->EditFloat ("Saturation"   , 1,1,1.00f,-16.0f,16.0f,0.001f);
  app->ParaWin->EditFloat ("Contrast"     , 2,1,1.00f, 0.0f, 16.0f,0.001f);
  app->ParaWin->EditFloat ("Brightness"   , 3,1,1.00f, 0.0f, 64.0f,0.001f);
}

void Edit_Bitmap_Rotate(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditCycle ("New Width"    , 6,0,"current|1|2|4|8|16|32|64|128|256|512|1024|2048");
  app->ParaWin->EditCycle ("New Height"   , 7,0,"current|1|2|4|8|16|32|64|128|256|512|1024|2048");
  app->ParaWin->EditFloat ("Angle"        , 0,1,0.00f,-4.0f, 4.0f,0.001f);
  app->ParaWin->EditFloat ("Zoom"         , 1,2,1.00f,-16.0f,16.0f,0.001f);
  app->ParaWin->EditFloat ("Scroll"       , 3,2,0.50f,-4.0f, 4.0f,0.001f);
  app->ParaWin->EditCycle ("Border"       , 5,0,"off|x|y|both");
}

void Edit_Bitmap_RotateMul(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("Angle"        , 0,1,0.00f,-4.0f, 4.0f,0.001f);
  app->ParaWin->EditFloat ("Zoom"         , 1,2,1.00f,-16.0f,16.0f,0.001f);
  app->ParaWin->EditFloat ("Scroll"       , 3,2,0.50f,-4.0f, 4.0f,0.001f);
  app->ParaWin->EditFlags ("Border"       , 5,0,"off|x|y|both");
  app->ParaWin->Space();
  app->ParaWin->EditRGBA  ("Pre-Adjust"   , 6,0xffffffff);
  app->ParaWin->EditFlags ("Mode"         , 7,0,"add|sub|mul|diff|alpha:*4linear|recursive");
  app->ParaWin->EditInt   ("Count"        , 8,1,    2,    1,  255,0.25f);
  app->ParaWin->EditRGBA  ("Fade"         , 9,0xffffffff);
}

void Edit_Bitmap_Twirl(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("Strength"     , 0,1,0.50f,-64.0f, 64.0f,0.001f);
  app->ParaWin->EditFloat ("Gamma"        , 1,1,2.00f,0.0f,64.0f,0.01f);
  app->ParaWin->EditFloat ("Radius"       , 2,2,0.25f, 0.0f, 4.0f,0.001f);
  app->ParaWin->EditFloat ("Center"       , 4,2,0.50f,-4.0f, 4.0f,0.001f);
  app->ParaWin->EditCycle ("Border"       , 6,0,"off|x|y|both");
}

void Edit_Bitmap_Distort(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("Strength"     , 0,1,0.02f,-4.0f, 4.0f,0.001f);
  app->ParaWin->EditCycle ("Border"       , 1,0,"off|x|y|both");
}

void Edit_Bitmap_Normals(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("Strength"     , 0,1,1.00f, 0.0f,16.0f,0.01f);
  app->ParaWin->EditCycle ("Mode"         , 1,1,"2d|3d|tangent 2d|tangent 3d");
}

void Edit_Bitmap_Light(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditCycle ("Mode"         , 0,0,"spot|point|directional");
  app->ParaWin->EditFloat ("Pos"          , 1,3,0.50f,-16.0f,16.0f,0.02f);
  app->ParaWin->EditFloat ("Dir"          , 4,2,0.125f, -4.0f, 4.0f,0.001f);
  app->ParaWin->EditRGBA  ("Diffuse"      , 6,0xffffffff);
  app->ParaWin->EditRGBA  ("Ambient"      , 7,0xffffffff);
  app->ParaWin->EditFloat ("Outer"        , 8,1,0.75f,  0.0f, 1.0f,0.001f);
  app->ParaWin->EditFloat ("Falloff"      , 9,1,1.00f,  0.0f,17.0f,0.001f);
  app->ParaWin->EditFloat ("Amplify"      ,10,1,0.50f, -4.0f, 4.0f,0.001f);
  if(op->SetDefaults)
    *op->Op.GetEditPtrF(5) = 0.250f;
}

void Edit_Bitmap_Bump(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditCycle ("Mode"         , 0,2,"spot|point|directional");
  app->ParaWin->EditFloat ("Pos"          , 1,3,0.50f,-16.0f,16.0f,0.02f);
  app->ParaWin->EditFloat ("Dir"          , 4,2,0.125f, -4.0f, 4.0f,0.001f);
  app->ParaWin->EditRGBA  ("Diffuse"      , 6,0xffffffff);
  app->ParaWin->EditRGBA  ("Ambient"      , 7,0xffffffff);
  app->ParaWin->EditFloat ("Outer"        , 8,1,0.75f,  0.0f, 4.0f,0.001f);
  app->ParaWin->EditFloat ("Falloff"      , 9,1,1.00f,  0.0f, 4.0f,0.001f);
  app->ParaWin->EditFloat ("Amplify"      ,10,1,0.50f, -4.0f, 4.0f,0.001f);
  app->ParaWin->EditGroup ("Specular");
  app->ParaWin->EditRGBA  ("Specular"     ,11,0xffffffff);
  app->ParaWin->EditFloat ("Power"        ,12,1,16.0f,  0.0f,  256,0.02f);
  app->ParaWin->EditFloat ("Amplify"      ,13,1,1.00f,  0.0f, 4.0f,0.001f);
  if(op->SetDefaults)
    *op->Op.GetEditPtrF(5) = 1.0f;
}

void Edit_Bitmap_Text(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("Position"     , 0,2,0.000f,-4.0f, 4.0f ,0.001f);
  app->ParaWin->EditFloat ("Height"       , 2,1,0.125f,-16.0f,16.0f,0.001f);
  app->ParaWin->EditFloat ("Line feed"    , 8,1,1.0f,-16.0f,16.0f,0.001f);
  app->ParaWin->EditFloat ("Width"        , 3,1,0.00f,-16.0f,16.0f,0.001f);
  app->ParaWin->EditRGBA  ("Color"        , 4,0xffffffff);
  app->ParaWin->EditFlags ("Center"       , 5,0,"|x:*1|y");
  app->ParaWin->EditFlags ("!Font Mode"   , 5,0,"*4off|page 1|page 2|page 3|page 4|page 5|page 6|page 7");
  app->ParaWin->AddBox(CMD_PARA_RELOAD ,6,0,"reload");
  app->ParaWin->EditFlags ("Store font in demo" , 5,0,"*7no|yes");
  app->ParaWin->EditSetGray((0x70&*op->Op.GetEditPtrU(5))==0);
  app->ParaWin->EditInt   ("external space",6,1,2,0,64,0.25f);
  app->ParaWin->EditFloat ("internal space",7,1,0.0f,-16.0f,16.0f,0.001f);
  app->ParaWin->EditSetGray(0);
  app->ParaWin->EditString("Font"         , 1);
  app->ParaWin->EditText  ("Text"         , 0);
  if(op->SetDefaults)
    op->Op.SetString(0,"hund.");
}

void Edit_Bitmap_Cell(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditCycle ("Width"        , 0,TEXTSIZE,"1|2|4|8|16|32|64|128|256|512|1024|2048");
  app->ParaWin->EditCycle ("Height"       , 1,TEXTSIZE,"1|2|4|8|16|32|64|128|256|512|1024|2048");
  app->ParaWin->EditRGBA  ("Color 0"      , 2,0xffff0000);
  app->ParaWin->EditRGBA  ("Color 1"      , 3,0xffffff00);
  app->ParaWin->EditInt   ("Max"          , 5,1,  255,    0,  255,0.25f);
  app->ParaWin->EditFloat ("Min Distance" ,10,1,0.125f,  0.0f, 1.0f,0.001f);
  app->ParaWin->EditInt   ("Seed"         , 6,1,    1,    0,  255,0.25f);
  app->ParaWin->EditFloat ("Amplify"      , 7,1,1.00f,  0.0f,16.0f,0.001f);
  app->ParaWin->EditFloat ("Gamma"        , 8,1,0.50f,  0.0f,16.0f,0.001f);
  app->ParaWin->EditFloat ("Aspect"       ,12,1,0.00f,-8.00f,+8.0f,0.01f);
  app->ParaWin->EditFlags ("Flags"        , 9,1,"inner|outer:*1|cell-color:*2|invert");
  app->ParaWin->EditGroup ("Cell-Color-Mode");
  app->ParaWin->EditRGBA  ("Color 2"      , 4,0xff000000);
  app->ParaWin->EditInt   ("Percentage"   ,11,1,    0,    0,  255,0.25f);
}

void Edit_Bitmap_Bricks(WerkkzeugApp *app,WerkOp *op)
{
  sControl *c0,*c1;

  app->ParaWin->EditOp(op);
  app->ParaWin->EditCycle ("Width"        , 0,TEXTSIZE,"1|2|4|8|16|32|64|128|256|512|1024|2048");
  app->ParaWin->EditCycle ("Height"       , 1,TEXTSIZE,"1|2|4|8|16|32|64|128|256|512|1024|2048");
  app->ParaWin->EditRGBA  ("Color 0"      , 2,0xffff0000);
  app->ParaWin->EditRGBA  ("Color 1"      , 3,0xffffff00);
  app->ParaWin->EditRGBA  ("Color Joints" , 4,0xff000000);
  c0 = app->ParaWin->EditFloat("Size Joints", 5,2,0.18f,    0,    1,0.001f);
  c1 = app->ParaWin->EditInt  ("Count"    , 7,2,   16,    1,  255,0.25f);
  app->ParaWin->EditInt   ("Seed"         , 9,1,    0,    0,  255,0.25f);
  app->ParaWin->EditInt   ("small ones"   ,10,1,   80,    0,  255,8);
  app->ParaWin->EditFlags ("Flags"        ,11,6,"*2|single small ones:*3|joints color");
  app->ParaWin->EditFlags ("Multiply"     ,11,6,"*4 1|2|4|8|16|32|64|128");
  app->ParaWin->EditFloat ("Side"         ,12,1,0.25f,    0,    1,0.01f);
  app->ParaWin->EditFloat ("Color Balance",13,1,1.00f,    0,   16,0.01f);
  if(!op->SetDefaults)
  {
    c0->Default[0] = 0.18f;
    c0->Default[1] = 0.28f;
    c1->Default[0] = 18;
    c1->Default[1] = 28;
  }
  else
  {
    *op->Op.GetEditPtrF(5) = 0.18f;
    *op->Op.GetEditPtrF(6) = 0.28f;
    *op->Op.GetEditPtrU(7) = 18;
    *op->Op.GetEditPtrU(8) = 28;
  }
}

void Edit_Bitmap_Wavelet(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditCycle ("Mode"         , 0,0,"encode|decode");
  app->ParaWin->EditInt   ("Max"          , 1,1,    2,    0,   16,0.25f);
}

void Edit_Bitmap_Gradient(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditCycle ("Width"        , 0,TEXTSIZE,"1|2|4|8|16|32|64|128|256|512|1024|2048");
  app->ParaWin->EditCycle ("Height"       , 1,TEXTSIZE,"1|2|4|8|16|32|64|128|256|512|1024|2048");
  app->ParaWin->EditRGBA  ("Color 0"      , 2,0xff000000);
  app->ParaWin->EditRGBA  ("Color 1"      , 3,0xffffffff);
  app->ParaWin->EditFloat ("Position"     , 4,1,0.00f,-16.0f,16.0f,0.02f);
  app->ParaWin->EditFloat ("Angle"        , 5,1,0.00f,-4.0f, 4.0f,0.001f);
  app->ParaWin->EditFloat ("Length"       , 6,1,1.00f,-16.0f,16.0f,0.001f);
  app->ParaWin->EditCycle ("Mode"         , 7,0,"gradient|gauss|sin");
}

void Edit_Bitmap_Sharpen(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditInt   ("Order"        , 0,1,    2,    1,    5,0.25f);
  app->ParaWin->EditFloat ("Blur"         , 1,2,0.005f, 0.0f, 4.0f,0.0001f);
  app->ParaWin->EditFloat ("Amplify"      , 3,1,1.00f,-8.0f, 8.0f,0.001f);
}

void Edit_Bitmap_Import(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFileName("Filename"     , 0);
  app->ParaWin->AddBox(CMD_PARA_RELOAD ,0,0,"reload");
}

void Edit_Bitmap_ColorBalance(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("Shadows"      , 0,3,0.0f,-1.0f,1.0f,0.001f);
  app->ParaWin->EditFloat ("Midtones"     , 3,3,0.0f,-1.0f,1.0f,0.001f);
  app->ParaWin->EditFloat ("Highlights"   , 6,3,0.0f,-1.0f,1.0f,0.001f);
}

void Edit_Bitmap_Unwrap(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFlags ("Mode"         , 0,0,"*0polar2normal|normal2polar|rect2normal");
  app->ParaWin->EditFlags ("Border"       , 0,0,"*4off|x|y|both");
}

void Edit_Bitmap_Bulge(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("Warp"         , 0,1,0.0f,-0.5f,16.0f,0.01f);
}

void Edit_Bitmap_Render(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditCycle ("Width"        , 0,TEXTSIZE,"1|2|4|8|16|32|64|128|256|512|1024|2048");
  app->ParaWin->EditCycle ("Height"       , 1,TEXTSIZE,"1|2|4|8|16|32|64|128|256|512|1024|2048");
}

void Edit_Bitmap_RenderAuto(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditCycle ("Width"        , 0,TEXTSIZE,"1|2|4|8|16|32|64|128|256|512|1024|2048");
  app->ParaWin->EditCycle ("Height"       , 1,TEXTSIZE,"1|2|4|8|16|32|64|128|256|512|1024|2048");
  app->ParaWin->EditFlags ("Dir"          , 2,1,"*0|left:*1|right:*2|bottom:*3|top");
  app->ParaWin->EditFlags ("Dir"          , 2,1,"*4|back:*5|front");
}


/****************************************************************************/

void Edit_Effect_Dummy(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditLink  ("Material"     , 0);
  app->ParaWin->EditFloat ("Size"         , 0,3, 1.0f,-256,256,0.01f);
}

void Edit_Effect_Print(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditLink  ("Material"     , 0);
  app->ParaWin->EditFlags ("Flags"       ,  0,16,"*8left|center x|right:*1|center y:*2|debug");
  app->ParaWin->EditFlags ("Font Mode"   ,  0,16,"*4off|page 1|page 2|page 3|page 4|page 5|page 6|page 7|page 8");
  app->ParaWin->EditFloat ("Size"         , 1,2, 1.0f,-256,256,0.01f);
  app->ParaWin->EditFloat ("Space"        , 3,2, 0.0f,-256,256,0.01f);
  app->ParaWin->EditFloat ("Radius"       , 5,1, 0.0f,-256,256,0.01f);
  app->ParaWin->EditText  ("Text"         , 0);
}

void Edit_Effect_Particles(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);

  app->ParaWin->EditGroup ("Movement");
  app->ParaWin->EditFlags ("Mode"         , 0,0,"*0|surface:*1|same speed:*3|speed from pos");
  app->ParaWin->EditFlags ("Mode"         , 0,0,":*4|one burst:*8|reset");
  app->ParaWin->EditInt   ("Max. Count"   , 1,1,64,1,0x1000,0.25f);
  app->ParaWin->EditFloat ("Lifetime"     , 2,1,1,0.01f,256,0.01f);
  app->ParaWin->EditFloat ("Rate"         , 3,1,1,0,0x10000,16);
  app->ParaWin->EditFloat ("Speed"        , 4,4, 0.0f, -256,  256,0.01f);
  app->ParaWin->EditFloat ("Rand Pos"     ,16,4, 0.0f, -256,  256,0.01f);
  app->ParaWin->EditFloat ("Rand Speed"   ,20,4, 0.0f, -256,  256,0.01f);


  app->ParaWin->EditGroup ("Look");
  app->ParaWin->EditFlags ("Mode"         , 0,0,"*2|fade color");
  app->ParaWin->EditLink  ("Material"     , 0);
  app->ParaWin->EditRGBA  ("Color 0"      , 8,0xffff0000);
  app->ParaWin->EditRGBA  ("Color 1"      , 9,0xff00ff00);
  app->ParaWin->EditRGBA  ("Color 2"      ,10,0xff0000ff);
  app->ParaWin->EditFloat ("Size 0-1-2"   ,11,3,0.125,0,256,0.01f);
  app->ParaWin->EditFloat ("Middle"       ,14,1,0.5f,0,1,0.01f);
  app->ParaWin->EditFloat ("Fade"         ,15,1,0.0f,0,0.5,0.01f);
}

void Edit_Effect_PartEmitter(WerkkzeugApp *app,WerkOp *op)
{
  sControl *con;
  app->ParaWin->EditOp(op);

  app->ParaWin->EditGroup ("Movement");
  app->ParaWin->EditInt   ("Particle Slot", 0,1,0,0,     15,0.25f);
  app->ParaWin->EditFlags ("Mode"         , 1,0,"*0|surface:*1|same speed:*3|speed from pos:*8|reset");
//  app->ParaWin->EditFlags ("Mode"         , 0,0,":*4|one burst:*8|reset");
  app->ParaWin->EditInt   ("Burst Count"  , 2,1,0,0,0x1000,0.25f);
  app->ParaWin->EditFloat ("Lifetime"     , 3,1,1,0.01f,256,0.01f);
  app->ParaWin->EditFloat ("*Rate"         , 4,1,16,0,0x10000,16);
  con=app->ParaWin->EditFloat ("Speed"        , 5,3, 0.0f, -256,  256,0.01f); if(con) con->LayoutInfo.x1-=2;
      app->ParaWin->EditFloat (":Speed Rot"   , 8,1, 0.0f, -256,  256,0.01f);
  con=app->ParaWin->EditFloat ("Rand Pos"     , 9,3, 0.0f, -256,  256,0.01f); if(con) con->LayoutInfo.x1-=2;
      app->ParaWin->EditFloat (":Rand Rot"    ,12,1, 0.0f, -256,  256,0.01f);
  con=app->ParaWin->EditFloat ("Rand Speed"   ,13,3, 0.0f, -256,  256,0.01f); if(con) con->LayoutInfo.x1-=2;
      app->ParaWin->EditFloat (":Rand SpeedR" ,16,1, 0.0f, -256,  256,0.01f);
}

void Edit_Effect_PartSystem(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);

  app->ParaWin->EditGroup ("Look");
  app->ParaWin->EditLink  ("Material"     , 0);
  app->ParaWin->EditInt   ("Particle Slot", 0,1,0,0,     15,0.25f);
  app->ParaWin->EditInt   ("Max Count"    , 1,1,1024,1,0x1000,0.25f);
  app->ParaWin->EditRGBA  ("*4Color 0"      , 2,0xffff0000);
  app->ParaWin->EditRGBA  ("*4Color 1"      , 3,0xff00ff00);
  app->ParaWin->EditRGBA  ("*4Color 2"      , 4,0xff0000ff);
  app->ParaWin->EditFloat ("*Size 0-1-2"   , 5,3,0.125,0,256,0.01f);
  app->ParaWin->EditFloat ("*Middle"       , 8,1,0.5f,0,1,0.01f);
  app->ParaWin->EditFloat ("*Fade"         , 9,1,0.0f,0,0.5,0.01f);
  app->ParaWin->EditFloat ("*Gravity"      ,10,1, 0.0f, -256,  256,0.01f);
  app->ParaWin->EditFlags ("Flags"         ,11,0,"camera align|world align");
}

void Edit_Effect_Chaos1(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditLink  ("Material"     , 0);
  app->ParaWin->EditFloat ("*A"            , 0,3, 0.0f,-256,256,0.001f);
  app->ParaWin->EditFloat ("*B"            , 3,3, 0.0f,-256,256,0.001f);
  app->ParaWin->EditFloat ("*C"            , 6,3, 0.0f,-256,256,0.001f);
  app->ParaWin->EditFlags ("Flags"         ,9,0,"|i:|j:|k:|l");
  app->ParaWin->EditInt   ("?EffektID"     ,10,1,0,0,255,0.25f);
  app->ParaWin->EditInt   ("Random Seed"   ,11,1,0,0,255,0.25f);
  app->ParaWin->EditInt   ("Count"         ,12,1,0,0,1024,0.25f);
}

void Edit_Effect_Tourque(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditLink  ("Material"       , 0);
  app->ParaWin->EditLink  ("Mesh"           , 1);
  app->ParaWin->EditInt   ("Slot"            ,14,1,0,0,15,0.25f);
  app->ParaWin->EditInt   ("Random Seed"     , 0,1,0,0,255,0.25f);
  app->ParaWin->EditInt   ("Count"           , 1,1,0,16,0xf000,0x100);
  app->ParaWin->EditInt   ("Rate"            , 2,1,0,0,1024,0.25f);
  app->ParaWin->EditFlags ("Flags"           , 3,0,"free|chain|cyclic:*4tri|quad:*5emit always|one burst");
  app->ParaWin->EditFloat ("Size (forw/side)", 4,2, 0.25f, 0.0f,16.0f,0.001f);
  app->ParaWin->EditFloat ("Speed"           , 6,1, 1.00f,-16.0f,16.0f,0.001f);
  app->ParaWin->EditFloat ("Damping"         , 7,1, 1.00f, 0.0f,1.0f,0.001f);
  app->ParaWin->EditFloat ("Pos"             , 8,3, 0.0f,-256,256,0.01f);
  app->ParaWin->EditFloat ("Range"           ,11,3, 0.01f,  -1,1,0.001f);
}

void Edit_Effect_Stream(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditLink  ("Material"       , 0);
  app->ParaWin->EditInt   ("Random Seed"     , 0,1,0,0,255,0.25f);
  app->ParaWin->EditInt   ("Count"           , 1,1,0,16,0xf000,0x100);
  app->ParaWin->EditInt   ("Rate"            , 2,1,0,0,1024,0.25f);
  app->ParaWin->EditFlags ("Flags"           , 3,0,"|nope");
  app->ParaWin->EditInt   ("Slot"            , 4,1,0,0,15,0.25f);
  app->ParaWin->EditFloat ("*Size (forw/side)", 5,2, 0.25f, 0.0f,16.0f,0.001f);
  app->ParaWin->EditFloat ("*Speed"           , 7,1, 1.00f,-16.0f,16.0f,0.001f);
  app->ParaWin->EditFloat ("*Pos A"           , 8,4, 0.0f,-256,256,0.01f);
  app->ParaWin->EditFloat ("*Pos B"           ,12,4, 0.0f,-256,256,0.01f);
  app->ParaWin->EditFloat ("*Pos C"           ,16,4, 0.0f,-256,256,0.01f);
  app->ParaWin->EditFloat ("*Pos D"           ,20,4, 0.0f,-256,256,0.01f);
  app->ParaWin->EditFloat ("*Stuff"           ,24,4, 0.0f,-256,256,0.01f);
  app->ParaWin->EditFloat ("*Light A"         ,28,4, 0.0f,-256,256,0.01f);
  app->ParaWin->EditFloat ("*Light B"         ,32,4, 0.0f,-256,256,0.01f);
}

void Edit_Effect_ImageLayer(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFileName("File name",0);
  app->ParaWin->AddBox(CMD_PARA_RELOAD,0,0,"reload");
  app->ParaWin->EditInt   ("Z-Index"          , 0,1, 0,0,255,0.25f);
  app->ParaWin->EditInt   ("2^(-1/atan(quantizer/Pi))", 1,1, 82,0,100,1);
  app->ParaWin->EditFloat ("*Fade"            , 2,1, 0.5f,0.0f,1.0f,0.01f);
  app->ParaWin->EditFlags ("Flags"            , 3,0,"|Force high quality");
}

void Edit_Effect_Billboards(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("Size"          , 0,1, 1.0f,-256,256,0.01f);
  app->ParaWin->EditFloat ("Aspect"        , 1,1, 1.0f,-256,256,0.01f);
  app->ParaWin->EditFloat ("Max Distance"  , 2,1,64.0f,1  ,4096,0.01f);
  app->ParaWin->EditFlags ("Flags"         , 3,0,"sprite|tree:*2show far|hide far");
  app->ParaWin->EditFlags ("Tile"          , 3,0,"*4no|2|4|8");
}

void Edit_Effect_WalkTheSpline(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("*Anim Position"   , 0,1, 1.0f,-16,16,0.01f);
  app->ParaWin->EditFloat ("*Anim Walk Cycle" , 1,1, 1.0f,-16,16,0.01f);
  app->ParaWin->EditInt ("*Count"             , 2,1,16.0f,0,0x4000,1);
  app->ParaWin->EditInt ("*Seed"              , 3,1, 0.0f,0,255,0.25f);
  app->ParaWin->EditFloat ("*Side"            , 4,1, 0.0f,-256,256,0.01f);
  app->ParaWin->EditFloat ("*Middle"          , 5,1, 0.0f,-256,256,0.01f);
}

void Edit_Effect_ChainLine(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditLink  ("Material"       , 0);
  app->ParaWin->EditGroup ("geometry");
  app->ParaWin->EditFloat ("*Pos A"            , 0,3, 0.0f,-4096,4096,0.01f);
  app->ParaWin->EditFloat ("*Pos B"            , 3,3, 0.0f,-4096,4096,0.01f);
  app->ParaWin->EditInt ("Marker A"            , 6,1,0,0,31,1);
  app->ParaWin->EditInt ("Marker B"            , 7,1,1,0,31,1);
  app->ParaWin->EditFloat ("Length"           , 8,1, 4.0f, 0,256,0.01f);
  app->ParaWin->EditFloat ("Thickness"        , 9,1, 0.125f,    0,1,0.00001f);
  app->ParaWin->EditGroup ("physics");
  app->ParaWin->EditFloat ("gravity"          ,10,1, 0.0033f,  0,1,0.00001f);
  app->ParaWin->EditFloat ("damping"          ,11,1, 0.9946f,  0,1,0.00001f);
  app->ParaWin->EditFloat ("spring"           ,12,1, 0.024f,   0,1,0.001f);
  app->ParaWin->EditFlags ("Collision"        ,16,1,"off|damping|reflecting");
  app->ParaWin->EditFlags ("Physic"           ,16,1,"*2|one-ended:*3|no simulation");
  app->ParaWin->EditGroup ("ripping");
  app->ParaWin->EditFloat ("rip distance"     ,13,1,16.0f, 0,256,0.01f);
  app->ParaWin->EditInt   ("rip pos"          ,20,1,16.0f, 0,31,0.25f);
  app->ParaWin->EditGroup ("wind");
  app->ParaWin->EditFloat ("*wind speed"      ,14,1, 0.002f,   0,1,0.00001f);
  app->ParaWin->EditFloat ("*wind force"      ,15,1, 0.0f,   0,64,0.01f);
  app->ParaWin->EditFloat ("*wind base"       ,17,3, 0.0f, -64,64,0.01f);



//  app->ParaWin->EditFloat ("hang position"    ,13,1, 0.5f,    0,1,0.01f);
//  app->ParaWin->EditFloat ("hang weigth"      ,13,1, 0.5f,    0,1,0.01f);
}

void Edit_Effect_LineShuffle(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditInt   ("Pass"             , 0,1, 1,0,31,0.125f);
}

void Edit_Effect_JPEG(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditInt   ("Pass"             , 0,1, 1,0,31,0.125f);
  app->ParaWin->EditFloat ("Downsample ratio" , 1,1, 1,1,255,0.125f);
  app->ParaWin->EditGroup ("JPEG Filter");
  app->ParaWin->EditInt   ("Iterations"       , 4,1, 0,0,4,0.125f);
  app->ParaWin->EditFloat ("Strength"         , 2,1, 1,-8,8,0.01f);
  app->ParaWin->EditCycle ("Filter mode"      , 3,0, "Blocky|Artifacts galore");
}

void Edit_Effect_BillboardField(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("*Size"         , 0,1, 1.0f,-256,256,0.01f);
  app->ParaWin->EditFloat ("Aspect"        , 1,1, 1.0f,-256,256,0.01f);
  app->ParaWin->EditFloat ("Max Distance"  , 2,1,32.0f,1,4096,0.01f);
  app->ParaWin->EditFlags ("Flags"         , 3,0,"sprite|tree:*2show far|hide far");
  app->ParaWin->EditFlags ("Tile"          , 3,0,"*4no|2|4|8");
  app->ParaWin->EditFloat ("*Rect0"        , 4,3, 0.0f,-4096,4096,0.01f);
  app->ParaWin->EditFloat ("*Rect1"        , 7,3, 0.0f,-4096,4096,0.01f);
  app->ParaWin->EditFloat ("*Anim"         ,10,1, 0.0f,-64,64,0.01f);
  app->ParaWin->EditInt   ("Count"         ,11,1,100,0,0x4000,10);
  app->ParaWin->EditInt   ("Seed"          ,12,1, 0 ,0, 255,0.125f);
  app->ParaWin->EditFloat ("Size Random"   ,13,1, 0 ,0, 256,0.01f);
  app->ParaWin->EditFloat ("Min Distance"  ,14,1, 0 ,0, 256,0.01f);
}

void Edit_Effect_BP06Spirit(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditLink  ("Part. / Main Material" , 0);
  app->ParaWin->EditLink  ("Core Material"         , 1);
  app->ParaWin->EditFloat ("*Pos"           , 0,3, 0.0f,-4096,4096,0.01f);
  app->ParaWin->EditFloat ("*Outline Radius", 3,1, 1.0f,0,256,0.01f);
  app->ParaWin->EditFloat ("*Core Radius"   , 4,1, 0.125f,0,256,0.01f);
  app->ParaWin->EditFloat ("*Perlin Freq"   , 5,1, 1.0f,-256,256,0.01f);
  app->ParaWin->EditFloat ("*Perlin Amp"    , 6,1, 0.250f,-256,256,0.01f);
  app->ParaWin->EditFloat ("*Perlin Anim"   , 7,1, 0.0f,-256,256,0.01f);
  app->ParaWin->EditRGBA  ("*Inner Color"   , 8,0xffffff00);
  app->ParaWin->EditRGBA  ("*Outline Color" , 9,0xff000000);
  app->ParaWin->EditRGBA  ("*Core Color"    ,10,0xff808080);

  app->ParaWin->EditInt   ("*Part. Count"   ,11,1,4096 ,0,0x4000,16);
  app->ParaWin->EditFloat ("*Part. Rad"     ,12,1,0.25f,-256,256,0.01f);
  app->ParaWin->EditFloat ("*Part. Speed"   ,13,1,0.25f,-256,256,0.01f);
  app->ParaWin->EditFloat ("*Part. Thick"   ,14,1, 0.0f,-256,256,0.01f);
  app->ParaWin->EditFloat ("*Part. Anim"    ,15,1, 0.0f,-256,256,0.01f);

  app->ParaWin->EditInt   ("Segments"       ,16,1, 32,0,256,0.01f);
}

void Edit_Effect_BP06Jungle(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditLink  ("Material"       , 0);
  app->ParaWin->EditInt   ("Segments"       , 0,1, 16,1,255,0.5f);
  app->ParaWin->EditInt   ("Slices"         , 1,1, 1,1,16,0.125f);
  app->ParaWin->EditFloat ("*Thickness"      , 2,1, 0.125f,0.0f,256.0f,0.001f);
  app->ParaWin->EditFloat ("*Thickness scale", 3,1, 0.9f,0.01f,1.0f,0.01f);
  app->ParaWin->EditFloat ("*Length"         , 4,1, 2.0f,0.0f,256.0f,0.001f);
  app->ParaWin->EditFloat ("*Length scale"   , 5,1, 0.9f,0.01f,1.0f,0.01f);
  app->ParaWin->EditFloat ("*Start angle"    , 6,1, 0.0f,-4.0f,4.0f,0.01f);
  app->ParaWin->EditFloat ("*Carrier freq"   , 7,1, 1.5f,-32.0f,32.0f,0.01f);
  app->ParaWin->EditFloat ("*Carrier amp"    , 8,1, 0.04f,-32.0f,32.0f,0.001f);
  app->ParaWin->EditFloat ("*Modulation freq", 9,1, 3.2f,-32.0f,32.0f,0.01f);
  app->ParaWin->EditFloat ("*Modulation amp" ,10,1, 0.1f,-4.0f,4.0f,0.001f);
  app->ParaWin->EditFloat ("*Modulation phase",11,1, 0.0f,-4.0f,4.0f,0.01f);
}

/****************************************************************************/

void Edit_Mesh_Cube(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditInt   ("Tesselate"    , 0,3,    1,    1,  255,0.25f);
  app->ParaWin->EditFlags ("Crease mode"  , 3,11,"smooth|faceted:*1normal uv|wraparound uv");
  app->ParaWin->EditFlags ("Origin"       , 3,11,"*2center|bottom");
  app->ParaWin->EditFlags ("Scale UV"     , 3,11,"*3no|yes");
  app->ParaWin->EditFlags ("Add Collision", 3,11,"*4no|add|sub:*6|only for shotsr");
  app->ParaWin->EditFloat ("Scale"        , 4,3, 1.0f,-4096, 4096,0.01f);
  app->ParaWin->EditFloat ("Rotate"       , 7,3, 0.0f,  -64,   64,0.002f);
  app->ParaWin->EditFloat ("Translate"    ,10,3, 0.0f,-4096, 4096,0.001f);
}

void Edit_Mesh_Cylinder(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditInt   ("Facets"       , 0,1,    8,    3,  255,0.25f);
  app->ParaWin->EditInt   ("Slices"       , 1,1,    1,    1,  255,0.25f);
  app->ParaWin->EditInt   ("Rings"        , 3,1,    1,    1,  255,0.25f);
  app->ParaWin->EditInt   ("Arc"          , 4,1,    0,    0,  255,0.25f);
  app->ParaWin->EditFlags ("Mode"         , 2,0,"closed|open");
  app->ParaWin->EditFlags ("Origin"       , 2,0,"*1center|bottom");
}

void Edit_Mesh_Torus(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditInt   ("Facets"       , 0,1,    8,    3,  255,0.25f);
  app->ParaWin->EditInt   ("Slices"       , 1,1,    4,    3,  255,0.25f);
  app->ParaWin->EditFloat ("Outer"        , 2,1, 0.5f,  -64,   64,0.002f);
  app->ParaWin->EditFloat ("Inner"        , 3,1, 0.125f,-64,   64,0.002f);
  app->ParaWin->EditFloat ("Phase"        , 4,1, 0.5f, -256,  256,0.05f);
  app->ParaWin->EditFloat ("Arclen"       , 5,1, 1.0f,  -64,   64,0.002f);
  app->ParaWin->EditFlags ("Radius mode"  , 6,1,"relative|absolute");
  app->ParaWin->EditFlags ("Origin"       , 6,0,"*1center|bottom");
}

void Edit_Mesh_Sphere(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditInt   ("Facets"       , 0,1,    8,    3,  255,0.25f);
  app->ParaWin->EditInt   ("Slices"       , 1,1,    4,    1,  255,0.25f);
}

void Edit_Mesh_SelectAll(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditMask  ("Mask"         , 0, 0x010100,24,"efv");
  app->ParaWin->EditCycle ("Mode"         , 1,2,"add|sub|set|set not");
}

void Edit_Mesh_SelectCube(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditMask  ("Mask"         , 0, 0x010100,24,"efv");
  app->ParaWin->EditFlags ("Mode"         , 1,2,"add|sub|set|set not:*2|face->vertex");
  app->ParaWin->EditFloat ("Center"       , 2,3, 0.0f, -4096, 4096,0.002f);
  app->ParaWin->EditFloat ("Size"         , 5,3, 1.25f,-4096, 4096,0.002f);
}

void Edit_Mesh_Subdivide(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditMask  ("Select"       , 0, 0,8,"f");
  app->ParaWin->EditFloat ("Alpha"        , 1,1, 1.0f,  -64,   64,0.002f);
  app->ParaWin->EditCycle ("Count"        , 2,1,"off|1|2|3|4|5|6|7|8");
}

void Edit_Mesh_Transform(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditMask  ("Select"       , 0, 0,8,"v");
  app->ParaWin->EditFloat ("Scale"        , 1,3, 1.0f,-4096, 4096,0.01f);
  app->ParaWin->EditFloat ("Rotate"       , 4,3, 0.0f,  -64,   64,0.002f);
  app->ParaWin->EditFloat ("Translate"    , 7,3, 0.0f,-4096, 4096,0.001f);
}

void Edit_Mesh_TransformEx(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditMask  ("Select"       , 0, 0,8,"v");
  app->ParaWin->EditFloat ("Scale"        , 1,3, 1.0f,-4096, 4096,0.01f);
  app->ParaWin->EditFloat ("Rotate"       , 4,3, 0.0f,  -64,   64,0.002f);
  app->ParaWin->EditFloat ("Translate"    , 7,3, 0.0f,-4096, 4096,0.001f);
  app->ParaWin->EditCycle ("Source"       ,10,5,"position|normal|tangent|color0|color1|uv0|uv1|uv2|uv3");
  app->ParaWin->EditCycle ("Dest"         ,11,5,"position|normal|tangent|color0|color1|uv0|uv1|uv2|uv3");
}

void Edit_Mesh_Crease(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditMask  ("Select"       , 0, 1,8,"x");
  app->ParaWin->EditCycle ("What"         , 1,0,"normal|tangent|color0|color1|uv0|uv1|uv2|uv3");
  app->ParaWin->EditCycle ("Type"         , 2,0,"border|edge|face");
}

void Edit_Mesh_UnCrease(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditMask  ("Select"       , 0, 0,8,"x");
  app->ParaWin->EditCycle ("What"         , 1,0,"normal|tangent|color0|color1|uv0|uv1|uv2|uv3");
  app->ParaWin->EditCycle ("Type"         , 2,0,"border|edge|face");
}

void Edit_Mesh_Triangulate(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditMask  ("Select"       , 0, 0,8,"f");
  app->ParaWin->EditInt   ("Treshold"     , 1,1,    4,    4,  16,0.25f);
  app->ParaWin->EditCycle ("Tesselation"  , 2,0,"neat|nasty");
}

void Edit_Mesh_Cut(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("Direction"    , 0,2, 0.0f,  -64,   64,0.001f);
  app->ParaWin->EditFloat ("Offset"       , 2,1, 0.0f,  -64,   64,0.001f);
  app->ParaWin->EditBool  ("Close"        , 3,0);
}

void Edit_Mesh_ExtrudeNormal(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditMask  ("Select"       , 0, 0,8,"v");
  app->ParaWin->EditFloat ("Distance"     , 1,1, 0.0f,  -64,   64,0.001f);
}

void Edit_Mesh_Displace(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditMask  ("Select"       , 0, 0,8,"v");
  app->ParaWin->EditFloat ("Amplify"      , 1,3, 1.0f,  -64,   64,0.001f);
}

void Edit_Mesh_Bevel(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditMask  ("Select"       , 0, 0,8,"f");
  app->ParaWin->EditFloat ("Elevation"    , 1,1, 0.05f, -64,   64,0.001f);
  app->ParaWin->EditFloat ("Pull"         , 2,1, 0.05f, -64,   64,0.001f);
  app->ParaWin->EditCycle ("Mode"         , 3,0,"normal|faces");
}

void Edit_Mesh_Perlin(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditMask  ("Select"       , 0, 0,8,"v");
  app->ParaWin->EditFloat ("*1Scale"       , 1,3, 1.0f,-4096, 4096,0.01f);
  app->ParaWin->EditFloat ("*2Rotate"      , 4,3, 0.0f,  -64,   64,0.002f);
  app->ParaWin->EditFloat ("*3Translate"   , 7,3, 0.0f,-4096, 4096,0.001f);
  app->ParaWin->EditFloat ("*Amplify"     ,10,3, 0.125f,-64,   64,0.001f);
}

void Edit_Mesh_Add(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
}

void Edit_Mesh_DeleteFaces(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditMask  ("Select"       , 0, 1,8,"f");
}

void Edit_Mesh_SelectRandom(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditMask  ("Mask"         , 0, 0x010100,24,"efv");
  app->ParaWin->EditCycle ("Mode"         , 1,2,"add|sub|set|set not");
  app->ParaWin->EditInt   ("Ratio"        , 2,1,  128,    0,  255,0.25f);
  app->ParaWin->EditInt   ("Seed"         , 3,1,    0,    0,  255,0.25f);
}

void Edit_Mesh_Multiply(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("Local Rotate" ,13,3, 0.0f,  -64,   64,0.002f);
  app->ParaWin->EditFloat ("Scale"        , 0,3, 1.0f,-4096, 4096,0.01f);
  app->ParaWin->EditFloat ("Rotate"       , 3,3, 0.0f,  -64,   64,0.002f);
  app->ParaWin->EditFloat ("Translate"    , 6,3, 0.0f,-4096, 4096,0.001f);
  app->ParaWin->EditFloat ("ExtrudeNormal",16,1, 0.0f, -256,  256,0.001f);
  app->ParaWin->EditInt   ("Count"        , 9,1,    2,    1,  255,0.25f);
  app->ParaWin->EditFlags ("Mode"         ,10,0, "|trans uv:*1|random");
  app->ParaWin->EditFloat ("Trans UV"     ,11,2, 0.0f,  -64,   64,0.01f);
}

void Edit_Mesh_BeginRecord(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
}

void Edit_Mesh_Extrude(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditMask  ("Select"       , 0,1,  8,"f");
  app->ParaWin->EditMask  ("OutMask"      , 1,0, 16,"fv");
  app->ParaWin->EditFlags ("Mode"         , 2,2,  "normal|faces:*1individual normal|average normal|direction|face normal:*3scale global|scale local:*4sel add|sel sub|sel set|sel set not");
  app->ParaWin->EditInt   ("Count"        , 3,1,    1,    1,  255,0.125f);
  app->ParaWin->EditFloat ("Distance/Dir" , 4,3, 0.125f,-4096, 4096,0.001f);
  app->ParaWin->EditFloat ("Scale"        , 7,3, 1.0f,  -64,   64,0.01f);
  app->ParaWin->EditFloat ("Rotation"     ,10,3, 0.0f,  -64,   64,0.002f);
}

void Edit_Mesh_Bend(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditMask  ("Select"       , 0,0,  8,"v");
  app->ParaWin->EditFloat ("Scale 1"      ,10,3, 1.0f,-4096, 4096,0.01f);
  app->ParaWin->EditFloat ("Rotate 1"     ,13,3, 0.0f,  -64,   64,0.002f);
  app->ParaWin->EditFloat ("Translate 1"  ,16,3, 0.0f,-4096, 4096,0.001f);
  app->ParaWin->EditFloat ("Scale 2"      , 1,3, 1.0f,-4096, 4096,0.01f);
  app->ParaWin->EditFloat ("Rotate 2"     , 4,3, 0.0f,  -64,   64,0.002f);
  app->ParaWin->EditFloat ("Translate 2"  , 7,3, 0.0f,-4096, 4096,0.001f);
  app->ParaWin->EditFloat ("Direction"    ,19,2, 0.0f,  -64,   64,0.002f);
  app->ParaWin->EditFloat ("Axial range"  ,21,2, 0.0f,-4096, 4096,0.01f);
  app->ParaWin->EditFlags ("Mode"         ,23,2,  "*2straight|tent:*0linear|smooth:*1|auto range");
  if(op->SetDefaults)
  {
    *op->Op.GetEditPtrF(21) = -1.0f;
    *op->Op.GetEditPtrF(22) = 1.0f;
  }
}

void Edit_Mesh_CollisionCube(WerkkzeugApp *app,WerkOp *op)
{
  sControl *c0,*c1,*c2;
  app->ParaWin->EditOp(op);
  c0 = app->ParaWin->EditFloat ("X"            , 0,2, -0.5f, -256, 256,0.01f);
  c1 = app->ParaWin->EditFloat ("Y"            , 2,2,  0.0f, -256, 256,0.01f);
  c2 = app->ParaWin->EditFloat ("Z"            , 4,2, -0.5f, -256, 256,0.01f);
  app->ParaWin->EditFlags ("!Mode"        , 6,0, "add|sub|zone:*2|only for shots:*4cube|cylinder");
  app->ParaWin->EditInt   ("Tesselation (careful!)"  , 7,3,  1,1,255,0.125f);
  app->ParaWin->EditGroup ("Zone Effects");
  app->ParaWin->EditSetGray((3&*op->Op.GetEditPtrU(6))!=2);
  app->ParaWin->EditFlags ("When"         ,10,0,"*4never|enter|leave|collect|respawn soon|respawn 2|respawn 3|respawn late|use|hit|always|inside|hit or enter");
  app->ParaWin->EditFlags ("!What"        ,10,0,"nop|enable switch|disable switch|toggle switch|set switch|set value|max value|min value|add value|sub value|jump-pad|player-respawn");
  switch(15&*op->Op.GetEditPtrU(10))
  {
  default:
    app->ParaWin->EditSetGray(1);
    app->ParaWin->EditInt   ("Condition Switch" ,11,1,0,0,255,0.25f);
    app->ParaWin->EditInt   ("Output Switch"    ,13,1,0,0,255,0.25f);
    app->ParaWin->EditInt   ("Value"            ,12,1,0,0,0x4000,0.25f);
    break;
  case KKEC_ENABLESWITCH:
  case KKEC_DISABLESWITCH:
  case KKEC_TOGGLESWITCH:
  case KKEC_SETSWITCH:
    app->ParaWin->EditInt   ("Condition Switch" ,11,1,0,0,255,0.25f);
    app->ParaWin->EditInt   ("Output Switch"    ,13,1,0,0,255,0.25f);
    app->ParaWin->EditSetGray(1);
    app->ParaWin->EditInt   ("Value"            ,12,1,0,0,0x4000,0.25f);
    break;
  case KKEC_SETVALUE:
  case KKEC_MAXVALUE:
  case KKEC_MINVALUE:
  case KKEC_INCVALUE:
  case KKEC_DECVALUE:
    app->ParaWin->EditInt   ("Condition Switch" ,11,1,0,0,255,0.25f);
    app->ParaWin->EditFlags ("Output"           ,13,0,"Life|Armor|Alpha|Beta|Ammo 0|Ammo 1|Ammo 2|Ammo 3|Weapon 0|Weapon 1|Weapon 2|Weapon 3|Weapon 4|Weapon 5|Weapon 6|Weapon 7");
    app->ParaWin->EditInt   ("Value"            ,12,1,0,0,0x4000,0.25f);
    break;
  case KKEC_JUMP:
    app->ParaWin->EditInt   ("Condition Switch" ,11,1,0,0,255,0.25f);
    app->ParaWin->EditSetGray(1);
    app->ParaWin->EditFlags ("Output"           ,13,0,"Life|Armor|Alpha|Beta|Ammo 0|Ammo 1|Ammo 2|Ammo 3|Weapon 0|Weapon 1|Weapon 2|Weapon 3|Weapon 4|Weapon 5|Weapon 6|Weapon 7");
    app->ParaWin->EditSetGray((3&*op->Op.GetEditPtrU(6))!=2);
    app->ParaWin->EditInt   ("Value"            ,12,1,0,0,0x4000,0.25f);
    break;
  }
  app->ParaWin->EditSetGray((3&*op->Op.GetEditPtrU(6))!=2);
  app->ParaWin->EditLink("Item Event",0);
  app->ParaWin->EditLink("Collect Event",1);

  app->ParaWin->EditSetGray(0);
  app->ParaWin->DefaultF2(0,c0,-0.5f,0.5f);
  app->ParaWin->DefaultF2(2,c1, 0.0f,1.0f);
  app->ParaWin->DefaultF2(4,c2,-0.5f,0.5f);
}

void Edit_Mesh_Grid(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFlags ("Mode"         , 0,3,  "1sided|2sided");
  app->ParaWin->EditInt   ("Tesselation"  , 1,2,  1,1,255,0.125f);
}

void Edit_Mesh_Lightmap(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("Resolution"   , 0,1,  4.0f,0.1f,12.0f,0.01f);
  app->ParaWin->EditFloat ("Extend"       , 1,3,  1.0f,0.0f,100.0f,0.01f);
}

/*void Edit_Mesh_MapLight(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("Position"     , 0,3, 0.0f, -256,  256,0.001f);
  app->ParaWin->EditFloat ("Range"        , 3,1,10.0f,  0.0f,  64,0.001f);
  app->ParaWin->EditRGBA  ("Color"        , 4,0xffffffff);
}*/

void Edit_Mesh_SelectLogic(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditMask  ("Source 1"     , 0,0, 24,"efv");
  app->ParaWin->EditMask  ("Source 2"     , 1,0, 24,"efv");
  app->ParaWin->EditMask  ("Destination"  , 2,0, 24,"efv");
  app->ParaWin->EditFlags ("Operation"    , 3,0,"*4(|not (:*2(1)|(not 1):*0or|and|eor|select1:*3(2))|(not 2))");
  app->ParaWin->EditFlags ("Post-XForm"   , 3,0,"*5|face->vertex");
}

void Edit_Mesh_SelectGrow(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditMask  ("Source"       , 0,1, 8,"f");
  app->ParaWin->EditMask  ("Destination"  , 1,0x0101, 16,"fv");
  app->ParaWin->EditCycle ("Mode"         , 2,2,"add|sub|set|set not");
  app->ParaWin->EditInt   ("Amount"       , 3,1, 1,0,255,0.125f);
}

void Edit_Mesh_Invert(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
}

void Edit_Mesh_SetPivot(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditCycle ("Attribute"    , 0,0,"position|normal|tangent|color0|color1|uv0|uv1|uv2|uv3");
  app->ParaWin->EditFloat ("Pivot"        , 1,3, 0.0f,-4096, 4096,0.001f);
}

void Edit_Mesh_UVProjection(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditMask  ("Mask"         , 0,0, 8,"f");
  app->ParaWin->EditFloat ("Scale"        , 1,3, 1.0f,-4096, 4096,0.01f);
  app->ParaWin->EditFloat ("Rotate"       , 4,3, 0.0f,  -64,   64,0.002f);
  app->ParaWin->EditFloat ("Translate"    , 7,3, 0.0f,-4096, 4096,0.001f);
  app->ParaWin->EditCycle ("Projection"   ,10,0,"cubic|cylindrical|spherical|3-plane");
}

void Edit_Mesh_Center(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditMask  ("Mask"         , 0,0, 8,"v");
  app->ParaWin->EditFlags ("Which"        , 1,7, "|x:*1|y:*2|z");
  app->ParaWin->EditFlags ("Operation"    , 1,7, "*7center|on ground");
}

void Edit_Mesh_AutoCollision(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("Enlarge"      , 0,1,0.0f,-256.0f,256.0f,0.001f);
  app->ParaWin->EditFlags ("Mode"         , 1,0, "add|sub:*2|only for shots:*4cube|cylinder");
  app->ParaWin->EditInt   ("Tesselation (careful!)"  , 2,3,  1,1,255,0.125f);
  app->ParaWin->EditMask  ("Output sel"   , 5,0, 8,"v");
}

void Edit_Mesh_SelectSphere(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditMask  ("Mask"         , 0, 0x010100,24,"efv");
  app->ParaWin->EditFlags ("Mode"         , 1,2,"add|sub|set|set not:*2|face->vertex");
  app->ParaWin->EditFloat ("Center"       , 2,3, 0.0f,-4096, 4096,0.002f);
  app->ParaWin->EditFloat ("Size"         , 5,3, 1.25f,-4096, 4096,0.002f);
}

void Edit_Mesh_SelectFace(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditMask  ("Mask"         , 0, 0x01,8,"f");
  app->ParaWin->EditFlags ("Mode"         , 1,2,"add|sub|set|set not");
  app->ParaWin->EditInt   ("Face #"       , 2,1, 0,0,16777215,0.5f);
}

void Edit_Mesh_SelectAngle(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditMask  ("Mask"         , 0, 0x010100,24,"efv");
  app->ParaWin->EditFlags ("Mode"         , 1,2,"add|sub|set|set not:*2|face->vertex");
  app->ParaWin->EditFloat ("Direction"    , 2,2, 0.0f,  -64,   64,0.001f);
  app->ParaWin->EditFloat ("Threshold"    , 4,1, 0.95f,0.0f, 1.0f,0.001f);
}

void Edit_Mesh_Bend2(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("Center"       , 0,3, 0.0f,-4096, 4096,0.002f);
  app->ParaWin->EditFloat ("Rotate"       , 3,3, 0.0f,  -64,   64,0.001f);
  app->ParaWin->EditFloat ("Length"       , 6,1, 1.0f,    0, 4096,0.002f);
  app->ParaWin->EditFloat ("Angle"        , 7,1,0.25f,  -64,   64,0.001f);
}

void Edit_Mesh_SmoothAngle(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("Threshold"    , 0,1, 0.9f, 0.0f, 1.0f,0.001f);
}

void Edit_Mesh_Color(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("Position"     , 0,3, 0.0f,-4096, 4096,0.002f);
  app->ParaWin->EditFloat ("Direction"    , 3,2, 0.0f,  -64,   64,0.001f);
  app->ParaWin->EditRGBA  ("Color"        , 5,0xffffffff);
  app->ParaWin->EditFloat ("Amplify"      , 6,1, 1.0f,  -64,   64,0.01f);
  app->ParaWin->EditFloat ("Range"        , 7,1, 8.0f,    0,  4096,0.01f);
  app->ParaWin->EditCycle ("Operation"    , 8,1, "Light Dir|Light Point|Render Slots|Ambient");
}

void Edit_Mesh_BendS(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("Anchor"       , 0,3, 0.0f,-4096, 4096,0.002f);
  app->ParaWin->EditFloat ("Rotate"       , 3,3, 0.0f,  -64,   64,0.001f);
  app->ParaWin->EditFloat ("Length"       , 6,1, 1.0f,    0, 4096,0.002f);
  app->ParaWin->EditSpline("Spline" , 0);
}

void Edit_Mesh_LightSlot(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("Position"     , 0,3, 0.0f,-4096, 4096,0.002f);
  app->ParaWin->EditRGBA  ("Color"        , 3,0xffffffff);
  app->ParaWin->EditFloat ("Amplify"      , 4,1, 1.0f,  -64,   64,0.01f);
  app->ParaWin->EditFloat ("Range"        , 5,1, 8.0f,    0, 4096,0.01f);
}

void Edit_Mesh_ShadowEnable(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditCycle ("Operation"    , 0,0, "Disable|Enable");
}

void Edit_Mesh_XSI(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditString("Filename"     , 0);
}

void Edit_Mesh_SingleVert(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
}

void Edit_Mesh_Multiply2(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditInt   ("Seed"         , 0,1,0,0,255,0.25f);
  app->ParaWin->EditInt   ("Emphasize last",13,1,0,0,255,0.25f);
  app->ParaWin->EditInt   ("Count"        , 1,3,1,1,255,0.25f);
  app->ParaWin->EditFloat ("Translate"    , 4,3,0.0f,-4096.0f,4096.0f,0.001f);
  app->ParaWin->EditInt   ("Count"        , 7,3,1,1,255,0.25f);
  app->ParaWin->EditFloat ("Translate"    ,10,3,0.0f,-4096.0f,4096.0f,0.001f);
  app->ParaWin->EditInt   ("Count"        ,14,3,1,1,255,0.25f);
  app->ParaWin->EditFloat ("Translate"    ,17,3,0.0f,-4096.0f,4096.0f,0.001f);
}

void Edit_Mesh_ToMin(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
}

/****************************************************************************/

void Edit_Mesh_MatLink(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFlags ("Select"       , 0,0,"all|selected|unselected");
  app->ParaWin->EditLink  ("Material"     , 0);
  app->ParaWin->EditInt   ("Renderpass"   , 1,1,0,0,31,0.25f);
}

void Edit_Material_Material(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);

  app->ParaWin->EditGroup ("General");
  app->ParaWin->EditFlags ("!Light flags"   ,48,0x00020b,"*1|Light:*2|Cast Shadow:*3|Texture:*4|Envi");
  app->ParaWin->EditInt   ("Renderpass"     ,49,1,0,0,  31,0.25f);

  sU32 multiFlags = *op->Op.GetEditPtrU(48);

  if(*op->Op.GetEditPtrU(48) & 0x1e)
  {
    app->ParaWin->EditGroup ("Lit material");
    app->ParaWin->EditFlags ("!Special"     ,48,0x00020b,"*8|envi-reflection:*9|specular:*13|specular map:*10|alpha test");
    if(multiFlags & 0x10)
      app->ParaWin->EditFlags (""           ,48,0x00020b,"*14|env alpha mask:*15|env opaque");

    if(multiFlags & 0x02)
    {
      if(multiFlags & 0x200)
      {
        app->ParaWin->EditFloat ("*Spec Power"  ,51, 1,32.0f,0,256,0.1f);
        app->ParaWin->EditRGBA  ("*4SpecA/DiffuseRGB",52, 0xffffffff);
      }
      else
        app->ParaWin->EditRGBA  ("*4Diffuse Color",52, 0xffffffff);

      app->ParaWin->EditRGBA  ("*4Ambient Color",53, 0xff000000);

      app->ParaWin->EditLink  ("Bump Texture" , 4);
      app->ParaWin->EditFlags ("Bump Flags"   ,56,0x000002, "*8tile|clamp:*0no filter|filter|mipmap|aniso|mip nofilter|bilinear:*12untransformed|scaled by ->|trans1|trans2");
      app->ParaWin->EditFloat (":*ScaleUV Bump",60, 1,1,-64,64,0.01f);
    }

    if(multiFlags & 0x10)
    {
      app->ParaWin->EditLink  ("Envi"         , 6);
      app->ParaWin->EditFlags ("Envi Flags"   ,58,0x000002, "*8tile|clamp:*0no filter|filter|mipmap|aniso|mip nofilter|bilinear:*12untransformed|scaled by ->|trans1|trans2");
      app->ParaWin->EditFloat (":*ScaleUV Envi",62, 1,1,-64,64,0.01f);
    }
  }
  else
  {
    app->ParaWin->EditGroup ("Unlit material");

    app->ParaWin->EditFlags ("Base Flags"   , 0,0x000300,"*0|alphatest:*1|doublesided:*2|invertcull:*6|fog");
    app->ParaWin->EditFlags ("Base Flags"   , 0,0x000300,"*8zoff|zwrite|zread|zon:*12opaque|alpha|add|mul|mul2|smooth|sub|invdestmul2");
    app->ParaWin->EditFlags ("Special"      , 1,0x000000,"*4no envi|sphere envi|reflect envi:*8no proj|model proj|world proj|eye proj");
    app->ParaWin->EditFlags ("Phase insert" ,48,0x00020b,"*20default|base|pre-light|shadow|light|post-light|post-light 2|other");
    app->ParaWin->EditFlags ("Finalizer"    ,48,0x00020b,"*16normal|sprites|thick lines");
    app->ParaWin->EditFloat ("*Size"        ,60, 1,1,-64,64,0.001f);
    app->ParaWin->EditFloat ("*Aspect"      ,62, 1,1,-64,64,0.001f);
  }

  if(op->SetDefaults)
    *op->Op.GetEditPtrF(62) = 1.0f;

  if(!(multiFlags & 0x1e) || (multiFlags & 0x08))
  {
    app->ParaWin->EditGroup ("Combiner");

#define ALPHA "|(unused)|(unused)|(unused)|tex1|tex2|tex3|tex4|color1|color2|color3|color4|0.5|0|2"

    app->ParaWin->AddSpecial("Auto-Combiner",0);
    app->ParaWin->EditFlags ("!Color 1"     , 9,0x000010, "*0(no alpha fade)"ALPHA":*4nop|set|add|sub|mul|mul2|mul4|mul8|smooth|lerp");
    app->ParaWin->EditFlags ("!Color 2"     ,10,0x000000, "*0(no alpha fade)"ALPHA":*4nop|set|add|sub|mul|mul2|mul4|mul8|smooth|lerp");
    app->ParaWin->EditFlags ("!Tex 1"       ,11,0x000000, "*0(no alpha fade)"ALPHA":*4nop|set|add|sub|mul|mul2|mul4|mul8|smooth|lerp");
    app->ParaWin->EditFlags ("!Tex 2"       ,12,0x000000, "*0(no alpha fade)"ALPHA":*4nop|set|add|sub|mul|mul2|mul4|mul8|smooth|lerp");
    app->ParaWin->EditFlags ("!Color 3"     ,15,0x000000, "*0(no alpha fade)"ALPHA":*4nop|set|add|sub|mul|mul2|mul4|mul8|smooth|lerp");
    app->ParaWin->EditFlags ("!Color 4"     ,16,0x000000, "*0(no alpha fade)"ALPHA":*4nop|set|add|sub|mul|mul2|mul4|mul8|smooth|lerp");
    app->ParaWin->EditFlags ("!Tex 3/Envi"  ,17,0x000000, "*0(no alpha fade)"ALPHA":*4nop|set|add|sub|mul|mul2|mul4|mul8|smooth|lerp");
    app->ParaWin->EditFlags ("!Tex 4/Proj"  ,18,0x000000, "*0(no alpha fade)"ALPHA":*4nop|set|add|sub|mul|mul2|mul4|mul8|smooth|lerp");
    app->ParaWin->EditFlags ("Vertex Color" ,19,0x000000, "*0(no alpha fade)"ALPHA":*4nop|set|add|sub|mul|mul2|mul4|mul8|smooth|lerp");
    app->ParaWin->EditFlags ("Alpha A*B"    ,23,0x000000, "*8|invert:*0 (unused)"ALPHA":"
                                                        "*9|invert:*4 (unused)|light|(unused)|(unused)|tex1|tex2|tex3|tex4");
#undef ALPHA

    app->ParaWin->EditGroup ("Combiner Input");

    if(*op->Op.GetEditPtrU(11) || op->Op.GetLink(0))
    {
      app->ParaWin->EditLink  ("Texture 1      " ,  0);
      app->ParaWin->EditFlags ("!Tex 1 Flags"     ,  4,0x000002, "*8tile|clamp:*0no filter|filter|mipmap|aniso|mip nofilter|bilinear:*12untransformed|scaled by ->|trans1|trans2:*4uv0|uv1");
      if(((*op->Op.GetEditPtrU(4))&sMTF_TRANSMASK)==sMTF_SCALE)
        app->ParaWin->EditFloat ("*ScaleUV Tex1"  , 24, 1,1,-64,64,0.01f);
    }

    if(*op->Op.GetEditPtrU(12) || op->Op.GetLink(1))
    {
      app->ParaWin->EditLink  ("Texture 2      " ,  1);
      app->ParaWin->EditFlags ("!Tex 2 Flags"     ,  5,0x000002, "*8tile|clamp:*0no filter|filter|mipmap|aniso|mip nofilter|bilinear:*12untransformed|scaled by ->|trans1|trans2:*4uv0|uv1");
      if(((*op->Op.GetEditPtrU(5))&sMTF_TRANSMASK)==sMTF_SCALE)
        app->ParaWin->EditFloat ("*ScaleUV Tex2"  , 25, 1,1,-64,64,0.01f);
    }
    
    if(*op->Op.GetEditPtrU(17) || op->Op.GetLink(2))
    {
      app->ParaWin->EditLink  ("Texture 3/Envi " ,  2);
      app->ParaWin->EditFlags ("!Tex 3 Flags"     ,  6,0x000002, "*8tile|clamp:*0no filter|filter|mipmap|aniso|mip nofilter|bilinear:*12untransformed|scaled by ->|trans1|trans2:*4uv0|uv1");
      if(((*op->Op.GetEditPtrU(6))&sMTF_TRANSMASK)==sMTF_SCALE)
        app->ParaWin->EditFloat ("*ScaleUV Tex3"  , 26, 1,1,-64,64,0.01f);
    }
    
    if(*op->Op.GetEditPtrU(18) || op->Op.GetLink(3))
    {
      app->ParaWin->EditLink  ("Texture 4/Proj " ,  3);
      app->ParaWin->EditFlags ("!Tex 4 Flags"     ,  7,0x000002, "*8tile|clamp:*0no filter|filter|mipmap|aniso|mip nofilter|bilinear:*12untransformed|scaled by ->|trans1|trans2:*4uv0|uv1");
      if(((*op->Op.GetEditPtrU(7))&sMTF_TRANSMASK)==sMTF_SCALE)
        app->ParaWin->EditFloat ("*ScaleUV Tex4"  , 27, 1,1,-64,64,0.01f);
    }

    if(*op->Op.GetEditPtrU(9))
      app->ParaWin->EditRGBA  ("*4Color 1"        , 28, 0xff000000);

    if(*op->Op.GetEditPtrU(10))
      app->ParaWin->EditRGBA  ("*4Color 2"        , 29, 0xffffffff);

    if(*op->Op.GetEditPtrU(15))
      app->ParaWin->EditRGBA  ("*4Color 3"        , 30, 0xff000000);

    if(*op->Op.GetEditPtrU(16))
      app->ParaWin->EditRGBA  ("*4Color 4"        , 31, 0xff000000);
  }

  app->ParaWin->EditGroup ("UV-Transformation");

  app->ParaWin->EditFloat ("*1trans1 scale",34, 3, 1.0f,  -64,   64,0.01f);
  app->ParaWin->EditFloat ("*2trans1 rot"  ,37, 3, 0.0f,  -64,   64,0.002f);
  app->ParaWin->EditFloat ("*3trans1 trans",40, 3, 0.0f,  -64,   64,0.002f);
  app->ParaWin->EditFloat ("*1trans2 scale",43, 2, 1.0f,  -64,   64,0.01f);
  app->ParaWin->EditFloat ("*2trans2 rot"  ,45, 1, 0.0f,  -64,   64,0.002f);
  app->ParaWin->EditFloat ("*3trans2 trans",46, 2, 0.0f,  -64,   64,0.002f);

  // böse falle: die default-parameter werden nur gesetzt wenn die EditFlag() 
  // auch ausgeführt werden, das passiert aber nur wenn eine textur gesetzt
  // ist, also beim default-initialisieren NIE.

  if(op->SetDefaults)
  {
    *op->Op.GetEditPtrU(4) = sMTF_MIPMAPS;
    *op->Op.GetEditPtrU(5) = sMTF_MIPMAPS;
    *op->Op.GetEditPtrU(6) = sMTF_MIPMAPS;
    *op->Op.GetEditPtrU(7) = sMTF_MIPMAPS;
  }
}

static sBool MakeDependendLink(WerkkzeugApp *app,WerkOp *op,const sChar *label,sInt link,sInt flag) // this uses bits 24:25 of the flag! (and better reserve 26)
{
  sBool isinput =  ((*op->Op.GetEditPtrU(flag))&0x03000000)?1:0;
  app->ParaWin->EditSetGray(isinput);
  app->ParaWin->EditLinkI(label,link,flag);
  app->ParaWin->EditSetGray(0);
  return (op->LinkName[link][0] || isinput);
}

void Edit_Material_Material20(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);

  app->ParaWin->EditGroup ("General section");

  app->ParaWin->EditFlags ("Flags"            , 0, 0x10,"*0|specular:*1|paralax:*2|smooth light:*3|vertex color");
  app->ParaWin->EditFlags (""                 , 0, 0x10,"*4|shadows:*5|renorm bump:*6|x2 intensity:*7|specularity map");
  app->ParaWin->EditRGBA  ("Diffuse"          , 1, 0xffffffff);
  app->ParaWin->EditRGBA  ("Specular"         , 2, 0xffffffff);
  app->ParaWin->EditFloat ("Specular power"   , 3, 1,16.0f,1.0f,1024.0f,0.01f);
  app->ParaWin->EditFloat ("Paralax strength" , 4, 1,0.01f,0.0f,2.0f,0.001f);

  app->ParaWin->EditGroup ("Texture section");

  if(MakeDependendLink(app,op,"Main",0,8))
  {
    app->ParaWin->EditFlags ("!  Flags"         , 8,0x000002, "*8tile|clamp:*0no filter|filter|mipmap|trilin|aniso:*4uv0|uv1:*12untransformed|scaled|trans1|trans2");
    app->ParaWin->EditFloat ("*  ScaleUV"       ,12, 1,1,-64,64,0.01f);
  }

  if(MakeDependendLink(app,op,"Detail 1",1,9))
  {
    app->ParaWin->EditFlags ("!  Flags"         , 9,0x000002, "*8tile|clamp:*0no filter|filter|mipmap|trilin|aniso:*4uv0|uv1:*12untransformed|scaled|trans1|trans2");
    app->ParaWin->EditFlags ("  Op"             , 9,0x000002, "*16mul|mul2|mul4|add|addsmooth");
    app->ParaWin->EditFloat (":  ScaleUV"       ,13, 1,1,-64,64,0.01f);
  }

  if(MakeDependendLink(app,op,"Detail 2",2,10))
  {
    app->ParaWin->EditFlags ("!  Flags"         ,10,0x000002, "*8tile|clamp:*0no filter|filter|mipmap|trilin|aniso:*4uv0|uv1:*12untransformed|scaled|trans1|trans2");
    app->ParaWin->EditFlags ("  Op"             ,10,0x000002, "*16mul|mul2|mul4|add|addsmooth");
    app->ParaWin->EditFloat (":  ScaleUV"       ,14, 1,1,-64,64,0.01f);
  }

  if(MakeDependendLink(app,op,"Alpha Map",3,11))
  {
    app->ParaWin->EditFlags ("!  Flags"         ,11,0x000002, "*8tile|clamp:*0no filter|filter|mipmap|trilin|aniso:*4uv0|uv1:*12untransformed|scaled|trans1|trans2");
    app->ParaWin->EditFloat ("*  ScaleUV"       ,15, 1,1,-64,64,0.01f);
  }

  app->ParaWin->EditGroup ("Bump/Lighting section");

  if(MakeDependendLink(app,op,"bump",4,16))
  {
    app->ParaWin->EditFlags ("!  Flags"         ,16,0x000002, "*8tile|clamp:*0no filter|filter|mipmap|trilin|aniso:*4uv0|uv1:*12untransformed|scaled|trans1|trans2");
    app->ParaWin->EditFloat ("*  ScaleUV"       ,20, 1,1,-64,64,0.01f);
  }

  if(MakeDependendLink(app,op,"Detail Bump",5,17))
  {
    app->ParaWin->EditFlags ("!  Flags"         ,17,0x000002, "*8tile|clamp:*0no filter|filter|mipmap|trilin|aniso:*4uv0|uv1:*12untransformed|scaled|trans1|trans2");
    app->ParaWin->EditFloat ("*  ScaleUV"       ,21, 1,1,-64,64,0.01f);
  }

  if(MakeDependendLink(app,op,"Envi",6,18))
    app->ParaWin->EditFlags ("!  Flags"         ,18,0x000002, "*8tile|clamp:*0no filter|filter|mipmap|trilin|aniso:*16normal|reflection:*17unbumped|bumped");

  app->ParaWin->EditGroup ("UV Transform");

  app->ParaWin->EditFloat ("*1trans1 scale",24, 3, 1.0f,  -64,   64,0.01f);
  app->ParaWin->EditFloat ("*2trans1 rot"  ,27, 3, 0.0f,  -64,   64,0.002f);
  app->ParaWin->EditFloat ("*3trans1 trans",30, 3, 0.0f,  -64,   64,0.002f);
  app->ParaWin->EditFloat ("*1trans2 scale",33, 2, 1.0f,  -64,   64,0.01f);
  app->ParaWin->EditFloat ("*2trans2 rot"  ,35, 1, 0.0f,  -64,   64,0.002f);
  app->ParaWin->EditFloat ("*3trans2 trans",36, 2, 0.0f,  -64,   64,0.002f);

  if(op->SetDefaults)   
  {
    *op->Op.GetEditPtrU(8) = sMTF_MIPMAPS;
    *op->Op.GetEditPtrU(9) = sMTF_MIPMAPS;
    *op->Op.GetEditPtrU(10) = sMTF_MIPMAPS;
    *op->Op.GetEditPtrU(11) = sMTF_MIPMAPS;
    *op->Op.GetEditPtrU(16) = sMTF_MIPMAPS;
    *op->Op.GetEditPtrU(17) = sMTF_MIPMAPS;
    *op->Op.GetEditPtrU(18) = sMTF_MIPMAPS;

    *op->Op.GetEditPtrF(12) = 1.0f;
    *op->Op.GetEditPtrF(13) = 1.0f;
    *op->Op.GetEditPtrF(14) = 1.0f;
    *op->Op.GetEditPtrF(15) = 1.0f;
    *op->Op.GetEditPtrF(20) = 1.0f;
    *op->Op.GetEditPtrF(21) = 1.0f;
  }
}

/****************************************************************************/

void Edit_Material_Add(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
}

/****************************************************************************/
/****************************************************************************/

void Edit_MinMesh_SingleVert(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
}

void Edit_MinMesh_Grid(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFlags ("Mode"         , 0,3,  "1sided|2sided");
  app->ParaWin->EditInt   ("Tesselation"  , 1,2,  1,1,255,0.125f);
}

void Edit_MinMesh_Cube(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditInt   ("Tesselate"    , 0,3,    1,    1,  255,0.25f);
  app->ParaWin->EditFlags ("Crease mode"  , 3,11,/*"smooth|faceted:*/"*1normal uv|wraparound uv");
  app->ParaWin->EditFlags ("Origin"       , 3,11,"*2center|bottom");
  app->ParaWin->EditFlags ("Scale UV"     , 3,11,"*3no|yes");
  app->ParaWin->EditFloat ("Scale"        , 4,3, 1.0f,-4096, 4096,0.01f);
  app->ParaWin->EditFloat ("Rotate"       , 7,3, 0.0f,  -64,   64,0.002f);
  app->ParaWin->EditFloat ("Translate"    ,10,3, 0.0f,-4096, 4096,0.001f);
}

void Edit_MinMesh_Cylinder(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditInt   ("Facets"       , 0,1,    8,    3,  255,0.25f);
  app->ParaWin->EditInt   ("Slices"       , 1,1,    1,    1,  255,0.25f);
  app->ParaWin->EditInt   ("Rings"        , 3,1,    1,    1,  255,0.25f);
  app->ParaWin->EditInt   ("Arc"          , 4,1,    0,    0,  255,0.25f);
  app->ParaWin->EditFlags ("Mode"         , 2,0,"closed|open");
  app->ParaWin->EditFlags ("Origin"       , 2,0,"*1center|bottom");
}

void Edit_MinMesh_Torus(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditInt   ("Facets"       , 0,1,    8,    3,  255,0.25f);
  app->ParaWin->EditInt   ("Slices"       , 1,1,    4,    3,  255,0.25f);
  app->ParaWin->EditFloat ("Outer"        , 2,1, 0.5f,  -64,   64,0.002f);
  app->ParaWin->EditFloat ("Inner"        , 3,1, 0.125f,-64,   64,0.002f);
  app->ParaWin->EditFloat ("Phase"        , 4,1, 0.5f, -256,  256,0.05f);
  app->ParaWin->EditFloat ("Arclen"       , 5,1, 1.0f,  -64,   64,0.002f);
  app->ParaWin->EditFlags ("Radius mode"  , 6,1,"relative|absolute");
  app->ParaWin->EditFlags ("Origin"       , 6,0,"*1center|bottom");
}

void Edit_MinMesh_Sphere(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditInt   ("Facets"       , 0,1,    8,    3,  255,0.25f);
  app->ParaWin->EditInt   ("Slices"       , 1,1,    4,    1,  255,0.25f);
}

void Edit_MinMesh_XSI(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFileName("Filename"     , 0);
}

/****************************************************************************/

void Edit_MinMesh_MatLink(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFlags ("Select"       , 0,0,"all|selected|unselected");
  app->ParaWin->EditLink  ("Material"     , 0);
  app->ParaWin->EditInt   ("Renderpass"   , 1,1,0,0,31,0.25f);
}

void Edit_MinMesh_MatLinkId(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditInt   ("Id"           , 0,1,0,0,255,0.25f);
  app->ParaWin->EditLink  ("Material"     , 0);
  app->ParaWin->EditInt   ("Renderpass"   , 1,1,0,0,31,0.25f);
}

void Edit_MinMesh_Add(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
}

void Edit_MinMesh_SelectAll(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFlags ("Select"       , 0,7,"clear|set:*1|vertices:*2|faces");
}

void Edit_MinMesh_SelectCube(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFlags ("Mode"         , 0,2,"add|sub|set|set not:*2vertex|fullface|partface");
  app->ParaWin->EditFloat ("Center"       , 1,3, 0.0f ,-4096, 4096,0.002f);
  app->ParaWin->EditFloat ("Size"         , 4,3, 1.25f,-4096, 4096,0.002f);
} 

void Edit_MinMesh_DeleteFaces(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
}

void Edit_MinMesh_Invert(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
}

void Edit_MinMesh_MatInput(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFlags ("Select"       , 0,0,"all|selected|unselected");
  app->ParaWin->EditInt   ("Renderpass"   , 1,1,0,0,31,0.25f);
}

/****************************************************************************/

void Edit_MinMesh_Transform(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFlags ("Select"       , 0,0,"all|selected|unselected");
  app->ParaWin->EditFloat ("Scale"        , 1,3, 1.0f,-4096, 4096,0.01f);
  app->ParaWin->EditFloat ("Rotate"       , 4,3, 0.0f,  -64,   64,0.002f);
  app->ParaWin->EditFloat ("Translate"    , 7,3, 0.0f,-4096, 4096,0.001f);
}

void Edit_MinMesh_TransformEx(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFlags ("Flags"        , 0,0,"all|selected|unselected:*2pos->|uv0->|uv1->:*5->pos|->uv0|->uv1");
  app->ParaWin->EditFloat ("Scale"        , 1,3, 1.0f,-4096, 4096,0.01f);
  app->ParaWin->EditFloat ("Rotate"       , 4,3, 0.0f,  -64,   64,0.002f);
  app->ParaWin->EditFloat ("Translate"    , 7,3, 0.0f,-4096, 4096,0.001f);
}

void Edit_MinMesh_Displace(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFlags ("Select"       , 0,0,"all|selected|unselected");
  app->ParaWin->EditFloat ("Amplify"      , 1,3, 1.0f,  -64,   64,0.001f);
}

void Edit_MinMesh_Perlin(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFlags ("Select"       , 0,0,"all|selected|unselected");
  app->ParaWin->EditFloat ("*1Scale"      , 1,3, 1.0f,-4096, 4096,0.01f);
  app->ParaWin->EditFloat ("*2Rotate"     , 4,3, 0.0f,  -64,   64,0.002f);
  app->ParaWin->EditFloat ("*3Translate"  , 7,3, 0.0f,-4096, 4096,0.001f);
  app->ParaWin->EditFloat ("*Amplify"     ,10,3, 0.125f,-64,   64,0.001f);
}

void Edit_MinMesh_ExtrudeNormal(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFlags ("Select"       , 0,0,"all|selected|unselected");
  app->ParaWin->EditFloat ("Distance"     , 1,1, 0.0f,  -64,   64,0.001f);
}

void Edit_MinMesh_Bend2(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("Center"       , 0,3, 0.0f,-4096, 4096,0.002f);
  app->ParaWin->EditFloat ("Rotate"       , 3,3, 0.0f,  -64,   64,0.001f);
  app->ParaWin->EditFloat ("Length"       , 6,1, 1.0f,    0, 4096,0.002f);
  app->ParaWin->EditFloat ("Angle"        , 7,1,0.25f,  -64,   64,0.001f);
}

void Edit_MinMesh_Extrude(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFlags ("Mode"         , 0,0,"average normal|individual normal|direction");
  app->ParaWin->EditInt   ("Count"        , 1,1,      1,    1, 255,0.125f);
  app->ParaWin->EditFloat ("Distance/Dir" , 2,3, 0.125f,-4096, 4096,0.001f);
}

void Edit_MinMesh_MTetra(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("Iso value"    , 0,1,0.25f,    0,  100,0.001f);
}

void Edit_MinMesh_BakeAnimation(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("*Fade"     , 0,1, 0.0f,  -64,   64,0.001f);
  app->ParaWin->EditFloat ("*Metamorph", 1,1, 0.0f,  -64,   64,0.001f);
}

void Edit_MinMesh_BoneChain(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("Start"       , 0,3, 0.0f,-4096, 4096,0.125f);
  app->ParaWin->EditFloat ("End"         , 3,3, 0.0f,-4096, 4096,0.125f);
  app->ParaWin->EditInt   ("Count"       , 6,1,    2,     2, 255,0.125f);
  app->ParaWin->EditFlags ("Flags"       , 7,0,"|auto");
}

void Edit_MinMesh_BoneTrain(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("Delta"       , 0,1, 0.0f,-1, 1,0.001f);
  app->ParaWin->EditFlags ("Flags"       , 1,0,"shift|scale");
//  app->ParaWin->EditFloat ("Metamorph"   , 1,1, 0.0f,-1, 1,0.001f);
}

void Edit_MinMesh_Kopuli(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
}

void Edit_MinMesh_Triangulate(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
}

void Edit_MinMesh_Compress(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
}

void Edit_MinMesh_Pipe(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFlags("Flags",0,0,"|scale tex:*1|add rings");
  app->ParaWin->EditFloat ("Scale Texture" , 1,1, 1.0f,-256, 256,0.001f);
  app->ParaWin->EditFloat ("Ring Distance" , 2,1, 1.0f,0, 4096,0.001f);
}

void Edit_MinMesh_ScaleAnim(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("Scale"        , 0,1, 1.0f,-4096, 4096,0.01f);
}

void Edit_MinMesh_RenderAutoMap(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFlags ("Dir"          , 0,1,"*0|left:*1|right:*2|bottom:*3|top");
  app->ParaWin->EditFlags ("Dir"          , 0,1,"*4|back:*5|front");
}

void Edit_MinMesh_Multiply(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("Local Rotate" ,13,3, 0.0f,  -64,   64,0.002f);
  app->ParaWin->EditFloat ("Scale"        , 0,3, 1.0f,-4096, 4096,0.01f);
  app->ParaWin->EditFloat ("Rotate"       , 3,3, 0.0f,  -64,   64,0.002f);
  app->ParaWin->EditFloat ("Translate"    , 6,3, 0.0f,-4096, 4096,0.001f);
//  app->ParaWin->EditFloat ("ExtrudeNormal",16,1, 0.0f, -256,  256,0.001f);
  app->ParaWin->EditInt   ("Count"        , 9,1,    2,    1,  255,0.25f);
  app->ParaWin->EditFlags ("Mode"         ,10,0, "|trans uv:*1|random");
  app->ParaWin->EditFloat ("Trans UV"     ,11,2, 0.0f,  -64,   64,0.01f);
}

void Edit_MinMesh_Multiply2(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditInt   ("Seed"         , 0,1,0,0,255,0.25f);
  app->ParaWin->EditInt   ("Emphasize last",13,1,0,0,255,0.25f);
  app->ParaWin->EditInt   ("Count"        , 1,3,1,1,255,0.25f);
  app->ParaWin->EditFloat ("Translate"    , 4,3,0.0f,-4096.0f,4096.0f,0.001f);
  app->ParaWin->EditInt   ("Count"        , 7,3,1,1,255,0.25f);
  app->ParaWin->EditFloat ("Translate"    ,10,3,0.0f,-4096.0f,4096.0f,0.001f);
  app->ParaWin->EditInt   ("Count"        ,14,3,1,1,255,0.25f);
  app->ParaWin->EditFloat ("Translate"    ,17,3,0.0f,-4096.0f,4096.0f,0.001f);
}

void Edit_MinMesh_Center(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFlags ("Which"        , 0,7, "|x:*1|y:*2|z");
  app->ParaWin->EditFlags ("Operation"    , 0,7, "*7center|on ground");
}

void Edit_MinMesh_SelectLogic(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFlags ("Mode"         , 0,0,"invert face|invert vertex|face->vertex");
} 

/****************************************************************************/
/****************************************************************************/

void Edit_Scene_Scene(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("*1Scale"       , 0,3, 1.0f,-4096, 4096,0.01f);
  app->ParaWin->EditFloat ("*2Rotate"      , 3,3, 0.0f,  -64,   64,0.002f);
  app->ParaWin->EditFloat ("*3Translate"   , 6,3, 0.0f,-4096, 4096,0.005f);
  app->ParaWin->EditFlags ("Flags"        , 9,0,"|lightmap");
}

void Edit_Scene_Add(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
}

void Edit_Scene_Multiply(WerkkzeugApp *app,WerkOp *op)
{
  sControl *con;
  sInt i;
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("*1Scale"       , 0,3, 1.0f,-4096, 4096,0.01f);
  app->ParaWin->EditFloat ("*2Rotate"      , 3,3, 0.0f,  -64,   64,0.002f);
  app->ParaWin->EditFloat ("*3Translate"   , 6,3, 0.0f,-4096, 4096,0.005f);
  con = app->ParaWin->EditInt   ("*Count"       , 9,1,    2,    1,  255,0.25f);
  if(con)
  {
    i = con->ChangeCmd&0xff;
    con->ChangeCmd = CMD_PARA_CHCHX|i;
    con->DoneCmd   = CMD_PARA_CHCHX|i;
  }
}

void Edit_Scene_Transform(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("*1Scale"       , 0,3, 1.0f,-4096, 4096,0.01f);
  app->ParaWin->EditFloat ("*2Rotate"      , 3,3, 0.0f,  -64,   64,0.002f);
  app->ParaWin->EditFloat ("*3Translate"   , 6,3, 0.0f,-4096, 4096,0.005f);
}

void Edit_Scene_Light(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
//  app->ParaWin->EditFloat ("Rotate"       , 0,3, 0.0f,  -64,   64,0.002f);
  app->ParaWin->EditFloat ("*3Translate"   , 3,3, 0.0f,-4096, 4096,0.005f);
  app->ParaWin->EditFlags ("*Flags"       , 6,2,"Ordinary|Weapon:*1|shadows");
//  app->ParaWin->EditFloat ("Zoom"         , 7,1, 1.0f,    0,   16,0.01f);
  app->ParaWin->EditRGBA  ("*4Color 1"     , 8,0xffffffff);
  app->ParaWin->EditFloat ("*Amplify"     , 9,1, 1.0f,  -64,   64,0.01f);
  app->ParaWin->EditFloat ("*Range"       ,10,1, 8.0f,    0, 4096,0.01f);
}

/*void Edit_Scene_MapLight(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("Center"       , 0,3, 0.0f, -256,  256,0.005f);
  app->ParaWin->EditFloat ("Box"          , 3,3, 0.0f,    0,  256,0.01f);
  app->ParaWin->EditFloat ("Radius"       , 6,3, 1.0f,    0,  256,0.01f);
  app->ParaWin->EditFloat ("Falloff"      , 9,1, 1.0f,    0,   64,0.01f);
  app->ParaWin->EditRGBA  ("Color"        ,10,0xffffffff);
}*/

void Edit_Scene_Camera(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("*1Scale"       , 0,3, 1.0f,-4096, 4096,0.01f);
  app->ParaWin->EditFloat ("*2Rotate"      , 3,3, 0.0f,  -64,   64,0.002f);
  app->ParaWin->EditFloat ("*3Translate"   , 6,3, 0.0f,-4096, 4096,0.005f);
}

void Edit_Scene_Limb(WerkkzeugApp *app,WerkOp *op)
{
  sControl *c0;
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("*3Translate"   , 0,3, 0.0f,-4096, 4096,0.005f);
  c0 = app->ParaWin->EditFloat ("*Plane"  , 3,3, 0.0f, -256,  256,0.01f);
  app->ParaWin->EditFloat ("*Upper Length", 6,1, 1.0f, -256,  256,0.005f);
  app->ParaWin->EditFloat ("*Lower Length", 7,1, 1.0f, -256,  256,0.005f);
  app->ParaWin->EditFlags("Flags",8,0,"hang|break:*1absolute|relative");

  app->ParaWin->DefaultF3(3,c0,1,0,0);
}

void Edit_Scene_Walk(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFlags ("Flags"              , 0,0,"next leg|best leg:*1|collision");
  app->ParaWin->EditFlags ("Body-Rotation"      , 0,0,"*2straight|y-axxis|xy-axxes|xz-axxes");
  app->ParaWin->EditFlags ("Animation-Helper"   , 0,0,"*4off|walk-cycle|run-cycle:*6|long steps:*7|in place:*8|reset");
  app->ParaWin->EditInt   ("Leg Count"          , 1,1,    2,    1,    8,0.25f);
  app->ParaWin->EditFloat ("Step Treshold"      , 2,1,0.15f,    0,   16,0.01f);
  app->ParaWin->EditFloat ("Switch back to Walk", 3,1,0.75f,    0,   16,0.01f);
  app->ParaWin->EditFloat ("Switch to Run"      , 4,1,1.25f,    0,   16,0.01f);
  app->ParaWin->EditSpline("Step-Spline" , 0);
  app->ParaWin->EditGroup ("walk/run");
  app->ParaWin->EditInt   ("Feet Groups"        , 5,2,    2,    1,    8,0.25f);
  app->ParaWin->EditFloat ("Step Length"        , 7,2,1.25f,    0,   16,0.01f);
  app->ParaWin->EditFloat ("Cycle Time (s)"     , 9,2,0.50f,    0,   64,0.01f);
  app->ParaWin->EditFloat ("Next Step Time (s)" ,11,2,0.50f,    0,   64,0.01f);
  app->ParaWin->EditFloat ("Step Scan Up"       ,37,1,    2,    0,   16,0.01f);
  app->ParaWin->EditFloat ("Step Scan DOwn"     ,38,1,    2,    0,   16,0.01f);
  app->ParaWin->EditGroup ("feet position");
  app->ParaWin->EditFloat ("Foot 0"             ,13,3,0.00f, -256,  256,0.005f);
  app->ParaWin->EditFloat ("Foot 1"             ,16,3,0.00f, -256,  256,0.005f);
  app->ParaWin->EditFloat ("Foot 2"             ,19,3,0.00f, -256,  256,0.005f);
  app->ParaWin->EditFloat ("Foot 3"             ,22,3,0.00f, -256,  256,0.005f);
  app->ParaWin->EditFloat ("Foot 4"             ,25,3,0.00f, -256,  256,0.005f);
  app->ParaWin->EditFloat ("Foot 5"             ,28,3,0.00f, -256,  256,0.005f);
  app->ParaWin->EditFloat ("Foot 6"             ,31,3,0.00f, -256,  256,0.005f);
  app->ParaWin->EditFloat ("Foot 7"             ,34,3,0.00f, -256,  256,0.005f);
}

void Edit_Scene_Rotate(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditCycle("Axxis"         , 3,0,"X|Y|Z|To Camera");
  app->ParaWin->EditSetGray(*op->Op.GetEditPtrU(0)==3);
  app->ParaWin->EditFloat("Direction"     , 0,3,0,-1,1,0.02f);
}

void Edit_Scene_Forward(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat("Treshold"      , 0,1,0,-256,256,0.02f);
}

void Edit_Scene_MatHack(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditLink  ("Update Material", 0);
}

void Edit_Scene_Physic(WerkkzeugApp *app,WerkOp *op)
{
  sInt mode;
  mode = (*op->Op.GetAnimPtrU(0))&255;
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFlags("Flags"         , 0,0,"<old>|<old>|Collision|<old>:*8|reset");
  app->ParaWin->EditFlags("Flags"         , 0,0,"*4|Monster:*5|Shot");
  app->ParaWin->EditSetGray(mode==2);
  app->ParaWin->EditFloat("Speed"         , 1,3,0,-16,16,0.0001f);
  app->ParaWin->EditSetGray(mode==0);
  app->ParaWin->EditFloat("Scale"         , 4,3,1,-256,256,0.0001f);
//  app->ParaWin->EditFloat("RMass"         , 7,3,0,-16,16,0.01f);
  app->ParaWin->EditSetGray(mode==1 || mode==2);
  app->ParaWin->EditFloat("Mass"          ,10,1,0,-16,16,0.01f);
  app->ParaWin->EditSetGray(mode==2);
  app->ParaWin->EditInt("Particle Kind"   ,11,1,0,0,31,0.25f);
}

void Edit_Scene_Sector(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
}

void Edit_Scene_Portal(WerkkzeugApp *app,WerkOp *op)
{
  sControl *c0,*c1,*c2;
  app->ParaWin->EditOp(op);
  app->ParaWin->EditLink  ("Scene 1"      , 0);
  app->ParaWin->EditLink  ("Scene 2"      , 1);
  app->ParaWin->EditLink  ("Inbetween"    , 2);
  c0 = app->ParaWin->EditFloat ("X"            , 0,2, -0.5f, -256, 256,0.01f);
  c1 = app->ParaWin->EditFloat ("Y"            , 2,2,  0.0f, -256, 256,0.01f);
  c2 = app->ParaWin->EditFloat ("Z"            , 4,2, -0.5f, -256, 256,0.01f);
  app->ParaWin->EditInt   ("Cost"         , 6,1, 32,0,255,2);
  app->ParaWin->EditFloat ("*Door (<0.02=closed)", 7,1, 1,0,1,0.01f);

  app->ParaWin->DefaultF2(0,c0,-0.5f,0.5f);
  app->ParaWin->DefaultF2(2,c1, 0.0f,1.0f);
  app->ParaWin->DefaultF2(4,c2,-0.5f,0.5f);
}

void Edit_Scene_ForceLights(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditCycle ("!Mode"        , 0,0,"always|switch");
  app->ParaWin->EditSetGray(*op->Op.GetEditPtrU(0) != 1);
  app->ParaWin->EditInt   ("Switch"       , 1,1,0,0,255,0.25f);
}

void Edit_Scene_Particles(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFlags ("!Mode"        , 0,0,"line|spline:*2cube|bubble|sphere:*4|rotate:*5random|ordered");
  app->ParaWin->EditInt   ("?Count"        , 1,1,0,0,0x4000,1.0f);      // '?' -> reconnect scene to flush Instance Memory
  app->ParaWin->EditInt   ("Random Seed"  , 2,1,0,0,255,0.25f);
  app->ParaWin->EditFloat ("*Animate"      ,12,1, 1.0f,-4096, 4096,0.01f);
  app->ParaWin->EditFloat ("3Rand Pos"   , 3,3, 1.0f,-4096, 4096,0.005f);
  app->ParaWin->EditSetGray((*op->Op.GetEditPtrU(0)&8)!=1);
  app->ParaWin->EditFloat ("2Rand Rotate", 6,3, 0.0f,  -64,   64,0.002f);
  app->ParaWin->EditFloat ("2Rotate Speed",9,3, 0.0f,  -64,   64,0.002f);
  app->ParaWin->EditSetGray((*op->Op.GetEditPtrU(0)&3)!=0);
  app->ParaWin->EditFloat ("*3Line"       ,13,3, 0.0f,-4096, 4096,0.005f);
  app->ParaWin->EditSetGray((*op->Op.GetEditPtrU(0)&3)!=1);
  app->ParaWin->EditSpline("Spline"       , 0);
}

void Edit_Scene_AdjustPass(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditInt   ("Adjustment"   , 0,1,0,-31,31,0.125f);
}

void Edit_Scene_ApplySpline(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("*Animate"      ,0,1, 1.0f,-16, 16,0.01f);
}

void Edit_Scene_Marker(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditInt   ("Marker"     , 0,1,0,0,31,0.25f);
}

void Edit_Scene_LOD(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditFloat ("*Distance"   ,0,1,16.0f,0,4096,1.0f);
}

void Edit_Scene_Ambient(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditRGBA  ("Color"      , 0,0x00000000);
}

/****************************************************************************/

void Edit_IPP_Viewport(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditCycle ("Size"         , 0,4,"Large|Medium|Small|Full|Screen");
  app->ParaWin->EditFlags ("!Clears"       , 1,3,"|color:*1|z-buffer:*2|master cam:*4shadows|simple|no specular");
  app->ParaWin->EditFlags ("!Flags"       , 1,3,"*6|stereo 3d:*8normal|2d bb|3d bb:*12screen aspect|rt aspect");
  app->ParaWin->EditRGBA  ("*4Color"      , 2,0xff000000);
  app->ParaWin->EditSetGray((*op->Op.GetEditPtrU(1)&4) || (*op->Op.GetEditPtrU(1)&0x300));
  app->ParaWin->EditFloat ("*2Rotate"     , 3,3, 0.0f,  -64,   64,0.002f);
  app->ParaWin->EditSetGray(0);
  app->ParaWin->EditFloat ("*3Translate"  , 6,3, 0.0f,-4096, 4096,0.001f);
  app->ParaWin->EditSetGray((*op->Op.GetEditPtrU(1)&0x04) && !(*op->Op.GetEditPtrU(1)&0x300));
  app->ParaWin->EditFloat ("Far Clip"     , 9,1, 4096.0f, 0,65536,1.000f);
  app->ParaWin->EditFloat ("Near Clip"    ,10,1, 0.125f,   0,  128,0.002f);
  app->ParaWin->EditFloat ("*Center"      ,11,2, 0.0f, -256,  256,0.002f);
  app->ParaWin->EditFloat ("*Zoom"        ,13,2, 1.0f,    0,  256,0.020f);
  app->ParaWin->EditSetGray(0);
  app->ParaWin->EditGroup ("Fogging");
  app->ParaWin->EditRGBA  ("Fog Color"    ,15,0xff000000);
  app->ParaWin->EditFloat ("Far Fog"      ,16,1, 4096.0f, 0,65536,0.020f);
  app->ParaWin->EditFloat ("Near Fog"     ,17,1, 16.0f, 0,65536,0.020f);
  app->ParaWin->EditGroup ("Window");
  app->ParaWin->EditFloat ("upper left"   ,20,2, 0,0,1,0.020f);
  app->ParaWin->EditFloat ("lower right"  ,22,2, 1,0,1,0.020f);

  if(*op->Op.GetEditPtrU(1) & 0x40) // stereo 3d enabled
  {
    app->ParaWin->EditGroup ("Anaglyphic (stereo) 3d");
    app->ParaWin->EditFloat ("Eye distance"   ,18,1, 0.015f, 0.0f,256,0.001f);
    app->ParaWin->EditFloat ("Focal length"   ,19,1, 2.0f, 0.01f,256,0.01f);
  }

  if(op->SetDefaults)   
  {
    *op->Op.GetEditPtrF(18) = 0.015f;
    *op->Op.GetEditPtrF(19) = 2.0f;
  }
}

void Edit_IPP_Blur(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditCycle ("Size"         , 0,5,"Large|Medium|Small|Full|Screen|Stay");
  app->ParaWin->EditFloat ("*Radius"      , 1,1, 1.0f,0.0f,100.0f,0.05f);
  app->ParaWin->EditFloat ("*Amplify"     , 2,1, 1.0f,0.0f,4.0f,0.01f);
  app->ParaWin->EditFlags ("Flags"        , 3,0, "Square|Diamond:*1Amp Once|Overbright");
  app->ParaWin->EditInt   ("Max Stages"   , 4,1, 4,1,16,0.25f);
}

void Edit_IPP_Copy(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditCycle ("Size"         , 0,5,"Large|Medium|Small|Full|Screen|Stay");
  app->ParaWin->EditRGBA  ("*4Color 1"     , 1,0xffffffff);
  app->ParaWin->EditFloat ("*Zoom"        , 2,1, 1.0f,0.0f,4.0f,0.01f);
}

void Edit_IPP_Crashzoom(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditCycle ("Size"         , 0,5,"Large|Medium|Small|Full|Screen|Stay");
  app->ParaWin->EditInt   ("Steps"        , 1,1, 3,1,5,0.25f);
  app->ParaWin->EditFloat ("*Zoom"        , 2,1, 1.5f,1.0f,20.0f,0.005f);
  app->ParaWin->EditFloat ("*Amplify"     , 3,1, 1.0f,0.0f,4.0f,0.01f);
  app->ParaWin->EditFloat ("*Center"      , 4,2, 0.5f,-8.0f,8.0f,0.01f);
}

void Edit_IPP_Sharpen(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditCycle ("Size"         , 0,5,"Large|Medium|Small|Full|Screen|Stay");
  app->ParaWin->EditFloat ("*Radius"      , 1,1, 1.0f,0.0f,100.0f,0.05f);
  app->ParaWin->EditRGBA  ("*Amplify"     , 2,0x202020);
  app->ParaWin->EditInt   ("Max Stages"   , 3,1, 4,1,16,0.25f);
}

void Edit_IPP_Color(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditCycle ("Size"         , 0,5,"Large|Medium|Small|Full|Screen|Stay");
  app->ParaWin->EditRGBA  ("*4Color"       , 1,0xff808080);
  app->ParaWin->EditCycle ("Operation"    , 2,0, "add|addsmooth|sub|rsub|mul|mul2x|blend|dp3");
  app->ParaWin->EditRGBA  ("*Amplify"     , 3,0x404040);
}

void Edit_IPP_Merge(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditCycle ("Size"         , 0,5,"Large|Medium|Small|Full|Screen|Stay");
  app->ParaWin->EditCycle ("Operation"    , 1,0, "add|addsmooth|sub|rsub|mul|mul2x|blend|dp3|alpha");
  app->ParaWin->EditInt   ("*Alpha"       , 2,1, 128,0,255,1);
  app->ParaWin->EditRGBA  ("*Amplify"     , 3,0x404040);
}

void Edit_IPP_Mask(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditCycle ("Size"         , 0,5,"Large|Medium|Small|Full|Screen|Stay");
}

void Edit_IPP_Layer2D(WerkkzeugApp *app,WerkOp *op)
{
  sControl *sc,*uv;

  app->ParaWin->EditOp(op);
  app->ParaWin->EditCycle ("Size"         , 0,4,"Large|Medium|Small|Full|Screen");
  app->ParaWin->EditLink  ("Material"     , 0);
  sc = app->ParaWin->EditFloat ("*Screen rect"  , 1,4, 0,-64,64,0.01f);
  uv = app->ParaWin->EditFloat ("*UV rect"      , 5,4, 0,-64,64,0.01f);
  app->ParaWin->EditFloat ("*Z"           , 9,1, 0,0,1,0.001f);
  app->ParaWin->EditFlags ("Clear"        ,10,0x30, "none|color|z|both");
  app->ParaWin->EditFlags ("Positioning"  ,10,0x30, "*4xy corner|xy center/size:*5uv corner|uv center/size");

  if(!op->SetDefaults)
  {
    sc->Default[0] = 0.5f;
    sc->Default[1] = 0.5f;
    sc->Default[2] = 1.0f;
    sc->Default[3] = 1.0f;
    uv->Default[0] = 0.0f;
    uv->Default[1] = 0.0f;
    uv->Default[2] = 1.0f;
    uv->Default[3] = 1.0f;
  }
  else
  {
    *op->Op.GetEditPtrF(1) = 0.5f;
    *op->Op.GetEditPtrF(2) = 0.5f;
    *op->Op.GetEditPtrF(3) = 1.0f;
    *op->Op.GetEditPtrF(4) = 1.0f;
    *op->Op.GetEditPtrF(5) = 0.0f;
    *op->Op.GetEditPtrF(6) = 0.0f;
    *op->Op.GetEditPtrF(7) = 1.0f;
    *op->Op.GetEditPtrF(8) = 1.0f;
  }
}

void Edit_IPP_Select(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
//  app->ParaWin->EditCycle ("Size"         , 0,4,"Large|Medium|Small|Full|Screen");
}

void Edit_IPP_RenderTarget(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditLink  ("Target"       , 0);
}

void Edit_IPP_HSCB(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditCycle ("Size"         , 0,5,"Large|Medium|Small|Full|Screen|Stay");
  app->ParaWin->EditFloat ("Hue"          , 1,1,0.00f,-4.0f, 4.0f,0.001f);
  app->ParaWin->EditFloat ("Saturation"   , 2,1,1.00f,-16.0f,16.0f,0.001f);
  app->ParaWin->EditFloat ("Contrast"     , 3,1,1.00f, 0.0f, 16.0f,0.001f);
  app->ParaWin->EditFloat ("Brightness"   , 4,1,1.00f, 0.0f, 64.0f,0.001f);
}

void Edit_IPP_JPEG(WerkkzeugApp *app,WerkOp *op)
{
  app->ParaWin->EditOp(op);
  app->ParaWin->EditCycle ("Size"         , 0,5,"Large|Medium|Small|Full|Screen|Stay");
  app->ParaWin->EditCycle ("Direction"    , 1,0,"X|Y");
  app->ParaWin->EditFloat ("*Strength"    , 2,1,1.0f,-8.0f,8.0f,0.01f);
}

/****************************************************************************/
/****************************************************************************/

WerkClass WerkClasses[] =
{

/****************************************************************************/

  {
    "Load"      ,0x01,KC_ANY,0,0,0x06001000,COL_ALL,'l',
    "",
    Edit_Misc_Load,
    Init_Misc_Load,
    Exec_Misc_Load,
    { KC_ANY },
    { KC_ANY },
  },
  {
    "Store"     ,0x02,KC_ANY,1,1,0x26000100,COL_ALL,'s',
    "",
    Edit_Misc_Store,
    Init_Misc_Nop,
    Exec_Misc_Nop,
    { KC_ANY },
    { 0 },
  },
  {
    "Nop"       ,0x03,KC_ANY,1,1,0x26000100,COL_ALL,0,
    "",
    Edit_Misc_Nop,
    Init_Misc_Nop,
    Exec_Misc_Nop,
    { KC_ANY },
    { 0 },
  },
  {
    "Unknown"   ,0x04,KC_ANY,1,1,0x24000100,COL_HIDE,0,
    "",
    Edit_Misc_Nop,
    Init_Misc_Nop,
    Exec_Misc_Nop,
    { KC_ANY },
    { 0 },
  },
  {
    "Time"      ,0x05,KC_ANY,1,1,0x24000100,COL_ALL,0,
    "",
    Edit_Misc_Nop,
    Init_Misc_Nop,
    Exec_Misc_Time,
    { KC_ANY },
    { 0 },
  },
  {
    "Event"     ,0x06,KC_ANY,1,1,0x00000101,COL_ALL,0,
    "f",
    Edit_Misc_Event,
    Init_Misc_Event,
    Exec_Misc_Event,
    { KC_ANY },
    { 0 },
  },
  {
    "Trigger"   ,0x07,KC_ANY,1,1,0x10101112,COL_ALL,0,
    "ffiibffbfffffffffc",
    Edit_Misc_Trigger,
    Init_Misc_Trigger,
    Exec_Misc_Trigger,
    { KC_ANY },
    { 0 },
  },
  {
    "Delay"     ,0x08,KC_ANY,1,1,0x20000107,COL_ALL,0,
    "bbeeeee",
    Edit_Misc_Delay,
    Init_Misc_Nop,
    Exec_Misc_Delay,
    { KC_ANY },
    { 0 },
  },
  {
    "If"        ,0x09,KC_ANY,1,2,0x20000202,COL_ALL,0,
    "bb",
    Edit_Misc_If,
    Init_Misc_If,
    Exec_Misc_If,
    { KC_ANY,KC_ANY },
    { 0 },
  },
  {
    "SetIf"     ,0x0a,KC_ANY,1,1,0x20000104,COL_ALL,0,
    "bbbb",
    Edit_Misc_SetIf,
    Init_Misc_Nop,
    Exec_Misc_SetIf,
    { KC_ANY },
    { 0 },
  },
  {
    "State"     ,0x0b,KC_ANY,1,1,0x20000105,COL_ALL,0,
    "bbbbb",
    Edit_Misc_State,
    Init_Misc_Nop,
    Exec_Misc_State,
    { KC_ANY },
    { 0 },
  },
  {
    "PlaySample"     ,0x0c,KC_ANY,1,1,0x20000107,COL_ALL,0,
    "bbebeeg",
    Edit_Misc_PlaySample,
    Init_Misc_Nop,
    Exec_Misc_PlaySample,
    { KC_ANY },
    { 0 },
  },
  {
    "Demo"           ,0x0d,KC_DEMO,1,KK_MAXINPUT,0x25000000,COL_ADD,'d',
    "",
    Edit_Misc_Demo,
    Init_Misc_Demo,
    Exec_Misc_Demo,
    { KC_ANY },
    { 0 },
  },
  {
    "Menu"     ,0x0e,KC_ANY,1,1,0x20000105,COL_ALL,0,
    "bbb",
    Edit_Misc_Menu,
    Init_Misc_Nop,
    Exec_Misc_Menu,
    { KC_ANY },
    { 0 },
  },
  {
    "Master Camera"           ,0x0f,KC_DEMO,0,0,0x0000000c,COL_ADD,0,
    "ffffffff"  "ffff",
    Edit_Misc_MasterCam,
    Init_Misc_MasterCam,
    Exec_Misc_MasterCam,
    { 0 },
    { 0 },
  },


/****************************************************************************/

  {
    "KKriegerPara",0x10,KC_SCENE,0,1,0x24800139,COL_NEW,0,
    "iiiiiiii"
    "iiiiiiiib"
    "ffffffffffff"
    "fff"
    "iiiiiiiiffffffffbbbbbbbb"
    "f",
    Edit_KKrieger_Para,
    Init_KKrieger_Para,
    Exec_KKrieger_Para,
    { KC_SCENE },
    { 0 },
  },
  {
    "Monster",0x11,KC_SCENE,1,2,0x3410021d,COL_NEW,0,
    "fffffffffb"
    "iiififfbff" "ffffff" "bib",
    Edit_KKrieger_Monster,
    Init_KKrieger_Monster,
    Exec_KKrieger_Monster,
    { KC_SCENE,KC_SCENE },
    { KC_ANY },
  },
  {
    "KKriegerEvents",0x12,KC_SCENE,0,1,0x34008101,COL_NEW,0,
    "b",
    Edit_KKrieger_Events,
    Init_KKrieger_Events,
    Exec_KKrieger_Events,
    { KC_SCENE },
    { 0 },
  },
  {
    "Respawn",0x13,KC_SCENE,0,0,0x00000005,COL_NEW,0,
    "fffbf",
    Edit_KKrieger_Respawn,
    Init_KKrieger_Respawn,
    Exec_KKrieger_Respawn,
    { KC_SCENE },
    { 0 },
  },
  {
    "SplashDamage",0x14,KC_SCENE,0,0,0x00000003,COL_NEW,0,
    "bff",
    Edit_KKrieger_SplashDamage,
    Init_KKrieger_SplashDamage,
    Exec_KKrieger_SplashDamage,
    { KC_SCENE },
    { 0 },
  },
  {
    "MonsterFlags",0x15,KC_SCENE,1,1,0x20000103,COL_NEW,0,
    "bbb",
    Edit_KKrieger_MonsterFlags,
    Init_KKrieger_MonsterFlags,
    Exec_KKrieger_MonsterFlags,
    { KC_SCENE,KC_SCENE },
    { KC_ANY },
  },

/****************************************************************************/

  {
    "Spline"        ,0x18,KC_SPLINE,0,0,0x40080000,COL_ADD,0,
    "",
    Edit_Misc_Spline,
    Init_Misc_Spline,
    Exec_Misc_Nop,
    { 0 },
    { 0 },
  },

  {
    "Shaker"        ,0x19,KC_SPLINE,1,1,0x40000111,COL_ADD,0,
    "bffffffffffffffff",
    Edit_Misc_Shaker,
    Init_Misc_Shaker,
    Exec_Misc_Nop,
    { KC_SPLINE },
    { 0 },
  },

  {
    "PipeSpline"    ,0x1a,KC_SPLINE,0,0,0x40080000,COL_ADD,0,
    "",
    Edit_Misc_PipeSpline,
    Init_Misc_PipeSpline,
    Exec_Misc_Nop,
    { 0 },
    { 0 },
  },

  {
    "MetaSpline"    ,0x1b,KC_SPLINE,1,8,0x00000808,COL_ADD,0,
    "ffffffff",
    Edit_Misc_MetaSpline,
    Init_Misc_MetaSpline,
    Exec_Misc_Nop,
    { KC_SPLINE,KC_SPLINE,KC_SPLINE,KC_SPLINE,KC_SPLINE,KC_SPLINE,KC_SPLINE,KC_SPLINE },
    { 0 },
  },



/****************************************************************************/

  {
    "Flat"      ,0x21,KC_BITMAP,0,0,0x06000003,COL_NEW,'f',
    "bbc",
    Edit_Bitmap_Flat,
    Bitmap_Flat,
    Exec_Misc_Nop,
    { KC_BITMAP },
    { 0 },
  },

  {
    "Perlin"    ,0x22,KC_BITMAP,0,0,0x0600000b,COL_NEW,'p',
    "bbbbebbggcc",
    Edit_Bitmap_Perlin,
    Bitmap_Perlin,
    Exec_Misc_Nop,
    { KC_BITMAP },
    { 0 },
  },
  {
    "Color"     ,0x23,KC_BITMAP,1,1,0x06000102,COL_MOD,'C'|sKEYQ_SHIFT,
    "bc",
    Edit_Bitmap_Color,
    Bitmap_Color,
    Exec_Misc_Nop,
    { KC_BITMAP },
    { 0 },
  },
  {
    "Merge"     ,0x24,KC_BITMAP,1,KK_MAXINPUT,0x07000001,COL_ADD,'a',
    "b",
    Edit_Bitmap_Merge,
    Bitmap_Merge,
    Exec_Misc_Nop,
    { KC_BITMAP },
    { 0 },
  },

  {
    "Format"    ,0x25,KC_BITMAP,1,1,0x06000103,COL_MOD,0,
    "bbb",
    Edit_Bitmap_Format,
    Bitmap_Format,
    Exec_Misc_Nop,
    { KC_BITMAP },
    { 0 },
  },
  {
    "Render Target" ,0x26,KC_BITMAP,0,0,0x06000003,COL_MOD,0,
    "bbb",
    Edit_Bitmap_RenderTarget,
    Bitmap_RenderTarget,
    Exec_Misc_Nop,
    { 0 },
    { 0 },
  },
  {
    "GlowRectOld"  ,0x27,KC_BITMAP,1,1,0x0600010b,COL_MOD,0,
    "eeeeeeceebb",
    Edit_Bitmap_GlowRectOld,
    Bitmap_GlowRect,
    Exec_Misc_Nop,
    { KC_BITMAP },
    { 0 },
  },
  {
    "Dots"      ,0x28,KC_BITMAP,1,1,0x06000104,COL_MOD,0,
    "ccib",
    Edit_Bitmap_Dots,
    Bitmap_Dots,
    Exec_Misc_Nop,
    { KC_BITMAP },
    { 0 },
  },
  {
    "Blur"      ,0x29,KC_BITMAP,1,1,0x06000104,COL_MOD,'b',
    //"beee",
    "bggg",
    Edit_Bitmap_Blur,
    Bitmap_Blur,
    Exec_Misc_Nop,
    { KC_BITMAP },
    { 0 },
  },
  {
    "Mask"      ,0x2a,KC_BITMAP,3,3,0x06000301,COL_ADD,0,
    "b",
    Edit_Bitmap_Mask,
    Bitmap_Mask,
    Exec_Misc_Nop,
    { KC_BITMAP,KC_BITMAP,KC_BITMAP },
    { 0 },
  },
  {
    "HSCB"      ,0x2b,KC_BITMAP,1,1,0x06000104,COL_MOD,'h',
    //"eeef",
    "gggg",
    Edit_Bitmap_HSCB,
    Bitmap_HSCB,
    Exec_Misc_Nop,
    { KC_BITMAP },
    { 0 },
  },
  {
    "Rotate"    ,0x2c,KC_BITMAP,1,1,0x06000108,COL_MOD,'r',
    "eggeebbb",
    Edit_Bitmap_Rotate,
    Bitmap_Rotate,
    Exec_Misc_Nop,
    { KC_BITMAP },
    { 0 },
  },
  {
    "Distort"   ,0x2d,KC_BITMAP,2,2,0x06000202,COL_ADD,'d',
    "eb",
    Edit_Bitmap_Distort,
    Bitmap_Distort,
    Exec_Misc_Nop,
    { KC_BITMAP,KC_BITMAP },
    { 0 },
  },
  {
    "Normals"   ,0x2e,KC_BITMAP,1,1,0x06000102,COL_MOD,'n',
    "eb",
    Edit_Bitmap_Normals,
    Bitmap_Normals,
    Exec_Misc_Nop,
    { KC_BITMAP },
    { 0 },
  },
  {
    "Light"     ,0x2f,KC_BITMAP,1,1,0x0600010b,COL_MOD,'L'|sKEYQ_SHIFT,
    "beeeeecceee",
    Edit_Bitmap_Light,
    Bitmap_Light,
    Exec_Misc_Nop,
    { KC_BITMAP },
    { 0 },
  },  
  {
    "Bump"      ,0x30,KC_BITMAP,2,2,0x0600020e,COL_MOD,'B'|sKEYQ_SHIFT,
    "bgggeecceeecge",
    Edit_Bitmap_Bump,
    Bitmap_Bump,
    Exec_Misc_Nop,
    { KC_BITMAP,KC_BITMAP },
    { 0 },
  },
  {
    "Text"      ,0x31,KC_BITMAP,1,1,0x468a0109,COL_MOD,'t',
    "eeeecbbee",
    Edit_Bitmap_Text,
    Bitmap_Text,
    Exec_Misc_Nop,
    { KC_BITMAP },
    { 0 },
  },  
  {
    "Cell"      ,0x32,KC_BITMAP,0,0,0x0600000d,COL_NEW,0,
    "bbcccbbeebebf",
    Edit_Bitmap_Cell,
    Bitmap_Cell,
    Exec_Misc_Nop,
    { KC_BITMAP },
    { 0 },
  },
  {
    "Wavelet"   ,0x33,KC_BITMAP,1,1,0x06000102,COL_MOD,0,
    "be",
    Edit_Bitmap_Wavelet,
    Bitmap_Wavelet,
    Exec_Misc_Nop,
    { KC_BITMAP },
    { 0 },
  },
  {
    "Gradient"  ,0x34,KC_BITMAP,0,0,0x06000008,COL_NEW,'G'|sKEYQ_SHIFT,
    "bbcceeeb",
    Edit_Bitmap_Gradient,
    Bitmap_Gradient,
    Exec_Misc_Nop,
    { KC_BITMAP },
    { 0 },
  },
  {
    "Range"         ,0x35,KC_BITMAP,1,1,0x06000103,COL_MOD,0,
    "bcc",
    Edit_Bitmap_Range,
    Bitmap_Range,
    Exec_Misc_Nop,
    { KC_BITMAP },
    { 0 },
  },
  {
    "RotateMul"     ,0x36,KC_BITMAP,1,1,0x0600010a,COL_MOD,sKEYQ_SHIFT|'R',
    "eggeebcbbc",
    Edit_Bitmap_RotateMul,
    Bitmap_RotateMul,
    Exec_Misc_Nop,
    { KC_BITMAP },
    { 0 },
  },
  {
    "Twirl"         ,0x37,KC_BITMAP,1,1,0x06000107,COL_MOD,0,
    "ggeeeeb",
    Edit_Bitmap_Twirl,
    Bitmap_Twirl,
    Exec_Misc_Nop,
    { KC_BITMAP },
    { 0 },
  },
  {
    "Sharpen"       ,0x38,KC_BITMAP,1,1,0x06000104,COL_MOD,0,
    "beee",
    Edit_Bitmap_Sharpen,
    Bitmap_Sharpen,
    Exec_Misc_Nop,
    { KC_BITMAP },
    { 0 },
  },
  {
    "GlowRect"      ,0x39,KC_BITMAP,1,1,0x0600010b,COL_MOD,'g',
    "eeeeeeceebb",
    Edit_Bitmap_GlowRect,
    Bitmap_GlowRect,
    Exec_Misc_Nop,
    { KC_BITMAP },
    { 0 },
  },
  {
    "Import"        ,0x3a,KC_BITMAP,0,0,0x46090000,COL_NEW,0,
    "",
    Edit_Bitmap_Import,
    Bitmap_Import,
    Exec_Misc_Nop,
    { KC_BITMAP },
    { 0 },
  },
  {
    "ColorBalance"  ,0x3b,KC_BITMAP,1,1,0x06000109,COL_MOD,'c',
    "eeeeeeeee",
    Edit_Bitmap_ColorBalance,
    Bitmap_ColorBalance,
    Exec_Misc_Nop,
    { KC_BITMAP },
    { 0 },
  },
  {
    "Unwrap"        ,0x3c,KC_BITMAP,1,1,0x06000101,COL_MOD,0,
    "b",
    Edit_Bitmap_Unwrap,
    Bitmap_Unwrap,
    Exec_Misc_Nop,
    { KC_BITMAP },
    { 0 },
  },
  {
    "Bricks"        ,0x3d,KC_BITMAP,0,0,0x0600000e,COL_NEW,0,
    "bbcccff" "bbbbbff",
    Edit_Bitmap_Bricks,
    Bitmap_Bricks,
    Exec_Misc_Nop,
    { KC_BITMAP },
    { 0 },
  },
  {
    "Bulge"         ,0x3e,KC_BITMAP,1,1,0x06000101,COL_MOD,0,
    "g",
    Edit_Bitmap_Bulge,
    Bitmap_Bulge,
    Exec_Misc_Nop,
    { KC_BITMAP },
    { 0 },
  },
  {
    "Render"        ,0x3f,KC_BITMAP,1,1,0x46000102,COL_NEW,0,
    "bb",
    Edit_Bitmap_Render,
    Bitmap_Render,
    Exec_Bitmap_Render,
    { KC_IPP },
    { 0 },
  },
  {
    "RenderAuto"    ,0x40,KC_BITMAP,1,1,0x46000103,COL_NEW,0,
    "bbb",
    Edit_Bitmap_RenderAuto,
    Bitmap_RenderAuto,
    Exec_Bitmap_RenderAuto,
    { KC_MINMESH },
    { 0 },
  },

/****************************************************************************/

  {
    "Dummy"         ,0x60,KC_EFFECT,0,0,0x00001003,COL_NEW,0,
    "fff",
    Edit_Effect_Dummy,
    Init_Effect_Dummy,
    Exec_Effect_Dummy,
    { 0 },
    { KC_MATERIAL },
  },
  {
    "Print"         ,0x61,KC_EFFECT,0,0,0x00011006,COL_NEW,0,
    "ifffff",
    Edit_Effect_Print,
    Init_Effect_Print,
    Exec_Effect_Print,
    { 0 },
    { KC_MATERIAL },
  },
/*
    {
    "Particles"     ,0x62,KC_EFFECT,0,0,0x00001018,COL_NEW,0,
    "bsffffffcccfffff" "ffffffff",
    Edit_Effect_Particles,
    Init_Effect_Particles,
    Exec_Effect_Particles,
    { 0 },
    { KC_MATERIAL },
  },
*/
  {
    "PartEmitter"   ,0x63,KC_SCENE,0,0,0x00000011,COL_NEW,0,
    "bbsff" "ffffffffffff",
    Edit_Effect_PartEmitter,
    Init_Effect_PartEmitter,
    Exec_Effect_PartEmitter,
    { 0 },
    { 0 },
  },
  {
    "PartSystem"    ,0x64,KC_EFFECT,0,0,0x0000100c,COL_NEW,0,
    "bscccffffffb",
    Edit_Effect_PartSystem,
    Init_Effect_PartSystem,
    Exec_Effect_PartSystem,
    { 0 },
    { KC_MATERIAL },
  },
  {
    "Chaos1"         ,0x65,KC_EFFECT,0,0,0x0000100d,COL_NEW,0,
    "fffffffffbbbi",
    Edit_Effect_Chaos1,
    Init_Effect_Chaos1,
    Exec_Effect_Chaos1,
    { 0 },
    { KC_MATERIAL },
  },
  {
    "Tourque"        ,0x66,KC_EFFECT,0,0,0x0000200f,COL_NEW,0,
    "biibffffffffffb",
    Edit_Effect_Tourque,
    Init_Effect_Tourque,
    Exec_Effect_Tourque,
    { 0 },
    { KC_MATERIAL,KC_MESH },
  },
  {
    "Stream"        ,0x67,KC_EFFECT,0,0,0x00001024,COL_NEW,0,
    "biibbfff"          // misc
    "ffffffffffffffff"  // pos
    "ffff"              // stuff
    "ffffffff",         // light
    Edit_Effect_Stream,
    Init_Effect_Stream,
    Exec_Effect_Stream,
    { 0 },
    { KC_MATERIAL },
  },
  {
    "ImgLayer"      ,0x68,KC_EFFECT,0,1,0x40090104,COL_NEW,0,
    "b-gb",
    Edit_Effect_ImageLayer,
    Init_Effect_ImageLayer,
    Exec_Effect_ImageLayer,
    { KC_BITMAP },
    { 0 },
  },
  {
    "Billboards"       ,0x69,KC_EFFECT,2,2,0x00000204,COL_NEW,0,
    "fffb",
    Edit_Effect_Billboards,
    Init_Effect_Billboards,
    Exec_Effect_Billboards,
    { KC_MINMESH,KC_MATERIAL },
    { 0 },
  },
  {
    "WalkTheSpline"       ,0x6a,KC_SCENE,2,2,0x00000206,COL_NEW,0,
    "ffibff",
    Edit_Effect_WalkTheSpline,
    Init_Effect_WalkTheSpline,
    Exec_Effect_WalkTheSpline,
    { KC_MINMESH,KC_SPLINE },
    { 0 },
  },
  {
    "ChainLine",        0x6b,KC_EFFECT,0,0,0x00001015,COL_NEW,0,
    "ffffffbbfffff"
    "fffbfffb",
    Edit_Effect_ChainLine,
    Init_Effect_ChainLine,
    Exec_Effect_ChainLine,
    { 0 },
    { KC_MATERIAL },
  },
  {
    "aRTifakktory"       ,0x6c,KC_EFFECT,0,0,0x00000005,COL_NEW,0,
    "bggbb",
    Edit_Effect_JPEG,
    Init_Effect_JPEG,
    Exec_Effect_JPEG,
    { 0 },
    { 0 },
  },
  {
    "LineShuffle"        ,0x6d,KC_EFFECT,0,0,0x00000001,COL_NEW,0,
    "b",
    Edit_Effect_LineShuffle,
    Init_Effect_LineShuffle,
    Exec_Effect_LineShuffle,
    { 0 },
    { 0 },
  },
  {
    "BillboardField", 0x6e,KC_EFFECT,1,1,0x0000010f,COL_NEW,0,
    "fffb" "fffffffibff",
    Edit_Effect_BillboardField,
    Init_Effect_BillboardField,
    Exec_Effect_BillboardField,
    { KC_MATERIAL },
    { 0 },
  },
  {
    "BP06Spirit"    , 0x6f,KC_EFFECT,0,0,0x00002011,COL_NEW,0,
    "fff" "fffffccciffffi" ,
    Edit_Effect_BP06Spirit,
    Init_Effect_BP06Spirit,
    Exec_Effect_BP06Spirit,
    { 0 },
    { KC_MATERIAL,KC_MATERIAL },
  },
  {
    "BP06Jungle"   , 0x70,KC_EFFECT,0,0,0x0000100c,COL_NEW,0,
    "bbffff" "ffffff",
    Edit_Effect_BP06Jungle,
    Init_Effect_BP06Jungle,
    Exec_Effect_BP06Jungle,
    { 0 },
    { KC_MATERIAL },
  },


/****************************************************************************/

  {
    "Cube"          ,0x81,KC_MESH,0,0,0x0600000d,COL_NEW,'q',
    //"bbbbfffeeefff",
    "bbbbggggggfff",
    Edit_Mesh_Cube,
    Mesh_Cube,
    Exec_Misc_Nop,
    { 0 },
    { 0 },
  },
  {
    "Cylinder"      ,0x82,KC_MESH,0,0,0x06000005,COL_NEW,'z',
    "bbbbb",
    Edit_Mesh_Cylinder,
    Mesh_Cylinder,
    Exec_Misc_Nop,
    { 0 },
    { 0 },
  },
  {
    "Torus"         ,0x83,KC_MESH,0,0,0x06000007,COL_NEW,'o',
    "bbffeeb",
    Edit_Mesh_Torus,
    Mesh_Torus,
    Exec_Misc_Nop,
    { 0 },
    { 0 },
  },
  {
    "Sphere"        ,0x84,KC_MESH,0,0,0x06000002,COL_NEW,'h',
    "bb",
    Edit_Mesh_Sphere,
    Mesh_Sphere,
    Exec_Misc_Nop,
    { 0 },
    { 0 },
  },

  {
    "Select All"    ,0x85,KC_MESH,1,1,0x06000102,COL_MOD,0,
    "mb",
    Edit_Mesh_SelectAll,
    Mesh_SelectAll,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },
  {
    "Select Cube"   ,0x86,KC_MESH,1,1,0x06000108,COL_MOD,0,
    "mbffffff",
    Edit_Mesh_SelectCube,
    Mesh_SelectCube,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },

  {
    "Subdivide"     ,0x87,KC_MESH,1,1,0x46000103,COL_XTR,'u',
    "bEb",
    Edit_Mesh_Subdivide,
    Mesh_Subdivide,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },
  {
    "Transform"     ,0x88,KC_MESH,1,1,0x4600010a,COL_MOD,'t',
    "bGGGGGGGGG",
    Edit_Mesh_Transform,
    Mesh_Transform,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },
  {
    "TransEx"   ,0x89,KC_MESH,1,1,0x4600010c,COL_MOD,'T'|sKEYQ_SHIFT,
    "bGGGGGGFFFbb",
    Edit_Mesh_TransformEx,
    Mesh_TransformEx,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },
  {
    "Crease"        ,0x8a,KC_MESH,1,1,0x06000103,COL_MOD,0,
    "bbb",
    Edit_Mesh_Crease,
    Mesh_Crease,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },
  {
    "UnCrease"      ,0x8b,KC_MESH,1,1,0x06000103,COL_MOD,0,
    "bbb",
    Edit_Mesh_UnCrease,
    Mesh_UnCrease,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },
  {
    "Triangulate"   ,0x8c,KC_MESH,1,1,0x06000103,COL_XTR,0,
    "bbb",
    Edit_Mesh_Triangulate,
    Mesh_Triangulate,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },
  {
    "Cut"           ,0x8d,KC_MESH,1,1,0x06000104,COL_XTR,'x',
    "eefb",
    Edit_Mesh_Cut,
    Mesh_Cut,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },
  {
    "Extrude Normal",0x8e,KC_MESH,1,1,0x46000102,COL_XTR,'E'|sKEYQ_SHIFT,
    "bF",
    Edit_Mesh_ExtrudeNormal,
    Mesh_ExtrudeNormal,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },
  {
    "Displace"      ,0x8f,KC_MESH,2,2,0x46000204,COL_XTR,'d',
    "bFFF",
    Edit_Mesh_Displace,
    Mesh_Displace,
    Exec_Misc_Nop,
    { KC_MESH,KC_BITMAP },
    { 0 },
  },
  {
    "Bevel"         ,0x90,KC_MESH,1,1,0x46000104,COL_XTR,0,
    "bFFb",
    Edit_Mesh_Bevel,
    Mesh_Bevel,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },
  {
    "Perlin"        ,0x91,KC_MESH,1,1,0x4600010d,COL_XTR,'p',
    "bFFFEEEFFFFFF",
    Edit_Mesh_Perlin,
    Mesh_Perlin,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },
  {
    "Add"           ,0x92,KC_MESH,1,KK_MAXINPUT,0x07000000,COL_ADD,'a',
    "",
    Edit_Mesh_Add,
    Mesh_Add,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },
  {
    "Delete Faces"  ,0x93,KC_MESH,1,1,0x06000101,COL_MOD,'D'|sKEYQ_SHIFT,
    "b",
    Edit_Mesh_DeleteFaces,
    Mesh_DeleteFaces,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },
  {
    "Select Random" ,0x94,KC_MESH,1,1,0x06000104,COL_MOD,'r',
    "mbbb",
    Edit_Mesh_SelectRandom,
    Mesh_SelectRandom,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },
  {
    "Multiply"      ,0x95,KC_MESH,1,1,0x46000111,COL_ADD,'M'|sKEYQ_SHIFT,
    //"FFFEEEFFFbbFFEEEF",
    "GGGGGGFFFbbGGGGGGF",
    Edit_Mesh_Multiply,
    Mesh_Multiply,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },

  {
    "BeginRec" ,0x99,KC_MESH,1,1,0x06000100,COL_ADD,0,
    "",
    Edit_Mesh_BeginRecord,
    Mesh_BeginRecord,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },
  {
    "Extrude"  ,0x9a,KC_MESH,1,1,0x4600010d,COL_XTR,'e',
    "bsbbFFFFFFEEE",
    Edit_Mesh_Extrude,
    Mesh_Extrude,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },
  {
    "CollCube" ,0x9c,KC_MESH,0,1,0x1600210e,COL_NEW,'Q'|sKEYQ_SHIFT,
    "FFFFFFbbbbbbbi",
    //"GGGGGGbbbbbbbi",
    Edit_Mesh_CollisionCube,
    Mesh_CollisionCube,
    Exec_Misc_Nop,
    { KC_MESH },
    { KC_ANY,KC_SCENE },
  },
  {
    "Grid"     ,0x9d,KC_MESH,0,0,0x06000003,COL_NEW,'g',
    "bbb",
    Edit_Mesh_Grid,
    Mesh_Grid,
    Exec_Misc_Nop,
    { 0 },
    { 0 },
  },
  /*{
    "Lightmap"      ,0x9e,KC_MESH,1,1,0x06000104,COL_XTR,0,
    "",
    Edit_Mesh_Lightmap,
    Mesh_Lightmap,
    Exec_Misc_Nop,
    { KC_MESH },
  },
  {
    "MapLight"      ,0x9f,KC_MESH,1,1,0x06000105,COL_XTR,0,
    "ffffc",
    Edit_Mesh_MapLight,
    Mesh_MapLight,
    Exec_Misc_Nop,
    { KC_MESH },
  },*/
  {
    "Bend"     ,0xa0,KC_MESH,1,1,0x46000118,COL_XTR,'b',
    "b"
    "GGGGGGFFF"
    "GGGGGGFFF"
    /*"FFFEEEFFF"
    "FFFEEEFFF"*/
    "EEFF"
    "b",
    Edit_Mesh_Bend,
    Mesh_Bend,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },
  {
    "Select Logic" ,0xa1,KC_MESH,1,1,0x06000104,COL_MOD,0,
    "mmmb",
    Edit_Mesh_SelectLogic,
    Mesh_SelectLogic,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },
  {
    "Select Grow" , 0xa2,KC_MESH,1,1,0x06000104,COL_MOD,0,
    "ssbb",
    Edit_Mesh_SelectGrow,
    Mesh_SelectGrow,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },
  {
    "Invert" , 0xa3,KC_MESH,1,1,0x06000100,COL_MOD,0,
    "",
    Edit_Mesh_Invert,
    Mesh_Invert,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },
  {
    "Set Pivot" ,0xa4,KC_MESH,1,1,0x06000104,COL_MOD,0,
    "bfff",
    Edit_Mesh_SetPivot,
    Mesh_SetPivot,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },
  {
    "UVProject" ,0xa5,KC_MESH,1,1,0x0600010b,COL_MOD,0,
    "bfffffffffb",
    //"beeeeeefffb",
    Edit_Mesh_UVProjection,
    Mesh_UVProjection,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },
  {
    "Center"    ,0xa6,KC_MESH,1,1,0x06000102,COL_MOD,'c',
    "bb",
    Edit_Mesh_Center,
    Mesh_Center,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },
  {
    "AutoColl"  ,0xa7,KC_MESH,1,1,0x06000106,COL_MOD,'A'|sKEYQ_SHIFT,
    "fbbbb",
    Edit_Mesh_AutoCollision,
    Mesh_AutoCollision,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },
  {
    "SelectSphere"  ,0xa8,KC_MESH,1,1,0x06000108,COL_MOD,0,
    "mbffffff",
    Edit_Mesh_SelectSphere,
    Mesh_SelectSphere,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },
  {
    "SelectFace"    ,0xa9,KC_MESH,1,1,0x06000103,COL_MOD,0,
    "bbi",
    Edit_Mesh_SelectFace,
    Mesh_SelectFace,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },
  {
    "SelectAngle"   ,0xaa,KC_MESH,1,1,0x06000105,COL_MOD,0,
    "bbeee",
    Edit_Mesh_SelectAngle,
    Mesh_SelectAngle,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },
  {
    "Bend 2"        ,0xab,KC_MESH,1,1,0x06000108,COL_XTR,'B'|sKEYQ_SHIFT,
    "fffeeeff",
    Edit_Mesh_Bend2,
    Mesh_Bend2,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },
  {
    "SmAngle"       ,0xac,KC_MESH,1,1,0x06000101,COL_MOD,0,
    "f",
    Edit_Mesh_SmoothAngle,
    Mesh_SmoothAngle,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },
  {
    "Color"         ,0xad,KC_MESH,1,1,0x06000109,COL_MOD,0,
    "fffeecffb",
    Edit_Mesh_Color,
    Mesh_Color,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },
  {
    "Bend Spline"   ,0xae,KC_MESH,1,1,0x06100107,COL_XTR,0,
    "fffeeef",
    Edit_Mesh_BendS,
    Mesh_BendS,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },
  {
    "Light Slot"    ,0xaf,KC_MESH,0,1,0x06000006,COL_MOD,0,
    "fffcff",
    Edit_Mesh_LightSlot,
    Mesh_LightSlot,
    Exec_Misc_Nop,
    { 0 },
    { 0 },
  },
  {
    "ShadowEnable"  ,0xb0,KC_MESH,1,1,0x06000101,COL_MOD,0,
    "b",
    Edit_Mesh_ShadowEnable,
    Mesh_ShadowEnable,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },
  {
    "XSI"           ,0xb1,KC_MESH,0,1,0x06010000,COL_NEW,'X'|sKEYQ_SHIFT,
    "",
    Edit_Mesh_XSI,
    Mesh_XSI,
    Exec_Misc_Nop,
    { 0 },
    { 0 },
  },
  {
    "BSP"           ,0xb2,KC_SMESH,1,1,0x06000100,COL_MOD,0,
    "",
    Edit_Misc_Nop,
    Mesh_BSP,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },
  {
    "SingleVert"    ,0xb3,KC_MESH,0,1,0x06000000,COL_NEW,'V'|sKEYQ_SHIFT,
    "",
    Edit_Mesh_SingleVert,
    Mesh_SingleVert,
    Exec_Misc_Nop,
    { 0 },
    { 0 },
  },
  {
    "Multiply 2"    ,0xb4,KC_MESH,1,KK_MAXINPUT,0x07000014,COL_ADD,0,
    "bbbbgggbbbgggbbbbggg",
    Edit_Mesh_Multiply2,
    Mesh_Multiply2,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },
  {
    "<- Mesh"    ,0xb5,KC_MINMESH,1,1,0x06000100,COL_MOD,0,
    "",
    Edit_Mesh_ToMin,
    Mesh_ToMin,
    Exec_Misc_Nop,
    { KC_MESH },
    { 0 },
  },
    
/****************************************************************************/
/****************************************************************************/

  {
    "SingleVert"    ,0x100,KC_MINMESH,0,1,0x06000000,COL_NEW,'V'|sKEYQ_SHIFT,
    "",
    Edit_MinMesh_SingleVert,
    MinMesh_SingleVert,
    Exec_Misc_Nop,
    { 0 },
    { 0 },
  },
  {
    "Grid"          ,0x101,KC_MINMESH,0,0,0x06000003,COL_NEW,'g',
    "bbb",
    Edit_MinMesh_Grid,
    MinMesh_Grid,
    Exec_Misc_Nop,
    { 0 },
    { 0 },
  },
  {
    "Cube"          ,0x102,KC_MINMESH,0,0,0x0600000d,COL_NEW,'q',
    "bbbbggggggfff",
    Edit_MinMesh_Cube,
    MinMesh_Cube,
    Exec_Misc_Nop,
    { 0 },
    { 0 },
  },
  {
    "Torus"         ,0x103,KC_MINMESH,0,0,0x06000007,COL_NEW,'o',
    "bbffeeb",
    Edit_MinMesh_Torus,
    MinMesh_Torus,
    Exec_Misc_Nop,
    { 0 },
    { 0 },
  },
  {
    "Sphere"        ,0x104,KC_MINMESH,0,0,0x06000002,COL_NEW,'h',
    "bb",
    Edit_MinMesh_Sphere,
    MinMesh_Sphere,
    Exec_Misc_Nop,
    { 0 },
    { 0 },
  },
  {
    "Cylinder"      ,0x105,KC_MINMESH,0,0,0x06000005,COL_NEW,'z',
    "bbbbb",
    Edit_MinMesh_Cylinder,
    MinMesh_Cylinder,
    Exec_Misc_Nop,
    { 0 },
    { 0 },
  },
  {
    "XSI"           ,0x106,KC_MINMESH,0,1,0x06010000,COL_NEW,'X'|sKEYQ_SHIFT,
    "",
    Edit_MinMesh_XSI,
    MinMesh_XSI,
    Exec_Misc_Nop,
    { 0 },
    { 0 },
  },
  {
    "MTetra"        ,0x107,KC_MINMESH,0,1,0x06010001,COL_NEW,0,
    "g",
    Edit_MinMesh_MTetra,
    MinMesh_MTetra,
    Exec_Misc_Nop,
    { 0 },
    { 0 },
  },
  {
    "Kopuli"        ,0x10f,KC_MINMESH,0,1,0x06000000,COL_NEW,0,
    "",
    Edit_MinMesh_Kopuli,
    MinMesh_Kopuli,
    Exec_Misc_Nop,
    { 0 },
    { 0 },
  },

/****************************************************************************/

  {
    "MatLink"       ,0x110,KC_MINMESH,1,1,0x06001102,COL_MOD,'m',
    "bb",
    Edit_MinMesh_MatLink,
    MinMesh_MatLink,
    Exec_Misc_Nop,
    { KC_MINMESH },
    { KC_MATERIAL },
  },
  {
    "Add"           ,0x111,KC_MINMESH,1,KK_MAXINPUT,0x07000000,COL_ADD,'a',
    "",
    Edit_MinMesh_Add,
    MinMesh_Add,
    Exec_Misc_Nop,
    { KC_MINMESH },
    { 0 },
  },
  {
    "Select All"     ,0x112,KC_MINMESH,1,1,0x06000101,COL_MOD,0,
    "b", 
    Edit_MinMesh_SelectAll,
    MinMesh_SelectAll,
    Exec_Misc_Nop,
    { KC_MINMESH },
    { KC_MATERIAL },
  }, 
  {
    "Select Cube"    ,0x113,KC_MINMESH,1,1,0x06000107,COL_MOD,0,
    "bffffff",
    Edit_MinMesh_SelectCube,
    MinMesh_SelectCube,
    Exec_Misc_Nop,
    { KC_MINMESH },
    { 0 },
  },
  {
    "Delete Faces"   ,0x114,KC_MINMESH,1,1,0x06000100,COL_MOD,0,
    "", 
    Edit_MinMesh_DeleteFaces,
    MinMesh_DeleteFaces,
    Exec_Misc_Nop,
    { KC_MINMESH },
    { KC_MATERIAL },
  }, 
  {
    "Invert"         ,0x115,KC_MINMESH,1,1,0x06000100,COL_MOD,0,
    "b", 
    Edit_MinMesh_Invert,
    MinMesh_Invert,
    Exec_Misc_Nop,
    { KC_MINMESH },
    { KC_MATERIAL },
  }, 
  {
    "MatLinkId"      ,0x116,KC_MINMESH,1,1,0x06001102,COL_MOD,0,
    "bb",
    Edit_MinMesh_MatLinkId,
    MinMesh_MatLinkId,
    Exec_Misc_Nop,
    { KC_MINMESH },
    { KC_MATERIAL },
  },
  {
    "MatInput"       ,0x117,KC_MINMESH,2,2,0x06000202,COL_MOD,'M'|sKEYQ_SHIFT,
    "bb",
    Edit_MinMesh_MatInput,
    MinMesh_MatLink,
    Exec_Misc_Nop,
    { KC_MINMESH,KC_MATERIAL },
    { 0 },
  },

/****************************************************************************/

  {
    "Transform"     ,0x120,KC_MINMESH,1,1,0x0600010a,COL_MOD,'t',
    "bGGGGGGGGG",
    Edit_MinMesh_Transform,
    MinMesh_TransformEx,
    Exec_Misc_Nop,
    { KC_MINMESH },
    { 0 },
  },
  {
    "TransEx"       ,0x121,KC_MINMESH,1,1,0x0600010c,COL_MOD,'T'|sKEYQ_SHIFT,
    "bGGGGGGFFFbb",
    Edit_MinMesh_TransformEx,
    MinMesh_TransformEx,
    Exec_Misc_Nop,
    { KC_MINMESH },
    { 0 },
  },

  {
    "Extrude Normal",0x122,KC_MINMESH,1,1,0x06000102,COL_XTR,'E'|sKEYQ_SHIFT,
    "bF",
    Edit_MinMesh_ExtrudeNormal,
    MinMesh_ExtrudeNormal,
    Exec_Misc_Nop,
    { KC_MINMESH },
    { 0 },
  },
  {
    "Displace"      ,0x123,KC_MINMESH,2,2,0x06000204,COL_XTR,'d',
    "bFFF",
    Edit_MinMesh_Displace,
    MinMesh_Displace,
    Exec_Misc_Nop,
    { KC_MINMESH,KC_BITMAP },
    { 0 },
  },
  {
    "Perlin"        ,0x124,KC_MINMESH,1,1,0x0600010d,COL_XTR,'p',
    "bFFFEEEFFFFFF",
    Edit_MinMesh_Perlin,
    MinMesh_Perlin,
    Exec_Misc_Nop,
    { KC_MINMESH },
    { 0 },
  },
  {
    "Bend 2"        ,0x125,KC_MINMESH,1,1,0x06000108,COL_XTR,'B'|sKEYQ_SHIFT,
    "fffeeeff",
    Edit_MinMesh_Bend2,
    MinMesh_Bend2,
    Exec_Misc_Nop,
    { KC_MINMESH },
    { 0 },
  },
  {
    "Extrude"        ,0x126,KC_MINMESH,1,1,0x0600105,COL_XTR,'e',
    "bbggg",
    Edit_MinMesh_Extrude,
    MinMesh_Extrude,
    Exec_Misc_Nop,
    { KC_MINMESH },
    { 0 },
  },
  {
    "BakeAnimation"  ,0x127,KC_MINMESH,1,1,0x06000102,COL_XTR,0,
    "FF",
    Edit_MinMesh_BakeAnimation,
    MinMesh_BakeAnimation,
    Exec_Misc_Nop,
    { KC_MINMESH },
    { 0 },
  },
  {
    "BoneChain"  ,0x128,KC_MINMESH,1,1,0x06000108,COL_XTR,0,
    "ffffffib",
    Edit_MinMesh_BoneChain,
    MinMesh_BoneChain,
    Exec_Misc_Nop,
    { KC_MINMESH },
    { 0 },
  },
  {
    "BoneTrain"  ,0x129,KC_MINMESH,2,3,0x06000202,COL_XTR,0,
    "fb",
    Edit_MinMesh_BoneTrain,
    MinMesh_BoneTrain,
    Exec_Misc_Nop,
    { KC_MINMESH,KC_SPLINE,KC_SPLINE },
    { 0 },
  },
  {
    "Triangulate",0x12a,KC_MINMESH,1,1,0x06000100,COL_MOD,0,
    "",
    Edit_MinMesh_Triangulate,
    MinMesh_Triangulate,
    Exec_Misc_Nop,
    { KC_MINMESH },
    { 0 },
  },
  /*{
    "Compress"  ,0x12b,KC_MINMESH,1,1,0x06000100,COL_MOD,0,
    "",
    Edit_MinMesh_Compress,
    MinMesh_Compress,
    Exec_Misc_Nop,
    { KC_MINMESH },
    { 0 },
  },*/
  {
    "Pipe",0x12c,KC_MINMESH,2,4,0x06000403,COL_MOD,0,
    "bff",
    Edit_MinMesh_Pipe,
    MinMesh_Pipe,
    Exec_Misc_Nop,
    { KC_SPLINE,KC_MINMESH,KC_MINMESH,KC_MINMESH },
    { 0 },
  },
  {
    "ScaleAnim"       ,0x12d,KC_MINMESH,1,1,0x06000101,COL_MOD,0,
    "f",
    Edit_MinMesh_ScaleAnim,
    MinMesh_ScaleAnim,
    Exec_Misc_Nop,
    { KC_MINMESH },
    { 0 },
  },
  {
    "RenderAutoMap"     ,0x12e,KC_MINMESH,1,1,0x06000101,COL_MOD,0,
    "b",
    Edit_MinMesh_RenderAutoMap,
    MinMesh_RenderAutoMap,
    Exec_Misc_Nop,
    { KC_MINMESH },
    { 0 },
  },
  {
    "Multiply"          ,0x12f,KC_MINMESH,1,1,0x46000111,COL_ADD,0,
    "GGGGGGFFFbbGGGGGGF",
    Edit_MinMesh_Multiply,
    MinMesh_Multiply,
    Exec_Misc_Nop,
    { KC_MINMESH },
    { 0 },
  },
  {
    "Multiply 2"        ,0x130,KC_MINMESH,1,KK_MAXINPUT,0x07000014,COL_ADD,0,
    "bbbbgggbbbgggbbbbggg",
    Edit_MinMesh_Multiply2,
    MinMesh_Multiply2,
    Exec_Misc_Nop,
    { KC_MINMESH },
    { 0 },
  },
  {
    "Center"            ,0x131,KC_MINMESH,1,1,0x06000101,COL_MOD,0,
    "b",
    Edit_MinMesh_Center,
    MinMesh_Center,
    Exec_Misc_Nop,
    { KC_MINMESH },
    { 0 },
  },
  {
    "Select Logic"      ,0x132,KC_MINMESH,1,1,0x06000101,COL_MOD,0,
    "b",
    Edit_MinMesh_SelectLogic,
    MinMesh_SelectLogic,
    Exec_Misc_Nop,
    { KC_MINMESH },
    { 0 },
  },

/****************************************************************************/
/****************************************************************************/

  {
    "Scene"         ,0xc0,KC_SCENE,0,1,0x0000010a,COL_NEW,'n',
    //"fffeeefffb",
    "ggggggfffb",
    Edit_Scene_Scene,
    Init_Scene_Scene,
    Exec_Scene_Scene,
    { KC_ANY },
    { 0 },
  },
  {
    "Add"           ,0xc1,KC_SCENE,1,15,0x05000000,COL_ADD,'a',
    "",
    Edit_Scene_Add,
    Init_Scene_Add,
    Exec_Scene_Add,
    { KC_ANY },
    { 0 },
  },
  {
    "Multiply"      ,0xc2,KC_SCENE,1,1,0x0000010a,COL_MOD,'m',
    //"fffeeefffb",
    "ggggggfffb",
    Edit_Scene_Multiply,
    Init_Scene_Multiply,
    Exec_Scene_Multiply,
    { KC_ANY },
    { 0 },
  },
  {
    "Transform"     ,0xc3,KC_SCENE,1,1,0x00000109,COL_MOD,'t',
    //"fffeeefff",
    "ggggggfff",
    Edit_Scene_Transform,
    Init_Scene_Transform,
    Exec_Scene_Transform,
    { KC_ANY },
    { 0 },
  },
  {
    "Light"         ,0xc4,KC_SCENE,0,0,0x0000000b,COL_MOD,'L'|sKEYQ_SHIFT,
    "eeefffbecff",
    Edit_Scene_Light,
    Init_Scene_Light,
    Exec_Scene_Light,
    { 0 },
    { 0 },
  },
/*{
    "MapLight"      ,0xc5,KC_SCENE,1,1,0x0000010b,COL_MOD,'M'|sKEYQ_SHIFT,
    "ffffffffffc",
    Edit_Scene_MapLight,
    Init_Scene_MapLight,
    Exec_Scene_MapLight,
    { KC_SCENE },
    { 0 },
  },*/
  {
    "Particles"      ,0xc5,KC_SCENE,1,1,0x00100110,COL_MOD,0,   // c5 was MapLight!
    //"fffeeefffb",
    "bibgggggggggf"
    "ggg",
    Edit_Scene_Particles,
    Init_Scene_Particles,
    Exec_Scene_Particles,
    { KC_ANY },
    { 0 },
  },
  {
    "Camera"         ,0xc6,KC_SCENE,0,0,0x00000009,COL_NEW,0,
    "fffeeefff",
    Edit_Scene_Camera,
    Init_Scene_Camera,
    Exec_Scene_Camera,
    { 0 },
    { 0 },
  },
  {
    "Limb"          ,0xc7,KC_SCENE,2,2,0x00000209,COL_XTR,0,
    "ffffffffb",
    Edit_Scene_Limb,
    Init_Scene_Limb,
    Exec_Scene_Limb,
    { KC_ANY,KC_ANY },
    { 0 },
  },
  {
    "Walk"          ,0xc8,KC_SCENE,1,1,0x00100127,COL_XTR,0,
    "bbfffbbffffff"
    "ffffffff" "ffffffff" "ffffffff"
    "ff",
    Edit_Scene_Walk,
    Init_Scene_Walk,
    Exec_Scene_Walk,
    { KC_SCENE,KC_SCENE },
    { 0 },
  },
  {
    "Rotate"        ,0xc9,KC_SCENE,1,1,0x00000104,COL_XTR,0,
    "fffb",
    Edit_Scene_Rotate,
    Init_Scene_Rotate,
    Exec_Scene_Rotate,
    { KC_SCENE },
    { 0 },
  },
  {
    "Forward"       ,0xca,KC_SCENE,1,1,0x00000101,COL_XTR,0,
    "f",
    Edit_Scene_Forward,
    Init_Scene_Forward,
    Exec_Scene_Forward,
    { KC_SCENE },
    { 0 },
  },
  {
    "Sector"        ,0xcb,KC_SCENE,1,1,0x00000100,COL_MOD,0,
    "",
    Edit_Scene_Sector,
    Init_Scene_Sector,
    Exec_Scene_Sector,
    { KC_SCENE },
    { 0 },
  },
  {
    "Portal"        ,0xcd,KC_SCENE,0,1,0x00003108,COL_ADD,0,
    "ffffffbe",
    Edit_Scene_Portal,
    Init_Scene_Portal,
    Exec_Scene_Portal,
    { KC_SCENE },
    { KC_SCENE,KC_SCENE,KC_SCENE },
  },
  {
    "Physic"        ,0xce,KC_SCENE,0,1,0x0000010c,COL_XTR,0,
    "bffffff" "ffffb",
    Edit_Scene_Physic,
    Init_Scene_Physic,
    Exec_Scene_Physic,
    { KC_SCENE },
    { 0 },
  },
  {
    "ForceLights"   ,0xcf,KC_SCENE,1,1,0x00000102,COL_XTR,0,
    "bb",
    Edit_Scene_ForceLights,
    Init_Scene_ForceLights,
    Exec_Scene_ForceLights,
    { KC_SCENE },
    { 0 },
  },
  {
    "Adjust Pass"     ,0x180,KC_SCENE,1,1,0x00000101,COL_MOD,0,
    "b",
    Edit_Scene_AdjustPass,
    Init_Scene_AdjustPass,
    Exec_Scene_AdjustPass,
    { KC_ANY },
    { 0 },
  },
  {
    "Apply Spline"    ,0x181,KC_SCENE,2,2,0x00000201,COL_MOD,0,
    "f",
    Edit_Scene_ApplySpline,
    Init_Scene_ApplySpline,
    Exec_Scene_ApplySpline,
    { KC_SCENE,KC_SPLINE },
    { 0 },
  },
  {
    "Marker"          ,0x182,KC_SCENE,0,1,0x00000101,COL_NEW,0,
    "b",
    Edit_Scene_Marker,
    Init_Scene_Marker,
    Exec_Scene_Marker,
    { KC_SCENE },
    { 0 },
  },
  {
    "LOD"             ,0x183,KC_SCENE,1,2,0x00000201,COL_ADD,0,
    "f",
    Edit_Scene_LOD,
    Init_Scene_LOD,
    Exec_Scene_LOD,
    { KC_ANY,KC_ANY },
    { 0 },
  },
  {
    "Ambient"         ,0x184,KC_SCENE,0,0,0x00000001,COL_MOD,'A'|sKEYQ_SHIFT,
    "c",
    Edit_Scene_Ambient,
    Init_Scene_Ambient,
    Exec_Scene_Ambient,
    { 0 },
    { 0 },
  },

  /****************************************************************************/

  {
    "MatLink",0x96,KC_MESH,1,1,0x06001102,COL_MOD,'m',
    "bb",
    Edit_Mesh_MatLink,
    Mesh_MatLink,
    Exec_Misc_Nop,
    { KC_MESH },
    { KC_MATERIAL },
  },
  {
    "MatHack",0xd2,KC_SCENE,0,0,0x00001000,COL_XTR,0,
    "",
    Edit_Scene_MatHack,
    Init_Scene_MatHack,
    Exec_Scene_MatHack,
    { 0 },
    { KC_MATERIAL },
  },
  {
    "Material"     ,0xd0,KC_MATERIAL,0,1,0x26007140,COL_MOD,'M'|sKEYQ_SHIFT,
    "iii-" // base special light pad
    "iiii" // tflags
    "iiiiiiiiiiiii--i" // combiners
    "ffffccccf-" // uvscale, colors, specpower, pad
    "fffeeefff" // uvxform1
    "ffeff" // uvxform2
    "iiifii--" // multimisc
    "iiiiffff", // multitflags/tscale
    Edit_Material_Material,
    Init_Material_Material,
    Exec_Material_Material,
    { KC_MATERIAL },
    { KC_BITMAP,KC_BITMAP,KC_BITMAP,KC_BITMAP,
      KC_BITMAP,KC_BITMAP,KC_BITMAP, },
  },
  {
    "Add Material" ,0xd1,KC_MATERIAL,1,KK_MAXINPUT,0x07000000,COL_MOD,'a',
    "",
    Edit_Material_Add,
    Material_Add,
    Exec_Misc_Nop,
    { KC_MATERIAL },
  },
  {
    "Material 2.0" ,0xd3,KC_MATERIAL,0,3,0x20007326,COL_MOD,'m',
    "iccgg---"    // general section
    "iiiigggg"    // texture section
    "iii-ggg-"    // bump/lighting section
    "ggggggggg"   // uvxform 1
    "ggggg",      // uvxform2
    Edit_Material_Material20,
    Init_Material_Material20,
    Exec_Misc_Nop,
    { KC_BITMAP,KC_BITMAP,KC_BITMAP },
    { KC_BITMAP,KC_BITMAP,KC_BITMAP,KC_BITMAP,
      KC_BITMAP,KC_BITMAP,KC_BITMAP},
  },

  /****************************************************************************/

  {
    "Viewport"      ,0xf0,KC_IPP,1,2,0x08000218,COL_NEW,'v',
    "bicggggggffffffcffggffff",
    Edit_IPP_Viewport,
    Init_IPP_Viewport,
    Exec_IPP_Viewport,
    { KC_SCENE,KC_SPLINE },
    { 0 },
  },
  {
    "Blur"          ,0xe3,KC_IPP,1,1,0x08000105,COL_MOD,'b',
    "bfebb",
    Edit_IPP_Blur,
    Init_IPP_Blur,
    Exec_IPP_Blur,
    { KC_IPP },
    { 0 },
  },
  {
    "Copy"          ,0xe2,KC_IPP,1,1,0x08000103,COL_MOD,'C'|sKEYQ_SHIFT,
    "bcf",
    Edit_IPP_Copy,
    Init_IPP_Copy,
    Exec_IPP_Copy,
    { KC_IPP },
    { 0 },
  },
  {
    "Crashzoom"     ,0xe4,KC_IPP,1,1,0x08000106,COL_MOD,'z',
    "bbgeee",
    Edit_IPP_Crashzoom,
    Init_IPP_Crashzoom,
    Exec_IPP_Crashzoom,
    { KC_IPP },
    { 0 },
  },
  {
    "Sharpen"       ,0xe5,KC_IPP,1,1,0x08000104,COL_MOD,0,
    "becb",
    Edit_IPP_Sharpen,
    Init_IPP_Sharpen,
    Exec_IPP_Sharpen,
    { KC_IPP },
    { 0 },
  },
  {
    "Color"         ,0xe6,KC_IPP,1,1,0x08000104,COL_ADD,'c',
    "bcbc",
    Edit_IPP_Color,
    Init_IPP_Color,
    Exec_IPP_Color,
    { KC_IPP },
    { 0 },
  },
  {
    "Merge"         ,0xe7,KC_IPP,2,2,0x08000204,COL_ADD,'m',
    "bbbc",
    Edit_IPP_Merge,
    Init_IPP_Merge,
    Exec_IPP_Merge,
    { KC_IPP,KC_IPP },
    { 0 },
  },
  {
    "Mask"          ,0xe8,KC_IPP,3,3,0x08000301,COL_XTR,'M'|sKEYQ_SHIFT,
    "b",
    Edit_IPP_Mask,
    Init_IPP_Mask,
    Exec_IPP_Mask,
    { KC_IPP,KC_IPP,KC_IPP },
    { 0 },
  },
  {
    "Layer2D"       ,0xe9,KC_IPP,0,1,0x0800110b,COL_MOD,'L'|sKEYQ_SHIFT,
    "bfffffffffb",
    Edit_IPP_Layer2D,
    Init_IPP_Layer2D,
    Exec_IPP_Layer2D,
    { KC_IPP },
    { KC_MATERIAL },
  },
  {
    "Select"        ,0xea,KC_IPP,1,KK_MAXINPUT,0x01000000,COL_ADD,0,
    "",
    Edit_IPP_Select,
    Init_IPP_Select,
    Exec_IPP_Select,
    { KC_IPP },
    { 0 },
  },
  {
    "Rendertarget"  ,0xeb,KC_IPP,1,1,0x00001100,COL_MOD,0,
    "",
    Edit_IPP_RenderTarget,
    Init_IPP_RenderTarget,
    Exec_IPP_RenderTarget,
    { KC_IPP },
    { KC_BITMAP },
  },
  {
    "HSCB"          ,0xec,KC_IPP,1,1,0x08000105,COL_ADD,0,
    "bgggg",
    Edit_IPP_HSCB,
    Init_IPP_HSCB,
    Exec_IPP_HSCB,
    { KC_IPP },
    { 0 },
  },
  {
    "JPEG"          ,0xed,KC_IPP,1,1,0x08000103,COL_ADD,0,
    "bbg",
    Edit_IPP_JPEG,
    Init_IPP_JPEG,
    Exec_IPP_JPEG,
    { KC_IPP },
    { 0 },
  },
    
/****************************************************************************/

  {
    0 
  }
};

/****************************************************************************/
/****************************************************************************/
