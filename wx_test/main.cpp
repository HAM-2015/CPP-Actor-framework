// wx_test.cpp : 定义控制台应用程序的入口点。
//

#include "wx_ui.h"
#include <wx/wx.h>

class MyApp :public wxApp
{
public:
	bool OnInit()
	{
		ios_proxy ios;
		ios.run();
		{
			wx_ui wxTest(ios);
			wxTest.ShowModal();
		}
		ios.stop();
		return false;
	}
};

IMPLEMENT_APP(MyApp)
