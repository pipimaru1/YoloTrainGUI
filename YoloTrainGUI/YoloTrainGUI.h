#pragma once

#include "pch.h"
#include "resource.h"
#include "tooltip.hpp"

// ------------------------------
// Globals
// ------------------------------
extern HINSTANCE g_hInst;
extern HWND g_hDlg;
extern std::atomic<HANDLE> g_hChildProc;
extern std::mutex g_logMutex;
extern std::wstring g_logBuffer;
extern const wchar_t* gRET;
extern bool _IDC_CHK_LOG_CRLF2LF;// = false;
extern std::mutex g_storeMutex;


#define MAXDIR 16 // 最大ディレクトリ数（Train/Valid 各 8 行）
extern const UINT IDC_CMB_TRAIN_SRC[MAXDIR];
extern const UINT IDC_CMB_VALID_SRC[MAXDIR];
// ★各行の「Train用」「Valid用」チェックを独立IDに
extern const UINT IDC_CHK_TRAIN_EN[MAXDIR];
extern const UINT IDC_CHK_VALID_EN[MAXDIR];
// スタティック表示先のID配列（既存のコンボ配列とペア）
extern const UINT IDC_STC_TRAIN_SRC[MAXDIR];
extern const UINT IDC_STC_VALID_SRC[MAXDIR];

extern Tooltip _ToolTipMainDlg; // グローバルなツールチップオブジェクト
extern Tooltip _ToolTipMultiCpDlg; // グローバルなツールチップオブジェクト

// RichEdit関連
//_return_mode
// 0 : \n
// 1 : \r
// 2 : \r\n
void LogAppendANSI(const std::wstring& s);
void AppendLog(const std::wstring& s);
extern int _RETRUN_MODE;        // 0:LF 1:CR 2:CRLF


// ------------------------------
// Utilities
// ------------------------------

std::wstring GetText(HWND hDlg, int id);
void SaveMRU(const std::wstring& section, const std::wstring& value, size_t maxItems = 256);
void LoadMRUToCombo(HWND hCombo, const std::wstring& section);
std::vector<std::wstring> LoadMRU(const std::wstring& section);
bool LoadFlagFromIni(const wchar_t* key, bool def = false);
std::string ToUTF8(const std::wstring& w);
std::wstring FromUTF8(const std::string& s);
void DoClearTemp(HWND hOwner,const UINT _ID ,bool confirm = true, bool keepRoot = false);
void ApplyEnableFromCheck(HWND hDlg, UINT idChk, UINT idCombo);
void UpdateFolderCounter(HWND hDlg, const UINT ID_COMBO, const UINT ID_STATICTXT);
uint64_t CountFilesUnder(const fs::path& root);
void UpdateAllFolderCounts(HWND hDlg);
void UpdateFolderCounter_Train(HWND hDlg, const UINT ID_COMBO, const UINT ID_STATICTXT);
void UpdateFolderCounter_Valid(HWND hDlg, const UINT ID_COMBO, const UINT ID_STATICTXT);
bool ExportMultiCopyListToCSV(HWND hDlg);
bool ImportMultiCopyListFromCSV(HWND hDlg);

bool PickFolderEx1(HWND owner, std::wstring& outPath, std::wstring initialDir, std::wstring _title);
bool PickFolderEx2(HWND hDlg, UINT _ComboID, std::wstring _title);//上記関数のラッパー
bool PickFolderEx3(HWND hDlg, UINT _ComboID, std::wstring _title, std::wstring _SECSTR);

std::map<std::wstring, std::vector<std::wstring>> ReadIniColon();
void WriteIniColon(const std::map<std::wstring, std::vector<std::wstring>>& data);
void ClearMRUSection(const std::wstring& section); // MRU（履歴）を「セクションごと消す」関数を追加
void ClearComboUI(HWND hCombo);                    // コンボボックスをクリアするユーティリティ 

//　汎用かな
int LoadIntFromIni(const wchar_t* key, int defValue);

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
void PostLogToUi(const std::wstring& s);
fs::path GetStoreDir();
fs::path GetIniFile();
fs::path GetHistoryFile();

std::wstring GetText(HWND hDlg, int id);
void SetProgress(int v);
void ResetProgress();

void AppendCmdHistory(const std::wstring& cmd);
void EnableControls(HWND hDlg, const int* ids, size_t n, BOOL enable);
void Updated_UI(HWND hDlg);


void CommitComboMRU(HWND hCombo, const std::wstring& section, const std::wstring& value);

//void PickCommitComboMRU(HWND hDlg, HWND hCombo, const std::wstring& section, const std::wstring& value);// フォルダ選択ダイアログ表示ユーティリティ

// ------------------------------
// CopyMultiDlgのProc
INT_PTR CALLBACK CopyMultiDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

// ------------------------------
// ツールチップ
//void SetTootips(HWND hDlg);
void SetTootips(HWND hDlg, Tooltip& ttTmpDir);


// 例: YoloTrainGUI.h など共通で見える場所
#define WM_APP_LOGAPPEND    (WM_APP + 100)
// UIスレッドに進捗(%)を投げるユーザーメッセージ
#ifndef WM_APP_PROGRESS
#define WM_APP_PROGRESS     (WM_APP + 101)
#endif

#define MAXLINES_RICHEDIT 10000
//////////////////////////////////////////////////////////////////////////////////////
// Get text from a control
//////////////////////////////////////////////////////////////////////////////////////
class TrainParams {
    std::wstring src_dir;
    std::wstring src_dir_image;
    std::wstring src_dir_label;

    std::wstring RatioTrain;
    std::wstring RatioUse; //for Hyperparameter tuning データを減らしたい時に使う

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
    std::wstring http_proxy; // HTTP プロキシ設定（v8/v11用）
    std::wstring https_proxy; // HTTPS プロキシ設定（v8/v11用）

    // 追加
    std::wstring option_str;   // 自由記述の追加オプション (yolov5のtrain.py または yolo CLI へそのまま連結)

    int chkResume = 0; // 0 or 1
    int chkCache = 0; // 0 or 1
    int chkUseHyp = 0; // 0 or 1
    int exist_ok = 0; // 0 or 1
    int chkUseProxy = 0; // 0 or 1

    //ラベルのあるファイルのみコピーする
    //int _IDC_BUTTON_APPLY_YAML = 0; // 0 or 1

    // v8 only
    std::wstring task;    // detect/segment/pose/classify
    std::wstring backend; // "YOLOV5" or "YOLOV8" or "YOLO11"

public:
    TrainParams() : backend(L"yolov5"), workdir(L""), trainpy(L"train.py"), datayaml(L""), hypyaml(L""), cfgyaml(L""),
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
    int SaveCurrentSettingsToIni(HWND hDlg);
    void DoTrain();
//    void DoTrain_old();

};

// ------------------------------
// MultiCopy
// ------------------------------
class MultiCopyParams {

private:
    std::wstring src_dir;

    //MultiCopyDlg用
    std::vector<std::wstring> src_dir_trains;
    std::vector<std::wstring> src_dir_valids;
    std::vector<bool>         src_dir_train_chks;
    std::vector<bool>         src_dir_valid_chks;

public:
    MultiCopyParams()
    {
        src_dir_trains.resize(8);
        src_dir_valids.resize(8);
        src_dir_train_chks.resize(8);
        src_dir_valid_chks.resize(8);
    }

    int ReadControls(HWND hDlg);
    int SaveCurrentSettingsToIni(HWND hDlg);
};
