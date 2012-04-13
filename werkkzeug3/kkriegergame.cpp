// This file is distributed under a BSD license. See LICENSE.txt for details.

#include "kkriegergame.hpp"
#include "genmesh.hpp"
#include "engine.hpp"
#include "genoverlay.hpp"

#if sPROFILE
#include "_util.hpp"

sMAKEZONE(GameLogic   ,"GameLogic"  ,0xff224466);
sMAKEZONE(GameCollide ,"GameCollide",0xff336699);
sMAKEZONE(GamePhysic  ,"GamePhysic" ,0xff55aaff);
sMAKEZONE(GameAI      ,"GameAI"     ,0xff77eeff);
#endif

//#define OVERLAP   (0.005f)
//#define EPSILON   (0.0001f)
#define EPSILON   (4.010e-3f) // determined from level size
#define OVERLAP   (EPSILON*2)
#define MOVE_EPSILON  (1e-6f)
#define DOUBLECHECK 0

#if !sPROFILE
#define GAMEPERF(x) ;
#else
#define GAMEPERF(x) x++;
sInt KKriegerGame::CallFindFirstIntersect;
sInt KKriegerGame::CallFindSubIntersect;
sInt KKriegerGame::CallMoveParticle;
sInt KKriegerGame::CallOutsideMask;
sInt KKriegerGame::CallIntersectOut;
sInt KKriegerGame::CallIntersectIn;
#endif

#pragma lekktor(off)

//static sInt SomethingBadHasHappened;    // collision panic

/****************************************************************************/

KKriegerMesh::KKriegerMesh(GenMesh *mesh)
{
  CellCount = mesh->Coll.Count;
  Cell = new KKriegerMeshCell[CellCount];

  for(sInt i=0;i<CellCount;i++)
  {
    GenMeshColl *inCell = &mesh->Coll[i];
    KKriegerMeshCell *outCell = &Cell[i];

    for(sInt j=0;j<8;j++)
      outCell->Vert[j] = mesh->VertPos(inCell->Vert[j]);

    outCell->Mode = inCell->Mode;
    outCell->Logic = inCell->Logic;
  }
}

KKriegerMesh::~KKriegerMesh()
{
  delete[] Cell;
}

void  KKriegerMesh::Copy(KObject *)
{
  // KKriegerMeshes should *never* get copied around
  sVERIFYFALSE;
}

/****************************************************************************/

#define LOGGING 0

/****************************************************************************/
/****************************************************************************/

void KKriegerMonster::Hit(sInt hits,KKriegerGame *game)
{
//  static sInt HitSounds[] = { 53,51,45,39 };

  if(Life>0)
  {
    if(State==KMS_INRANGE)
      State = KMS_ACTIVE;
    if(hits>Armor)
      Life -= (hits-Armor)+(Armor/4);
    else
      Life -= hits/4;
//    sDPrintF("monster hit for %d, -> %d\n",hits,Life);
    if(Life<=0)
    {
      Life = 0;
      if(KillSwitch>0)
        game->Switches[KillSwitch]++;
    }

    //sSystem->SamplePlay(HitSounds[GetType()],0.6f);
  }
}

void KKriegerMonster::Sound(sInt handle,sF32 volume)
{
  sVector zero;
  sInt buf;

  zero.Init(0,0,0,0);
#if !sPLAYER
  if(GenOverlayManager->SoundEnable)
#endif
  {
    buf = sSystem->SamplePlay(handle,volume);
    sSystem->Sample3DParam(handle,buf,Collider.Pos,zero,0.2f,160.0f);
  }
}

/****************************************************************************/

void KKriegerPlayer::Init()
{
  Life = 100;
  Armor = 0;
  Alpha = 0;
  Beta = 0;
  Ammo[0] = 100;
  Ammo[1] = 50;
  Ammo[2] = 0;
  Ammo[3] = 0;
  Weapon[0] = 1;
  Weapon[1] = 1;
  Weapon[2] = 1;
  Weapon[3] = 0;
  Weapon[4] = 0;
  Weapon[5] = 0;
  Weapon[6] = 0;
  Weapon[7] = 0;
  LifeMax = 100;
  ArmorMax = 100;
  AlphaMax = 100;
  BetaMax = 8;
  AmmoMax[0] = 500;
  AmmoMax[1] = 200;
  AmmoMax[2] = 100;
  AmmoMax[3] = 1000;
  WeaponMax[0] = 1;
  WeaponMax[1] = 1;
  WeaponMax[2] = 1;
  WeaponMax[3] = 1;
  WeaponMax[4] = 1;
  WeaponMax[5] = 1;
  WeaponMax[6] = 1;
  WeaponMax[7] = 1;

  CurrentWeapon = 1;
  NextWeapon = 0;
  FireKey = 0;
  UseKey = 0;
  CoolTimer = 0;
}

void KKriegerPlayer::Hit(sInt hits)
{
  if(hits>Armor)
    Life -= (hits-Armor)+(Armor/4);
  else
    Life -= hits/4;
  if(Life<0)
    Life = 0;
  if(hits>4)
    Sound(10);
}

void KKriegerPlayer::Sound(sInt handle,sF32 volume)
{
  sVector zero;
  sInt buf;

  zero.Init(0,0,0,0);
#if !sPLAYER
  if(GenOverlayManager->SoundEnable)
#endif
  {
    buf = sSystem->SamplePlay(handle,volume);
    sSystem->Sample3DParam(handle,buf,Collider.Pos,zero,0.2f,160.0f);
  }
}

/****************************************************************************/
/****************************************************************************/

void KKriegerGame::Init()
{
  sInputData id;

#if sPROFILE
  sREGZONE(GameLogic);
  sREGZONE(GameCollide);
  sREGZONE(GamePhysic);
  sREGZONE(GameAI);
#endif

  CellAdd.Init(128);
  CellSub.Init(4096);
  CellZone.Init(1024);

  Monsters.Init(64);
  Shots.Init(64);

  PlayerPos.Init();
  PlayerDir = 0;
  PlayerLook = 0;
  PlayerMat.Init();
  PlayerCell = 0;
  Player.Init();

  LastTime = sSystem->GetTime();
  sSystem->GetInput(0,id);
  LastMouseX = id.Analog[0];
  LastMouseY = id.Analog[1];
  LastCamY = 0;
  CamPos.Init();
  Environment = 0;
  TickCount = 0;
  PlayerAdds = 0;
  
#if !sPLAYER
  TickPerFrame = 0;
  MaxPlanes = 0;
#endif

  sSetMem(Respawn,0,sizeof(Respawn));

#if !sPLAYER
  FlatMat = new sSimpleMaterial(sINVALID,sMBF_ZOFF|sMBF_BLENDADD|sMBF_DOUBLESIDED,0,0);
  QuadGeo = sSystem->GeoAdd(sFVF_COMPACT,sGEO_QUAD);
  LineGeo = sSystem->GeoAdd(sFVF_COMPACT,sGEO_LINE);
#endif

  WeaponEvent.Init();
  Flush();
  Panic();

  sSetMem(Switches,0,KKRIEGER_SWITCHES);
  Switches[KGS_ONE] = 1;
  Switches[KGS_GLARE] = 2;
  Switches[KGS_RESOLUTION] = 1;
  Switches[KGS_TEXTURES] = 1;
  Switches[KGS_SHADOWS] = 1;
  Switches[KGS_MOUSESPEED] = 5;
  Switches[KGS_BRIGHTNESS] = 5;
  Switches[KGS_SPECULAR] = 1;
  WasInOptions = sFALSE;

  OnOptionsChanged();
}

void KKriegerGame::Exit()
{
#if !sPLAYER
  if(QuadGeo!=sINVALID) sSystem->GeoRem(QuadGeo);
  if(LineGeo!=sINVALID) sSystem->GeoRem(LineGeo);
  QuadGeo = sINVALID;
  LineGeo = sINVALID;
  sRelease(FlatMat);
#endif

  Flush();
  CellAdd.Exit();
  CellSub.Exit();
  CellZone.Exit();
  Monsters.Exit();
  Shots.Exit();
  WeaponEvent.Exit();
}

void KKriegerGame::Flush()
{
  sInt i,j;
  for(i=0;i<CellAdd.Count;i++)
  {
    CellAdd[i]->Adds.Exit(); 
    CellAdd[i]->Subs.Exit();   
    CellAdd[i]->Zones.Exit();
    CellAdd[i]->DeleteFaces();
    delete CellAdd[i];
  }
  for(i=0;i<CellSub.Count;i++)
    delete CellSub[i];
  for(i=0;i<CellZone.Count;i++)
  {
    for(j=0;j<2;j++)
    {
      if(CellZone[i]->Event[j])
      {
        CellZone[i]->Event[j]->Exit();
        delete CellZone[i]->Event[j];
        CellZone[i]->Event[j] = 0;
      }
    }
    delete CellZone[i];
  }

  for(i=0;i<Shots.Count;i++)
  {
    Shots[i].Exit();
  }

  for(i=0;i<8;i++)
  {
    WeaponShot[i] = 0;
    WeaponOptics[i] = 0;
    WeaponExplode[0][i] = 0;
    WeaponExplode[1][i] = 0;
  }
  CellAdd.Count = 0;
  CellSub.Count = 0;
  CellZone.Count = 0;
  Shots.Count = 0;
#if !sPLAYER
  PlayerCell = 0;
  MaxPlanes = 0;
#endif

  PartUsed = 0;
  ConsUsed = 0;
  WeaponTimer = 0;
  StepTimer = 0;
  StepFoot = 0;
  OffGroundTime = 0;
  Player.CurrentWeapon = 1;
  Player.NextWeapon = 0;
}

#pragma lekktor(on)

void KKriegerGame::Panic()
{
  sVector speed,scale;
  sInputData id;

  SpeedForw = 0;
  AccelForw = 0;
  SpeedSide = 0;
  AccelSide = 0;
  SpeedUp = 0;
  OnGround = 0;
  ResetByOp = 1;
  JumpTimer = 0;
  CollisionForMonsterMode = 0;

  Gravity = 0.0002f;
  GravityPlayer = 0.01f;
  DampPlayerAir = 0.05f;
  DampPlayerCam = 0.05f;
  DampPlayerGround = 0.05f;
  DampPlayerHeight = 0.05f;
  StairHeight = 0.55f;
  EyeHeight = 1.60f;
  MouseTurnSpeed = 0.01f;
  MouseLookSpeed = 0.01f;
  AccelSideFactor = 0.005f;
  AccelForwFactor = 0.005f;
  AccelBackFactor = 0.005f;
  JumpForce = 0.35f;
  FlyMode =0;
  SplashEnable = 0;

  CamOffset = EyeHeight-StairHeight;
  CamZoomPos = 0;
     
  scale.Init(0.5f,1.0f/4,0.5f);
  speed.Init();

//  PlayerPart = MakeBody((const sS8 *)playerv,7-2,(const sU8 *)playere,18-8,scale,0x09,PlayerPos,speed);
//  PlayerPart->Kind = 0x08;

  sSystem->GetInput(0,id);
  LastMouseX = id.Analog[0];
  LastMouseY = id.Analog[1];

  // gameplay init

#if LOGGING
  sDPrintF("KKriegerGame::Panic() called\n");
#endif
}

/****************************************************************************/
/****************************************************************************/

void KKriegerGame::SetPainter(KOp *op,KEnvironment *kenv)
{
  while(op && op->Cache)
  {
    if(op->Cache->ClassId == KC_IPP || op->Cache->ClassId == KC_DEMO)
      op = op->GetInput(0);
    else if(op->CheckOutput(KC_SCENE))
    {
      SetScene((GenScene *)op->Cache);
      break;
    }
    else
      break;
  }
}

void KKriegerGame::SetScene(GenScene *scene)
{
  sMatrix mat;

  Flush();
  mat.Init();

  CellList.Init();
  
  SetSceneR(scene,mat,scene,0);
#if sPLAYER
  SetSceneR(scene,mat,scene,1);
#endif

  CellConnect();

#if !sPLAYER
  sInt i,j;
  KKriegerMonster *mon;

  if(CellAdd.Count)
  {
    sVector globmin,globmax;
    sF32 maxworld;
    sInt i;

    // find global min/max coord
    globmin = CellAdd[0]->BBMin;
    globmax = CellAdd[0]->BBMax;

    for(i=0;i<CellAdd.Count;i++)
    {
      globmin.x = sMin(globmin.x,CellAdd[i]->BBMin.x);
      globmin.y = sMin(globmin.y,CellAdd[i]->BBMin.y);
      globmin.z = sMin(globmin.z,CellAdd[i]->BBMin.z);
      globmax.x = sMax(globmax.x,CellAdd[i]->BBMax.x);
      globmax.y = sMax(globmax.y,CellAdd[i]->BBMax.y);
      globmax.z = sMax(globmax.z,CellAdd[i]->BBMax.z);
    }

    // find max world coordinate
    maxworld = 0.0f;
    maxworld = sMax<sF32>(maxworld,sFAbs(globmin.x));
    maxworld = sMax<sF32>(maxworld,sFAbs(globmin.y));
    maxworld = sMax<sF32>(maxworld,sFAbs(globmin.z));
    maxworld = sMax<sF32>(maxworld,sFAbs(globmax.x));
    maxworld = sMax<sF32>(maxworld,sFAbs(globmax.y));
    maxworld = sMax<sF32>(maxworld,sFAbs(globmax.z));

    // print world scale
    static sF32 dynRange = 1.0f / 1048576.0f; // 2**(-20)

    sDPrintF("max world coordinate is %.2f\n",maxworld);
    if(maxworld * dynRange >= EPSILON)
    {
      sDPrintF("warning, kkriegergame epsilon too small!\n");
      sDPrintF("use at least %e to guarantee ok precision.\n",maxworld * dynRange);
    }
  }

  // this fixes 

//  FlushCells
  for(i=0;i<Monsters.Count;i++)
  {
    mon = Monsters.Get(i);
    mon->Collider.Cell = FindCell(mon->Collider.Pos);
    for(j=0;j<8;j++)
      mon->WalkMem.FootCell[j] = mon->Collider.Cell;
  }

#endif

  CellList.Exit();
}

void KKriegerGame::SetMesh(GenMesh *mesh)
{
  sMatrix mat;
  KKriegerMesh *tempMesh;

  Flush();
  mat.Init();

  CellList.Init();

  tempMesh = new KKriegerMesh(mesh);
  AddMesh(tempMesh,mat,0);
  tempMesh->Release();

  CellConnect();
  CellList.Exit();
}

