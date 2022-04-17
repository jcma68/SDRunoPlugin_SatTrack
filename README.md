# SDRunoPlugin_SatTrack
Satellite tracker plugin for SDR uno (Requires SDRuno 1.40.2 or above).

## Main display
![image](https://user-images.githubusercontent.com/102866095/163732012-c691846a-0de0-42c1-92db-9cbfa61ae528.png)

## Settings
![image](https://user-images.githubusercontent.com/102866095/163732040-b100f153-4cc4-4db4-80f9-d8631a024fa0.png)

//TODO:


## Passes prediction
![image](https://user-images.githubusercontent.com/102866095/163732063-38e2b966-22ca-485f-98c9-8d693aaa566e.png)

//TODO:


## Installation

//TODO:


## Compiling from source:
- Open the Visual Studio 2019 solution. Make sure that the x86 Release configuration is selected. Do not use the Debug version, this one crashes SDRuno.
- The sdruno_kit\include folder contains all the include files provided by SDRPlay (see: https://github.com/SDRplay/plugins for more informations).
- The sdruno_kit\nana\build\bin folder contains two zip files of a prebuilt version of the Nana library with its extensions (see: https://github.com/cnjinhao/nana for more informations). These files need to be unpacked in the same folder before compilation.
