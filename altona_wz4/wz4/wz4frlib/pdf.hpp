/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#ifndef FILE_INTEL09LIB_PDF_HPP
#define FILE_INTEL09LIB_PDF_HPP

/****************************************************************************/

#include "adf.hpp"

/****************************************************************************/

class Wz4PDFObj : public wObject
{         
  protected:
    static sF32 ND; //Normal Distance
  public:
    Wz4PDFObj();
    virtual ~Wz4PDFObj();

    // Returns true if p is inside the volume
    virtual sBool IsIn(const sVector31 &p);

    // Returns distance to p (<0 means inside)
    virtual sF32 GetDistance(const sVector31 &p) = 0;

    // Returns normal 
    virtual void GetNormal(const sVector31 &p, sVector30 &n);
};

/****************************************************************************/

class Wz4PDF : public wObject
{       
  protected:
    Wz4PDFObj *Obj;
    
  public:

    Wz4PDF();
    virtual ~Wz4PDF();
    
    inline Wz4PDFObj *GetObj()
    {
      return Obj;
    }

    inline void SetObj(Wz4PDFObj *obj)
    {
      Obj=obj;
    }
    
    //  
    sBool TraceRay(sVector31 &p, sVector30 &n, const sRay &ray, const sF32 md=0.005f, const sF32 mx=100.0f, const sInt mi=512);

    static sMatrix34 Camera;
};

/****************************************************************************/

class Wz4PDFAdd : public Wz4PDFObj
{   
  protected:
    sArray < Wz4PDFObj *> Array;
  public:
    Wz4PDFAdd();    
    virtual ~Wz4PDFAdd();    
    virtual sF32 GetDistance(const sVector31 &p);
    virtual void GetNormal(const sVector31 &p, sVector30 &n);
    void AddObj(Wz4PDFObj *obj);    
};

/****************************************************************************/

class Wz4PDFTransform : public Wz4PDFObj
{       
  public:
    Wz4PDFObj  *Obj;
    sVector31   Scale;
    sVector30   Rot;
    sVector31   Trans;    
    sMatrix34   Mat;

    Wz4PDFTransform();    
    virtual ~Wz4PDFTransform();
    void Init(Wz4PDFObj *obj, sVector31 &scale, sVector30 &rot, sVector31 &trans);
    virtual sF32 GetDistance(const sVector31 &p);
    virtual void GetNormal(const sVector31 &p, sVector30 &n);

    inline void Modify(const sVector31 &p, sVector31 &np)
    {
      np=p*Mat;
    }
};

/****************************************************************************/

class Wz4PDFCube : public Wz4PDFObj
{   
  public:    
    Wz4PDFCube();
    virtual ~Wz4PDFCube();
    virtual sF32 GetDistance(const sVector31 &p);    
};

/****************************************************************************/

class Wz4PDFSphere : public Wz4PDFObj
{       
  public:
  Wz4PDFSphere();
  virtual ~Wz4PDFSphere();  
  virtual sF32 GetDistance(const sVector31 &p);
};


/****************************************************************************/

class Wz4PDFTwirl : public Wz4PDFObj
{       
  public:
    Wz4PDFObj  *Obj;
    sVector30   Scale;    
    sVector30   Bias;    
    
    Wz4PDFTwirl();    
    virtual ~Wz4PDFTwirl();
    void Init(Wz4PDFObj *obj, sVector30 &scale, sVector30 &bias);
    virtual sF32 GetDistance(const sVector31 &p);
    inline void Modify(const sVector31 &p, sVector31 &np)
    {
      sMatrix34 m;
      sF32 x=p.x * Scale.x + Bias.x;
      sF32 y=p.y * Scale.y + Bias.y;
      sF32 z=p.z * Scale.z + Bias.z;
      m.EulerXYZ(x,y,z);
      np=p*m;
    }
};

/****************************************************************************/

class Wz4PDFFromADF: public Wz4PDFObj
{       
  private:
  Wz4ADF *ADF;
  public:
  Wz4PDFFromADF(Wz4ADF *adf);
  virtual ~Wz4PDFFromADF();  
  virtual sF32 GetDistance(const sVector31 &p);
  virtual void GetNormal(const sVector31 &p, sVector30 &n);
};


/****************************************************************************/

class Wz4PDFMerge: public Wz4PDFObj
{         
  public:
  Wz4PDFObj  *Obj1;
  Wz4PDFObj  *Obj2;
  int         Type;
  sF32        Factor;

  Wz4PDFMerge();
  virtual ~Wz4PDFMerge();  
  void Init(Wz4PDFObj *obj1, Wz4PDFObj *obj2, sInt type, sF32 factor);
  virtual sF32 GetDistance(const sVector31 &p);
};

void Wz4PDFObj_Render(sImage *img, Wz4PDF *pdf, wPaintInfo &pi);

/****************************************************************************/

#endif // FILE_INTEL09LIB_PDF_HPP