void KKriegerGame::SetPlayer(const sVector &p,sF32 d,sF32 l)
{
  sInputData id;
  sMatrix mat;
  sVector v;

  sSystem->GetInput(0,id);
  LastMouseX = id.Analog[0];
  LastMouseY = id.Analog[1];

  PlayerPos = p;
  PlayerDir = d;
  PlayerLook = l;
  PlayerCell = FindCell(PlayerPos);
  if(PlayerCell==0 && CellAdd.Count>0)
  {
    PlayerCell = CellAdd[0];
    PlayerPos.Add3(CellAdd[0]->BBMin,CellAdd[0]->BBMax);
    PlayerPos.Scale3(0.5f);
  }
  LastCamY = p.y;
  CamPos = p;

  Player.Collider.Pos = PlayerPos;
  Player.Collider.Cell = PlayerCell;
  Player.Collider.Radius = 0.5f;
//  Player.Collider.Init(PlayerPos, PlayerCell);
  Player.Collider.OldPos = Player.Collider.Pos;

  mat.Init();
  mat.l.Init(0,-0.25f,0,1.0f);
  v.Init(0.75f,2.5f,0.75f,0);
  Player.HitCell.Init(mat,v,KCM_ZONE);
  mat.l = PlayerPos;
  mat.l.y += 0.85f;
  Player.HitCell.PrepareDynamic(mat);
  
  //  Panic();
}

/****************************************************************************/

void KKriegerGame::SetSceneR(GenScene *scene,const sMatrix &base,GenScene *usescene,sInt phase)
{
  sMatrix mat,srt,tmat;
  sInt i,j,max;

  srt.InitSRT(scene->SRT);

  max = scene->Count;
  if(max==0)
    max = 1;

  if(scene->IsSector)
    usescene = scene;

  mat = base;

  for(i=0;i<max;i++)
  {
    if(scene->Count==0 || i!=0)
    {
      tmat.MulA(srt,mat);
      mat = tmat;
    }

    for(j=0;j<scene->Childs.Count;j++)
      SetSceneR(scene->Childs.Get(j),mat,usescene,phase);

#if sLINK_KKRIEGER
    if(scene->CollMesh)
    {
      if(phase == 0)
        AddMesh(scene->CollMesh,mat,usescene);
      else
      {
#if sPLAYER
//        scene->CollMesh->Release();
//        scene->CollMesh = 0;
#endif
      }
    }
#endif
  }  
}


void KKriegerGame::AddMesh(KKriegerMesh *mesh,const sMatrix &mat,GenScene *usescene)
{
  sInt i,j,k;
  KKriegerCell *kc;
  KKriegerCellAdd *kca;
  KKriegerMeshCell *mc;
  sVector va[8];
  sInt mode;

  for(j=0;j<mesh->CellCount;j++)
  {
    mc = &mesh->Cell[j];
    mode = mc->Mode & 3;

    for(k=0;k<8;k++)
      va[k].Rotate34(mat,mc->Vert[k]);

    switch(mode)
    {
    default:
#if !sINTRO
      sVERIFYFALSE;
#endif
    case KCM_ADD:
      kca = new KKriegerCellAdd;
      *CellAdd.Add() = kca;
      kca->Adds.Init();
      kca->Subs.Init();
      kca->Zones.Init();
      kca->Faces = 0;
      kc = kca;
      break;
    case KCM_SUB:
      kc = new KKriegerCell;
      *CellSub.Add() = kc;
      break;
    case KCM_ZONE:
      kc = new KKriegerCell;
      *CellZone.Add() = kc;
      break;
    }

    kc->Init(va,mc->Mode);
    kc->Logic = mc->Logic;
    kc->Scene = usescene;

    for(i=0;i<2;i++)
    {
      kc->Event[i] = 0;
      if(kc->Logic.Event[i])
      {
        kc->Event[i] = new KEvent;
        kc->Event[i]->Init();
        kc->Event[i]->Op = kc->Logic.Event[i];
        kc->Event[i]->Translate.x = (kc->BBMin.x + kc->BBMax.x)/2;
        kc->Event[i]->Translate.y =  kc->BBMin.y;
        kc->Event[i]->Translate.z = (kc->BBMin.z + kc->BBMax.z)/2;
        kc->Event[i]->CullDist = 35;
      }
    }

#if !sPLAYER
    MaxPlanes = sMax(MaxPlanes,kc->PlaneCount);
#endif

    *CellList.Add() = kc;
  }
}

/****************************************************************************/

// For heapsort in CellConnect.
void KKriegerGame::CellSiftDown(sInt n,sInt k)
{
  KKriegerCell *v = CellList[k];
  sF32 key = v->BBMin.x;

  while(k < (n >> 1))
  {
    sInt j = k*2+1;
    if(j+1 < n && CellList[j]->BBMin.x < CellList[j+1]->BBMin.x)
      j++;

    if(key >= CellList[j]->BBMin.x)
      break;

    CellList[k] = CellList[j];
    k = j;
  }

  CellList[k] = v;
}

//sInt debug_count_splits;

// Connect cells. Uses CellHeapSort.
void KKriegerGame::CellConnect()
{
  // Heapsort the list of cells by minimum x coordinate of bbox.
  sInt n = CellList.Count;
  for(sInt k=n/2-1;k>=0;k--)
    CellSiftDown(n,k);

  while(--n > 0)
  {
    sSwap(CellList[0],CellList[n]);
    CellSiftDown(n,0);
  }

  // Verify the resulting list is sorted
  for(sInt i=0;i<CellList.Count-1;i++)
    sVERIFY(CellList[i]->BBMin.x <= CellList[i+1]->BBMin.x);

  // Sweep-and-prune the list, connecting cells as we identify pairs.
  n = CellList.Count;
  for(sInt box1=0;box1 < n;box1++)
  {
    KKriegerCell *cell1 = CellList[box1];
    sInt cell1m = cell1->Mode & 3;
    sF32 maxKey = cell1->BBMax.x;

    for(sInt box2=box1+1;box2 < n && CellList[box2]->BBMin.x <= maxKey;box2++)
    {
      KKriegerCell *cell2 = CellList[box2];
      sInt cell2m = cell2->Mode & 3;

      if(cell1m == KCM_ADD || cell2m == KCM_ADD)
      {
        // BBoxes overlap in x direction, and at least one of them is an add.
        // Check whether bboxes really intersect.

        if(cell1->BBMax.z < cell2->BBMin.z || cell1->BBMin.z > cell2->BBMax.z)
          continue;

        if(cell1->BBMax.y < cell2->BBMin.y || cell1->BBMin.y > cell2->BBMax.y)
          continue;

        // We have actual overlap (of bboxes atleast). Now connect.
        if(cell1m == KCM_ADD && cell2m == KCM_ADD)
        {
          // Adds need to link to each other, so this is a special case.
          KKriegerCellAdd *c1a = (KKriegerCellAdd *) cell1;
          KKriegerCellAdd *c2a = (KKriegerCellAdd *) cell2;

          *c1a->Adds.Add() = c2a;
          *c2a->Adds.Add() = c1a;
        }
        else // only is one add
        {
          KKriegerCellAdd *cella; // add cell
          KKriegerCell *cello; // other cell
          sInt type;

          if(cell1m == KCM_ADD) // first is add
          {
            cella = (KKriegerCellAdd *) cell1;
            cello = cell2;
            type = cell2m;
          }
          else // second is add
          {
            cella = (KKriegerCellAdd *) cell2;
            cello = cell1;
            type = cell1m;
          }

          if(type == KCM_SUB)
            *cella->Subs.Add() = cello;
          else if(type == KCM_ZONE)
            *cella->Zones.Add() = cello;
        }
      }
    }
  }

  // now generate the collision geometrie of the adds
//  sInt time = sSystem->GetTime();
  for(sInt i = 0; i < CellAdd.Count; i++)
  {
//    debug_count_splits = 0;
    CreateCollisionFaces(*CellAdd[i]);
//    sDPrintF("(%2d)%5d ",CellAdd[i]->Adds.Count,debug_count_splits);
//    if(!(i%15)) sDPrintF("\n");
  }
//  sDPrintF("Create it all %d\n",sSystem->GetTime()-time);
}

/****************************************************************************/
/****************************************************************************/

#pragma lekktor(off)

sBool KKriegerGame::OnKey(sU32 key)
{
  sInt i;
  static sInt CheatOn;
  static sS8 weaponswap[8] = {-1,0,1,2,4,6,-1,-1};

  LastKey = key&0x8001ffff;

  switch(key&(0x8001ffff))
  {
  case ' ':
    if(OnGround && JumpTimer==0)
    {
      SpeedUp = JumpForce;
      PlayerSound(4,0.9f);
      JumpTimer = 10;
    }
    break;
  case 'k':
  case 'K':
    Player.Hit(10);
    break;
#if !sPLAYER
  case 'i':
  case 'I':
    Switches[KGS_GAME] = KGS_GAME_INTRO;
    break;
#endif
  case 'r':
  case 'R':
#if !sPLAYER
    Restart();
#endif
    break;
  case 'w':
  case 'W':
    AccelForw = AccelForwFactor;
    CheatOn = 0;
    return sTRUE;
  case 's':
  case 'S':
    AccelForw = -AccelBackFactor;
    CheatOn = 0;
    return sTRUE;
  case 'd':
  case 'D':
    AccelSide = AccelSideFactor;
    CheatOn = 0;
    return sTRUE;
  case 'a':
  case 'A':
    AccelSide = -AccelSideFactor;
    CheatOn = 0;
    return sTRUE;
  case 'w'|sKEYQ_BREAK:
  case 'W'|sKEYQ_BREAK:
  case 's'|sKEYQ_BREAK:
  case 'S'|sKEYQ_BREAK:
    AccelForw = 0;
    CheatOn = 0;
    return sTRUE;
  case 'd'|sKEYQ_BREAK:
  case 'D'|sKEYQ_BREAK:
  case 'a'|sKEYQ_BREAK:
  case 'A'|sKEYQ_BREAK:
    AccelSide = 0;
    CheatOn = 0;
    return sTRUE;

  case 'y':
  case 'Y':
    FlyMode = !FlyMode;
    return sTRUE;

  case sKEY_SHIFTL:
    Player.UseKey = 1;
    return sTRUE;

  case sKEY_CTRLR:
  case sKEY_MOUSEL:
    Player.FireKey = 1;
    return sTRUE;
  case sKEY_CTRLR|sKEYQ_BREAK:
  case sKEY_MOUSEL|sKEYQ_BREAK:
    Player.FireKey = 0;
    return sTRUE;

#if !sPLAYER
  case 'z':
    CamZoomPos = (CamZoomPos+1)%2;
    return sTRUE;
#endif

  case 'm':
  case 'M':
    CheatOn = 1;
    return sTRUE;
  case 'n':
  case 'N':
    if(CheatOn)
      sCopyMem(&Player.Life,&Player.LifeMax,16*4);
    CheatOn = 0;
    return sTRUE;

  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    if(CheatOn)
    {
      RespawnAt(key&15);
      CheatOn = 0;
    }
    else
    {
      i = weaponswap[key&7];
      if(i>=0 && Player.Weapon[i])
        Player.NextWeapon = i;
    }
    return sTRUE;
  }
  return sFALSE;
}

/****************************************************************************/

void KKriegerGame::AddEvents(KEnvironment *kenv)
{
  sInt i;
  KKriegerShot *shot;
  KKriegerCell *cell;


  for(i=0;i<Shots.Count;)
  {
    shot = &Shots[i];
    if(shot->Event->Select)
    {
      Shots[i] = Shots[--Shots.Count];
    }
    else
    {
      i++;
      if(shot->Mode!=0)
      {
        shot->Event->Matrix = shot->Matrix;
        kenv->AddStaticEvent(shot->Event);
      }
    }
  }

  for(i=0;i<CellZone.Count;i++)
  {
    cell = CellZone[i];
    if(cell->Event[0] && cell->Respawn==0)
      kenv->AddStaticEvent(cell->Event[0]);
    if(cell->Event[1] && cell->Respawn>0)
      kenv->AddStaticEvent(cell->Event[1]);
  }

  if(Player.NextWeapon!=Player.CurrentWeapon && WeaponTimer>=1.0f)
  {
    Player.CurrentWeapon = Player.NextWeapon;
    WeaponEvent.Exit();
    WeaponEvent.Init();
    WeaponEvent.Op = WeaponOptics[Player.CurrentWeapon];
    WeaponTimer = 0.0f;
  }

  if(WeaponEvent.Op)
  {
    WeaponEvent.Translate = PlayerPos;
    WeaponEvent.Translate.y = LastCamY+CamOffset,
    WeaponEvent.Rotate.Init(PlayerLook/sPI2F,PlayerDir/sPI2F,0,0);
    WeaponEvent.Velocity = WeaponTimer;
    WeaponEvent.Modulation = 0;
    if(WeaponCool[Player.CurrentWeapon]>0.01f)
      WeaponEvent.Modulation = (WeaponCool[Player.CurrentWeapon]-Player.CoolTimer)/WeaponCool[Player.CurrentWeapon];
    kenv->AddStaticEvent(&WeaponEvent);
  }
}

/****************************************************************************/

void KKriegerGame::ResetRoot(KEnvironment *env,KOp *root,sBool firsttime)
{
  SetPainter(root,env);
  Restart();
  if(firsttime)
    Switches[KGS_GAME] = KGS_GAME_INTRO;
}

sInt KKriegerGame::GetNewRoot()
{
#if sLINK_KKRIEGER
  sInt sw = Switches[KGS_GAME];
  sInt mode = 1;

  if(sw==KGS_GAME_RUN || sw==KGS_GAME_INGAME || sw==KGS_GAME_RESTART)
    mode = 2;
  if(sw==KGS_GAME_INTRO)
    mode = 0;
  if(sw==KGS_GAME_QUIT)
  {
    sSystem->Exit();
    Switches[KGS_GAME] = KGS_GAME_START;
  }
  if(sw==KGS_GAME_RESTART)
  {
    Switches[KGS_GAME] = KGS_GAME_RUN;
    Restart();
  }


  return mode;
#else
  return 0;
#endif
}

void KKriegerGame::Restart()
{
  sInt i;

  ResetByOp = 1;

  SetPlayer(PlayerStartPos,0,0);
//  RespawnAt(1);
  LastCamY = PlayerPos.y;
  CamPos = PlayerPos;

  for(i=0;i<Monsters.Count;i++)
    Monsters[i]->State = KMS_RESPAWN;

  for(i=0;i<CellZone.Count;i++)
    CellZone[i]->DoRespawn();

  sSetMem(Switches+16,0,KKRIEGER_SWITCHES-16);

  //  Panic();    
}

