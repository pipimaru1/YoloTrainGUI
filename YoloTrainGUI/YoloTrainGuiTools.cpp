#define _WIN32_WINNT 0x0601
#include "pch.h"
#include "tools.hpp"
#include "resource.h"
//#include "resource_user.h"
#include "YoloTrainGUI.h"
#include "Tooltip.hpp"



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
void PostLogToUi(const std::wstring& s) {
    if (!g_hDlg || s.empty()) return;
    auto* payload = new std::wstring(s);        // 後で UI スレッドが delete
    PostMessageW(g_hDlg, WM_APP_LOGAPPEND, 0, reinterpret_cast<LPARAM>(payload));
}

fs::path GetStoreDir() {
    PWSTR p = nullptr;
    SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, nullptr, &p);
    fs::path dir = fs::path(p) / L"YoloV5Trainer";
    CoTaskMemFree(p);
    std::error_code ec; fs::create_directories(dir, ec);
    return dir;
}
fs::path GetIniFile()
{
    //return GetStoreDir() / L"mru_history.ini"; 

    return L"mru_history.ini";
}
fs::path GetHistoryFile()
{
    //return GetStoreDir() / L"history.txt"; 

    return  L"history.txt";
}


// ------------------------------
// INI（コロン区切り）簡易パーサ
// data[section] = {value0, value1, ...}
// ------------------------------

std::map<std::wstring, std::vector<std::wstring>> ReadIniColon()
{
    std::map<std::wstring, std::vector<std::wstring>> data;
    std::lock_guard<std::mutex> lk(g_storeMutex);
    fs::path ini = GetIniFile();
    if (!fs::exists(ini)) 
        return data;

    std::ifstream ifs(ini, std::ios::binary);
    if (!ifs) 
        return data;
    std::string bytes((std::istreambuf_iterator<char>(ifs)), {});
    
    // strip UTF-8 BOM if present
    if (bytes.size() >= 3 && (unsigned char)bytes[0] == 0xEF && 
        (unsigned char)bytes[1] == 0xBB && 
        (unsigned char)bytes[2] == 0xBF)
    {
        bytes.erase(0, 3);
    }
    std::wstring text = FromUTF8(bytes);

    std::wstring cur;
    size_t pos = 0;
    while (pos < text.size()) {
        size_t eol = text.find_first_of(L"\r\n", pos);
        std::wstring line = text.substr(pos, (eol == std::wstring::npos ? text.size() - pos : eol - pos));
        if (eol != std::wstring::npos) {
            if (text[eol] == L'\r' && eol + 1 < text.size() && text[eol + 1] == L'\n') 
                eol++;
            pos = eol + 1;
        }
        else 
            pos = text.size();

        // trim
        auto ltrim = [&](std::wstring& s) { s.erase(0, s.find_first_not_of(L" \t")); };
        auto rtrim = [&](std::wstring& s) { size_t k = s.find_last_not_of(L" \t"); if (k == std::wstring::npos) s.clear(); else s.erase(k + 1); };
        ltrim(line); rtrim(line);
        if (line.empty() || line[0] == L';' || line[0] == L'#') 
            continue;

        if (line.size() > 2 && line.front() == L'[' && line.back() == L']') {
            cur = line.substr(1, line.size() - 2);
            continue;
        }
        if (cur.empty()) 
            continue;
        // "index:value"
        size_t colon = line.find(L':');
        if (colon == std::wstring::npos) 
            continue;
        std::wstring value = line.substr(colon + 1);
        ltrim(value); rtrim(value);
        if (!value.empty()) 
            data[cur].push_back(value);
    }
    return data;
}
void WriteIniColon(const std::map<std::wstring, std::vector<std::wstring>>& data)
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

