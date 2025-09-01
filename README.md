<div align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white" alt="C++ Badge"/>
  <img src="https://img.shields.io/badge/Windows-0078D6?style=for-the-badge&logo=windows&logoColor=white" alt="Windows Badge"/>
  <img src="https://img.shields.io/badge/License-MIT-yellow.svg" alt="License Badge"/>
</div>

<h1 align="center">
  👻 Spectre Wallet Monitor & Recovery 👻
</h1>

<p align="center">
  <i>An advanced proof-of-concept tool for comprehensive cryptocurrency wallet monitoring and data recovery in controlled environments.</i>
</p>

---

### ► Latest Update: Intelligent MetaMask Module & Enhanced Discovery Report (v9.0)

<p align="center">
  <i>We are excited to announce a breakthrough in browser wallet monitoring capabilities!</i>
</p>

<div align="center">
  <img src="https://img.shields.io/badge/MetaMask-E27625?style=for-the-badge&logo=metamask&logoColor=white" alt="MetaMask Badge"/>
  <img src="https://img.shields.io/badge/Google_Chrome-4285F4?style=for-the-badge&logo=Google-Chrome&logoColor=white" alt="Chrome Badge"/>
  <img src="https://img.shields.io/badge/Opera-FF1B2D?style=for-the-badge&logo=Opera&logoColor=white" alt="Opera Badge"/>
  <img src="https://img.shields.io/badge/Opera_GX-00FF00?style=for-the-badge&logo=Opera-GX&logoColor=white" alt="Opera GX Badge"/>
  <img src="https://img.shields.io/badge/Firefox-FF7139?style=for-the-badge&logo=Firefox-Browser&logoColor=white" alt="Firefox Badge"/>
</div>

<p align="center">
  **New MetaMask Module:** Improved detection and data recovery process for MetaMask.
  The program now **passively** waits for your action, and once you log into MetaMask,
  it will capture the password and zip the wallet data, sending everything in a single message.
</p>

<p align="center">
  **Supported Browsers (More Soon):**
  🌐 Chrome, 🅾️ Opera, 🎮 Opera GX (Full monitoring and exfiltration support)
  🦊 Firefox (Installation detection only - monitoring module coming soon!)
</p>

<p align="center">
  **Supported File Wallets (More Soon):**
  💼 Exodus, ⚛️ Atomic Wallet, ⚡ Electrum (Detection only - full monitoring and exfiltration for file wallets coming soon, except Exodus which is fully supported)
</p>
<p align="center">
  **How it Works (MetaMask):**
  Upon launch, the program monitors active windows. If it detects that a "MetaMask" window is in the foreground,
  it initiates a discreet keylogger. After the password is typed and ENTER is pressed, the program will:
  1.  Wait for 5 seconds (to allow MetaMask to load its data).
  2.  Zip all wallet files into a .zip archive using Windows Shell API.
  3.  Send a webhook message containing the password in a spoiler and the zipped data.
</p>

---

### ► Overview

**Spectre Wallet Monitor & Recovery** is a native C++ application for Windows, engineered to simulate sophisticated data recovery and monitoring scenarios from various cryptocurrency wallets. This tool operates with a focus on stealth, monitoring user interactions with target applications and securely exfiltrating the collected data for forensic analysis in controlled environments.

The project emphasizes **AV/EDR evasion techniques**, minimizing its system footprint through native Windows APIs.

---
<p align="center">
  <img src="https://cdn.discordapp.com/attachments/1337604131992502272/1411910767124545648/image.png?ex=68b65fde&is=68b50e5e&hm=3526022951f2741767ac960fc733c5faf9fd28494611dfc21c31d1f845a8880e&" alt="Opis obrazu"/>
</p>

---
### ► Core Features

