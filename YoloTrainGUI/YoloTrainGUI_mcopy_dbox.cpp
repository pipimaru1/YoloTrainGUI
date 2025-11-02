#define _WIN32_WINNT 0x0601
#include "pch.h"

#include "tools.hpp"

#include "resource.h"
#include "resource_user.h"

#include "YoloTrainGUI.h"
#include "Tooltip.hpp"

// 例: 冒頭付近にID配列を置く
static const UINT IDC_CMB_TRAIN_SRC[8] = {
    IDC_CMB_TRAIN_SRC_0, IDC_CMB_TRAIN_SRC_1, IDC_CMB_TRAIN_SRC_2, IDC_CMB_TRAIN_SRC_3,
    IDC_CMB_TRAIN_SRC_4, IDC_CMB_TRAIN_SRC_5, IDC_CMB_TRAIN_SRC_6, IDC_CMB_TRAIN_SRC_7,
};
static const UINT IDC_CMB_VALID_SRC[8] = {
    IDC_CMB_VALID_SRC_0, IDC_CMB_VALID_SRC_1, IDC_CMB_VALID_SRC_2, IDC_CMB_VALID_SRC_3,
    IDC_CMB_VALID_SRC_4, IDC_CMB_VALID_SRC_5, IDC_CMB_VALID_SRC_6, IDC_CMB_VALID_SRC_7,
};
// ★各行の「Train用」「Valid用」チェックを独立IDに
static const UINT IDC_CHK_TRAIN_EN[8] = {
    IDC_CHK_TRAIN_EN_0, IDC_CHK_TRAIN_EN_1, IDC_CHK_TRAIN_EN_2, IDC_CHK_TRAIN_EN_3,
    IDC_CHK_TRAIN_EN_4, IDC_CHK_TRAIN_EN_5, IDC_CHK_TRAIN_EN_6, IDC_CHK_TRAIN_EN_7,
};
static const UINT IDC_CHK_VALID_EN[8] = {
    IDC_CHK_VALID_EN_0, IDC_CHK_VALID_EN_1, IDC_CHK_VALID_EN_2, IDC_CHK_VALID_EN_3,
    IDC_CHK_VALID_EN_4, IDC_CHK_VALID_EN_5, IDC_CHK_VALID_EN_6, IDC_CHK_VALID_EN_7,
};

// スタティック表示先のID配列（既存のコンボ配列とペア）
static const UINT IDC_STC_TRAIN_SRC[8] = {
    IDC_STATIC_TRAIN_SRC_0, IDC_STATIC_TRAIN_SRC_1, IDC_STATIC_TRAIN_SRC_2, IDC_STATIC_TRAIN_SRC_3,
    IDC_STATIC_TRAIN_SRC_4, IDC_STATIC_TRAIN_SRC_5, IDC_STATIC_TRAIN_SRC_6, IDC_STATIC_TRAIN_SRC_7,
};
static const UINT IDC_STC_VALID_SRC[8] = {
    IDC_STATIC_VALID_SRC_0, IDC_STATIC_VALID_SRC_1, IDC_STATIC_VALID_SRC_2, IDC_STATIC_VALID_SRC_3,
    IDC_STATIC_VALID_SRC_4, IDC_STATIC_VALID_SRC_5, IDC_STATIC_VALID_SRC_6, IDC_STATIC_VALID_SRC_7,
};


int MultiCopyParams::SaveCurrentSettingsToIni(HWND hDlg)
{
    if (hDlg != nullptr)
    {
        SaveMRU(L"Temp Dir", src_dir);

        // 2) INI 保存（現在値＋各フラグ）
        for (int i = 0; i < 8; ++i)
        {
            // 現在の選択（train/valid）
            SaveMRU(L"Train Data " + std::to_wstring(i), src_dir_trains[i]);
            SaveMRU(L"Valid Data " + std::to_wstring(i), src_dir_valids[i]);

            // 使う/使わない（train/valid 独立）
            SaveMRU(L"Use Train Data " + std::to_wstring(i), src_dir_train_chks[i] ? L"1" : L"0");
            SaveMRU(L"Use Valid Data " + std::to_wstring(i), src_dir_valid_chks[i] ? L"1" : L"0");
        }
/*
        SaveMRU(L"Train Data 0", src_dir_trains[0]);
        SaveMRU(L"Train Data 1", src_dir_trains[1]);
        SaveMRU(L"Train Data 2", src_dir_trains[2]);
        SaveMRU(L"Train Data 3", src_dir_trains[3]);
        SaveMRU(L"Train Data 4", src_dir_trains[4]);
        SaveMRU(L"Train Data 5", src_dir_trains[5]);
        SaveMRU(L"Train Data 6", src_dir_trains[6]);
        SaveMRU(L"Train Data 7", src_dir_trains[7]);

        SaveMRU(L"Valid Data 0", src_dir_valids[0]);
        SaveMRU(L"Valid Data 1", src_dir_valids[1]);
        SaveMRU(L"Valid Data 2", src_dir_valids[2]);
        SaveMRU(L"Valid Data 3", src_dir_valids[3]);
        SaveMRU(L"Valid Data 4", src_dir_valids[4]);
        SaveMRU(L"Valid Data 5", src_dir_valids[5]);
        SaveMRU(L"Valid Data 6", src_dir_valids[6]);
        SaveMRU(L"Valid Data 7", src_dir_valids[7]);

        SaveMRU(L"Use Data 0", src_dir_train_chks[0] ? L"1" : L"0");
        SaveMRU(L"Use Data 1", src_dir_train_chks[1] ? L"1" : L"0");
        SaveMRU(L"Use Data 2", src_dir_train_chks[2] ? L"1" : L"0");
        SaveMRU(L"Use Data 3", src_dir_train_chks[3] ? L"1" : L"0");
        SaveMRU(L"Use Data 4", src_dir_train_chks[4] ? L"1" : L"0");
        SaveMRU(L"Use Data 5", src_dir_train_chks[5] ? L"1" : L"0");
        SaveMRU(L"Use Data 6", src_dir_train_chks[6] ? L"1" : L"0");
        SaveMRU(L"Use Data 7", src_dir_train_chks[7] ? L"1" : L"0");
*/
    }
    return 0;
}

