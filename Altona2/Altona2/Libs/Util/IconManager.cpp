/****************************************************************************/
/***                                                                      ***/
/***   (C) 2012-2014 Dierk Ohlerich et al., all rights reserved.          ***/
/***                                                                      ***/
/***   Released under BSD 2 clause license, see LICENSE.TXT               ***/
/***                                                                      ***/
/****************************************************************************/

#include "Altona2/Libs/Base/Base.hpp"
#include "IconManager.hpp"

using namespace Altona2;

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/

sIconManager::sIconManager()
{
    Loaded = 0;
    Atlas = 0;
}

sIconManager::~sIconManager()
{
    delete Atlas;
}

bool sIconManager::Load(const char *path)
{
    sArray<sDirEntry> Dir;
    sString<sMaxPath> file;
    sASSERT(!Loaded);

    if(!sLoadDir(Dir,path))
        return false;

    // load all images

    int maxx = 32;
    for(const auto &de : Dir)
    {
        if(de.Name[0]=='!')
            continue;

        file.PrintF("%s/%s",path,de.Name);

        sIconPrivate icon;
        icon.Image = new sImage();
        if(icon.Image->Load(file))
        {
            icon.Name = de.Name;
            sRemoveLastSuffix(icon.Name);

            icon.SizeX = icon.Image->GetSizeX();
            icon.SizeY = icon.Image->GetSizeY();
            Icons.Add(icon);

            maxx = sMax(maxx,icon.SizeX);
        }
        else
        {
            sDelete(icon.Image);
        }
    }

    // sort

    for(int i=0;i<Icons.GetCount()-1;i++)
        for(int j=i+1;j<Icons.GetCount();j++)
            if(Icons[i].SizeY < Icons[j].SizeY)
                Icons.Swap(i,j);

    // puzzle

    int sx = (1<<sFindHigherPower(maxx))/2;
    int y1 = 0;
    do
    {
        sx *= 2;
        int x0 = sx*2;
        int y0 = 0;
        y1 = 0;

        for(auto &icon : Icons)
        {
            if(x0+icon.SizeX >= sx)
            {
                y0 = y1;
                y1 = y0 + icon.SizeY;
                x0 = 0;
            }
            icon.Uv.Set(x0,y0,x0+icon.SizeX,y1);
            x0 += icon.SizeX;
        }
    }
    while(y1>sx);

    // copy

    Atlas = new sImage(sx,1<<sFindHigherPower(y1));
    for(auto &icon : Icons)
    {
        uint *s = icon.Image->GetData();
        uint *d = Atlas->GetData();
        int sp = icon.Image->GetSizeX();
        int dp = Atlas->GetSizeX();
        d += icon.Uv.x0 + icon.Uv.y0*dp;

        for(int y=0;y<icon.Uv.SizeY();y++)
            for(int x=0;x<icon.Uv.SizeX();x++)
                d[y*dp+x] = s[y*sp+x];
    }

    // debug save

    file.PrintF("%s/!atlas.png",path);
    Atlas->Save(file);

    // cleanup

    for(auto &icon : Icons)
        sDelete(icon.Image);

    Loaded = true;
    return true;
}

sPainterImage *sIconManager::GetAtlas(sAdapter *ada)
{
    if(!Loaded)
        return 0;
    return new sPainterImage(ada,Atlas,0);
}

bool sIconManager::GetIcon(const char *label,sIcon &icon)
{
    for(auto &i : Icons)
    {
        if(sCmpStringI(label,i.Name)==0)
        {
            icon.Uv = i.Uv;
            icon.Loaded = true;
            return true;
        }
    }

    return false;
}

bool sIconManager::GetIcon(const char *label,sIcon &icon,const char *&tooltip)
{
    int dummy0 = 0;
    return GetIcon(label,(int)sGetStringLen(label),icon,tooltip,dummy0);
}

bool sIconManager::GetIcon(const char *label,int labellen,sIcon &icon,const char *&tooltip,int &tooltiplen)
{
    if(!label || label[0]!='#')
        return false;

    sString<256> buffer; buffer.Init(label+1,labellen-1);

    const char *tt = 0;
    int i = 0;

    while(buffer[i]!=0 && buffer[i]!=' ')
        i++;
    if(buffer[i]==' ')
    {
        tt = label+2+i;
        buffer[i] = 0;
    }
    if(GetIcon(buffer,icon))
    {
        if(tt)
        {
            tooltip = tt;
            tooltiplen = labellen-2-i;
        }
        return true;
    }
    return false;
}

/****************************************************************************/

