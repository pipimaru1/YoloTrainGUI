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

 //////////////////////////////////////////////////////////////////////////////
 // data.yaml編集
 // YoloTrainGUI.cpp など適切な場所に追加（ヘッダにプロトタイプ宣言してもOK）
 using std::wstring;
 using std::string;

 static std::string ToForwardSlashes(const std::wstring& ws) {
     std::wstring w = ws;
     for (auto& ch : w) if (ch == L'\\') ch = L'/';
     // UTF-16 -> UTF-8
     int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
     std::string s(len, '\0');
     WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), s.data(), len, nullptr, nullptr);
     return s;
 }

 static bool ReadAllTextUTF8(const std::filesystem::path& path, std::string& out) {
     std::ifstream ifs(path, std::ios::binary);
     if (!ifs) return false;
     std::ostringstream ss;
     ss << ifs.rdbuf();
     out = ss.str();
     return true;
 }

 static bool WriteAllTextUTF8(const std::filesystem::path& path, const std::string& text) {
     std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
     if (!ofs) return false;
     ofs.write(text.data(), (std::streamsize)text.size());
     return (bool)ofs;
 }

 ////////////////////////////////////////////////////////////////////////////////
// 2) YAMLのtrain: / val : だけを安全に置換する関数
// 行頭の空白や「train : 」「train:」「train: 値」などのゆらぎに対応
// コメント行（#開始）は無視
// 最初に見つかったtrain / valのみ置換（複数重複がなければ十分）
 bool UpdateYoloYamlTrainVal(
     const std::filesystem::path& yamlPath,
     const std::wstring& baseDirW, // 例: L"C:\\yolodata"
     std::wstring& outMessage      // ログ用（UIに出すなど）
 ) {
     std::string content;
     if (!ReadAllTextUTF8(yamlPath, content)) {
         outMessage = L"YAML読み込みに失敗: " + yamlPath.wstring();
         return false;
     }

     // 新しい値（UTF-8, / 区切り）
     std::wstring trainW = baseDirW + L"\\dataset\\train\\images";
     std::wstring valW = baseDirW + L"\\dataset\\valid\\images";
     std::string trainNew = ToForwardSlashes(trainW);
     std::string valNew = ToForwardSlashes(valW);

     // 行単位で処理
     std::istringstream iss(content);
     std::ostringstream oss;
     std::string line;
     bool trainReplaced = false;
     bool valReplaced = false;

     // 正規表現： ^(\s*)(train|val)\s*:\s*(.*)$
     // 先頭空白とキーを保持して、値を差し替える
     std::regex kvre{ R"(^(\s*)(train|val)\s*:\s*(.*)$)" };

     while (std::getline(iss, line)) {
         std::smatch m;
         // コメント行はそのまま
         std::string trimmed = line;
         // 先頭の空白を飛ばして#ならコメント
         auto nonsp = trimmed.find_first_not_of(" \t\r");
         bool isComment = (nonsp != std::string::npos && trimmed[nonsp] == '#');

         if (!isComment && std::regex_match(line, m, kvre)) {
             std::string indent = m[1].str();
             std::string key = m[2].str();

             if (key == "train" && !trainReplaced) {
                 oss << indent << "train: " << trainNew << "\n";
                 trainReplaced = true;
                 continue;
             }
             if (key == "val" && !valReplaced) {
                 oss << indent << "val: " << valNew << "\n";
                 valReplaced = true;
                 continue;
             }
         }
         // マッチしない行はそのまま
         oss << line << "\n";
     }

     // バックアップ作成
     std::error_code ec;
     auto bak = yamlPath;
     bak += L".bak";
     std::filesystem::copy_file(yamlPath, bak, std::filesystem::copy_options::overwrite_existing, ec);

     std::string out = oss.str();
     if (!WriteAllTextUTF8(yamlPath, out)) {
         outMessage = L"YAML書き込みに失敗: " + yamlPath.wstring();
         return false;
     }

     std::wostringstream msg;
     msg << L"更新完了: " << yamlPath.wstring() << L"\n"
         << L"  train: " << std::filesystem::path(trainW).wstring() << L"\n"
         << L"  val  : " << std::filesystem::path(valW).wstring();
     if (!trainReplaced || !valReplaced) {
         msg << L"\n(注意: trainまたはvalが見つからず、新規行の追加はしていません。必要なら追加ロジックを足してください。)";
     }
     outMessage = msg.str();
     return true;
 }

