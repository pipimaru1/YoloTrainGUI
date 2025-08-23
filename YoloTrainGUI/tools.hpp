#pragma once

std::wstring GetComboText(HWND hCombo);
void SetComboText(HWND hCombo, const std::wstring& s);


//bool PickFolder(HWND owner, std::wstring& outPath, const std::wstring& initialDir = L"");
bool PickFolder(HWND owner, std::wstring& outPath);

// ディレクトリ選択ダイアログを表示する
bool PickFolderEx(HWND owner, std::wstring& outPath, const std::wstring& initialDir, std::wstring _title);
// コンボボックスからディレクトリを選択して、ディレクトリ選択ダイアログを表示する
bool PickFolderEx(HWND hDlg, UINT _ComboID, std::wstring title=L"");

bool PickFile(HWND owner, const COMDLG_FILTERSPEC* spec, UINT nSpec, std::wstring& outPath);
bool PickFileEx(HWND owner, const COMDLG_FILTERSPEC* spec, UINT nSpec,
    std::wstring& outPath, const std::wstring& initialDir, std::wstring _title);

bool PickFileEx(HWND hDlg, UINT _ComboID, const COMDLG_FILTERSPEC* spec, std::wstring title = L"");
