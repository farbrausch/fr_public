// This code is in the public domain. See LICENSE for details.

// GraphWindow.cpp: implementation of the CGraphWindow class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "GraphWindow.h"
#include "frOpGraph.h"
#include "frcontrol.h"
#include "WindowMapper.h"
#include "plugbase.h"
#include "texbase.h"
#include "viewcomm.h"
#include "resource.h"
#include "animbase.h"
#include "tstream.h"
#include <algorithm>

// ---- rulefunktion(tm)

static inline sInt snap(sInt x)
{
  x += x < 0 ? - 13 : 13;
  return x - (x % 25);
}

// ---- operator buttons

class CGraphWindow::opData;

class CGraphWindow::opButton
{
	sU32														m_opID;
  unsigned												m_fMouseOver:1;
  unsigned												m_fDisplayed:1;
  unsigned												m_fEdited;
  unsigned												m_fSelected:1;

	mutable frOpGraph::frOperator*	m_ref;

  friend class CGraphWindow::opData;

  void adjustClipRectangle(LPRECT rc) const
  {
    rc->top += 4;
    rc->bottom -= 4;

		sInt type = getRef()->plg->getButtonType();
    if (type < 0 || type > 4)
      type = 0;

    static sInt leftBorder[5]  = { 4, 15, 4, 4, 4 };
    static sInt rightBorder[5] = { 4, 4, 15, 22, 22 };

    rc->left += leftBorder[type];
    rc->right -= rightBorder[type];
  }

	void updateRef() const
	{
		frOpGraph::opMapIt it = g_graph->m_ops.find(m_opID);

		m_ref = (it != g_graph->m_ops.end()) ? &it->second : 0;
	}

public:
  opButton(sU32 id)
    : m_opID(id), m_fMouseOver(0), m_fDisplayed(0), m_fEdited(0), m_fSelected(0)
  {
		updateRef();
  }

  opButton(const opButton& x)
    : m_opID(x.m_opID), m_fMouseOver(x.m_fMouseOver), m_fDisplayed(x.m_fDisplayed), m_fEdited(x.m_fEdited), m_fSelected(x.m_fSelected)
  {
		updateRef();
  }

  opButton& operator =(const opButton& x)
  {
		m_opID = x.m_opID;
    m_fMouseOver = x.m_fMouseOver;
    m_fDisplayed = x.m_fDisplayed;
    m_fEdited = x.m_fEdited;
    m_fSelected = x.m_fSelected;

		updateRef();

    return *this;
  }

  inline frOpGraph::frOperator* getRef()
  {
		return m_ref;
  }

  inline const frOpGraph::frOperator* getRef() const
  {
		return m_ref;
  }

	inline sU32 getID() const
	{
		return m_opID;
	}

  inline void setDisplay(sBool display)
  {
    m_fDisplayed = !!display;
  }

  inline void setEdit(sBool edit)
  {
    m_fEdited = !!edit;
  }

  inline void setSelect(sBool select)
  {
    m_fSelected = !!select;
  }

  inline sBool getSelect() const
  {
    return m_fSelected;
  }

  inline void setHotLight(sBool state)
  {
    m_fMouseOver = !!state;
  }
};

bool operator == (const sU32 id, const CGraphWindow::opButton& x)
{
	return id == x.getID();
}

bool operator == (const CGraphWindow::opButton& x, const sU32 id)
{
	return x.getID() == id;
}

bool operator < (const sU32 id, const CGraphWindow::opButton& x)
{
	return id < x.getID();
}

bool operator < (const CGraphWindow::opButton& x, const sU32 id)
{
	return x.getID() < id;
}

// ---- saved selection

struct savedSelection
{
	sInt	selSize;
	sU32*	selection;

	savedSelection(sInt size)
		: selSize(size)
	{
		if (selSize)
			selection = new sU32[selSize];
		else
			selection = 0;
	}

	~savedSelection()
	{
		if (selection)
			delete[] selection;
	}
};

// ---- operator handling

class CGraphWindow::opData
{
	struct calcTask
	{
		sU32    opID;
		sBool   display;
		sInt    type;

		calcTask(sU32 id, sBool disp, sInt typ)
			: opID(id), display(disp), type(typ)
		{
		}
	};

	typedef std::list<calcTask>									calcTaskList;
	typedef calcTaskList::iterator							calcTaskIt;
	typedef std::vector<CGraphWindow::opButton>	opButtonList;
	typedef opButtonList::iterator							opButtonListIt;
	typedef opButtonList::const_iterator				opButtonListCIt;
	typedef std::vector<sU32>										selSet;
  typedef selSet::iterator                    selSetIt;
  typedef selSet::const_iterator              selSetCIt;

	opButtonList	m_opButtons;
  
  selSet        m_selection, m_oldSelection;
  selSet        m_drawSet;

  sBool         m_fUpdateEdit, m_fSetDisplay2D, m_fSetDisplay3D;
  sBool         m_fUpdateDisplay2D, m_fUpdateDisplay3D;

	calcTaskList	m_tasks;
	fr::cSection	m_taskLock, m_calcLock;
	fr::event			m_closeCalc, m_closeCalcAck, m_change;

  void updateGUIState()
  {
		if (m_fUpdateEdit)			hlSetEdit(g_graph->m_editOp, sTRUE),						m_fUpdateEdit = sFALSE;
		if (m_fSetDisplay2D)		hlSetDisplay(g_graph->m_viewOp, 0, sTRUE),			m_fSetDisplay2D = sFALSE;
		if (m_fSetDisplay3D)		hlSetDisplay(g_graph->m_view3DOp, 1, sTRUE),		m_fSetDisplay3D = sFALSE;
		if (m_fUpdateDisplay2D)	hlMarkForUpdate(g_graph->m_viewOp, 0, sTRUE),		m_fUpdateDisplay2D = sFALSE;
		if (m_fUpdateDisplay3D)	hlMarkForUpdate(g_graph->m_view3DOp, 1, sTRUE),	m_fUpdateDisplay3D = sFALSE;
  }

	opButtonListIt findOpButton(sU32 id)
	{
		opButtonListIt it = std::lower_bound(m_opButtons.begin(), m_opButtons.end(), id);
		return (it == m_opButtons.end() || it->getID() != id) ? m_opButtons.end() : it;
	}

	opButtonListCIt findOpButton(sU32 id) const
	{
		opButtonListCIt it = std::lower_bound(m_opButtons.begin(), m_opButtons.end(), id);
		return (it == m_opButtons.end() || it->getID() != id) ? m_opButtons.end() : it;
	}

public:
	CGraphWindow*	m_host;

  opData(CGraphWindow* host)
    : m_host(host), m_fUpdateEdit(sFALSE), m_fSetDisplay2D(sFALSE), m_fSetDisplay3D(sFALSE), m_fUpdateDisplay2D(sFALSE),
		m_fUpdateDisplay3D(sFALSE)
  {
  }

  // ---- management

  void setCurrentPage(sU32 pageID)
  {
    m_opButtons.clear();

    for (frOpGraph::opMapIt it = g_graph->m_ops.begin(); it != g_graph->m_ops.end(); ++it)
    {
      if (it->second.pageID == pageID)
			{
#ifdef _DEBUG
				if (m_opButtons.size())
					FRDASSERT(m_opButtons.back().getID() < it->first);
#endif

				m_opButtons.push_back(opButton(it->first));

				if (it->second.opID == g_graph->m_viewOp || it->second.opID == g_graph->m_view3DOp)
					m_opButtons.back().setDisplay(sTRUE);
      }
    }
  }

  void centerScroll()
  {
    sInt cx = 0, cy = 0, div = 0;

		for (opButtonListCIt it = m_opButtons.begin(); it != m_opButtons.end(); ++it)
		{
			const frOpGraph::frOperator* op = it->getRef();

			cx += op->rc.left + op->rc.right;
			cy += op->rc.top + op->rc.bottom;
			div++;
		}

		if (div)
		{
			cx /= div * 2;
			cy /= div * 2;
		}

    RECT rc;
    m_host->GetClientRect(&rc);
    cx -= rc.right/2;
    cy -= rc.bottom/2;

    m_host->SetScrollOffset(cx, cy);
  }

  void centerOnOperator(sU32 id)
  {
		opButtonListCIt it = findOpButton(id);

		if (it != m_opButtons.end())
		{
			const frOpGraph::frOperator* op = it->getRef();

			RECT rc;
			m_host->GetClientRect(&rc);
			m_host->SetScrollOffset((op->rc.left + op->rc.right - rc.right) / 2, (op->rc.top + op->rc.bottom - rc.bottom) / 2);
		}
  }

  // ---- drawing

  void draw(CDCHandle dc, const RECT &client)
  {
    CPoint scrl;
    m_host->GetScrollOffset(scrl);
    dc.SetViewportOrg(CPoint(-scrl.x, -scrl.y));
    
    CRect window=client;
    window.OffsetRect(scrl.x, scrl.y);

    m_drawSet.clear();

    // draw buttons
		for (opButtonListCIt it = m_opButtons.begin(); it != m_opButtons.end(); ++it)
		{
			if (frRectsIntersect(window, it->getRef()->rc))
				m_drawSet.push_back(it->getID());
		}

    drawSelection(dc, m_drawSet);

    // draw warnings/errors (if present)
    COLORREF colWarn = RGB(255, 0, 0);

		for (opButtonListCIt it = m_opButtons.begin(); it != m_opButtons.end(); ++it)
		{
			const frOpGraph::frOperator* op = it->getRef();

      sInt cnt = 0;
      for (sInt i = 0; i < op->def->nInputs; i++)
      {
        if (op->plg->getInID(i))
          cnt++;
      }
        
      cnt = (sInt) op->def->minInputs - cnt;
      
      if (cnt > 0)
      {
        CRect rc = op->rc;
        
        rc.bottom = rc.top + ::GetSystemMetrics(SM_CYEDGE);
        rc.left += 4;
        rc.right -= 4;
        
        dc.FillSolidRect(&rc, colWarn);
      }
      
      if (op->def->hasOutput && !op->nOutputs)
      {
        CRect rc = op->rc;
        
        rc.top = rc.bottom - ::GetSystemMetrics(SM_CYEDGE);
        rc.left += 4;
        rc.right -= 4;
        
        dc.FillSolidRect(&rc, colWarn);
      }
    }
  }
  
  void drawSelection(CDCHandle dc, const selSet& sel)
  {
    CDC tempDC;
    tempDC.CreateCompatibleDC(dc);
    
    CBitmapHandle oldBitmap = tempDC.SelectBitmap(m_host->m_buttonsBitmap);
    CFontHandle oldFont = dc.SelectFont(AtlGetDefaultGuiFont());

    dc.SetTextColor(0);
    dc.SetBkMode(TRANSPARENT);
    dc.SetStretchBltMode(COLORONCOLOR);

		opButtonListIt bit = m_opButtons.begin();
    
    for (selSetCIt it = sel.begin(); it != sel.end(); ++it)
    {
			bit = std::lower_bound(bit, m_opButtons.end(), *it);
			FRDASSERT(bit != m_opButtons.end() && bit->getID() == *it);

      const opButton& btn = *bit;
			const frOpGraph::frOperator* op = btn.getRef();
      const sInt srcx = ((btn.m_fDisplayed << 2) | (btn.m_fMouseOver << 1) | btn.m_fSelected) * 100;
      const sInt srcy = op->plg->getButtonType() * 25;
			CRect rc = op->rc;
      
      dc.BitBlt(rc.left, rc.top, 21, 25, tempDC, srcx, srcy, SRCCOPY);
      dc.StretchBlt(rc.left + 21, rc.top, rc.Width() - 42, 25, tempDC, srcx + 21, srcy, 100 - 21 * 2, 25, SRCCOPY);
      dc.BitBlt(rc.right - 21, rc.top, 21, 25, tempDC, srcx + 99 - 21, srcy, SRCCOPY);

      btn.adjustClipRectangle(&rc);

			const sChar* name = op->plg->getDisplayName();
			const fr::string& opName = op->getName();

      if (opName.getLength())
      {
        sChar tempBuf[259];
        strcpy(tempBuf, "\"");
        strcat(tempBuf, opName);
        strcat(tempBuf, "\"");
        
        name = tempBuf;
      }

      dc.DrawText(name, -1, &rc, DT_CENTER | DT_END_ELLIPSIS | DT_SINGLELINE | DT_VCENTER);
    }

    dc.SelectFont(oldFont);
    tempDC.SelectBitmap(oldBitmap);
  }

