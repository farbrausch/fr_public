/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include "wz4_mesh.hpp"
#include "wz4_mtrl2_ops.hpp"
#include "util/scanner.hpp"
#include "wz4lib/basic_ops.hpp"

/****************************************************************************/

// TODO: support polygons with more than 4 vertices as soon as somebody writes
// a proper triangulator for non-convex polys

sBool Wz4Mesh::LoadOBJ(const sChar *filename)
{
  sArray<sVector31> positions;
  sArray<sVector30> normals;
  sArray<sVector30> uvs;

  sScanner scan;
  scan.Init();
  scan.Flags = sSF_NUMBERCOMMENT|sSF_NEWLINE;
  scan.AddTokens(L"/-.\\_");
  scan.StartFile(filename);

  sInt basecluster = Clusters.GetCount();
  sInt curcluster = basecluster-1;

  while (!scan.Errors && scan.Token!=sTOK_END)
  {
    if (scan.IfName(L"mtllib")) // material lib
    {
      scan.Scan(); // skip next token (material name)
    }
    else if (scan.IfName(L"o")) // object
    {
      scan.Scan(); // skip next token (object name)
    }
    else if (scan.IfName(L"g")) // group
    {
      scan.Scan(); // skip name
      curcluster++;
      AddDefaultCluster();
      Clusters.GetTail()->LocalRenderPass=curcluster-basecluster;
    }
    else if (scan.IfName(L"usemtl")) // material ref
    {
      scan.Scan(); // skip next token (material name)
    }
    else if (scan.IfName(L"s")) // smooth
    {
      scan.Scan(); // skip next token (smooth on/off)
    }
    else if (scan.IfName(L"v")) // vertex pos
    {
      sVector31 pos;
      pos.x=scan.ScanFloat();
      pos.y=scan.ScanFloat();
      pos.z=scan.ScanFloat();
      positions.AddTail(pos);
    }
    else if (scan.IfName(L"vt")) // vertex uv
    {
      sVector30 uv;
      uv.x=scan.ScanFloat();
      uv.y=scan.ScanFloat();
      if (scan.Token==sTOK_INT || scan.Token==sTOK_FLOAT || scan.Token=='-')
        uv.z=scan.ScanFloat();
      else
        uv.z=0;
      uvs.AddTail(uv);
    }
    else if (scan.IfName(L"vn")) // vertex normal
    {
      sVector30 n;
      n.x=scan.ScanFloat();
      n.y=scan.ScanFloat();
      n.z=scan.ScanFloat();
      normals.AddTail(n);
    }
    else if (scan.IfName(L"f")) // face
    {
      sInt verts[4];
      sInt vcount=0;
      while ((scan.Token==sTOK_INT || scan.Token=='-') && vcount<4)
      {
        sInt pindex = scan.IfToken('-') ? positions.GetCount() - scan.ScanInt() : scan.ScanInt() - 1;
        sInt tindex=-1;
        sInt nindex=-1;
          
        if (pindex<0 || pindex>=positions.GetCount())
        {
          scan.Error(L"invalid vtx index");
          break;
        }
        if (scan.IfToken(L'/'))
        {
          if (scan.Token==sTOK_INT || scan.Token=='-')
          {
            tindex = scan.IfToken('-') ? uvs.GetCount() - scan.ScanInt() : scan.ScanInt() - 1;
            if (tindex<0 || tindex>=uvs.GetCount())
            {
              scan.Error(L"invalid texcoord index");
              break;
            }
          }
          if (scan.IfToken(L'/'))
          {
            nindex = scan.IfToken('-') ? normals.GetCount() - scan.ScanInt() : scan.ScanInt() - 1;
            if (nindex<0 || nindex>=normals.GetCount())
            {
              scan.Error(L"invalid normal index");
              break;
            }
          }
        }

        // create vertex
        Wz4MeshVertex v;
        v.Zero();
        v.Pos=positions[pindex];
        if (nindex>=0) 
          v.Normal = normals[nindex];
        if (tindex>=0)
        {
          v.U0 = uvs[tindex].x;
          v.V0 = uvs[tindex].y;
        }
        verts[vcount++]=Vertices.GetCount();
        Vertices.AddTail(v);
      }

      if (scan.Token==sTOK_INT)
      {
        scan.Error(L"too many vertices per face (max 4)");
        break;
      }
      
      // create face
      if (!scan.Errors && vcount>2)
      {
        // add default cluster if there's no group yet
        if (curcluster<basecluster)
        {
          curcluster++;
          AddDefaultCluster();
        }

        Wz4MeshFace f;
        f.Init(vcount);
        f.Cluster = curcluster;
        for (sInt i=0; i<vcount; i++)
          f.Vertex[i]=verts[i];
        Faces.AddTail(f);       
      }
    }

    // consume rest of line (who knows)
    while (!scan.Errors && scan.Token!=sTOK_END && scan.Token!=sTOK_NEWLINE)
      scan.Scan();
    scan.IfToken(sTOK_NEWLINE);
  }

  if (!scan.Errors)
  {
    RemoveDegenerateFaces();
    MergeVertices();
    //MergeClusters(); // merges down to one if no mtrl set -> bad idea.
    if (normals.IsEmpty()) CalcNormals();
    CalcTangents();
  }

  return !scan.Errors;
}

sBool Wz4Mesh::SaveOBJ(const sChar *file)
{
  sInt i;
  sTextFileWriter tw(file);
  //tw.Print(L"test%d",10);
  
  tw.PrintF(L"#WZ4 Export\n\n");
    
  //Vertices
  tw.PrintF(L"#begin %d vertices\n",Vertices.GetCount());
  for (i=0;i<Vertices.GetCount();i++)
  {
    tw.PrintF(L"v %f %f %f\n",Vertices[i].Pos.x,Vertices[i].Pos.y,Vertices[i].Pos.z);
  }
  tw.PrintF(L"#end %d vertices\n\n",Vertices.GetCount());
  

  //Normals
  tw.PrintF(L"#begin %d normals\n",Vertices.GetCount());
  for (i=0;i<Vertices.GetCount();i++)
  {
    tw.PrintF(L"vn %f %f %f\n",Vertices[i].Normal.x,Vertices[i].Normal.y,Vertices[i].Normal.z);
  }
  tw.PrintF(L"#end %d normals\n\n",Vertices.GetCount());

  //UV
  tw.PrintF(L"#begin texture vertices\n");
  for (i=0;i<Vertices.GetCount();i++)
  {
    tw.PrintF(L"vt %f %f 0.00000\n",Vertices[i].U0,Vertices[i].V0);
  }
  tw.PrintF(L"#end texture vertices\n\n");

  //faces  
  tw.PrintF(L"#begin %d faces\n",Faces.GetCount());
  for (i=0;i<Faces.GetCount();i++)
  {
    tw.Print(L"f");
    for (int j=0;j<Faces[i].Count;j++)
    {
      sInt t=Faces[i].Vertex[j]+1;
      tw.PrintF(L" %d/%d/%d",t,t,t);
    }
    tw.Print(L"\n");
  }
  tw.PrintF(L"#end %d faces\n\n",Faces.GetCount());
  
  return true;
}

/****************************************************************************/

