<div align="center">
  <img src="https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white" alt="C++ Badge"/>
  <img src="https://img.shields.io/badge/Windows-0078D6?style=for-the-badge&logo=windows&logoColor=white" alt="Windows Badge"/>
  <img src="https://img.shields.io/badge/License-MIT-yellow.svg" alt="License Badge"/>
</div>

<h1 align="center">
  🛡️ Exodus Settler 🛡️
</h1>

<p align="center">
  <i>An advanced proof-of-concept tool for wallet recovery and analysis in controlled environments.</i>
</p>

---

### ► Overview

**Exodus Settler** is a native C++ application for Windows, engineered to simulate sophisticated data recovery scenarios from the Exodus wallet. This tool operates with a focus on stealth, monitoring user interactions with the target application and securely exfiltrating the collected data for forensic analysis.

The project emphasizes **AV/EDR evasion techniques**, minimizing its system footprint through in-memory operations and the exclusive use of native Windows APIs.

---

### ► Core Features

-   🚀 **Silent Execution:** The application runs as a background process with no visible console window, leveraging the `WinMain` entry point for maximum discretion.
-   🤫 **Stealthy Keystroke Monitoring:** Instead of invasive global hooks (`SetWindowsHookEx`), the program employs a dedicated polling thread. This thread uses `GetAsyncKeyState` to become active **only when** the `Exodus.exe` window is in the foreground, making its behavior contextual and less suspicious.
-   🧠 **In-Memory Operations:** The archival of `.seco` wallet files is performed **100% in-memory** using an embedded `miniz` library. No temporary `.zip` files ever touch the disk, eliminating a common source of forensic evidence.
-   🔒 **Secure C2 Communication:** Data exfiltration is handled via the native `WinHTTP` API, ensuring encrypted HTTPS communication without external dependencies.
-   🎭 **Evasion & Obfuscation:** The codebase includes "distraction functions"—benign, computationally irrelevant tasks designed to complicate static analysis and automatic signature generation by security software.

---

### ► Operational Workflow

The execution flow is designed to be linear, efficient, and discreet.

1.  **Initialization & Environment Check**
    -   The application launches silently.
    -   Initial distraction functions are called.
    -   The presence of an Exodus installation is verified in default user directories (`%LocalAppData%`, `%AppData%`) using `PathFileExistsW`.

2.  **Social Engineering & Trigger**
    -   A carefully crafted, authentic-looking `MessageBoxW` is displayed, reporting a wallet synchronization error to prompt the user to launch the application.

3.  **Target Execution & Monitoring**
    -   Upon user confirmation, the `Exodus.exe` process is launched via `CreateProcessW`.
    -   The stealth monitoring thread begins its polling loop, waiting for the Exodus window to become active.

4.  **Data Acquisition & In-Memory Compression**
    -   Once the target window is active, keystrokes are captured until the **Enter** key is pressed.
    -   The captured text is buffered.
    -   The tool locates the `exodus.wallet` directory, reads all `.seco` files, and creates a ZIP archive directly in a memory buffer.

5.  **Secure Data Exfiltration**
    -   The captured text (formatted as a Discord spoiler) and the in-memory ZIP archive are sent as a `multipart/form-data` payload to a predefined webhook endpoint via an HTTPS POST request.

6.  **Termination**
    -   After successful data transmission, the application cleanly terminates its process.

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
