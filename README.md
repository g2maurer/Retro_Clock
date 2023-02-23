# Retro_Clock
This is a highly modified version of TechKiwiGadgets clock found on 

1. Instructables     https://www.instructables.com/ESP32-Pacman-Clock/
2. Makerbot Thingiverse (ESP32 Pacman Clock)    https://www.thingiverse.com/thing:5136672

Modifications:
1. Changed TFT display to an SPI version which has a much improved Touch interface.
2. Added SPIFFS to support files for data and images. This freed up memory and added flexability.
3. Added support for optional RTC (real time clock IC). Improves time keeping accuracy.
4. Added support for LDR (light dependent resistor) so that display is dimmed at night.
5. Added audio amplifier (LM386) and volume control via touch screen.
6. Redesigned user interface (Touch menu) to support added features.
7. Made many changes to firmware to improve speed and readability. 
8. Designed PCB to support above mentioned changes.
9. Redesigned 3D printed parts to support hardware changes.

TFT_eSPI library:
1. The TFT_eSPI library requires it be changed to support each particular build (unusal but that's way way it works).
2. The file User_Setup_Select.h specifies which setup file in the User_Setups directory is to be used.
3. Added to the User_Setups directory is the file Setup_Pacman_Clock.h which contains the specifies the configuration and features (Touch) used by this project.
4. Move the file "Setup_Pacman_Clock.h" into directory   ...Arduino/libraries/TFT_eSPI/User_Setups
5. Move the file "User_Setup_Select.h" into the directory    ...Arduino/libraries/TFT_eSPI   (replaces the existing User_Setup_Select.h file). 

Tools:
1. Arduino IDE 1.8.19, Board: DOIT ESP32 DEVKIT V1
2. Initialize SPIFFS using Arduino IDE  "Tools/ESP32 Sketch Data Upload" (which will initializee SPIFFS and upload all the files from the Data directory).
3. Creality Slicer, PLA, 0.2mm, no supports.
