#include <lib/gui/esubtitle.h>
#include <lib/gdi/grc.h>
#include <lib/base/estring.h>
#include <cmath>
#include <lib/gdi/gfbdc.h>
	/*
		ok, here's much room for improvements.
	
		first, the placing of the individual elements is sub-optimal.
		then maybe a colored background would be an option.
		....
 	*/	

eSubtitleWidget::eSubtitleWidget(eWidget *parent)
	: eWidget(parent), m_hide_subtitles_timer(eTimer::create(eApp))
{
	setBackgroundColor(gRGB(0,0,0,255));
	m_page_ok = 0;
	m_dvb_page_ok = 0;
	m_pango_page_ok = 0;
#ifdef WITH_SDL
	ePtr<gSDLDC> my_dc;
	gSDLDC::getInstance(my_dc);
#else
	ePtr<gFBDC> my_dc;
	gFBDC::getInstance(my_dc);
#endif	
	fontSize = std::ceil(28 * my_dc->getVerticalResolution() / 576); // PAL (576 lines) is default. For HD (more vertical resolution) we will increase font size, for NTSC we will decrease.
	eDebug("Subtitle font size is %d", fontSize);
	CONNECT(m_hide_subtitles_timer->timeout, eSubtitleWidget::clearPage);
}

#define startX 50
void eSubtitleWidget::setPage(const eDVBTeletextSubtitlePage &p)
{
	m_page = p;
	m_page_ok = 1;
	invalidate(m_visible_region);  // invalidate old visible regions
	m_visible_region.rects.clear();

	int elements = m_page.m_elements.size();
	if (elements)
	{
		int startY = elements > 1
			? size().height() / 2
			: size().height() / 3 * 2;
		int width = size().width() - startX * 2;
		int height = size().height() - startY;
		int size_per_element = height / (elements ? elements : 1);
		for (int i=0; i<elements; ++i)
		{
			eRect &area = m_page.m_elements[i].m_area;
			area.setLeft(startX);
			area.setTop(size_per_element * i + startY);
			area.setWidth(width);
			area.setHeight(size_per_element);
			m_visible_region.rects.push_back(area);
		}
	}
	m_hide_subtitles_timer->start(7500, true);
	invalidate(m_visible_region);  // invalidate new regions
}

void eSubtitleWidget::setPage(const eDVBSubtitlePage &p)
{
	eDebug("setPage");
	m_dvb_page = p;
	invalidate(m_visible_region);  // invalidate old visible regions
	m_visible_region.rects.clear();
	for (std::list<eDVBSubtitleRegion>::iterator it(m_dvb_page.m_regions.begin()); it != m_dvb_page.m_regions.end(); ++it)
	{
		eDebug("add %d %d %d %d", it->m_position.x(), it->m_position.y(), it->m_pixmap->size().width(), it->m_pixmap->size().height());
		m_visible_region.rects.push_back(eRect(it->m_position, it->m_pixmap->size()));
	}
	m_dvb_page_ok = 1;
	m_hide_subtitles_timer->start(7500, true);
	invalidate(m_visible_region);  // invalidate new regions
}

void eSubtitleWidget::setPage(const ePangoSubtitlePage &p)
{
	m_pango_page = p;
	m_pango_page_ok = 1;
	invalidate(m_visible_region);  // invalidate old visible regions
	m_visible_region.rects.clear();

	int elements = m_pango_page.m_elements.size();
	if (elements)
	{
		int startY = elements > 1
			? size().height() / 2
			: size().height() / 3 * 2;
		int width = size().width() - startX * 2;
		int height = size().height() - startY;
		int size_per_element = height / (elements ? elements : 1);
		for (int i=0; i<elements; ++i)
		{
			eRect &area = m_pango_page.m_elements[i].m_area;
			area.setLeft(startX);
			area.setTop(size_per_element * i + startY);
			area.setWidth(width);
			area.setHeight(size_per_element);
			m_visible_region.rects.push_back(area);
		}
	}
	int timeout_ms = m_pango_page.m_timeout;
	m_hide_subtitles_timer->start(timeout_ms, true);
	invalidate(m_visible_region);  // invalidate new regions
}

void eSubtitleWidget::clearPage()
{
	eDebug("subtitle timeout... hide");
	m_page_ok = 0;
	m_dvb_page_ok = 0;
	m_pango_page_ok = 0;
	invalidate(m_visible_region);
	m_visible_region.rects.clear();
}

