///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __WX_UI_BASE_H__
#define __WX_UI_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/string.h>
#include <wx/textctrl.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/stattext.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class MyDialog1
///////////////////////////////////////////////////////////////////////////////
class MyDialog1 : public wxDialog 
{
	private:
	
	protected:
		wxPanel* m_panel1;
		wxTextCtrl* m_textCtrl1;
		wxPanel* m_panel2;
		wxPanel* m_panel3;
		wxButton* btn_run;
		
		// Virtual event handlers, overide them in your derived class
		virtual void btn_click_run( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		MyDialog1( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 274,88 ), long style = wxDEFAULT_DIALOG_STYLE|wxMINIMIZE_BOX|wxSYSTEM_MENU ); 
		~MyDialog1();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class MyDialog2
///////////////////////////////////////////////////////////////////////////////
class MyDialog2 : public wxDialog 
{
	private:
	
	protected:
		wxStaticText* m_staticText1;
		wxButton* m_button3;
		
		// Virtual event handlers, overide them in your derived class
		virtual void btn_cancel( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		MyDialog2( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 133,115 ), long style = wxDEFAULT_DIALOG_STYLE ); 
		~MyDialog2();
	
};

#endif //__WX_UI_BASE_H__
