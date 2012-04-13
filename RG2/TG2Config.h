// This code is in the public domain. See LICENSE for details.

// TG2Config.h: interface for the CTG2Config class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TG2CONFIG_H__AE153F5A_A2BD_48AB_AB9E_7F2137453B0D__INCLUDED_)
#define AFX_TG2CONFIG_H__AE153F5A_A2BD_48AB_AB9E_7F2137453B0D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CTG2Config  
{
  TCHAR     m_contentPath[MAX_PATH];
  TCHAR     m_backupPath[MAX_PATH];
  sU32      m_slopeKeyDelay;
  
  sInt      m_fwdKey, m_backKey, m_leftKey, m_rgtKey, m_upKey, m_downKey;

public:
	CTG2Config();
	virtual ~CTG2Config();

  const TCHAR   *getContentPath() const       { return m_contentPath; }
  const TCHAR   *getBackupPath() const        { return m_backupPath; }
  sU32          getSlopeKeyDelay() const      { return m_slopeKeyDelay; }
  sInt          getForwardKey() const         { return m_fwdKey; }
  sInt          getBackwardKey() const        { return m_backKey; }
  sInt          getLeftKey() const            { return m_leftKey; }
  sInt          getRightKey() const           { return m_rgtKey; }
  sInt          getUpKey() const              { return m_upKey; }
  sInt          getDownKey() const            { return m_downKey; }
  
  void          setContentPath(LPCTSTR path)  { lstrcpy(m_contentPath, path); }
  void          setBackupPath(LPCTSTR path)   { lstrcpy(m_backupPath, path); }
  void          setSlopeKeyDelay(sU32 delay)  { m_slopeKeyDelay=delay; }
  void          setForwardKey(sInt key)       { m_fwdKey=key; }
  void          setBackwardKey(sInt key)      { m_backKey=key; }
  void          setLeftKey(sInt key)          { m_leftKey=key; }
  void          setRightKey(sInt key)         { m_rgtKey=key; }
  void          setUpKey(sInt key)            { m_upKey=key; }
  void          setDownKey(sInt key)          { m_downKey=key; }
};

extern CTG2Config *g_config;

#endif // !defined(AFX_TG2CONFIG_H__AE153F5A_A2BD_48AB_AB9E_7F2137453B0D__INCLUDED_)
