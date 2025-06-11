// ReSharper disable CppNonInlineFunctionDefinitionInHeaderFile
#pragma once

//add any other includes here your driver might use
#include <Windows.h>
#include <tlhelp32.h>

#include "vmmdll.h"
#include "leechcore.h"

#pragma comment(lib, "leechcore.lib")
#pragma comment(lib, "vmm.lib")

#include "Frontend/Windows/LogWindow.h"

/*
██████╗░██╗░░░░░███████╗░█████╗░░██████╗███████╗  ██████╗░███████╗░█████╗░██████╗░██╗
██╔══██╗██║░░░░░██╔════╝██╔══██╗██╔════╝██╔════╝  ██╔══██╗██╔════╝██╔══██╗██╔══██╗██║
██████╔╝██║░░░░░█████╗░░███████║╚█████╗░█████╗░░  ██████╔╝█████╗░░███████║██║░░██║██║
██╔═══╝░██║░░░░░██╔══╝░░██╔══██║░╚═══██╗██╔══╝░░  ██╔══██╗██╔══╝░░██╔══██║██║░░██║╚═╝
██║░░░░░███████╗███████╗██║░░██║██████╔╝███████╗  ██║░░██║███████╗██║░░██║██████╔╝██╗
╚═╝░░░░░╚══════╝╚══════╝╚═╝░░╚═╝╚═════╝░╚══════╝  ╚═╝░░╚═╝╚══════╝╚═╝░░╚═╝╚═════╝░╚═╝
*/


/// here should be your driver functions
///	DO NOT call any of those functions from any class,
///	they should only get called from the memory class (memory.cpp and memory.h)
/// DO NOT include this file in any other file, you might get linker errors!
/// ANY CHANGES you do to the params in functions, make sure you also edit the memory.cpp and memory.h file!

//global variables here
VMM_HANDLE vmm_handle;
DWORD vmm_process;

//in case you need to initialize anything BEFORE your com works, you can do this in here.
//this function IS NOT DESIGNED to already take the process name as input or anything related to the target process
//use the function "load" below which will contain data about the process name
inline void init()
{
	AllocConsole();

	const char* args[] = { "-device", "fpga" };

	vmm_handle = VMMDLL_Initialize(2, args);

	if (!vmm_handle)
		windows::LogWindow::Log(windows::LogWindow::logLevels::LOGLEVEL_ERROR, "DMA", "Failed to connect to the FPGA (dma) device.");
	else
		windows::LogWindow::Log(windows::LogWindow::logLevels::LOGLEVEL_INFO, "DMA", "Successfully connected to the DMA device.");
}

uint64_t _getBaseAddress(std::string processName, int& pid);

/**
 * \brief use this function to initialize the target process
 * \param processName process name as input
 * \param baseAddress base address of the process gets returned
 * \param processID process id of the process gets returned
 */
inline void loadData(std::string& processName, uint64_t& baseAddress, int& processID)
{
	baseAddress = _getBaseAddress(processName, processID);
}

/**
 * \brief read function (replace with your read logic)
 * \param address memory address to read from
 * \param buffer memory address to write to
 * \param size size of memory to read (expects the buffer/address to have this size too)
 */
inline void _read(const void* address, void* buffer, const DWORD64 size)
{
	BOOL b = VMMDLL_MemReadEx(vmm_handle, vmm_process, (uintptr_t)address, (PBYTE)buffer, size, NULL, VMMDLL_FLAG_NOCACHE);

	//if failed, try with lower byte amount
	if (!b)
	{
		//always read 10 bytes lower
		for (int i = 1; i < size && !b; i += 10)
		{
			b = VMMDLL_MemReadEx(vmm_handle, vmm_process, (uintptr_t)address, (PBYTE)buffer, size, NULL, VMMDLL_FLAG_NOCACHE);
		}
	}
}

/**
 * \brief write function (replace with your write logic)
 * \param address memory address to write to
 * \param buffer memory address to write from
 * \param size size of memory to write (expects the buffer/address to have this size too)
 */
inline void _write(void* address, const void* buffer, const DWORD64 size)
{
	VMMDLL_MemWrite(vmm_handle, vmm_process, (uintptr_t)address, (PBYTE)buffer, size);
}

/**
 * \brief gets the process base address. If you adjust the params, make sure to change them in memory.cpp too
 * \param processName the name of the process
 * \param pid returns the process id
 * \return process base address
 */
uint64_t _getBaseAddress(std::string processName, int& pid)
{
	pid = VMMDLL_PidGetFromName(vmm_handle, processName.c_str(), &vmm_process);

	PVMMDLL_MAP_MODULEENTRY entry;
	if (VMMDLL_Map_GetModuleFromNameU(vmm_handle, vmm_process, processName.c_str(), &entry, 0)) {
		return (uint64_t)entry->vaBase;
	}
}
