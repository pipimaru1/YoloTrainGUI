#define _WIN32_WINNT 0x0601
#include "pch.h"

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Uuid.lib")
#pragma comment(lib, "Comctl32.lib")

#include "tools.hpp"
#include "resource.h"
//#include "resource_user.h"
#include "YoloTrainGUI.h"
#include "Tooltip.hpp"

namespace fs = std::filesystem;
static std::mutex g_storeMutex;
 
// ------------------------------
// Globals
// ------------------------------
HWND g_hDlg = nullptr;
//std::atomic<HANDLE> g_hChildProc(nullptr);
static std::atomic<HANDLE> g_hChildProc{ nullptr };
static HANDLE g_hJob = NULL;  // ← 追加

std::mutex g_logMutex;
std::wstring g_logBuffer;

const wchar_t *RET = L"\r\n";
static TOOLINFOW g_TipTI{};
static bool      bTmpTipShow = false;

//サイズ変更対応
static int  g_initCx = 0, g_initCy = 0;   // 初期クライアントサイズ
static RECT g_logInit = {};               // IDC_LOG の初期位置（クライアント座標）
static int  g_logLeft = 0, g_logTop = 0, g_logWidth = 0, g_logHeight = 0;

static void GetChildRectClient(HWND hParent, HWND hChild, RECT& rcOut)
{
    GetWindowRect(hChild, &rcOut);
    MapWindowPoints(nullptr, hParent, (POINT*)&rcOut, 2); // 画面座標→親のクライアント座標
}

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
L"./TempDir/train および./TempDir/valid にコピーされます。\r\n"
L"【注意】yolov8はRAMディスク使えません!!【注意】\r\n";

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
L"Yolovの作業ディレクトリを指定します。\r\n"
L"train.pyのあるディレクトリを指定してください。\r\n"
L"./yolov5や./yolov8, ./yolo11 等を指定します。\r\n";

static std::wstring strGitSafe =
L"Yolo作業ディレクトリに必要なファイルを\r\n"
L"gitでダウンロードすることがあります\r\n"
L"このボタンを押して、gitsafe に指定してください。\r\n";

static std::wstring strChkEnv =
L"Pythonの環境設定のリストを表示します。\r\n"
L"Condaに対応しています。\r\n";

static std::wstring strTipCfgYaml =
L"不要な時は空欄にしてください。\r\n"
L"ptファイルがあれば不要です。\r\n";

static std::wstring strTipAutoEditYamlBtn =
L"Tempフォルダをdata.yamlに適用します。\r\n";


void SetTootips(HWND hDlg)
{
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STC_TEMP, L"Tempolary Directory", strTipTempDir.c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STC_DATA_YAML, L"Tempolary Directory", strTipDataYaml.c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_EDIT_YAML, L"Tempolary Directory", strTipDataYaml.c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STC_TRAINPY, L"train.py", strTipTrainPy.c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STC_PYTHONEXE, L"train.py", strTipPythonExe.c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STC_WORKDIR, L"WorkDir", strTipWorkDir.c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_SAFE_DIR, L"Git Safe", strGitSafe.c_str());   
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BTN_VIEW_PYENV, L"Python Env", strChkEnv.c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_STC_CFGYAML, L"CFG Yaml", strTipCfgYaml.c_str());
    ttTmpDir.AddHoverTooltipForCtrl(hDlg, IDC_BUTTON_APPLY_YAML, L"CFG Yaml", strTipAutoEditYamlBtn.c_str());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// richieditboxの不具合対応
// 原因はほぼ確実に「別スレッドから RichEdit に SendMessage している」ことです。
// サイズ変更中（UIスレッドが WM_SIZE 等で占有）に、バックグラウンドの読取スレッドが 
// AppendLog→LogAppendANSI→SendMessage(EM_...) を投げると、UI スレッドと相互に待ち合って“詰まり”、
// 以降の追記が出なくなる（止まる／落ちる）ことがあります。
// ※ よくある誤解ですが、Win32 コントロールは“作成したスレッド＝UIスレッド”以外から触ってはいけません。
// ミューテックスはスレッドアフィニティ問題を解決しません。
////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ログを UI スレッドに投げる
// 文字列をヒープに積んで UI スレッドへ投げる
static void PostLogToUi(const std::wstring& s) {
    if (!g_hDlg || s.empty()) return;
    auto* payload = new std::wstring(s);        // 後で UI スレッドが delete
    PostMessageW(g_hDlg, WM_APP_LOGAPPEND, 0, reinterpret_cast<LPARAM>(payload));
}

// ------------------------------
// 文字コード変換＆保存先パスユーティリティ
// ------------------------------

//static 
std::string ToUTF8(const std::wstring& w) {
    if (w.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), s.data(), len, nullptr, nullptr);
    return s;
}
//static 
std::wstring FromUTF8(const std::string& s) {
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

std::wstring GetText(HWND hDlg, int id)
{
    wchar_t tmp[4096];
    GetDlgItemTextW(hDlg, id, tmp, 4096);
    return tmp;
}

static void SetProgress(int v)
{
    SendDlgItemMessageW(g_hDlg, IDC_PROGRESS, PBM_SETPOS, (WPARAM)v, 0);
}
static void ResetProgress()
{
    SendDlgItemMessageW(g_hDlg, IDC_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SetProgress(0);
}

//static 
std::vector<std::wstring> LoadMRU(const std::wstring& section)
{
    auto m = ReadIniColon();
    auto it = m.find(section);
    if (it == m.end()) return {};
    return it->second;
}

//static 
void SaveMRU(const std::wstring& section, const std::wstring& value, size_t maxItems)
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

//static 
void LoadMRUToCombo(HWND hCombo, const std::wstring& section)
{
    SendMessageW(hCombo, CB_RESETCONTENT, 0, 0);
    for (auto& s : LoadMRU(section)) {
        SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)s.c_str());
    }
}

static void LoadMRUToTextControl(HWND hEditText, const std::wstring& section)
{
    SendMessageW(hEditText, WM_SETTEXT, 0, (LPARAM)L"");
    auto v = LoadMRU(section);
    if (!v.empty()) {
        std::wstring s = v.front(); // 先頭要素を採用
        SendMessageW(hEditText, WM_SETTEXT, 0, (LPARAM)s.c_str());
    }
}

//単純に文字列を
//std::wstring LoadMRUToString(HWND hEditText, const std::wstring& section)
std::wstring LoadMRUToString(const std::wstring& section)
{
	std::wstring s = L"";
    auto v = LoadMRU(section);
    if (!v.empty()) 
    {
        std::wstring s = v.front(); // 先頭要素を採用
        return s;
    }
    return s;
}

// --- Flags loader for simple 0/1 states in mru_history.ini ---
//static 
bool LoadFlagFromIni(const wchar_t* key, bool def)// = false)
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
// radioボタンヘルパー
// ------------------------------
// 指定ID群の Enable/Disable をまとめて行うユーティリティ
static void EnableControls(HWND hDlg, const int* ids, size_t n, BOOL enable)
{
    for (size_t i = 0; i < n; ++i) {
        if (HWND h = GetDlgItem(hDlg, ids[i])) {
            EnableWindow(h, enable);
        }
    }
}

// v5専用UIの有効/無効を切替（今回グレーアウト対象の6コントロール）
//static void SetV5OnlyControlsEnabled(HWND hDlg, BOOL enable)
//{
//    const int ids[] = {
//        IDC_STC_TRAINPY,
//        IDC_COMBO_TRAINPY,
//        IDC_STC_CFG,        // 新規追加ラベル
//        IDC_COMBO_CFG,
//        IDC_STC_PYTHONEXE,
//        IDC_COMBO_PYTHON,
//        IDC_BTN_BROWSE_TRAINPY,
//        IDC_BTN_BROWSE_CFG,
//        IDC_BTN_BROWSE_PYTHON,
//        IDC_BTN_EDIT_YAML,
//        IDC_BTN_EDIT_CFG
//    };
//    EnableControls(hDlg, ids, _countof(ids), enable);
//}

