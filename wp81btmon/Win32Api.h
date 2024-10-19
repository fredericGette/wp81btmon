#pragma once

// See https://github.com/tandasat/SecRuntimeSample/blob/master/SecRuntimeSampleNative/Win32Api.h

// In case of error "error LNK2019: unresolved external symbol __imp_SetConsoleCtrlHandler referenced in function "
// see https://stackoverflow.com/a/19455274
extern "C" {
	WINBASEAPI HMODULE WINAPI GetModuleHandleW(LPCWSTR lpModuleName);

	WINBASEAPI HANDLE WINAPI CreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
	WINBASEAPI BOOL	WINAPI SetConsoleCtrlHandler(PHANDLER_ROUTINE HandlerRoutine, BOOL Add);
	char *getenv(const char *varname);
	WINSOCK_API_LINKAGE	u_long WSAAPI htonl(_In_ u_long hostlong);
	WINBASEAPI BOOL WINAPI DeviceIoControl(HANDLE hDevice, DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped);
}

#define WIN32API_TOSTRING(x) #x

// Link exported function
#define WIN32API_INIT_PROC(Module, Name)  \
  Name(reinterpret_cast<decltype(&::Name)>( \
      ::GetProcAddress((Module), WIN32API_TOSTRING(Name))))

// Convenientmacro to declare function
#define WIN32API_DEFINE_PROC(Name) const decltype(&::Name) Name

class Win32Api {

private:
	// Returns a base address of KernelBase.dll
	static HMODULE GetKernelBase() {
		return GetBaseAddress(&::DisableThreadLibraryCalls);
	}

	// Returns a base address of the given address
	static HMODULE GetBaseAddress(const void *Address) {
		MEMORY_BASIC_INFORMATION mbi = {};
		if (!::VirtualQuery(Address, &mbi, sizeof(mbi))) {
			return nullptr;
		}
		const auto mz = *reinterpret_cast<WORD *>(mbi.AllocationBase);
		if (mz != IMAGE_DOS_SIGNATURE) {
			return nullptr;
		}
		return reinterpret_cast<HMODULE>(mbi.AllocationBase);
	}

public:
	const HMODULE m_Kernelbase;
	WIN32API_DEFINE_PROC(GetModuleHandleW);
	WIN32API_DEFINE_PROC(CreateFileA);
	WIN32API_DEFINE_PROC(SetConsoleCtrlHandler);
	WIN32API_DEFINE_PROC(DeviceIoControl);
	const HMODULE m_MSVCR110;
	WIN32API_DEFINE_PROC(getenv);
	const HMODULE m_WS2_32;
	WIN32API_DEFINE_PROC(htonl);

	Win32Api()
		: m_Kernelbase(GetKernelBase()),
		WIN32API_INIT_PROC(m_Kernelbase, GetModuleHandleW),
		WIN32API_INIT_PROC(m_Kernelbase, CreateFileA),
		WIN32API_INIT_PROC(m_Kernelbase, SetConsoleCtrlHandler),
		WIN32API_INIT_PROC(m_Kernelbase, DeviceIoControl),
		m_MSVCR110(GetModuleHandleW(L"MSVCR110.DLL")),
		WIN32API_INIT_PROC(m_MSVCR110, getenv),
		m_WS2_32(GetModuleHandleW(L"m_WS2_32.DLL")),
		WIN32API_INIT_PROC(m_WS2_32, htonl)
	{};

};