#include "tools.hpp" // GetComboText, SetComboText

int MultiCopyParams::ReadControls(HWND hDlg)
{
    if (!hDlg) 
        return -1;

    src_dir = GetText(hDlg, IDC_COMBO_TEMP_MCOPY);

    // ★ここを実プロジェクトのIDに置き換え
    static const UINT CMB_TRAIN[8] = {
        IDC_CMB_TRAIN_SRC_0, 
        IDC_CMB_TRAIN_SRC_1, 
        IDC_CMB_TRAIN_SRC_2, 
        IDC_CMB_TRAIN_SRC_3, 
        IDC_CMB_TRAIN_SRC_4, 
        IDC_CMB_TRAIN_SRC_5, 
        IDC_CMB_TRAIN_SRC_6, 
        IDC_CMB_TRAIN_SRC_7
    };
    static const UINT CMB_VALID[8] = {
        IDC_CMB_VALID_SRC_0,
        IDC_CMB_VALID_SRC_1,
        IDC_CMB_VALID_SRC_2,
        IDC_CMB_VALID_SRC_3,
        IDC_CMB_VALID_SRC_4,
        IDC_CMB_VALID_SRC_5,
        IDC_CMB_VALID_SRC_6,
        IDC_CMB_VALID_SRC_7
    };
    // 行の有効チェック（1行＝Train/Valid のペアをまとめてON/OFFする想定）
    static const UINT CHK_USE_TRAIN[8] = {
        IDC_CHK_TRAIN_EN_0,
        IDC_CHK_TRAIN_EN_1,
        IDC_CHK_TRAIN_EN_2,
        IDC_CHK_TRAIN_EN_3,
        IDC_CHK_TRAIN_EN_4,
        IDC_CHK_TRAIN_EN_5,
        IDC_CHK_TRAIN_EN_6,
        IDC_CHK_TRAIN_EN_7
    };

    // 行の有効チェック（1行＝Train/Valid のペアをまとめてON/OFFする想定）
    static const UINT CHK_USE_VALID[8] = {
        IDC_CHK_VALID_EN_0,
        IDC_CHK_VALID_EN_1,
        IDC_CHK_VALID_EN_2,
        IDC_CHK_VALID_EN_3,
        IDC_CHK_VALID_EN_4,
        IDC_CHK_VALID_EN_5,
        IDC_CHK_VALID_EN_6,
        IDC_CHK_VALID_EN_7
    };

    // 念のためサイズを保証
    if (src_dir_trains.size() != 8) src_dir_trains.resize(8);
    if (src_dir_valids.size() != 8) src_dir_valids.resize(8);
    if (src_dir_train_chks.size() != 8) src_dir_train_chks.resize(8);
    if (src_dir_valid_chks.size() != 8) src_dir_valid_chks.resize(8);

    int enabledCount = 0;

    for (int i = 0; i < 8; ++i)
    {
        // --- Train 側 ---
        if (CMB_TRAIN[i] != 0) {
            HWND hCmbTrain = GetDlgItem(hDlg, CMB_TRAIN[i]);
            src_dir_trains[i].clear();
            if (hCmbTrain) {
                std::wstring s = GetComboText(hCmbTrain);
                // 必要なら前後空白カット（プロジェクトにトリム関数が無ければそのまま）
                // TrimInPlace(s);
                src_dir_trains[i] = std::move(s);
            }
        }
        else {
            src_dir_trains[i].clear();
        }

        // --- Valid 側 ---
        if (CMB_VALID[i] != 0) {
            HWND hCmbValid = GetDlgItem(hDlg, CMB_VALID[i]);
            src_dir_valids[i].clear();
            if (hCmbValid) {
                std::wstring s = GetComboText(hCmbValid);
                // TrimInPlace(s);
                src_dir_valids[i] = std::move(s);
            }
        }
        else {
            src_dir_valids[i].clear();
        }

        // --- 行の有効チェック ---
        bool use_train = false;
        if (CHK_USE_TRAIN[i] != 0) 
        {
            use_train = (IsDlgButtonChecked(hDlg, CHK_USE_TRAIN[i]) == BST_CHECKED);
        }
        src_dir_train_chks[i] = use_train;
        // 統計：有効で、どちらかにパスが入っていればカウント
        if (use_train && (!src_dir_trains[i].empty() )) {
            ++enabledCount;
        }

        bool use_valid = false;
        if (CHK_USE_VALID[i] != 0)
        {
            use_valid = (IsDlgButtonChecked(hDlg, CHK_USE_VALID[i]) == BST_CHECKED);
        }
        src_dir_valid_chks[i] = use_valid;
        // 統計：有効で、どちらかにパスが入っていればカウント
        if (use_valid && (!src_dir_valids[i].empty())) 
        {
            ++enabledCount;
        }
    }
    return enabledCount; // 利用可能行数（不要なら 0 固定でOK）
}