// ラジオ選択に応じてUIを更新
static void Updated_UI(HWND hDlg)
{
	// YoloV5/V8/V11 のラジオボタンの状態に応じてUIを更新
    const BOOL isV5 = (IsDlgButtonChecked(hDlg, IDC_RAD_YOLOV5) == BST_CHECKED);
    const int ids[] = {
    IDC_STC_TRAINPY,
    IDC_COMBO_TRAINPY,
    IDC_STC_CFG,        // 新規追加ラベル
    IDC_COMBO_CFG,
    IDC_STC_PYTHONEXE,
    IDC_COMBO_PYTHON,
    IDC_BTN_BROWSE_TRAINPY,
    IDC_BTN_BROWSE_CFG,
    IDC_BTN_BROWSE_PYTHON,
    //IDC_BTN_EDIT_YAML,
    IDC_BTN_EDIT_CFG
    };
    //コントロールの有効/無効をまとめて切り替える
    EnableControls(hDlg, ids, _countof(ids), isV5);

	// Proxy UI 更新
    bool chkUseProxy = (IsDlgButtonChecked(hDlg, IDC_CHK_USEPROXY) == BST_CHECKED);
    const int ids_proxy[] = {
    IDC_STC_PROXY_HTTP,
    IDC_STC_PROXY_HTTPS,
    IDC_CMB_PROXY_HTTP,
    IDC_CMB_PROXY_HTTPS
    };
    EnableControls(hDlg, ids_proxy, _countof(ids_proxy), chkUseProxy);

	// Hyperparameter tuning UI 更新
    const BOOL isUseHyp = (IsDlgButtonChecked(hDlg, IDC_CHK_USEHYPERPARAM) == BST_CHECKED);
    const int ids_hyp[] = {
        IDC_COMBO_HYP,
        IDC_BTN_BROWSE_HYP,
        IDC_BTN_EDIT_HYP
    };
    EnableControls(hDlg, ids_hyp, _countof(ids_hyp), isUseHyp);

	// Resume UI 更新
    const BOOL isResume = (IsDlgButtonChecked(hDlg, IDC_CHECK_RESUME) == BST_CHECKED);
    const int ids_resume[] = {
        IDC_CMB_RESUME,
        IDC_BTN_RESUME_BROWSE
    };
	EnableControls(hDlg, ids_resume, _countof(ids_resume), isResume);

    const BOOL isOptionStr = (IsDlgButtonChecked(hDlg, IDC_CHK_OPTION_STR) == BST_CHECKED);
    const int ids_optionstr[] = {
        IDC_COMBO_OPTION_STR
	};
	EnableControls(hDlg, ids_optionstr, _countof(ids_optionstr), isOptionStr);


    const BOOL isOptionShuffle = !(IsDlgButtonChecked(hDlg, IDC_CHK_SPLIT_SHUFFLE) == BST_CHECKED);
    const int ids_optionsfl[] = {
        IDC_RADIO_SPLITUNIT_1,
        IDC_RADIO_SPLITUNIT_5,
        IDC_RADIO_SPLITUNIT_10,
        IDC_RADIO_SPLITUNIT_20,
        IDC_RADIO_SPLITUNIT_50,
        IDC_RADIO_SPLITUNIT_100
    };
    EnableControls(hDlg, ids_optionsfl, _countof(ids_optionsfl), isOptionShuffle);

}

// ------------------------------

///////////////////////////////////////////////////////////////////////////
//
// コピーする関数本体
// Copy dir -> dir with progress
// 
///////////////////////////////////////////////////////////////////////////
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

///////////////////////////////////////////////////////////////////////////
//
// OMP版
// コピーする関数本体 
// Copy dir -> dir with progress
// 
///////////////////////////////////////////////////////////////////////////

// 追加: ヘッダ
//#include <omp.h>

//// UIスレッドに進捗(%)を投げるユーザーメッセージ
//#ifndef WM_APP_PROGRESS
//#define WM_APP_PROGRESS (WM_APP + 100)
//#endif

// 進捗通知（並列スレッドから呼ぶ）
static inline void NotifyProgressPercent(int v) {
    if (g_hDlg) PostMessageW(g_hDlg, WM_APP_PROGRESS, (WPARAM)v, 0);
}

// 並列版コピー
static bool CopyTreeWithProgress_omp(const fs::path& src, const fs::path& dst)
{
    if (!fs::exists(src)) {
        AppendLog(L"[COPY] Source not found: " + src.wstring()); AppendLog(RET);
        return false;
    }
    if (!EnsureDir(dst)) {
        AppendLog(L"[COPY] Cannot create: " + dst.wstring()); AppendLog(RET);
        return false;
    }

    // 1) まず全ディレクトリを逐次で作る（idempotent）
    {
        for (auto it = fs::recursive_directory_iterator(src,
            fs::directory_options::skip_permission_denied);
            it != fs::recursive_directory_iterator(); ++it)
        {
            if (it->is_directory()) {
                const auto rel = fs::relative(it->path(), src);
                EnsureDir(dst / rel);
            }
        }
    }

    // 2) ファイル一覧を収集
    std::vector<std::pair<fs::path, fs::path>> files;
    files.reserve(4096);
    for (auto it = fs::recursive_directory_iterator(src,
        fs::directory_options::skip_permission_denied);
        it != fs::recursive_directory_iterator(); ++it)
    {
        if (it->is_regular_file()) {
            const auto rel = fs::relative(it->path(), src);
            files.emplace_back(it->path(), dst / rel);
        }
    }

    const uint64_t total = static_cast<uint64_t>(files.size());
    if (total == 0) { NotifyProgressPercent(100); return true; }

    // 3) 並列コピー
    std::atomic<uint64_t> done{ 0 };

    // スレッド数は環境やディスクに応じて適宜（例: 4〜8）
    const int max_threads = (std::min)(8, (std::max)(2, omp_get_max_threads()));
#pragma omp parallel for schedule(dynamic,1) num_threads(max_threads)
    for (int i = 0; i < (int)files.size(); ++i) {
        const auto& srcPath = files[i].first;
        const auto& dstPath = files[i].second;

        // 念のため親ディレクトリを確保（前段で作っているので多くはno-op）
        EnsureDir(dstPath.parent_path());

        std::error_code ec;
        fs::copy_file(srcPath, dstPath, fs::copy_options::overwrite_existing, ec);

        // 進捗（UIへは疎に通知）
        uint64_t cur = ++done;
        if ((cur & 0x3F) == 0 || cur == total) { // 64件ごとに通知
            int pct = (int)((cur * 100) / total);
            NotifyProgressPercent(pct);
        }
    }

    NotifyProgressPercent(100);
    return true;
}
/////////////////////////////////////////////////////////////////////////////////


// ------------------------------
// Process runner with output capture
// ------------------------------