// MRU（履歴）を「セクションごと消す」関数を追加
void ClearMRUSection(const std::wstring& section)
{
    auto m = ReadIniColon();
    auto it = m.find(section);
    if (it == m.end()) return;
    m.erase(it);
    WriteIniColon(m);
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

void SetProgress(int v)
{
    SendDlgItemMessageW(g_hDlg, IDC_PROGRESS, PBM_SETPOS, (WPARAM)v, 0);
}

void ResetProgress()
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

void LoadMRUToTextControl(HWND hEditText, const std::wstring& section)
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

///////////////////////////////////////////////////////////////////////////////////
// チェックボックスの状態に応じて、コンボボックスを有効化/無効化
///////////////////////////////////////////////////////////////////////////////////
void ApplyEnableFromCheck(HWND hDlg, UINT idChk, UINT idCombo)
{
    if (!hDlg) return;
    const BOOL enabled = (IsDlgButtonChecked(hDlg, idChk) == BST_CHECKED);
    if (HWND hCombo = GetDlgItem(hDlg, idCombo)) {
        EnableWindow(hCombo, enabled);
    }
}

/////////////////////////////////////////////////////////////////////////////////
// ディレクトリ以下の“ファイル数だけ”を数える（存在しなければ0）
//////////////////////////////////////////////////////////////////////////////////
//static 
uint64_t CountFilesUnder(const fs::path& root)
{
    std::error_code ec;
    if (!fs::exists(root, ec)) return 0;
    uint64_t cnt = 0;
    for (auto it = fs::recursive_directory_iterator(root, fs::directory_options::skip_permission_denied, ec);
        it != fs::recursive_directory_iterator(); ++it)
    {
        if (it->is_regular_file(ec)) ++cnt;
    }
    return cnt;
}

void UpdateFolderCounter(HWND hDlg, const UINT ID_COMBO, const UINT ID_STATICTXT)
{
    std::wstring root = GetComboText(GetDlgItem(hDlg, ID_COMBO));
    std::wstring text;
    if (!root.empty())
    {
        fs::path p(root);
        uint64_t nImg = CountFilesUnder(p / L"images");
        uint64_t nLbl = CountFilesUnder(p / L"labels");
        uint64_t nTot = nImg + nLbl;

        //text = std::to_wstring(nTot);
        text = std::to_wstring(nImg); // 画像数のみ表示
    }
    else {
        text = L"-";
    }
    SetDlgItemTextW(hDlg, ID_STATICTXT, text.c_str());
    UpdateAllFolderCounts(hDlg);
}

// IDC_STATIC_TRAIN_SRC_0 ~ IDC_STATIC_TRAIN_SRC_15 のスタティックコントロールに入っている
// 値を読取って、IDC_STATIC_TOTAL_TR に合計を表示する
// IDC_CHK_TRAIN_EN_0 ~ IDC_CHK_TRAIN_EN_15 チェックされていないものは無視する
// ループは使わない
void UpdateAllFolderCounts(HWND hDlg)
{
    //Train 側合計
    uint64_t total = 0;
    for (int i = 0; i < MAXDIR; ++i) {

        if (IsDlgButtonChecked(hDlg, IDC_CHK_TRAIN_EN[i]) != BST_CHECKED)
            continue; // チェックされていない行は無視

        std::wstring text;
        wchar_t buf[64] = {};
        GetDlgItemTextW(hDlg, IDC_STC_TRAIN_SRC[i], buf, _countof(buf));
        text = buf;
        if (!text.empty()) {
            try {
                uint64_t n = std::stoull(text);
                total += n;
            }
            catch (...) {
                // 変換失敗は無視
            }
        }
    }
    //Valid 側合計
    uint64_t total_vl = 0;
    for (int i = 0; i < MAXDIR; ++i) {
        if (IsDlgButtonChecked(hDlg, IDC_CHK_VALID_EN[i]) != BST_CHECKED)
            continue; // チェックされていない行は無視
        std::wstring text;
        wchar_t buf[64] = {};
        GetDlgItemTextW(hDlg, IDC_STC_VALID_SRC[i], buf, _countof(buf));
        text = buf;
        if (!text.empty()) {
            try {
                uint64_t n = std::stoull(text);
                total_vl += n;
            }
            catch (...) {
                // 変換失敗は無視
            }
        }
    }
    SetDlgItemTextW(hDlg, IDC_STATIC_TOTAL_TR, std::to_wstring(total).c_str());
    SetDlgItemTextW(hDlg, IDC_STATIC_TOTAL_VL, std::to_wstring(total_vl).c_str());
}
//テンポラリフォルダ用Train
void UpdateFolderCounter_Train(HWND hDlg, const UINT ID_COMBO, const UINT ID_STATICTXT)
{
    std::wstring root = GetComboText(GetDlgItem(hDlg, ID_COMBO));
    std::wstring text;
    if (!root.empty())
    {
        fs::path p(root);
        uint64_t nImg = CountFilesUnder(p / L"dataset" / L"train" / L"images");
        uint64_t nLbl = CountFilesUnder(p / L"dataset" / L"train" / L"labels");
        uint64_t nTot = nImg + nLbl;

        //text = std::to_wstring(nTot);
        text = std::to_wstring(nImg); // 画像数のみ表示
    }
    else {
        text = L"-";
    }
    SetDlgItemTextW(hDlg, ID_STATICTXT, text.c_str());
}
//テンポラリフォルダ用Valid
void UpdateFolderCounter_Valid(HWND hDlg, const UINT ID_COMBO, const UINT ID_STATICTXT)
{
    std::wstring root = GetComboText(GetDlgItem(hDlg, ID_COMBO));
    std::wstring text;
    if (!root.empty())
    {
        fs::path p(root);
        uint64_t nImg = CountFilesUnder(p / L"dataset" / L"valid" / L"images");
        uint64_t nLbl = CountFilesUnder(p / L"dataset" / L"valid" / L"labels");
        uint64_t nTot = nImg + nLbl;

        //text = std::to_wstring(nTot);
        text = std::to_wstring(nImg); // 画像数のみ表示
    }
    else {
        text = L"-";
    }
    SetDlgItemTextW(hDlg, ID_STATICTXT, text.c_str());
}



//////////////////////////////////////////////////////////
//
// フォルダリスト保存・インポート
//
//////////////////////////////////////////////////////////

std::string ToUTF8Local(const std::wstring& w) {
    if (w.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), s.data(), len, nullptr, nullptr);
    return s;
}
std::wstring FromUTF8Local(const std::string& s) {
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), w.data(), len);
    return w;
}