  void killButtonsFromRegion(HRGN rgn, const RECT& client)
  {
    CPoint scrl;
    m_host->GetScrollOffset(scrl);

    CRect window = client;
    window.OffsetRect(scrl.x, scrl.y);

		for (opButtonListIt it = m_opButtons.begin(); it != m_opButtons.end(); ++it)
		{
      if (frRectsIntersect(window, it->getRef()->rc))
      {
        CRgn buttonRgn;
        RECT& rc = it->getRef()->rc;

        buttonRgn.CreateRectRgn(rc.left - scrl.x, rc.top - scrl.y, rc.right - scrl.x, rc.bottom - scrl.y);
        ::CombineRgn(rgn, rgn, buttonRgn, RGN_DIFF);
      }
    }
  }

  // ---- operators

  void createButton(frOpGraph::frOperator& op)
  {
		m_opButtons.insert(std::lower_bound(m_opButtons.begin(), m_opButtons.end(), op.opID), opButton(op.opID));

    m_fUpdateEdit   |= op.opID == g_graph->m_editOp;
    m_fSetDisplay2D |= op.opID == g_graph->m_viewOp;
    m_fSetDisplay3D |= op.opID == g_graph->m_view3DOp;

    updateGUIState();
  }

  void destroyButton(frOpGraph::frOperator& op)
  {
		opButtonListIt it = findOpButton(op.opID);
		FRDASSERT(it != m_opButtons.end());

		m_opButtons.erase(it);
  }

  void setDisplay(sU32 id, sBool state)
  {
		opButtonListIt it = findOpButton(id);

    if (it != m_opButtons.end())
    {
      it->setDisplay(state);
      invalidateOpButton(id);
    }
  }

  void setEdit(sU32 id, sBool state)
  {
    opButtonListIt it = findOpButton(id);

    if (it != m_opButtons.end())
      it->setEdit(state);
  }

  void setHotLight(sU32 id, sBool state)
  {
		opButtonListIt it = findOpButton(id);

    if (it != m_opButtons.end())
    {
      it->setHotLight(state);
      invalidateOpButton(id);
    }
  }

  void invalidateOpButton(sU32 id, sBool erase = sFALSE)
  {
    frOpGraph::opMapIt it = g_graph->m_ops.find(id);
    
    if (it != g_graph->m_ops.end())
    {
      CRect   rc = it->second.rc;
      CPoint  scrl;
      
      m_host->GetScrollOffset(scrl);
      rc.OffsetRect(-scrl.x, -scrl.y);
      
      m_host->InvalidateRect(&rc, !!erase);
    }
  }

  void setConnections(frOpGraph::frOperator& op)
  {
    const sBool res = op.setConnections();
    invalidateOpButton(op.opID);

    m_fUpdateEdit |= res;
    m_fUpdateDisplay2D = sTRUE;
    m_fUpdateDisplay3D = sTRUE;
  }

  void linkOpIn(frOpGraph::frOperator& op)
  {
    CRect rc = op.rc;

    // go through all ops and build the connections
    for (frOpGraph::opMapIt it = g_graph->m_ops.begin(); it != g_graph->m_ops.end(); ++it)
    {
      if (it->second.pageID != op.pageID || it->second.rc.left >= rc.right || it->second.rc.right <= rc.left)
        continue; // not on the same page or no intersection on the x axis, skip this!

      if (it->second.rc.top == rc.top - 25) // in input row, add the input
      {
        op.addInput(it->second.rc.left, it->first);
        it->second.addOutput(op.opID);
        invalidateOpButton(it->first);
      }
      else if (it->second.rc.top == rc.top + 25) // in output row, add the output
      {
        op.addOutput(it->first);
        it->second.addInput(rc.left, op.opID);
        setConnections(it->second);
      }
    }

    // check all link connections for cycles (may be introduced by moves!)
    sBool linksChanged = sFALSE;

    for (sInt i = 0; i < op.nInputs; i++)
    {
      if (op.inputs[i].id & 0x80000000) // there's a link
      {
        const sU32 linkRef = op.inputs[i].id & 0x7fffffff;
        
        if (g_graph->dependsOn(linkRef, op.opID)) // linked op depends on this one, so...
        {
          op.inputs[i].id = 0x80000000; // ...break the cycle.
          linksChanged = sTRUE;
        }
      }
    }

    setConnections(op);

    if (linksChanged && g_graph->m_editOp == op.opID) // update display if we changed some links for this op
      m_fUpdateEdit = sTRUE;
  }

  void deleteConnections(frOpGraph::frOperator& op, sBool deleteLinks = sFALSE)
  {
    // go through the inputs (via connections in the plugin this time!)
    frPlugin* plg = op.plg;

    for (sU32 i = 0; i < (deleteLinks ? plg->getNTotalInputs() : op.def->nInputs); i++)
    {
      const sU32 inID = plg->getInID(i);
      
      if (inID) // there's a connection in that slot, kill it.
      {
        frOpGraph::opMapIt it = g_graph->m_ops.find(inID);

        if (it != g_graph->m_ops.end())
        {
          it->second.delOutput(op.opID);
          invalidateOpButton(it->second.opID);
        }
        else
          fr::debugOut("This shouldn't happen! Invalid input relation found in deleteConnections for op %d\n", op.opID);
      }
    }

    // go through the outputs
    for (sU32 i = 0; i < op.nOutputs; i++)
    {
      sU32 outID = op.outputs[i];
      
      if ((outID & 0x80000000) && !deleteLinks) // skip link outputs if deleteLinks is false.
        continue;

      frOpGraph::opMapIt it = g_graph->m_ops.find(outID & 0x7fffffff);

      if (it != g_graph->m_ops.end())
      {
        if (outID & 0x80000000) // explicitly unset if this is a link connection
        {
          for (sInt i = 0; i < it->second.nInputs; i++)
          {
            if ((it->second.inputs[i].id & 0x7fffffff) == op.opID)
            {
              frParam* param = it->second.plg->getParam(it->second.inputs[i].priority);
              FRDASSERT(param->type == frtpLink);

              param->linkv->opID = 0;
              param->linkv->plg = 0;
              it->second.plg->setParam(it->second.inputs[i].priority, param);
            }
          }
        }

        it->second.delInput(op.opID); // delete the connection
        setConnections(it->second); // and update the plugin connections
      }
      else
        fr::debugOut("This shouldn't happen! Invalid output relation found in deleteConnection for op %d\n", op.opID);
    }

    invalidateOpButton(op.opID);

    op.delInputs(deleteLinks);
    op.delOutputs(deleteLinks);
  }

  void deleteOp(sU32 id)
  {
    removeFromSelection(id);

    frOpGraph::opMapIt it = g_graph->m_ops.find(id);
    FRDASSERT(it != g_graph->m_ops.end());

		if (id == g_graph->m_viewOp)		hlSetDisplay(0, 0);
		if (id == g_graph->m_view3DOp)	hlSetDisplay(0, 1);
		if (id == g_graph->m_editOp)		hlSetEdit(0);

    deleteConnections(it->second, sTRUE); // delete the connections!
    invalidateOpButton(id, sTRUE);

    frOpGraph::pgMapIt pgIt = g_graph->m_pages.find(it->second.pageID);
    pgIt->second.delNamedOp(id); // not every op is named, but it is safe to "delete" a non-named op from the named ops list.

		opButtonListIt oit = findOpButton(id);
    if (oit != m_opButtons.end())
      m_opButtons.erase(oit);
    
    g_graph->m_ops.erase(it);
    g_graph->m_curves->deleteOperatorCurves(id);
    g_graph->m_clips->deleteOperatorClips(id);
    g_graph->modified();

    m_host->SetFocus();
    updateGUIState();
  }

  void deleteCurPage()
  {
    clearSelection();

		for (opButtonListCIt it = m_opButtons.begin(); it != m_opButtons.end(); ++it)
			m_selection.push_back(it->getID());

    deleteSelection();
  }

  // ---- selections

  sBool gotSelection() const
  {
    return m_selection.size() != 0;
  }

  void clearSelection()
  {
		opButtonListIt bit = m_opButtons.begin();

    for (selSetIt it = m_selection.begin(); it != m_selection.end(); ++it)
    {
			bit = std::lower_bound(bit, m_opButtons.end(), *it);

			if (bit != m_opButtons.end() && bit->getID() == *it)
      {
				bit->setSelect(sFALSE);
				invalidateOpButton(*it);
      }
    }

    m_selection.clear();
  }
  
  void selectFromRect(CRect rect, sBool add = sFALSE)
  {
    if (!add)
      clearSelection();
    else
      m_selection = m_oldSelection;

		for (opButtonListIt it = m_opButtons.begin(); it != m_opButtons.end(); ++it)
		{
			const frOpGraph::frOperator* ref = it->getRef();
			const sBool currentState = it->getSelect();
			sBool state = currentState;

			if (frRectsIntersect(rect, ref->rc))
			{
				state = sTRUE;
				if (!add)
					m_selection.push_back(it->getID());
				else
					m_selection.insert(std::lower_bound(m_selection.begin(), m_selection.end(), it->getID()), it->getID());
			}
			else if (!add || !inSelection(ref->opID))
				state = sFALSE;

			if (state != currentState)
			{
				it->setSelect(state);
				invalidateOpButton(it->getID());
			}
		}
  }

  void addToSelection(sU32 id)
  {
    if (!inSelection(id))
    {
			m_selection.insert(std::lower_bound(m_selection.begin(), m_selection.end(), id), id);

			opButtonListIt it = findOpButton(id);
			if (it != m_opButtons.end())
			{
				it->setSelect(sTRUE);
				invalidateOpButton(it->getID());
			}
    }
  }

  void removeFromSelection(sU32 id)
  {
		selSetIt sit = std::lower_bound(m_selection.begin(), m_selection.end(), id);
		if (sit != m_selection.end())
		{
			FRDASSERT(*sit == id);

			m_selection.erase(sit);

			opButtonListIt it = findOpButton(id);
			if (it != m_opButtons.end())
			{
				it->setSelect(sFALSE);
				invalidateOpButton(it->getID());
			}
		}
  }

  sBool inSelection(sU32 id)
  {
		selSetCIt it = std::lower_bound(m_selection.begin(), m_selection.end(), id);
		return (it != m_selection.end() && *it == id);
  }

  void deleteSelection()
  {
		hlSyncCalculations();

		while (m_selection.size())
			deleteOp(m_selection.back());

		hlUpdateEditOp();
    m_host->UpdateWindow();
  }

	sBool checkSelectionCollides() const
	{
		// ...loop through the selection
		for (selSetCIt sit = m_selection.begin(); sit != m_selection.end(); ++sit)
		{
			frOpGraph::opMapIt opit = g_graph->m_ops.find(*sit);
			FRASSERT(opit != g_graph->m_ops.end());

			RECT selRc = opit->second.rc;

			// loop through all ops
			for (opButtonListCIt oit = m_opButtons.begin(); oit != m_opButtons.end(); ++oit)
			{
				if (oit->getID() == *sit)
					continue; // don't check for intersections with self :)

				// if there is an intersection, return true.
				if (frRectsIntersect(selRc, oit->getRef()->rc))
					return sTRUE;
			}
		}
		
		return sFALSE;
	}

