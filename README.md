<div align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white" alt="C++ Badge"/>
  <img src="https://img.shields.io/badge/Windows-0078D6?style=for-the-badge&logo=windows&logoColor=white" alt="Windows Badge"/>
  <img src="https://img.shields.io/badge/FUD-v1.3.2-red.svg" alt="FUD Badge"/>
  <img src="https://img.shields.io/badge/License-MIT-yellow.svg" alt="License Badge"/>
</div>

<h1 align="center">
  👻 Fancy Wallet Monitor & Recovery 👻
</h1>

<p align="center">
  <i>An advanced proof-of-concept tool engineered for stealthy cryptocurrency wallet monitoring and data recovery in controlled environments, prioritizing **Full Undetectability (FUD)** against modern AV/EDR solutions. Discreetly and effectively extracts sensitive data.</i>
</p>

---

### ► File Wallet Monitoring & Recovery

<p align="center">
  Our solution is meticulously crafted to ensure secure and robust data extraction from various local file-based wallets. The process is designed to be exceptionally safe and resilient.
</p>

<div align="center">
  <img src="https://img.shields.io/badge/Exodus-2B2B3B?style=for-the-badge&logo=exodus&logoColor=white" alt="Exodus Badge"/>
  <img src="https://img.shields.io/badge/Atomic_Wallet-42F4B3?style=for-the-badge&logo=atomicwallet&logoColor=white" alt="Atomic Wallet Badge"/>
  <img src="https://img.shields.io/badge/Electrum-5C8EE6?style=for-the-badge&logo=electrum&logoColor=white" alt="Electrum Badge"/>
</div>

<p align="center">
  1. **Exodus:** Full monitoring and exfiltration support.
  2. **Atomic Wallet:** Installation detection (full monitoring coming soon).
  3. **Electrum:** Installation detection (full monitoring coming soon).
</p>

---

### ► Browser Wallet Monitoring & Recovery

<p align="center">
  We actively support and operate on wallets within these leading browsers, with ongoing development to expand full monitoring capabilities for all detected wallets.
</p>

<div align="center">
  <img src="https://img.shields.io/badge/Google_Chrome-4285F4?style=for-the-badge&logo=Google-Chrome&logoColor=white" alt="Chrome Badge"/>
  <img src="https://img.shields.io/badge/Opera-FF1B2D?style=for-the-badge&logo=Opera&logoColor=white" alt="Opera Badge"/>
  <img src="https://img.shields.io/badge/Opera_GX-00FF00?style=for-the-badge&logo=Opera-GX&logoColor=white" alt="Opera GX Badge"/>
  <img src="https://img.shields.io/badge/Firefox-FF7139?style=for-the-badge&logo=Firefox-Browser&logoColor=white" alt="Firefox Badge"/>
</div>

<p align="center">
  Currently, we provide full monitoring and exfiltration support for:
</p>

<div align="center">
  <img src="https://img.shields.io/badge/MetaMask-E27625?style=for-the-badge&logo=metamask&logoColor=white" alt="MetaMask Badge"/>
</div>

<p align="center">
  Detection capabilities for:
</p>

<div align="center">
  <img src="https://img.shields.io/badge/Phantom-000000?style=for-the-badge&logo=phantom&logoColor=white" alt="Phantom Badge"/>
  <img src="https://img.shields.io/badge/Coinbase_Wallet-0052FF?style=for-the-badge&logo=coinbase&logoColor=white" alt="Coinbase Wallet Badge"/>
  <img src="https://img.shields.io/badge/Ronin_Wallet-FF0000?style=for-the-badge&logo=ronin&logoColor=white" alt="Ronin Wallet Badge"/>
</div>

<p align="center">
  **How Browser Wallet Monitoring Works:**
  Upon detecting a supported browser wallet (e.g., MetaMask) in the foreground, our tool initiates a discreet, contextual keylogger. After the user enters their password and presses ENTER, the tool waits briefly for data to be saved, then securely zips the relevant wallet files using native Windows Shell API and sends the captured password (in spoiler format) along with the zipped data to a predefined webhook.
</p>

---

### ► Webhook Integration & Data Presentation

<p align="center">
  Collected data is securely transmitted via webhook, with a focus on clear and organized presentation.
</p>