void eSubtitleWidget::setPixmap(ePtr<gPixmap> &pixmap, gRegion changed)
{
	m_pixmap = pixmap;
	invalidate(changed);
}

int eSubtitleWidget::event(int event, void *data, void *data2)
{
	switch (event)
	{
	case evtPaint:
	{
		ePtr<eWindowStyle> style;
		gPainter &painter = *(gPainter*)data2;

		getStyle(style);
		eWidget::event(event, data, data2);

		if (m_pixmap)
			painter.blit(m_pixmap, ePoint(0,0));
		else if (m_page_ok)
		{
			int elements = m_page.m_elements.size();
			ePtr<gFont> font = new gFont("Regular", fontSize); // TODO: Make font variant and font size a setting somewhere? Or perhaps let the skin decide?
			painter.setFont(font);
			for (int i=0; i<elements; ++i)
			{
				eDVBTeletextSubtitlePageElement &element = m_page.m_elements[i];
				eRect &area = element.m_area;
				eRect shadow = area;
				shadow.moveBy(3,3);
				if (!element.m_line1.empty())
				{
					//eDebug("Rendering subtitle line 1 %s at yPos %d", element.m_line1.c_str(), area.top());
					painter.setForegroundColor(gRGB(0,0,0));
					painter.renderText(shadow, element.m_line1, gPainter::RT_VALIGN_TOP|gPainter::RT_HALIGN_CENTER);
					painter.setForegroundColor(gRGB(255,255,255)); // TODO: make subtitle color a setting somewhere?
					painter.renderText(area, element.m_line1, gPainter::RT_VALIGN_TOP|gPainter::RT_HALIGN_CENTER);
					shadow.moveBy(0, 44); // TODO: Use font size * 1.2 or something as the line height instead of this hard-coded number.
					area.moveBy(0, 44);
				}
				if (!element.m_line2.empty())
				{
					//eDebug("Rendering subtitle line 2 %s at yPos %d", element.m_line2.c_str(), area.top());
					painter.setForegroundColor(gRGB(0,0,0));
					painter.renderText(shadow, element.m_line2, gPainter::RT_VALIGN_TOP|gPainter::RT_HALIGN_CENTER);
					painter.setForegroundColor(gRGB(255,255,255));
					painter.renderText(area, element.m_line2, gPainter::RT_VALIGN_TOP|gPainter::RT_HALIGN_CENTER);
				}
			}
		}
		else if (m_pango_page_ok)
		{
			int elements = m_pango_page.m_elements.size();
			ePtr<gFont> font = new gFont("Regular", fontSize);
			for (int i=0; i<elements; ++i)
			{
				ePangoSubtitlePageElement &element = m_pango_page.m_elements[i];
				std::string text = element.m_pango_line;				
				std::string::size_type loc = text.find("<", 0 );
				if ( loc != std::string::npos )
				{
					switch (char(text.at(1)))
					{
					case 'i':
						eDebug("found italic");
						font = new gFont("LCD", fontSize + 2);
						break;
					case 'b':
						eDebug("found bold");
						font = new gFont("Replacement", fontSize + 2);
						break;
					default:
						break;
					}
					text = text.substr(3, text.length()-7);
				}
				text = replace_all(text, "&apos;", "'");
				text = replace_all(text, "&quot;", "\"");
				text = replace_all(text, "&amp;", "&");
				painter.setFont(font);		
				eRect &area = element.m_area;
				eRect shadow = area;
				shadow.moveBy(3,3);
				painter.setForegroundColor(gRGB(0,0,0));
				painter.renderText(shadow, text, gPainter::RT_WRAP|gPainter::RT_VALIGN_TOP|gPainter::RT_HALIGN_CENTER);
				painter.setForegroundColor(gRGB(255,255,255));
				painter.renderText(area, text, gPainter::RT_WRAP|gPainter::RT_VALIGN_TOP|gPainter::RT_HALIGN_CENTER);
			}
		}
		else if (m_dvb_page_ok)
		{
			for (std::list<eDVBSubtitleRegion>::iterator it(m_dvb_page.m_regions.begin()); it != m_dvb_page.m_regions.end(); ++it)
				painter.blit(it->m_pixmap, it->m_position);
		}
		return 0;
	}
	default:
		return eWidget::event(event, data, data2);
	}
}