	void copyToStream(fr::stream& copyBuf, CPoint* pastePoint = 0)
	{
		// find the top-left point and build the list of external references
		CPoint ptTopLeft;
		std::vector<sU32> externalRefs, linkInputs;

		for (selSetCIt it = m_selection.begin(); it != m_selection.end(); ++it)
		{
			const frOpGraph::frOperator& op = g_graph->m_ops[*it];

			if (it == m_selection.begin())
			{
				ptTopLeft.x = op.rc.left;
				ptTopLeft.y = op.rc.top;
			}
			else
			{
				ptTopLeft.x = min(ptTopLeft.x, op.rc.left);
				ptTopLeft.y = min(ptTopLeft.y, op.rc.top);
			}

			sInt nLinks = 0;

			for (sInt i = 0; i < op.nInputs; i++)
			{
				sU32 inID = op.inputs[i].id;

				if (inID & 0x80000000)
				{
					nLinks++;
					inID &= 0x7fffffff;

					if (inID && !inSelection(inID)) // reference is external
					{
						std::vector<sU32>::iterator it = std::lower_bound(externalRefs.begin(), externalRefs.end(), inID);

						if (it == externalRefs.end() || *it != inID) // not yet in the references list, add it
							externalRefs.insert(it, inID);
					}
				}
			}

			linkInputs.push_back(nLinks);
		}

		// serialize the selection to copy: create the buffer and the header
		copyBuf.putsInt(1); // version
		copyBuf.putsInt(g_graph->m_pages[g_graph->m_curPage].type);
		copyBuf.putsU32(m_selection.size());
		copyBuf.putsU32(externalRefs.size());

		fr::streamWrapper strm(copyBuf, sTRUE);

		// write information about the external references first
		for (sInt i = 0; i < externalRefs.size(); i++)
		{
			// write the operator name
			const frOpGraph::frOperator& op = g_graph->m_ops[externalRefs[i]];
			fr::string temp = op.getName();
			strm << temp;

			// then write the name of the respective page
			const frOpGraph::frOpPage& pg = g_graph->m_pages[op.pageID];
			temp = pg.name;
			strm << temp;
		}

		// then save the actual operator data
		for (sInt i = 0; i < m_selection.size(); i++)
		{
			// the original copy&paste code used frOperator::serialize. for the new data structures, this causes
			// more problems than it solves, so I write custom serialization here.

			frOpGraph::opMapIt ooit = g_graph->m_ops.find(m_selection[i]);
			FRDASSERT(ooit != g_graph->m_ops.end());

			frOpGraph::frOperator& op = ooit->second;

			copyBuf.putsS32(op.def->ID);
			if (op.def->type == 0) // texture? write dimensions too
			{
				frTexturePlugin* tpg = static_cast<frTexturePlugin*>(op.plg);

				copyBuf.putsInt(tpg->targetWidth);
				copyBuf.putsInt(tpg->targetHeight);
			}

			// put location, width and name
			copyBuf.putsInt(op.rc.left - ptTopLeft.x);
			copyBuf.putsInt(op.rc.top - ptTopLeft.y);
			copyBuf.putsInt(op.rc.right - op.rc.left);
			fr::string name = op.getName();
			strm << name;

			// put (link) references
			copyBuf.putsInt(linkInputs[i]);

			for (sInt in = 0; in < op.nInputs; in++)
			{
				sU32 inID = op.inputs[in].id;

				if (inID & 0x80000000)
				{
					inID &= 0x7fffffff;
					sU32 writeID = 0;

					if (inSelection(inID))
						writeID = std::lower_bound(m_selection.begin(), m_selection.end(), inID) - m_selection.begin() + 1;
					else if (inID)
						writeID = std::lower_bound(externalRefs.begin(), externalRefs.end(), inID) - externalRefs.begin() + 1 + m_selection.size();

					copyBuf.putsU32(writeID);
					copyBuf.putsInt(op.inputs[in].priority);
				}
			}

			// put operator-specific data
			op.plg->serialize(strm);
		}

		// update paste point info
		if (pastePoint)
		{
			pastePoint->x = ptTopLeft.x;
			pastePoint->y = ptTopLeft.y;
		}
	}

	void copySelection()
	{
		// prepare clipboard and buffers
		if (!m_selection.size() || !m_host->OpenClipboard())
			return;

		EmptyClipboard();
		fr::streamMTmp copyBuf;
		copyBuf.open();

		// perform actual copy to stream
		copyToStream(copyBuf);

		// we now have the data set, set it as clipboard data.
		HGLOBAL hGlobalCopy = GlobalAlloc(GMEM_MOVEABLE, copyBuf.size());
		if (hGlobalCopy)
		{
			copyBuf.seek(0);
			copyBuf.read(GlobalLock(hGlobalCopy), copyBuf.size());
			GlobalUnlock(hGlobalCopy);
			SetClipboardData(m_host->m_clipboardFormat, hGlobalCopy);
		}

		CloseClipboard();
	}

	void pasteFromStream(fr::stream& clipData, sInt pasteX, sInt pasteY, savedSelection* pasteSel = 0)
	{
		if (!clipData.size())
			return;

		fr::streamWrapper strm(clipData, sFALSE);
		clearSelection();

		CPoint pastePos(pasteX, pasteY);
		sInt ver = clipData.getsInt();
		sInt pageType = clipData.getsInt();

		if (ver == 1 && pageType == g_graph->m_pages[g_graph->m_curPage].type)
		{
			sU32 selSize = clipData.getsU32();
			sU32 extRefs = clipData.getsU32();

			sU32* idRemap = new sU32[1 + selSize + extRefs];
			idRemap[0] = 0; // 0 maps to 0 (big surpise, that)

			// create new operators
			for (sU32 i = 0; i < selSize; i++)
			{
				if (!pasteSel)
					idRemap[i + 1] = g_graph->addOperator().opID;
				else
					idRemap[i + 1] = g_graph->addOperatorWithID(pasteSel->selection[i]).opID;
			}

			// resolve the external links (which is really the biggest of the problems to solve)
			sBool guessedPage = sFALSE;
			sBool guessedOp = sFALSE;
			sBool notFoundOp = sFALSE;

			for (sU32 i = 0; i < extRefs; i++)
			{
				fr::string opName, pageName;

				strm << opName << pageName;

				sInt matchesFound;
				sU32 opID = 0;
				sU32 pageID = 0;
				
				// try to find the page
				matchesFound = 0;

				for (frOpGraph::pgMapIt pit = g_graph->m_pages.begin(); pit != g_graph->m_pages.end(); ++pit)
				{
					if (pit->second.name == pageName) // page name matches
					{
						pageID = matchesFound ? 0 : pit->first;
						matchesFound++;
					}
				}

				// try to find the operator
				matchesFound = 0;

				while (!matchesFound)
				{
					matchesFound = 0;

					for (frOpGraph::opMapIt oit = g_graph->m_ops.begin(); oit != g_graph->m_ops.end(); ++oit)
					{
						if ((!pageID || oit->second.pageID == pageID) && oit->second.getName() == opName) // op matches criteria?
						{
							matchesFound++;
							opID = oit->first;
						}
					}

					if (!matchesFound)
					{
						if (pageID)
							pageID = 0;
						else
							break;
					}
				}

				// set some flags and update the link
				if (!pageID && opID)
					guessedPage = sTRUE;
					
				if (matchesFound > 1)
					guessedOp = sTRUE;

				if (!opID)
					notFoundOp = sTRUE;

				idRemap[selSize + 1 + i] = opID;
			}

			// we're done with the links, now perform the actual paste
			sBool invalidLinks = sFALSE;

			for (sU32 i = 0; i < selSize; i++)
			{
				// again, (as in the copy code) this one has custom serialization, for pretty much the same reasons.
				frOpGraph::frOperator& op = g_graph->m_ops[idRemap[i + 1]];

				op.pageID = g_graph->m_curPage;

				// get type id and create the operator.
				sS32 defID = clipData.getsS32();
				op.def = frGetPluginByID(defID);
				op.plg = op.def->create(op.def);
				
				if (op.def->type == 0) // texture
				{
					sInt w, h;

					w = clipData.getsInt();
					h = clipData.getsInt();

					static_cast<frTexturePlugin*>(op.plg)->resize(w, h);
				}

				// get location and name
				op.rc.left = clipData.getsInt() + pastePos.x;
				op.rc.top = clipData.getsInt() + pastePos.y;
				op.rc.right = op.rc.left + clipData.getsInt();
				op.rc.bottom = op.rc.top + 25;
				fr::string temp;
				strm << temp;
				op.setName(temp);

				// get number of link references and read them
				sInt nLinks = clipData.getsInt();
				for (sInt i = 0; i < nLinks; i++)
				{
					sU32 id = clipData.getsU32();

					if (id < selSize + extRefs + 1)
						id = idRemap[id];
					else
					{
						id = 0;
						invalidLinks = sTRUE;
					}

					op.addInput(clipData.getsInt(), id | 0x80000000);
				}

				if (nLinks)
					op.setConnections();

				// get op-specific data
				op.plg->serialize(strm);

				// the rest of the per-op work
				createButton(op);
				addToSelection(op.opID);
			}

			// clean up and report problems if we had some
			delete[] idRemap;
			updateSelectionConnections();

			if (guessedPage || guessedOp || notFoundOp || invalidLinks)
			{
				sChar buffer[512];
				strcpy(buffer, "There were problems with the paste operation:\n\n");
				
				if (guessedPage)
					strcat(buffer, "- One or more links referred to pages that don't exist and had to be guessed.\n");

				if (guessedOp)
					strcat(buffer, "- One or more links referred to operator names that weren't unique.\n");

				if (notFoundOp)
					strcat(buffer, "- Wasn't able to resolve one or more links.\n");

				if (invalidLinks)
					strcat(buffer, "- Encountered invalid links. (This may have had dramatic effects)\n");

				strcat(buffer, "\nYou should check whether the result is what you expected.");
				m_host->MessageBox(buffer, _T("RG2.1"), MB_ICONERROR | MB_OK);
			}
		}
		else
		{
			if (ver != 1)
				m_host->MessageBox(_T("Can't paste, unknown data format version"), _T("RG2.1"), MB_ICONERROR | MB_OK);
			else
				m_host->MessageBox(_T("Wrong page type of data in clipboard"), _T("RG2.1"), MB_ICONERROR | MB_OK);
		}

		m_host->SetFocus();
	}

	void saveSelection()
  {
    m_oldSelection = m_selection;
  }

  void moveSelection(sInt offX, sInt offY)
  {
    for (selSetCIt it = m_selection.begin(); it != m_selection.end(); ++it)
    {
      frOpGraph::opMapIt i = g_graph->m_ops.find(*it);
      if (i == g_graph->m_ops.end())
        continue;

      invalidateOpButton(*it, sTRUE);
      ::OffsetRect(&i->second.rc, offX, offY);
      invalidateOpButton(*it, sFALSE);

      g_graph->modified();
    }
  }

  void updateSelectionConnections()
  {
    for (selSetCIt it = m_selection.begin(); it != m_selection.end(); ++it)
    {
      frOpGraph::opMapIt i = g_graph->m_ops.find(*it);
      if (i == g_graph->m_ops.end())
        continue;
      
      deleteConnections(i->second);
      linkOpIn(i->second);
    }

    updateGUIState();
  }

  void reconnectAll()
  {
    for (frOpGraph::opMapIt it = g_graph->m_ops.begin(); it != g_graph->m_ops.end(); ++it)
    {
      deleteConnections(it->second);
      linkOpIn(it->second);
      if(it->second.plg)
        it->second.plg->markNew();
    }

    updateGUIState();
  }

	sBool isEqualSelection(const savedSelection* sel)
	{
		if (sel->selSize != m_selection.size())
			return sFALSE;

		for (sInt i = 0; i < m_selection.size(); i++)
		{
			if (sel->selection[i] != m_selection[i])
				return sFALSE;
		}

		return sTRUE;
	}

  // ---- ui support

  sU32 hitTest(CPoint pt)
  {
		for (opButtonListIt it = m_opButtons.begin(); it != m_opButtons.end(); ++it)
		{
			if (::PtInRect(&it->getRef()->rc, pt))
				return it->getID();
		}

    return 0;
  }
  
  void mouseMove(CPoint pt)
  {
    static sU32 oldHotLight = 0;
    sU32 id = hitTest(pt);
    
    if (oldHotLight != id)
    {
      setHotLight(oldHotLight, sFALSE);
      setHotLight(id, sTRUE);
    }
    
    oldHotLight=id;
  }
  
