//	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org)
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.

#include <crtdbg.h>

#include "application.h"

#include "resources/resource.h"

#include <atlbase.h>
#include <atlcom.h>
#include <common/constants.h>
#include <common/string.h>
#include <common/time.h>
#include <common/win32/configuration_registry.h>
#include <frontend/factory.h>
#include <frontend/system_stylesheet.h>
#include <ipc/com/init.h>
#include <scheduler/thread_queue.h>
#include <scheduler/ui_queue.h>
#include <ShellAPI.h>
#include <wpl/factory.h>
#include <wpl/freetype2/font_loader.h>
#include <wpl/win32/cursor_manager.h>

using namespace std;
using namespace wpl;

namespace micro_profiler
{
	class application::impl	{	};

	namespace
	{
		class Module : public CAtlExeModuleT<Module>
		{
		public:
			Module()
			{
				if (::AttachConsole(ATTACH_PARENT_PROCESS))
				{
					_stdout.reset(freopen("CONOUT$", "wt", stdout), [] (FILE *f) {
						fclose(f);
						::FreeConsole();
					});
				}
			}

			HRESULT RegisterServer(BOOL /*bRegTypeLib*/ = FALSE, const CLSID* pCLSID = NULL) throw()
			{	return CAtlExeModuleT<Module>::RegisterServer(FALSE, pCLSID);	}

			HRESULT UnregisterServer(BOOL /*bRegTypeLib*/ = FALSE, const CLSID* pCLSID = NULL) throw()
			{	return CAtlExeModuleT<Module>::UnregisterServer(FALSE, pCLSID);	}

		private:
			shared_ptr<FILE> _stdout;
		};


		class FauxFrontend : public IUnknown, public CComObjectRoot, public CComCoClass<FauxFrontend>
		{
		public:
			DECLARE_REGISTRY_RESOURCEID(IDR_PROFILER_FRONTEND)
			BEGIN_COM_MAP(FauxFrontend)
				COM_INTERFACE_ENTRY(IUnknown)
			END_COM_MAP()
		};

		OBJECT_ENTRY_NON_CREATEABLE_EX_AUTO(static_cast<const GUID &>(constants::standalone_frontend_id), FauxFrontend);


		shared_ptr<hive> open_configuration(const vector<string> &configuration_path)
		{
			auto h = registry_hive::open_user_settings("Software");

			for (auto i = configuration_path.begin(); i != configuration_path.end(); ++i)
				h = h->create(i->c_str());
			return h;
		}
	}


	application::application(const char *argv[], size_t argc)
		: _arguments(argv, argv + argc)
	{
		const auto clock_raw = &clock;
		const auto clock_mt = [clock_raw] {	return mt::milliseconds(clock_raw());	};
		const auto queue = make_shared<scheduler::ui_queue>(clock_mt);
		const auto text_engine = create_text_engine();
		const factory_context context = {
			make_shared<gcontext::surface_type>(1, 1, 16),
			make_shared<gcontext::renderer_type>(2),
			text_engine,
			make_shared<system_stylesheet>(text_engine),
			make_shared<wpl::win32::cursor_manager>(),
			clock_raw,
			[queue] (wpl::queue_task t, wpl::timespan defer_by) {
				return queue->schedule(move(t), mt::milliseconds(defer_by)), true;
			},
		};

		_factory = wpl::factory::create_default(context);
		_queue = queue;
		_worker_queue.reset(new scheduler::thread_queue(clock_mt));
		_config = open_configuration(c_configuration_path);
		setup_factory(*_factory);
	}

	application::~application()
	{	_queue->stop();	}

	scheduler::queue &application::get_ui_queue()
	{	return *_queue;	}

	scheduler::queue &application::get_worker_queue()
	{	return *_worker_queue;	}

	void application::run()
	{
		MSG msg;

		while (::GetMessage(&msg, NULL, 0, 0))
		{
			::TranslateMessage(&msg);
			::DispatchMessageW(&msg);
		}
	}

	void application::stop()
	{	::PostQuitMessage(0);	}

	void application::clipboard_copy(const string &text_utf8)
	{
		const auto text = unicode(text_utf8);

		if (::OpenClipboard(NULL))
		{
			if (HGLOBAL gtext = ::GlobalAlloc(GMEM_MOVEABLE, (text.size() + 1) * sizeof(wchar_t)))
			{
				wchar_t *gtext_memory = static_cast<wchar_t *>(::GlobalLock(gtext));

				copy(text.c_str(), text.c_str() + text.size() + 1, gtext_memory);
				::GlobalUnlock(gtext_memory);
				::EmptyClipboard();
				if (!::SetClipboardData(CF_UNICODETEXT, gtext))
					::GlobalFree(gtext);
			}
			::CloseClipboard();
		}
	}

	void application::open_link(const string &address)
	{	::ShellExecuteW(NULL, L"open", unicode(address).c_str(), NULL, NULL, SW_SHOWNORMAL);	}
}

int WINAPI _tWinMain(HINSTANCE /*instance*/, HINSTANCE /*previous_instance*/, LPTSTR command_line, int /*show_command*/)
try
{
	using namespace micro_profiler;

	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	HRESULT hresult;
	Module module;

	if (!module.ParseCommandLine(command_line, &hresult))
	{
		printf("(Un)Registration result: 0x%08X\n", hresult);
	}
	else
	{
		ipc::com::com_initialize ci;
		application app(nullptr, 0);

		main(app);
	}
	return 0;
}
catch (const exception &e)
{
	printf("Caught exception: %s...\nExiting!\n", e.what());
	return -1;
}
catch (...)
{
	printf("Caught an unknown exception...\nExiting!\n");
	return -1;
}

int main(int argc, const char *argv[])
try
{
	using namespace micro_profiler;

	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	application app(argv, argc);

	main(app);
	return 0;
}
catch (const exception &e)
{
	printf("Caught exception: %s...\nExiting!\n", e.what());
	return -1;
}
catch (...)
{
	printf("Caught an unknown exception...\nExiting!\n");
	return -1;
}
