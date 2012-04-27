/****************************************************************************/

#ifndef FILE_WZ4LIB_WIKI_HPP
#define FILE_WZ4LIB_WIKI_HPP

#include "base/types2.hpp"
#include "wz4lib/doc.hpp"

/****************************************************************************/

class WikiHelper : public sObject
{
  class sWireTextWindow *WriteWin;
  class sLayoutWindow *ReadWin;
  sTextBuffer Text;
  class Markup *Parser;

  sString<512> Filename;
  sBool Commit;
  sBool Update;
  sBool Inititalize;
  sBool UpdateIndexWhenChanged;
  sTextBuffer Original;
  sArray<sPoolString> History;

  sInt ClickViewHasHit;

  void UpdateIndex();
  void UpdateWindows();
  void DragClickView(const sWindowDrag &dd);
public:
  WikiHelper();
  ~WikiHelper();
  void Tag();
  void InitWire(const sChar *);

  void CmdWikiChange();
  void CmdCursorChange();
  void CmdIndex();
  void CmdDefault();
  void CmdBack();
  void CmdLink();
  void CmdUndo();

  void SetOp(wOp *);
  void Close();
  void Load(const sChar *page,const sChar *def=0);
  void FakeScript(const sChar *code,sTextBuffer *output);


  void SetCommit() { Commit = 1; }
  void InitSvn();
  void Execute(const sChar *format,const sChar *a=L"",const sChar *b=L"");

  const sChar *WikiPath;
  const sChar *WikiCheckout;
  wOp *Op;
};

/****************************************************************************/

#endif // FILE_WZ4LIB_WIKI_HPP