  void doScroll(sInt offX, sInt offY)
  {
    m_host->ScrollWindowEx(-offX, -offY, SW_ERASE | SW_INVALIDATE);
    m_host->UpdateWindow();
  }

	// ---- highlevel functions: display/edit management

	void hlSetDisplay(sU32 id, sInt type, sBool force = sFALSE)
	{
		sBool found = sFALSE;
		frOpGraph::opMapIt it;

		if (type == -1)
		{
			it = g_graph->m_ops.find(id);
			type = (it != g_graph->m_ops.end()) ? it->second.def->type : 0;
		}

		sU32& viewOp = (type == 0) ? g_graph->m_viewOp : g_graph->m_view3DOp;

		if (!force && viewOp == id)
			return;

		if ((it = g_graph->m_ops.find(viewOp)) != g_graph->m_ops.end())
		{
			setDisplay(it->second.opID, sFALSE);
			it->second.active = sFALSE;
		}

		if ((it = g_graph->m_ops.find(id)) != g_graph->m_ops.end())
		{
			hlMarkForUpdate(id, type, sTRUE);
			setDisplay(id, sTRUE);
			it->second.active = sTRUE;
			found = sTRUE;
		}
		else
			hlMarkForUpdate(0, type, sTRUE);

		viewOp = id;
		m_host->UpdateWindow();

		if (!g_graph->m_editOp && found)
			hlSetEdit(id);
	}

	void hlSetEdit(sU32 id, sBool force = sFALSE)
	{
		sBool found = sFALSE;
		frOpGraph::opMapIt it;

		if (g_graph->m_editOp == id && !force)
			return;

		setEdit(g_graph->m_editOp, sFALSE);
		if ((it = g_graph->m_ops.find(id)) != g_graph->m_ops.end())
		{
			setEdit(id, sTRUE);
			found = sTRUE;
		}

		::SendMessage(g_winMap.Get(ID_PARAM_WINDOW), WM_SET_EDIT_PLUGIN, force, found ? id : 0);
		m_host->UpdateWindow();

		g_graph->m_editOp = id;
		if (found)
		{
			if (!g_graph->m_viewOp && it->second.def->type == 0)
				hlSetDisplay(id, 0);
			else if (!g_graph->m_view3DOp && it->second.def->type != 0)
				hlSetDisplay(id, 1);
		}
	}

	// ---- highlevel functions: update handling

	void hlStartUpdateThread()
	{
		m_closeCalc.create();
		m_closeCalcAck.create();
		m_change.create();
		_beginthread(hlUpdateThreadFunc, 0x20000, this);
	}

	void hlStopUpdateThread()
	{
		m_closeCalc.set();
		m_closeCalcAck.wait();
		m_closeCalc.destroy();
		m_closeCalcAck.destroy();
		m_change.destroy();
	}

	void hlMarkForUpdate(sU32 id, sInt type, sBool display)
	{
		calcTaskIt it;

		if (g_graph->m_ops.find(id) == g_graph->m_ops.end()) // op doesn't exist
		{
			id = 0;
			display = sTRUE;
		}

		m_taskLock.enter();
		for (it = m_tasks.begin(); it != m_tasks.end() && it->opID != id; ++it);

		if (it != m_tasks.end()) // task already in task list, update display flag
			it->display |= display; 
		else // add the task
		{
			m_tasks.push_back(calcTask(id, display, type));
			it = m_tasks.end();
			--it;
		}

		if (it->display) // display flag set? remove display flag from all previous update entries (of the same type).
		{
			for (calcTaskIt it2 = m_tasks.begin(); it2 != it; ++it2)
			{
				if (it2->type == type)
					it2->display = sFALSE;
			}
		}

		m_taskLock.leave();
		m_change.set();
	}

	static void __cdecl hlUpdateThreadFunc(LPVOID param)
	{
		CGraphWindow::opData* me = (CGraphWindow::opData*) param;
		sU32 last3DOp = 0;
		HANDLE waitEvents[2] = { me->m_closeCalc, me->m_change };

		while (1)
		{
			if (WaitForMultipleObjects(2, waitEvents, FALSE, INFINITE) == WAIT_OBJECT_0)
				break;

			while (1)
			{
				me->m_taskLock.enter();
				if (me->m_tasks.size())
				{
					calcTask task = me->m_tasks.front();
					me->m_tasks.pop_front();

					if (me->m_tasks.size())
						me->m_change.set();
					else
						me->m_change.reset();

					frPlugin* plug = 0;
					if (g_graph->m_ops.find(task.opID) != g_graph->m_ops.end())
						plug = g_graph->m_ops[task.opID].plg;

					me->m_taskLock.leave();
					me->m_calcLock.enter();

					// recalculate
					LARGE_INTEGER start, end, freq;
					QueryPerformanceFrequency(&freq);
					QueryPerformanceCounter(&start);
					sBool ok = plug ? plug->process() : sFALSE;
					QueryPerformanceCounter(&end);

					::PostMessage(g_winMap.Get(IDR_MAINFRAME), WM_SET_ACTION_MESSAGE, 0, 0);
          const sInt time = (end.QuadPart - start.QuadPart) / (freq.QuadPart / 1000);

					if (time && ok)
						::PostMessage(g_winMap.Get(IDR_MAINFRAME), WM_SET_CALC_TIME, 0, time);

					if (task.display)
					{
						if (task.type == 0) // texture (may affect 3dview too)
							::PostMessage(g_winMap.Get(ID_PREVIEW_WINDOW), WM_SET_DISPLAY_TEXTURE, 0, ok ? task.opID : 0);
						else // geo/compose
							last3DOp = ok ? task.opID : 0;

						::PostMessage(g_winMap.Get(ID_3DPREVIEW_WINDOW), WM_SET_DISPLAY_MESH, 0, last3DOp);
					}

					me->m_calcLock.leave();
				}
				else
				{
					me->m_taskLock.leave();
					break;
				}
			}
		}

		me->m_closeCalcAck.set();
	}

	void hlSyncCalculations()
	{
		sInt sz = 1;

		while (sz)
		{
			m_taskLock.enter();
			sz = m_tasks.size();
			m_taskLock.leave();
		}

		m_calcLock.enter();
		m_calcLock.leave();
	}

	// ---- highlevel functions: operator creation/deletion
	sU32 hlCreateOp(sInt type, sInt posX, sInt posY)
	{
		frPluginDef* d;

		if (type < 0)
			d = frGetPluginByID(-type);
		else
			d = frGetPluginByIndex(type);

		frOpGraph::frOperator& op = g_graph->addOperator();

		op.def = d;
		op.plg = d->create(d);
		op.rc = CRect(CPoint(posX, posY), CSize(100, 25));

		createButton(op);
		linkOpIn(op);
		clearSelection();
		addToSelection(op.opID);
		hlSetDisplay(op.opID, -1);
		hlSetEdit(op.opID);

		if (op.def->ID != 13 && op.def->ID != 14 && op.def->ID != 138 && op.def->ID != 139 && op.def->ID != 141) // weder load noch store
			m_host->SetFocus();

		m_host->UpdateWindow();
		return op.opID;
	}

	// ---- highlevel functions: selections

	savedSelection* hlSaveSelection() const
	{
		savedSelection* sel = new savedSelection(m_selection.size());
    if (m_selection.size())
		  memcpy(sel->selection, &m_selection[0], m_selection.size() * sizeof(sU32));

		return sel;
	}

	void hlRestoreSelection(savedSelection* sel)
	{
		clearSelection();
		m_selection.reserve(sel->selSize);

		for (sInt i = 0; i < sel->selSize; i++)
		{
			sU32 id = sel->selection[i];
			m_selection.push_back(id);

			opButtonListIt it = findOpButton(id);
			if (it != m_opButtons.end())
			{
				it->setSelect(sTRUE);
				invalidateOpButton(it->getID());
			}
		}
	}

	// ---- highlevel functions: delete support

	void hlUpdateEditOp()
	{
		// get the current edit op
		frOpGraph::opMapIt it = g_graph->m_ops.find(g_graph->m_editOp);
		if (it != g_graph->m_ops.end())
		{
			// if it has links, we need to update it, because that link may be broken now
			frPlugin* plg = it->second.plg;

			if (plg->def->nInputs != plg->getNTotalInputs()) // we have link params?
				hlSetEdit(g_graph->m_editOp, sTRUE); // update it!
		}
	}
};

// ---- undo operations

class moveSelectionCommand: public frUndoCommand
{
	CGraphWindow::opData*	m_opData;
	sInt									m_moveX;
	sInt									m_moveY;

public:
	moveSelectionCommand(CGraphWindow::opData* opData, sInt moveX, sInt moveY)
		: frUndoCommand(1), m_opData(opData), m_moveX(moveX), m_moveY(moveY)
	{
	}

	void execute()
	{
		m_opData->moveSelection(m_moveX, m_moveY);
		m_opData->updateSelectionConnections();
	}

	void unExecute()
	{
		m_opData->moveSelection(-m_moveX, -m_moveY);
		m_opData->updateSelectionConnections();
	}

	sBool mergeWith(frUndoCommand* b)
	{
		moveSelectionCommand* cmd = (moveSelectionCommand*) b;

		FRDASSERT(m_opData == cmd->m_opData);
		m_moveX += cmd->m_moveX;
		m_moveY += cmd->m_moveY;

		return sTRUE;
	}
};

class setViewCommand: public frUndoCommand
{
	CGraphWindow::opData*	m_opData;
	sU32									m_prevView;
	sU32									m_prevView3D;
	sU32									m_newView;

public:
	setViewCommand(CGraphWindow::opData* opData, sU32 newView)
		: frUndoCommand(2), m_opData(opData), m_newView(newView)
	{
		m_prevView = g_graph->m_viewOp;
		m_prevView3D = g_graph->m_view3DOp;
	}

	void execute()
	{
		FRDASSERT(g_graph->m_viewOp == m_prevView);
		FRDASSERT(g_graph->m_view3DOp == m_prevView3D);

		m_opData->hlSetDisplay(m_newView, -1);
	}

	void unExecute()
	{
		FRDASSERT(g_graph->m_viewOp == m_newView || g_graph->m_view3DOp == m_newView);

		m_opData->hlSetDisplay(m_prevView, 0);
		m_opData->hlSetDisplay(m_prevView3D, 1);
	}

	sBool mergeWith(frUndoCommand* b)
	{
		return sFALSE;
	}
};

class setEditCommand: public frUndoCommand
{
	CGraphWindow::opData*	m_opData;
	sU32									m_oldEditOp;
	sU32									m_newEditOp;

public:
	setEditCommand(CGraphWindow::opData* opData, sU32 newEditOp)
		: frUndoCommand(8), m_opData(opData), m_newEditOp(newEditOp)
	{
		m_oldEditOp = g_graph->m_editOp;
	}

	void execute()
	{
		FRDASSERT(g_graph->m_editOp == m_oldEditOp);
		m_opData->hlSetEdit(m_newEditOp);
	}

	void unExecute()
	{
		FRDASSERT(g_graph->m_editOp == m_newEditOp);
		m_opData->hlSetEdit(m_oldEditOp);
	}

	sBool mergeWith(frUndoCommand* b)
	{
		setEditCommand* cmd = (setEditCommand*) b;

		m_newEditOp = cmd->m_newEditOp;
		return sTRUE;
	}
};

class setSelectionCommand: public frUndoCommand
{
	CGraphWindow::opData*	m_opData;
	savedSelection*				m_oldSelection;
	savedSelection*				m_newSelection;

public:
	setSelectionCommand(CGraphWindow::opData* opData, savedSelection* oldSelection)
		: frUndoCommand(3), m_opData(opData), m_oldSelection(oldSelection)
	{
		m_newSelection = m_opData->hlSaveSelection();
	}

	~setSelectionCommand()
	{
		delete m_oldSelection;
		delete m_newSelection;

		m_oldSelection = 0;
		m_newSelection = 0;
	}

	void execute()
	{
		m_opData->hlRestoreSelection(m_newSelection);
	}

	void unExecute()
	{
		m_opData->hlRestoreSelection(m_oldSelection);
	}

