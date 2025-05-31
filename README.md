# QuakeCord
<p align="center">
Whenever an earthquake strikes, you should know what's going on.
</div>
<p align="center">
    <img src="https://raw.githubusercontent.com/KnowScratcher/QuakeCord/refs/heads/main/img/20250531model.png" height="400">
</div>

# BOM
| Item                      | Amount | Price(USD) | Total(USD) |
| ------------------------- | ------ | ---------- | ---------- |
| ESP32 dev module          | 1      | 5.34       | 5.34       |
| MPU6050 (soldered)        | 1      | 10.02      | 10.02      |
| Case (3D print)           | 1      | 0.91       | 0.91       |
| Jump wire (pin - socket)  | 5      | 0.19       | 0.19       |
| Breadboard mini (45x35mm) | 1      | 0.17       | 0.17       |
| Total                     |        |            | 16.63      |

> [!NOTE] 
> - If you'd like to find a case for the project, go ahead.
> - A 3D printed case will be a cheaper option. (Assuming PLA costs $13.4/KG)

# Journal
The journal is in [JOURNAL.md](JOURNAL.md)

# How to make one
1. Head over to [src > print](https://github.com/KnowScratcher/QuakeCord/tree/main/src/print) and download the file you need.

   `QuakeCord_case.3mf`: the whole case including base and cap. (if you choose this, you don't have to download the others.)
   
   `QuakeCord_case_case.3mf`: the base of the case.
   
   `QuakeCord_case_cap.3mf`: the cap of the case.
   
4. Assemble the parts as follows:
   1. glue or foam tape the breadboard onto the case. If 3d printed, there should be an area marked the same size as the breadboard.
   2. plug your MPU6050 onto the breadboard. Note that the pins should align vertically, that is, along the direction which has more holes.
      <img src="https://raw.githubusercontent.com/KnowScratcher/QuakeCord/refs/heads/main/img/20250531connected.png" alt="what is connected on a breadboard" height="200">
      
      (breadboard image from [arduino store](https://store.arduino.cc/products/mini-breadboard-white))
      
   4. connect the jump wires from ESP32 to the breadboard as follows:
      | ESP32 pin | MPU6050 pin (breadboard) |
      | --------- | ------------------------ |
      | 3v3       | VCC                      |
      | GND       | GND                      |
      | D22       | SCL                      |
      | D21       | SDA                      |
      | D15       | INT                      |
   5. find the script at [not available](about:blank) and upload it to your ESP32.
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