bool SaveFileDialogCSV(HWND hOwner, std::wstring& outPath) {
    wchar_t fileBuf[MAX_PATH] = L"list.csv";
    OPENFILENAMEW ofn{ sizeof(ofn) };
    ofn.hwndOwner = hOwner;
    ofn.lpstrFilter = L"CSV (*.csv)\0*.csv\0All Files (*.*)\0*.*\0\0";
    ofn.lpstrFile = fileBuf;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = L"csv";
    if (GetSaveFileNameW(&ofn)) { outPath = fileBuf; return true; }
    return false;
}
bool OpenFileDialogCSV(HWND hOwner, std::wstring& outPath) {
    wchar_t fileBuf[MAX_PATH] = L"";
    OPENFILENAMEW ofn{ sizeof(ofn) };
    ofn.hwndOwner = hOwner;
    ofn.lpstrFilter = L"CSV (*.csv)\0*.csv\0All Files (*.*)\0*.*\0\0";
    ofn.lpstrFile = fileBuf;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;
    if (GetOpenFileNameW(&ofn)) { outPath = fileBuf; return true; }
    return false;
}

//////////////////////////////////////////////////////////////////////////
//
// Export list
//
//////////////////////////////////////////////////////////////////////////


bool ExportMultiCopyListToCSV(HWND hDlg) {
    std::wstring path;
    if (!SaveFileDialogCSV(hDlg, path)) return false;

    // 1) 行データ収集（0..15）
    std::wostringstream wout;
    for (int i = 0; i < MAXDIR; ++i) {
        // Train
        const BOOL enT = (IsDlgButtonChecked(hDlg, IDC_CHK_TRAIN_EN[i]) == BST_CHECKED);
        const std::wstring dirT = GetComboText(GetDlgItem(hDlg, IDC_CMB_TRAIN_SRC[i]));
        wout << L"Train" << std::setw(2) << std::setfill(L'0') << i
            << L", " << (enT ? L"1" : L"0") << L", " << dirT << L"\r\n";
    }

    for (int i = 0; i < MAXDIR; ++i) {
        // Valid
        const BOOL enV = (IsDlgButtonChecked(hDlg, IDC_CHK_VALID_EN[i]) == BST_CHECKED);
        const std::wstring dirV = GetComboText(GetDlgItem(hDlg, IDC_CMB_VALID_SRC[i]));
        wout << L"Valid" << std::setw(2) << std::setfill(L'0') << i
            << L", " << (enV ? L"1" : L"0") << L", " << dirV << L"\r\n";
    }

    // 2) UTF-8(BOM付)で保存
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    const unsigned char bom[3] = { 0xEF,0xBB,0xBF };
    ofs.write((const char*)bom, 3);
    const std::string u8 = ToUTF8Local(wout.str());
    ofs.write(u8.data(), (std::streamsize)u8.size());
    return true;
}