void KKriegerGame::Continue()
{
  sInt i;
  SetPlayer(PlayerStartPos,0,0);
  for(i=0;i<16;i++)
  {
    if(Switches[KGS_RESPAWN+i] && Respawn[i].Valid)
      SetPlayer(Respawn[i].Pos,Respawn[i].Dir,0);
  }
  for(i=0;i<Monsters.Count;i++)
    if(Monsters[i]->Life>0)
      Monsters[i]->State = KMS_RESPAWN;

  Player.Life = 100;
  if(Player.Ammo[0]<200)
    Player.Ammo[0] = 200;
}

sInt KKriegerGame::SplashCalc(const sVector &pos,KKriegerCellAdd *cell)
{
  sVector v,p1;
  sF32 dist;
  KKriegerCollideInfo ci;

  v.Sub3(pos,SplashPos);
  dist = v.Abs3();
  if(dist<SplashRange)
  {
    p1 = pos;
    p1.y += 0.5f;
    ci.Init();
    if(cell==0 || !CollideRay(cell,p1,SplashPos,ci))
    {
      dist = 1-(dist/SplashRange);
      return SplashHits*dist;
    }
  }
  return 0;
}

struct ShotInfo
{
  sInt Mode;                // 0=off, 1=point, 2=cube
  sF32 Speed;               // shot initial speed
  sF32 rx,ry,rz;            // initial rotation
  sF32 px,py,pz;            // initial position
  sF32 sx,sy,sz;            // "cube" size (for physic)
};

ShotInfo ShotInfoTable[8] =  // shot0..7 player, shot 8..11 monster
{
  { 1,0.250f  ,  -0.020f,0,0  ,  0.575f,-0.250f,1.250f },                  // shotgun
  { 1,0.200f  ,  -0.020f,0,0  ,  0.585f,-0.430f,1.250f },                  // automatic gun
  { 1,0.1177f ,  -0.020f,0,0  ,  0.410f,-0.135f,1.275f },                  // zapper
  { 0 },
  { 2,0.100f  ,  -0.020f,0,0  ,  0.505f,-0.135f,1.625f ,  0.5,0.5,0.5 },   // light bomb
  { 1,0.250f  ,   0     ,0,0  ,  0.000f, 1.500f,1.000f },                  // killerbug
  { 2,0.100f  ,  -0.020f,0,0  ,  0.505f,-0.135f,1.625f ,  0.5,0.5,0.5 },   // shadow bomb
  { 1,0.250f  ,   0     ,0,0  ,  0.000f, 1.935f,1.000f },                  // ultimate bot
};

void KKriegerGame::FireShot(KEnvironment *kenv,sInt weapon,KKriegerMonster *monster,const sVector *monsterfiredir)
{
  KKriegerShot *shot;
  ShotInfo *info;
//  sInt i;
  sMatrix mat;
//  sVector v;
  sVector speed;
  sVector scale;
  KKriegerCellDynamic *skipcell;

  
  sVERIFY(weapon>=0 && weapon<8);
  info = &ShotInfoTable[weapon];
  if(info->Mode==0) return;

  shot = Shots.Add();
  sSetMem(shot,0,sizeof(*shot));

  shot->Mode = info->Mode;
  shot->Tensor.Init(0,0,0,0);
  shot->Weapon = weapon;
  shot->Event = new KEvent;
  shot->Event->Init();
  shot->Event->Active = 0;
  shot->Matrix.Init();
  shot->DoubleKill = 0;

  if(monster)
  {
//    mat = monster->Matrix;
    mat.InitDir(*monsterfiredir);
    mat.l = monster->Collider.Pos;
    mat.l.y += 0.75f;
//    v.Init(info->px,info->py,info->pz);
//    v.Rotate3(mat);
//    mat.l.Add3(v);
    skipcell = 0;
    shot->Who = KCRF_ISMONSTER;
    shot->Cell = monster->Collider.Cell;
  }
  else
  {
    mat.InitEuler(PlayerLook+info->rx*sPI2F,PlayerDir,0);
    mat.l.x = info->px;
    mat.l.y = info->py;
    mat.l.z = info->pz;
    mat.l.Rotate3(mat);
    mat.l.x += PlayerPos.x;
    mat.l.y += LastCamY+CamOffset;
    mat.l.z += PlayerPos.z;

    skipcell = &kenv->Game->Player.HitCell;
    shot->Who = KCRF_ISPLAYER;
    shot->Cell = kenv->Game->PlayerCell;
  }

  speed = mat.k;
  speed.Scale4(info->Speed);
  scale.Init(0.5f,0.5f,0.5f);

  shot->Event->Matrix = mat;
  shot->Matrix.l = mat.l;
  shot->OldPos.Sub4(shot->Matrix.l,speed);

  if(info->Mode==2)
  {
    shot->Tensor.InitRnd();
    shot->Tensor.Scale3(0.01f);
    shot->Tensor.w = 0;
  }

  shot->Event->Op = WeaponShot[weapon];
  shot->Event->Monster = monster;
}

extern "C" int __cdecl wsprintfA(sChar *buffer,const sChar *format,...);
extern "C" void __stdcall OutputDebugStringA(const sChar *str);

void KKriegerGame::OnTick(KEnvironment *kenv,sInt slices)
{
  sInputData id;
  sInt time;
  sF32 d;
  sVector speed;
  sInt i;
  KKriegerMonster *mon;
  sInt sl;
  sBool inOptions;
  sF32 f;
  KKriegerCollideInfo ci;

  inOptions = Switches[KGS_GAME] == KGS_GAME_OPTIONS
    || Switches[KGS_GAME] == KGS_GAME_INGAME && Switches[KGS_INGAME_MENU] == 1;

  if(WasInOptions && !inOptions)
    OnOptionsChanged();

  WasInOptions = inOptions;

  // check if everything is safe.

  if(PlayerCell==0 || Switches[KGS_GAME]!=KGS_GAME_RUN)
    return;

  TickCount += slices;

//  sDPrintF("slices %d\n",slices);

  // get mouse looking

  Environment = kenv;

#if DOUBLECHECK
  slices = 1;
#endif

#if !sPLAYER
  TickPerFrame = slices;
#endif
  time = sSystem->GetTime()-LastTime;   // don't get mouse movement if a lot of time ticked away
  LastTime += time;
  sSystem->GetInput(0,id);
  if(time<1000)
  {
    f = MouseTurnSpeed*(Switches[KGS_MOUSESPEED]+2)/7;
    PlayerDir  += (id.Analog[0] - LastMouseX)*f;
    f = MouseLookSpeed*(Switches[KGS_MOUSESPEED]+2)/7*(-Switches[KGS_MOUSEINVERT]*2+1);
    PlayerLook += (id.Analog[1] - LastMouseY)*f;
    PlayerLook = sRange<sF32>(PlayerLook,sPIF/2,-sPIF/2);
  }
  LastMouseX = id.Analog[0];
  LastMouseY = id.Analog[1];
  PlayerMat.InitEuler(FlyMode?PlayerLook:0,PlayerDir,0);

  // weapon animation

  if(Player.NextWeapon!=Player.CurrentWeapon)
  {
    if(WeaponTimer<0.75f)
      WeaponTimer += slices*0.01f*5;
    WeaponTimer += slices*0.01f;
  }
  else
  {
    if(WeaponTimer<0.25f)
    {
      WeaponTimer+=slices*0.01f;
      if(WeaponTimer>0.25f)
        WeaponTimer = 0.25f;
    }
    else
    {
      if(WeaponTimer>0.251f || AccelForw>0)
      {
        WeaponTimer+=slices*0.003f;
        if(WeaponTimer>0.75f)
        {
          if(AccelForw>0)
          {
            WeaponTimer-=0.5f;
            if(WeaponTimer>0.75)
              WeaponTimer = 0.25f;
          }
          else
            WeaponTimer = 0.25f;
        }
      }
    }
  }

  // player step
  if(AccelForw || AccelSide)
  {
    StepTimer+=slices*0.01f;
    if(StepTimer>0.5f && OnGround) // issue player step
    {
      StepTimer -= 0.5f;
      StepFoot = 1-StepFoot;
      PlayerSound(StepFoot,0.8f);
    }
  }

  // splash damage

  if(SplashEnable)
  {    
    KKriegerCellAdd *cell;

    cell = FindCell(SplashPos);

    for(i=0;i<Monsters.Count;i++)
    {
      mon = kenv->Game->Monsters.Get(i);
      mon->Hit(SplashCalc(mon->Collider.Pos,cell),this);
    }
    SplashHits = SplashHits/2;
    Player.Hit(SplashCalc(PlayerPos,cell));
    SplashEnable = 0;
  }

  // fire?

  if(Player.FireKey)
  {
    if(!Player.Ammo[Player.CurrentWeapon/2] && Player.CurrentWeapon!=0)
      PlayerSound(15,0.8f);

    if((Player.Ammo[Player.CurrentWeapon/2]>0 || Player.CurrentWeapon==0) && Player.CurrentWeapon==Player.NextWeapon && Player.CoolTimer<=0 && WeaponTimer>=0.25f)
    {
      if(Player.CurrentWeapon!=0)
        Player.Ammo[Player.CurrentWeapon/2]--;
      Player.CoolTimer = WeaponCool[Player.CurrentWeapon];
      if(!(WeaponFlags[Player.CurrentWeapon]&KWF_CONTINUOUS))
        Player.FireKey = 0;

      if(WeaponShot[Player.CurrentWeapon])
      {
        FireShot(kenv,Player.CurrentWeapon,0,0);
      }
      if(!(WeaponFlags[Player.CurrentWeapon]&KWF_CONTINUOUS))
        Player.FireKey = 0;
    }
  }
  if(Player.CoolTimer>=0)
    Player.CoolTimer -= slices*0.01f;

  // the timesliced-loop ...

  for(sl=0;sl<slices;sl++)
  {
    sInt upforce;
    // find height

    if(JumpTimer>0)
      JumpTimer--;

    sF32 testh = StairHeight + GravityPlayer + Player.Collider.Radius;
    d = testh;
    if(Player.Collider.Cell)
    {
      sVector p0,p1;
      KKriegerCellAdd *cell;

      ci.Init(0,KCRF_ISPLAYER);
      p0 = Player.Collider.Pos;//part->Pos;
      p1 = p0; 
      p1.y -= testh;
      cell = Player.Collider.Cell;    // cell might get changed by CollideRay!
        
      if(CollideRay(cell,p1,p0,ci))
        d += ci.Dist;
    }

    // move player

    speed.Init();
    speed.Sub4(Player.Collider.Pos,Player.Collider.OldPos);
    speed.Scale3(1.0f-(OnGround?DampPlayerGround:DampPlayerAir));

    speed.AddScale3(PlayerMat.i,AccelSide);
    speed.AddScale3(PlayerMat.k,AccelForw);
    speed.y += SpeedUp;
    SpeedUp = 0;
    OnGround = 0;


    upforce = 0;
    if(d<testh)
    {
      if(OffGroundTime >= 0.4f) // player was flying, issue "land" sample
      {
        PlayerSound(5);
        StepFoot = 0;
        StepTimer = 0;
      }

      OnGround = 1;
      OffGroundTime = 0.0f;

      upforce += (testh-d)*0.1f;
    }
    else
    {
      OffGroundTime += slices*0.01f;
      speed.y -= GravityPlayer;
    }

//    sDPrintF("speed %f pos %f dist %f Ground %d %08x\n",speed.y,Player.Collider.Pos.y,d,OnGround,Player.Collider.Cell);

//    upforce = 0; speed.y-=GravityPlayer; // slide on the ground
    Player.Collider.OldPos = Player.Collider.Pos;
    Player.Collider.OldPos.y += upforce;
    speed.y += upforce;
    MoveCollider(Player.Collider,speed,KCRF_ISPLAYER);

    PlayerPos = Player.Collider.Pos;
    if(Player.Collider.Cell)
      PlayerCell = Player.Collider.Cell;

    Player.HitCell.Matrix1.InitEuler(0,PlayerDir,0);
    Player.HitCell.Matrix1.l = PlayerPos;
    Player.HitCell.Matrix1.l.y += 0.85f;
    Player.HitCell.Matrix0.l = Player.HitCell.Matrix1.l;


    // slice

    Physic((sl+1.0f)/slices);
    Zones(kenv);

    // cam

    LastCamY = sFade(LastCamY,PlayerPos.y,DampPlayerCam);
    CamPos = PlayerPos;
    CamPos.y = LastCamY;
    CamPos.y += CamOffset;

    // 3d sound listener
    speed.Scale3(1000.0f / time);
    sSystem->Sample3DListener(CamPos,speed,PlayerMat.j,PlayerMat.k);

    // AI

    {
      sBool prev;
      sBool mach;

      prev = 0;
      mach = 0;
    #if sPROFILE
      sZONE(GameAI);
    #endif
  
      MonsterMagnetAI();

      for(i=0;i<Monsters.Count;i++)
      {
        mon = Monsters[i];
        MonsterAI(mon,kenv,mach,prev);
        prev = mon->Life>0;
        if(mon->Flags&KMF_MACHINE)
        {
          mach = prev;
          prev = 0;         // HACK: first monster from machine spawns even with KMF_WHENPREVDEAD set. makes editing simpler.
        }
      }
    }

    for(i=0;i<Shots.Count;)
    {
      if(ShotAI(&Shots[i]))
        i++;                                // shot is ok
      else
      {
        Shots[i].Exit();
        Shots[i] = Shots[--Shots.Count];    // shot has ended
      }
    }


    Player.UseKey = 0;

  // ensure we always have a correct in-volume for player position (portals)

    if(PlayerCell==0 || PlayerCell->OutsideMask(PlayerPos)!=0)
    {
      KKriegerCellAdd *cell;
      cell = FindCell(PlayerPos);
      if(cell!=0)
      {
        PlayerCell = cell;
#if LOGGING
        sDPrintF("KKriegerGame::OnTick() corrected player pos\n");
#endif
      }
    }
  }

  // finally move collisions

  for(i=0;i<DCellUsed;i++)
    if(!DCellPtr[i]->Blocked)
      DCellPtr[i]->Matrix0 = DCellPtr[i]->Matrix1;


  Environment = 0;

  // check death of player

  if(Player.Life==0 && Switches[KGS_GAME]==KGS_GAME_RUN)
  {
    //Switches[KGS_GAME] = KGS_GAME_LOST;
    Continue();
  }

// diagnostics

#if !sPLAYER
  {
    sInt i;
    KKriegerCellAdd *cell,*test;

    cell = PlayerCell;
    PlayerAdds = 1;
    for(i=0;i<cell->Adds.Count;i++)
    {
      test = cell->Adds.Get(i);
      if(test->OutsideMask(PlayerPos,0)==0)
        PlayerAdds++;
    }
  }

#endif

}

