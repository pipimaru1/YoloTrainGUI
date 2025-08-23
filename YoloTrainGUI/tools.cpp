#define _WIN32_WINNT 0x0601
#include "pch.h"

#include "tools.hpp"

// ------------------------------
// ComboBox helpers
// ------------------------------

// コンボボックスのテキストを取得・設定する
std::wstring GetComboText(HWND hCombo)
{
    int len = (int)SendMessageW(hCombo, WM_GETTEXTLENGTH, 0, 0);
    std::wstring s(len, L'\0');
    SendMessageW(hCombo, WM_GETTEXT, len + 1, (LPARAM)s.data());
    return s;
}
void SetComboText(HWND hCombo, const std::wstring& s)
{
    SendMessageW(hCombo, WM_SETTEXT, 0, (LPARAM)s.c_str());
}


// ------------------------------
// IFileDialog helpers
// ------------------------------

//bool PickFolder(HWND owner, std::wstring& outPath, const std::wstring& initialDir)
bool PickFolder(HWND owner, std::wstring& outPath)
{
    IFileDialog* pfd = nullptr;
	// IFileDialogのインスタンスを作成する
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
    if (FAILED(hr)) 
        return false;
    else{
		// ダイアログボックスのオプションを取得して、フォルダ選択モードに変更する
        DWORD opts = 0; pfd->GetOptions(&opts);
        pfd->SetOptions(opts | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);

		// 初期表示フォルダの指定（省略可）
        //if (!initialDir.empty()) 
        //{
        //    IShellItem* psiInit = nullptr;
        //    if (SUCCEEDED(SHCreateItemFromParsingName(initialDir.c_str(), nullptr, IID_PPV_ARGS(&psiInit)))) {
        //        // 見た目だけ指定：ユーザーが他フォルダにも移動できる
        //        pfd->SetDefaultFolder(psiInit);
        //        // 「必ずこのフォルダを開いて開始したい」場合は SetFolder も
        //        // pfd->SetFolder(psiInit);
        //        psiInit->Release();
        //    }
        //}

        hr = pfd->Show(owner);
        if (FAILED(hr))
        {
            pfd->Release();
            return false;
        }else{
            IShellItem* psi = nullptr;
            hr = pfd->GetResult(&psi);
            if (FAILED(hr))
            {
                pfd->Release();
                return false;
            }else{
                PWSTR psz = nullptr;
                psi->GetDisplayName(SIGDN_FILESYSPATH, &psz);
                if (psz)
                {
                    outPath = psz;
                    CoTaskMemFree(psz);
                }else{
                    psi->Release();
                    pfd->Release();
                    return !outPath.empty();
                }
            }
        }
    }
}

// 拡張版：初期フォルダ指定、タイトル変更対応
static bool PickFolderEx(HWND owner, std::wstring& outPath, const std::wstring& initialDir, std::wstring _title)
{
    Microsoft::WRL::ComPtr<IFileDialog> pfd;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd))))
        return false;

    DWORD opts = 0; pfd->GetOptions(&opts);
    pfd->SetOptions(opts | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);

    // ★タイトルを変更
    pfd->SetTitle(_title.c_str());

    // 1) 初期フォルダを強制
    if (!initialDir.empty() && std::filesystem::exists(initialDir)) {
        // ルートは "C:\" のように末尾 '\' を必ず付ける
        std::wstring dir = initialDir;
        if (dir.size() == 2 && dir[1] == L':') dir += L'\\';

        Microsoft::WRL::ComPtr<IShellItem> psiInit;
        if (SUCCEEDED(SHCreateItemFromParsingName(dir.c_str(), nullptr, IID_PPV_ARGS(&psiInit)))) {
            // 「必ずこのフォルダで開く」
            pfd->SetFolder(psiInit.Get());
            // それでも OS 側の履歴が勝つケースへの保険として既定値も重ねておく
            pfd->SetDefaultFolder(psiInit.Get());
        }
    }

    // 前回ファイル名が残っていると吸われることがあるので明示クリア
    pfd->SetFileName(L"");

    if (FAILED(pfd->Show(owner))) return false;

    Microsoft::WRL::ComPtr<IShellItem> psi;
    if (FAILED(pfd->GetResult(&psi))) return false;

    PWSTR psz = nullptr;
    psi->GetDisplayName(SIGDN_FILESYSPATH, &psz);
    if (!psz) return false;
    outPath = psz; CoTaskMemFree(psz);
    return !outPath.empty();
}

