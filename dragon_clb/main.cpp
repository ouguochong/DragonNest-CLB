#include "generic.hpp"
#include "main_form.hpp"

int __stdcall WinMain(HINSTANCE instance, HINSTANCE prev_instance, char* command_line, int command_show)
{
	HANDLE host_mutex = CreateMutex(NULL, FALSE, "sparta_mtx");

	if (!host_mutex || host_mutex == INVALID_HANDLE_VALUE)
	{
		return 0;
	}
	else if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		return 0;
	}
	
#ifdef PRINT_DEBUG_INFO
	AllocConsole();
	SetConsoleTitle("Terminal");
	AttachConsole(GetCurrentProcessId());
	
	FILE* pFile = nullptr;
	freopen_s(&pFile, "CON", "r", stdin);
	freopen_s(&pFile, "CON", "w", stdout);
	freopen_s(&pFile, "CON", "w", stderr);
#endif
	
	WSADATA wsa_data;

	if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
	{
		ReleaseMutex(host_mutex);
		return 0;
	}

	try
	{
		dragonnest_clb::gui::main_form::get_instance().show(true);
		dragonnest_clb::execute();
	}
	catch (std::exception& exception)
	{
		MessageBox(0, exception.what(), "An exception occured!", MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
	}
	
	if (WSACleanup() != 0)
	{
		ReleaseMutex(host_mutex);
		return 0;
	}

#ifdef PRINT_DEBUG_INFO
	FreeConsole();
#endif
	
	ReleaseMutex(host_mutex);
	return 0;
}