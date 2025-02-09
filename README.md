# running-lights

This is a running lights with a clickable button implementation on Raspberry Pi Pico W. The lights are running on/off sequentially and their direction is changed upon a button press. The button triggers an interrupt on the falling edge to change the direction when the LED is on for longer periods of time. Otherwise the button click would not be registered at the moment it is pressed, i.e., only at the moment of LED switching on/off. Moreover, in order to debounce the button via software, an alarm has been added, that turns off the button interrupts for a specific period of time, which allows to have more control over how many times the button interrupt is triggered. Additionally, UART communication is enabled on the default RX/TX pins, where sending a `+` increases the LED speed and sending a `-` decreases it. The maximum and minimum times LED is on are defined separately and can be adjusted accordingly.
An implementation example based on a basic button read without interrupts (simple register read) is present in the hw-registers branch.


## Prerequisites
In order to be able to run the project, [The Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk) must be present. There are two ways to satisfy this requirement:
* Install [The SDK](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf) locally (Appendix C)
* Use the provided Dockerfile

While for local development the former method was used, for ease of setup it is recommended to use the custom Docker image.
Please note that although it is also recommended to install the Picotool separately, this is not mandatory and the build system downloads it automatically if Picotool is not present.
### Setting up the environment with Docker
The Docker image build should take a couple of minutes, start the build within the cloned project directory:
```bash
docker build . -t pico-sdk
```
Within the same cloned project directory, run the container:
```bash
docker run -it --name pico-running-lights -v ${PWD}:/home/ubuntu/running-lights:rw --user $(id -u):$(id -g) pico-sdk
```

This will bring you inside the container at `/home/ubuntu` location, to get to the running lights project run:
```bash
cd running-lights
```

## Building the project
The project can be built in Release (default) mode or Debug mode. The difference between the two is additional debug information added in the Debug build, as well as a `DEBUG` compiler definition added that allows to receive additional logs via UART.\
The build result that should be used is an .uf2 file located within the build directory. If the project is being built within the Docker container, the output files should appear on the host side in the mounted directory.
### Release mode

To build the project in Release mode, in the project root folder:
```bash
mkdir build
cd build
cmake ..
make
```

### Debug mode

To build the project in Debug mode, in the project root folder:
```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
```


## Running the project
Unplug the RPi Pico from the computer, press and hold the BOOTSEL button and plug the RPi Pico in once again, it should appear as a mass storage device. Drag and drop the .uf2 file onto the device, the Pico will reset and run the project.