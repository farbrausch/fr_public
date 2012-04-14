#pragma once

#include <gdiplus.h>

namespace lr
{
	namespace GUI
	{
		using namespace Gdiplus;

		template<class TOwner>
		class OffscreenPaint
		{
        protected:
			RECT m_rcPaint;
            bool m_bPaintOffscreen;
			CBitmap m_bitmap;

		public:

            OffscreenPaint()
            {
                m_bPaintOffscreen = true;
            }

			~OffscreenPaint()
			{
			}

			void UpdateOffscreenBitmap()
			{
				if (m_bitmap)
				{
					m_bitmap.DeleteObject();
				}
			}

            void NoOffscreenPainting()
            {
                m_bPaintOffscreen = false;
            }

			// override
			void Paint(Graphics& graphics)
			{
			}

			void PaintOffscreen(HWND hwnd)
			{
                if (!m_bPaintOffscreen)
                {
                    PaintRegular(hwnd);
                    return;
                }
				CPaintDC dc(hwnd);
				m_rcPaint = dc.m_ps.rcPaint;
				int rw = dc.m_ps.rcPaint.right - dc.m_ps.rcPaint.left;
				int rh = dc.m_ps.rcPaint.bottom - dc.m_ps.rcPaint.top;
				CRect rect;
				rect.SetRect(0,0,rw,rh);

				CDC bmp_dc;
				HBITMAP old_bmp;
				bmp_dc.CreateCompatibleDC(dc);

				if (!m_bitmap)
				{
					CRect clientRect;
					::GetClientRect(hwnd,&clientRect);
					m_bitmap.CreateCompatibleBitmap(dc,clientRect.Width(),clientRect.Height());
					old_bmp = bmp_dc.SelectBitmap(m_bitmap);
					Graphics graphics(bmp_dc);
					((TOwner*)this)->Paint(graphics);
				}
				else
				{
					old_bmp = bmp_dc.SelectBitmap(m_bitmap);
				}

				dc.BitBlt(dc.m_ps.rcPaint.left,dc.m_ps.rcPaint.top,rw,rh,bmp_dc,dc.m_ps.rcPaint.left,dc.m_ps.rcPaint.top,SRCCOPY);
				
				bmp_dc.SelectBitmap(old_bmp);	
			}

			void PaintRegular(HWND hwnd)
			{
				HDC				hdc;
				PAINTSTRUCT		ps;

				hdc = ::BeginPaint(hwnd,&ps);
				m_rcPaint = ps.rcPaint;

				Graphics graphics(hdc);
				((TOwner*)this)->Paint(graphics);

				::EndPaint(hwnd,&ps);				
			}
		};
	};
};
