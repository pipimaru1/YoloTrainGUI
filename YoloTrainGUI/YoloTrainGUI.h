#pragma once

#include "pch.h"
#include "resource.h"

// ------------------------------
// Globals
// ------------------------------
extern HINSTANCE g_hInst;
extern HWND g_hDlg;
extern std::atomic<HANDLE> g_hChildProc;
extern std::mutex g_logMutex;
extern std::wstring g_logBuffer;

void LogAppendANSI(const std::wstring& s);
void AppendLog(const std::wstring& s);