//////////////////////////////////////////////////////////////////////////
//
// Import list
//
//////////////////////////////////////////////////////////////////////////

inline std::wstring TrimW(std::wstring s) {
    const wchar_t* ws = L" \t\r\n";
    const size_t b = s.find_first_not_of(ws);
    if (b == std::wstring::npos) return L"";
    const size_t e = s.find_last_not_of(ws);
    return s.substr(b, e - b + 1);
}
bool ImportMultiCopyListFromCSV(HWND hDlg) {
    std::wstring path;
    if (!OpenFileDialogCSV(hDlg, path)) return false;

    // 1) 読み込み（UTF-8/BOM対応）
    std::ifstream ifs(path, std::ios::binary);
    std::string bytes((std::istreambuf_iterator<char>(ifs)), {});
    if (bytes.size() >= 3 && (unsigned char)bytes[0] == 0xEF && (unsigned char)bytes[1] == 0xBB && (unsigned char)bytes[2] == 0xBF)
        bytes.erase(0, 3);
    std::wstring text = FromUTF8Local(bytes);

    // 2) 解析
    std::wstringstream ss(text);
    std::wstring line;
    while (std::getline(ss, line)) {
        line = TrimW(line);
        if (line.empty()) continue;

        // フィールド分割 "xxx, y, zzz"
        std::vector<std::wstring> cols;
        size_t pos = 0;
        while (pos != std::wstring::npos) {
            size_t comma = line.find(L',', pos);
            std::wstring token = (comma == std::wstring::npos) ? line.substr(pos) : line.substr(pos, comma - pos);
            cols.push_back(TrimW(token));
            if (comma == std::wstring::npos) break;
            pos = comma + 1;
        }
        if (cols.size() < 3) continue;

        std::wstring tag = cols[0]; // "Train00" / "Valid01" / "Vaild01"
        if (tag.size() < 6) continue;

        // 種別/番号
        std::wstring head = tag.substr(0, 5); // "Train" or "Valid"（"Vaild"対応）
        std::wstring idxs = tag.substr(tag.size() - 2); // "00".."15"
        int idx = _wtoi(idxs.c_str());
        if (idx < 0 || idx >= MAXDIR) continue;

        // 表記ゆれ補正
        for (auto& ch : head) ch = (wchar_t)towlower(ch);
        if (head == L"vaild") head = L"valid";

        // enable と パス
        const int enable = _wtoi(cols[1].c_str());
        const std::wstring dir = cols[2];

        if (head == L"train") {
            CheckDlgButton(hDlg, IDC_CHK_TRAIN_EN[idx], enable ? BST_CHECKED : BST_UNCHECKED);
            ApplyEnableFromCheck(hDlg, IDC_CHK_TRAIN_EN_0 + idx, IDC_CMB_TRAIN_SRC_0 + idx);
            if (!dir.empty()) 
                SetComboText(GetDlgItem(hDlg, IDC_CMB_TRAIN_SRC[idx]), dir);
            // 件数表示を更新
            UpdateFolderCounter(hDlg, IDC_CMB_TRAIN_SRC_0 + idx, IDC_STATIC_TRAIN_SRC_0 + idx);
        }
        else if (head == L"valid") {
            CheckDlgButton(hDlg, IDC_CHK_VALID_EN[idx], enable ? BST_CHECKED : BST_UNCHECKED);
            ApplyEnableFromCheck(hDlg, IDC_CHK_VALID_EN_0 + idx, IDC_CMB_VALID_SRC_0 + idx);
            if (!dir.empty()) 
                SetComboText(GetDlgItem(hDlg, IDC_CMB_VALID_SRC[idx]), dir);
            // 件数表示を更新
            UpdateFolderCounter(hDlg, IDC_CMB_VALID_SRC_0 + idx, IDC_STATIC_VALID_SRC_0 + idx);
        }
    }

    // Temp側の合計も更新
    UpdateAllFolderCounts(hDlg);
    UpdateFolderCounter_Train(hDlg, IDC_COMBO_TEMP_MCOPY, IDC_STATIC_TMP_TRAIN);
    UpdateFolderCounter_Valid(hDlg, IDC_COMBO_TEMP_MCOPY, IDC_STATIC_TMP_VALID);
    return true;
}