//////////////////////////////////////////////////////////////////////////////////////
// 
// コピー実装
// 
//////////////////////////////////////////////////////////////////////////////////////
//#include <filesystem>
//#include <atomic>
//namespace fs = std::filesystem;

// 無ければ簡易EnsureDir
static inline void EnsureDirIfMissing(const fs::path& p) {
    std::error_code ec; fs::create_directories(p, ec);
}

// 進捗更新
static inline void CopyProgress(HWND hDlg, UINT idPrg, uint64_t done, uint64_t total) {
    if (total == 0) { SendDlgItemMessageW(hDlg, idPrg, PBM_SETPOS, 0, 0); return; }
    int pct = (int)((done * 100) / total);
    if (pct < 0) pct = 0; if (pct > 100) pct = 100;
    SendDlgItemMessageW(hDlg, idPrg, PBM_SETPOS, pct, 0);
}

// サブツリー（src 以下）を dst に再帰コピー（上書き）
static bool CopySubtreeWithProgress(const fs::path& src, const fs::path& dst,
    std::atomic<uint64_t>& done, uint64_t total, HWND hDlg)
{
    if (!fs::exists(src)) return true; // 無ければスキップ
    EnsureDirIfMissing(dst);
    std::error_code ec;

    for (auto it = fs::recursive_directory_iterator(
        src, fs::directory_options::skip_permission_denied, ec);
        it != fs::recursive_directory_iterator(); ++it)
    {
        const auto& p = it->path();
        if (it->is_directory(ec)) continue;
        if (!it->is_regular_file(ec)) continue;

        auto rel = fs::relative(p, src, ec);
        fs::path out = dst / rel;

        fs::create_directories(out.parent_path(), ec);
        fs::copy_file(p, out, fs::copy_options::overwrite_existing, ec);

        uint64_t v = ++done;
        CopyProgress(hDlg, IDC_PROGRESS_COPY, v, total);
    }
    return true;
}

// フォルダが存在すれば、その直下の images / labels の総ファイル数を数える
static uint64_t CountImagesLabels(const fs::path& root) {
    uint64_t total = 0;
    std::error_code ec;
    auto countDir = [&](const fs::path& d) {
        if (!fs::exists(d, ec)) return;
        for (auto it = fs::recursive_directory_iterator(
            d, fs::directory_options::skip_permission_denied, ec);
            it != fs::recursive_directory_iterator(); ++it)
        {
            if (it->is_regular_file(ec)) ++total;
        }
        };
    countDir(root / L"images");
    countDir(root / L"labels");
    return total;
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

////////////////////////////////////////////////////////////////////////////////
// 1行分（Train/Valid それぞれ）の表示を更新
////////////////////////////////////////////////////////////////////////////////
//static 
/*
void UpdateFolderCountRow(HWND hDlg, int rowIndex)
{
    if (rowIndex < 0 || rowIndex >= 8) 
        return;

    // ---- Train 側 ----
    {
        std::wstring root = GetComboText(GetDlgItem(hDlg, IDC_CMB_TRAIN_SRC[rowIndex]));
        std::wstring text;
        if (!root.empty())
        {
            fs::path p(root);
            uint64_t nImg = CountFilesUnder(p / L"images");
            uint64_t nLbl = CountFilesUnder(p / L"labels");
            uint64_t nTot = nImg + nLbl;
            text = L"images: " + std::to_wstring(nImg)
                + L"  labels: " + std::to_wstring(nLbl)
                + L"  total: " + std::to_wstring(nTot);
        }
        else {
            text = L"-";
        }
        SetDlgItemTextW(hDlg, IDC_STC_TRAIN_SRC[rowIndex], text.c_str());
    }

    // ---- Valid 側 ----
    {
        std::wstring root = GetComboText(GetDlgItem(hDlg, IDC_CMB_VALID_SRC[rowIndex]));
        std::wstring text;
        if (!root.empty())
        {
            fs::path p(root);
            uint64_t nImg = CountFilesUnder(p / L"images");
            uint64_t nLbl = CountFilesUnder(p / L"labels");
            uint64_t nTot = nImg + nLbl;
            text = L"images: " + std::to_wstring(nImg)
                + L"  labels: " + std::to_wstring(nLbl)
                + L"  total: " + std::to_wstring(nTot);
        }
        else {
            text = L"-";
        }
        SetDlgItemTextW(hDlg, IDC_STC_VALID_SRC[rowIndex], text.c_str());
    }
}
// 全行をまとめて更新
static void UpdateAllFolderCounts(HWND hDlg)
{
    for (int i = 0; i < 8; ++i) {
        UpdateFolderCountRow(hDlg, i);
    }
}

*/

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
        text = std::to_wstring(nTot);
    }
    else {
        text = L"-";
    }
    SetDlgItemTextW(hDlg, ID_STATICTXT, text.c_str());
}




