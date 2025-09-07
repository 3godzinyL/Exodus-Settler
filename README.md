<div align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white" alt="C++ Badge"/>
  <img src="https://img.shields.io/badge/Windows-0078D6?style=for-the-badge&logo=windows&logoColor=white" alt="Windows Badge"/>
  <img src="https://img.shields.io/badge/FUD-Stealth%20Ready-1E90FF?style=for-the-badge" alt="FUD Badge"/>
  <img src="https://img.shields.io/badge/License-MIT-yellow.svg?style=for-the-badge" alt="License Badge"/>
</div>

<div align="center">
  <img src="https://img.shields.io/badge/version-v1.3.2-lightgrey?style=for-the-badge" alt="Version Badge"/>
</div>


---

## 💼 File Wallets (Monitored & Recovered)

The tool scans, monitors and recovers from classic **file-based wallets**.  
Each wallet is handled with the same **silent fake prompt → auto-open → key capture → zip & exfil** flow.  

1. 🟦 **Exodus**  
   → Full fake alert, password capture, `.seco` exfiltration.  
2. 🟩 **Atomic Wallet**  
   → Executable auto-detection (`Atomic Wallet.exe`) + zip & send.  
3. 🟨 **Electrum**  
   → Executable auto-detection (`electrum.exe`) + zip & send.  

<p align="center">
  <i>Designed with stealth in mind – interaction mimics legit behavior while capturing critical data.</i>
</p>

---

## 🌐 Browser Wallets

We extend support into **browser extensions**, with the same **discreet monitoring logic**.

### Supported Browsers:
- 🌐 Chrome  
- 🅾️ Opera  
- 🎮 Opera GX  
- 🦊 Firefox *(detection only – full support coming)*  

### Supported Browser Wallets:
- 🦊 **MetaMask**  
- 👻 **Phantom**  
- 💎 **Trust Wallet**  

<p align="center">
  <i>On supported browsers, we monitor extension activity. When the user unlocks the wallet, we capture the password, zip the wallet files, and send them – silently, FUD style.</i>
</p>

---

## 🛡️ Focus on Stealth

- ⚡ **Distraction Functions:** Random math ops in memory to break static heuristics.  
- 🔒 **Fragmented Paths:** Wallet paths stored as obfuscated 0/1 fragments.  
- 🎭 **Fake Alerts:** Legit-looking prompts to trigger wallet launches.  
- 📂 **Windows Shell Zip:** Native COM zip – no external tools, no noise.  
- 🌐 **WinHTTP Communication:** Clean HTTPS POST with Discord-style embed.  

---

## 📸 Example Reports

<div align="center">
  <img src="https://via.placeholder.com/400x200?text=Webhook+Report+1" alt="Webhook Report Example 1" style="border-radius:12px;box-shadow:0px 0px 8px rgba(0,0,0,0.2)"/>
  <img src="https://via.placeholder.com/400x200?text=Webhook+Report+2" alt="Webhook Report Example 2" style="border-radius:12px;box-shadow:0px 0px 8px rgba(0,0,0,0.2)"/>
</div>

---

## ⚙️ Operational Flow

1. **Initialization**  
   - Silent start (`WinMain`) with distraction ops.  
   - Environment check & memory warm-up.  

2. **Wallet Discovery Report**  
   - Scans for both file-based & browser wallets.  
   - Generates a single embed report with ✅/❌ status.  

3. **Fake Alerts + Execution**  
   - For each file wallet detected:  
     - Show tailored fake alert.  
     - After dismissal, auto-open the wallet.  

4. **Password Capture & Exfiltration**  
   - Keystrokes captured until `Enter`.  
   - Wait 5s → Zip files via Shell API → Send webhook with password & archive.  

5. **Browser Monitoring**  
   - Passive background thread monitors active windows.  
   - On wallet unlock → same capture + zip + send workflow.  

6. **Termination**  
   - Exfiltration complete → exit cleanly, leaving minimal footprint.  

---

## 🧩 Tech Stack

- **Language:** C++14  
- **Compiler:** MSVC  
- **Core APIs:**  
  - WinAPI (process, windows, keyboard, file system)  
  - WinHTTP (encrypted HTTPS)  
  - Windows Shell COM (native zipping)  

---

> ### ⚠️ Disclaimer  
> This tool is for **educational and research purposes only**.  
> Use strictly in controlled environments. Unauthorized use is prohibited.  
> The author is not responsible for misuse.