// ------------------------------
// Command history
// ------------------------------
void AppendCmdHistory(const std::wstring& cmd)
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
void EnableControls(HWND hDlg, const int* ids, size_t n, BOOL enable)
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
void Updated_UI(HWND hDlg)
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


// 拡張版：初期フォルダ指定、タイトル変更対応
bool PickFolderEx1(HWND owner, std::wstring& outPath, std::wstring initialDir, std::wstring _title)
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
bool PickFolderEx2(HWND hDlg, UINT _ComboID, std::wstring _title)
{
    std::wstring _outPath;
    std::wstring _initdir = GetComboText(GetDlgItem(hDlg, _ComboID));

    if (PickFolderEx1(hDlg, _outPath, _initdir, _title))
    {
        SetComboText(GetDlgItem(hDlg, _ComboID), _outPath);
        return true;
    }

    return false;
}


/////////////////////////////////////////////////////////////////
// コンボボックスの MRU 更新ユーティリティ
void CommitComboMRU(HWND hCombo, const std::wstring& section, const std::wstring& value)
{
    if (!hCombo || value.empty()) return;

    SaveMRU(section, value);

    // ちらつき防止
    SendMessageW(hCombo, WM_SETREDRAW, FALSE, 0);

    LoadMRUToCombo(hCombo, section);

    // 追加した値を選択＆見える位置へ（CBS_SORTでも「見失わない」）
    int idx = (int)SendMessageW(hCombo, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)value.c_str());
    if (idx != CB_ERR) {
        SendMessageW(hCombo, CB_SETCURSEL, idx, 0);
        SendMessageW(hCombo, CB_SETTOPINDEX, idx, 0);   // ←重要（高さが小さい/ソートされる対策）
    }
    else {
        // 念のため
        SetComboText(hCombo, value);
    }

    SendMessageW(hCombo, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hCombo, nullptr, TRUE);
}

//上記関数のラッパー さらに MRU 保存も行う版
bool PickFolderEx3(HWND hDlg, UINT comboID, std::wstring title, std::wstring secStr)
{
    HWND hCombo = GetDlgItem(hDlg, comboID);
    std::wstring initdir = GetComboText(hCombo);

    std::wstring value;
    if (PickFolderEx1(hDlg, value, initdir, title))  // 既存の「パスを返す」PickFolderEx
    {
        CommitComboMRU(hCombo, secStr, value);
        SendMessageW(hCombo, CB_SETEDITSEL, 0, 0);
        return true;
    }
    return false;
}

//////////////////////////////////////
//
// MRU 保存・読み込みユーティリティ
// ここに書くのかが適切かどうかは不明
// 
//////////////////////////////////////
int LoadIntFromIni(const wchar_t* key, int defValue)
{
    auto v = LoadMRU(key);
    if (v.empty()) return defValue;
    try {
        return std::stoi(v.front());
    }
    catch (...) {
        return defValue;
    }
}
//上記関数のラッパー さらに MRU 保存も行う版
//bool PickFolderEx(HWND hDlg, UINT _ComboID, std::wstring _title, const wchar_t* _SECSTR)
//{
//    std::wstring _value;
//	HWND hCombo = GetDlgItem(hDlg, _ComboID);
//    //std::wstring _initdir = GetComboText(GetDlgItem(hDlg, _ComboID));
//    std::wstring _initdir = GetComboText(hCombo);
//
//    if (PickFolderEx(hDlg, _value, _initdir, _title))
//    {
//        SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)_value.c_str());
//        //SetComboText(hCombo, _value);
//        SaveMRU(_SECSTR, _value);
//        LoadMRUToCombo(hCombo, _SECSTR);
//        SetComboText(hCombo, _value);
//        SendMessageW(GetDlgItem(hDlg, _ComboID), CB_SETEDITSEL, 0, 0);
//
//        return true;
//    }
//    return false;
//}