	sBool mergeWith(frUndoCommand* b)
	{
		setSelectionCommand* cmd = (setSelectionCommand*) b;

		FRDASSERT(cmd->m_opData == m_opData);

		delete m_newSelection;
		m_newSelection = cmd->m_newSelection;
		cmd->m_newSelection = 0;

		return sTRUE;
	}
};

class createOpCommand: public frUndoCommand
{
	CGraphWindow::opData*	m_opData;
	savedSelection*				m_oldSelection;
	sInt									m_type;
	sInt									m_posX, m_posY;
	sInt									m_sizeX, m_sizeY;
	sU32									m_opID;
	sU32									m_oldCounter;
	sU32									m_oldViewOp, m_oldView3DOp, m_oldEditOp;

public:
	createOpCommand(CGraphWindow::opData* opData, sInt type, sInt posX, sInt posY, sInt sizeX = 100, sInt sizeY = 25)
		: frUndoCommand(4), m_opData(opData), m_type(type), m_posX(posX), m_posY(posY), m_sizeX(sizeX), m_sizeY(sizeY)
	{
		m_oldSelection = m_opData->hlSaveSelection();
		m_oldCounter = g_graph->m_opIDCounter;

		m_oldViewOp = g_graph->m_viewOp;
		m_oldView3DOp = g_graph->m_view3DOp;
		m_oldEditOp = g_graph->m_editOp;
	}

	~createOpCommand()
	{
		delete m_oldSelection;
		m_oldSelection = 0;
	}

	void execute()
	{
		FRDASSERT(g_graph->m_viewOp == m_oldViewOp);
		FRDASSERT(g_graph->m_view3DOp == m_oldView3DOp);
		FRDASSERT(g_graph->m_editOp == m_oldEditOp);
		FRDASSERT(g_graph->m_opIDCounter == m_oldCounter);

		m_opID = m_opData->hlCreateOp(m_type, m_posX, m_posY);
	}

	void unExecute()
	{
		m_opData->deleteOp(m_opID);
		m_opData->hlRestoreSelection(m_oldSelection);
		
		g_graph->m_opIDCounter = m_oldCounter;
		m_opData->hlSetEdit(m_oldEditOp);
		m_opData->hlSetDisplay(m_oldView3DOp, 1);
		m_opData->hlSetDisplay(m_oldViewOp, 0);
	}

	sBool mergeWith(frUndoCommand* b)
	{
		return sFALSE;
	}

	frOpGraph::frOperator& getOp() const
	{
		return g_graph->m_ops[m_opID];
	}
};

class deleteOpCommand: public frUndoCommand
{
	CGraphWindow::opData*	m_opData;
	sU32									m_opToDelete;
	sBool									m_wasSelected, m_wasViewed, m_wasEdited;
	fr::streamMTmp				m_mStream;
	savedSelection*				m_singleOpSelection;
	CPoint								m_pastePoint;

public:
	deleteOpCommand(CGraphWindow::opData* opData, sU32 opToDelete)
		: frUndoCommand(5), m_opData(opData), m_opToDelete(opToDelete)
	{
		m_wasSelected = m_opData->inSelection(opToDelete);
		m_wasViewed = g_graph->m_viewOp == opToDelete || g_graph->m_view3DOp == opToDelete;
		m_wasEdited = g_graph->m_editOp == opToDelete;
		m_mStream.open();

		savedSelection* oldSelection = m_opData->hlSaveSelection(); // hacky, but who cares
		m_opData->clearSelection();
		m_opData->addToSelection(m_opToDelete);
		m_singleOpSelection = m_opData->hlSaveSelection();
		m_opData->copyToStream(m_mStream, &m_pastePoint);
		m_opData->hlRestoreSelection(oldSelection);
		delete oldSelection;
	}

	~deleteOpCommand()
	{
		delete m_singleOpSelection;
	}

	void execute()
	{
		FRDASSERT(!m_wasSelected || m_opData->inSelection(m_opToDelete));
		FRDASSERT(!m_wasViewed || g_graph->m_viewOp == m_opToDelete || g_graph->m_view3DOp == m_opToDelete);
		FRDASSERT(!m_wasEdited || g_graph->m_editOp == m_opToDelete);

		m_opData->hlSyncCalculations();
		m_opData->deleteOp(m_opToDelete);
		m_opData->hlUpdateEditOp();
	}

	void unExecute()
	{
		savedSelection* oldSelection = m_opData->hlSaveSelection();
		m_mStream.seek(0);
		m_opData->pasteFromStream(m_mStream, m_pastePoint.x, m_pastePoint.y, m_singleOpSelection);
		if (!m_wasSelected)
			m_opData->hlRestoreSelection(oldSelection);

		delete oldSelection;

		if (m_wasViewed)
			m_opData->hlSetDisplay(m_opToDelete, -1);

		if (m_wasEdited)
			m_opData->hlSetEdit(m_opToDelete);
	}

	sBool mergeWith(frUndoCommand* b)
	{
		return sFALSE;
	}
};

class deleteSelectionCommand: public frUndoCommand
{
	CGraphWindow::opData*	m_opData;
	savedSelection*				m_oldSelection;
	fr::streamMTmp				m_dataStream;
	CPoint								m_pastePos;
	sU32									m_oldCounter;
	sU32									m_oldView;
	sU32									m_oldView3D;
	sU32									m_oldEdit;

public:
	deleteSelectionCommand(CGraphWindow::opData* opData)
		: frUndoCommand(6), m_opData(opData)
	{
		m_dataStream.open();
		m_opData->copyToStream(m_dataStream, &m_pastePos);
		m_oldCounter = g_graph->m_opIDCounter;
		m_oldSelection = m_opData->hlSaveSelection();

		m_oldEdit = m_opData->inSelection(g_graph->m_editOp) ? g_graph->m_editOp : 0;
		m_oldView = m_opData->inSelection(g_graph->m_viewOp) ? g_graph->m_viewOp : 0;
		m_oldView3D = m_opData->inSelection(g_graph->m_view3DOp) ? g_graph->m_view3DOp : 0;
	}

	~deleteSelectionCommand()
	{
		delete m_oldSelection;
	}

	void execute()
	{
		m_opData->deleteSelection();
		m_opData->hlUpdateEditOp();
	}

	void unExecute()
	{
		m_dataStream.seek(0);
		m_opData->pasteFromStream(m_dataStream, m_pastePos.x, m_pastePos.y, m_oldSelection);
		g_graph->m_opIDCounter = m_oldCounter;

		if (m_oldEdit)		m_opData->hlSetEdit(m_oldEdit);
		if (m_oldView)		m_opData->hlSetDisplay(m_oldView, 0);
		if (m_oldView3D)	m_opData->hlSetDisplay(m_oldView3D, 1);
	}

	sBool mergeWith(frUndoCommand* b)
	{
		return sFALSE;
	}
};

class pasteCommand: public frUndoCommand
{
	CGraphWindow::opData*	m_opData;
	savedSelection*				m_oldSelection;
	savedSelection*				m_pasteSelection;
	fr::streamM						m_mStream;
	CPoint								m_pastePos;
	sU32									m_oldCounter;

public:
	pasteCommand(CGraphWindow::opData* opData)
		: frUndoCommand(7), m_opData(opData), m_pasteSelection(0)
	{
		m_oldSelection = m_opData->hlSaveSelection();
		m_oldCounter = g_graph->m_opIDCounter;

		sBool dataOk = sFALSE;

		if (IsClipboardFormatAvailable(m_opData->m_host->m_clipboardFormat) && m_opData->m_host->OpenClipboard())
		{
			HGLOBAL hClipboardData = GetClipboardData(m_opData->m_host->m_clipboardFormat);
			void* ptr = GlobalLock(hClipboardData);
			
			if (ptr)
			{
				fr::streamM clipData;

				if (clipData.open(ptr, GlobalSize(hClipboardData)))
				{
					m_mStream.openNew(clipData.size());
					m_mStream.copy(clipData);
					dataOk = sTRUE;
				}

				GlobalUnlock(hClipboardData);
			}

			CloseClipboard();
		}

		if (!dataOk)
			m_mStream.openNew(0);

		CPoint cursorPos;
		GetCursorPos(&cursorPos);
		m_opData->m_host->GetScrollOffset(m_pastePos);
		m_opData->m_host->ScreenToClient(&cursorPos);

		m_pastePos.x = snap(cursorPos.x + m_pastePos.x - 13);
		m_pastePos.y = snap(cursorPos.y + m_pastePos.y - 13);
	}

	~pasteCommand()
	{
		delete m_oldSelection;
		delete m_pasteSelection;
	}

	void execute()
	{
		m_mStream.seek(0);
		m_opData->pasteFromStream(m_mStream, m_pastePos.x, m_pastePos.y, m_pasteSelection);
		if (!m_pasteSelection)
			m_pasteSelection = m_opData->hlSaveSelection();
	}

	void unExecute()
	{
		m_opData->deleteSelection();
		m_opData->hlRestoreSelection(m_oldSelection);
		g_graph->m_opIDCounter = m_oldCounter;
	}

	sBool mergeWith(frUndoCommand* b)
	{
		return sFALSE;
	}
};

// setParameter => ParamWindow.

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGraphWindow::CGraphWindow()
  : m_fStartDrag(0), m_fDrag(0), m_onButton(0), m_scrlX(0), m_scrlY(0), m_fDragMenuActive(0), m_originalSelection(0)
{
  m_buttonsBitmap.LoadBitmap(IDB_BUTTONBITMAPS);

  m_operators = new opData(this);
  FRASSERT(m_operators != 0);
}

CGraphWindow::~CGraphWindow()
{
  KillDragMenu();

  FRSAFEDELETE(m_operators);
}

// --- message handling

LRESULT CGraphWindow::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  m_clipboardFormat = RegisterClipboardFormat(_T("frRG2.1"));
	m_operators->hlStartUpdateThread();
  m_fJustCreated=1;
    
  bHandled=FALSE;
  return 0;
}

LRESULT CGraphWindow::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CPoint ptCursor(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
  CMenu addMenu;
  
  addMenu.LoadMenu(IDR_GRAPHADDMENU);

  CMenuHandle addMenuPopup(addMenu.GetSubMenu(0));

  for (sInt i=0; i<4; i++)
    CMenuHandle(addMenuPopup.GetSubMenu(i)).RemoveMenu(0, MF_BYPOSITION);

  frOpGraph::frOpPage &curPage=g_graph->m_pages[g_graph->m_curPage];
  
  //sInt cmds=frEnumPlugins((curPage.type==0)?TextureEnumFunc:(curPage.type==1)?GeometryEnumFunc:ComposeEnumFunc, addMenuPopup);

	sInt cmds = 0;
	for (frPluginDefIterator pdi; pdi; ++pdi)
	{
		if (pdi->type == curPage.type && pdi->isVisible)
		{
			CMenuHandle subMenu(addMenuPopup.GetSubMenu(pdi->isSystem ? 3 : pdi->minInputs));
			subMenu.AppendMenu(0, pdi.getIndex() + 100, pdi->name);
		}

		cmds++;
	}

  sInt cmd = TrackPopupMenu(addMenuPopup, TPM_NONOTIFY|TPM_RETURNCMD, ptCursor.x, ptCursor.y, 0, m_hWnd, 0);

  if (cmd >= 100 && cmd < (cmds + 100))
  {
		CPoint scrl;

    ScreenToClient(&ptCursor);
		GetScrollOffset(scrl);
		ptCursor += scrl;

		g_graph->m_undo.addAndExecute(new createOpCommand(m_operators, cmd - 100, snap(ptCursor.x), snap(ptCursor.y)));
  }
  else if (cmd==ID_GRAPH_CENTER)
    m_operators->centerScroll();
  else if (cmd==ID_GRAPH_SEARCHSTORE)
    DoSearchStore();
  else if (cmd==ID_GRAPH_RECONNECTALL)
    m_operators->reconnectAll();
  else
  {
    CPoint ptScroll;
    GetScrollOffset(ptScroll);
    ScreenToClient(&ptCursor);

    sU32 ht=m_operators->hitTest(ptCursor+ptScroll);
    if (ht)
    {
      switch (cmd)
      {
      case ID_GRAPH_EDIT:
				g_graph->m_undo.addAndExecute(new setEditCommand(m_operators, ht));
        break;

      case ID_GRAPH_VIEW:
				g_graph->m_undo.addAndExecute(new setViewCommand(m_operators, ht));
        break;

      case ID_GRAPH_DELETE:
				g_graph->m_undo.addAndExecute(new deleteOpCommand(m_operators, ht));
        break;
      }
    }
  }
  
  return 0;
}