/****************************************************************************/

// check all collision zones...

void KKriegerGame::Zones(KEnvironment *kenv)
{
  sInt *valp;
  sInt i,action,out;
  KKriegerCell *cell;
  KKriegerCellDynamic *dcell;

#if sPROFILE
  sZONE(GameLogic);
#endif

  for(i=0;i<CellZone.Count;i++)
  {
    cell = CellZone[i];
    sVERIFY(cell->Mode==KCM_ZONE);

    // respawn

    if(cell->Respawn>0 && cell->Respawn<0x40000000)
    {
      if(cell->Respawn<=10)
        cell->DoRespawn();
      else
        cell->Respawn -= 10;
    }

    // logic

    if(Switches[cell->Logic.Condition])
    {
      action = 0;
      switch(cell->Logic.Code&0xf0)
      {
      case KKEE_ENTER:
        if(cell->Enter&KPM_PLAYER)
          action = 1;
        break;
      case KKEE_LEAVE:
        if(cell->Leave&KPM_PLAYER)
          action = 1;
        break;
      case KKEE_COLLECT:
        if((cell->Collided&KPM_PLAYER) && cell->Respawn==0)
        {
          action = 1;
          cell->Respawn = 0x7fffffff;
        }
        break;
      case KKEE_RESPAWN0:
        if((cell->Collided&KPM_PLAYER) && cell->Respawn==0)
        {
          action = 1;
          cell->Respawn = 10*1000;
        }
        break;
      case KKEE_RESPAWN1:
        if((cell->Collided&KPM_PLAYER) && cell->Respawn==0)
        {
          action = 1;
          cell->Respawn = 30*1000;
        }
        break;
      case KKEE_RESPAWN2:
        if((cell->Collided&KPM_PLAYER) && cell->Respawn==0)
        {
          action = 1;
          cell->Respawn = 60*1000;
        }
        break;
      case KKEE_RESPAWN3:
        if((cell->Collided&KPM_PLAYER) && cell->Respawn==0)
        {
          action = 1;
          cell->Respawn = 5*60000;
        }
        break;
      case KKEE_HIT:
        if(cell->Enter&KPM_WEAPONS)
          action = 1;
        break;
      case KKEE_USE:
        if((cell->Collided&KPM_PLAYER) && Player.UseKey)
          action = 1;
        break;
      case KKEE_ALWAYS:
        action = 1;
        break;
      case KKEE_INSIDE:
        action = (cell->Collided&KPM_PLAYER)?1:0;
        break;
      case KKEE_HITORENTER:
        if(cell->Enter&(KPM_PLAYER|KPM_WEAPONS))
          action = 1;
        break;
      }

      valp = (&Player.Life)+(cell->Logic.Output&15);
      switch(cell->Logic.Code&0x0f)
      {
      case KKEC_ENABLESWITCH:
        if(action) 
          Switches[cell->Logic.Output] = 1;
        break;
      case KKEC_DISABLESWITCH:
        if(action) 
          Switches[cell->Logic.Output] = 0;
        break;
      case KKEC_TOGGLESWITCH:
        if(action) 
          Switches[cell->Logic.Output] = !Switches[cell->Logic.Output];
        break;
      case KKEC_SETSWITCH:
        Switches[cell->Logic.Output] = action;
        break;
      case KKEC_SETVALUE:
        if(action) 
          *valp = cell->Logic.Value;
        break;
      case KKEC_MAXVALUE:
        if(action) 
          *valp = sMax<sInt>(*valp,cell->Logic.Value);
        break;
      case KKEC_MINVALUE:
        if(action) 
          *valp = sMin<sInt>(*valp,cell->Logic.Value);
        break;
      case KKEC_INCVALUE:
        if(action) 
        {
          out = cell->Logic.Output&15;
          if(/*(&Player.Life)[out]==0 &&*/ out>=8) // always collect weapon, auch wenn man die waffe schon besitzt
            Player.NextWeapon = out-8;
          *valp = sMin<sInt>(*valp+cell->Logic.Value,(&Player.LifeMax)[out]);
        }
        break;
      case KKEC_DECVALUE:
        if(action) 
          *valp = sMax<sInt>(*valp-cell->Logic.Value,0);
        break;

      case KKEC_JUMP:
        if(action)
          SpeedUp += 0.001f * cell->Logic.Value;
        break;
      case KKEC_RESPAWN:
        if(action)
          Switches[KGS_RESPAWN+(cell->Logic.Value&15)]=1;
        break;
      }
    }

    // done

    cell->Enter = 0;
    cell->Leave = 0;
    cell->Collided = 0;
  }

  // reset cell enter/leave info

  for(i=0;i<DCellUsed;i++)
  {
    dcell = DCellPtr[i];

    dcell->Enter = 0;
    dcell->Leave = 0;
    dcell->Collided = 0;
  }
}

/****************************************************************************/

void KKriegerGame::RespawnAt(sInt i)
{
  if(Respawn[i].Valid || i==1)
  {
    SetPlayer(Respawn[i].Pos,Respawn[i].Dir,0);
    PlayerSound(11);
  }
}

/****************************************************************************/

void KKriegerGame::OnPaint(sMaterialEnv *env)
{
#if !sINTRO

  sInt qc,qcmax;

  sVector v0,v1;
  sMatrix mat;
  sU32 col;

  qcmax = 0x4000;
  qc = 0;
  col = 0xff301020;
  sSystem->GetTransform(sGT_MODELVIEW,mat);
  mat.Trans3();
  v0 = mat.i; v0.Scale3(0.125f);
  v1 = mat.j; v1.Scale3(0.125f);

#endif
}

/****************************************************************************/

void KKriegerGame::OnOptionsChanged()
{
  // update engine usage mask using "shadows" switch
  sU32 usageMask = ~0U & ~(1<<ENGU_SHADOW);

  if(Switches[KGS_SHADOWS])
    usageMask |= 1<<ENGU_SHADOW;

  Engine->SetUsageMask(usageMask);

  // update resolution if necessary
  static const sInt xRes[] = { 640,800,1024,1280 };
  static const sInt yRes[] = { 480,600, 768,1024 };
  sInt res = Switches[KGS_RESOLUTION];

#if sPLAYER
  if(xRes[res] != sSystem->ConfigX) // resolution changed?
  {
    sSystem->ConfigX = xRes[res];
    sSystem->ConfigY = yRes[res];

    sSystem->InitScreens();
  }
#endif
}

/****************************************************************************/
/****************************************************************************/

void KKriegerGame::GetCamera(sMaterialEnv &env)
{
  if(CamZoomPos)
  {
    env.CameraSpace.InitEuler(0,PlayerDir,0);
    env.CameraSpace.l = PlayerPos;
    env.CameraSpace.l.w = 1.0f;
    env.CameraSpace.l.AddScale3(env.CameraSpace.k,-2.0f);
  }
  else
  {
    env.CameraSpace.InitEuler(PlayerLook,PlayerDir,0);
    env.CameraSpace.l = CamPos;
    env.CameraSpace.l.w = 1.0f;
  }
  env.ZoomX = env.ZoomY = 1.0f;
  env.Orthogonal =  sMEO_PERSPECTIVE;
}

void KKriegerGame::GetPlayer(sMatrix &m)
{
  m.Init();
  m.l.z = -5;
}

/****************************************************************************/
/***                                                                      ***/
/***   Sound                                                              ***/
/***                                                                      ***/
/****************************************************************************/

