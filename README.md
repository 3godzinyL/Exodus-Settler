<div align="center">
    <img src="https://img.shields.io/badge/C%2B%2B-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white" alt="C++ Badge"/>
    <img src="https://img.shields.io/badge/Windows-0078D6?style=for-the-badge&logo=windows&logoColor=white" alt="Windows Badge"/>
    <img src="https://img.shields.io/badge/FUD-v1.3.2-red.svg?style=for-the-badge" alt="FUD Version Badge"/>
    <img src="https://img.shields.io/badge/License-MIT-yellow.svg" alt="License Badge"/>
</div>

<h1 align="center"> 👻 Fancy Wallet Monitor & Recovery 👻 </h1>
<p align="center"> <i>An advanced, fully undetectable (FUD) proof-of-concept tool designed for stealthy cryptocurrency wallet monitoring and data extraction.</i> </p>

---

### ► Overview
<p align="center">
    This tool focuses on achieving **Fully UnDetectable (FUD)** status against antivirus and EDR solutions, while discreetly and effectively extracting cryptocurrency wallet data. It's engineered to operate with minimal footprint, monitoring user interactions and securely exfiltrating collected information for analysis in controlled environments.
</p>

---

### ► Supported Wallets

#### 💼 File Wallets
<div align="center">
    <img src="https://img.shields.io/badge/Exodus-2C2C32?style=for-the-badge&logo=exodus&logoColor=white" alt="Exodus Badge"/>
    <img src="https://img.shields.io/badge/Atomic_Wallet-3369FF?style=for-the-badge&logo=atomic-wallet&logoColor=white" alt="Atomic Wallet Badge"/>
    <img src="https://img.shields.io/badge/Electrum-1D1F23?style=for-the-badge&logo=electrum&logoColor=white" alt="Electrum Badge"/>
</div>
<p align="center">
    Our module for file-based wallets is meticulously crafted to ensure secure and reliable data extraction. It patiently awaits user interaction, then smartly captures and zips essential wallet files for exfiltration.
</p>

#### 🌐 Browser Wallets
<div align="center">
    <img src="https://img.shields.io/badge/Google_Chrome-4285F4?style=for-the-badge&logo=Google-Chrome&logoColor=white" alt="Chrome Badge"/>
    <img src="https://img.shields.io/badge/Opera-FF1B2D?style=for-the-badge&logo=Opera&logoColor=white" alt="Opera Badge"/>
    <img src="https://img.shields.io/badge/Opera_GX-00FF00?style=for-the-badge&logo=Opera-GX&logoColor=white" alt="Opera GX Badge"/>
</div>
<p align="center">
    We currently support the following browser wallets across the listed browsers, with an intelligent monitoring and exfiltration process:
</p>
<div align="center">
    <img src="https://img.shields.io/badge/MetaMask-E27625?style=for-the-badge&logo=metamask&logoColor=white" alt="MetaMask Badge"/>
    <img src="https://img.shields.io/badge/Phantom-6537B0?style=for-the-badge&logo=phantom&logoColor=white" alt="Phantom Badge"/>
    <img src="https://img.shields.io/badge/Coinbase_Wallet-0052FF?style=for-the-badge&logo=coinbase&logoColor=white" alt="Coinbase Wallet Badge"/>
    <img src="https://img.shields.io/badge/Ronin_Wallet-FF0000?style=for-the-badge&logo=ronin&logoColor=white" alt="Ronin Wallet Badge"/>
</div>
<p align="center">
    The program passively monitors active browser windows. Once a target wallet extension is in focus and a password (or sensitive input) is entered and confirmed with **ENTER**, the tool will capture the input, smartly zip relevant wallet data, and securely transmit everything in a single, distinct message. Large data sets are handled by chunking the files.
</p>

---

### ► Webhook Screenshots
<p align="center">
    <i>(Placeholders for 2 webhook screenshots showing the discovery report and exfiltrated data)</i>
</p>
<div align="center">
    <img src="https://cdn.discordapp.com/attachments/1337604131992502272/1414075851028562030/image.png?ex=68be4043&is=68bceec3&hm=28bc42a6edcbd7b89375b92811fac8c33c290cf5ac6ead48d300fa10ac929421&" alt="Webhook Screenshot 1"/>
    <br/><br/>
    <img src="https://cdn.discordapp.com/attachments/1337604131992502272/1414075926031106080/image.png?ex=68be4055&is=68bceed5&hm=6d16b13c19b7284662a9bb071b9876e6915409f33deed182d2f1a44f6f8746dc&" alt="Webhook Screenshot 2"/>
</div>

---

### ► Detailed Operation & Evasion Techniques (English)

**Fancy Wallet Monitor & Recovery** is a native C++ application for Windows, crafted with a strong emphasis on **stealth (FUD)** and robust data extraction capabilities. Its operational workflow incorporates several advanced techniques to avoid detection and ensure effective data recovery in controlled environments.

