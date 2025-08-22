#define _WIN32_WINNT 0x0601
#include "pch.h"

#include "YoloTrainGUI.h"


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


// ANSI�F�̏��
struct AnsiState {
    COLORREF fg = RGB(0, 0, 0);
    bool bold = false;
    void reset() { fg = RGB(0, 0, 0); bold = false; }
};

// RichEdit �I��͈͂ɐF/������K�p
static void SetSelFormat(HWND hRe, const AnsiState& st)
{
    CHARFORMAT2 cf{};
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR | CFM_BOLD;
    cf.crTextColor = st.fg;
    cf.dwEffects = st.bold ? CFE_BOLD : 0;
    SendMessageW(hRe, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
}

// �����ɒǋL�iREADONLY ���ꎞ�����j
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
    // �X�N���[��
    len = SendMessageW(hRe, WM_GETTEXTLENGTH, 0, 0);
    cr.cpMin = cr.cpMax = (LONG)len;
    SendMessageW(hRe, EM_EXSETSEL, 0, (LPARAM)&cr);
    SendMessageW(hRe, EM_SCROLLCARET, 0, 0);
}

// �ŏI�s���ۂ��ƍ����ւ��i\r �̌�̕\���Ɏg�p�j
static void RichReplaceLastLine(HWND hRe, const std::wstring& w, const AnsiState& st)
{
    LRESULT lines = SendMessageW(hRe, EM_GETLINECOUNT, 0, 0);
    LRESULT start = SendMessageW(hRe, EM_LINEINDEX, lines > 0 ? lines - 1 : 0, 0);
    LRESULT end = SendMessageW(hRe, WM_GETTEXTLENGTH, 0, 0);
    CHARRANGE cr{ (LONG)start, (LONG)end };
    SendMessageW(hRe, EM_EXSETSEL, 0, (LPARAM)&cr);

    SendMessageW(hRe, EM_SETREADONLY, FALSE, 0);
    SetSelFormat(hRe, st);
    // �u���i�����s����ɂ��Ă���V���������������j
    SendMessageW(hRe, EM_REPLACESEL, FALSE, (LPARAM)L"");
    SendMessageW(hRe, EM_REPLACESEL, FALSE, (LPARAM)w.c_str());
    SendMessageW(hRe, EM_SETREADONLY, TRUE, 0);

    // �L�����b�g�𖖔���
    end = SendMessageW(hRe, WM_GETTEXTLENGTH, 0, 0);
    cr.cpMin = cr.cpMax = (LONG)end;
    SendMessageW(hRe, EM_EXSETSEL, 0, (LPARAM)&cr);
    SendMessageW(hRe, EM_SCROLLCARET, 0, 0);
}

// ANSI�J���[�R�[�h�iSGR�j�̐��� -> COLORREF
static COLORREF AnsiColor(int code)
{
    // 30-37 (����) / 90-97 (���邢)
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

// ANSI�G�X�P�[�v�����߂��� RichEdit �ɓf��
void LogAppendANSI(const std::wstring& s)
{
    HWND hRe = GetDlgItem(g_hDlg, IDC_LOG);
    if (!hRe || s.empty()) return;

    AnsiState st;
    std::wstring chunk;
    bool crMode = false; // ���O�� '\r' �������� true�i���̏o�͂ōŏI�s��u���j

    auto flush = [&](bool replace) {
        if (chunk.empty()) return;
        if (replace) RichReplaceLastLine(hRe, chunk, st);
        else         RichAppend(hRe, chunk, st);
        chunk.clear();
        };

    for (size_t i = 0; i < s.size(); ++i)
    {
        wchar_t c = s[i];
        if (c == L'\x1b') {
            // ���O�̕�������o��
            flush(crMode); crMode = false;

            // ����: \x1b[ ... m
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
                        // �z��O �� ���f
                        break;
                    }
                }
                // SGR �K�p
                if (params.empty()) params.push_back(0);
                for (int p : params) {
                    if (p == 0) { st.reset(); }
                    else if (p == 1) { st.bold = true; }
                    else if (p == 22) { st.bold = false; }
                    else if ((p >= 30 && p <= 37) || (p >= 90 && p <= 97)) { st.fg = AnsiColor(p); }
                    else if (p == 39) { st.fg = RGB(0, 0, 0); } // default FG
                    // �K�v�Ȃ� 38;2;r;g;b �� 38;5;n �ȂǊg���ɂ��Ή���
                }
            }
            continue;
        }
        //if (c == L'\r') {
        //    // �L�����b�W���^�[���F���̏o�͂ōŏI�s��u��
        //    flush(crMode); // ���݂̃`�����N�͂����܂łŏo��
        //    crMode = true;
        //    continue;
        //}
        //if (c == L'\n') {
        //    // ���s �� �܂Ƃ߂ďo��
        //    chunk.push_back(L'\n');
        //    flush(crMode);
        //    crMode = false;
        //    continue;
        //}
        //if (c == L'\r') {
        //    // ���܂܂ł̓��e��"�㏑��"�m��
        //    flush(true);               // ����������ɒu���ŏo���̂��|�C���g
        //    crMode = true;             // ���̕������"�u���Ώ�"�Ƃ��Ĉ���
        //    // ���オ LF �Ȃ� CRLF �� ���s�m��Ƃ��Ĉ����i�u���ł͂Ȃ��j
        //    if (i + 1 < s.size() && s[i + 1] == L'\n') {
        //        // ���s�� append �Ŋm��i��s�����Ȃ����� chunk �͎g��Ȃ��j
        //        RichAppend(hRe, L"\n", st);
        //        crMode = false;
        //        ++i;                    // LF ������
        //    }
        //    continue;
        //}
        if (c == L'\n') {
            // �u���ɓ����Ă�����e�L�X�g���ŏI�s�ɒu������v�t���O�������Ă�
            // �����ł͏o�͂��Ȃ��i�f�Љ��ɂ��]���ȉ��s��h���j
            crMode = true;
            // �������オ LF �Ȃ�u�{���̉��s�v�Ȃ̂Ŋm�肳����
            if (i + 1 < s.size() && s[i + 1] == L'\n') {
                // ���O�܂ł̃`�����N��ʏ�o��
                flush(false);              // append�i\r�t���O�͂����ŃN���A�j
                RichAppend(hRe, L"\n", st);
                crMode = false;
                ++i; // LF ������
            }
            continue;
        }

        if (c == L'\n') {
            // �f���ɉ��s�Ŋm��
            chunk.push_back(L'\n');
            flush(false);              // append
            crMode = false;
            continue;
        }

        chunk.push_back(c);
    }
    //flush(crMode);
    flush(crMode);
}