LRESULT CGraphWindow::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  UninitializeFlatSB(*this);
	m_operators->hlStopUpdateThread();
  
  bHandled = FALSE;
  return 0;
}

LRESULT CGraphWindow::OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CDCHandle dc=(HDC) wParam;

  RECT rc;
  GetClientRect(&rc);

  CRgn fillRgn;
  fillRgn.CreateRectRgn(rc.left, rc.top, rc.right, rc.bottom);
  m_operators->killButtonsFromRegion(fillRgn, rc);

  CBrush fillBrush;
  fillBrush.CreateSolidBrush(RGB(174, 169, 167));
  dc.FillRgn(fillRgn, fillBrush);
  
  return 1;
}

LRESULT CGraphWindow::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  switch (wParam)
  {
  case VK_DELETE:
  case VK_BACK:
		g_graph->m_undo.addAndExecute(new deleteSelectionCommand(m_operators));
    return 0;

  case 'X': // werkkzeug-style copy & paste: cut
    m_operators->copySelection();
		g_graph->m_undo.addAndExecute(new deleteSelectionCommand(m_operators));
    return 0;

  case 'C': // werkkzeug-style copy & paste: copy
    m_operators->copySelection();
    return 0;

  case 'V': // werkkzeug-style copy & paste: paste
		g_graph->m_undo.addAndExecute(new pasteCommand(m_operators));
    return 0;

  case 'Y': // delete
    if (!m_operators->gotSelection()) // if nothing selected, select item under (mouse) cursor
    {
      CPoint ptCursor, ptScroll;
      CRect myRect;

      GetCursorPos(&ptCursor);
      GetScrollOffset(ptScroll);
      ScreenToClient(&ptCursor);

      const sU32 hit=m_operators->hitTest(ptCursor+ptScroll);
      if (hit)
        m_operators->addToSelection(hit);
    }
      
		g_graph->m_undo.addAndExecute(new deleteSelectionCommand(m_operators));
    return 0;

  case 'S': // show
    {
      CPoint ptCursor, ptScroll;

      GetCursorPos(&ptCursor);
      GetScrollOffset(ptScroll);
      ScreenToClient(&ptCursor);

      const sU32 hit = m_operators->hitTest(ptCursor + ptScroll);
      if (hit)
				g_graph->m_undo.addAndExecute(new setViewCommand(m_operators,  hit));
    }
    return 0;

  case VK_SPACE:
    ShowDragMenu();
    return 0;
  }

  bHandled=FALSE;
  return 0;
}

LRESULT CGraphWindow::OnKeyUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  switch (wParam)
  {
  case VK_SPACE:
    if (m_fDragMenuActive)
      KillDragMenu();

    SetFocus();

    return 0;
  }

  bHandled=FALSE;
  return 0;
}

LRESULT CGraphWindow::OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CPoint ptCursor(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
  CPoint ptScroll;

  SetFocus();
  GetScrollOffset(ptScroll);

  sU32 onButton = m_operators->hitTest(ptCursor+ptScroll);
  m_onButton = 0;

  if (!onButton)
  {
    m_fDrag = 1;
    m_fStartDrag = 1;
    m_dragOrigin = ptCursor;
		m_originalSelection = m_operators->hlSaveSelection();
  
    if (::GetCapture() != m_hWnd)
      SetCapture();
  
    if (!(wParam & MK_SHIFT))
      m_operators->clearSelection();
    else
      m_operators->saveSelection();
  }
  else
  {
    if (wParam & MK_CONTROL)
			g_graph->m_undo.addAndExecute(new setViewCommand(m_operators, onButton));
    else
    {
      if (!m_operators->inSelection(onButton))
      {
        if (!(wParam & MK_SHIFT))
        {
          m_operators->clearSelection();
          m_operators->addToSelection(onButton);
        }
        else
          m_operators->addToSelection(onButton);
      }

      m_baseRect = g_graph->m_ops[onButton].rc;
      m_oldRect = m_baseRect;
      m_dragOrigin = ptCursor;
      m_onButton = onButton;
      m_fMoved = 0;
      
      if (::GetCapture()!=m_hWnd)
        SetCapture();
    }
  }

  bHandled=FALSE;
  return 0;
}

LRESULT CGraphWindow::OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  if (::GetCapture() == m_hWnd)
   ReleaseCapture();

  if (m_onButton)
  {
    if (!(GetKeyState(VK_SPACE) & 0x80)) // space not pressed?
    {
      CPoint ptCursor(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

      if (!m_fMoved) // l-click
				g_graph->m_undo.addAndExecute(new setEditCommand(m_operators, m_onButton));
      else
			{
				if (m_operators->checkSelectionCollides())
				{
					g_graph->m_undo.undo(); // undo move.
					g_graph->m_undo.purgeRedo();
					Beep(440, 100);
				}
				else
					m_operators->updateSelectionConnections();
			}

      m_onButton = 0;
    } // else kill selected op(s)
    else
			g_graph->m_undo.addAndExecute(new deleteSelectionCommand(m_operators));
  }

  if (m_fDrag)
  {
		if (!m_operators->isEqualSelection(m_originalSelection))
			g_graph->m_undo.add(new setSelectionCommand(m_operators, m_originalSelection));
		else
			delete m_originalSelection;

		m_originalSelection = 0;

    m_fDrag = 0;

    CRect rcu;
    rcu.SetRect(m_dragOrigin, m_oldDragPoint);
    rcu.NormalizeRect();
    rcu.InflateRect(2, 2);
    
    InvalidateRect(&rcu, TRUE);
    UpdateWindow();
  }

  bHandled = FALSE;
  return 0;
}

LRESULT CGraphWindow::OnRButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  m_fSized=0;

  SetFocus();
  
  if ((wParam & MK_LBUTTON) && m_onButton)
  {
    m_sizeOrigin=CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    CRect rc=g_graph->m_ops[m_onButton].rc;
    m_origSize=rc.Size();
  }
  else
    bHandled=FALSE;

  return 0;
}

LRESULT CGraphWindow::OnRButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  if (m_fSized)
    m_dragOrigin+=CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))-m_sizeOrigin;
  else
    bHandled=FALSE;

  return 0;
}

LRESULT CGraphWindow::OnMButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  m_scrlOrigin=CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

  if (::GetCapture()!=m_hWnd)
    SetCapture();

  SetFocus();

  bHandled=FALSE;
  return 0;
}

LRESULT CGraphWindow::OnMButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  if (::GetCapture()==m_hWnd && !(wParam & (MK_LBUTTON | MK_RBUTTON)))
    ::ReleaseCapture();

  bHandled=FALSE;
  return 0;
}

LRESULT CGraphWindow::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CPoint ptCursor(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

  ::SendMessage(g_winMap.Get(ID_MAIN_VIEW), WM_SET_FS_WINDOW, 2, 0);

  if (m_fDrag)
  {
    CDCHandle dc(GetDC());
    CRect     rc, rc2;

    CPen pen;
    pen.CreatePen(PS_SOLID, 2, RGB(174, 169, 167));

    CPenHandle oldPen=dc.SelectPen(pen);
    CBrushHandle oldBrush=dc.SelectBrush(AtlGetStockBrush(NULL_BRUSH));

    dc.SetROP2(R2_XORPEN);
    
    if (!m_fStartDrag)
    {
      rc2.SetRect(m_dragOrigin, m_oldDragPoint);
      rc2.NormalizeRect();
      dc.Rectangle(&rc2);
    }

    dc.SelectPen(oldPen);
    dc.SelectBrush(oldBrush);
    
    ReleaseDC(dc);
    
    rc.SetRect(m_dragOrigin, ptCursor);
    rc.NormalizeRect();
    
    InvalidateRect(&rc, FALSE);
    
    CPoint pt;
    CRect tempRC=rc;
    GetScrollOffset(pt);
    tempRC.OffsetRect(pt.x, pt.y);
    
    m_operators->selectFromRect(tempRC, !!(wParam & MK_SHIFT));
    UpdateWindow();
    
    dc=GetDC();
    oldPen=dc.SelectPen(pen);
    oldBrush=dc.SelectBrush(AtlGetStockBrush(NULL_BRUSH));
    dc.SetROP2(R2_XORPEN);
    dc.Rectangle(&rc);
    
    dc.SelectPen(oldPen);
    dc.SelectBrush(oldBrush);
    
    ReleaseDC(dc);
    
    m_fStartDrag=0;
    m_oldDragPoint=ptCursor;
    
    // autoscroll handling
    GetClientRect(&rc);
    if (!::PtInRect(&rc, ptCursor))
      SetTimer(1, 20);
    
    return 0;
  }
  else
  {
    CPoint ptScroll;
    GetScrollOffset(ptScroll);
    
    if (!m_onButton)
    {
      CRect rc;
      GetClientRect(&rc);
    
      if (::GetCapture()==m_hWnd)
      {
        m_operators->mouseMove(ptCursor+ptScroll);
      
        if (!::PtInRect(&rc, ptCursor) && !(wParam & MK_MBUTTON))
          ::ReleaseCapture();
      }
      else
      {
        if (m_operators->hitTest(ptCursor+ptScroll))
        {
          m_operators->mouseMove(ptCursor+ptScroll);
          SetCapture();
        }
      }

      UpdateWindow();
    }
    else
    {
      if (!(wParam & MK_RBUTTON))
      {
        CRect rc=m_baseRect;
        rc.OffsetRect(ptCursor-m_dragOrigin);

        rc.left=snap(rc.left);
        rc.top=snap(rc.top);
        rc.right=snap(rc.right);
        rc.bottom=snap(rc.bottom);

        if (rc != m_oldRect)
        {
					m_operators->moveSelection(rc.left - m_oldRect.left, rc.top - m_oldRect.top);
					g_graph->m_undo.add(new moveSelectionCommand(m_operators, rc.left - m_oldRect.left, rc.top - m_oldRect.top));
          m_oldRect = rc;
          m_operators->invalidateOpButton(m_onButton, sFALSE);
          m_fMoved = 1;
        }
      }
      else
      {
        m_fSized=1;

        CSize sz=m_origSize;
        
        sz.cx+=ptCursor.x-m_sizeOrigin.x+13;
        sz.cx=max(sz.cx-sz.cx%25, 75);

        m_operators->invalidateOpButton(m_onButton, sTRUE);

        RECT &rc=g_graph->m_ops[m_onButton].rc;
        m_oldRect.right=m_oldRect.left+sz.cx;
        m_baseRect.right=m_baseRect.left+sz.cx;
        rc.right=rc.left+sz.cx;

        m_operators->invalidateOpButton(m_onButton, sFALSE);
      }

      UpdateWindow();
    }
  }

  if (wParam & MK_MBUTTON)
  {
    CPoint pt;

    GetScrollOffset(pt);
    pt-=ptCursor-m_scrlOrigin;
    SetScrollOffset(pt.x, pt.y);

    m_scrlOrigin=ptCursor;
  }

  bHandled=FALSE;
  return 0;
}

LRESULT CGraphWindow::OnParmChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	m_operators->hlMarkForUpdate(g_graph->m_viewOp, 0, sTRUE);
	m_operators->hlMarkForUpdate(g_graph->m_view3DOp, 1, sTRUE);
  g_graph->modified();

  return 0;
}

