#define _WIN32_WINNT 0x0601
#include "pch.h"

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Uuid.lib")
#pragma comment(lib, "Comctl32.lib")


#include "resource.h"
#include "resource_user.h"
#include "YoloTrainGUI.h"
#include "Tooltip.hpp"

namespace fs = std::filesystem;
static std::mutex g_storeMutex;
 
// ------------------------------
// Globals
// ------------------------------
HWND g_hDlg = nullptr;
std::atomic<HANDLE> g_hChildProc(nullptr);
std::mutex g_logMutex;
std::wstring g_logBuffer;

const wchar_t *RET = L"\r\n";
static TOOLINFOW g_TipTI{};
static bool      bTmpTipShow = false;

// ツールチップ
Tooltip ttTmpDir; // グローバルなツールチップオブジェクト

static std::wstring strTipTempDir =
L"TempDir\r\n"
L"├─ source\r\n"
L"│   ├─ images (from shared)\r\n"
L"│   └─ labels (from shared)\r\n"
L"├─ train\r\n"
L"│   ├─ images\r\n"
L"│   └─ labels\r\n"
L"└─ valid\r\n"
L"    ├─ images\r\n"
L"    └─ labels\r\n"
L"\r\n"
L"元データは ./TempDir/source にコピーされます。\r\n"
L"トレーニング用と検証用に分割されると、\r\n"
L"./TempDir/train および./TempDir/valid にコピーされます。\r\n";

static std::wstring strTipDataYaml = 
L"クラシフィケーション定義とデータフォルダを記載します。 \r\n"
L"下記を参考に記述してください。絶対パスの方が安定します。\r\n"
L"\r\n"
L"train: C:/TempDir/train/images \r\n"
L"val : C:/TempDir/valid/images \r\n";

static std::wstring strTipTrainPy =
L"train.pyを指定します。\r\n"
L"通常はパスを入れずに train.py とだけ記述してください。\r\n";

static std::wstring strTipPythonExe =
L"python.exeを指定します。\r\n"
L"通常はパスを入れずに Python.exe とだけ記述してください。\r\n";

static std::wstring strTipWorkDir =
L"Yolov5の作業ディレクトリを指定します。\r\n"
L"train.pyのあるディレクトリを指定してください。\r\n";

void SetTootips(HWND hDlg)
{
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STC_TEMP, L"Tempolary Directory", strTipTempDir.c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STC_DATA_YAML, L"Tempolary Directory", strTipDataYaml.c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STC_TRAINPY, L"train.py", strTipTrainPy.c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STC_PYTHONEXE, L"train.py", strTipPythonExe.c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STC_WORKDIR, L"WorkDir", strTipWorkDir.c_str());
}

// ------------------------------
// 文字コード変換＆保存先パスユーティリティ
// ------------------------------

static std::string ToUTF8(const std::wstring& w) {
    if (w.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), s.data(), len, nullptr, nullptr);
    return s;
}
static std::wstring FromUTF8(const std::string& s) {
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), w.data(), len);
    return w;
}
static fs::path GetStoreDir() {
    PWSTR p = nullptr;
    SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, nullptr, &p);
    fs::path dir = fs::path(p) / L"YoloV5Trainer";
    CoTaskMemFree(p);
    std::error_code ec; fs::create_directories(dir, ec);
    return dir;
}
static fs::path GetIniFile() 
{ 
    //return GetStoreDir() / L"mru_history.ini"; 

    return L"mru_history.ini";
}
static fs::path GetHistoryFile() 
{ 
    //return GetStoreDir() / L"history.txt"; 

    return  L"history.txt";
}

// ------------------------------
// INI（コロン区切り）簡易パーサ
// data[section] = {value0, value1, ...}
// ------------------------------
static std::map<std::wstring, std::vector<std::wstring>> ReadIniColon()
{
    std::map<std::wstring, std::vector<std::wstring>> data;
    std::lock_guard<std::mutex> lk(g_storeMutex);
    fs::path ini = GetIniFile();
    if (!fs::exists(ini)) return data;

    std::ifstream ifs(ini, std::ios::binary);
    if (!ifs) return data;
    std::string bytes((std::istreambuf_iterator<char>(ifs)), {});
    // strip UTF-8 BOM if present
    if (bytes.size() >= 3 && (unsigned char)bytes[0] == 0xEF && (unsigned char)bytes[1] == 0xBB && (unsigned char)bytes[2] == 0xBF) {
        bytes.erase(0, 3);
    }
    std::wstring text = FromUTF8(bytes);

    std::wstring cur;
    size_t pos = 0;
    while (pos < text.size()) {
        size_t eol = text.find_first_of(L"\r\n", pos);
        std::wstring line = text.substr(pos, (eol == std::wstring::npos ? text.size() - pos : eol - pos));
        if (eol != std::wstring::npos) {
            if (text[eol] == L'\r' && eol + 1 < text.size() && text[eol + 1] == L'\n') eol++;
            pos = eol + 1;
        }
        else pos = text.size();

        // trim
        auto ltrim = [&](std::wstring& s) { s.erase(0, s.find_first_not_of(L" \t")); };
        auto rtrim = [&](std::wstring& s) { size_t k = s.find_last_not_of(L" \t"); if (k == std::wstring::npos) s.clear(); else s.erase(k + 1); };
        ltrim(line); rtrim(line);
        if (line.empty() || line[0] == L';' || line[0] == L'#') continue;

        if (line.size() > 2 && line.front() == L'[' && line.back() == L']') {
            cur = line.substr(1, line.size() - 2);
            continue;
        }
        if (cur.empty()) continue;
        // "index:value"
        size_t colon = line.find(L':');
        if (colon == std::wstring::npos) continue;
        std::wstring value = line.substr(colon + 1);
        ltrim(value); rtrim(value);
        if (!value.empty()) data[cur].push_back(value);
    }
    return data;
}
static void WriteIniColon(const std::map<std::wstring, std::vector<std::wstring>>& data)
{
    std::lock_guard<std::mutex> lk(g_storeMutex);
    std::wstring out;
    for (auto& kv : data) {
        out += L"[" + kv.first + L"]\r\n";
        const auto& vec = kv.second;
        size_t n = vec.size();
        for (size_t i = 0; i < n; i++) {
            out += std::to_wstring(i) + L":" + vec[i] + L"\r\n";
        }
        out += L"\r\n";
    }
    std::ofstream ofs(GetIniFile(), std::ios::binary | std::ios::trunc);

    std::string u8 = ToUTF8(out);
    ofs.write(u8.data(), (std::streamsize)u8.size());
}


// ------------------------------
// Utilities
// ------------------------------

static void SetProgress(int v)
{
    SendDlgItemMessageW(g_hDlg, IDC_PROGRESS, PBM_SETPOS, (WPARAM)v, 0);
}
static void ResetProgress()
{
    SendDlgItemMessageW(g_hDlg, IDC_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SetProgress(0);
}

static std::vector<std::wstring> LoadMRU(const std::wstring& section)
{
    auto m = ReadIniColon();
    auto it = m.find(section);
    if (it == m.end()) return {};
    return it->second;
}
static void SaveMRU(const std::wstring& section, const std::wstring& value, size_t maxItems = 256)
{
    if (value.empty()) return;
    auto m = ReadIniColon();
    auto& v = m[section];
    // 前方重複排除
    v.erase(std::remove(v.begin(), v.end(), value), v.end());
    v.insert(v.begin(), value);
    if (v.size() > maxItems) v.resize(maxItems);
    WriteIniColon(m);
}
static void LoadMRUToCombo(HWND hCombo, const std::wstring& section)
{
    SendMessageW(hCombo, CB_RESETCONTENT, 0, 0);
    for (auto& s : LoadMRU(section)) {
        SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)s.c_str());
    }
}

