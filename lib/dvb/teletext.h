#ifndef __lib_dvb_teletext_h
#define __lib_dvb_teletext_h

#include <lib/base/object.h>
#include <lib/dvb/idvb.h>
#include <lib/dvb/pesparse.h>
#include <lib/dvb/pmt.h>
#include <lib/gdi/gpixmap.h>
#include <map>

struct eDVBTeletextSubtitlePageElement
{
	gRGB m_color;
	std::string m_line1;
	std::string m_line2;
	eRect m_area;

	int m_source_line;

	eDVBTeletextSubtitlePageElement(const gRGB &color, const std::string &line1, const std::string &line2, int source_line)
		: m_color(color), m_line1(line1), m_line2(line2), m_source_line(source_line)
	{
	}
};

struct eDVBTeletextSubtitlePage
{
	pts_t m_pts;
	int m_have_pts;
	int m_timeout; /* in pts */
	std::vector<eDVBTeletextSubtitlePageElement> m_elements;
	
	void clearLine(int line) { for (unsigned int i = 0; i < m_elements.size(); ) if (m_elements[i].m_source_line == line) m_elements.erase(m_elements.begin() + i); else ++i; }
	void clear() { m_elements.clear(); }
};

class eDVBTeletextParser: public iObject, public ePESParser, public Object
{
	DECLARE_REF(eDVBTeletextParser);
public:
	eDVBTeletextParser(iDVBDemux *demux);
	virtual ~eDVBTeletextParser();
	int start(int pid);
	void setPageAndMagazine(int page, int magazine);
	void setMagazine(int magazine);
	void connectNewPage(const Slot1<void,const eDVBTeletextSubtitlePage &> &slot, ePtr<eConnection> &connection);
	std::set<eDVBServicePMTHandler::subtitleStream> m_found_subtitle_pages;
private:
	std::map<int, unsigned int> m_modifications;
	void processPESPacket(__u8 *pkt, int len);
	
	ePtr<iDVBPESReader> m_pes_reader;
	ePtr<eConnection> m_read_connection;
	
	eDVBTeletextSubtitlePage m_subtitle_page;
	
	int m_C, m_Y, m_pid, m_page_M, m_page_X, m_page_open, m_double_height, m_box_open;
	int m_X28_0_valid, m_X28_t1, m_X28_t2;
	int m_M29_0_valid, m_M29_t1, m_M29_t2;

	void handlePageStart();
	void handleLine(unsigned char *line, int len);
	void handlePageEnd(int have_pts, const pts_t &pts);
	
	std::string m_subtitle_line1;
	std::string m_subtitle_line2;
	int m_subtitle_color;
	int m_current_source_line;
	
	void addSubtitleString(int color, std::string string, int source_line);
	
	void sendSubtitlePage();
	
	Signal1<void,const eDVBTeletextSubtitlePage&> m_new_subtitle_page;
};

#endif