/////////////////////////////////////////////////////////////////////////////////
// メインウィンドウへログ送信
/////////////////////////////////////////////////////////////////////////////////
static void LogToMain(const std::wstring& s) {
    if (!g_hDlg) return;
    auto* payload = new std::wstring(s);               // UI側で delete される
    PostMessageW(g_hDlg, WM_APP_LOGAPPEND, 0, (LPARAM)payload);
}

// 新規ファイルでも既存 .cpp 末尾でもOK
INT_PTR CALLBACK CopyMultiDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            // 1) プログレスバー初期化
            if (HWND hPrg = GetDlgItem(hDlg, IDC_PROGRESS_COPY)) {
                SendMessageW(hPrg, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
                SendMessageW(hPrg, PBM_SETPOS, 0, 0);
            }

            // 2) 各行を初期化（MRU投入 → 先頭値を現在値に → CueBanner → チェック復元）
            for (int i = 0; i < 8; ++i)
            {
                // --- Train側コンボ ---
                if (HWND hCmbT = GetDlgItem(hDlg, IDC_CMB_TRAIN_SRC[i])) {
                    const std::wstring secT = L"Train Data " + std::to_wstring(i);
                    LoadMRUToCombo(hCmbT, secT);                                       // MRU → コンボ投入
                    auto vt = LoadMRU(secT); if (!vt.empty()) SetComboText(hCmbT, vt.front()); // 先頭MRUを現在値へ
                    SendMessageW(hCmbT, CB_SETCUEBANNER, TRUE, (LPARAM)L"Train root (images / labels)");
                }

                // --- Valid側コンボ ---
                if (HWND hCmbV = GetDlgItem(hDlg, IDC_CMB_VALID_SRC[i])) {
                    const std::wstring secV = L"Valid Data " + std::to_wstring(i);
                    LoadMRUToCombo(hCmbV, secV);
                    auto vv = LoadMRU(secV); if (!vv.empty()) SetComboText(hCmbV, vv.front());
                    SendMessageW(hCmbV, CB_SETCUEBANNER, TRUE, (LPARAM)L"Valid root (images / labels)");
                }

                // 3) チェックボックス復元（train/valid を独立キーで）
                if (HWND hChkT = GetDlgItem(hDlg, IDC_CHK_TRAIN_EN[i])) {
                    const std::wstring keyT = L"Use Train Data " + std::to_wstring(i);
                    const bool useT = LoadFlagFromIni(keyT.c_str(), false);
                    CheckDlgButton(hDlg, IDC_CHK_TRAIN_EN[i], useT ? BST_CHECKED : BST_UNCHECKED);
                }
                if (HWND hChkV = GetDlgItem(hDlg, IDC_CHK_VALID_EN[i])) {
                    const std::wstring keyV = L"Use Valid Data " + std::to_wstring(i);
                    const bool useV = LoadFlagFromIni(keyV.c_str(), false);
                    CheckDlgButton(hDlg, IDC_CHK_VALID_EN[i], useV ? BST_CHECKED : BST_UNCHECKED);
                }
            }

            //LoadMRUToCombo(GetDlgItem(hDlg, IDC_COMBO_MCOPY_TEMP), L"Temp Dir");
            if (HWND hCmbV = GetDlgItem(hDlg, IDC_COMBO_TEMP_MCOPY)) {
                const std::wstring secV = L"Temp Dir";
                LoadMRUToCombo(hCmbV, secV);
                auto vv = LoadMRU(secV); if (!vv.empty()) SetComboText(hCmbV, vv.front());
                SendMessageW(hCmbV, CB_SETCUEBANNER, TRUE, (LPARAM)L"Temp (images / labels)");
            }

            // 末尾でカウント表示を初期化
            //UpdateAllFolderCounts(hDlg);
            UpdateFolderCounter(hDlg, IDC_CMB_TRAIN_SRC_0, IDC_STATIC_TRAIN_SRC_0);
            UpdateFolderCounter(hDlg, IDC_CMB_TRAIN_SRC_1, IDC_STATIC_TRAIN_SRC_1);
            UpdateFolderCounter(hDlg, IDC_CMB_TRAIN_SRC_2, IDC_STATIC_TRAIN_SRC_2);
            UpdateFolderCounter(hDlg, IDC_CMB_TRAIN_SRC_3, IDC_STATIC_TRAIN_SRC_3);
            UpdateFolderCounter(hDlg, IDC_CMB_TRAIN_SRC_4, IDC_STATIC_TRAIN_SRC_4);
            UpdateFolderCounter(hDlg, IDC_CMB_TRAIN_SRC_5, IDC_STATIC_TRAIN_SRC_5);
            UpdateFolderCounter(hDlg, IDC_CMB_TRAIN_SRC_6, IDC_STATIC_TRAIN_SRC_6);
            UpdateFolderCounter(hDlg, IDC_CMB_TRAIN_SRC_7, IDC_STATIC_TRAIN_SRC_7);

            UpdateFolderCounter(hDlg, IDC_CMB_VALID_SRC_0, IDC_STATIC_VALID_SRC_0);
			UpdateFolderCounter(hDlg, IDC_CMB_VALID_SRC_1, IDC_STATIC_VALID_SRC_1);
			UpdateFolderCounter(hDlg, IDC_CMB_VALID_SRC_2, IDC_STATIC_VALID_SRC_2);
			UpdateFolderCounter(hDlg, IDC_CMB_VALID_SRC_3, IDC_STATIC_VALID_SRC_3);
			UpdateFolderCounter(hDlg, IDC_CMB_VALID_SRC_4, IDC_STATIC_VALID_SRC_4);
			UpdateFolderCounter(hDlg, IDC_CMB_VALID_SRC_5, IDC_STATIC_VALID_SRC_5);
			UpdateFolderCounter(hDlg, IDC_CMB_VALID_SRC_6, IDC_STATIC_VALID_SRC_6);
			UpdateFolderCounter(hDlg, IDC_CMB_VALID_SRC_7, IDC_STATIC_VALID_SRC_7);

    
            // （任意）先頭コンボにフォーカス
            if (HWND hFirst = GetDlgItem(hDlg, IDC_CMB_TRAIN_SRC[0])) {
                SetFocus(hFirst);
                return FALSE; // 自分でフォーカス設定したので FALSE
            }
            return TRUE;
        }
            
        case WM_COMMAND:

            switch (LOWORD(wParam))
            {
                case IDC_BTN_TRAIN_BROWSE_0: PickFolderEx(hDlg, IDC_CMB_TRAIN_SRC_0, L"Original Sorce Directory"); UpdateFolderCounter(hDlg, IDC_CMB_TRAIN_SRC_0, IDC_STATIC_TRAIN_SRC_0); break;
                case IDC_BTN_TRAIN_BROWSE_1: PickFolderEx(hDlg, IDC_CMB_TRAIN_SRC_1, L"Original Sorce Directory"); UpdateFolderCounter(hDlg, IDC_CMB_TRAIN_SRC_1, IDC_STATIC_TRAIN_SRC_1); break;
                case IDC_BTN_TRAIN_BROWSE_2: PickFolderEx(hDlg, IDC_CMB_TRAIN_SRC_2, L"Original Sorce Directory"); UpdateFolderCounter(hDlg, IDC_CMB_TRAIN_SRC_2, IDC_STATIC_TRAIN_SRC_2); break;
                case IDC_BTN_TRAIN_BROWSE_3: PickFolderEx(hDlg, IDC_CMB_TRAIN_SRC_3, L"Original Sorce Directory"); UpdateFolderCounter(hDlg, IDC_CMB_TRAIN_SRC_3, IDC_STATIC_TRAIN_SRC_3); break;
                case IDC_BTN_TRAIN_BROWSE_4: PickFolderEx(hDlg, IDC_CMB_TRAIN_SRC_4, L"Original Sorce Directory"); UpdateFolderCounter(hDlg, IDC_CMB_TRAIN_SRC_4, IDC_STATIC_TRAIN_SRC_4); break;
                case IDC_BTN_TRAIN_BROWSE_5: PickFolderEx(hDlg, IDC_CMB_TRAIN_SRC_5, L"Original Sorce Directory"); UpdateFolderCounter(hDlg, IDC_CMB_TRAIN_SRC_5, IDC_STATIC_TRAIN_SRC_5); break;
                case IDC_BTN_TRAIN_BROWSE_6: PickFolderEx(hDlg, IDC_CMB_TRAIN_SRC_6, L"Original Sorce Directory"); UpdateFolderCounter(hDlg, IDC_CMB_TRAIN_SRC_6, IDC_STATIC_TRAIN_SRC_6); break;
                case IDC_BTN_TRAIN_BROWSE_7: PickFolderEx(hDlg, IDC_CMB_TRAIN_SRC_7, L"Original Sorce Directory"); UpdateFolderCounter(hDlg, IDC_CMB_TRAIN_SRC_7, IDC_STATIC_TRAIN_SRC_7); break;

                case IDC_BTN_VALID_BROWSE_0: PickFolderEx(hDlg, IDC_CMB_VALID_SRC_0, L"Original Sorce Directory"); UpdateFolderCounter(hDlg, IDC_CMB_TRAIN_SRC_0, IDC_STATIC_TRAIN_SRC_0); break;
                case IDC_BTN_VALID_BROWSE_1: PickFolderEx(hDlg, IDC_CMB_VALID_SRC_1, L"Original Sorce Directory"); UpdateFolderCounter(hDlg, IDC_CMB_TRAIN_SRC_1, IDC_STATIC_TRAIN_SRC_1); break;
                case IDC_BTN_VALID_BROWSE_2: PickFolderEx(hDlg, IDC_CMB_VALID_SRC_2, L"Original Sorce Directory"); UpdateFolderCounter(hDlg, IDC_CMB_TRAIN_SRC_2, IDC_STATIC_TRAIN_SRC_2); break;
                case IDC_BTN_VALID_BROWSE_3: PickFolderEx(hDlg, IDC_CMB_VALID_SRC_3, L"Original Sorce Directory"); UpdateFolderCounter(hDlg, IDC_CMB_TRAIN_SRC_3, IDC_STATIC_TRAIN_SRC_3); break;
                case IDC_BTN_VALID_BROWSE_4: PickFolderEx(hDlg, IDC_CMB_VALID_SRC_4, L"Original Sorce Directory"); UpdateFolderCounter(hDlg, IDC_CMB_TRAIN_SRC_4, IDC_STATIC_TRAIN_SRC_4); break;
                case IDC_BTN_VALID_BROWSE_5: PickFolderEx(hDlg, IDC_CMB_VALID_SRC_5, L"Original Sorce Directory"); UpdateFolderCounter(hDlg, IDC_CMB_TRAIN_SRC_5, IDC_STATIC_TRAIN_SRC_5); break;
                case IDC_BTN_VALID_BROWSE_6: PickFolderEx(hDlg, IDC_CMB_VALID_SRC_6, L"Original Sorce Directory"); UpdateFolderCounter(hDlg, IDC_CMB_TRAIN_SRC_6, IDC_STATIC_TRAIN_SRC_6); break;
                case IDC_BTN_VALID_BROWSE_7: PickFolderEx(hDlg, IDC_CMB_VALID_SRC_7, L"Original Sorce Directory"); UpdateFolderCounter(hDlg, IDC_CMB_TRAIN_SRC_7, IDC_STATIC_TRAIN_SRC_7); break;

                case IDC_BTN_BROWSE_TEMP_MCOPY: {
                    PickFolderEx(hDlg, IDC_COMBO_TEMP_MCOPY, L"Tempolary Source Directory");
					//ここで親ウィンドウのTEMPも更新しておく
                    HWND hMain = GetParent(hDlg);
                    if (hMain) {
                        std::wstring tempDir = GetText(hDlg, IDC_COMBO_TEMP);
                        SetComboText(GetDlgItem(hMain, IDC_COMBO_TEMP), tempDir);
					}
                } break;
                
                //  一時フォルダクリアボタン
                case IDC_BTN_CLEARTMP_MCOPY:
                {
					if (HIWORD(wParam) != BN_CLICKED) //ボタンがクリックされた場合のみ処理
                        return TRUE;
                    AppendLog(L"[TEMP] Clear requested."); AppendLog(RET);
                    DoClearTemp(hDlg, true, false);
                    AppendLog(L"[TEMP] Clear request done."); AppendLog(RET);
                         return TRUE;

                } break; //IDC_BTN_CLEARTMP

				//  コピー実行ボタン
                /*
                case IDC_BTN_COPY_TO_TEMP+9999:
                {
                    // 1) 入力収集（train/valid の 0 番だけ使用）と有効チェック
                    std::wstring trainRoot = GetComboText(GetDlgItem(hDlg, IDC_CMB_TRAIN_SRC_0));
                    std::wstring validRoot = GetComboText(GetDlgItem(hDlg, IDC_CMB_VALID_SRC_0));
                    const bool useTrain = (IsDlgButtonChecked(hDlg, IDC_CHK_TRAIN_EN_0) == BST_CHECKED);
                    const bool useValid = (IsDlgButtonChecked(hDlg, IDC_CHK_VALID_EN_0) == BST_CHECKED);

                    // 2) Temp ルート（親のメインダイアログから取得）
                    HWND hMain = GetParent(hDlg);
                    if (!hMain) { 
                        MessageBoxW(hDlg, L"No parent dialog.", L"Error", MB_ICONERROR); break; 
                    }
                    std::wstring tempRoot = GetComboText(GetDlgItem(hMain, IDC_COMBO_TEMP));
                    if (tempRoot.empty()) 
                    {
                        MessageBoxW(hDlg, L"Temp folder is empty.", L"Error", MB_ICONERROR); break; 
                    }

                    // 3) 何も有効でなければ終了
                    if ((!useTrain || trainRoot.empty()) && (!useValid || validRoot.empty())) {
                        MessageBoxW(hDlg, L"No valid source selected.", L"Info", MB_ICONINFORMATION);
                        break;
                    }

                    // 4) UI無効化
                    EnableWindow(GetDlgItem(hDlg, IDC_BTN_COPY_TO_TEMP), FALSE);

                    // 5) スレッド起動
                    std::thread([=]() {
                        // パス構築
                        fs::path temp = fs::path(tempRoot);
                        fs::path dTrainImg = temp / L"dataset" / L"train" / L"images";
                        fs::path dTrainLbl = temp / L"dataset" / L"train" / L"labels";
                        fs::path dValidImg = temp / L"dataset" / L"valid" / L"images";
                        fs::path dValidLbl = temp / L"dataset" / L"valid" / L"labels";

                        // 出力先を作成
                        EnsureDirIfMissing(dTrainImg); EnsureDirIfMissing(dTrainLbl);
                        EnsureDirIfMissing(dValidImg); EnsureDirIfMissing(dValidLbl);

                        // 総数カウント
                        uint64_t total = 0;
                        if (useTrain && !trainRoot.empty())
                            total += CountImagesLabels(fs::path(trainRoot));
                        if (useValid && !validRoot.empty())
                            total += CountImagesLabels(fs::path(validRoot));

                        std::atomic<uint64_t> done{ 0 };
                        PostMessageW(hDlg, WM_APP + 100, 0, 0); // 任意: ログ開始等

                        // 6) コピー本体（images / labels のみ）
                        if (useTrain && !trainRoot.empty()) {
                            fs::path s = fs::path(trainRoot);
                            CopySubtreeWithProgress(s / L"images", dTrainImg, done, total, hDlg);
                            CopySubtreeWithProgress(s / L"labels", dTrainLbl, done, total, hDlg);
                        }
                        if (useValid && !validRoot.empty()) {
                            fs::path s = fs::path(validRoot);
                            CopySubtreeWithProgress(s / L"images", dValidImg, done, total, hDlg);
                            CopySubtreeWithProgress(s / L"labels", dValidLbl, done, total, hDlg);
                        }

                        // 7) 100% にしてUI復帰
                        SendDlgItemMessageW(hDlg, IDC_PROGRESS_COPY, PBM_SETPOS, 100, 0);

                        // ログ（任意・メインに出すなら AppendLog を使う）
                        // AppendLog(L"[COPY] Completed.\r\n");

                        EnableWindow(GetDlgItem(hDlg, IDC_BTN_COPY_TO_TEMP), TRUE);
                        MessageBeep(MB_OK);
                        }).detach();

                    return TRUE;
                }
                */

                //  コピー実行ボタン
                case IDC_BTN_COPY_TO_TEMP:
                {
                    // 1) Temp ルート（親のメインダイアログから取得）
                    HWND hMain = GetParent(hDlg);
                    if (!hMain) { MessageBoxW(hDlg, L"No parent dialog.", L"Error", MB_ICONERROR); break; }
                    std::wstring tempRoot = GetComboText(GetDlgItem(hMain, IDC_COMBO_TEMP));
                    if (tempRoot.empty()) { MessageBoxW(hDlg, L"Temp folder is empty.", L"Error", MB_ICONERROR); break; }

                    // 2) 選択行をまとめて収集（0..7）
                    std::vector<std::wstring> trainRoots, validRoots;
                    for (int i = 0; i < 8; ++i) {
                        const bool useT = (IsDlgButtonChecked(hDlg, IDC_CHK_TRAIN_EN[i]) == BST_CHECKED);
                        const bool useV = (IsDlgButtonChecked(hDlg, IDC_CHK_VALID_EN[i]) == BST_CHECKED);
                        if (useT) {
                            std::wstring t = GetComboText(GetDlgItem(hDlg, IDC_CMB_TRAIN_SRC[i]));
                            if (!t.empty()) trainRoots.push_back(std::move(t));
                        }
                        if (useV) {
                            std::wstring v = GetComboText(GetDlgItem(hDlg, IDC_CMB_VALID_SRC[i]));
                            if (!v.empty()) validRoots.push_back(std::move(v));
                        }
                    }
                    if (trainRoots.empty() && validRoots.empty()) {
                        MessageBoxW(hDlg, L"No valid source selected.", L"Info", MB_ICONINFORMATION);
                        break;
                    }

                    // 3) UI無効化
                    EnableWindow(GetDlgItem(hDlg, IDC_BTN_COPY_TO_TEMP), FALSE);

                    // 4) スレッド起動
                    std::thread([=]() {
                        namespace fs = std::filesystem;

                        // 進捗ユーティリティ
                        auto setPos = [&](int pct) {
                            SendDlgItemMessageW(hDlg, IDC_PROGRESS_COPY, PBM_SETPOS, pct, 0);
                            };
                        auto ensureDir = [&](const fs::path& p) {
                            std::error_code ec; if (!fs::exists(p, ec)) fs::create_directories(p, ec);
                            };

                        // カウント：images/labels の “ファイル数”
                        auto countImagesLabels = [&](const fs::path& root) -> uint64_t {
                            uint64_t cnt = 0;
                            for (const wchar_t* sub : { L"images", L"labels" }) {
                                fs::path d = root / sub;
                                std::error_code ec;
                                if (!fs::exists(d, ec)) continue;
                                for (auto& e : fs::recursive_directory_iterator(d, fs::directory_options::skip_permission_denied, ec)) {
                                    if (e.is_regular_file()) ++cnt;
                                }
                            }
                            return cnt;
                        };

                        // コピー（進捗つき）
                        auto copySubtreeWithProgress = [&](const fs::path& src, const fs::path& dst,
                            std::atomic<uint64_t>& done, uint64_t total,
                            const wchar_t* tag) 
                        {
                            std::error_code ec;
                            if (!fs::exists(src, ec)) 
                            {
                                LogToMain(L"[COPY] Skip (not found): " + src.wstring() + L"\r\n");
                                return; // 無ければスキップ
                            }
                            LogToMain(L"[COPY] " + std::wstring(tag) + L": " + src.wstring() + L"  ->  " + dst.wstring() + L"\r\n");
                            size_t tick = 0;
                            for (auto& e : fs::recursive_directory_iterator(src, fs::directory_options::skip_permission_denied, ec)) {
                                if (e.is_directory()) {
                                    ensureDir(dst / fs::relative(e.path(), src, ec));
                                    continue;
                                }
                                if (e.is_regular_file())
                                {
                                    fs::path out = dst / fs::relative(e.path(), src, ec);
                                    ensureDir(out.parent_path());
                                    fs::copy_file(e.path(), out, fs::copy_options::overwrite_existing, ec);
                                    uint64_t cur = ++done;
                                    if (total > 0)
                                        setPos(int((cur * 100) / total));

                                    // ★Nファイルごとに現在ファイルをログ（多すぎないよう適度に）
                                    if ((++tick % 200) == 0) {
                                        LogToMain(L"[COPY] ... " + e.path().filename().wstring() + L"\r\n");
                                    }
                                }
                            }   
                        };

                        // 5) 出力先ディレクトリ
                        fs::path temp = fs::path(tempRoot);
                        fs::path dTrainImg = temp / L"dataset" / L"train" / L"images";
                        fs::path dTrainLbl = temp / L"dataset" / L"train" / L"labels";
                        fs::path dValidImg = temp / L"dataset" / L"valid" / L"images";
                        fs::path dValidLbl = temp / L"dataset" / L"valid" / L"labels";
                        ensureDir(dTrainImg); ensureDir(dTrainLbl);
                        ensureDir(dValidImg); ensureDir(dValidLbl);

                        // 6) 総数カウント
                        uint64_t total = 0;
                        for (auto& r : trainRoots) total += countImagesLabels(fs::path(r));
                        for (auto& r : validRoots) total += countImagesLabels(fs::path(r));
                        std::atomic<uint64_t> done{ 0 };
                        setPos(0);

                        LogToMain(L"[COPY] ---- START ----\r\n");

                        // 7) コピー本体（Train 全行 → Valid 全行）
                        for (auto& r : trainRoots) {
                            fs::path s(r);
                            LogToMain(L"[COPY] Train root: " + s.wstring() + L"\r\n");
                            copySubtreeWithProgress(s / L"images", dTrainImg, done, total, L"train/images");
                            copySubtreeWithProgress(s / L"labels", dTrainLbl, done, total, L"train/labels");
                            }
                        for (auto& r : validRoots) {
                            fs::path s(r);
                            LogToMain(L"[COPY] Valid root: " + s.wstring() + L"\r\n");
                            copySubtreeWithProgress(s / L"images", dValidImg, done, total, L"valid/images");
                            copySubtreeWithProgress(s / L"labels", dValidLbl, done, total, L"valid/labels");
                        }

                        // 8) 100% & UI復帰
                        setPos(100);
                        LogToMain(L"[COPY] ---- COMPLETED ----\r\n");

                        EnableWindow(GetDlgItem(hDlg, IDC_BTN_COPY_TO_TEMP), TRUE);
                        MessageBeep(MB_OK);
                        }).detach();

                    return TRUE;
                }

                case IDCANCEL2: // OKボタン 同じ処理 
                case IDCANCEL: // キャンセルボタン／×ボタン
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
            }
            break;

        case WM_DESTROY:
        {
            // 1) 画面の内容をクラスへ読み取り
            MultiCopyParams prm;
            prm.ReadControls(hDlg);  // ← train/valid の各コンボと各チェックを埋める

            // 2) INI 保存（現在値＋各フラグ）
			prm.SaveCurrentSettingsToIni(hDlg);

            return 0;
        }
    }
    return FALSE;
}
