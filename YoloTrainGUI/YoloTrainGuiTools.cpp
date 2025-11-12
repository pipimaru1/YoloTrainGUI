#define _WIN32_WINNT 0x0601
#include "pch.h"
#include "tools.hpp"
#include "resource.h"
//#include "resource_user.h"
#include "YoloTrainGUI.h"
#include "Tooltip.hpp"

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
            if (!dir.empty()) SetComboText(GetDlgItem(hDlg, IDC_CMB_TRAIN_SRC[idx]), dir);
            // 件数表示を更新
            UpdateFolderCounter(hDlg, IDC_CMB_TRAIN_SRC_0 + idx, IDC_STATIC_TRAIN_SRC_0 + idx);
        }
        else if (head == L"valid") {
            CheckDlgButton(hDlg, IDC_CHK_VALID_EN[idx], enable ? BST_CHECKED : BST_UNCHECKED);
            ApplyEnableFromCheck(hDlg, IDC_CHK_VALID_EN_0 + idx, IDC_CMB_VALID_SRC_0 + idx);
            if (!dir.empty()) SetComboText(GetDlgItem(hDlg, IDC_CMB_VALID_SRC[idx]), dir);
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
