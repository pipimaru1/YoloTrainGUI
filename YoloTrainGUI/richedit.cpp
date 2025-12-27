#define _WIN32_WINNT 0x0601
#include "pch.h"

#include "YoloTrainGUI.h"

bool _IDC_CHK_LOG_CRLF2LF = false;
//static LONG g_curLineStartCp = 0;   // 最後の改行直後（論理行先頭）
//static bool g_pendingCR = false;    // CR が来たが CRLF か単体 CR か未確定
int _RETRUN_MODE = 2;           // 0:LF 1:CR 2:CRLF
//const wchar_t* rRET = L"\r\n";   // 既存の const wchar_t* RET があるなら置き換え

/// <summary>
/// //////////////
/// </summary>
/// <param name="s"></param>
void AppendLog(const std::wstring& s)
{
    if (0)
    {
        std::lock_guard<std::mutex> lock(g_logMutex);
        g_logBuffer += s;
        g_logBuffer += L"\r\n";
        if (g_hDlg) {
            SetDlgItemTextW(g_hDlg, IDC_LOG, g_logBuffer.c_str());
            // Scroll to bottom
            HWND hLog = GetDlgItem(g_hDlg, IDC_LOG);
            SendMessageW(hLog, EM_LINESCROLL, 0, 0x7fff);
        }
    }
    else
        LogAppendANSI(s);
}

// ANSI色の状態
struct AnsiState {
    COLORREF fg = RGB(0, 0, 0);
    bool bold = false;
    void reset() { fg = RGB(0, 0, 0); bold = false; }
};

