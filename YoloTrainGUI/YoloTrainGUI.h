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
extern const wchar_t* RET;
extern bool _IDC_CHK_LOG_CRLF2LF;// = false;


//
void LogAppendANSI(const std::wstring& s);
void AppendLog(const std::wstring& s);

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
void DoClearTemp(HWND hOwner, bool confirm = true, bool keepRoot = false);


// ------------------------------
// CopyMultiDlgのProc
INT_PTR CALLBACK CopyMultiDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);



// 例: YoloTrainGUI.h など共通で見える場所
#define WM_APP_LOGAPPEND   (WM_APP + 100)
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

    int chkResume = 0; // 0 or 1
    int chkCache = 0; // 0 or 1
    int chkUseHyp = 0; // 0 or 1
    int exist_ok = 0; // 0 or 1
    int chkUseProxy = 0; // 0 or 1

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
