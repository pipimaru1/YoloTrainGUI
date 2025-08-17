// pch.h: プリコンパイル済みヘッダー ファイルです。
// 次のファイルは、その後のビルドのビルド パフォーマンスを向上させるため 1 回だけコンパイルされます。
// コード補完や多くのコード参照機能などの IntelliSense パフォーマンスにも影響します。
// ただし、ここに一覧表示されているファイルは、ビルド間でいずれかが更新されると、すべてが再コンパイルされます。
// 頻繁に更新するファイルをここに追加しないでください。追加すると、パフォーマンス上の利点がなくなります。

#ifndef PCH_H
#define PCH_H


// プリコンパイルするヘッダーをここに追加します
#include "framework.h"

#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <shlwapi.h>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <random>
#include <thread>
#include <atomic>
#include <mutex>
#include <sstream>
#include <iomanip>

#include <fstream>
#include <map>
#include <KnownFolders.h>  // FOLDERID_LocalAppData
#include <shellapi.h>

#include <richedit.h>
#include <richole.h>

#include <commdlg.h>   // GetOpenFileName

#endif //PCH_H
