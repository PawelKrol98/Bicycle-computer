# Bicycle computer README
This project was made by Michał Bogoń and Paweł Król and was final project to Microprocessor programming study subject. <br />
It presents fully functional bicycle computer with basic functionality (below).

# Functionality
* Timer
* Clock
* Speedometer
* Distance measurer

# Components
* Arduino UNO
* LCD screen 2x16
* Potentiometer 5k Ohm
* Reed switch & magnet
* Tact switches
* Buzzer

# How does the computer work
LCD screen shows our actual speed and travelled distance (depending on working mode). <br />
Whenever magnet is close to reed switch, it ignites high logic state, which starts the interruption. This way we calculate travelled distance and speed, knowing wheel radius. <br />
We change screen brightness using potentiometer. <br />
Travelled distance can be reset by using tact switch. <br />
Buzzer is used to signal computer startup and travelled distance reset. <br />
Using tact switches, we can change computer operating mode. <br />


# Port connections
![obraz](https://user-images.githubusercontent.com/48327929/111916078-88755980-8a79-11eb-90f4-4b8d86091182.png)


