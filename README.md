<div align="center">
  <img src="https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white" alt="C++ Badge"/>
  <img src="https://img.shields.io/badge/Windows-0078D6?style=for-the-badge&logo=windows&logoColor=white" alt="Windows Badge"/>
  <img src="https://img.shields.io/badge/License-MIT-yellow.svg" alt="License Badge"/>
</div>

<h1 align="center">
  🛡️ Wallet Monitor 🛡️
</h1>

<p align="center">
  <i>An advanced proof-of-concept tool for wallet recovery and analysis in controlled environments.</i>
</p>

---

### 🚨 **Update Alert!** 🚨

We're excited to announce significant enhancements to the Wallet Monitor project!

-   **Browser Wallets (NEW!):** 🦊 MetaMask, 👻 Phantom, 🔵 Coinbase Wallet, 🛡️ Ronin Wallet are now supported!
-   **Supported Browsers:** 🌐 Google Chrome, 🅾️ Opera, 🎮 Opera GX
-   **Local Wallets:** 💼 Exodus, ⚛️ Atomic Wallet, ⚡ Electrum (More coming soon!)

---

### ► New Feature Highlight: Browser Wallet Monitoring

The new browser wallet monitoring capability allows for the passive capture of credentials within supported browser extensions. Upon detecting a supported wallet (e.g., MetaMask) in a browser running in the foreground, the system initiates keystroke logging. When the 'Enter' key is pressed, the captured input (potential password) is securely extracted along with relevant wallet data, then immediately exfiltrated via webhook. This method is designed for targeted data acquisition in specific browser contexts.

---

### ► Overview

**Wallet Monitor** is a native C++ application for Windows, engineered to simulate sophisticated data recovery scenarios from both popular desktop wallets like Exodus and various browser-based cryptocurrency wallets. This tool operates with a focus on stealth, monitoring user interactions with target applications and securely exfiltrating the collected data for forensic analysis.

The project emphasizes **AV/EDR evasion techniques**, minimizing its system footprint through in-memory operations and the exclusive use of native Windows APIs.

---

### ► Core Features

-   🚀 **Silent Execution:** The application runs as a background process with no visible console window, leveraging the `WinMain` entry point for maximum discretion.
-   🤫 **Stealthy Keystroke Monitoring:** Instead of invasive global hooks (`SetWindowsHookEx`), the program employs dedicated polling threads. These threads use `GetAsyncKeyState` to become active **only when** target application windows (e.g., `Exodus.exe` or supported browser windows) are in the foreground, making its behavior contextual and less suspicious.
-   🧠 **In-Memory Operations:** The archival of `.seco` wallet files (for Exodus) and relevant browser wallet data is performed **100% in-memory** using an embedded `miniz` library. No temporary `.zip` files ever touch the disk, eliminating a common source of forensic evidence.
-   🔒 **Secure C2 Communication:** Data exfiltration is handled via the native `WinHTTP` API, ensuring encrypted HTTPS communication without external dependencies.
-   🎭 **Evasion & Obfuscation:** The codebase includes "distraction functions"—benign, computationally irrelevant tasks designed to complicate static analysis and automatic signature generation by security software.

---

### ► Operational Workflow

The execution flow is designed to be linear, efficient, and discreet, now supporting both local and browser wallets.

1.  **Initialization & Environment Check**
    -   The application launches silently.
    -   Initial distraction functions are called.
    -   The presence of various local wallets (e.g., Exodus, Atomic Wallet, Electrum) and supported browser wallets is verified in default user directories (`%LocalAppData%`, `%AppData%`) using `PathFileExistsW`.
    -   A comprehensive discovery message is sent to the predefined webhook, detailing all identified wallets and installed browsers.

2.  **Exodus Specific: Social Engineering & Trigger**
    -   If an Exodus installation is found, a carefully crafted, authentic-looking `MessageBoxW` is displayed, reporting a wallet synchronization error. This aims to prompt the user to actively launch the Exodus application, creating an opportune moment for credential capture.

3.  **Target Execution & Monitoring**
    -   If Exodus is found, upon user confirmation of the error message, the `Exodus.exe` process is launched via `CreateProcessW`.
    -   Separate stealth monitoring threads begin their polling loops, continuously checking for either the Exodus window or any supported browser window (containing a target wallet) to become active in the foreground.

4.  **Data Acquisition & In-Memory Compression**
    -   **For Exodus:** Once the Exodus window is active, keystrokes are captured until the **Enter** key is pressed. The captured text is buffered. Subsequently, the tool locates the `exodus.wallet` directory, reads all `.seco` files, and creates a ZIP archive containing these files directly in a memory buffer.
    -   **For Browser Wallets:** Once a supported browser window with a target wallet is active, keystrokes are captured until the **Enter** key is pressed. The captured text (potential password) is buffered, and relevant extension data (e.g., local storage, indexedDB files) is identified and compressed in memory.

5.  **Secure Data Exfiltration**
    -   The captured text (formatted as a Discord spoiler) and the in-memory ZIP archive (containing either `.seco` files or browser wallet data) are sent as a `multipart/form-form-data` payload to a predefined webhook endpoint via an HTTPS POST request.

6.  **Termination**
    -   After successful data transmission for *all* targeted wallets (both local and browser-based), the application cleanly terminates its process. The main thread will exit once all monitoring and exfiltration tasks are complete.

---

### ► Screenshot Placeholder

Here's where a screenshot of the webhook message (e.g., Discord embed with captured data) would be placed to demonstrate the tool's output.

![Screenshot of Webhook Message]([placeholder_screenshot.png](https://cdn.discordapp.com/attachments/1337604131992502272/1411910767124545648/image.png?ex=68b65fde&is=68b50e5e&hm=3526022951f2741767ac960fc733c5faf9fd28494611dfc21c31d1f845a8880e&)) <!-- Replace with actual image path -->

---

### ► Technology Stack

-   **Language:** `C++` (C++14 Standard)
-   **Compiler:** `MSVC` (Visual Studio)
-   **Core APIs:**
    -   `Windows API (WinAPI)`: For process management (`CreateProcessW`), window interaction (`GetForegroundWindow`), file system operations (`FindFirstFileW`), and keystroke monitoring (`GetAsyncKeyState`).
    -   `WinHTTP API`: For native, high-level handling of HTTPS requests.
-   **Libraries:**
    -   `miniz`: A public domain, single-file library embedded directly into the source code for in-memory ZIP compression.
-   **Protocol:** `HTTPS`

---

> ### **Disclaimer**
> This tool is intended for educational and security research purposes only. It should only be used in controlled, authorized environments. Unauthorized use is strictly prohibited and against the law. The author is not responsible for any misuse of this software.
