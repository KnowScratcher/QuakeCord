# QuakeCord
<p align="center">
Whenever an earthquake strikes, you should know what's going on.
</div>
<p align="center">
    <img src="https://raw.githubusercontent.com/KnowScratcher/QuakeCord/refs/heads/main/img/20250531new_model.png" height="400">
</div>

# BOM
Because I live in Taiwan, so I'll add a TWD column.

| Item                      | Amount | Price(USD) | Total(USD) | Total(TWD) |
| ------------------------- | ------ | ---------- | ---------- | ---------- |
| ESP32 dev module          | 1      | 5.34       | 5.34       | 160        |
| MPU6050 (soldered)        | 1      | 4.33       | 4.33       | 130        |
| Case (3D print)           | 1      | 0.91       | 0.91       | 27         |
| Jump wire (pin - socket)  | 5      | 0.19       | 0.19       | 6          |
| Breadboard mini (45x35mm) | 1      | 0.17       | 0.17       | 5          |
| Total                     |        |            | 10.75      | 328        |

> [!NOTE] 
> - If you'd like to find a case for the project, go ahead.
> - A 3D printed case will be a cheaper option. (Assuming PLA costs $13.4/KG)

# Journal
The journal is in [JOURNAL.md](JOURNAL.md)

A brief summery of the journal:

## How it all started
I've been wanting to make something useful and about earthquake since I lived in a place with lots of earthquake. Just this simple reason.

## What will it do?
- detect earthquake
- record the acceleration data for further research
- send notification to discord
<img src="https://raw.githubusercontent.com/KnowScratcher/QuakeCord/refs/heads/main/img/20250525structure.png" height="400">

# How to make one
1. Head over to [src > print](https://github.com/KnowScratcher/QuakeCord/tree/main/src/print) and download the file you need.

   `QuakeCord_case.3mf`: the whole case including base and cap. (if you choose this, you don't have to download the others.)
   
   `QuakeCord_case_case.3mf`: the base of the case.
   
   `QuakeCord_case_cap.3mf`: the cap of the case.

   Also stl file is provided.

   `QuakeCord_case.stl`: the whole case including base and cap. (if you choose this, you don't have to download the others.)
   
   `QuakeCord_case_case.stl`: the base of the case.
   
   `QuakeCord_case_cap.stl`: the cap of the case.
   
2. Assemble the parts as follows:
   Detail in [wiki](https://github.com/KnowScratcher/QuakeCord/wiki).
   1. glue or foam tape the breadboard onto the case. If 3d printed, there should be an area marked the same size as the breadboard.
   2. plug your MPU6050 onto the breadboard. Note that the pins should align vertically, that is, along the direction which has more holes.
      <img src="https://raw.githubusercontent.com/KnowScratcher/QuakeCord/refs/heads/main/img/20250531connected.png" alt="what is connected on a breadboard" height="200">
      
      (breadboard image from [arduino store](https://store.arduino.cc/products/mini-breadboard-white))
      
   3. connect the jump wires from ESP32 to the breadboard as follows:
      | ESP32 pin | MPU6050 pin (breadboard) |
      | --------- | ------------------------ |
      | 3v3       | VCC                      |
      | GND       | GND                      |
      | D22       | SCL                      |
      | D21       | SDA                      |
      | D15       | INT                      |
   4. find the script at [not available](about:blank) and edit the setting. (Detail in [wiki](https://github.com/KnowScratcher/QuakeCord/wiki).)
   5. upload the script to your ESP32.
   6. Plug the device somewhere you won't be often and you're good!!!
  
# Software
CAD: PowerPoint, FreeCAD

Programming: VS code, Arduino IDE

Image editing: Paint 3D

# Contribute
## Files
### Step Files
step files are available at [src > step](https://github.com/KnowScratcher/QuakeCord/tree/main/src/step)

`QuakeCord_case.step`: All parts together.

`QuakeCord_case_case.step`: only the base.

`QuakeCord_case_cap.step`: only the cap.

Please update all three CADs to your design.
### FreeCAD File
find the `.FCStd` at [src](https://github.com/KnowScratcher/QuakeCord/tree/main/src)

`QuakeCord.FCStd`: the model.

`QuakeCord_case.FCStd`: the actual case. (both base and cap)

## PR
Please describe what exactly you have changed, I will review the PRs once in a while, be patient!