LRESULT CGraphWindow::OnSetPage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  sU32 pageID = (sU32) wParam;

  frOpGraph::pgMapIt it;

  it=g_graph->m_pages.find(g_graph->m_curPage);
  if (it!=g_graph->m_pages.end())
  {
    CPoint scrl;
    GetScrollOffset(scrl);
    g_graph->m_pages[g_graph->m_curPage].scrlx=scrl.x;
    g_graph->m_pages[g_graph->m_curPage].scrly=scrl.y;
  }
  
  if (g_graph->m_curPage==pageID && !lParam || g_graph->m_pages.find(pageID)==g_graph->m_pages.end())
    return 0;

  m_operators->setCurrentPage(pageID);
  g_graph->m_curPage=pageID;
  
  frOpGraph::frOpPage &pg=g_graph->m_pages[pageID];
  fr::string title="Operator Graph: ";
  
  SetScrollOffset(pg.scrlx, pg.scrly);

  title+=pg.name;
  title+=" (";

  if (pg.type==0)
    title+="Texture";
  else if (pg.type==1)
    title+="Geometry";
  else if (pg.type==2)
    title+="Compositing";

  title+=")";

  SetWindowText(title);
  ::InvalidateRect(GetParent(), 0, FALSE);
  ::UpdateWindow(GetParent());

  Invalidate();
  UpdateWindow();
  
  return 0;
}

LRESULT CGraphWindow::OnDeleteCurPage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
  m_operators->deleteCurPage();
  UpdateWindow();

  return 0;
}

LRESULT CGraphWindow::OnSyncCalculations(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
	m_operators->hlSyncCalculations();
  
  return 0;
}

LRESULT CGraphWindow::OnClearGraph(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  m_operators->hlSetEdit(0);
  m_operators->hlSetDisplay(0, 0);
  m_operators->hlSetDisplay(0, 1);
	m_operators->hlSyncCalculations();
  g_graph->clear();
  
  return 0;
}

LRESULT CGraphWindow::OnFileLoaded(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CPoint ptTopLeft;

  ::SendMessage(m_hWnd, WM_SETPAGE, g_graph->m_curPage, 1);

  m_operators->hlSetEdit(g_graph->m_editOp, sTRUE);
  m_operators->hlSetDisplay(g_graph->m_viewOp, 0, sTRUE);
  m_operators->hlSetDisplay(g_graph->m_view3DOp, 1, sTRUE);
  
  ::SendMessage(g_winMap.Get(ID_SPLINE_WINDOW), WM_SET_EDIT_SPLINE, 0, 0);
  ::SendMessage(g_winMap.Get(ID_TIMELINE_WINDOW), WM_SET_CURRENT_CLIP, 0, 0);

  UpdateWindow();

  return 0;
}

LRESULT CGraphWindow::OnUpdateOpButton(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  sU32 opID = (sU32) lParam;

  // invalidate the op button...
  m_operators->invalidateOpButton(opID, sFALSE);

  frOpGraph::opMapIt it = g_graph->m_ops.find(opID);
  if (it != g_graph->m_ops.end()) // ...and all its outputs.
  {
    for (sInt i = 0; i < it->second.nOutputs; i++)
      m_operators->invalidateOpButton(it->second.outputs[i] & 0x7fffffff, sFALSE);
  }

  UpdateWindow();

  return 0;
}

LRESULT CGraphWindow::OnEditCopy(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  m_operators->copySelection();
  return 0;
}

LRESULT CGraphWindow::OnEditCut(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  m_operators->copySelection();
	g_graph->m_undo.addAndExecute(new deleteSelectionCommand(m_operators));

  return 0;
}

LRESULT CGraphWindow::OnEditPaste(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	g_graph->m_undo.addAndExecute(new pasteCommand(m_operators));
  return 0;
}

LRESULT CGraphWindow::OnSearchStore(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  DoSearchStore();
  return 0;
}

// ---- scroll stuff

void CGraphWindow::GetScrollOffset(CPoint &pt) const
{
  pt.x=m_scrlX;
  pt.y=m_scrlY;
}

void CGraphWindow::SetScrollOffset(sInt scrlX, sInt scrlY)
{
  if (scrlX!=m_scrlX || scrlY!=m_scrlY)
  {
    const sInt dx=scrlX-m_scrlX;
    const sInt dy=scrlY-m_scrlY;
    
    m_scrlX=scrlX;
    m_scrlY=scrlY;

    m_operators->doScroll(dx, dy);
  }
}

// ---- painting

LRESULT CGraphWindow::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  RECT rc;
  if (GetUpdateRect(&rc, FALSE))
  {
    CPaintDC dc(m_hWnd);
    m_operators->draw(dc.m_ps.hdc, rc);
  }

  return 0;
}

// ---- timer

LRESULT CGraphWindow::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  CPoint ptCursor;
  CRect rc;

  GetCursorPos(&ptCursor);
  ScreenToClient(&ptCursor);
  GetClientRect(&rc);

  if (::GetCapture()==m_hWnd && m_fDrag)
  {
    if (!::PtInRect(&rc, ptCursor))
    {
      sInt xscrl=0, yscrl=0;
    
      if (ptCursor.x<rc.left)
      {
        const sInt df=max(rc.left-ptCursor.x, 15);
        xscrl=-df/3;
      }
    
      if (ptCursor.x>=rc.right)
      {
        const sInt df=max(ptCursor.x-rc.right+1, 15);
        xscrl=df/3;
      }
    
      if (ptCursor.y<rc.top)
      {
        const sInt df=max(rc.top-ptCursor.y, 15);
        yscrl=-df/3;
      }
    
      if (ptCursor.y>=rc.bottom)
      {
        const sInt df=max(ptCursor.y-rc.bottom+1, 15);
        yscrl=df/3;
      }
    
      if (xscrl || yscrl)
      {
        CPoint pt;
        GetScrollOffset(pt);
        SetScrollOffset(pt.x+xscrl, pt.y+yscrl);
        
        m_fStartDrag=1;
        m_dragOrigin.x-=xscrl;
        m_dragOrigin.y-=yscrl;
      }
    }
    else
      KillTimer(1);
  }
  else
    KillTimer(1);

  return 0;
}

// ---- sizing

LRESULT CGraphWindow::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  if (!m_fJustCreated)
  {
    CRect newRC;
    CPoint pt;
    
    GetWindowRect(&newRC);
    GetScrollOffset(pt);
    SetScrollOffset(pt.x-m_windowRect.left+newRC.left, pt.y-m_windowRect.top+newRC.top);
  }
  
  m_fJustCreated=0;
  GetWindowRect(&m_windowRect);
  
  bHandled=FALSE;
  return 0;
}

LRESULT CGraphWindow::OnSetDisplayMesh(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  m_operators->hlSetDisplay(lParam, -1);

  return 0;
}

// ---- drag menu

class CGraphWindow::dragMenu: public CWindowImpl<CGraphWindow::dragMenu>
{
public:
  DECLARE_WND_CLASS(0);

  struct layoutItem
  {
    sUInt                   xpos, ypos;
    fr::sharedPtr<frPlugin> plg;
    CGraphWindow            *host;

    layoutItem()
      : xpos(0), ypos(0), host(0), plg(0)
    {
    }

    layoutItem(CGraphWindow *thehost, frPlugin *plug)
      : xpos(0), ypos(0), host(thehost), plg(plug)
    {
    }

    layoutItem(const layoutItem &x)
      : xpos(x.xpos), ypos(x.ypos), host(x.host), plg(x.plg)
    {
    }

    layoutItem &operator =(const layoutItem &x)
    {
      xpos=x.xpos;
      ypos=x.ypos;
      host=x.host;
      plg=x.plg;

      return *this;
    }

    void draw(CDCHandle dc, sBool hotLight=sFALSE, sInt xpos=-1, sInt ypos=-1, sInt sizex=100)
    {
      sInt type=plg->getButtonType();
      if (type<0 || type>4)
        type=0;

      const sInt srcx=hotLight?200:0;
      const sInt srcy=type*25;

      if (xpos==-1)
        xpos=this->xpos;

      if (ypos==-1)
        ypos=this->ypos;

      CDC tempDC;
      tempDC.CreateCompatibleDC(dc);

      CBitmapHandle oldBitmap=tempDC.SelectBitmap(host->m_buttonsBitmap);
      dc.BitBlt(xpos, ypos, 21, 25, tempDC, srcx, srcy, SRCCOPY);
      dc.StretchBlt(xpos+21, ypos, sizex-42, 25, tempDC, srcx+21, srcy, 100-21*2, 25, SRCCOPY);
      dc.BitBlt(xpos+sizex-21, ypos, 21, 25, tempDC, srcx+99-21, srcy, SRCCOPY);
      tempDC.SelectBitmap(oldBitmap);

      CRect rc(xpos, ypos+4, xpos+sizex, ypos+21);
      sInt leftBorder[5]  = { 4, 15, 4, 4, 4 };
      sInt rightBorder[5] = { 4, 4, 15, 22, 22 };

      rc.left+=leftBorder[type];
      rc.right-=rightBorder[type];

      CFontHandle oldFont=dc.SelectFont(AtlGetDefaultGuiFont());
      dc.SetBkMode(TRANSPARENT);
      dc.SetTextColor(RGB(0, 0, 0));
      dc.DrawText(plg->def->name, -1, &rc, DT_CENTER|DT_END_ELLIPSIS|DT_SINGLELINE|DT_VCENTER);
      dc.SelectFont(oldFont);
    }
  };

  typedef std::list<layoutItem>   layoutList;
  typedef layoutList::iterator    layoutListIt;

  CGraphWindow    *m_host;
  layoutList      m_plugins[4];
  layoutItem      *m_hotLight;
  CPoint          m_rectTop, m_dragPnt, m_dragPos, m_sizeOrigin;
  CSize           m_origSize, m_dragSize;
  CRect           m_menuRect;
  layoutItem      *m_dragging;
  unsigned        m_fRBDown:1;

  BEGIN_MSG_MAP(CGraphWindow::dragMenu)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
    MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
    MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
    MESSAGE_HANDLER(WM_RBUTTONDOWN, OnRButtonDown)
    MESSAGE_HANDLER(WM_RBUTTONUP, OnRButtonUp)
  END_MSG_MAP()

  dragMenu(CGraphWindow *host)
    : m_host(host), m_hotLight(0), m_dragging(0), m_fRBDown(0)
  {
  }

  ~dragMenu()
  {
    if (IsWindow())
      DestroyWindow();
  }

  LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    CRect rc;
    m_host->GetWindowRect(&rc);
    SetWindowPos(0, &rc, SWP_NOZORDER);

		for (frPluginDefIterator pdi; pdi; ++pdi)
		{
			if (pdi->isVisible && pdi->type == g_graph->m_pages[g_graph->m_curPage].type)
			{
				layoutItem itm(m_host, pdi->create(pdi));
				
				sInt type = 2;

				if (pdi->isSystem)
					type = 3;
				else if (pdi->nInputs < 2)
					type = pdi->nInputs;

				m_plugins[type].push_back(itm);
			}
		}

    CSize sz;
    CPoint ref, crsr;
    GetCursorPos(&crsr);
    crsr-=rc.TopLeft();
    calcLayout(sz, ref, crsr);
    
    m_menuRect=CRect(crsr, sz);
    m_rectTop+=m_menuRect.TopLeft();

    for (sInt i=0; i<4; i++)
    {
      for (layoutListIt it=m_plugins[i].begin(); it!=m_plugins[i].end(); ++it)
      {
        it->xpos+=m_menuRect.left;
        it->ypos+=m_menuRect.top;
      }
    }

    if (::GetCapture()!=m_hWnd)
      SetCapture();