// RichEdit 選択範囲に色/太字を適用
static void SetSelFormat(HWND hRe, const AnsiState& st)
{
    CHARFORMAT2 cf{};
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR | CFM_BOLD;
    cf.crTextColor = st.fg;
    cf.dwEffects = st.bold ? CFE_BOLD : 0;
    SendMessageW(hRe, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
}

// 末尾に追記（READONLY を一時解除）
static void RichAppend(HWND hRe, const std::wstring& w, const AnsiState& st)
{
    if (w.empty()) return;
    LRESULT len = SendMessageW(hRe, WM_GETTEXTLENGTH, 0, 0);
    CHARRANGE cr{ (LONG)len, (LONG)len };
    SendMessageW(hRe, EM_EXSETSEL, 0, (LPARAM)&cr);
    SendMessageW(hRe, EM_SETREADONLY, FALSE, 0);
    SetSelFormat(hRe, st);
    SendMessageW(hRe, EM_REPLACESEL, FALSE, (LPARAM)w.c_str());
    SendMessageW(hRe, EM_SETREADONLY, TRUE, 0);
    // スクロール
    len = SendMessageW(hRe, WM_GETTEXTLENGTH, 0, 0);
    cr.cpMin = cr.cpMax = (LONG)len;
    SendMessageW(hRe, EM_EXSETSEL, 0, (LPARAM)&cr);
    SendMessageW(hRe, EM_SCROLLCARET, 0, 0);
}

// 最終行を丸ごと差し替え（\r の後の表示に使用）
static void RichReplaceLastLine(HWND hRe, const std::wstring& w, const AnsiState& st)
{
    LRESULT lines = SendMessageW(hRe, EM_GETLINECOUNT, 0, 0);
    LRESULT start = SendMessageW(hRe, EM_LINEINDEX, lines > 0 ? lines - 1 : 0, 0);
    LRESULT end = SendMessageW(hRe, WM_GETTEXTLENGTH, 0, 0);
    CHARRANGE cr{ (LONG)start, (LONG)end };
    SendMessageW(hRe, EM_EXSETSEL, 0, (LPARAM)&cr);

    SendMessageW(hRe, EM_SETREADONLY, FALSE, 0);
    SetSelFormat(hRe, st);
    // 置換（既存行を空にしてから新しい文字列を入れる）
    SendMessageW(hRe, EM_REPLACESEL, FALSE, (LPARAM)L"");
    SendMessageW(hRe, EM_REPLACESEL, FALSE, (LPARAM)w.c_str());
    SendMessageW(hRe, EM_SETREADONLY, TRUE, 0);

    // キャレットを末尾へ
    end = SendMessageW(hRe, WM_GETTEXTLENGTH, 0, 0);
    cr.cpMin = cr.cpMax = (LONG)end;
    SendMessageW(hRe, EM_EXSETSEL, 0, (LPARAM)&cr);
    SendMessageW(hRe, EM_SCROLLCARET, 0, 0);
}
/*
static void RichReplaceCurrentLogicalLine(HWND hRe, const std::wstring& w, const AnsiState& st)
{
    LRESULT end = SendMessageW(hRe, WM_GETTEXTLENGTH, 0, 0);
    if (g_curLineStartCp < 0) g_curLineStartCp = 0;
    if (g_curLineStartCp > end) g_curLineStartCp = (LONG)end;

    CHARRANGE cr{ g_curLineStartCp, (LONG)end };
    SendMessageW(hRe, EM_EXSETSEL, 0, (LPARAM)&cr);

    SendMessageW(hRe, EM_SETREADONLY, FALSE, 0);
    SetSelFormat(hRe, st);
    SendMessageW(hRe, EM_REPLACESEL, FALSE, (LPARAM)w.c_str());
    SendMessageW(hRe, EM_SETREADONLY, TRUE, 0);

    // caret を末尾へ
    end = SendMessageW(hRe, WM_GETTEXTLENGTH, 0, 0);
    cr.cpMin = cr.cpMax = (LONG)end;
    SendMessageW(hRe, EM_EXSETSEL, 0, (LPARAM)&cr);
    SendMessageW(hRe, EM_SCROLLCARET, 0, 0);
}
*/

// ANSIカラーコード（SGR）の整数 -> COLORREF
static COLORREF AnsiColor(int code)
{
    // 30-37 (普通) / 90-97 (明るい)
    switch (code) {
    case 30: case 90: return RGB(0, 0, 0);
    case 31: case 91: return RGB(205, 49, 49);
    case 32: case 92: return RGB(13, 188, 121);
    case 33: case 93: return RGB(229, 229, 16);
    case 34: case 94: return RGB(36, 114, 200);
    case 35: case 95: return RGB(188, 63, 188);
    case 36: case 96: return RGB(17, 168, 205);
    case 37: case 97: return RGB(229, 229, 229);
    default:           return RGB(0, 0, 0);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// 先頭から古い行を削る。hysteresis で「少し余分に削る」→削除頻度を下げて高速化


static void TrimOldLines(HWND hRe, int maxLines = MAXLINES_RICHEDIT, int hysteresis = 200)
{
    if (!hRe || maxLines <= 0) return;

    LRESULT lineCount = SendMessageW(hRe, EM_GETLINECOUNT, 0, 0);
    if (lineCount <= maxLines) return;

    int drop = (int)(lineCount - maxLines + hysteresis);
    if (drop < 1) drop = 1;

    // 先頭から drop 行の末尾までの文字オフセットを求める
    // EM_LINEINDEX(n): n 行目の先頭の文字インデックス
    LRESULT end = SendMessageW(hRe, EM_LINEINDEX, drop, 0);
    if (end <= 0) return;

    // まとめて削除（描画抑止でチラつき防止）
    SendMessageW(hRe, WM_SETREDRAW, FALSE, 0);
    CHARRANGE cr{ 0, (LONG)end };
    SendMessageW(hRe, EM_SETREADONLY, FALSE, 0);
    SendMessageW(hRe, EM_EXSETSEL, 0, (LPARAM)&cr);
    SendMessageW(hRe, EM_REPLACESEL, FALSE, (LPARAM)L"");
    SendMessageW(hRe, EM_SETREADONLY, TRUE, 0);
    SendMessageW(hRe, WM_SETREDRAW, TRUE, 0);

    // キャレットを末尾へ（スクロール維持）
    LRESULT len = SendMessageW(hRe, WM_GETTEXTLENGTH, 0, 0);
    cr.cpMin = cr.cpMax = (LONG)len;
    SendMessageW(hRe, EM_EXSETSEL, 0, (LPARAM)&cr);
    SendMessageW(hRe, EM_SCROLLCARET, 0, 0);
}

//////////////////////////////////////////////////////////////
// 
// 改行挿入ヘルパ（挿入後に g_curLineStartCp を更新）
// 
//////////////////////////////////////////////////////////////
/*
static void RichAppendNewLine(HWND hRe, const AnsiState& st)
{
    RichAppend(hRe, L"\r\n", st); // または L"\r"
    g_curLineStartCp = (LONG)SendMessageW(hRe, WM_GETTEXTLENGTH, 0, 0);
}
*/
//////////////////////////////////////////////////////////////
// 
// ANSIエスケープを解釈して RichEdit に吐く
// _return_mode
// 0 : \n
// 1 : \r
// 2 : \r\n
//////////////////////////////////////////////////////////////
void LogAppendANSI( const std::wstring& s)
{
    HWND hRe = GetDlgItem(g_hDlg, IDC_LOG);
    if (!hRe || s.empty()) return;

    AnsiState st;
    std::wstring chunk;
    bool crMode = false; // 直前に '\r' が来たら true（次の出力で最終行を置換）

    auto flush = [&](bool replace) {
        if (chunk.empty()) 
            return;
        if (replace) 
            RichReplaceLastLine(hRe, chunk, st);
        else         
            RichAppend(hRe, chunk, st);

        // ★ここで古い行を落とす（上限 10000 行）
        TrimOldLines(hRe, MAXLINES_RICHEDIT, 200);

        chunk.clear();
        };

    for (size_t i = 0; i < s.size(); ++i)
    {
        wchar_t c0 = s[i];

        wchar_t c1;
        if (i < s.size())
            c1= s[i + 1];
        else
			c1 = L'\0';


        if (c0 == L'\x1b') {
            // 直前の文字列を出力
            flush(crMode); crMode = false;

            // 期待: \x1b[ ... m
            if (i + 1 < s.size() && s[i + 1] == L'[') {
                i += 2;
                std::vector<int> params;
                int val = 0; bool have = false;
                for (; i < s.size(); ++i) {
                    wchar_t ch = s[i];
                    if (ch >= L'0' && ch <= L'9') {
                        val = val * 10 + (ch - L'0');
                        have = true;
                    }
                    else if (ch == L';') {
                        params.push_back(have ? val : 0);
                        val = 0; have = false;
                    }
                    else if (ch == L'm') {
                        params.push_back(have ? val : 0);
                        break;
                    }
                    else {
                        // 想定外 → 中断
                        break;
                    }
                }
                // SGR 適用
                if (params.empty()) params.push_back(0);
                for (int p : params) {
                    if (p == 0) { st.reset(); }
                    else if (p == 1) { st.bold = true; }
                    else if (p == 22) { st.bold = false; }
                    else if ((p >= 30 && p <= 37) || (p >= 90 && p <= 97)) { st.fg = AnsiColor(p); }
                    else if (p == 39) { st.fg = RGB(0, 0, 0); } // default FG
                    // 必要なら 38;2;r;g;b や 38;5;n など拡張にも対応可
                }
            }
            continue;
        }
#if(0)
        if (c == L'\r') {
            // キャリッジリターン：次の出力で最終行を置換
            flush(crMode); // 現在のチャンクはここまでで出す
            crMode = true;
            continue;
        }
        if (c == L'\n') {
            // 改行 → まとめて出力
            chunk.push_back(L'\n');
            flush(crMode);
            crMode = false;
            continue;
        }
#else
        if (c0 == L'\r' && c1 != L'\n')
        {
            // いままでの内容で"上書き"確定
            flush(false);                       // 上書きするのでフラッシュする必要ない
            RichReplaceLastLine(hRe, L"", st);  // 空行に置換（CRLFをLFに変換）
            continue;
        }
        //以下の処理は要らないのかもしれない
        else if (c0 == L'\r' && c1 == L'\n') 
        {
            //リターン
            flush(true);                   
            // ★ここを常に置換で出すのがポイント
            // 改行を append で確定（空行を作らないため chunk は使わない）
            if (_RETRUN_MODE == 0)
                RichAppend(hRe, L"\n", st);
            else if (_RETRUN_MODE == 1)
                RichAppend(hRe, L"\r", st);
            else // (_return_mode == 2)
                RichAppend(hRe, L"\r\n", st);
            crMode = false;
            ++i;                            // LF を消費
            continue;
		}
        if (c0 == L'\n') {
            // 直前までのチャンクを通常出力
            flush(true);
            //flush(false);              // append（\rフラグはここでクリア）
            if (_RETRUN_MODE == 0)
                RichAppend(hRe, L"\n", st);
            else if (_RETRUN_MODE == 1)
                RichAppend(hRe, L"\r", st);
            else // (_return_mode == 2)
                RichAppend(hRe, L"\r\n", st);
            //++i; // LF を消費
            continue;
        }
#endif

        chunk.push_back(c0);
    }
    flush(crMode);
}

/*
///////////////////////////////////////////////////////////////
//　改行対策版 しかしだめ
void LogAppendANSI_test(
    const std::wstring& s,
    int _return_mode
)
{
    HWND hRe = GetDlgItem(g_hDlg, IDC_LOG);
    if (!hRe || s.empty()) return;

    AnsiState st;
    std::wstring chunk;
    bool crMode = false; // 直前に '\r' が来たら true（次の出力で最終行を置換）

    bool replaceMode = false; // 次 flush で置換するか

    auto flush = [&]() {
        if (chunk.empty()) return;
        if (replaceMode) RichReplaceCurrentLogicalLine(hRe, chunk, st);
        else             RichAppend(hRe, chunk, st);
        chunk.clear();
        };


    for (size_t i = 0; i < s.size(); ++i) {
        wchar_t c = s[i];

        // 前回末尾が \r だった（CRLF か単体CRか未確定）
        if (g_pendingCR) {
            if (c == L'\n') {
                // CRLF確定 → 改行
                flush();
                RichAppendNewLine(hRe, st);
                g_pendingCR = false;
                replaceMode = false;
                continue;
            }
            else {
                // 単体CR → 上書きモード
                g_pendingCR = false;
                replaceMode = true;
            }
        }

        if (c == L'\r') {
            flush();
            g_pendingCR = true;  // 次文字で CRLF か判定
            continue;
        }
        if (c == L'\n') {
            flush();
            RichAppendNewLine(hRe, st);
            replaceMode = false;
            continue;
        }
        chunk.push_back(c);
    }
    flush();
}
*/
