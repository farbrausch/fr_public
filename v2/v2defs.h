#pragma once

namespace V2
{
  const int nPrograms=128;
  const int nChannels=16;

  enum CtlTypes { VCTL_SKIP, VCTL_SLIDER, VCTL_MB, };

  typedef struct 
  {
	  int		no;
	  const char *name;
	  const char *name2;
  } Topic;

  typedef struct 
  {
	  int   version;
	  const char  *name;
	  CtlTypes ctltype;
	  int	  offset, min, max;
	  int   isdest;
	  const char  *ctlstr;
  } Param;

  extern const Topic Topics[];
  extern const int nTopics;

  extern const char *ModSources[];
  extern const int nModSources;

  extern const Param Params[];
  extern const int nParams;

  extern const int SoundSize;

  extern const Topic GTopics[];
  extern const int nGTopics;

  extern const Param GParams[];
  extern const int nGParams;

  extern const unsigned char InitSound[];
  extern const unsigned char InitGlobals[];

  extern const int SoundMemSize;
  
  extern int *TopicMap;
  extern int *GTopicMap;

  extern int Version;
  extern int *SoundSizes;
  extern int *GlobalSizes;

  void InitDefs();
  void ExitDefs();

};