    bHandled=FALSE;
    return 0;
  }

  LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    if (::GetCapture()==m_hWnd)
      ::ReleaseCapture();

    if (m_dragging)
    {
      CPoint ptDest=m_dragPos, ptScroll;

      m_host->GetScrollOffset(ptScroll);
      ClientToScreen(&ptDest);
      m_host->ScreenToClient(&ptDest);

			createOpCommand* cmd = new createOpCommand(m_host->m_operators, -m_dragging->plg->def->ID, snap(ptDest.x + ptScroll.x),
				snap(ptDest.y + ptScroll.y), m_dragSize.cx, m_dragSize.cy);
			g_graph->m_undo.addAndExecute(cmd);

			frOpGraph::frOperator& op = cmd->getOp();
			CPoint ptCursor;
			GetCursorPos(&ptCursor);
			m_host->ScreenToClient(&ptCursor);

      m_host->m_onButton = op.opID;
      m_host->m_baseRect = op.rc;
      m_host->m_oldRect = op.rc;
      m_host->m_dragOrigin = ptCursor;
    }

    bHandled=FALSE;
    return 0;
  }
  
  LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    m_host->Invalidate();
    m_host->UpdateWindow();

    return 1;
  }

  LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    CPaintDC dc(m_hWnd);

    CFontHandle myFont=AtlGetDefaultGuiFont();
    LOGFONT lf;
    myFont.GetLogFont(lf);

    if (lf.lfHeight<0)
      lf.lfHeight=-MulDiv(lf.lfHeight, dc.GetDeviceCaps(LOGPIXELSY), 72);

    CFontHandle oldFont=dc.SelectFont(myFont);

    CRect rc=m_menuRect;
    dc.FillSolidRect(rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, RGB(174, 169, 167));
    dc.Draw3dRect(rc.left-2, rc.top-2, rc.right-rc.left+4, rc.bottom-rc.top+4, RGB(200, 198, 196), RGB(103, 99, 96));
    dc.Draw3dRect(rc.left-1, rc.top-1, rc.right-rc.left+2, rc.bottom-rc.top+2, RGB(200, 198, 196), RGB(103, 99, 96));
    
    rc=CRect(m_rectTop, CSize(100, 100));
    dc.Draw3dRect(rc.left+1, rc.top+1, rc.right-rc.left-2, rc.bottom-rc.top-2, RGB(66, 65, 64),
      RGB(200, 198, 196));

    rc.DeflateRect(4, 2);
    dc.SetBkMode(TRANSPARENT);
    dc.SetTextColor(RGB(0, 0, 0));
    
    CRect tmp=rc;
    tmp.bottom=tmp.top+lf.lfHeight;
    dc.DrawText("Generator", -1, &tmp, DT_SINGLELINE|DT_CENTER);
    tmp=rc;
    tmp.top=tmp.bottom-lf.lfHeight;
    dc.DrawText("System", -1, &tmp, DT_SINGLELINE|DT_CENTER);
    tmp=rc;
    tmp.right=(tmp.left+tmp.right)/2;
    dc.DrawText("Filter", -1, &tmp, DT_SINGLELINE|DT_VCENTER|DT_LEFT);
    tmp=rc;
    tmp.left=(tmp.left+tmp.right)/2;
    dc.DrawText("Combiner", -1, &tmp, DT_SINGLELINE|DT_VCENTER|DT_RIGHT);

    dc.SelectFont(oldFont);

    for (sInt i=0; i<4; i++)
    {
      for (layoutListIt it=m_plugins[i].begin(); it!=m_plugins[i].end(); ++it)
        it->draw(dc.m_hDC);
    }

    if (m_dragging)
      m_dragging->draw(dc.m_hDC, sTRUE, m_dragPos.x, m_dragPos.y, m_dragSize.cx);

    return 0;
  }

  LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    CPoint ptCursor(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

    if (!m_dragging)
    {
      if (!::PtInRect(&m_menuRect, ptCursor))
        setHotLight(0);
      else
        setHotLight(hitTest(ptCursor.x, ptCursor.y));
    }
    else
    {
      if (!m_fRBDown)
      {
        CPoint snapPnt;

        snapPnt.x=snap(ptCursor.x-m_dragPnt.x);
        snapPnt.y=snap(ptCursor.y-m_dragPnt.y);

        if (snapPnt!=m_dragPos)
        {
          CRect rect(m_dragPos, m_dragSize);
          m_host->InvalidateRect(&rect);
          m_host->UpdateWindow();
        
          m_dragPos=snapPnt;
          Invalidate();
          UpdateWindow();
        }
      }
      else
      {
        CSize sz=m_origSize;
        
        sz.cx+=ptCursor.x-m_sizeOrigin.x;
        sz.cx=max(sz.cx-sz.cx%25, 75);

        if (sz!=m_dragSize)
        {
          CRect rect(m_dragPos, m_dragSize);
          m_host->InvalidateRect(&rect);
          m_host->UpdateWindow();

          m_dragSize=sz;
          Invalidate();
          UpdateWindow();
        }
      }
    }

    return 0;
  }

  LRESULT OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    CPoint ptCursor(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

    m_host->SetFocus();

    layoutItem *itm=hitTest(ptCursor.x, ptCursor.y);
    if (itm)
    {
      m_dragging=itm;
      m_dragPos=CPoint(itm->xpos, itm->ypos);
      m_dragPnt=ptCursor-m_dragPos;
      m_dragSize=CSize(100, 25);

      return 0;
    }

    return 0;
  }

  LRESULT OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    if (m_dragging)
    {
      m_dragging=0;
      Invalidate();
      UpdateWindow();
    }

    return 0;
  }

  LRESULT OnRButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    m_fRBDown=1;
    m_sizeOrigin=CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    m_origSize=m_dragSize;

    return 0;
  }

  LRESULT OnRButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    m_fRBDown=0;
    m_dragPnt+=CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))-m_sizeOrigin;

    return 0;
  }

  // ---- layout

  void calcLayout(CSize &sz, CPoint &ref, CPoint &crsrTopLeft)
  {
    sInt          topSize, botSize, lftSize, rgtSize, counter, curX, curY;
    layoutListIt  it;

    topSize=((m_plugins[0].size()+1)/2)*25;
    lftSize=((m_plugins[1].size()+3)/4)*100; if (!lftSize) lftSize=50;
    rgtSize=((m_plugins[2].size()+3)/4)*100; if (!rgtSize) rgtSize=50;
    botSize=((m_plugins[3].size()+1)/2)*25;

    // generators
    curX=lftSize-50;
    curY=topSize-25;
    for (it=m_plugins[0].begin(), counter=0; it!=m_plugins[0].end(); ++it, ++counter)
    {
      if ((counter&1)==0 && (m_plugins[0].size()-counter)<2)
        curX+=(m_plugins[0].size()&1)*50;

      it->xpos=curX+(counter&1)*100;
      it->ypos=curY;

      if (counter&1)
        curY-=25;
    }

    // filters
    curX=lftSize-100;
    curY=topSize;
    for (it=m_plugins[1].begin(), counter=0; it!=m_plugins[1].end(); ++it, ++counter)
    {
      it->xpos=curX;
      it->ypos=curY+(counter&3)*25;

      if ((counter&3)==3)
        curX-=100;
    }
    
    // combiners
    curX=lftSize+100;
    curY=topSize;
    for (it=m_plugins[2].begin(), counter=0; it!=m_plugins[2].end(); ++it, ++counter)
    {
      it->xpos=curX;
      it->ypos=curY+(counter&3)*25;

      if ((counter&3)==3)
        curX+=100;
    }

    // system
    curX=lftSize-50;
    curY=topSize+100;
    for (it=m_plugins[3].begin(), counter=0; it!=m_plugins[3].end(); ++it, ++counter)
    {
      if ((counter&1)==0 && (m_plugins[3].size()-counter)<2)
        curX+=(m_plugins[3].size()&1)*50;
      
      it->xpos=curX+(counter&1)*100;
      it->ypos=curY;

      if (counter&1)
        curY+=25;
    }

    m_rectTop=CPoint(lftSize, topSize);

    sz.cx=lftSize+100+rgtSize;
    sz.cy=topSize+100+botSize;

    ref=CPoint(lftSize+50, topSize+50);

    CPoint scrl;
    CRect clnt;
    m_host->GetScrollOffset(scrl);
    m_host->GetClientRect(&clnt);

    crsrTopLeft.x=fr::clamp(crsrTopLeft.x-ref.x, 0L, clnt.right-sz.cx-13);
    crsrTopLeft.y=fr::clamp(crsrTopLeft.y-ref.y, 0L, clnt.bottom-sz.cy-13);

    crsrTopLeft.x=snap(crsrTopLeft.x)-scrl.x%25;
    crsrTopLeft.y=snap(crsrTopLeft.y)-scrl.y%25;
  }

  // ---- misc

  layoutItem *hitTest(sInt x, sInt y)
  {
    for (sInt i=0; i<4; i++)
    {
      for (layoutListIt it=m_plugins[i].begin(); it!=m_plugins[i].end(); ++it)
      {
        if (x>=it->xpos && x<it->xpos+100 && y>=it->ypos && y<it->ypos+25)
          return &(*it);
      }
    }

    return 0;
  }

  void setHotLight(layoutItem *item)
  {
    if (m_hotLight!=item)
    {
      CDCHandle dc=GetDC();
      
      if (m_hotLight)
        m_hotLight->draw(dc, sFALSE);

      if (item)
        item->draw(dc, sTRUE);

      ReleaseDC(dc);

      m_hotLight=item;
    }
  }
};

void CGraphWindow::ShowDragMenu()
{
  if (!m_fDragMenuActive)
  {
    m_dragMenu=new dragMenu(this);
    m_dragMenu->Create(m_hWnd, rcDefault, 0, WS_POPUP|WS_VISIBLE, WS_EX_TRANSPARENT);
    m_fDragMenuActive=1;

    SetFocus();
  }
}

void CGraphWindow::KillDragMenu()
{
  if (m_fDragMenuActive)
  {
    FRSAFEDELETE(m_dragMenu);
    m_fDragMenuActive=0;
  }
}

// ---- search store stuff

class CSearchStoreDialog: public CSimpleDialog<IDD_SEARCHSTORE>
{
public:
	fr::string					m_oldSearch;
	frOpGraph::opMapIt	m_searchPos, m_searchStart;
	static sInt					s_searchMode;
	sBool								m_matchesFound;
	CGraphWindow*				m_host;

	CSearchStoreDialog(CGraphWindow* host)
		: m_host(host)
	{
		m_searchPos = g_graph->m_ops.begin();
		m_searchStart = m_searchPos;
	}

  BEGIN_MSG_MAP(CSearchStoreDialog)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    COMMAND_ID_HANDLER(IDOK, OnFindNextButton)
    CHAIN_MSG_MAP(CSimpleDialog<IDD_SEARCHSTORE>)
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
		SendDlgItemMessage(s_searchMode ? IDC_SEARCH_ALLPAGES : IDC_SEARCH_THISPAGE, BM_SETCHECK, 1, 0);

    bHandled = FALSE;
    return 0;
  }

  LRESULT OnFindNextButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
  {
		sChar buf[256];
    GetDlgItemText(IDC_SEARCHNAME, buf, 255);

		if (!buf[0])
			return 0;

		if (m_oldSearch.compare(buf, sTRUE))
		{
			m_searchStart = m_searchPos;
			m_matchesFound = sFALSE;
		}

		m_oldSearch = buf;
		s_searchMode = SendDlgItemMessage(IDC_SEARCH_ALLPAGES, BM_GETCHECK, 0, 0);

		sU32 searchPage = s_searchMode ? 0 : g_graph->m_curPage;
		frOpGraph::opMapIt oldIt, it = m_searchPos;

		do
		{	
			oldIt = it;
			if (++it == g_graph->m_ops.end())
				it = g_graph->m_ops.begin();

			if ((!searchPage || oldIt->second.pageID == searchPage) && !oldIt->second.getName().compare(buf, sTRUE))
			{
				// match found
				m_matchesFound = sTRUE;
				m_host->CenterOnOperator(oldIt->first);
				m_searchPos = it;

				return 0;
			}
		} while (it != m_searchStart);

		MessageBox(m_matchesFound ? _T("Finished search") : _T("No matches found"), _T("Find Op"), MB_ICONINFORMATION | MB_OK);

    return 0;
  }
};

sInt CSearchStoreDialog::s_searchMode = 0;

void CGraphWindow::DoSearchStore()
{
	CSearchStoreDialog dlg(this);
	dlg.DoModal();
}

void CGraphWindow::CenterOnOperator(sU32 id)
{
  frOpGraph::opMapIt it = g_graph->m_ops.find(id);

  if (it != g_graph->m_ops.end())
  {
    BOOL b;
    OnSetPage(WM_SETPAGE, it->second.pageID, 0, b); // dirty hack, who cares.

    m_operators->centerOnOperator(id);
    UpdateWindow();
  }
}
