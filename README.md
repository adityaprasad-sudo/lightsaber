# LightSaber

Pretty much everyone knows what a light saber is Well i am here to make it huhuhaha and share how to make it for #pixl.


# Code

The code is pretty simple and can be found inside the code folder in the GitHub repo you might want to calibrate your sensor(MPU_6050) to match your likings (Calibration is Important).

You can also adjust the thresholds required for a specific action to play. For ex: you can increase the clash threshold to active sound_clash on higher level of impact.

You can also change the color of clash and light saber and add as many modes as you want(as long as your esp32 has storage to afford it) and add sounds(how to add sounds? watch below!)

## How to add custom sounds

1. first find a light saber sounds (light_saber_on, light_saber_strike etc.) not longer then 0.5s to 1sec then convert them to wav format(If not in wav format).
2. upload them on audacity and reduce their band hertz to 11250hz and 8 bit audio and export them.
   <img width="1365" height="719" alt="lightsaber" src="https://github.com/user-attachments/assets/3ab52c43-74d2-467c-b6cc-9d8765c865c5" />

3. After that we need to convert them to c++ source files to do that i used a software called HxD to convert wav format to c++ source file.
  <img width="1043" height="612" alt="lightsaber2" src="https://github.com/user-attachments/assets/13a19fee-e95e-4e08-a199-afc2e1d04ed3" />

4. After that you will get a c++ source file copy its content only the no. of entires and the data.
5. Paste all those into the SoundData array available in the code folder of the repo(Remember to use correct variable names)

# Circuit diagram

Here is the full circuit diagram

<img width="3120" height="4208" alt="IMG_20260930_211355991_HDR_AE" src="https://github.com/user-attachments/assets/6c6df28f-a361-447d-bb40-f2859ed4d382" />

## how did you get an amplifier ?

I had a old Bluetooth Speaker with an XF-BT90-V3.0 chipset in it in which the bluetooth ic handles the wheater the amplifier would work or not.
The photo attached contains two rectangles

1. **the blue rectangle represents the amplifier ic**
2.  **the pink box represents Bluetooth and audio controller** 
3.   **the orange line is the trace of the audio output from the audio controller to the amplifier** 
4. **The orange arrow represents gnd connection and red arrow represents an mute signal** (*Both are shorted to prevent the amplifier from being in mute mode*)
5. **at the end of the orange trace i connected an blue wire which goes to the dac output of the esp32**

***Note:-** I am using this board also as a charging board since the battery connects to this board and can charge the battery.*

## Parts list
1. ESP32 dev module
2. WS128B leds
3. Amplifier board(Dont have one i told a workaround below)
4. speaker
5. wires
6. MPU 6050
7. power switch
8. Push button
9. Voltage regulator (LM1117t 3.3v)
## 3D files

I made them in free cad to fit all the componets properly and compactly so nothing moves inside when we swing and clash the lightsaber!!!

# Thank you

Thank for reading till here, all the required files could be found in the github repo
