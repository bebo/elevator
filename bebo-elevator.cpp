#include "stdafx.h"
#include "args.hxx"
#include <iostream>
#include <string>
#include <windows.h>

const wchar_t *GetWC(const char *c)
{
	const size_t cSize = strlen(c) + 1;
	wchar_t* wc = new wchar_t[cSize];
	mbstowcs(wc, c, cSize);

	return wc;
}

int main(int argc, char* argv[])
{
	args::ArgumentParser parser("", "");
	args::Positional<unsigned long> wait_pid(parser, "PID", "process id to wait");
	args::Positional<std::string> exe_path(parser, "EXE", "executable path to launch");
	args::HelpFlag help(parser, "help", "Display help menu", { 'h', "help" });
	args::Flag admin(parser, "admin", "run exe as admin", { 'a', "admin" }, false);
	args::ValueFlag<int> timeout(parser, "timeout", "timeout", { 't', "timeout" }, 5000);

	try {
		parser.ParseCLI(argc, argv);
	} catch (args::Help) {
		std::cout << parser;
		return 0;
	} catch (args::ParseError e) {
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return 1;
	}

	if (!wait_pid || !exe_path) {
		std::cout << parser;
		return 0;
	}

	DWORD wait_result = 0;
	HANDLE wait_handle = 0;

	if (wait_pid.Get() != 0) {
		wait_handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, false, wait_pid.Get());

		if (!wait_handle) {
			std::cout << "unable to open process";
			return 1;
		}

		wait_result = WaitForSingleObject(wait_handle, timeout.Get());
	}

	bool ok = false;
	HWND hwnd = GetActiveWindow();
	if (wait_result == WAIT_OBJECT_0) {
		SHELLEXECUTEINFO sei = { sizeof(sei) };
		sei.lpVerb = admin ? L"runas" : L"open";
		sei.lpFile = GetWC(exe_path.Get().c_str());
		sei.hwnd = hwnd;
		sei.nShow = SW_NORMAL;

		if (!ShellExecuteEx(&sei)) {
			DWORD dwError = GetLastError();
			if (dwError == ERROR_CANCELLED) {
				std::cout << "user refused the elevation";
			} else {
				std::cout << "unexpected error";
			}
		} else {
			ok = true;
		}
	} else if (wait_result == WAIT_TIMEOUT) {
		std::cout << "Timeout waiting for the process";
	} else {
		std::cout << "Unknown code: " << wait_result;
		if (wait_result == WAIT_FAILED) {
			std::cout << " (" << GetLastError() << ")";
		}
	}

	if (wait_handle != 0) {
		CloseHandle(wait_handle);
	}
    return 0;
}