//_only_launch: true ならばプロセスハンドルを返すだけで出力キャプチャはしない
static HANDLE LaunchWithCapture(const std::wstring& cmdLineFull, bool _only_launch=false)
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

    if(_only_launch)
    {
        //if (!ok) {
        //    CloseHandle(hRead);
        //    AppendLog(L"[EXEC] Failed to start: " + cmdLineFull);
        //    AppendLog(RET);
        //    return nullptr;
        //}
        CloseHandle(pi.hThread);
        g_hChildProc.store(pi.hProcess);
        return pi.hProcess;
	}

    if (!ok) {
        CloseHandle(hRead);
        //AppendLog(L"[EXEC] Failed to start: " + cmdLineFull);
        //AppendLog(RET);
        PostLogToUi(L"[EXEC] Failed to start: " + cmdLineFull);
        PostLogToUi(RET);
        return nullptr;
    }

    // Job の用意（毎回作り直してOK）
    if (g_hJob) { CloseHandle(g_hJob); g_hJob = NULL; }
    g_hJob = CreateJobObjectW(nullptr, nullptr);

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION ji{};
    ji.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    SetInformationJobObject(g_hJob, JobObjectExtendedLimitInformation, &ji, sizeof(ji));

    // 起動したプロセス（cmd.exe）を Job に登録
    AssignProcessToJobObject(g_hJob, pi.hProcess);

    // reader thread
    std::thread([hRead, piH = pi.hProcess]() {
        char buf[4096];
        DWORD n = 0;
        while (ReadFile(hRead, buf, sizeof(buf) - 1, &n, nullptr) && n > 0) {
            buf[n] = 0;
            // Convert to UTF-16 (best-effort)
            //int wlen = MultiByteToWideChar(CP_UTF8, 0, buf, n, nullptr, 0);
            int wlen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, buf, n, nullptr, 0);
            std::wstring ws;
            if (wlen > 0) {
                ws.resize(wlen);
                //MultiByteToWideChar(CP_UTF8, 0, buf, n, ws.data(), wlen);
                MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, buf, n, ws.data(), wlen);
            }
            else {
                int wlen2 = MultiByteToWideChar(CP_ACP, 0, buf, n, nullptr, 0);
                ws.resize(wlen2);
                MultiByteToWideChar(CP_ACP, 0, buf, n, ws.data(), wlen2);
            }
            //AppendLog(ws);
            PostLogToUi(ws);
			//LogAppendANSI(ws); // ANSIカラーコードを含む場合はここで処理
        }
        CloseHandle(hRead);
        WaitForSingleObject(piH, INFINITE);
        DWORD ec = 0; GetExitCodeProcess(piH, &ec);
        //AppendLog(L"[EXEC] Exit code: " + std::to_wstring(ec));
        //AppendLog(RET);
        PostLogToUi(L"[EXEC] Exit code: " + std::to_wstring(ec));
        PostLogToUi(RET);

        }).detach();

    CloseHandle(pi.hThread);
    g_hChildProc.store(pi.hProcess);
    return pi.hProcess;
}
//static void StopChild()
//{
//    HANDLE h = g_hChildProc.exchange(nullptr);
//    if (h) {
//        TerminateProcess(h, 1);
//        CloseHandle(h);
//        AppendLog(L"[EXEC] Terminated.");
//        AppendLog(RET);
//    }
//}
static void StopChild()
{
    // 先に Job を閉じる or 終了させる（ツリー全体を止める）
    if (g_hJob) {
        // すぐに全員に ExitCode=1 を投げたいなら TerminateJobObject
        TerminateJobObject(g_hJob, 1);
        CloseHandle(g_hJob);
        g_hJob = NULL;
    }

    // 個別プロセスハンドルも後片付け（安全側）
    HANDLE h = g_hChildProc.exchange(nullptr);
    if (h) {
        // 念のため（もう死んでいるはずだが）単体も殺して閉じる
        TerminateProcess(h, 1);
        CloseHandle(h);
        AppendLog(L"[EXEC] Terminated (job).");
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
        AppendLog(RET);
    }
    else
    {
        //MessageBoxW(NULL, L"git の実行に失敗しました。PATH設定を確認してください。", L"Git設定", MB_OK | MB_ICONERROR);
		AppendLog(L"[GIT] git の実行に失敗しました。PATH設定を確認してください。");
        AppendLog(RET);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// 現在の設定を INI ファイルに保存
int TrainParams::ReadControls(HWND hDlg)
{

    src_dir =       GetText(hDlg, IDC_COMBO_TEMP);
    src_dir_image = GetText(hDlg, IDC_COMBO_IMG);
    src_dir_label = GetText(hDlg, IDC_COMBO_LABEL);

	RatioTrain = GetText(hDlg, IDC_EDIT_TRAINPCT);
	RatioUse = GetText(hDlg, IDC_EDIT_REDUCTION); 

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

    option_str = GetText(hDlg, IDC_COMBO_OPTION_STR);  // ← 追加

	// HTTP/HTTPS プロキシ設定（v8/v11用）
	http_proxy = GetText(hDlg, IDC_CMB_PROXY_HTTP);
	https_proxy = GetText(hDlg, IDC_CMB_PROXY_HTTPS);
	chkUseProxy = IsDlgButtonChecked(hDlg, IDC_CHK_USEPROXY);

    //チェックボックスの状態を取得
    chkResume = IsDlgButtonChecked(hDlg, IDC_CHECK_RESUME);
    chkCache = IsDlgButtonChecked(hDlg,  IDC_CHK_CACHE);
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
        
    //MultiCopyDlg用
}


// どこか共通の .cpp（YoloTrainGUI.cpp でも可）に置く
// #include <filesystem>
// #include <atomic>
// 再入ガード（関数スコープ静的）

static std::atomic<bool> g_clearTempRunning{ false };

// keepRoot = false: ルートごと削除（従来メインの挙動）
// keepRoot = true : ルートは残して「中身だけ」削除
//static 
void DoClearTemp(HWND hOwner,const UINT _ID_COMBO_TEMP, bool confirm, bool keepRoot)
{
    if (g_clearTempRunning.exchange(true)) {
        // すでに実行中なら何もしない
        return;
    }

    // 進捗バーがメイン/サブどちらにあるか不定なので控えめに使う
    auto setProgress = [&](int pct) {
        HWND hPrg = GetDlgItem(hOwner, IDC_PROGRESS_COPY); // サブにある想定
        if (hPrg) SendMessageW(hPrg, PBM_SETPOS, pct, 0);
        };

    try {
        //std::wstring tempDir = GetText(hOwner, IDC_COMBO_TEMP);
        std::wstring tempDir = GetText(hOwner, _ID_COMBO_TEMP);
        if (tempDir.empty()) {
            AppendLog(L"[TEMP] Temp directory is not set."); AppendLog(RET);
            g_clearTempRunning = false; return;
        }
        fs::path tempPath(tempDir);
        if (!fs::exists(tempPath)) {
            AppendLog(L"[TEMP] Temp directory does not exist: " + tempPath.wstring()); AppendLog(RET);
            g_clearTempRunning = false; return;
        }

        if (confirm) {
            int r = MessageBoxW(hOwner,
                L"Temp directory will be cleared. Continue?",
                L"Confirm",
                MB_OKCANCEL | MB_ICONWARNING);
            if (r != IDOK) {
                AppendLog(L"[TEMP] Clear operation cancelled."); AppendLog(RET);
                g_clearTempRunning = false; return;
            }
        }

        setProgress(0);

        if (keepRoot) {
            // ルートは残し、中身だけ削除
            std::error_code ec;
            for (auto it = fs::directory_iterator(tempPath, fs::directory_options::skip_permission_denied, ec);
                it != fs::directory_iterator(); ++it)
            {
                fs::remove_all(it->path(), ec); // 失敗しても継続
            }
        }
        else {
            // ルートごと削除（従来メインのコードと同等）
            fs::remove_all(tempPath);
        }

        setProgress(100);
        AppendLog(L"[TEMP] Cleared temp directory: " + tempPath.wstring());
        AppendLog(RET);
    }
    catch (const fs::filesystem_error& e) {
        std::wstring msg = FromUTF8(e.what());
        AppendLog(L"[TEMP] Error clearing temp directory: " + msg);
        AppendLog(RET);
    }

    g_clearTempRunning = false;
}

/////////////////////////////////////////////////////////////
// 
// トレーニング実行
// ここで設定値を取得してコマンドラインを構築し、
// Python スクリプトを呼び出す
//
/////////////////////////////////////////////////////////////
/*
void TrainParams::DoTrain_old()
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
    ss << Quote(python) << L" -u " << Quote(trainpy)
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
}
*/

/////////////////////////////////////////////////////////////
// 
// トレーニング実行
// ここで設定値を取得してコマンドラインを構築し、
// Python スクリプトを呼び出す
//
/////////////////////////////////////////////////////////////
void TrainParams::DoTrain()
{
    if (python.empty())
        python = L"python";

    // UIの exist_ok チェックは ReadControls で取っていないのでここで拾う
    exist_ok = (IsDlgButtonChecked(g_hDlg, IDC_CHK_EXIST_OK) == BST_CHECKED);

    // どのバックエンドか（ラジオ）
    std::wstring be = L"YOLOV5";
    if (IsDlgButtonChecked(g_hDlg, IDC_RAD_YOLOV8) == BST_CHECKED)
        be = L"YOLOV8";
    else if (IsDlgButtonChecked(g_hDlg, IDC_RAD_YOLO11) == BST_CHECKED)
        be = L"YOLO11";

    // 共通必須チェック
    if (datayaml.empty()) {
        AppendLog(L"[TRAIN] data.yaml is required.");
        AppendLog(RET);
        return;
    }
    if (workdir.empty()) {
        AppendLog(L"[TRAIN] workdir is required.");
        AppendLog(RET);
        return;
    }

    //コマンド組立
    std::wstringstream ss;
    // ① まず UTF-8 に統一
    //ss << L"chcp 65001>nul && set PYTHONUTF8=1 && set PYTHONIOENCODING=utf-8 && ";

	//Proxy設定（v8/v11用）
	chkUseProxy = (IsDlgButtonChecked(g_hDlg, IDC_CHK_USEPROXY) == BST_CHECKED);
    if(chkUseProxy == BST_CHECKED) {
        ss << L"set HTTP_PROXY="  << http_proxy  << L" && ";
        ss << L"set HTTPS_PROXY=" << https_proxy << L" && ";
        ss << L"set http_proxy="  << http_proxy  << L" && ";
        ss << L"set https_proxy=" << https_proxy << L" && ";
    }

    // conda/venv の有効化
    if (!activate.empty()) {
        ss << L"call activate " << Quote(activate) << L" && ";
    }
    // 作業ディレクトリへ
    ss << L"cd /d " << Quote(workdir) << L" && ";

    if (be == L"YOLOV5")
    {
        // --- 従来どおり：train.py を直接呼ぶ ---
        if (trainpy.empty()) {
            AppendLog(L"[TRAIN] train.py path/name is required for YOLOv5.");
            AppendLog(RET);
            return;
        }
        ss << Quote(python) << L" " << Quote(trainpy)
            << L" --data " << Quote(datayaml);

        if (!epochs.empty())   ss << L" --epochs " << epochs;
        if (!patience.empty()) ss << L" --patience " << patience;

        if (chkResume == BST_CHECKED) {
            if (resume.empty()) ss << L" --resume";
            else                ss << L" --resume " << Quote(resume);
        }

        if (!batch.empty()) ss << L" --batch " << batch;
        if (!imgsz.empty()) ss << L" --img " << imgsz;   // v5 は --img
        if (!device.empty()) ss << L" --device " << device;

        if (chkUseHyp == BST_CHECKED) {
            if (hypyaml.empty()) ss << L" --hyp";
            else                 ss << L" --hyp " << Quote(hypyaml);
        }
        if (!cfgyaml.empty())   ss << L" --cfg " << Quote(cfgyaml);
        if (!weights.empty())   ss << L" --weights " << Quote(weights);
        if (chkCache == BST_CHECKED) ss << L" --cache disk";
        if (exist_ok)                ss << L" --exist-ok";

        if (!_NAME.empty())  ss << L" --name " << Quote(_NAME);
        if (!project.empty()) ss << L" --project " << Quote(project);

        if (!option_str.empty()) ss << L" " << option_str;  // ← 追加（生の文字列をそのまま足す）
    }
    else
    {
        // --- YOLOv8 / YOLO11：Ultralytics CLI を使用 ---
        // model は UI の「weights」欄を流用（yolov8n.pt / yolo11n.pt / 任意パス）
        if (weights.empty()) {
            AppendLog(L"[TRAIN] model (.pt) is recommended for YOLOv8/YOLO11 (set in Weights field).");
            AppendLog(RET);
        }

        // 既定は "python -m ultralytics"。yolo コマンドが PATH にあるなら差し替えたい場合はここで検出して置換も可。
        //ss << Quote(python) << L" -m ultralytics "
        ss << L"yolo "
            << task << L" train"
            << L" data=" << Quote(datayaml);

        if (!weights.empty())   ss << L" model=" << Quote(weights);  // v8/11 は model=
        if (!epochs.empty())    ss << L" epochs=" << epochs;
        if (!patience.empty())  ss << L" patience=" << patience;
        if (!batch.empty())     ss << L" batch=" << batch;
        if (!imgsz.empty())     ss << L" imgsz=" << imgsz;           // v8/11 は imgsz=
        if (!device.empty())    ss << L" device=" << device;

        if (chkResume == BST_CHECKED) {
            if (resume.empty()) ss << L" resume=True";
            else                ss << L" resume=" << Quote(resume);
        }
        //if (chkCache == BST_CHECKED) ss << L" cache=True";
        if (chkCache == BST_CHECKED) ss << L" cache=disk";
        if (exist_ok)                ss << L" exist_ok=True";

        if (!_NAME.empty())     ss << L" name=" << Quote(_NAME);
        if (!project.empty())   ss << L" project=" << Quote(project);

        // hyp/cfg は v8/11 では通常不要。必要なら overrides 用に cfg= を追加する等で拡張可。
        if (!option_str.empty()) ss << L" " << option_str;  // ← 追加（生の文字列をそのまま足す）
    }

    const std::wstring command = ss.str();
    AppendLog(L"[TRAIN] " + command);
    AppendLog(RET);

    // 実行（既存の外部プロセス起動＋標準出力をRichEditに反映）
    LaunchWithCapture(command);                  // 進捗・ANSI対応のログ表示あり
    AppendCmdHistory(command);                   // 履歴へ追記
    //SaveCurrentSettingsToIni(nullptr);           // MRU保存}
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

    if(0){
        if (!CopyTreeWithProgress(img, dstImages))
        {
            AppendLog(L"[COPY Images] Not Completed.");
            return;
        }
    }
    else {//並列版
        if (!CopyTreeWithProgress_omp(img, dstImages))
        {
            AppendLog(L"[COPY Images] Not Completed.");
            return;
        }
    }

    AppendLog(L"[COPY] labels -> " + dstLabels.wstring());
    AppendLog(RET);
    if(0)
    {
        if (!CopyTreeWithProgress(lab, dstLabels))
        {
            AppendLog(L"[COPY Labels] Not Completed.");
            return;
        }
    }
    else {
        if (!CopyTreeWithProgress_omp(lab, dstLabels))
        {
            AppendLog(L"[COPY Labels] Not Completed.");
            return;
        }
    
    }
    SetProgress(100);
    AppendLog(L"[COPY] Completed.");
    AppendLog(RET);
    SaveMRU(L"Image Data", img);
    SaveMRU(L"Label Data", lab);
    SaveMRU(L"Temp Dir", tmp);
}

static void DoSplit()
{
    int splitunit = 1;
    if (IsDlgButtonChecked(g_hDlg, IDC_RADIO_SPLITUNIT_5) == BST_CHECKED) splitunit = 5;
    else if (IsDlgButtonChecked(g_hDlg, IDC_RADIO_SPLITUNIT_10) == BST_CHECKED) splitunit = 10;
	else if (IsDlgButtonChecked(g_hDlg, IDC_RADIO_SPLITUNIT_20) == BST_CHECKED) splitunit = 20;
	else if (IsDlgButtonChecked(g_hDlg, IDC_RADIO_SPLITUNIT_50) == BST_CHECKED) splitunit = 50;
	else if (IsDlgButtonChecked(g_hDlg, IDC_RADIO_SPLITUNIT_100) == BST_CHECKED) splitunit = 100;

    bool _is_shuffle = IsDlgButtonChecked(g_hDlg, IDC_CHK_SPLIT_SHUFFLE);

    std::wstring tmp = GetText(g_hDlg, IDC_COMBO_TEMP);
    int trainPct = _wtoi(GetText(g_hDlg, IDC_EDIT_TRAINPCT).c_str());
    float reduc = _wtof(GetText(g_hDlg, IDC_EDIT_REDUCTION).c_str());
    if (tmp.empty() || trainPct <= 0) {
        AppendLog(L"[SPLIT] Missing temp or train%");
        AppendLog(RET);
        return;
    }

    fs::path src = fs::path(tmp) / "source";
    fs::path dst = fs::path(tmp) / "dataset";
    try
    {
        if (fs::exists(dst))
            fs::remove_all(dst);
    }
    catch (...) {}
    ResetProgress();
    AppendLog(L"[SPLIT] source=" + src.wstring() + L"   dest=" + dst.wstring());
    AppendLog(RET);
    SplitDataset(src, dst, trainPct, reduc, _is_shuffle, splitunit);   // based on your divfiles.cpp  :contentReference[oaicite:2]{index=2}
    SetProgress(100);
}


/////////////////////////////////////////////////////////////////
// 
// 共有データ（画像／ラベル）とテンポラリディレクトリの設定を保存
// メモリまたはコントロールから取得して保存
//
/////////////////////////////////////////////////////////////////
int TrainParams::SaveCurrentSettingsToIni(HWND hDlg)
{
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

		//オプション文字列の保存
        SaveMRU(L"option_str", GetText(hDlg, IDC_COMBO_OPTION_STR));  // ← 追加
        chkResume = (IsDlgButtonChecked(hDlg, IDC_CHK_OPTION_STR) == BST_CHECKED);
        SaveMRU(L"option_str_chk", chkResume ? L"1" : L"0");

        // チェックボックスは "1" / "0" を保存
        chkCache = (IsDlgButtonChecked(hDlg, IDC_CHK_CACHE) == BST_CHECKED);
        SaveMRU(L"cache", chkCache ? L"1" : L"0");

        exist_ok = (IsDlgButtonChecked(hDlg, IDC_CHK_EXIST_OK) == BST_CHECKED);
        SaveMRU(L"exist_ok", exist_ok ? L"1" : L"0");

        chkResume = (IsDlgButtonChecked(hDlg, IDC_CHECK_RESUME) == BST_CHECKED);
        SaveMRU(L"resume_chk", chkResume ? L"1" : L"0");

        chkUseHyp = (IsDlgButtonChecked(hDlg, IDC_CHK_USEHYPERPARAM) == BST_CHECKED);
        SaveMRU(L"hyper_chk", chkUseHyp ? L"1" : L"0");

        // Yolov5,yolov8, yolo11ラジオボタンの状態を保存
        if (IsDlgButtonChecked(hDlg, IDC_RAD_YOLOV5) == BST_CHECKED) {
            SaveMRU(L"backend", L"YOLOV5");
        }
        else if (IsDlgButtonChecked(hDlg, IDC_RAD_YOLOV8) == BST_CHECKED) {
            SaveMRU(L"backend", L"YOLOV8");
        }
        else if (IsDlgButtonChecked(hDlg, IDC_RAD_YOLO11) == BST_CHECKED) {
            SaveMRU(L"backend", L"YOLO11");
        }
        else {
            SaveMRU(L"backend", L"YOLOV5");
	    }

        // チェックボックスは "1" / "0" を保存
        chkCache = (IsDlgButtonChecked(hDlg, IDC_CHK_SPLIT_SHUFFLE) == BST_CHECKED);
        SaveMRU(L"SplitShuffle", chkCache ? L"1" : L"0");

        // Split Unitラジオボタンの状態を保存
        if (IsDlgButtonChecked(hDlg, IDC_RADIO_SPLITUNIT_1) == BST_CHECKED) {
            SaveMRU(L"SplitUnit", L"1");
        }
        else if (IsDlgButtonChecked(hDlg, IDC_RADIO_SPLITUNIT_5) == BST_CHECKED) {
            SaveMRU(L"SplitUnit", L"5");
        }
        else if (IsDlgButtonChecked(hDlg, IDC_RADIO_SPLITUNIT_10) == BST_CHECKED) {
            SaveMRU(L"SplitUnit", L"10");
        }
        else if (IsDlgButtonChecked(hDlg, IDC_RADIO_SPLITUNIT_20) == BST_CHECKED) {
            SaveMRU(L"SplitUnit", L"20");
        }
        else if (IsDlgButtonChecked(hDlg, IDC_RADIO_SPLITUNIT_50) == BST_CHECKED) {
            SaveMRU(L"SplitUnit", L"50");
        }
        else if (IsDlgButtonChecked(hDlg, IDC_RADIO_SPLITUNIT_100) == BST_CHECKED) {
            SaveMRU(L"SplitUnit", L"100");
        }
        else
        {
            SaveMRU(L"SplitUnit", L"1");
        }

        // v8/v11 用の HTTP/HTTPS プロキシ設定
        SaveMRU(L"proxy_http", GetText(hDlg, IDC_CMB_PROXY_HTTP));
        SaveMRU(L"proxy_https", GetText(hDlg, IDC_CMB_PROXY_HTTPS));
	    chkUseProxy = (IsDlgButtonChecked(hDlg, IDC_CHK_USEPROXY) == BST_CHECKED);
        SaveMRU(L"chkUseProxy", chkUseProxy ? L"1" : L"0");
    }
/*
    else // hDlgが無効な場合は、メモリ上の共有データの設定を保存
    {
        SaveMRU(L"Image Data", this->src_dir_image); // 例: "C:\path\to\images"
        SaveMRU(L"Label Data", this->src_dir_label); // 例: "C:\path\to\labels"
        SaveMRU(L"Temp Dir", this->src_dir); // 例: "C:\path\to\temp"

        // 分割パラメータ
        SaveMRU(L"TrainPercent", this->RatioTrain);  // 例: "80"
        SaveMRU(L"Reduction", this->RatioUse ); // 例: "1.0"

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

		// Yolov5,yolov8, yolo11ラジオボタンの状態を保存
        SaveMRU(L"backend", this->backend);

		// v8/v11 用の HTTP/HTTPS プロキシ設定
        SaveMRU(L"proxy_http", this->http_proxy);
		SaveMRU(L"proxy_https", this->https_proxy);
        SaveMRU(L"chkUseProxy", this->chkUseProxy ? L"1" : L"0");
    }
*/
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////
// 
// Build train command and launch
//
//////////////////////////////////////////////////////////////////////////////////////

static void DoTrain(HWND hDlg = nullptr)
{
	TrainParams _params;
    _params.ReadControls(hDlg); // コントロールから値を読み取る
    // ここでコマンドを実行
    _params.DoTrain();
    // 設定をINIに保存
	_params.SaveCurrentSettingsToIni(hDlg); // hDlgが有効ならコントロールから値を保存 無効ならメモリ上の共有データを保存
}

//////////////////////////////////////////////////////////////////////////////////////
// Dialog 初期化
//////////////////////////////////////////////////////////////////////////////////////
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

	// LOGボックスの初期化
    HWND hLog = GetDlgItem(hDlg, IDC_LOG);
    // 1) プレーンテキストモード（CR/LFの扱いを安定化）
    //    ※コントロールが空のうちに行う必要あり
    SendMessageW(hLog, EM_SETTEXTMODE, TM_PLAINTEXT, 0);
    // 2) ワードラップ無効化（横方向に伸ばす）
    //    EM_SETTARGETDEVICE(NULL, 1) で折り返しなし
    SendMessageW(hLog, EM_SETTARGETDEVICE, (WPARAM)NULL, (LPARAM)1);
    // 3) 横スクロールバーを出す（任意）
    LONG_PTR st = GetWindowLongPtrW(hLog, GWL_STYLE);
    st |= WS_HSCROLL | ES_AUTOHSCROLL;    // 既存: WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY
    SetWindowLongPtrW(hLog, GWL_STYLE, st);
    // 4) テキスト上限を拡大（任意：既定が小さい環境の保険）
    SendMessageW(hLog, EM_EXLIMITTEXT, 0, 16 * 1024 * 1024);  // 16MB
    // 5) 等幅フォントを既定に（日本語も等幅にしたいので MS ゴシック推奨）

    CHARFORMAT2 cf{}; cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_FACE | CFM_SIZE;
    cf.yHeight = 180; // 10pt（お好みで）
    lstrcpynW(cf.szFaceName, L"ＭＳ ゴシック", _countof(cf.szFaceName));
    //SendMessageW(hLog, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
    SendMessageW(hLog, EM_SETCHARFORMAT, SCF_ALL | SCF_DEFAULT, (LPARAM)&cf);
    // ※欧文専用で良ければ "Consolas" でもOK（日本語は等幅になりません）
    // lstrcpynW(cf.szFaceName, L"Consolas", _countof(cf.szFaceName));
    // SendMessageW(hLog, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);

    // 2.5) デュアルフォント無効化（英字/日本語での自動フォント切替を止める）
    DWORD opts = (DWORD)SendMessageW(hLog, EM_GETLANGOPTIONS, 0, 0);
    opts &= ~IMF_DUALFONT; // ← このビットだけ落として…
    SendMessageW(hLog, EM_SETLANGOPTIONS, 0, opts); // ← 全体を“上書き”で戻す

    
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

	LoadMRUToCombo(GetDlgItem(hDlg, IDC_CMB_PROXY_HTTP), L"proxy_http");
	LoadMRUToCombo(GetDlgItem(hDlg, IDC_CMB_PROXY_HTTPS), L"proxy_https");

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

	ShowFirstComboItem(GetDlgItem(hDlg, IDC_CMB_PROXY_HTTP));
	ShowFirstComboItem(GetDlgItem(hDlg, IDC_CMB_PROXY_HTTPS));

    // 規定値
    LoadMRUToTextControl(GetDlgItem(hDlg, IDC_EDIT_TRAINPCT), L"TrainPercent");
    LoadMRUToTextControl(GetDlgItem(hDlg, IDC_EDIT_REDUCTION), L"Reduction");
    // 空なら既定値を入れる
    if (GetWindowTextLengthW(GetDlgItem(hDlg, IDC_EDIT_TRAINPCT)) == 0)
        SetDlgItemTextW(hDlg, IDC_EDIT_TRAINPCT, L"80");
    if (GetWindowTextLengthW(GetDlgItem(hDlg, IDC_EDIT_REDUCTION)) == 0)
        SetDlgItemTextW(hDlg, IDC_EDIT_REDUCTION, L"1.0");

    CheckDlgButton(hDlg, IDC_CHK_CACHE,         LoadFlagFromIni(L"cache",       false) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHK_EXIST_OK,      LoadFlagFromIni(L"exist_ok",    false) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHECK_RESUME,      LoadFlagFromIni(L"resume_chk",  false) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHK_USEHYPERPARAM, LoadFlagFromIni(L"hyper_chk",   false) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hDlg, IDC_CHK_USEPROXY,      LoadFlagFromIni(L"chkUseProxy", false) ? BST_CHECKED : BST_UNCHECKED);

	LoadMRUToTextControl(GetDlgItem(hDlg, IDC_EDIT_PROJECT), L"project");
    ShowFirstComboItem(GetDlgItem(hDlg, IDC_EDIT_PROJECT));           

    LoadMRUToCombo(GetDlgItem(hDlg, IDC_COMBO_OPTION_STR), L"option_str"); // ← 追加
    ShowFirstComboItem(GetDlgItem(hDlg, IDC_COMBO_OPTION_STR));            // ← 追加
    CheckDlgButton(hDlg, IDC_CHK_OPTION_STR, LoadFlagFromIni(L"option_str_chk", false) ? BST_CHECKED : BST_UNCHECKED);

	//UpdateBackendUI(hDlg); // バックエンドのUI更新
 //   UpdateProxyUI(hDlg);   // プロキシのUI更新
	//UpdateHyperUI(hDlg);  // ハイパーパラメータのUI更新

	//std::wstring backend = LoadMRUToString(GetDlgItem(hDlg, IDC_RAD_YOLOV5), L"backend");
    std::wstring backend = LoadMRUToString(L"backend");

    if( backend == L"YOLOV5") {
        CheckRadioButton(hDlg, IDC_RAD_YOLOV5, IDC_RAD_YOLO11, IDC_RAD_YOLOV5);
    }else if (backend == L"YOLOV8") {
        CheckRadioButton(hDlg, IDC_RAD_YOLOV5, IDC_RAD_YOLO11, IDC_RAD_YOLOV8);
    }else if (backend == L"YOLO11") {
        CheckRadioButton(hDlg, IDC_RAD_YOLOV5, IDC_RAD_YOLO11, IDC_RAD_YOLO11);
    }else {
        CheckRadioButton(hDlg, IDC_RAD_YOLOV5, IDC_RAD_YOLO11, IDC_RAD_YOLOV5); // default
	}

    CheckDlgButton(hDlg, IDC_CHK_SPLIT_SHUFFLE, LoadFlagFromIni(L"SplitShuffle", false) ? BST_CHECKED : BST_UNCHECKED);
    
    std::wstring SplitUnit = LoadMRUToString(L"SplitUnit");
    if (SplitUnit == L"1") {
        CheckRadioButton(hDlg, IDC_RADIO_SPLITUNIT_1, IDC_RADIO_SPLITUNIT_100, IDC_RADIO_SPLITUNIT_1);
    }
    else if (SplitUnit == L"5") {
        CheckRadioButton(hDlg, IDC_RADIO_SPLITUNIT_1, IDC_RADIO_SPLITUNIT_100, IDC_RADIO_SPLITUNIT_5);
    }
    else if (SplitUnit == L"10") {
        CheckRadioButton(hDlg, IDC_RADIO_SPLITUNIT_1, IDC_RADIO_SPLITUNIT_100, IDC_RADIO_SPLITUNIT_10);
    }
    else if (SplitUnit == L"20") {
        CheckRadioButton(hDlg, IDC_RADIO_SPLITUNIT_1, IDC_RADIO_SPLITUNIT_100, IDC_RADIO_SPLITUNIT_20);
    }
    else if (SplitUnit == L"50") {
        CheckRadioButton(hDlg, IDC_RADIO_SPLITUNIT_1, IDC_RADIO_SPLITUNIT_100, IDC_RADIO_SPLITUNIT_50);
    }
    else if (SplitUnit == L"100") {
        CheckRadioButton(hDlg, IDC_RADIO_SPLITUNIT_1, IDC_RADIO_SPLITUNIT_100, IDC_RADIO_SPLITUNIT_100);
    }
    else {
        CheckRadioButton(hDlg, IDC_RADIO_SPLITUNIT_1, IDC_RADIO_SPLITUNIT_100, IDC_RADIO_SPLITUNIT_1); // default
	}
 
    // チェックボックス設定の後、関連するコントロールをグレーアウトしたりする
    Updated_UI(hDlg);

    // ボタンのツールチップを設定
    INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_WIN95_CLASSES };
    InitCommonControlsEx(&icc);
    SetTootips(hDlg);

    //サイズ変更対応
    RECT rcCli{}; GetClientRect(hDlg, &rcCli);
    g_initCx = rcCli.right - rcCli.left;
    g_initCy = rcCli.bottom - rcCli.top;

    //HWND hLog = GetDlgItem(hDlg, IDC_LOG);          // ログ（最下部の RichEdit）
    if (hLog) {
        GetChildRectClient(hDlg, hLog, g_logInit);
        g_logLeft = g_logInit.left;
        g_logTop = g_logInit.top;
        g_logWidth = g_logInit.right - g_logInit.left;
        g_logHeight = g_logInit.bottom - g_logInit.top;
    }
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

// tensorboard起動 起動しない(ToT)
bool RUNNING_TENSORBOARD = false;
void StartTensorBoardDetached_old(const std::wstring& logdir, const std::wstring& activate, int port = 6006) {
    if (RUNNING_TENSORBOARD)
        return;
    else
    {
		RUNNING_TENSORBOARD = true;

        std::wostringstream ss;

        if (!activate.empty()) {
            ss << L"cmd.exe /c call activate.bat " << activate << L" && ";
        }
        // 作業ディレクトリへ
        ss << L"cd /d " << Quote(logdir) << L" && ";

        ss << L"tensorboard --logdir " << Quote(logdir)
            << L" --host 127.0.0.1 --port " << std::to_wstring(port)
            << L" > c:\\tensorboard_stdout.log";

        // CreateProcessW で親コンソールにぶら下げない
        STARTUPINFOW si{}; si.cb = sizeof(si);
        PROCESS_INFORMATION pi{};
        //std::wstring cmdline = L"cmd.exe /c " + cmd; // 直接 tensorboard でも可
        std::wstring cmdline = ss.str();

        DWORD flags = CREATE_NO_WINDOW | DETACHED_PROCESS; // 画面無し・親切り離し
        if (CreateProcessW(nullptr, cmdline.data(), nullptr, nullptr, FALSE, flags, nullptr, nullptr, &si, &pi)) 
        {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess); // PID保持したければ保持する
        }
        else
            {
            RUNNING_TENSORBOARD = false;
            AppendLog(L"[TENSORBOARD] CreateProcess failed.");
            AppendLog(RET);
		}
    }
}
void StartTensorBoardDetached(const std::wstring& logdir,
    const std::wstring& condaEnvName, // 例: L"yolov5"
    int port = 6006)
{
    if (RUNNING_TENSORBOARD) return;

    // conda.bat は PATH が通っていればこれでOK。通っていなければフルパスを渡す欄をUIで持つ
    const std::wstring condaBat = L"conda.bat";

    // 1) ブロック全体を括弧でまとめて、一括で stdout/stderr を捕まえる
    //    ※ activate 失敗・cd 失敗も %TEMP% に出る
    std::wostringstream cmd;
    cmd << L"cmd.exe /d /c ("
        << L"call " << Quote(condaBat) << L" activate " << Quote(condaEnvName)
        << L" && tensorboard --logdir " << Quote(logdir)
        << L" --host 127.0.0.1 --port " << port
        << L") > %TEMP%\\tensorboard_stdout.log 2>&1";

    std::wstring cmdline = cmd.str();
    std::vector<wchar_t> buf(cmdline.begin(), cmdline.end());
    buf.push_back(L'\0');

    STARTUPINFOW si{}; si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    // 2) まずは NO_WINDOW のみで十分（親と切り離したい運用なら NEW_CONSOLE に）
    DWORD flags = CREATE_NO_WINDOW;

    // 3) 作業ディレクトリはここで指定（不要なら nullptr でも可）
    LPCWSTR currentDir = logdir.c_str();

    BOOL ok = CreateProcessW(
        nullptr,            // lpApplicationName
        buf.data(),         // 書き換え可能バッファ
        nullptr, nullptr, FALSE,
        flags,
        nullptr,
        currentDir,         // ここでCWDを渡すので「cd /d」は不要
        &si, &pi
    );
    AppendLog(L"[TENSORBOARD] ");
    AppendLog(buf.data());
    AppendLog(RET);

    if (ok) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        RUNNING_TENSORBOARD = true;
        AppendLog(L"[TENSORBOARD] launched. Open http://127.0.0.1:" + std::to_wstring(port) + L"/");
        AppendLog(RET);
    }
    else {
        RUNNING_TENSORBOARD = false;
        AppendLog(L"[TENSORBOARD] CreateProcess failed.");
        AppendLog(RET);
    }
}