//上記関数のラッパー
bool PickFolderEx(HWND hDlg, UINT _ComboID, std::wstring _title)
{
    std::wstring _p;
    std::wstring _initdir = GetComboText(GetDlgItem(hDlg, _ComboID));

    if (PickFolderEx(hDlg, _p, _initdir, _title))
    {
        SetComboText(GetDlgItem(hDlg, _ComboID), _p);
        return true;
    }

    return false;
}


// ファイル選択ダイアログを表示する
bool PickFile(HWND owner, const COMDLG_FILTERSPEC* spec, UINT nSpec, std::wstring& outPath)
{
    IFileDialog* pfd = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
    if (FAILED(hr)) return false;
    pfd->SetFileTypes(nSpec, spec);
    hr = pfd->Show(owner);
    if (FAILED(hr)) { pfd->Release(); return false; }
    IShellItem* psi = nullptr;
    hr = pfd->GetResult(&psi);
    if (FAILED(hr)) { pfd->Release(); return false; }
    PWSTR psz = nullptr;
    psi->GetDisplayName(SIGDN_FILESYSPATH, &psz);
    if (psz) { outPath = psz; CoTaskMemFree(psz); }
    psi->Release();
    pfd->Release();
    return !outPath.empty();
}

// 拡張版：初期フォルダ指定、タイトル変更対応
 bool PickFileEx(HWND owner, const COMDLG_FILTERSPEC* spec, UINT nSpec,
    std::wstring& outPath, const std::wstring& initialDir, std::wstring _title)
{
    Microsoft::WRL::ComPtr<IFileDialog> pfd;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd))))
        return false;

    pfd->SetFileTypes(nSpec, spec);

    if (!initialDir.empty() && std::filesystem::exists(initialDir)) {
        std::wstring dir = initialDir;
        if (dir.size() == 2 && dir[1] == L':') 
            dir += L'\\';
        Microsoft::WRL::ComPtr<IShellItem> psiInit;
        if (SUCCEEDED(SHCreateItemFromParsingName(dir.c_str(), nullptr, IID_PPV_ARGS(&psiInit)))) {
            pfd->SetFolder(psiInit.Get());
            pfd->SetDefaultFolder(psiInit.Get());
        }
    }
    pfd->SetFileName(L""); // 念のため

    // ★タイトルを変更
    pfd->SetTitle(_title.c_str());

    if (FAILED(pfd->Show(owner))) 
        return false;

    Microsoft::WRL::ComPtr<IShellItem> psi;
    if (FAILED(pfd->GetResult(&psi))) 
        return false;

    PWSTR psz = nullptr;
    psi->GetDisplayName(SIGDN_FILESYSPATH, &psz);
    if (!psz) 
        return false;
    outPath = psz; CoTaskMemFree(psz);
    return !outPath.empty();
}
 bool PickFileEx(HWND hDlg, UINT _ComboID, const COMDLG_FILTERSPEC* spec, std::wstring title)
 {
     std::wstring _p;
     std::wstring _initdir = GetComboText(GetDlgItem(hDlg, _ComboID));
     if (PickFileEx(hDlg, spec, 1, _p, _initdir, title))
     {
         SetComboText(GetDlgItem(hDlg, _ComboID), _p);
         return true;
     }
     return false;
 }