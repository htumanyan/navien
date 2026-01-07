# Legal Notices

**READ THIS FIRST.** This custom controller (“Device”) is an aftermarket, third-party product that is **not manufactured, endorsed, certified, or supported by Navien**. Use of this Device may affect equipment operation, safety protections, warranty coverage, and regulatory compliance.

By installing, powering, or using the Device, you acknowledge and agree that:

- **Use is entirely at your own risk.**
- The Device is provided **“AS IS” and “AS AVAILABLE,”** without warranties of any kind, express or implied, including (without limitation) implied warranties of merchantability, fitness for a particular purpose, non-infringement, or that the Device will be error-free or uninterrupted.
- **No liability is assumed** by the developer, designer, seller, or distributor of the Device for any direct, indirect, incidental, special, consequential, exemplary, or punitive damages, including (without limitation) property damage, personal injury, loss of hot water, loss of heating, loss of data, loss of profits, or business interruption, arising out of or related to installation or use of the Device—even if advised of the possibility of such damages.
- You are solely responsible for **safe installation**, verifying compatibility, following all applicable **electrical codes**, and complying with all local laws and regulations.
- If you do not accept these terms, **do not install or use** the Device.

---

# User Manual — Navien Water Heater Controller (Custom Hardware)

## Table of Contents
1. Precautions  
2. Connections  
3. Powering the Unit  
4. LED Indicators  
5. Wi-Fi Connection  
6. Troubleshooting  

---

## Precautions

- **Never connect or disconnect the controller when the Navien unit is powered on.**  
  Always turn off the Navien unit and **unplug the power cord from the receptacle** before making or changing any connections.

- Before making any connections, ensure that the **power is turned off to all units** involved (Navien unit, any external power supply, and any nearby control equipment).

- Handle cables gently. Avoid bending, pinching, or pulling wires under tension.

---

## Connections

Your controller has **two connectors**:

### A) Micro-USB connector (side)
Used for:
- **Firmware upgrades**
- **Powering the controller**, if your Navien unit does not provide power (see **Powering the Unit**)

### B) 6-pin connector (Navien cable)
This connector receives the cable that connects the controller to the Navien unit.

Steps:
1. Unpackage the controller and cable.
2. Plug the cable into the controller’s **6-pin jack**.
3. The connector is **keyed** and fits **only one orientation**.
4. **Do not force it.** If insertion feels wrong, remove it, flip the connector orientation, and try again.

---

## Powering the Unit

### Option 1 (Universal): USB 5V power
The simplest way to power the unit is using a **standard 5V USB charger**, such as a typical phone power brick.

Steps:
1. Connect Micro-USB to the controller.
2. Plug the charger into AC power.
3. Confirm the controller powers on (see **LED Indicators**).

### Option 2: Power from the Navien unit (Navien 240 models)
**Your Navien 240 cable kit includes an additional small 5-pin connector with only one red wire.** This connector is used to draw **+5V power** from Navien 240 models (including **240A and 240A2**) and power the controller without an external USB charger.

Navien 240 models provide +5V power on the **second 5-pin jack on the front panel** (as shown in the photo). The controller can be powered from that jack using the single-red-wire connector.

To power the controller from a Navien 240 model:
1. Turn the Navien unit **OFF** and **unplug** it from the wall.
2. Locate the **second 5-pin jack** on the front panel.
3. Insert the **single-red-wire 5-pin connector** into that jack.  
   **Important:** this jack is **not keyed**. The connector must be inserted with the **red wire positioned at the bottom** (as shown in the photo).
4. Reconnect the main controller cable as usual.
5. Plug the Navien unit back in and power it on.


<div style="display: flex; gap: 24px; align-items: flex-start;">
   <div style="text-align: center; background: #fff; padding: 8px; border-radius: 6px;">
      <img src="240a_panel.png" alt="Navien 240A front panel" width="300"/><br/>
      <b>Navien 240A front panel</b>
   </div>
   <div style="width:2px; background:#ccc; height: 240px; margin: 0 12px;"></div>
   <div style="text-align: center;">
      <img src="240a2_panel.png" alt="Navien 240A2 front panel" width="300"/><br/>
      <b>Navien 240A2 front panel</b>
   </div>
</div>

---

## LED Indicators

There are **two LED indicators**:

### Left LED (Navien connectivity)
This LED can flash **green** or **red** and indicates connectivity to the Navien unit:
- **Green (flashing):** Connected; sending/receiving data  
- **Red (flashing):** Disconnected

✅ The left LED **must be green** for normal controller operation.

### Right LED (Wi-Fi status)
- **Solid Blue:** Connected to Wi-Fi  
- **Blinking Blue:** Attempting to connect to Wi-Fi  
- **Solid Purple:** Failed to connect to Wi-Fi and switched to **setup mode**

---

## Wi-Fi Connection

1. Power on the controller.  
   The **right LED (Wi-Fi)** will begin **blinking blue**.

2. Wait several minutes.  
   If the controller cannot connect to a previously saved Wi-Fi network, it will switch to setup mode and the LED will turn **solid purple**.

3. On your phone or laptop, connect to the Wi-Fi network:
   - **SSID:** `navien_wifi`  
   - **Password:** `navien_wifi`

4. After connecting, a browser popup should appear prompting you to:
   - Select your home Wi-Fi network
   - Enter the Wi-Fi password

5. After credentials are submitted:
   - The unit will **restart automatically**
   - The right LED should change to **solid blue** once connected to your Wi-Fi

---

## Troubleshooting

| Problem | Solution |
|---|---|
| No browser popup after connecting to `navien_wifi` | “Forget” the `navien_wifi` network on your phone/tablet/laptop and connect again. |
| Purple light does not switch to blue after network credentials are entered | Unplug the device, wait a few seconds, and plug it back in. |

Additional practical checks:
- Ensure your home Wi-Fi is **2.4 GHz** if your router separates 2.4/5 GHz networks (many embedded Wi-Fi devices require 2.4 GHz).
- Keep the controller close to the router during initial setup.