void KKriegerGame::PlayerSound(sInt sound,sF32 volume)
{
  sInt buf;
  sVector zero;

  zero.Init(0,0,0,0);
#if !sPLAYER
  if(GenOverlayManager->SoundEnable)
#endif
  {
    buf = sSystem->SamplePlay(sound,volume);
    sSystem->Sample3DParam(sound,buf,PlayerPos,zero,0.2f,1.0f);
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Game Logic                                                         ***/
/***                                                                      ***/
/****************************************************************************/


struct KKMonsterWalkInfo
{
  sU32 Flags;
  sInt FootCount;
  sF32 StepTresh,RunLowTresh,RunHighTresh;
  sF32 scanup,scandown;
  sInt fgw;
  sF32 slw,ssw,snw;
  sInt fgr;
  sF32 slr,ssr,snr;
  sF323 LegPos[8];
};

KKMonsterWalkInfo KKMonsterWalkInfoTable[5] =
{
  { // robospider
    2,3,0.150f,0.750f,1.250f,2.000f,2.000f,
    3,0.870f,0.390f,0.500f,
    3,1.250f,0.290f,0.350f,
    {
      { -0.500f,0.125f,0.000f },
      {  0.500f,0.125f,0.000f },
      {  0.000f,0.205f,0.750f },
    }
  },
  { // killerbug
    2,6,0.150f,0.750f,1.250f,2.000f,2.000f,
    6,1.255f,0.320f,0.545f,
    6,2.160f,0.155f,0.210f,
    {
      { -1.125f,0,0 },
      {  1.125f,0,0 },
      { -0.625f,0,0 },
      {  0.625f,0,0 },
      { -1.000f,0,-0.125f },
      {  1.000f,0,-0.125f },
    }
  },
  { // hellbug
    2,4,0.150f,0.750f,1.250f,2.000f,2.000f,
    4,1.130f,0.270f,0.510f,
    4,2.160f,0.170f,0.260f,
    {
      { -0.600f,0.250f, 0.500f },
      {  0.600f,0.250f, 0.500f },
      { -0.450f,0.030f,-0.860f },
      {  0.450f,0.030f,-0.875f },
    }
  },
  { // ultimate bot
    2,2,0.150f,1.750f,1.250f,2.000f,2.000f,
    2,0.505f,0.920f,1.125f,
    2,1.470f,0.800f,0.960f,
    {
      { -0.800f,0.125f,0 },
      {  0.800f,0.125f,0 },
    }
  },
  { // no walking (spawn machines)
    2,2,0.150f,1.750f,1.250f,2.000f,2.000f,
    2,0.505f,0.920f,1.125f,
    2,1.470f,0.800f,0.960f,
    {
      { -0.800f,0.125f,0 },
      {  0.800f,0.125f,0 },
    }
  }
};

void KKMonsterWalk(KEnvironment *kenv,KKMonsterWalkMem *mem,KKMonsterWalkInfo *wi,KSpline *stepspline,sMatrix &matrix,KKriegerCellAdd *moncell)
{
  sMatrix mat;//,save;               // local vars
  sMatrix dir;                    // desired orientation of walker
  sVector v,sum,foot[8],spline[8],stepdir;
  sInt i,j;
  sInt bestleg;
  sInt time;
  sF32 dist,f,t;
  sInt changestate;
  KKriegerCellAdd *cell;
  KKriegerCollideInfo ci;
  sVector p0,p1;

  // copy of instance-vars

  sInt State;
  sInt LastLeg;
  sInt NextLeg;

  // parameters

  sInt FootGroup[2];              // groups of feet.
  sF32 StepLength[2];             // max. length of a step
  sInt StepSpeed[2];              // duration of a step in ms.
  sInt StepNext[2];               // when to start the next step in ms. steps may overlap

  // init parameters

  FootGroup[0] = wi->fgw;
  FootGroup[1] = wi->fgr;
  StepLength[0] = wi->slw;
  StepLength[1] = wi->slr;
  StepSpeed[0] = sFtol(wi->ssw*1000);
  StepSpeed[1] = sFtol(wi->ssr*1000);
  StepNext[0] = sMin(sFtol(wi->snw*1000),StepSpeed[0]);
  StepNext[1] = sMin(sFtol(wi->snr*1000),StepSpeed[1]);

// init instance storage

  if(mem->Reset || kenv->TimeReset)
  {
    mem->Reset = 0;
    sum.Init();
    mem->FootCenter.Init();
    mem->InitSteps = wi->FootCount*2;
    for(i=0;i<wi->FootCount;i++)
    {
      mem->FootDelta[i].Init(wi->LegPos[i].x,wi->LegPos[i].y,wi->LegPos[i].z,1);
      p0.Rotate4(matrix,mem->FootDelta[i]);
      p0.w = 1;
      p1 = p0;
      p1.y -= 2.0f;
      ci.Init();
      v = p0;
      cell = moncell;
      if(cell && kenv->Game->CollideRay(cell,p1,p0,ci))
        v = ci.Pos;
      v.w = 0;
      mem->FootOld[i] = mem->FootNew[i] = v;
      mem->StepTime[i] = StepSpeed[0];
      mem->FootCell[i] = cell;
      mem->FootCenter.x += mem->FootDelta[i].x/wi->FootCount;
      mem->FootCenter.z += mem->FootDelta[i].z/wi->FootCount;
    }
    mem->LastTime = kenv->CurrentTime;
    mem->NewPos = matrix.l;
    mem->State = 0;
    mem->LastLeg = 0;
    for(i=0;i<wi->FootCount;i++)
      mem->FootDelta[i].Sub3(mem->FootCenter);
  }

  State = mem->State;
  LastLeg = mem->LastLeg;
  NextLeg = mem->NextLeg;


  // animate

  sum.Init();
  f = 0;
  for(i=0;i<wi->FootCount;i++)          
  {
    t = sRange(1.0f*mem->StepTime[i]/StepSpeed[State],1.0f,0.0f);
    if(i<FootGroup[State])
    {
      f += t;
    }
    if(stepspline)
    {
      stepspline->Eval((t+State)*0.5f,spline[i]);
      if(mem->FootDelta[i].x<0) spline[i].x=-spline[i].x;
      if(mem->FootDelta[i].z<0) spline[i].z=-spline[i].z;
      spline[i].w = spline[i].w-State;
    }
    else
    {
      spline[i].x = 0;
      spline[i].y = sFSin(t*sPI)*0.25f; 
      spline[i].z = 0;
      spline[i].w = t;
    }

    foot[i].Lin3(mem->FootOld[i],mem->FootNew[i],spline[i].w);
    foot[i].w = 1.0f;
    /*
    if(Flags&0x02)
    {
      foot[i].y += spline[i].y;
      mem->FootPart[i].Control(foot[i]);
      foot[i].y -= spline[i].y;
    }
    */
    sum.Add3(foot[i]);
  }
  sum.Scale3(1.0f/wi->FootCount);
  mat.i.Init(0,0,0,0);
  mat.j.Init(0,0,0,0); // ryg 040820
  mat.k.Init(0,0,0,0);
  mat.l.Init(sum.x,sum.y,sum.z,1);
  for(i=0;i<wi->FootCount;i++)
  {
    v.Sub3(foot[i],sum);
    mat.i.AddScale3(v,mem->FootDelta[i].x);
    mat.j.AddScale3(v,mem->FootDelta[i].y);
    mat.k.AddScale3(v,mem->FootDelta[i].z);
  }
  switch((wi->Flags>>2)&3)
  {
  case 0:   // straight, copy from input
    mat.i = matrix.i; 
    mat.j = matrix.j; 
    mat.k = matrix.k; 
    break;
  case 2:   // use xy
    mat.i.Unit3();
    mat.j.Init(0,1,0,0);
    mat.k.Cross4(mat.i,mat.j);
    mat.k.Unit3();
    mat.j.Cross4(mat.k,mat.i);
    mat.j.Unit3();
    break;
  case 1:   // use y
    mat.k = matrix.k;
  case 3:   // use xz
    mat.k.Unit3();
    mat.j.Cross4(mat.k,mat.i);
    mat.j.Unit3();
    mat.i.Cross4(mat.j,mat.k);
    mat.i.Unit3();
    break;
  }

  t = (f+LastLeg)/FootGroup[State];
  if(t>=2.0f) t-=2.0f;
  if(t>=1.0f) t-=1.0f;
  t = (t+State)*0.5f;
  mem->LegTimeOut = t;

  // advance

  time = kenv->CurrentTime-mem->LastTime;
  mem->LastTime = kenv->CurrentTime;
  if(time<0)
    time = 0;
  dir = matrix;
  if(mem->DeviationFlag>0)
  {
    dir.l.Add3(mat.l,mem->Deviation);
  }
  dir.k.y = 0;
  dir.k.Unit3();
  dir.j.Init(0,1,0,0);
  dir.i.Cross4(dir.j,dir.k);
  for(i=0;i<wi->FootCount;i++)
  {
    if(mem->StepTime[i]<StepSpeed[State])
      mem->StepTime[i] += time;
  }

  // find distance

  dist = 0;
  stepdir.Init();
  NextLeg = (LastLeg+1)%FootGroup[State];
  bestleg = NextLeg;
  for(i=0;i<wi->FootCount;i++)
  {
    v.Rotate34(dir,mem->FootDelta[i]);
    v.Sub3(foot[i]);
    v.y = 0;
    f = v.Abs3();
    if(f>dist && mem->StepTime[i]>=StepSpeed[State])
    {
      dist = f;
      stepdir = v;
      bestleg = i;
    }
    v.Rotate3(dir,spline[i]);
//    kenv->Var[KV_LEG_L0+i].Add3(foot[i],v);
//    kenv->Var[KV_LEG_L0+i].w = spline[i].w;
    mem->LegPosOut[i].Add3(foot[i],v);
    mem->LegPosOut[i].w = spline[i].w;
  }

  // change walking/running

  changestate = 1;
  for(i=0;i<wi->FootCount;i++)
    if(mem->StepTime[i]<StepSpeed[State])
      changestate = 0;
  if((State==0 && dist>wi->RunHighTresh) || (State==1 && dist<wi->RunLowTresh))
    changestate +=2;

  // changestate = 0 -> in animation                  ->
  // changestate = 1 -> animation done.               -> issue bestleg when idle
  // changestate = 2 -> in animation, change state    -> don't issue new steps
  // changestate = 3 -> animation done, change state  -> change state really


//  sDPrintF("%08x:state %d change %d dist %f | %4d %4d %4d %4d %4d %4d\n",mem,State,changestate,dist,mem->StepTime[0],mem->StepTime[1],mem->StepTime[2],mem->StepTime[3],mem->StepTime[4],mem->StepTime[5]);
  if(changestate==3)
  {
    if((State==0 && dist>wi->RunHighTresh) || (State==1 && dist<wi->RunLowTresh))
    {
      State=1-State;
      changestate = 0;
      LastLeg = LastLeg%FootGroup[State];
      for(i=0;i<wi->FootCount;i++)
        mem->StepTime[i]=StepSpeed[State];
      NextLeg = bestleg%FootGroup[State];
    }
  }

  bestleg = bestleg%FootGroup[State];
  if(mem->Idle && changestate==1)
    NextLeg = bestleg;

  // do the next step

  mem->Idle = 0;
  if(changestate!=2 && mem->StepTime[LastLeg]>=StepNext[State] && mem->StepTime[NextLeg]>=StepSpeed[State])
  {
    if(mem->InitSteps>0)
    {
      mem->InitSteps--;
      dist = 1024;
    }
    if(mem->DeviationFlag>0)
      mem->DeviationFlag--;
    if(dist>wi->StepTresh)
    {
      v.Sub3(dir.l,mat.l);
      f = v.Abs3();
      if((wi->Flags & 1) && mem->InitSteps==0)
        NextLeg = bestleg;

      if(f>StepLength[State])
      {
        mem->NewPos = mat.l;
        mem->NewPos.AddScale3(v,StepLength[State]/f);
      }
      else
      {
        mem->NewPos = dir.l;
      }
//      mem->NewPos.y = 0;
      dir.l = mem->NewPos;
      j = mem->StepTime[LastLeg]-StepNext[State];
      for(i=NextLeg;i<wi->FootCount;i+=FootGroup[State])
      {
        mem->FootOld[i] = foot[i];//mem->FootNew[i];
//        sDPrintF("soll: %06.3f %06.3f %06.3f --\n ist: %06.3f %06.3f %06.3f\n",mem->FootOld[i].x,mem->FootOld[i].y,mem->FootOld[i].z,dir.l.x,dir.l.y,dir.l.z);
        v = mem->FootDelta[i];
        v.y = 0;
        v.Rotate34(dir);
        if(moncell)
        {
          mem->FootNew[i] = v;        // do the step in any case...
          mem->StepTime[i] = j;

          if(mem->FootCell[i]==0)
            mem->FootCell[i] = moncell;

          kenv->Game->CollisionForMonsterMode = 1;      // and then find a better point--

          p0 = v; p0.y += wi->scanup;
          p1 = v; p1.y -= wi->scandown;
          ci.Init();

          if(kenv->Game->CollideRay(mem->FootCell[i],p1,p0,ci))//FindFirstIntersect(p0,p1,cell,&plane))
          {
            if(ci.Plane.y>0.75 && ci.Pos.y<p0.y)
            {
              mem->FootNew[i] = ci.Pos;
              mem->StepTime[i] = j;
            }
          }

          kenv->Game->CollisionForMonsterMode = 0;
        }
        else
        {
          mem->FootNew[i] = v;
          mem->StepTime[i] = j;
        }
      }
      LastLeg = NextLeg;
    }
    else
    {
      mem->Idle = 1;
    }
  }

  // paint & cleanup

  matrix = mat;

  mem->State = State;
  mem->LastLeg = LastLeg;
  mem->NextLeg = NextLeg;
}

void KKriegerShot::Exit()
{
  if(Event)
  {
    Event->Exit();
    delete Event;
  }
}

sBool KKriegerGame::ShotAI(KKriegerShot *shot)
{
  KKriegerCollideInfo ci;
  sVector newpos,speed;
  sMatrix mat;
  KEvent *event;
  sF32 d;
  sVector dir;

  ci.Init(1<<(KPK_WEAPON0+shot->Weapon),shot->Who);
  
  speed.Sub3(shot->Matrix.l,shot->OldPos);
  if(shot->Mode==2)
    speed.Scale3(0.999f);
  newpos.Add3(speed,shot->Matrix.l);
  newpos.w = 1;
  if(shot->Mode==2)
  {
    newpos.y -= 0.00025f;
    mat.InitRot(shot->Tensor);
    shot->Tensor.Scale3(0.990f);
    shot->Matrix.Mul3(mat);
  }

  shot->OldPos = shot->Matrix.l;
  shot->Matrix.l = newpos;
  if(CollideRay(shot->Cell,shot->Matrix.l,shot->OldPos,ci))
  {
    if(shot->Mode==2 && !shot->DoubleKill)
    {
      shot->Matrix.l = ci.Pos;
      shot->Matrix.l.AddScale3(ci.Plane,0.125f);
      shot->DoubleKill=1;
      dir = shot->Tensor;
      shot->Tensor.Cross3(dir,ci.Plane);
      dir.Cross3(speed,ci.Plane);
      shot->Tensor.AddScale3(dir,-0.5f);
      speed.Scale3(0.75f);
      shot->OldPos.Sub3(shot->Matrix.l,speed);
      d = speed.Dot3(ci.Plane);
      shot->OldPos.AddScale3(ci.Plane,d*2.0f);
    }
    else
    {
      shot->Matrix.l = ci.Pos;

      event = new KEvent;
      event->Init();
      event->Op = WeaponExplode[0][shot->Weapon];
      event->Matrix.InitDir(ci.Plane);
      event->Matrix.l = shot->Matrix.l;
      Environment->AddDynamicEvent(event);
      return sFALSE;
    }
  }
  shot->DoubleKill = 0;
  if(ci.DCell && ci.DCell->Monster && shot->Who==KCRF_ISPLAYER)
  {
    ci.DCell->Monster->Hit(WeaponHits[shot->Weapon],this);

    event = new KEvent;
    event->Init();
    event->Op = WeaponExplode[1][shot->Weapon];
    event->Matrix = shot->Matrix;
    Environment->AddDynamicEvent(event);

    return sFALSE;
  }
  if(ci.DCell && ci.DCell==&Player.HitCell && shot->Who==KCRF_ISMONSTER)
  {
    Player.Hit(WeaponHits[shot->Weapon]);
    return sFALSE;
  }
  return sTRUE;
}


void KKriegerGame::MonsterAI(KKriegerMonster *mon,KEnvironment *kenv,sBool machinelife,sBool prevlife)
{
  sF32 dist;
  sVector delta;
  sVector pos;
  sVector speed;
  sVector v;
  KKriegerCellAdd *cadd;
//  KKriegerCellAdd *cell;
  sVector p0,p1;
  sF32 attackdist;
  sBool spawn;
  KKriegerCollideInfo ci;
//  sMatrix mat;
//  sInt i;

  // kill monster

  if(mon->Life<=0)
  {
    mon->State = KMS_IDLE;
    mon->Life = 0;
    mon->DeathEvent.Matrix = mon->Matrix;
    return;
  }

  // if no player collision, do nothing

  if(Player.Collider.Cell==0)
    return;

  // spawn monster

  if(mon->State==KMS_IDLE)
  {
    spawn = 1;                  // normally, all monsters spawn immediatly
    if((mon->Flags&KMF_WHENPREVDEAD) && prevlife)
      spawn = 0;                // spawning not allowed when previous is alife
    if((mon->Flags&KMF_WHENMACHINELIVES) && !machinelife)
      spawn = 0;                // spawning not allowed when machine is dead
    if(!kenv->Game->Switches[mon->SpawnSwitch])
      spawn = 0;                // spawning not allowed by switch
    if(spawn)
      mon->State = KMS_SPAWNED;
  }

  // calculate distance

  if(mon->State!=KMS_IDLE)
  {
    pos = mon->Collider.Pos;
    delta.Sub3(pos,Player.Collider.Pos);
    delta.y = 0;
    dist = delta.UnitAbs3();
  }

  // monster gets in range

  if(mon->State==KMS_SPAWNED && dist<mon->NoticeRadius)
  {
//    cell = FindCell(mon->Collider.Pos);
//    if(cell)
    {      
      mon->State=KMS_INRANGE;
    }
  }

  // monster goes out of range

  if((mon->State==KMS_INRANGE || mon->State==KMS_ACTIVE) && dist>mon->NoticeRadius*1.5f)
    mon->State=KMS_SPAWNED;

  // if monster sees player, it gets active.
  // it is not wise to deactivae monster when it looses the line of sight.
  // monsters also get active when shot at.

  if(mon->State==KMS_INRANGE)
  {
    p0 = Player.Collider.Pos; 
    p1 = mon->Collider.Pos;
    p0.y += 1.5f;
    p1.y += 1.5f;
    cadd = PlayerCell;
    ci.Init();
    if(!CollideRay(cadd,p1,p0,ci))
      mon->State=KMS_ACTIVE;
  }

  // now the real monster AI

  if(mon->State==KMS_ACTIVE)
  {
    mon->MeeleeTimer += 0.01f/mon->MeeleeTime;
    if(mon->MeeleeTimer>=1.0f)
      mon->MeeleeTimer = 1.0f;
    mon->RangedTimer += 0.01f/mon->RangedTime;
    if(mon->RangedTimer>=1.0f)
      mon->RangedTimer = 1.0f;

    v.Sub3(mon->Collider.Pos,PlayerPos);
    attackdist = v.Dot3(v);

    if(attackdist < 1.5f*1.5f)
    {
      if(mon->RangedTimer==1.0f && mon->MeeleeTimer==1.0f && mon->MeeleeHits>0)
      {
        mon->MeeleeTimer = 0.0f;
        Player.Hit(mon->MeeleeHits);
      }
    }
    else
    {
      if(mon->RangedTimer==1.0f && mon->MeeleeTimer==1.0f && mon->RangedHits>0 && mon->WeaponKind>=0)
      {
        sVector p0,p1;
        KKriegerCellAdd *cell;

        mon->RangedTimer = 0.0f;
        p0 = Player.Collider.Pos;
        p0.y += 1.5f;
        p1 = mon->Collider.Pos;
        p1.y += 1.5f;
        ci.Init();
        cell = Player.Collider.Cell;
        if(!CollideRay(cell,p1,p0,ci))
        {
          p0.Sub4(p1);
          p0.Unit3();
          FireShot(kenv,mon->WeaponKind,mon,&p0);
        }
      }
    }

    if(dist<mon->ChargeRadius)
    {
      dist -= mon->MinRadius;
      dist = sRange<sF32>(dist,mon->Speed,-mon->Speed);
      speed.Scale3(delta,-dist);
    }
    else
    {
      dist -= mon->WaitRadius;
      dist = sRange<sF32>(dist,mon->Speed,-mon->Speed);
      speed.Scale3(delta,-dist);
    }

    if(!(mon->Flags & KMF_IMMOBILE) && mon->Collider.Cell)
    {
      sVector p0;
      sVector p1;
      sF32 d,testh,upforce;
      KKriegerCellAdd *cell;
      KKriegerCollideInfo ci;

      testh = StairHeight + GravityPlayer + mon->Collider.Radius;
      d = testh;
      ci.Init(0,KCRF_ISMONSTER);
      p0 = mon->Collider.Pos;
      p0.AddScale3(mon->Matrix.k,mon->Collider.Radius);
      p1 = p0; 
      p1.y -= testh;
      cell = mon->Collider.Cell;

      v.Sub3(mon->Collider.Pos,mon->Collider.OldPos);
      speed.Add3(mon->MagnetForce);
      speed.y = 0;
      speed.w = 0;
      speed.AddScale3(v,0.9f);

      mon->HitCell.Mode |= KCM_DISABLED;
      if(CollideRay(cell,p1,p0,ci))
        d += ci.Dist;
      mon->HitCell.Mode &= ~KCM_DISABLED;

      upforce = 0;
      if(d<testh)
      {
        upforce += (testh-d)*0.1f;
      }
      else
      {
        speed.y -= GravityPlayer;
      }

      mon->Collider.OldPos = mon->Collider.Pos;
      mon->Collider.OldPos.y += upforce;
      speed.y += upforce;
      MoveCollider(mon->Collider,speed,KCRF_ISMONSTER);
//      mon->Collider.Pos.Add3(speed);

      mon->Matrix.i.Init(-delta.z,0,delta.x,0);     // make monster look to player
      mon->Matrix.j.Init(0,1,0,0);
      mon->Matrix.k.Init(-delta.x,0,-delta.z,0);
      mon->Matrix.l = mon->Collider.Pos;

      KKMonsterWalk(kenv,&mon->WalkMem,&KKMonsterWalkInfoTable[mon->WalkStyle],mon->Spline,mon->Matrix,mon->Collider.Cell);

//      mon->Matrix.l = mon->Collider.Pos;
//      mon->Collider.Pos = mon->Matrix.l;
    }

    if(mon->Collider.Cell==0)
    {
      mon->Hit(65536,kenv->Game);
    }
  }

  // set event for painting

  mon->Event.Matrix = mon->Matrix;

  mon->Event.Scale.x = mon->MeeleeTimer-0.5f;
  if(mon->Event.Scale.x<0) mon->Event.Scale.x+=1;
  mon->Event.Scale.y = mon->RangedTimer-0.5f;
  if(mon->Event.Scale.y<0) mon->Event.Scale.y+=1;
  mon->Event.Scale.z = 1.0f * mon->Life / mon->LifeMax;

  mon->Event.CullDist = 35;
}

// gegenseitige abstoﬂung der monster

void KKriegerGame::MonsterMagnetAI()
{
  KKriegerMonster *mons[256];
  KKriegerMonster *mon0,*mon1;
  sInt count;
  sInt i,j;
  sF32 dist,radius;
  sVector v;

  // liste aller aktiven monster

  count = 0;
  for(i=0;i<Monsters.Count;i++)
  {
    mon0 = Monsters.Get(i);
    mon0->MagnetForce.Init(0,0,0,0);
    if(mon0->State==KMS_ACTIVE && count<256)
      mons[count++] = mon0;
  }

  // n^2/2 tests

  radius = 1.0f;
  for(i=0;i<count-1;i++)
  {
    mon0 = mons[i];
    for(j=i+1;j<count;j++)
    {
      mon1 = mons[j];
      v.Sub3(mon0->Collider.Pos,mon1->Collider.Pos);
      dist = v.Dot3(v);
      if(dist<radius*radius)
      {
        dist = 1.0f-sFSqrt(dist)/radius;
        if(dist>0.99f)
          v.InitRnd();    // vermeide einheitsvector von nullvector..
        else
          v.Unit3();
        mon0->MagnetForce.AddScale3(v,dist*0.001f);
        mon1->MagnetForce.AddScale3(v,dist*-0.001f);
      }
    }
  }
  
}

/****************************************************************************/
/***                                                                      ***/
/***   Particles and Collision                                            ***/
/***                                                                      ***/
/****************************************************************************/

void KKriegerGame::FlushPhysic()
{
//  sInt i;
  PartUsed = 0;
  ConsUsed = 0;
  DCellUsed = 0;
  Monsters.Count = 0;
  AddDCell(&Player.HitCell);
}

void KKriegerGame::AddDCell(KKriegerCellDynamic *d)
{
  sVERIFY(DCellUsed < KKRIEGER_MAXDCELL);
  DCellPtr[DCellUsed++] = d;
}

/****************************************************************************/

void KKriegerGame::Physic(sF32 fade)
{
  sInt i;
  KKriegerCellDynamic *cell;

#if sPROFILE
  sZONE(GamePhysic);

  CallFindFirstIntersect = 0;
  CallFindSubIntersect = 0;
  CallMoveParticle = 0;
  CallOutsideMask = 0;
  CallIntersectOut = 0;
  CallIntersectIn = 0;
#endif

  // dynamic cells

  for(i=0;i<DCellUsed;i++)
  {
    cell = DCellPtr[i];
    cell->CurrentMatrix.Lin4(cell->Matrix0,cell->Matrix1,fade);
    cell->ApplyMatrix(cell->CurrentMatrix);
    cell->ConfirmedMatrix = cell->CurrentMatrix;
    cell->Blocked = 0;
    cell->Mode &= ~KCM_UNCONFIRMED;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Totally new math                                                   ***/
/***                                                                      ***/
/****************************************************************************/


sBool KKriegerGame::CollideRay(
  KKriegerCellAdd *&cadd,         // old / new add-cell
  const sVector &p1,              // move to here
  const sVector &p0,              // move from here
  KKriegerCollideInfo &ci)        // collide info, or 0
{
  sInt i,j;
  sVector v;
  KKriegerCellAdd *list[16];
  KKriegerCollideInfo ci2;
  sInt count;
  sVector d;
  sBool isnotplayer;

  GAMEPERF(CallFindFirstIntersect);
  list[0] = cadd;
  count = 1;

  isnotplayer = !(ci.Flags&KCRF_ISPLAYER);
  d.Sub4(p1,p0);

again:

// see if we leave this cadd

  if(cadd->IntersectOut(p1,d,ci.Plane,ci.Dist))
  {
    v = p1;
    v.AddScale3(d,ci.Dist);
    ci.Pos = v;
    if(count<16)
    {
      for(i=0;i<cadd->Adds.Count;i++)
      {
        sVERIFY(cadd->Adds[i]!=cadd);
        if(cadd->Adds[i]->OutsideMask(v)==0)
        {
          for(j=0;j<count;j++)
            if(list[j]==cadd->Adds[i])
              goto retry;

          cadd = cadd->Adds[i];
          list[count++] = cadd;
          goto again;
        }
retry:;
      }
    }
    else
    {
#if LOGGING
      sDPrintF("FindFirstIntersect(): to many retries\n");
#endif
    }

    // we collided with the outside of an add, even after following all 
    // connected adds. we have a list of all visited adds in (list,count)
    // and we need to check them for a collision that might be before the
    // collision with the add's outside wall.

    ci2 = ci;
    if(CollideRaySub(d,p1,p0,list,count,ci2))
    {
      if(ci2.Dist<ci.Dist)
      {
        ci.Dist  = ci2.Dist;
        ci.Plane = ci2.Plane;
        ci.Pos   = ci2.Pos;
      }
    }
    return sTRUE;
  }

  // we are well in add, check subs
  // add check for 2 cadd'S here!

  return CollideRaySub(d,p1,p0,list,count,ci);
}

// find nearest intersection with subcell.
// .. add code to check zones here ..

sBool KKriegerGame::CollideRaySub(const sVector &d,const sVector &p1,const sVector &p0,KKriegerCellAdd **list,sInt count,KKriegerCollideInfo &ci)
{
  sInt i,j;
  sBool collided;
  KKriegerCell *cell;
  KKriegerCellAdd *cadd;
  KKriegerCellDynamic *dcell;
  sInt isnotplayer;
  sBool hit0,hit1;

  GAMEPERF(CallFindSubIntersect);

  isnotplayer = !(ci.Flags&KCRF_ISPLAYER);
  collided = 0;
  ci.Dist = (sF32) EPSILON;
  for(j=0;j<count;j++)
  {
    cadd = list[j];
    for(i=0;i<cadd->Subs.Count;i++)
    {
      cell = cadd->Subs[i];
      if((cell->Mode&3)==KCM_SUB)
      {
        if(CollideRaySub2(d,p1,p0,cell,ci))
        {
          collided = 1;
          ci.DCell = 0;
//          if(SomethingBadHasHappened) goto fatal;
        }
      }
    }

    for(i=0;i<cadd->Zones.Count;i++)
    {
      cell = cadd->Zones[i];
      hit0 = (cell->OutsideMask(p0)==0);
      hit1 = (cell->OutsideMask(p1)==0);

      if(hit0 && !hit1) cell->Leave |= ci.ZoneMask;
      if(!hit0 && hit1) cell->Enter |= ci.ZoneMask;
      if(hit1) cell->Collided |= ci.ZoneMask;
    }
  }
  for(i=0;i<DCellUsed;i++)
  {
    cell = dcell = DCellPtr[i];
    if((cell->Mode&3)==KCM_SUB)
    {
      if(CollideRaySub2(d,p1,p0,cell,ci))
      {
        collided = 1;
        ci.DCell = dcell;
//        if(SomethingBadHasHappened) goto fatal;
      }
    }
    if((cell->Mode&3)==KCM_ZONE)
    {
      hit0 = (cell->OutsideMask(p0)==0);
      hit1 = (cell->OutsideMask(p1)==0);

      if(hit0 && !hit1) cell->Leave |= ci.ZoneMask;
      if(!hit0 && hit1) cell->Enter |= ci.ZoneMask;
      if(hit1)
      {
        cell->Collided |= ci.ZoneMask;
        ci.DCell = dcell;
      }
    }
  }

//fatal:
  return collided;
}

sBool KKriegerGame::CollideRaySub2(const sVector &d,const sVector &p1,const sVector &p0,KKriegerCell *cell,KKriegerCollideInfo &ci)
{
  sF32 besti;
  sVector v,planei;
  sBool collided;
  sF32 distresult;

  // find any rejection plane
  if(cell->RejectPlane(p0,p1))
    return 0;

  // if there's none, we have a collision
  collided = 0;
  distresult = ci.Dist;
  if(cell->IntersectIn(p1,d,planei,besti,ci.Radius))
  {
    if(besti<distresult)
    {
      v = p1;
      v.AddScale3(d,besti);
      if(cell->OutsideMask(v,ci.Radius-EPSILON)==0)
      {
        distresult = besti;
        collided = 1;

        ci.Dist = distresult;
        ci.Plane = planei;
        ci.Plane.Neg4();
        ci.Pos = p1;
        ci.Pos.AddScale3(d,distresult);
      }
    }
  }

  return collided;
}


/****************************************************************************/
/***                                                                      ***/
/***   New Math                                                           ***/
/***                                                                      ***/
/****************************************************************************/


// calculate which planes are out
// positive epsilons will enlarge the box!



sBool KKriegerCell::RejectPlane(const sVector &v0,const sVector &v1)
{
  sInt i;

  for(i=0;i<PlaneCount;i++)
  {
    if(Planes[i].x*v0.x+Planes[i].y*v0.y+Planes[i].z*v0.z+Planes[i].w<0)
      if(Planes[i].x*v1.x+Planes[i].y*v1.y+Planes[i].z*v1.z+Planes[i].w<0)
        return sTRUE;
  }

  return sFALSE;
}

sInt KKriegerCell::OutsideMask(const sVector &v,sF32 radius)    // normal points inside!
{
  sInt i;
  sInt mask;
  sF32 f;

  GAMEPERF(KKriegerGame::CallOutsideMask);

  mask = 0;
  for(i=0;i<PlaneCount;i++)
  {
    f = Planes[i].x*v.x+Planes[i].y*v.y+Planes[i].z*v.z+Planes[i].w-radius;
    if(f<0)
      mask |= 1<<i;
  }
  return mask;
}

// calculate intersection point ray -> cell when casting from inside to outside

sBool KKriegerCell::IntersectOut(const sVector &p,const sVector &d,sVector &plane,sF32 &distresult,sF32 radius)
{
  sInt bestplane;
  sInt i;
  sF32 dist,det;
  sF32 best;

  GAMEPERF(KKriegerGame::CallIntersectOut);

  best = 0;
  bestplane = -1;
  for(i=0;i<PlaneCount;i++)
  {
    dist = Planes[i].Dot3(p)+Planes[i].w-radius;
    if(dist<0)
    {
      det = Planes[i].Dot3(d);
      if(det>-1.0e-16)
      {
#if LOGGING
        sDPrintF("KKriegerCell::IntersectOut() Panic\n");
#endif
//        return sFALSE;
      }
      dist = -dist/det;
      if(dist<best)
      {
        best = dist;
        bestplane = i;
      }
    }
  }
  if(bestplane>=0)
  {
    plane = Planes[bestplane];
    distresult = best;
    return sTRUE;
  }
  return sFALSE;
}

// calculate intersection point ray -> cell when casting from the outside

sBool KKriegerCell::IntersectIn(const sVector &p,const sVector &d,sVector &plane,sF32 &distresult,sF32 radius)
{
  sInt bestplane;
  sInt i;
  sF32 dist,det;
  sF32 best;

  GAMEPERF(KKriegerGame::CallIntersectIn);

  best = -1024*1024;
  bestplane = -1;
  for(i=0;i<PlaneCount;i++)
  {
    dist = Planes[i].Dot3(p)+Planes[i].w-radius;
    det = Planes[i].Dot3(d);
    if(det>1.0e-16)
    {
      dist = -dist/det;
      if(dist>best)
      {
        best = dist;
        bestplane = i;
      }
    }
    else
    {
#if LOGGING
      sDPrintF("KKriegerCell::IntersectIn() Panic\n");
#endif
    }
  }
  if(bestplane>=0)
  {
    plane = Planes[bestplane];
    distresult = best;
    return sTRUE;
  }
  return sFALSE;
}

/****************************************************************************/

// find an ADD cell.

KKriegerCellAdd *KKriegerGame::FindCell(const sVector &v)
{
  sInt i;
  for(i=0;i<CellAdd.Count;i++)
  {
    if(CellAdd[i]->OutsideMask(v)==0)
      return CellAdd[i];
  }
  return 0;
}

// faked code to check CollideRay

sBool KKriegerGame::MoveColliderX(KKSolidCollider &collider, const sVector &v,sInt who)
{
  KKriegerCollideInfo ci;
  KKriegerCellAdd *cell;
  sVector old;

  ci.Init((who&KCRF_ISPLAYER)?KPM_PLAYER:0,who);
  ci.Radius = collider.Radius;
  old = collider.Pos;
  collider.Pos.Add3(v);
  if(CollideRay(collider.Cell,collider.Pos,old,ci))
  {
    collider.Pos = ci.Pos;
  }

  if(collider.Cell->OutsideMask(collider.Pos,0)!=0)
  {
    cell = FindCell(collider.Pos);
    if(cell)
    {
      collider.Cell = cell;
      sDPrintF("Collision: Rescued Desaster\n");
    }
    else
    {
      sDPrintF("Collision: Lasting Desaster\n");
    }    
  }

  return sFALSE;
}

// cool code that does not work

sBool KKriegerGame::MoveCollider(KKSolidCollider &collider, const sVector &v,sInt who)
{
  sVector tmp;
  KKriegerCellAdd *cell;
  KKriegerCellAdd *list[16];
  sInt count;
  sVector sphere;
  sInt i;

  tmp = collider.Pos;
  collider.Pos.Add3(v);//collider.Movement);

  sphere = collider.Pos;
  sphere.w = collider.Radius;

  count = 1;
  list[0] = collider.Cell;
  CollideSoftSphereAdd(sphere, list, count);
  for(i = 0; i < DCellUsed; i++)
  {
    if((who & KCRF_ISMONSTER) && DCellPtr[i]->Monster) continue;
    if((who & KCRF_ISPLAYER)  && DCellPtr[i] == &Player.HitCell) continue;

    CollideSoftSphereSub(sphere, DCellPtr[i]);
  }
  collider.Pos = sphere;
  collider.Pos.w = 1.0f;

  // update cell (there might be a cheaper way...)

  KKriegerCollideInfo ci;

  ci.Init((who&KCRF_ISPLAYER)?KPM_PLAYER:0,who);

  CollideRay(collider.Cell,collider.Pos,tmp,ci);

  if(collider.Cell->OutsideMask(collider.Pos,0)!=0)
  {
    cell = FindCell(collider.Pos);
    if(cell)
    {
      collider.Cell = cell;
      sDPrintF("Collision: Rescued Desaster\n");
    }
    else
    {
      sDPrintF("Collision: Lasting Desaster\n");
    }    
  }
  return sFALSE;
}

void KKriegerGame::CollideSoftSphereAdd(sVector &sphere, KKriegerCellAdd **cellList, sInt &count)
{
  KKriegerCellAdd *cell;
  int i, j;

  cell = cellList[count-1];
  if(sFAbs(cell->approxDistance(sphere)) < sphere.w)
  {
    GenSimpleFaceList *face;

    face = cell->Faces;
    while(face != 0)
    {
      CollideSoftSphereFace(sphere, face->Vertices, face->VertexCount);
      face = face->Next;
    }
  }

  for(i = 0; i < cell->Adds.Count; i++)
  {
    if(cell->Adds[i]->approxDistance(sphere) < sphere.w)
    {
      for(j = 0; j < count; j++)
        if(cell->Adds[i] == cellList[j])
          break;

      if(j == count && count < 16)
      {
        cellList[count++] = cell->Adds[i];
        CollideSoftSphereAdd(sphere, cellList, count);
      }
    }
  }

  for(i = 0; i < cell->Subs.Count; i++)
    if(cell->Subs[i]->approxDistance(sphere) < sphere.w)
      CollideSoftSphereSub(sphere, cell->Subs[i]);
}

void KKriegerGame::CollideSoftSphereSub(sVector &sphere, KKriegerCell *cell)
{
  sVector plane[4];
  static const sInt order[6][4] = 
  {
    { 0,2,3,1 },
    { 2,6,7,3 },
    { 3,7,5,1 },
    { 4,0,1,5 },
    { 2,0,4,6 },
    { 7,6,4,5 },
  };
  int i;

  for(i = 0; i < 6; i++)
  {
    plane[0] = cell->Vertices[order[i][0]];
    plane[1] = cell->Vertices[order[i][1]];
    plane[2] = cell->Vertices[order[i][2]];
    plane[3] = cell->Vertices[order[i][3]];
    CollideSoftSphereFace(sphere, plane, 4);
  }
}

sF32 KKriegerCell::approxDistance(const sVector &p)
{
  sInt i;
  sF32 result, distance;

  result = 10000;
  for(i = 0; i < PlaneCount; i++)
  {
    distance = p.Dot3(Planes[i]) + Planes[i].w;
    if(distance < result)
      result = distance;
  }

  return -result;
}

void KKriegerGame::CollideSoftSphereFace(sVector &sphere, const sVector *vertices, int numVertices)
{
  sVector collPoint, shift;
  sF32 length, distance;

  if(CheckSphereVsFace(sphere, vertices, numVertices, collPoint))
  {
    shift.Sub3(sphere, collPoint);
    length = shift.Abs3();
    distance = (sphere.w - length) * 0.5f;
    sphere.AddScale3(shift, distance / length);
    sphere.w -= distance;
  }
}

sBool KKriegerGame::CheckSphereVsFace(const sVector &sphere, const sVector *vertices, int numVertices, sVector &nearestPoint)
{
  sVector normal;
  sVector a, b, toSphere;
  sVector tmp2, tmp;
  sVector sphereCopy;
  sF32 distance;
  sInt i;
  sBool result, inside;

  a.Sub3(vertices[2], vertices[1]);
  b.Sub3(vertices[0], vertices[1]);
  normal.Cross3(a, b);
  if(normal.Abs3() <= EPSILON)
    return false;
  normal.Unit3();

  toSphere.Sub3(sphere, vertices[0]);
  distance = normal.Dot3(toSphere);
  // early out if sphere to far from plane
  if(sFAbs(distance) > sphere.w)
    return sFALSE;

  // now test for each edge whether this point lies inside the face
  // if it lies outside, check for collision with the edge
  sphereCopy = sphere;
  result = sFALSE;
  inside = sTRUE;
  for(i = 0; i < numVertices; i++)
  {
    tmp.Sub3(vertices[(i+1) % numVertices], vertices[i]);
    tmp2.Cross3(tmp, normal);
    tmp.Sub3(sphere, vertices[i]);
    if(tmp.Dot3(tmp2) > 0)
    {
      inside = sFALSE;
      // check edge
      if(CheckSphereVsEdge(sphereCopy, vertices[i], vertices[(i+1) % numVertices], nearestPoint))
      {
        result = sTRUE;
        sphereCopy.w = sphereCopy.Distance(nearestPoint);
      }
    }
  }

  if(inside == sFALSE)
    return result;
  
  // just find and return the nearest point on the plane
  nearestPoint.Scale3(normal, -distance);
  nearestPoint.Add3(sphere);
  return sTRUE;
}

sBool KKriegerGame::CheckSphereVsEdge(const sVector &sphere, const sVector &v1, const sVector &v2, sVector &nearestPoint)
{
  sVector unit, toSphere;
  sF32 a, length;

  unit.Sub3(v2, v1);
  length = unit.Abs3();
  if(length < EPSILON)
    return false;
  unit.Scale3(1.0f / length);
  toSphere.Sub3(sphere, v1);
  a = unit.Dot3(toSphere);
  if(a < 0)
  {
    // test vertice 1
    if(toSphere.Abs3() > sphere.w)
      return sFALSE;
    nearestPoint = v1;
    return sTRUE;
  }
  if(a > length)
  // the second vertice is tested as the first one of the next edge if appropriate
    return sFALSE;
  
  nearestPoint.Scale3(unit, a);
  nearestPoint.Add3(v1);
  toSphere.Sub3(sphere, nearestPoint);
  if(toSphere.Abs3() > sphere.w)
    return sFALSE;
  return sTRUE;
}

#define SPLITHEAPMAX 1024
sVector SplitHeapVector[2][SPLITHEAPMAX];
sInt SplitHeapIndex[2];

void KKriegerGame::CreateCollisionFaces(KKriegerCellAdd &add)
{
  GenSimpleFaceList *cur, *next;
  int i;
  static const sInt order[6][4] = 
  {
    { 0,2,3,1 },
    { 2,6,7,3 },
    { 3,7,5,1 },
    { 4,0,1,5 },
    { 2,0,4,6 },
    { 7,6,4,5 },
  };

  // add original faces
  add.DeleteFaces();
  for(i = 0; i < 6; i++)
  {
    cur = new GenSimpleFaceList;
    cur->Init(4);
    cur->Vertices[0] = add.Vertices[order[i][0]];
    cur->Vertices[1] = add.Vertices[order[i][1]];
    cur->Vertices[2] = add.Vertices[order[i][2]];
    cur->Vertices[3] = add.Vertices[order[i][3]];
    cur->Next = add.Faces;
    add.Faces = cur;
  }

  for(i = 0; i < add.Adds.Count; i++)
  {
    cur = add.Faces;
    add.Faces = 0;
    while(cur != 0)
    {
      next = cur->Next;
      SplitHeapIndex[0] = 0;
      SplitHeapIndex[1] = 0;
      if(SplitCollisionFace(*cur, add.Adds[i]->Planes, 0, add.Adds[i]->PlaneCount, add))
      {
        cur->Next = add.Faces;
        add.Faces = cur;
      }
      else
      {
        cur->Exit();
        delete cur;
      }
      sVERIFY(SplitHeapIndex[0]==0);
      sVERIFY(SplitHeapIndex[1]==0);

      cur = next;
    }
  }
}

sBool KKriegerGame::SplitCollisionFace(const GenSimpleFace &face, const sVector *planes, sInt plane, sInt nPlanes, KKriegerCellAdd &add)
{
  GenSimpleFace sides[2];
  sBool use[2];
  sInt i, j;
  sVector center;
  sVector normal;
  
//  debug_count_splits++;
  if(nPlanes == 0)
    return sTRUE;

  {
    sVector a, b;
    a.Sub3(face.Vertices[2], face.Vertices[1]);
    b.Sub3(face.Vertices[0], face.Vertices[1]);
    normal.Cross3(a, b);
  }

  sides[0].Vertices = &SplitHeapVector[0][SplitHeapIndex[0]];
  sides[1].Vertices = &SplitHeapVector[1][SplitHeapIndex[1]];
  face.Clip(planes[plane], sides);
  SplitHeapIndex[0] += sides[0].VertexCount;
  SplitHeapIndex[1] += sides[1].VertexCount;
  sVERIFY(SplitHeapIndex[0]<SPLITHEAPMAX)
  sVERIFY(SplitHeapIndex[1]<SPLITHEAPMAX)


  for(i = 0; i < 2; i++)
  {
    use[i] = sides[i].VertexCount > 2;
    
    if(use[i] && plane == nPlanes - 1)
    {
      use[i] = sFALSE;
      center.Init3();
      for(j = 0; j < sides[i].VertexCount; j++)
        center.Add3(sides[i].Vertices[j]);
      center.Scale3(1.0f / sides[i].VertexCount);
      for(j = 0; j <= nPlanes; j++)
      {
        float d = planes[j].Dot3(center) + planes[j].w;
        if(d < 0 || (d < EPSILON*4 && planes[j].Dot3(normal) > 0))
          use[i] = sTRUE;
      }
    }

    if(use[i] && plane < nPlanes - 1)
      use[i] = SplitCollisionFace(sides[i], planes, plane + 1, nPlanes, add);
  }

  if(!(use[0] && use[1]))
  {
    for(i = 0; i < 2; i++)
    {
      if(use[i])
      {
        GenSimpleFaceList *NewFace = new GenSimpleFaceList;
        NewFace->Init(sides[i].VertexCount);
        for(j = 0; j < sides[i].VertexCount; j++)
          NewFace->Vertices[j] = sides[i].Vertices[j];
        NewFace->Next = add.Faces;
        add.Faces = NewFace;
      }
    }
  }

  SplitHeapIndex[0] -= sides[0].VertexCount;
  SplitHeapIndex[1] -= sides[1].VertexCount;

//  sides[0].Exit();
//  sides[1].Exit();

  return use[0] && use[1];
}

void KKriegerCellAdd::DeleteFaces()
{
  GenSimpleFaceList *cur, *next;

  cur = Faces;
  while(cur != 0)
  {
    next = cur->Next;
    cur->Exit();
    delete cur;
    cur = next;
  }
}

/****************************************************************************/
/***                                                                      ***/
/***   Operators                                                          ***/
/***                                                                      ***/
/****************************************************************************/

#pragma lekktor(on)

void __stdcall Exec_KKrieger_Para(KOp *op,KEnvironment *kenv)
{
  sU32 *du;
  sF32 *df;
  sInt i;

  du = op->GetAnimPtrU(0);
  df = op->GetAnimPtrF(0);

  kenv->Game->Player.LifeMax    = du[1];
  kenv->Game->Player.ArmorMax  = du[3];
  kenv->Game->Player.AlphaMax   = du[5];
  kenv->Game->Player.BetaMax    = du[7];
  kenv->Game->Player.AmmoMax[0] = du[9];
  kenv->Game->Player.AmmoMax[1] = du[11];
  kenv->Game->Player.AmmoMax[2] = du[13];
  kenv->Game->Player.AmmoMax[3] = du[15];
  if(kenv->Game->ResetByOp)
  {
    kenv->Game->ResetByOp = 0;
    kenv->Game->Player.Life    = du[0];
    kenv->Game->Player.Armor  = du[2];
    kenv->Game->Player.Alpha   = du[4];
    kenv->Game->Player.Beta    = du[6];
    kenv->Game->Player.Ammo[0] = du[8];
    kenv->Game->Player.Ammo[1] = du[10];
    kenv->Game->Player.Ammo[2] = du[12];
    kenv->Game->Player.Ammo[3] = du[14];
    for(i=0;i<8;i++)
      kenv->Game->Player.Weapon[i] = (du[16]>>i)&1;;
  }

  kenv->Game->GravityPlayer     = df[17];
  kenv->Game->DampPlayerGround  = df[18];
  kenv->Game->DampPlayerAir     = df[19];
  kenv->Game->DampPlayerHeight  = df[20];
  kenv->Game->StairHeight       = df[21];
  kenv->Game->EyeHeight         = df[22];
  kenv->Game->MouseTurnSpeed    = df[23];
  kenv->Game->MouseLookSpeed    = df[24];
  kenv->Game->AccelSideFactor   = df[25];
  kenv->Game->AccelForwFactor   = df[26];
  kenv->Game->AccelBackFactor   = df[27];
  kenv->Game->JumpForce         = df[28];
  kenv->Game->PlayerStartPos.Init(df[29],df[30],df[31]);
  kenv->Game->DampPlayerCam     = df[56];


  sCopyMem(kenv->Game->WeaponHits,du+32,24*4);

  kenv->Game->CamOffset = kenv->Game->EyeHeight-kenv->Game->StairHeight;

  op->ExecInputs(kenv);
}

void __stdcall Exec_KKrieger_Events(KOp *op,KEnvironment *kenv)
{
  KOp **ptr;
  sInt i;

  switch(*op->GetAnimPtrU(0))
  {
  case 0:
  default:
    ptr = kenv->Game->WeaponShot;
    break;
  case 1:
    ptr = kenv->Game->WeaponOptics;
    break;
  case 2:
    ptr = kenv->Game->WeaponExplode[0];
    break;
  case 3:
    ptr = kenv->Game->WeaponExplode[1];
    break;
  }

  for(i=0;i<8;i++)
    ptr[i] = op->GetLink(i);

  op->ExecInputs(kenv);
}

class KObject * __stdcall Init_KKrieger_Para(KOp *op,KEnvironment *kenv)
{
  sF32 *df;

  df = op->GetAnimPtrF(0);
  kenv->Game->PlayerStartPos.Init(df[29],df[30],df[31]);

  if(op->GetInput(0))
    return op->GetInput(0)->Cache;
  else
    return new GenScene;
}

class KObject * __stdcall Init_KKrieger_Events(KOp *op)
{
  if(op->GetInput(0))
    return op->GetInput(0)->Cache;
  else
    return new GenScene;
}

/****************************************************************************/

struct MonsterMem : public KInstanceMem
{
  KKriegerMonster Monster;
};

void __stdcall Exec_KKrieger_Monster(KOp *op,KEnvironment *kenv)
{
  KKriegerMonster *mon;
  sF32 *fp;
  sInt *ip;
  //sMatrix oldmat;
  sMatrix mat;
  sVector scale,delta;
//  KKriegerCellAdd *cell;
  sInt i;

  MonsterMem *mem;

  mem = kenv->GetInst<MonsterMem>(op);
  mon = &mem->Monster;
  if(mem->Reset)
  {
    sSetMem(mon,0,sizeof(KKriegerMonster));
    mem->DeleteChild = &mon->Event.FirstInstance;
    mem->DeleteChild2 = &mon->DeathEvent.FirstInstance;
  }
  /*
  if(mon->Life==-1)
  {
    mon->Event.FirstInstance->DeleteChain();
    mon->Event.FirstInstance = 0;
  }
  */

  if(mem->Reset || mon->State==KMS_RESPAWN)
  {
    fp = op->GetAnimPtrF(0);
    ip = op->GetAnimPtrS(0);

    mon->Event.Init();
    sCopyMem(&mon->Speed          ,fp+ 6,16*4);

    mon->Speed = mon->Speed/1000;
    mon->WalkStyle = ip[26];
    mon->WeaponKind = ip[27];
    mon->Spline = op->GetSpline(0);
    mon->Life = mon->LifeMax;
    mon->State = KMS_IDLE;
    mon->SpawnSwitch = ip[28];
    mon->Event.Op = 0;
    mon->Event.Monster = mon;
    mon->DeathEvent.Init();
    mon->PosOffset.Init4(fp[23],fp[24],fp[25],0);

    mon->Matrix = kenv->ExecStack.Top();
    delta.Sub3(mon->Matrix.l,kenv->Game->Player.Collider.Pos);
    delta.y = 0;
    delta.Unit3();
    mon->Matrix.i.Init(-delta.z,0,delta.x,0);     // make monster look to player
    mon->Matrix.j.Init(0,1,0,0);
    mon->Matrix.k.Init(-delta.x,0,-delta.z,0);

    mon->Collider.Pos = mon->Matrix.l;
    mon->Collider.Cell = kenv->Game->FindCell(mon->Collider.Pos);
    mon->Collider.Radius = 0.25f;
    mon->Collider.OldPos = mon->Collider.Pos;
    if(mon->Collider.Cell==0)
      mon->Life = 0;

    scale.Init(fp[20],fp[21],fp[22]);
    mat.Init();
    mat.l.Add3(mon->PosOffset);
    mon->HitCell.Init(mat,scale,KCM_ZONE);
    mat = mon->Matrix;
    mon->HitCell.PrepareDynamic(mat);
    mon->HitCell.Monster = mon;
    mon->WalkMem.Reset = 1;

    mon->Matrix.l = mon->Collider.Pos;
    KKMonsterWalk(kenv,&mon->WalkMem,&KKMonsterWalkInfoTable[mon->WalkStyle],mon->Spline,mon->Matrix,mon->Collider.Cell);

    if(kenv->NextMonsterOverride)
    {
      mon->KillSwitch = kenv->NextMonsterKillSwitch;
      mon->SpawnSwitch = kenv->NextMonsterSpawnSwitch;
      mon->Flags |= kenv->NextMonsterFlags;
      kenv->NextMonsterOverride = 0;
    }
  }

  sVERIFY(op->GetInput(0));
  if(mon->Life>0)
  {
    mon->HitCell.Matrix1 = mon->Event.Matrix;
    *kenv->Game->Monsters.Add() = mon;
    if(mon->State>=KMS_INRANGE)
    {
      kenv->Game->AddDCell(&mon->HitCell);
      kenv->ExecStack.PushIdentity();
      for(i=0;i<8;i++)
        kenv->Var[KV_LEG_L0+i] = mon->WalkMem.LegPosOut[i];
      kenv->Var[KV_LEG_TIMES].x = mon->WalkMem.LegTimeOut;
      op->GetInput(0)->ExecEvent(kenv,&mon->Event,1);
      kenv->ExecStack.Pop();
    }
  }
  else
  {
    if(op->GetInput(1))
    {
      kenv->ExecStack.PushIdentity();
      mon->DeathEvent.Matrix = mon->Event.Matrix;
      op->GetInput(1)->ExecEvent(kenv,&mon->DeathEvent,1);
      kenv->ExecStack.Pop();
    }
  }
}

class KObject * __stdcall Init_KKrieger_Monster(KOp *op)
{
  sInt i;

  for(i=0;i<op->GetInputCount();i++)
  {
    sVERIFY(op->GetInput(i));
    sVERIFY(op->GetInput(i)->Cache);
    op->GetInput(i)->Cache->Release();
  }
  return new GenScene();
}

/****************************************************************************/

void __stdcall Exec_KKrieger_MonsterFlags(KOp *op,KEnvironment *kenv,sInt kill,sInt spawn,sInt flags)
{
  kenv->NextMonsterOverride = 1;
  kenv->NextMonsterKillSwitch = kill;
  kenv->NextMonsterSpawnSwitch = spawn;
  kenv->NextMonsterFlags = flags;

  op->ExecInputs(kenv);

  kenv->NextMonsterOverride = 0;
}

class KObject * __stdcall Init_KKrieger_MonsterFlags(KOp *op)
{
  sVERIFY(op->GetInput(0));
  sVERIFY(op->GetInput(0)->Cache);
  return op->GetInput(0)->Cache;
//  op->GetInput(0)->Cache->Release();
//  return new GenScene();
}

/****************************************************************************/

class KObject * __stdcall Init_KKrieger_Respawn(sF323 pos,sInt zone,sF32 dir)
{
  return new GenScene;
}

void __stdcall Exec_KKrieger_Respawn(KOp *op,KEnvironment *kenv,sF323 pos,sInt zone,sF32 dir)
{
  kenv->Game->Respawn[zone].Valid = 1;
  kenv->Game->Respawn[zone].Pos.Init(pos.x,pos.y,pos.z);
  kenv->Game->Respawn[zone].Dir = dir*sPI2F;
}

/****************************************************************************/

class KObject * __stdcall Init_KKrieger_SplashDamage(sInt hits,sF32 range,sF32 enable)
{
  return new GenScene;
}

struct SplashDamageMem : public KInstanceMem
{
  sInt old;
};

void __stdcall Exec_KKrieger_SplashDamage(KOp *op,KEnvironment *kenv,sInt hits,sF32 range,sF32 enable)
{
  SplashDamageMem *mem;
  sBool ok;

  mem = kenv->GetInst<SplashDamageMem>(op);
  
  ok = (enable>0);
  if(ok && !mem->old)
  {
    kenv->Game->SplashEnable = 1;
    kenv->Game->SplashPos = kenv->ExecStack.Top().l;
    kenv->Game->SplashRange = range;
    kenv->Game->SplashHits = hits;
  }
  mem->old = ok;
}

/****************************************************************************/
/***                                                                      ***/
/***   Helpers                                                            ***/
/***                                                                      ***/
/****************************************************************************/

static sBool sMakePlane(sVector &p,const sVector &v0,const sVector &v1,const sVector &v2)
{
  sVector d0,d1,v;
  sF64 e;

  d0.Sub3(v0,v1);
  d1.Sub3(v1,v2);
  v.Cross3(d0,d1);

  if(v.Dot3(v)<1e-10) return sFALSE;
  
  e = sFSqrt(v.Dot3(v));                    // we need some really normal normals here
  v.x = v.x/e;
  v.y = v.y/e;
  v.z = v.z/e;
  v.w = v.Dot3(v1);

  p = v;
  return sTRUE;
}

void KKriegerCell::Init(const sMatrix &mat,const sVector &scale,sInt mode)
{
  sVector va[8];
  sInt i;

  for(i=0;i<8;i++)
  {
    va[i].x = scale.x * -(((i>>0)&1)-0.5f);
    va[i].y = scale.y * -(((i>>1)&1)-0.5f);
    va[i].z = scale.z * -(((i>>2)&1)-0.5f);
    va[i].w = 1;
    va[i].Rotate34(mat);
  }
  Init(va,mode);
}

void KKriegerCell::Init(const sVector *va,sInt mode)
{
  sInt k;
  sF32 d;
  sVector p;
  sBool ok;
  static sInt order[6][4] = 
  {
    { 0,2,3,1 },
    { 2,6,7,3 },
    { 3,7,5,1 },
    { 4,0,1,5 },
    { 2,0,4,6 },
    { 7,6,4,5 },
  };

  sSetMem(this,0,sizeof(*this));

  for(k=0;k<8;k++)
    Vertices[k] = va[k];

  Mode = mode&3;//(3|8);
  OnlyForShot = (mode&4)?1:0;
//  NotForPlayer = (mode&KCM_NOTFORPLAYER)?1:0;

  for(k=0;k<6;k++)
  {
    ok = sFALSE;
    if(!sMakePlane(p,va[order[k][0]],va[order[k][1]],va[order[k][2]]))
    {
      if(!sMakePlane(p,va[order[k][1]],va[order[k][2]],va[order[k][3]]))
        if(!sMakePlane(p,va[order[k][2]],va[order[k][3]],va[order[k][0]]))
          if(!sMakePlane(p,va[order[k][3]],va[order[k][0]],va[order[k][1]]))
            ; // no way finding even a single plane, no        
          else
            ok = sTRUE; // it's just a triange
        else
          ok = sTRUE; // it's just a triange
      else
        ok = sTRUE; // it's just a triange
    }
    else  // it's a quad. lets check if it's a plane one!
    {
      ok = sTRUE;
      d = p.Dot3(va[order[k][3]])-p.w;
      if(d<-0.005f)    // it's a bad one, and even inconvex
      {
        sMakePlane(p,va[order[k][1]],va[order[k][2]],va[order[k][3]]);
        sVERIFY(PlaneCount<=KKRIEGER_MAXPLANES);
        Planes[PlaneCount++] = p;
        sMakePlane(p,va[order[k][3]],va[order[k][0]],va[order[k][1]]);
      }
      else if(d>0.005f)   // it's a bad one, but convex
      {
        sVERIFY(PlaneCount<=KKRIEGER_MAXPLANES);
        Planes[PlaneCount++] = p;
        sMakePlane(p,va[order[k][2]],va[order[k][3]],va[order[k][0]]);
      }
    }
    if(ok)
    {
      sVERIFY(PlaneCount<=KKRIEGER_MAXPLANES);
      Planes[PlaneCount++] = p;
    }
  }

  for(k=0;k<PlaneCount;k++)
  {
    Planes[k].w = -Planes[k].w+OVERLAP;
    if((Mode&3)==KCM_SUB)         // extrude subs twice, so that subs directly on add's still overlap
      Planes[k].w += OVERLAP;
  }
  Center = BBMin = BBMax = va[0];
  for(k=1;k<8;k++)
  {
    Center.Add3(va[k]);
    BBMin.x = sMin(BBMin.x,va[k].x);
    BBMin.y = sMin(BBMin.y,va[k].y);
    BBMin.z = sMin(BBMin.z,va[k].z);
    BBMax.x = sMax(BBMax.x,va[k].x);
    BBMax.y = sMax(BBMax.y,va[k].y);
    BBMax.z = sMax(BBMax.z,va[k].z);
  }
  BBMin.x -= OVERLAP;
  BBMin.y -= OVERLAP;
  BBMin.z -= OVERLAP;
  BBMax.x += OVERLAP;
  BBMax.y += OVERLAP;
  BBMax.z += OVERLAP;
  Center.Scale3(1.0f/8);
  Center.w = 1;
}

/****************************************************************************/

void KKriegerCell::DoRespawn()
{
  Respawn = 0;
  if(Event[1])
  {
    if(Event[1]->FirstInstance)
      Event[1]->FirstInstance->DeleteChain();
    Event[1]->FirstInstance = 0;
  }
}

/****************************************************************************/

void KKriegerCellDynamic::PrepareDynamic(sMatrix &mat)
{
  sInt i;
  for(i=0;i<PlaneCount;i++)
    OrigPlanes[i] = Planes[i];

  for(i = 0; i < 8; i++)
    OrigVertices[i] = Vertices[i];

  Mode |= KCM_UNCONFIRMED;
  Blocked = 0;
  ConfirmedMatrix = mat;
  CurrentMatrix = mat;
  Matrix0 = mat;
  Matrix1 = mat;
  Monster = 0;
}

void KKriegerCellDynamic::ApplyMatrix(sMatrix &mat)
{
  sInt i;
  for(i=0;i<PlaneCount;i++)
  {
    Planes[i].RotatePl(mat,OrigPlanes[i]);
    Planes[i].Scale4(1.0f / Planes[i].Abs3());
  }

  for(i = 0; i < 8; i++)
  {
    Vertices[i].Rotate3(mat, OrigVertices[i]);
    Vertices[i].Add3(mat.l);
  }

  Center = mat.l;
  CurrentMatrix = mat;
  Blocked = 0;
}

/****************************************************************************/