// --- Flags loader for simple 0/1 states in mru_history.ini ---
static bool LoadFlagFromIni(const wchar_t* key, bool def = false)
{
    auto v = LoadMRU(key);               // section=key の先頭要素を採用（"1" or "0"想定）
    if (v.empty()) return def;
    const std::wstring& s = v.front();
    if (s == L"1") return true;
    if (s == L"0") return false;
    // 念のため true 系テキストも許容
    if (_wcsicmp(s.c_str(), L"true") == 0 || _wcsicmp(s.c_str(), L"on") == 0) return true;
    return def;
}


// ------------------------------
// Command history
// ------------------------------
static void AppendCmdHistory(const std::wstring& cmd)
{
    SYSTEMTIME st; GetLocalTime(&st);
    wchar_t ts[64];
    wsprintfW(ts, L"%04u-%02u-%02u %02u:%02u:%02u  ",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

    std::wstring line = std::wstring(ts) + cmd + L"\r\n";
    std::string  u8 = ToUTF8(line);

    std::lock_guard<std::mutex> lk(g_storeMutex);
    std::ofstream ofs(GetHistoryFile(), std::ios::binary | std::ios::app);
    ofs.write(u8.data(), (std::streamsize)u8.size());
}

// ------------------------------
// ComboBox helpers
// ------------------------------
static std::wstring GetComboText(HWND hCombo)
{
    int len = (int)SendMessageW(hCombo, WM_GETTEXTLENGTH, 0, 0);
    std::wstring s(len, L'\0');
    SendMessageW(hCombo, WM_GETTEXT, len + 1, (LPARAM)s.data());
    return s;
}
static void SetComboText(HWND hCombo, const std::wstring& s)
{
    SendMessageW(hCombo, WM_SETTEXT, 0, (LPARAM)s.c_str());
}

// IFileDialog helpers
static bool PickFolder(HWND owner, std::wstring& outPath)
{
    IFileDialog* pfd = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
    if (FAILED(hr)) return false;
    DWORD opts = 0; pfd->GetOptions(&opts);
    pfd->SetOptions(opts | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
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
static bool PickFile(HWND owner, const COMDLG_FILTERSPEC* spec, UINT nSpec, std::wstring& outPath)
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

// Count files recursively
static uint64_t CountFiles(const fs::path& root)
{
    uint64_t cnt = 0;
    if (!fs::exists(root)) return 0;
    for (auto& p : fs::recursive_directory_iterator(root, fs::directory_options::skip_permission_denied)) {
        if (p.is_regular_file()) cnt++;
    }
    return cnt;
}
static bool EnsureDir(const fs::path& p)
{
    std::error_code ec;
    if (fs::exists(p, ec)) return true;
    return fs::create_directories(p, ec);
}

// Copy dir -> dir with progress
static bool CopyTreeWithProgress(const fs::path& src, const fs::path& dst)
{
    if (!fs::exists(src)) { 
        AppendLog(L"[COPY] Source not found: " + src.wstring()); 
        AppendLog(RET);
        return false; 
    }
    uint64_t total = CountFiles(src);
    if (!EnsureDir(dst)) { 
        AppendLog(L"[COPY] Cannot create: " + dst.wstring()); 
        AppendLog(RET);
        return false;
    }
    uint64_t done = 0;
    for (auto& entry : fs::recursive_directory_iterator(src, fs::directory_options::skip_permission_denied)) {
        
        //auto rel = fs::relative(entry.path(), src, std::error_code{});
        auto rel = fs::relative(entry.path(), src);

        auto out = dst / rel;
        if (entry.is_directory()) {
            EnsureDir(out);
        }
        else if (entry.is_regular_file()) {
            EnsureDir(out.parent_path());
            std::error_code ec;
            fs::copy_file(entry.path(), out, fs::copy_options::overwrite_existing, ec);
            done++;
            if (total > 0) SetProgress(int((done * 100) / total));
        }
    }
    return true;
}

// ------------------------------
// Split Dataset (based on your divfiles.cpp)  filecite：使用している分割ロジックは添付コードに準拠
// ------------------------------
static void SplitDataset(const fs::path& sourceRoot, const fs::path& destRoot,
    int trainPercent, double reductionFactor)
{
    fs::path srcImages = sourceRoot / "images";
    fs::path srcLabels = sourceRoot / "labels";

    fs::path trainImages = destRoot / "train" / "images";
    fs::path trainLabels = destRoot / "train" / "labels";
    fs::path validImages = destRoot / "valid" / "images";
    fs::path validLabels = destRoot / "valid" / "labels";

    EnsureDir(trainImages); EnsureDir(trainLabels);
    EnsureDir(validImages); EnsureDir(validLabels);

    // collect *.jpg / *.JPG
    std::vector<fs::path> jpgFiles;
    for (auto& e : fs::directory_iterator(srcImages)) {
        if (!e.is_regular_file()) continue;
        auto ext = e.path().extension().wstring();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
        if (ext == L".jpg") jpgFiles.push_back(e.path());
    }

    // shuffle
    std::random_device rd; std::mt19937 g(rd());
    std::shuffle(jpgFiles.begin(), jpgFiles.end(), g);

    // reduction
    if (reductionFactor < 0.001) reductionFactor = 0.001;
    if (reductionFactor > 1.0) reductionFactor = 1.0;
    size_t total = jpgFiles.size();
    size_t used = (size_t)std::ceil(total * reductionFactor);
    if (used < jpgFiles.size()) jpgFiles.resize(used);

    // train count
    if (trainPercent < 1) trainPercent = 1;
    if (trainPercent > 99) trainPercent = 99;
    size_t nTrain = (size_t)std::ceil(used * (trainPercent / 100.0));

    // copy train
    for (size_t i = 0; i < nTrain && i < jpgFiles.size(); ++i) {
        const auto& img = jpgFiles[i];
        fs::copy_file(img, trainImages / img.filename(), fs::copy_options::overwrite_existing);
        fs::path lblSrc = srcLabels / (img.stem().wstring() + L".txt");
        if (fs::exists(lblSrc)) {
            fs::copy_file(lblSrc, trainLabels / lblSrc.filename(), fs::copy_options::overwrite_existing);
        }
    }
    // copy valid (rest)
    for (size_t i = nTrain; i < jpgFiles.size(); ++i) {
        const auto& img = jpgFiles[i];
        fs::copy_file(img, validImages / img.filename(), fs::copy_options::overwrite_existing);
        fs::path lblSrc = srcLabels / (img.stem().wstring() + L".txt");
        if (fs::exists(lblSrc)) {
            fs::copy_file(lblSrc, validLabels / lblSrc.filename(), fs::copy_options::overwrite_existing);
        }
    }
    AppendLog(L"[SPLIT] Done. total=" + std::to_wstring(total) +
        L" used=" + std::to_wstring(used) +
        L" train=" + std::to_wstring(nTrain) +
        L" valid=" + std::to_wstring(used - nTrain));
}

// ------------------------------
// Process runner with output capture
// ------------------------------
static HANDLE LaunchWithCapture(const std::wstring& cmdLineFull)
{
    SECURITY_ATTRIBUTES sa{ sizeof(sa), nullptr, TRUE };
    HANDLE hRead = 0, hWrite = 0;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return nullptr;
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si{}; si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

    PROCESS_INFORMATION pi{};
    std::wstring cmd = L"cmd.exe /C " + cmdLineFull;

    BOOL ok = CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, TRUE,
        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    CloseHandle(hWrite);

    if (!ok) {
        CloseHandle(hRead);
        AppendLog(L"[EXEC] Failed to start: " + cmdLineFull);
        AppendLog(RET);
        return nullptr;
    }

    // reader thread
    std::thread([hRead, piH = pi.hProcess]() {
        char buf[4096];
        DWORD n = 0;
        while (ReadFile(hRead, buf, sizeof(buf) - 1, &n, nullptr) && n > 0) {
            buf[n] = 0;
            // Convert to UTF-16 (best-effort)
            int wlen = MultiByteToWideChar(CP_UTF8, 0, buf, n, nullptr, 0);
            std::wstring ws;
            if (wlen > 0) {
                ws.resize(wlen);
                MultiByteToWideChar(CP_UTF8, 0, buf, n, ws.data(), wlen);
            }
            else {
                int wlen2 = MultiByteToWideChar(CP_ACP, 0, buf, n, nullptr, 0);
                ws.resize(wlen2);
                MultiByteToWideChar(CP_ACP, 0, buf, n, ws.data(), wlen2);
            }
            AppendLog(ws);
			//LogAppendANSI(ws); // ANSIカラーコードを含む場合はここで処理
        }
        CloseHandle(hRead);
        WaitForSingleObject(piH, INFINITE);
        DWORD ec = 0; GetExitCodeProcess(piH, &ec);
        AppendLog(L"[EXEC] Exit code: " + std::to_wstring(ec));
        AppendLog(RET);
        }).detach();

    CloseHandle(pi.hThread);
    g_hChildProc.store(pi.hProcess);
    return pi.hProcess;
}
static void StopChild()
{
    HANDLE h = g_hChildProc.exchange(nullptr);
    if (h) {
        TerminateProcess(h, 1);
        CloseHandle(h);
        AppendLog(L"[EXEC] Terminated.");
        AppendLog(RET);
    }
}

// ------------------------------
// Helpers for building command
// ------------------------------
static std::wstring Quote(const std::wstring& s)
{
    if (s.empty()) return L"";
    if (s.find(L' ') != std::wstring::npos || s.find(L'&') != std::wstring::npos)
        return L"\"" + s + L"\"";
    return s;
}
static std::wstring GetText(HWND hDlg, int id)
{
    wchar_t tmp[4096]; GetDlgItemTextW(hDlg, id, tmp, 4096);
    return tmp;
}

// ------------------------------
// Actions
// ------------------------------
static void DoCopyToTemp()
{
    std::wstring img = GetText(g_hDlg, IDC_COMBO_IMG);
    std::wstring lab = GetText(g_hDlg, IDC_COMBO_LABEL);
    std::wstring tmp = GetText(g_hDlg, IDC_COMBO_TEMP);
    if (img.empty() || lab.empty() || tmp.empty()) 
    { 
        AppendLog(L"[COPY] Missing path."); 
        AppendLog(RET);
        return;
    }

    fs::path dstImages = fs::path(tmp) / "source" / "images";
    fs::path dstLabels = fs::path(tmp) / "source" / "labels";

    // Clean previous
    try {
        if (fs::exists(dstImages)) fs::remove_all(dstImages);
        if (fs::exists(dstLabels)) fs::remove_all(dstLabels);
    }
    catch (...) {}

    ResetProgress();
    AppendLog(L"[COPY] images -> " + dstImages.wstring());
    AppendLog(RET);
    if (!CopyTreeWithProgress(img, dstImages)) 
        return;
    AppendLog(L"[COPY] labels -> " + dstLabels.wstring());
    AppendLog(RET);
    if (!CopyTreeWithProgress(lab, dstLabels))
        return;
    SetProgress(100);
    AppendLog(L"[COPY] Completed.");
    AppendLog(RET);
    SaveMRU(L"Image Data", img);
    SaveMRU(L"Label Data", lab);
    SaveMRU(L"Temp Dir", tmp);
}
static void DoSplit()
{
    std::wstring tmp = GetText(g_hDlg, IDC_COMBO_TEMP);
    int trainPct = _wtoi(GetText(g_hDlg, IDC_EDIT_TRAINPCT).c_str());
    double reduc = _wtof(GetText(g_hDlg, IDC_EDIT_REDUCTION).c_str());
    if (tmp.empty() || trainPct <= 0) { 
        AppendLog(L"[SPLIT] Missing temp or train%"); 
        AppendLog(RET);
        return;
    }

    fs::path src = fs::path(tmp) / "source";
    fs::path dst = fs::path(tmp) / "dataset";
    try { if (fs::exists(dst)) fs::remove_all(dst); }
    catch (...) {}
    ResetProgress();
    AppendLog(L"[SPLIT] source=" + src.wstring() + L"   dest=" + dst.wstring());
    AppendLog(RET);
    SplitDataset(src, dst, trainPct, reduc);   // based on your divfiles.cpp  :contentReference[oaicite:2]{index=2}
    SetProgress(100);
}

//////////////////////////////////////////////////////////////////////////////////////
// 
// Resume checkpoint file picker
//
//////////////////////////////////////////////////////////////////////////////////////
static bool OpenResumeCheckpointFile(HWND hParent, std::wstring& outPath)
{
    wchar_t fileBuf[MAX_PATH] = L"";
    OPENFILENAMEW ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hParent;
    ofn.lpstrFilter = L"PyTorch Weights (*.pt;*.pth)\0*.pt;*.pth\0All Files (*.*)\0*.*\0\0";
    ofn.lpstrFile = fileBuf;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    ofn.lpstrDefExt = L"pt";

    if (GetOpenFileNameW(&ofn))
    {
        outPath = fileBuf;
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////////////////
// 
// Add safe.directory to git config
//
//////////////////////////////////////////////////////////////////////////////////////

static void AddSafeDirectory(const std::wstring& dir)
{
    // git config --global --add safe.directory "dir"
    std::wstring cmd = L"git config --global --add safe.directory \"" + dir + L"\"";

    // 非同期に実行（コンソールは出さない）
    STARTUPINFOW si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);
	AppendLog(L"[GIT] " + cmd);

    if (CreateProcessW(
        NULL,
        cmd.data(),   // コマンドライン（可変なので直接渡す）
        NULL, NULL,
        FALSE,
        CREATE_NO_WINDOW,
        NULL, NULL,
        &si, &pi))
    {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        //MessageBoxW(NULL, L"safe.directory に追加しました。", L"Git設定", MB_OK | MB_ICONINFORMATION);
		AppendLog(L"[GIT] safe.directory に追加しました。");
    }
    else
    {
        //MessageBoxW(NULL, L"git の実行に失敗しました。PATH設定を確認してください。", L"Git設定", MB_OK | MB_ICONERROR);
		AppendLog(L"[GIT] git の実行に失敗しました。PATH設定を確認してください。");
    }
}
//////////////////////////////////////////////////////////////////////////////////////
// Get text from a control
//////////////////////////////////////////////////////////////////////////////////////
class TrainParams {
    std::wstring backend;  // "v5" or "v8" or "11"
    std::wstring workdir;
    std::wstring trainpy;
    std::wstring datayaml;
    std::wstring hypyaml;
    std::wstring cfgyaml;
    std::wstring weights;
    std::wstring python;
    std::wstring activate;
    std::wstring resume; 
    std::wstring epochs;
    std::wstring patience;
    std::wstring batch;
    std::wstring imgsz;
    std::wstring device;
    std::wstring _NAME;
    std::wstring project;

	int chkResume = 0; // 0 or 1
	int chkCache = 0; // 0 or 1
	int chkUseHyp = 0; // 0 or 1
    int exist_ok = 0; // 0 or 1

    // v8 only
    std::wstring task;     // detect/segment/pose/classify

public:
    TrainParams() : backend(L"v8"), workdir(L""), trainpy(L"train.py"), datayaml(L""), hypyaml(L""), cfgyaml(L""), 
        weights(L""), python(L"python"), activate(L""), resume(L""), epochs(L"300"), patience(L"50"), batch(L"16"), imgsz(L"640"),
		device(L"0"), _NAME(L""), project(L"runs/train"), task(L"detect")
    {
        chkResume = 0;
		chkCache = 0;
        chkUseHyp = 0;
		exist_ok = 0; 
    }

    //メソッド
    int ReadControls(HWND hDlg);
    void DoTrain();
    int SaveCurrentSettingsToIni(HWND hDlg);

};

int TrainParams::ReadControls(HWND hDlg)
{
    workdir = GetText(hDlg, IDC_COMBO_WORKDIR);
    trainpy = GetText(hDlg, IDC_COMBO_TRAINPY);
    datayaml = GetText(hDlg, IDC_COMBO_YAML);
    hypyaml = GetText(hDlg, IDC_COMBO_HYP);
    cfgyaml = GetText(hDlg, IDC_COMBO_CFG);
    weights = GetText(hDlg, IDC_COMBO_WEIGHTS);
    python  = GetText(hDlg, IDC_COMBO_PYTHON);
    activate = GetText(hDlg, IDC_COMBO_ACTIVATE);
    resume  = GetText(hDlg, IDC_CMB_RESUME);
    epochs  = GetText(hDlg, IDC_COMBO_EPOCHS);
    patience = GetText(hDlg, IDC_CMB_PATIENCE);
    batch   = GetText(hDlg, IDC_COMBO_BATCHSIZE);
    imgsz   = GetText(hDlg, IDC_COMBO_IMGSZ);
    device  = GetText(hDlg, IDC_COMBO_DEVICE);
    _NAME   = GetText(hDlg, IDC_COMBO_NAME);
    project = GetText(hDlg, IDC_EDIT_PROJECT);

    //チェックボックスの状態を取得
    chkResume = IsDlgButtonChecked(hDlg, IDC_CHECK_RESUME);
    chkCache = IsDlgButtonChecked(hDlg, IDC_CHK_CACHE);
    chkUseHyp = IsDlgButtonChecked(hDlg, IDC_CHK_USEHYPERPARAM);

    // v8 only
    task = L"detect"; // default
    if (IsDlgButtonChecked(hDlg, IDC_RADIO_SEGMENT) == BST_CHECKED) {
        task = L"segment";
    }
    else if (IsDlgButtonChecked(hDlg, IDC_RADIO_POSE) == BST_CHECKED) {
        task = L"pose";
    }
    else if (IsDlgButtonChecked(hDlg, IDC_RADIO_CLASSIFY) == BST_CHECKED) {
        task = L"classify";
    }
    return 0;
}

void TrainParams::DoTrain()
{
    if (python.empty()) python = L"python";

    if (workdir.empty() || trainpy.empty() || datayaml.empty()) {
        AppendLog(L"[TRAIN] workdir/train.py/data.yaml is required.");
        AppendLog(RET);
        return;
    }

    // Build command
    std::wstringstream ss;
    if (!activate.empty()) {
        ss << L"call activate " << Quote(activate) << L" && ";
    }
    ss << L"cd /d " << Quote(workdir) << L" && ";
    ss << Quote(python) << L" " << Quote(trainpy)
        << L" --data " << Quote(datayaml);

    if (!epochs.empty()) ss << L" --epochs " << epochs;

    if (!patience.empty()) ss << L" --patience " << patience;

    if (chkResume == BST_CHECKED)
    {
        if (resume.empty())
            ss << L" --resume ";
        else
            ss << L" --resume " << Quote(resume);
    }

    if (!batch.empty()) ss << L" --batch " << batch;
    if (!imgsz.empty()) ss << L" --img " << imgsz;
    if (!device.empty()) ss << L" --device " << device;

    if (chkUseHyp == BST_CHECKED)
    {
        if (hypyaml.empty())
            ss << L" --hyp ";
        else
            ss << L" --hyp " << Quote(hypyaml);
    }

    if (!cfgyaml.empty()) ss << L" --cfg " << Quote(cfgyaml);
    if (!weights.empty()) ss << L" --weights " << Quote(weights);
    if (chkCache == BST_CHECKED)
        ss << L" --cache disk";

    if (!_NAME.empty()) ss << L" --name " << Quote(_NAME);

    if (!project.empty()) ss << L" --project " << Quote(project);

    std::wstring command = ss.str();
    AppendLog(L"[TRAIN] " + command);
    AppendLog(RET);

    //ここでコマンドを実行
    LaunchWithCapture(command);

    // ここでコマンド履歴を追記（無制限）
    AppendCmdHistory(command);

    // MRU 保存（各セクション256件まで）
    SaveCurrentSettingsToIni(nullptr);

    //SaveMRU(L"WorkDir", workdir);
    //SaveMRU(L"train.py", trainpy);
    //SaveMRU(L"data.yaml", datayaml);
    //SaveMRU(L"hyp.yaml", hypyaml);
    //SaveMRU(L"cfg.yaml", cfgyaml);
    //SaveMRU(L"weights.pt", weights);
    //SaveMRU(L"Python.exe", python);
    //SaveMRU(L"Activate.bat", activate);
    //SaveMRU(L"resume", resume);
}

int TrainParams::SaveCurrentSettingsToIni(HWND hDlg)
{
	// 共有データ（画像／ラベル）とテンポラリディレクトリの設定を保存
	
    // hDlgが有効なら各コントロールから値を取得して保存
	if (hDlg != nullptr)
    {
        SaveMRU(L"Image Data", GetText(hDlg, IDC_COMBO_IMG));
        SaveMRU(L"Label Data", GetText(hDlg, IDC_COMBO_LABEL));
        SaveMRU(L"Temp Dir", GetText(hDlg, IDC_COMBO_TEMP));

        // 分割パラメータ
        SaveMRU(L"TrainPercent", GetText(hDlg, IDC_EDIT_TRAINPCT));  // 例: "80"
        SaveMRU(L"Reduction", GetText(hDlg, IDC_EDIT_REDUCTION)); // 例: "1.0"

        // 実行ファイル／ワークスペース
        SaveMRU(L"WorkDir", GetText(hDlg, IDC_COMBO_WORKDIR));
        SaveMRU(L"train.py", GetText(hDlg, IDC_COMBO_TRAINPY));
        SaveMRU(L"data.yaml", GetText(hDlg, IDC_COMBO_YAML));
        SaveMRU(L"hyp.yaml", GetText(hDlg, IDC_COMBO_HYP));
        SaveMRU(L"cfg.yaml", GetText(hDlg, IDC_COMBO_CFG));
        SaveMRU(L"weights.pt", GetText(hDlg, IDC_COMBO_WEIGHTS));
        SaveMRU(L"Python.exe", GetText(hDlg, IDC_COMBO_PYTHON));
        SaveMRU(L"Environment", GetText(hDlg, IDC_COMBO_ACTIVATE));
        SaveMRU(L"Name", GetText(hDlg, IDC_COMBO_NAME));

        // 学習オプション
        SaveMRU(L"epochs", GetText(hDlg, IDC_COMBO_EPOCHS));
        SaveMRU(L"patience", GetText(hDlg, IDC_CMB_PATIENCE));
        SaveMRU(L"resume", GetText(hDlg, IDC_CMB_RESUME));
        SaveMRU(L"batch", GetText(hDlg, IDC_COMBO_BATCHSIZE));
        SaveMRU(L"imgsz", GetText(hDlg, IDC_COMBO_IMGSZ));
        SaveMRU(L"device", GetText(hDlg, IDC_COMBO_DEVICE));

        //SaveMRU(L"name", GetText(hDlg, IDC_COMBO_NAME));
        SaveMRU(L"project", GetText(hDlg, IDC_EDIT_PROJECT));

        // チェックボックスは "1" / "0" を保存
        chkCache = (IsDlgButtonChecked(hDlg, IDC_CHK_CACHE) == BST_CHECKED);
        SaveMRU(L"cache", chkCache ? L"1" : L"0");

        exist_ok = (IsDlgButtonChecked(hDlg, IDC_CHK_EXIST_OK) == BST_CHECKED);
        SaveMRU(L"exist_ok", exist_ok ? L"1" : L"0");

        chkResume = (IsDlgButtonChecked(hDlg, IDC_CHECK_RESUME) == BST_CHECKED);
        SaveMRU(L"resume_chk", chkResume ? L"1" : L"0");

        chkUseHyp = (IsDlgButtonChecked(hDlg, IDC_CHK_USEHYPERPARAM) == BST_CHECKED);
        SaveMRU(L"hyper_chk", chkUseHyp ? L"1" : L"0");
    }
    else // hDlgが無効な場合は、メモリ上の共有データの設定を保存
    {
        SaveMRU(L"Image Data", this->workdir); // 例: "C:\path\to\images"
        SaveMRU(L"Label Data", this->datayaml); // 例: "C:\path\to\labels"
        SaveMRU(L"Temp Dir", this->weights); // 例: "C:\path\to\temp"

        // 分割パラメータ
        SaveMRU(L"TrainPercent", this->epochs);  // 例: "80"
        SaveMRU(L"Reduction", this->patience); // 例: "1.0"

        // 実行ファイル／ワークスペース
        SaveMRU(L"WorkDir", this->workdir);
        SaveMRU(L"train.py", this->trainpy);
        SaveMRU(L"data.yaml", this->datayaml);
        SaveMRU(L"hyp.yaml", this->hypyaml);
        SaveMRU(L"cfg.yaml", this->cfgyaml);
        SaveMRU(L"weights.pt", this->weights);
        SaveMRU(L"Python.exe", this->python);
        SaveMRU(L"Environment", this->activate);
        SaveMRU(L"Name", this->_NAME);

        // 学習オプション
        SaveMRU(L"epochs", this->epochs);
        SaveMRU(L"patience", this->patience);
        SaveMRU(L"resume", this->resume);
        SaveMRU(L"batch", this->batch);
        SaveMRU(L"imgsz", this->imgsz);
		SaveMRU(L"device", this->device);

        // task
        SaveMRU(L"task", this->task); //taskは文字列 detect/segment/pose/classify

		// Checkboxの状態を保存 メモリから
        SaveMRU(L"cache", this->chkCache ? L"1" : L"0");
        SaveMRU(L"exist_ok", this->exist_ok ? L"1" : L"0");
        SaveMRU(L"resume_chk", this->chkResume ? L"1" : L"0");
        SaveMRU(L"hyper_chk", this->chkUseHyp ? L"1" : L"0");
    }
    return 0;
}


//////////////////////////////////////////////////////////////////////////////////////
// 
// Build train command and launch
//
//////////////////////////////////////////////////////////////////////////////////////
static void DoTrain(HWND hDlg)
{
	TrainParams _params;
    _params.ReadControls(hDlg); // コントロールから値を読み取る
    // ここでコマンドを実行
    _params.DoTrain();
    // 設定をINIに保存
	_params.SaveCurrentSettingsToIni(nullptr); // hDlgが有効ならコントロールから値を保存 無効ならメモリ上の共有データを保存
}

static void DoTrain_old()
{
	//コントロールからの値取得
    std::wstring workdir = GetText(g_hDlg, IDC_COMBO_WORKDIR);
    std::wstring trainpy = GetText(g_hDlg, IDC_COMBO_TRAINPY);
    std::wstring datayaml = GetText(g_hDlg, IDC_COMBO_YAML);
    std::wstring hypyaml = GetText(g_hDlg, IDC_COMBO_HYP);
    std::wstring cfgyaml = GetText(g_hDlg, IDC_COMBO_CFG);
    std::wstring weights = GetText(g_hDlg, IDC_COMBO_WEIGHTS);
    std::wstring python = GetText(g_hDlg, IDC_COMBO_PYTHON);
    std::wstring activate = GetText(g_hDlg, IDC_COMBO_ACTIVATE);
    std::wstring resume = GetText(g_hDlg, IDC_CMB_RESUME);
    std::wstring epochs = GetText(g_hDlg, IDC_COMBO_EPOCHS);
    std::wstring patience = GetText(g_hDlg, IDC_CMB_PATIENCE);
    std::wstring batch = GetText(g_hDlg, IDC_COMBO_BATCHSIZE);
    std::wstring imgsz = GetText(g_hDlg, IDC_COMBO_IMGSZ);
    std::wstring device = GetText(g_hDlg, IDC_COMBO_DEVICE);
    std::wstring _NAME = GetText(g_hDlg, IDC_COMBO_NAME);
    std::wstring project = GetText(g_hDlg, IDC_EDIT_PROJECT);

	//チェックボックスの状態を取得
    int chkResume = IsDlgButtonChecked(g_hDlg, IDC_CHECK_RESUME);
    int chkCache = IsDlgButtonChecked(g_hDlg, IDC_CHK_CACHE);
	int chkUseHyp = IsDlgButtonChecked(g_hDlg, IDC_CHK_USEHYPERPARAM);

    if (python.empty()) python = L"python";

    if (workdir.empty() || trainpy.empty() || datayaml.empty()) {
        AppendLog(L"[TRAIN] workdir/train.py/data.yaml is required.");
        AppendLog(RET);
        return;
    }

    // Build command
    std::wstringstream ss;
    if (!activate.empty()) {
        ss << L"call activate " << Quote(activate) << L" && ";
    }
    ss << L"cd /d " << Quote(workdir) << L" && ";
    ss << Quote(python) << L" " << Quote(trainpy)
        << L" --data " << Quote(datayaml);

    if (!epochs.empty()) ss << L" --epochs " << epochs;

    if (!patience.empty()) ss << L" --patience " << patience;

    //if (IsDlgButtonChecked(g_hDlg, IDC_CHECK_RESUME) == BST_CHECKED)
    if (chkResume == BST_CHECKED)
    {
        if (resume.empty())
            ss << L" --resume ";
        else
            ss << L" --resume " << Quote(resume);
    }

    if (!batch.empty()) ss << L" --batch " << batch;
    if (!imgsz.empty()) ss << L" --img " << imgsz;
    if (!device.empty()) ss << L" --device " << device;

    //hyper parameters
    //if (!hypyaml.empty()) ss << L" --hyp " << Quote(hypyaml);
    //if (IsDlgButtonChecked(g_hDlg, IDC_CHK_USEHYPERPARAM) == BST_CHECKED)
    if (chkUseHyp == BST_CHECKED)
    {
        //std::wstring _hyper = GetText(g_hDlg, IDC_COMBO_HYP);
        if (hypyaml.empty())
            ss << L" --hyp ";
        else
            ss << L" --hyp " << Quote(hypyaml);
    }

    if (!cfgyaml.empty()) ss << L" --cfg " << Quote(cfgyaml);
    if (!weights.empty()) ss << L" --weights " << Quote(weights);
    //if (IsDlgButtonChecked(g_hDlg, IDC_CHK_CACHE) == BST_CHECKED)
    if (chkCache == BST_CHECKED)
            ss << L" --cache disk";

    if (!_NAME.empty()) ss << L" --name " << Quote(_NAME);

    if (!project.empty()) ss << L" --project " << Quote(project);

    std::wstring command = ss.str();
    AppendLog(L"[TRAIN] " + command);
    AppendLog(RET);

	//ここでコマンドを実行
    LaunchWithCapture(command);

    // ここでコマンド履歴を追記（無制限）
    AppendCmdHistory(command);

    // MRU 保存（各セクション256件まで）
    SaveMRU(L"WorkDir", workdir);
    SaveMRU(L"train.py", trainpy);
    SaveMRU(L"data.yaml", datayaml);
    SaveMRU(L"hyp.yaml", hypyaml);
    SaveMRU(L"cfg.yaml", cfgyaml);
    SaveMRU(L"weights.pt", weights);
    SaveMRU(L"Python.exe", python);
    SaveMRU(L"Activate.bat", activate);
    SaveMRU(L"resume", resume);
}

static void SaveCurrentSettingsToIni(HWND hDlg)
{
    // 共有データ（画像／ラベル）とテンポラリ
    SaveMRU(L"Image Data", GetText(hDlg, IDC_COMBO_IMG));
    SaveMRU(L"Label Data", GetText(hDlg, IDC_COMBO_LABEL));
    SaveMRU(L"Temp Dir", GetText(hDlg, IDC_COMBO_TEMP));

    // 分割パラメータ
    SaveMRU(L"TrainPercent", GetText(hDlg, IDC_EDIT_TRAINPCT));  // 例: "80"
    SaveMRU(L"Reduction", GetText(hDlg, IDC_EDIT_REDUCTION)); // 例: "1.0"

    // 実行ファイル／ワークスペース
    SaveMRU(L"WorkDir", GetText(hDlg, IDC_COMBO_WORKDIR));
    SaveMRU(L"train.py", GetText(hDlg, IDC_COMBO_TRAINPY));
    SaveMRU(L"data.yaml", GetText(hDlg, IDC_COMBO_YAML));
    SaveMRU(L"hyp.yaml", GetText(hDlg, IDC_COMBO_HYP));
    SaveMRU(L"cfg.yaml", GetText(hDlg, IDC_COMBO_CFG));
    SaveMRU(L"weights.pt", GetText(hDlg, IDC_COMBO_WEIGHTS));
    SaveMRU(L"Python.exe", GetText(hDlg, IDC_COMBO_PYTHON));
    SaveMRU(L"Environment", GetText(hDlg, IDC_COMBO_ACTIVATE));

    SaveMRU(L"Name", GetText(hDlg, IDC_COMBO_NAME));

    // 学習オプション
    SaveMRU(L"epochs",      GetText(hDlg,   IDC_COMBO_EPOCHS));
    SaveMRU(L"patience",    GetText(hDlg,   IDC_CMB_PATIENCE));
    SaveMRU(L"resume",      GetText(hDlg,   IDC_CMB_RESUME));
    SaveMRU(L"batch",       GetText(hDlg,   IDC_COMBO_BATCHSIZE));
    SaveMRU(L"imgsz",       GetText(hDlg,   IDC_COMBO_IMGSZ));
    SaveMRU(L"device",      GetText(hDlg,   IDC_COMBO_DEVICE));
    
    //SaveMRU(L"name", GetText(hDlg, IDC_COMBO_NAME));
    SaveMRU(L"project", GetText(hDlg, IDC_EDIT_PROJECT));

    // チェックボックスは "1" / "0" を保存
    const bool cacheOn = (IsDlgButtonChecked(hDlg, IDC_CHK_CACHE) == BST_CHECKED);
    SaveMRU(L"cache", cacheOn ? L"1" : L"0");

    const bool exist_ok = (IsDlgButtonChecked(hDlg, IDC_CHK_EXIST_OK) == BST_CHECKED);
    SaveMRU(L"exist_ok", exist_ok ? L"1" : L"0");

    const bool resume_chk = (IsDlgButtonChecked(hDlg, IDC_CHECK_RESUME) == BST_CHECKED);
    SaveMRU(L"resume_chk", resume_chk ? L"1" : L"0");

    const bool hyper_chk = (IsDlgButtonChecked(hDlg, IDC_CHK_USEHYPERPARAM) == BST_CHECKED);
    SaveMRU(L"hyper_chk", hyper_chk ? L"1" : L"0");

}

// ------------------------------
// Dialog 初期化
// ------------------------------
// 先頭アイテムを選択して"表示"まで行う（CBS_DROPDOWN/CBS_SIMPLE/CBS_DROPDOWNLIST 全対応）
static void ShowFirstComboItem(HWND hCombo)
{
    const int count = (int)SendMessageW(hCombo, CB_GETCOUNT, 0, 0);
    if (count <= 0) return;

    // まず選択（CBS_DROPDOWNLIST ならこれだけで表示される）
    SendMessageW(hCombo, CB_SETCURSEL, 0, 0);

    // CBS_DROPDOWN/CBS_SIMPLE では編集部に文字が出ない場合があるので明示的にセット
    LONG_PTR style = GetWindowLongPtrW(hCombo, GWL_STYLE);
    if (style & (CBS_DROPDOWN | CBS_SIMPLE))
    {
        LRESULT len = SendMessageW(hCombo, CB_GETLBTEXTLEN, 0, 0);
        if (len == CB_ERR || len < 0) return;
        std::vector<wchar_t> buf((size_t)len + 1);
        if (SendMessageW(hCombo, CB_GETLBTEXT, 0, (LPARAM)buf.data()) != CB_ERR) {
            SetWindowTextW(hCombo, buf.data());           // 編集部の表示を更新
            SendMessageW(hCombo, CB_SETEDITSEL, 0, 0);    // キャレット選択を解除（見た目の整え）
        }
    }
}

// リストボックス（LB_系）用：先頭を選択＆先頭をスクロールの最上段に
static void ShowFirstListItem(HWND hList)
{
    const int count = (int)SendMessageW(hList, LB_GETCOUNT, 0, 0);
    if (count <= 0) return;
    SendMessageW(hList, LB_SETCURSEL, 0, 0);
    SendMessageW(hList, LB_SETTOPINDEX, 0, 0);
}


// ------------------------------
static void InitDialog(HWND hDlg)
{
    g_hDlg = hDlg;
    
    SendDlgItemMessageW(hDlg, IDC_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    ResetProgress();

	// 設定ファイルから読み込んでコンボにセット
    LoadMRUToCombo(GetDlgItem(hDlg, IDC_COMBO_IMG), L"Image Data");
    LoadMRUToCombo(GetDlgItem(hDlg, IDC_COMBO_LABEL), L"Label Data");
    LoadMRUToCombo(GetDlgItem(hDlg, IDC_COMBO_TEMP), L"Temp Dir");
    LoadMRUToCombo(GetDlgItem(hDlg, IDC_COMBO_WORKDIR), L"WorkDir");
    LoadMRUToCombo(GetDlgItem(hDlg, IDC_COMBO_TRAINPY), L"train.py");
    LoadMRUToCombo(GetDlgItem(hDlg, IDC_COMBO_YAML), L"data.yaml");
    LoadMRUToCombo(GetDlgItem(hDlg, IDC_COMBO_HYP), L"hyp.yaml");
    LoadMRUToCombo(GetDlgItem(hDlg, IDC_COMBO_CFG), L"cfg.yaml");
    LoadMRUToCombo(GetDlgItem(hDlg, IDC_COMBO_WEIGHTS), L"weights.pt");
    LoadMRUToCombo(GetDlgItem(hDlg, IDC_COMBO_PYTHON), L"Python.exe");
    LoadMRUToCombo(GetDlgItem(hDlg, IDC_COMBO_ACTIVATE), L"Environment");

    LoadMRUToCombo(GetDlgItem(hDlg, IDC_COMBO_NAME),        L"Name");
    LoadMRUToCombo(GetDlgItem(hDlg, IDC_COMBO_EPOCHS),      L"epochs");
    LoadMRUToCombo(GetDlgItem(hDlg, IDC_COMBO_BATCHSIZE),   L"batch");
    LoadMRUToCombo(GetDlgItem(hDlg, IDC_COMBO_IMGSZ),       L"imgsz");
    LoadMRUToCombo(GetDlgItem(hDlg, IDC_COMBO_DEVICE),      L"device");

    LoadMRUToCombo(GetDlgItem(hDlg, IDC_CMB_PATIENCE),      L"patience");
    LoadMRUToCombo(GetDlgItem(hDlg, IDC_CMB_RESUME),        L"resume");

    // ★ 先頭を表示（空なら何もしない）
    ShowFirstComboItem(GetDlgItem(hDlg, IDC_COMBO_IMG));
    ShowFirstComboItem(GetDlgItem(hDlg, IDC_COMBO_LABEL));
    ShowFirstComboItem(GetDlgItem(hDlg, IDC_COMBO_TEMP));
    ShowFirstComboItem(GetDlgItem(hDlg, IDC_COMBO_WORKDIR));
    ShowFirstComboItem(GetDlgItem(hDlg, IDC_COMBO_TRAINPY));
    ShowFirstComboItem(GetDlgItem(hDlg, IDC_COMBO_YAML));
    ShowFirstComboItem(GetDlgItem(hDlg, IDC_COMBO_HYP));
    ShowFirstComboItem(GetDlgItem(hDlg, IDC_COMBO_CFG));
    ShowFirstComboItem(GetDlgItem(hDlg, IDC_COMBO_WEIGHTS));
    ShowFirstComboItem(GetDlgItem(hDlg, IDC_COMBO_PYTHON));
    ShowFirstComboItem(GetDlgItem(hDlg, IDC_COMBO_ACTIVATE));

	ShowFirstComboItem(GetDlgItem(hDlg, IDC_COMBO_NAME));
	ShowFirstComboItem(GetDlgItem(hDlg, IDC_COMBO_EPOCHS));
	ShowFirstComboItem(GetDlgItem(hDlg, IDC_COMBO_BATCHSIZE));
	ShowFirstComboItem(GetDlgItem(hDlg, IDC_COMBO_IMGSZ));
	ShowFirstComboItem(GetDlgItem(hDlg, IDC_COMBO_DEVICE));

    ShowFirstComboItem(GetDlgItem(hDlg, IDC_CMB_PATIENCE));
    ShowFirstComboItem(GetDlgItem(hDlg, IDC_CMB_RESUME));

    // 規定値
    SetDlgItemTextW(hDlg, IDC_EDIT_TRAINPCT, L"80");
    SetDlgItemTextW(hDlg, IDC_EDIT_REDUCTION, L"1.0");

    CheckDlgButton(hDlg, IDC_CHK_CACHE,         LoadFlagFromIni(L"cache",       false) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHK_EXIST_OK,      LoadFlagFromIni(L"exist_ok",    false) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHECK_RESUME,      LoadFlagFromIni(L"resume_chk",  false) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHK_USEHYPERPARAM, LoadFlagFromIni(L"hyper_chk",   false) ? BST_CHECKED : BST_UNCHECKED);

    // ボタンのツールチップを設定
    INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_WIN95_CLASSES };
    InitCommonControlsEx(&icc);

    SetTootips(hDlg);
}

// 既定アプリで開く（“open” 動詞）
// 成功: true / 失敗: false（ログ出力あり）
static bool OpenFileWithDefaultEditor(const std::wstring& filePathRaw)
{
    std::wstring path = filePathRaw;
    //TrimInPlace(path);
    //UnquoteInPlace(path);
    if (path.empty()) {
        AppendLog(L"[OPEN] data.yaml path is empty.");
        AppendLog(RET);
        return false;
    }

    // 存在チェック（存在しなくても ShellExecute は投げられるが、ユーザに優しく）
    if (!fs::exists(path)) {
        AppendLog(L"[OPEN] File not found: " + path);
        AppendLog(RET);
        return false;
    }

    // 作業フォルダに親ディレクトリを指定（関連付けアプリが相対参照するケースに備え）
    fs::path parent = fs::path(path).parent_path();

    
    
    HINSTANCE h = ShellExecuteW(g_hDlg, L"open", path.c_str(), nullptr,parent.empty() ? nullptr : parent.wstring().c_str(),SW_SHOWNORMAL);



    auto code = reinterpret_cast<INT_PTR>(h);
    if (code <= 32) {
        // 代表的なエラー値を簡単に表示
        std::wstringstream ss;
        ss << L"[OPEN] ShellExecute failed (" << code << L") : " << path;
        AppendLog(RET);
        AppendLog(ss.str());
        return false;
    }
    AppendLog(L"[OPEN] Launched editor for: " + path);
    AppendLog(RET);
    return true;
}

// ------------------------------
// Dialog Procedure     
// ------------------------------
static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_INITDIALOG: 
        InitDialog(hDlg); 
        return TRUE;
    case WM_COMMAND:
    {
        //if (LOWORD(wParam) == IDC_STC_TEMP && HIWORD(wParam) == STN_CLICKED) {
        //    if (bTmpTipShow) HideTempTip(hDlg);
        //    else                ShowTempTipAtCursor(hDlg);
        //    return TRUE;
        //}
        switch (LOWORD(wParam))
        {
        case IDC_BTN_BROWSE_IMG: {
            std::wstring p; if (PickFolder(hDlg, p)) { SetComboText(GetDlgItem(hDlg, IDC_COMBO_IMG), p); }
        } break;
        case IDC_BTN_BROWSE_LABEL: {
            std::wstring p; if (PickFolder(hDlg, p)) { SetComboText(GetDlgItem(hDlg, IDC_COMBO_LABEL), p); }
        } break;
        case IDC_BTN_BROWSE_TEMP: {
            std::wstring p; if (PickFolder(hDlg, p)) { SetComboText(GetDlgItem(hDlg, IDC_COMBO_TEMP), p); }
        } break;
        case IDC_BTN_BROWSE_WORKDIR: {
            std::wstring p; if (PickFolder(hDlg, p)) { SetComboText(GetDlgItem(hDlg, IDC_COMBO_WORKDIR), p); }
        } break;
        case IDC_BTN_BROWSE_TRAINPY: {
            COMDLG_FILTERSPEC spec[] = { {L"Python (*.py)", L"*.py"} };
            std::wstring p; if (PickFile(hDlg, spec, 1, p)) { SetComboText(GetDlgItem(hDlg, IDC_COMBO_TRAINPY), p); }
        } break;
        case IDC_BTN_BROWSE_YAML: {
            COMDLG_FILTERSPEC spec[] = { {L"YAML (*.yaml;*.yml)", L"*.yaml;*.yml"} };
            std::wstring p; if (PickFile(hDlg, spec, 1, p)) { SetComboText(GetDlgItem(hDlg, IDC_COMBO_YAML), p); }
        } break;

        case IDC_BTN_EDIT_YAML: {
            std::wstring p = GetText(g_hDlg, IDC_COMBO_YAML);
            if (OpenFileWithDefaultEditor(p)) {
                // ついでに MRU の先頭へ
                SaveMRU(L"data.yaml", p);
            }
        }break;

        case IDC_BTN_BROWSE_HYP: {
            COMDLG_FILTERSPEC spec[] = { {L"YAML (*.yaml;*.yml)", L"*.yaml;*.yml"} };
            std::wstring p; if (PickFile(hDlg, spec, 1, p)) { SetComboText(GetDlgItem(hDlg, IDC_COMBO_HYP), p); }
        } break;
        case IDC_BTN_EDIT_HYP: {
            std::wstring p = GetText(g_hDlg, IDC_COMBO_HYP);
            if (OpenFileWithDefaultEditor(p)) {
                // ついでに MRU の先頭へ
                SaveMRU(L"hyp.yaml", p);
            }
        } break;

        case IDC_BTN_BROWSE_CFG: {
            COMDLG_FILTERSPEC spec[] = { {L"YAML (*.yaml;*.yml)", L"*.yaml;*.yml"} };
            std::wstring p; if (PickFile(hDlg, spec, 1, p)) { SetComboText(GetDlgItem(hDlg, IDC_COMBO_CFG), p); }
        } break;

        case IDC_BTN_EDIT_CFG: {
            std::wstring p = GetText(g_hDlg, IDC_COMBO_CFG);
            if (OpenFileWithDefaultEditor(p)) {
                // ついでに MRU の先頭へ
                SaveMRU(L"cfg.yaml", p);
            }
        } break;

        case IDC_BTN_BROWSE_WEIGHTS: {
            COMDLG_FILTERSPEC spec[] = { {L"PyTorch (*.pt)", L"*.pt"} };
            std::wstring p; if (PickFile(hDlg, spec, 1, p)) { SetComboText(GetDlgItem(hDlg, IDC_COMBO_WEIGHTS), p); }
        } break;

        case IDC_BTN_RESUME_BROWSE: {
            COMDLG_FILTERSPEC spec[] = { {L"PyTorch (*.pt)", L"*.pt"} };
            std::wstring p; if (PickFile(hDlg, spec, 1, p)) { SetComboText(GetDlgItem(hDlg, IDC_CMB_RESUME), p); }
        } break;

        // conda info --envsを実行して環境を確認する
        case IDC_BTN_VIEW_PYENV:
        {
            std::wstring cmd = L"conda info --envs";
            AppendLog(L"[ENV] " + cmd);
            AppendLog(RET);
            LaunchWithCapture(cmd);
		} break;

		//TempDirのディレクトリの下にあるディレクトリ、ファイル消去する。
        case IDC_BTN_CLEARTMP: {
            if (HIWORD(wParam) != BN_CLICKED) return TRUE; // ← これを追加

            std::wstring tempDir = GetText(hDlg, IDC_COMBO_TEMP);
            if (tempDir.empty()) {
                AppendLog(L"[TEMP] Temp directory is not set.");
                AppendLog(RET);
                return TRUE;
            }
            fs::path tempPath(tempDir);
            if (!fs::exists(tempPath)) {
                AppendLog(L"[TEMP] Temp directory does not exist: " + tempPath.wstring());
                AppendLog(RET);
                return TRUE;
            }
            //一応本当に消すか確認する
            int _ret = MessageBoxW(hDlg, L"Temp directory will be cleared. Continue?", L"Confirm", MB_OKCANCEL | MB_ICONWARNING);
            if (_ret == IDOK) {
                try {
                    fs::remove_all(tempPath);
                    AppendLog(L"[TEMP] Cleared temp directory: " + tempPath.wstring());
                }
                catch (const fs::filesystem_error& e) {
                    std::wstring _tmp = FromUTF8(e.what());
                    AppendLog(L"[TEMP] Error clearing temp directory: " + _tmp);
                }
            }
            else
            {
                AppendLog(L"[TEMP] Clear operation cancelled.");
                AppendLog(RET);
                return TRUE;
            }

            AppendLog(RET);
		} break;

        case IDC_BTN_BROWSE_PYTHON: {
            COMDLG_FILTERSPEC spec[] = { {L"Python executable (python.exe)", L"python.exe"} };
            std::wstring p; if (PickFile(hDlg, spec, 1, p)) { SetComboText(GetDlgItem(hDlg, IDC_COMBO_PYTHON), p); }
        } break;
        case IDC_BTN_BROWSE_ACTIVATE: {
            COMDLG_FILTERSPEC spec[] = { {L"Batch (*.bat;*.cmd)", L"*.bat;*.cmd"} };
            std::wstring p; if (PickFile(hDlg, spec, 1, p)) { SetComboText(GetDlgItem(hDlg, IDC_COMBO_ACTIVATE), p); }
        } break;
        case IDC_BTN_COPY: {
            std::thread([]() { DoCopyToTemp(); }).detach();
        }break;
        case IDC_BTN_SPLIT: {
            std::thread([]() { DoSplit(); }).detach();
        }break;
        case IDC_BTN_TRAIN: {
            //std::thread([]() { DoTrain(); }).detach();

            std::thread([hDlg]() { DoTrain(hDlg); }).detach();


        }break;
        case IDC_BTN_STOP: {
            StopChild();
        }break;
        case IDC_BTN_SAFE_DIR:
        {
            if (HIWORD(wParam) == BN_CLICKED)
            {
                // 例：あなたのGUIで「作業フォルダ」を保持している変数を使う
                // ここでは仮に g_WorkDir として説明します
                std::wstring workdir = GetText(g_hDlg, IDC_COMBO_WORKDIR);
                if (!workdir.empty()) {
                    AddSafeDirectory(workdir);
                }
                else {
                    MessageBoxW(hDlg, L"作業ディレクトリが未設定です。", L"Git設定", MB_OK | MB_ICONWARNING);
                }
                //return TRUE;
            }
        }
        break;

        default:
            break;
        }
        return false;
    }

	//case WM_DESTROY:
	//	g_hDlg = nullptr;
 //       SaveSettings(hDlg);
 //       PostQuitMessage(0);
	//	return TRUE;

    case WM_CLOSE:
		SaveCurrentSettingsToIni(hDlg);

        StopChild();
        EndDialog(hDlg, 0);
        return TRUE;
    }
    return FALSE;
}

// ------------------------------
// WinMain
// ------------------------------
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int)
{
    //g_hInst = hInstance;
    LoadLibraryW(L"Msftedit.dll"); // RICHEDIT50W 用

    INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_WIN95_CLASSES | ICC_PROGRESS_CLASS };
    InitCommonControlsEx(&icc);
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    DialogBoxParamW(hInstance, MAKEINTRESOURCEW(IDD_MAIN), nullptr, DlgProc, 0);
    CoUninitialize();
    return 0;
}
