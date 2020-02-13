# miniAirQ

The miniAirQ project was created for the Neumann Janos Infromatics Competition in Hungary. More information can be found here http://eldanor.hu/miniairq

The aim of the project is to create a cheap and easy indoor air quality sensor and share the measured data on the website. You can get some STEAM knowledge if you build this project. For the hardware part you need an NodeMCU ESP32 development board, a CCS811 air quality sensor, some jumper cables and a battery (for eg. LiFePo4 18650). You need the Arduino IDE to flash the code the the ESP32.

The web backend receives data from air quality sensor which was registered on the project website.

### Prerequisites
 - The Arduino IDE installed on your computer
   If not, refer to "Install the Arduino Desktop IDE" on the
   [Arduino site](https://www.arduino.cc/en/Guide/HomePage).
 - The Arduino library directory is at its default location.
    `C:\Users\your_user_name\Documents\Arduino\libraries`.

### Installation
Installation steps
 - First, visit the [CCS811 project page](https://github.com/maarten-pennings/CCS811) for the Arduino CCS811 library.
 - Click the green button `Clone or download` on the right side.
 - From the pop-up choose `Download ZIP`.
 - In Arduino IDE, select Sketch > Include Library > Manage Libraries ...
   and browse to the just downloaded ZIP file.
 - When the IDE is ready this `README.md` should be located at e.g.
   `C:\Users\your_user_name\Documents\Arduino\libraries\CCS811\README.md`. 
 - After the CCS811 library installation download the air_quality_CCS811.ino file to your computer.
 - Open it with the Arduino IDE
 - Compile and upload it to your ESP32 board
