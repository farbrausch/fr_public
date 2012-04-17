// This code is in the public domain. See LICENSE for details.

// TG2Config.cpp: implementation of the CTG2Config class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TG2Config.h"

extern LPCTSTR  lpcstrTG2RegKey;

CTG2Config      *g_config=0;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTG2Config::CTG2Config()
{
  CRegKey key;
  sU32    cpLen=MAX_PATH, bpLen=MAX_PATH;

  m_contentPath[0]=0;
  m_backupPath[0]=0;
  m_slopeKeyDelay=250;
  
  m_fwdKey='W';
  m_leftKey='A';
  m_backKey='S';
  m_rgtKey='D';
  m_upKey='R';
  m_downKey='F';

  if (key.Open(HKEY_CURRENT_USER, lpcstrTG2RegKey, KEY_READ) == ERROR_SUCCESS)
  {
    key.QueryStringValue(_T("ContentPath"), m_contentPath, &cpLen);
    key.QueryStringValue(_T("BackupPath"), m_backupPath, &bpLen);
    key.QueryDWORDValue(_T("SlopeKeyDelay"), m_slopeKeyDelay);
    key.Close();
  }
}

CTG2Config::~CTG2Config()
{
  CRegKey key;

  key.Create(HKEY_CURRENT_USER, lpcstrTG2RegKey);
  key.SetStringValue(_T("ContentPath"), m_contentPath);
  key.SetStringValue(_T("BackupPath"), m_backupPath);
  key.SetDWORDValue(_T("SlopeKeyDelay"), m_slopeKeyDelay);
  key.Close();
}