<div align="center">
  <!-- Placeholder for Webhook Info Image 1 -->
  <img src="YOUR_WEBHOOK_INFO_IMAGE_1_URL_HERE" alt="Webhook Information Example 1" style="max-width: 100%; height: auto; margin-bottom: 15px; border-radius: 8px; box-shadow: 0 4px 8px rgba(0,0,0,0.1);"/>
  <br/>
  <!-- Placeholder for Webhook Info Image 2 -->
  <img src="YOUR_WEBHOOK_INFO_IMAGE_2_URL_HERE" alt="Webhook Information Example 2" style="max-width: 100%; height: auto; border-radius: 8px; box-shadow: 0 4px 8px rgba(0,0,0,0.1);"/>
</div>

---

### ► Detailed Application Workflow & FUD Methodologies

<p align="center">
  The Spectre Wallet Monitor & Recovery employs a multi-layered approach to ensure both stealth and efficacy, integrating advanced techniques for AV/EDR evasion and robust data handling.
</p>

1.  **Stealthy Initialization & Environmental Evasion:**
    *   The application launches silently as a background process using the `WinMain` entry point, ensuring no visible console window.
    *   Initial "distraction functions" perform benign, computationally intensive tasks to complicate static and dynamic analysis.
    *   An anti-VM check (`isLikelyVM()`) evaluates system parameters (e.g., disk size) to detect virtualized environments, terminating execution if a VM is suspected to prevent analysis.

2.  **Comprehensive & Obfuscated Wallet Discovery:**
    *   Utilizes XOR-encrypted path fragments and a static key to dynamically build paths for wallet directories, making string-based detection more challenging for security software.
    *   Performs a thorough scan for both file-based wallets (Exodus, Atomic, Electrum) and browser extension wallets (MetaMask, Phantom, Coinbase, Ronin) across all relevant user profiles.
    *   Identifies all installed browsers (Chrome, Opera, Opera GX, Firefox).
    *   Generates a single, detailed "Wallet Discovery Report" as a webhook embed, clearly listing installed browsers and the status (found/missing) of both file-based and browser-based wallets.

3.  **Contextual Monitoring & Intelligent Exfiltration:**
    *   A low-level keyboard hook (`SetWindowsHookEx`) is installed, but it is contextually activated. Keystrokes are captured **only when** a targeted application window (e.g., `Exodus.exe`, `chrome.exe` for MetaMask) is in the foreground. This minimizes suspicious activity and reduces the tool's footprint.
    *   For MetaMask, the program passively waits for user interaction. Upon successful login (password entry followed by ENTER), it waits 5 seconds to ensure wallet data is updated, then captures the password.
    *   Deceptive `MessageBoxW` alerts are used for file-based wallets (e.g., Exodus), mimicking data corruption errors to prompt the user to open the wallet for "resynchronization," thereby triggering the monitoring.

4.  **Robust Data Archiving with Windows Shell API:**
    *   Wallet files (e.g., `.seco` files for Exodus, browser extension data for MetaMask) are securely archived into `.zip` files. This process exclusively leverages native `Windows Shell COM Objects` (`IShellDispatch`, `Folder`, `FolderItem`) for file zipping. This method avoids external libraries and in-memory compression techniques (like `miniz` previously used), ensuring a lower detection surface and utilizing trusted system components.
    *   Temporary `.zip` files are created in the system's temporary directory and are automatically deleted after data transmission.

5.  **Secure & Flexible Data Transmission:**
    *   Data exfiltration is handled via the native `WinHTTP API`, ensuring encrypted HTTPS communication without relying on external dependencies.
    *   The payload (captured password in a Discord spoiler, zipped wallet files) is sent as a `multipart/form-data` POST request to a predefined webhook endpoint.
    *   For larger browser wallet data, the tool implements a chunking mechanism: if the zipped data exceeds 8.5 MB, it is split into multiple smaller `.zip` files, each sent in a separate webhook message, to bypass Discord's attachment size limits.

6.  **Evasion & Obfuscation Techniques:**
    *   The codebase includes "distraction functions" that execute benign, computationally irrelevant tasks (e.g., trigonometric calculations, random number generation) to complicate static analysis and automatic signature generation by security software.
    *   Dynamic path construction and XOR encryption for sensitive strings further obscure hardcoded paths and make it harder for analysts to quickly identify target locations.

7.  **Clean Termination:**
    *   After all designated wallet data has been sent, the main application thread terminates gracefully, leaving background monitoring threads to complete any remaining tasks, ensuring minimal lingering presence.

---

> ### **Disclaimer**
> This tool is intended for educational and security research purposes only. It should only be used in controlled, authorized environments. Unauthorized use is strictly prohibited and against the law. The author is not responsible for any misuse of this software.