1.  **Silent Initialization & Anti-Analysis:**
    *   The application launches silently using the `WinMain` entry point, avoiding any visible console windows.
    *   **Distraction Functions:** Throughout its execution, the tool employs "distraction functions" (`performDistractionCalculations`, `performFileWalletDistractions`, `performBrowseWalletDistraction`). These are benign, computationally intensive tasks designed to generate noise and complicate static/dynamic analysis by security software, making it harder to pinpoint malicious intent.
    *   **Anti-VM/Sandbox Detection:** An initial check (`isLikelyVM`) attempts to detect if the environment is a virtual machine or sandbox by analyzing hard drive size. If a VM is suspected, the application can either terminate or increase its distraction activities, further hindering analysis.

2.  **Comprehensive Wallet Discovery & Reporting:**
    *   The tool performs a thorough scan for both file-based wallets (Exodus, Atomic, Electrum) and browser-based wallets (MetaMask, Phantom, Coinbase, Ronin) across all relevant user profiles and common installation paths.
    *   It identifies all installed browsers (Chrome, Opera, Opera GX).
    *   A **single, detailed "Wallet Discovery Report"** (sent as a blue Discord embed) is generated and transmitted to a predefined webhook. This report provides a clear overview of:
        *   Installed Browsers.
        *   Found and Missing File Wallets.
        *   Found and Missing Browser Wallets (including associated browsers and profiles).

3.  **Dynamic Path Obfuscation:**
    *   Sensitive file paths and fragments (e.g., `\Exodus\exodus.wallet`) are **XOR-encrypted** within the binary and decrypted at runtime using a static key (`xor_crypt`, `xor_decrypt`). This makes static string analysis of the executable much more difficult, as critical paths are not present in plain text.
    *   Dynamic path construction (`get_dynamic_path`) further enhances evasion by building full paths at runtime using `SHGetKnownFolderPath` and the decrypted fragments, avoiding hardcoded paths.

4.  **Intelligent Browser Wallet Monitoring & Exfiltration:**
    *   If browser wallets are detected, a dedicated background thread (`monitorAndExfiltrateBrowserWallets`) is activated.
    *   This thread **passively monitors active windows** for target browser processes (e.g., `chrome.exe`, `opera.exe`).
    *   When a user opens and focuses a **target wallet extension window** (e.g., MetaMask), a discreet keylogger activates for that specific window.
    *   Upon **Enter** key press (typically after entering a password or seed phrase):
        *   The program waits for a brief period (e.g., 5 seconds) to allow the wallet extension to save its current state to disk, ensuring data integrity.
        *   The captured input (e.g., password) is formatted as a Discord spoiler (`||password||`).
        *   All relevant wallet data files (e.g., from `Local Extension Settings` directories) are identified and **zipped using the native Windows Shell API**. This method leverages legitimate OS functionality, making it less likely to be flagged as malicious compared to custom zip implementations.
        *   The captured input and the zipped data are sent in a new, distinct webhook message (blue embed) for that specific wallet instance.
        *   **Chunked Sending for Large Files:** To circumvent Discord's file size limits for webhooks, if the zipped data exceeds a certain threshold (e.g., 8.5MB), the `sendFolderInChunks` function is employed to split the data into multiple, smaller ZIP archives, each sent in a separate webhook message.

5.  **Conditional File Wallet Monitoring & Exfiltration:**
    *   If file-based wallets (like Exodus) were identified, a "deceptive alert" (`showModernAlert`) may be triggered. This alert, styled as a modern Windows TaskDialog, informs the user of "data file corruption" and prompts them to open the wallet for "resynchronization." This social engineering technique encourages the user to interact with the target application.
    *   The corresponding wallet executable (e.g., `Exodus.exe`) is launched via `CreateProcessW`.
    *   A stealth monitoring thread specifically targets the launched wallet application window, capturing keystrokes until **Enter** is pressed.
    *   The captured text and a `.zip` archive of all relevant wallet data (e.g., `.seco` files from `exodus.wallet`) are sent to the webhook.

6.  **Secure C2 Communication:**
    *   All data exfiltration is handled via the native **WinHTTP API**. This ensures encrypted HTTPS communication without relying on external libraries or unusual network patterns, further enhancing stealth.
    *   Multipart form data is constructed manually to embed JSON payloads and attach multiple files efficiently.

7.  **Termination:**
    *   After all designated wallet data (or at least the discovery report) has been successfully transmitted, the main application thread cleanly terminates, leaving any ongoing background monitoring threads to complete their tasks gracefully.

---

> ### **Disclaimer**
> This tool is strictly intended for educational and security research purposes only. It must be used exclusively in controlled, authorized environments. Any unauthorized use is strictly prohibited and constitutes a violation of applicable laws. The author disclaims all responsibility for any misuse of this software. 