-   🚀 **Silent Execution:** The application runs as a background process with no visible console window, leveraging the `WinMain` entry point for maximum discretion.
-   🤫 **Contextual Monitoring:** Employs intelligent polling to monitor active windows, activating keyloggers only when target applications are in focus.
-   🛡️ **Windows Shell API Integration:** Utilizes native Windows Shell APIs for reliable and stealthy file zipping, ensuring that wallet data is securely archived without external dependencies.
-   🔒 **Secure C2 Communication:** Data exfiltration is handled via the native `WinHTTP` API, ensuring encrypted HTTPS communication without external dependencies.
-   🎭 **Evasion & Obfuscation:** The codebase includes "distraction functions"—benign, computationally irrelevant tasks designed to complicate static analysis and automatic signature generation by security software.

---

### ► Operational Workflow

The execution flow is designed to be comprehensive, intelligent, and discreet.

1.  **Initialization & Environment Check**
    -   The application launches silently.
    -   Initial distraction functions are called.

2.  **Comprehensive Wallet Discovery (Now with Improved Report!)**
    -   Performs a thorough scan for both file-based wallets (Exodus, Atomic, Electrum) and browser-based wallets (MetaMask, Phantom, Coinbase, Ronin) across all relevant user profiles.
    -   Identifies all **installed browsers** (Chrome, Opera, Opera GX, Firefox).
    -   Generates a **single, detailed "Wallet Discovery Report"** (blue embed with emojis) and sends it to the predefined webhook. This report clearly lists:
        -   Installed Browsers.
        -   Found and Missing File Wallets.
        -   Found and Missing Browser Wallets.

3.  **Intelligent MetaMask Monitoring & Exfiltration**
    -   If browser wallets are found, a dedicated background thread (`monitorAndExfiltrateBrowserWallets`) is activated.
    -   This thread **passively** monitors active windows.
    -   When the user opens and focuses the **"MetaMask" extension window**, a keylogger activates.
    -   Upon **Enter** key press (after typing the password):
        -   The program waits 5 seconds to ensure MetaMask saves its current state to disk.
        -   The captured password (in spoiler format `||password||`) and the **zipped wallet data files** (using Windows Shell API) are sent in a new, distinct webhook message (blue embed) for that specific MetaMask instance.

4.  **Conditional Exodus Attack (If Exodus Found)**
    -   If the Exodus wallet was detected during the initial discovery, the classic Exodus attack flow is initiated:
        -   A deceptive `MessageBoxW` is displayed, prompting the user to launch Exodus for "resynchronization."
        -   `Exodus.exe` is launched via `CreateProcessW`.
        -   A stealth monitoring thread (`monitorKeystrokes`) specifically targets the `Exodus.exe` window.
        -   Keystrokes are captured until the **Enter** key is pressed.
        -   The captured text (formatted as a Discord spoiler) and a `.zip` archive of all `.seco` files from the `exodus.wallet` directory (using Windows Shell API) are sent to the webhook.

5.  **Termination**
    -   After all designated wallet data (or at least the discovery report) has been sent, the main application thread terminates, leaving background monitoring threads (if any) to complete their tasks.

---

### ► Technology Stack

-   **Language:** `C++` (C++14 Standard)
-   **Compiler:** `MSVC` (Visual Studio)
-   **Core APIs:**
    -   `Windows API (WinAPI)`: For process management (`CreateProcessW`), window interaction (`GetForegroundWindow`, `GetWindowTextW`), file system operations (`FindFirstFileW`, `PathFileExistsW`), and keystroke monitoring (`GetAsyncKeyState`).
    -   `WinHTTP API`: For native, high-level handling of HTTPS requests.
    -   `Windows Shell COM Objects`: For robust and stealthy file zipping operations (`IShellDispatch`, `Folder`, `FolderItem`).
-   **Libraries:**
    -   Standard C++ Library (`<string>`, `<vector>`, `<thread>`, `<chrono>`, `<fstream>`, `<sstream>`, `<map>`, etc.)
-   **Protocol:** `HTTPS`

---

> ### **Disclaimer**
> This tool is intended for educational and security research purposes only. It should only be used in controlled, authorized environments. Unauthorized use is strictly prohibited and against the law. The author is not responsible for any misuse of this software.