static void TrimInPlace(std::wstring& s)
{
    // 前方の空白を削除
    s.erase(0, s.find_first_not_of(L" \t\r\n"));
    // 後方の空白を削除
    size_t endpos = s.find_last_not_of(L" \t\r\n");
    if (endpos != std::wstring::npos)
        s.erase(endpos + 1);
    else
        s.clear();
}

//URLを開く http:6006/tensorboard等
static bool OpenURLWithDefaultBrowser(const std::wstring& url)
{
    std::wstring link = url;
    TrimInPlace(link);
    // 文字列の前後の空白をその場で削除するユーティリティ

    if (link.empty()) {
        AppendLog(L"[OPEN] URL is empty.");
        AppendLog(RET);
        return false;
    }
    // "http://" などのスキームがない場合は "https://" を付与してみる
    if (link.find(L"://") == std::wstring::npos) {
        link = L"https://" + link;
    }
    HINSTANCE h = ShellExecuteW(g_hDlg, L"open", link.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    auto code = reinterpret_cast<INT_PTR>(h);
    if (code <= 32) {
        // 代表的なエラー値を簡単に表示
        std::wstringstream ss;
        ss << L"[OPEN] ShellExecute failed (" << code << L") : " << link;
        AppendLog(RET);
        AppendLog(ss.str());
        return false;
    }
    AppendLog(L"[OPEN] Launched browser for: " + link);
    AppendLog(RET);
    return true;
}

// ------------------------------
// Dialog Procedure     
// ------------------------------
static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    //WORD id = 0;            // = LOWORD(wParam);
    //WORD code = 0;          // = HIWORD(wParam);
    //HWND hFrom = nullptr;   // = (HWND)lParam;

    switch (msg) {
        case WM_INITDIALOG:
        {
            InitDialog(hDlg);
            return TRUE;
        }
        break;
        case WM_COMMAND:
        {
            //if (LOWORD(wParam) == IDC_STC_TEMP && HIWORD(wParam) == STN_CLICKED) {
            //    if (bTmpTipShow) HideTempTip(hDlg);
            //    else                ShowTempTipAtCursor(hDlg);
            //    return TRUE;
            //}
            //id = LOWORD(wParam);
            //code = HIWORD(wParam);
            //hFrom = (HWND)lParam;

            //switch (id)
            switch (LOWORD(wParam))
            {
                case IDC_BTN_OPEN_COPY_MULTI:
                {
                    // 親はメインダイアログ、インスタンスは親ウィンドウから取得
                    HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(hDlg, GWLP_HINSTANCE);

                    // モーダルで開く
                    INT_PTR ret = DialogBoxParamW(
                        hInst,
                        MAKEINTRESOURCEW(IDD_COPY_MULTI),
                        hDlg,
                        CopyMultiDlgProc,
                        0);

                    // 失敗チェック（必要ならログ出力など）
                    if (ret == -1) {
                        // 例: AppendLog(L"[ERROR] IDD_COPY_MULTI dialog failed.\r\n");
                    }
                    return TRUE;
                }

                case IDC_BTN_BROWSE_IMG: 
					PickFolderEx(hDlg, IDC_COMBO_IMG, L"Original IMAGE Directory");
                break;
                case IDC_BTN_BROWSE_LABEL: {
                    //std::wstring p; if (PickFolder(hDlg, p)) { SetComboText(GetDlgItem(hDlg, IDC_COMBO_LABEL), p); }
					PickFolderEx(hDlg, IDC_COMBO_LABEL, L"Original LABEL Directory");
                } break;
                case IDC_BTN_BROWSE_TEMP: {
                    //std::wstring p; if (PickFolder(hDlg, p)) { SetComboText(GetDlgItem(hDlg, IDC_COMBO_TEMP), p); }
                    PickFolderEx(hDlg, IDC_COMBO_TEMP, L"Tempolary Source Directory");
                } break;
                case IDC_BTN_BROWSE_WORKDIR: {
                    //std::wstring p; if (PickFolder(hDlg, p)) { SetComboText(GetDlgItem(hDlg, IDC_COMBO_WORKDIR), p); }
                    PickFolderEx(hDlg, IDC_COMBO_WORKDIR);
                } break;
                case IDC_BTN_BROWSE_TRAINPY:{
                    COMDLG_FILTERSPEC spec[] = { {L"Python (*.py)", L"*.py"} };
                    //std::wstring p; if (PickFile(hDlg, spec, 1, p)) { SetComboText(GetDlgItem(hDlg, IDC_COMBO_TRAINPY), p);}
                    PickFileEx(hDlg, IDC_COMBO_TRAINPY, spec, L"train.py");
                } break;
                case IDC_BTN_BROWSE_YAML: {
                    COMDLG_FILTERSPEC spec[] = { {L"YAML (*.yaml;*.yml)", L"*.yaml;*.yml"} };
                    //std::wstring p; if (PickFile(hDlg, spec, 1, p)) { SetComboText(GetDlgItem(hDlg, IDC_COMBO_YAML), p); }
                    PickFileEx(hDlg, IDC_COMBO_YAML, spec, L"Data.yaml(クラシフィケーション定義とデータ指定のあるファイル)");
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
                    //std::wstring p; if (PickFile(hDlg, spec, 1, p)) { SetComboText(GetDlgItem(hDlg, IDC_COMBO_HYP), p); }
                    PickFileEx(hDlg, IDC_COMBO_HYP, spec, L"Hyp.yaml(ハイパーチューニング定義ファイル)");
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
                    //std::wstring p; if (PickFile(hDlg, spec, 1, p)) { SetComboText(GetDlgItem(hDlg, IDC_COMBO_CFG), p); }
                    PickFileEx(hDlg, IDC_COMBO_CFG, spec, L"Cfg.yaml(コンフィグレーション定義ファイル)");
                } break;

                case IDC_BTN_EDIT_CFG: {
                    std::wstring p = GetText(g_hDlg, IDC_COMBO_CFG);
                    if (OpenFileWithDefaultEditor(p)) {
                        // ついでに MRU の先頭へ
                        SaveMRU(L"cfg.yaml", p);
                    }
                } break;

                case IDC_BTN_BROWSE_WEIGHTS: {
                    COMDLG_FILTERSPEC spec[] = { {L"Weight (*.pt)", L"*.pt"} };
                    //std::wstring p; if (PickFile(hDlg, spec, 1, p)) { SetComboText(GetDlgItem(hDlg, IDC_COMBO_WEIGHTS), p); }
                    PickFileEx(hDlg, IDC_COMBO_WEIGHTS, spec, L"weight.pt(初期重みファイル)");
                } break;

                case IDC_BTN_RESUME_BROWSE: {
                    COMDLG_FILTERSPEC spec[] = { {L"Resume (*.pt)", L"*.pt"} };
                    //std::wstring p; if (PickFile(hDlg, spec, 1, p)) { SetComboText(GetDlgItem(hDlg, IDC_CMB_RESUME), p); }
                    PickFileEx(hDlg, IDC_CMB_RESUME, spec, L"Resume.pt(トレーニング再開の時の初期重みファイル)");
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
                case IDC_BTN_CLEARTMP:
                {
                    if (HIWORD(wParam) != BN_CLICKED) return TRUE; // ノイズ排除
                    DoClearTemp(hDlg, IDC_COMBO_TEMP, /*confirm*/true, /*keepRoot*/false); // 従来通り「ルートごと削除」
                } break; //IDC_BTN_CLEARTMP

                case IDC_BTN_BROWSE_PYTHON: {
                    COMDLG_FILTERSPEC spec[] = { {L"Python executable (python.exe)", L"python.exe"} };
                    //std::wstring p; if (PickFile(hDlg, spec, 1, p)) { SetComboText(GetDlgItem(hDlg, IDC_COMBO_PYTHON), p); }
                    PickFileEx(hDlg, IDC_COMBO_PYTHON, spec, L"Python.exe(必要な時のみ)");
                } break;
                case IDC_BTN_BROWSE_ACTIVATE: {
                    COMDLG_FILTERSPEC spec[] = { {L"Batch (*.bat;*.cmd)", L"*.bat;*.cmd"} };
                    std::wstring p; if (PickFile(hDlg, spec, 1, p)) { SetComboText(GetDlgItem(hDlg, IDC_COMBO_ACTIVATE), p); }
                } break;
                case IDC_BTN_COPY: {
                    if (HIWORD(wParam) != BN_CLICKED)
                        return TRUE; // これ必須：押下以外は無視
                    std::thread([]() { DoCopyToTemp(); }).detach();
                }break;
                case IDC_BTN_SPLIT: {
                    if (HIWORD(wParam) != BN_CLICKED)
                        return TRUE; // これ必須：押下以外は無視
                    std::thread([]() { DoSplit(); }).detach();
                }break;
                case IDC_BTN_TRAIN: {
                    if (HIWORD(wParam) != BN_CLICKED) 
                        return TRUE; // これ必須：押下以外は無視
                    std::thread([hDlg]() { DoTrain(hDlg); }).detach();
                }break;
                case IDC_BTN_STOP: {
                    if (HIWORD(wParam) != BN_CLICKED)
                        return TRUE; // これ必須：押下以外は無視
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
                }break;
                case IDC_CHK_SPLIT_SHUFFLE:
                case IDC_RAD_YOLOV5:
                case IDC_RAD_YOLOV8:
                case IDC_RAD_YOLO11:
                case IDC_CHK_USEPROXY:
                case IDC_CHK_USEHYPERPARAM:
                case IDC_CHECK_RESUME:
                case IDC_CHK_OPTION_STR:
                {
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        Updated_UI(hDlg);
                        return TRUE;
                    }
                }break;

                case IDC_CHK_LOG_CRLF2LF:
                {
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        // チェック状態を取得
                        _IDC_CHK_LOG_CRLF2LF = (IsDlgButtonChecked(hDlg, IDC_CHK_LOG_CRLF2LF) == BST_CHECKED);
					}

                }break;

                //chk_crlf.pyを実行する
                case IDC_BTN_CHKCRLF2LF:
                    {
                    if (HIWORD(wParam) != BN_CLICKED) 
                        return TRUE; // ← これを追加
                    std::wstring python = GetText(hDlg, IDC_COMBO_PYTHON);
                    std::wstring script = GetText(hDlg, IDC_COMBO_ACTIVATE);
                    if (python.empty() || script.empty()) {
                        AppendLog(L"[CHKCRLF] Python or activate script is not set.");
                        AppendLog(RET);
                        return TRUE;
                    }
                    std::wstring cmd = L"activate " + script + +L" && "+ python + L" chk_crlf.py";
                    AppendLog(L"[CHKCRLF] " + cmd);
                    AppendLog(RET);
					LaunchWithCapture(cmd);
				}break;


                //過去のコマンドヒストリーを開く
                case IDC_BTN_EDITHISTORY:
                {
                    std::wstring p = GetHistoryFile();
                    OpenFileWithDefaultEditor(p);
                }break;

				//TensorBoardを開く http://localhost:6006/
                case IDC_BTN_OPENTENSORBOARD:
                {
                    if (HIWORD(wParam) != BN_CLICKED)
                        return TRUE; // ← これを追加
					StartTensorBoardDetached(GetText(hDlg, IDC_COMBO_WORKDIR), GetText(hDlg, IDC_COMBO_ACTIVATE),6006);
                    OpenURLWithDefaultBrowser(L"http://localhost:6006/");
				}break;

                //ログをクリアする
                case IDC_BTN_CLEARLOG:
                {
                    if (HIWORD(wParam) == BN_CLICKED) {
                        SetWindowTextW(GetDlgItem(hDlg, IDC_LOG), L"");
                    }
                }break;

                case IDC_BUTTON_APPLY_YAML:
                {
                    if (HIWORD(wParam) == BN_CLICKED) {
                        // 1) UIから文字列取得
                        wchar_t tempBuf[MAX_PATH * 4] = {};
                        wchar_t yamlBuf[MAX_PATH * 4] = {};
                        GetDlgItemTextW(hDlg, IDC_COMBO_TEMP, tempBuf, _countof(tempBuf));
                        GetDlgItemTextW(hDlg, IDC_COMBO_YAML, yamlBuf, _countof(yamlBuf));

                        // トリム（最低限）
                        std::wstring tempDir = tempBuf;
                        std::wstring yamlPathW = yamlBuf;

                        // 空チェック
                        if (tempDir.empty() || yamlPathW.empty()) {
                            //MessageBoxW(hDlg, L"TEMPまたはYAMLパスが空です。", L"エラー", MB_ICONERROR);
                            AppendLog(L"[YAML] TEMP or YAML path is empty.");
                            AppendLog(RET);
                            break;
                        }

                        // 末尾の \ / を除去（正規化しやすく）
                        while (!tempDir.empty() && (tempDir.back() == L'\\' || tempDir.back() == L'/')) tempDir.pop_back();

                        // 2) 実行
                        std::wstring log;
                        bool ok = UpdateYoloYamlTrainVal(std::filesystem::path(yamlPathW), tempDir, log);

                        // 3) 結果表示（ログやリッチエディットに追加）
                        if (ok) {
                            //MessageBoxW(hDlg, log.c_str(), L"YAML更新", MB_ICONINFORMATION);
                            AppendLog(L"[YAML] Updated YAML: " + yamlPathW);
                            AppendLog(RET);
                            // AppendToRichEdit(log); // 既存のログ関数があれば
                        }
                        else {
                            //MessageBoxW(hDlg, log.c_str(), L"YAML更新失敗", MB_ICONERROR);
                            AppendLog(L"[YAML] Failed to update YAML: " + yamlPathW);
                            AppendLog(RET);
                        }
                    }
                }break;

                default:
                    //break;
                    return false;
			}break; // switch (LOWORD(wParam))

            //case WM_DESTROY:
            //	g_hDlg = nullptr;
            //       SaveSettings(hDlg);
            //       PostQuitMessage(0);
            //	return TRUE;
            case WM_CLOSE:
            {
                TrainParams _params;
                // ここで設定をINIに保存
                _params.SaveCurrentSettingsToIni(hDlg); // hDlgが有効ならコントロールから値を保存 無効ならメモリ上の共有データを保存
                //SaveCurrentSettingsToIni(hDlg);

                StopChild();
                EndDialog(hDlg, 0);
                return TRUE;
            }
		}// WM_COMMAND

        case WM_APP_PROGRESS:  // ← WM_COMMAND の外でトップレベル
        {
            SetProgress((int)wParam); // 0～100
            return TRUE;              // 処理済み
        }

        case WM_APP_LOGAPPEND:
        {
            auto* p = reinterpret_cast<std::wstring*>(lParam);
            if (p) {
                // ※ここは UI スレッド。初めて RichEdit を触るのも UI スレッドだけ
                LogAppendANSI(*p);   // ← 既存の追記関数:contentReference[oaicite:3]{index=3}
                delete p;
            }
            return TRUE;
        }
        case WM_GETMINMAXINFO:
        {
            if (g_initCx > 0 && g_initCy > 0) 
            {
                LPMINMAXINFO p = (LPMINMAXINFO)lParam;
                // 追跡サイズは「ウィンドウ外枠」単位なので、クライアント→ウィンドウ差分を一度計算
                RECT rcWin{}, rcCli{};
                GetWindowRect(hDlg, &rcWin);
                GetClientRect(hDlg, &rcCli);
                const int dX = (rcWin.right - rcWin.left) - (rcCli.right - rcCli.left);
                const int dY = (rcWin.bottom - rcWin.top) - (rcCli.bottom - rcCli.top);
                const int winW = g_initCx + dX;
                const int winH_min = g_initCy + dY;

                // 横は最小=最大=初期幅 → 横方向のリサイズを事実上禁止
                p->ptMinTrackSize.x = winW;
                p->ptMaxTrackSize.x = 32767;// winW;

                // 縦は初期高さ以上（最大は任意に大きく）
                p->ptMinTrackSize.y = winH_min;
                p->ptMaxTrackSize.y = 32767;
            }
            return 0;
        }

        // 縦に伸びた分だけ IDC_LOG の高さを増やす（他は動かさない）
        case WM_SIZE:
        {
            if (wParam == SIZE_MINIMIZED) return TRUE;

            const int cx = LOWORD(lParam);
            const int cy = HIWORD(lParam);

            HWND hLog = GetDlgItem(hDlg, IDC_LOG);
            if (hLog && g_initCx > 0 && g_initCy > 0) {
                // 追加分
                const int deltaW = cx - g_initCx;                  // クライアント幅の増分
                int newW = g_logWidth + (deltaW > 0 ? deltaW : 0); // “拡大のみ”ならこのガード
                if (newW < g_logWidth) newW = g_logWidth;          // 縮小も許可したいならこの2行は外す

                const int deltaH = cy - g_initCy;                  // クライアント高さの増分
                int newH = g_logHeight + (deltaH > 0 ? deltaH : 0);
                if (newH < g_logHeight) newH = g_logHeight;

                SetWindowPos(hLog, nullptr,
                    g_logLeft, g_logTop,   // 左上は固定（左アンカー/上アンカー）
                    newW, newH,
                    SWP_NOZORDER | SWP_NOACTIVATE);
            }
            //if (hLog && g_initCy > 0) {
            //    const int deltaH = cy - g_initCy;                   // クライアント高さの増分
            //    int newH = g_logHeight + (deltaH > 0 ? deltaH : 0); // 縮小させたくないなら0で打ち止め
            //    if (newH < g_logHeight) newH = g_logHeight;         // “拡大のみ”にする場合

            //    // 横幅は固定（g_logWidth）、X/Yも固定（g_logLeft/g_logTop）
            //    SetWindowPos(hLog, nullptr,
            //        g_logLeft, g_logTop,
            //        g_logWidth, newH,
            //        SWP_NOZORDER | SWP_NOACTIVATE);
            //}
            return TRUE;
        }

        return FALSE;

	} // switch (msg)
	return FALSE; // デフォルトの処理へ
}// DlgProc

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
