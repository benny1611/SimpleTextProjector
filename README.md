# SimpleTextProjector

- [Design goals](#design-goals)
- [Platforms](#platforms)
- [Installation and Contribution](#installantion-and-contribution)
- [Documentation](#documentation)
- [Example](#example)
- [Used third-party tools/libraries](#used-third-party-tools/libraries)
- [License](#license)

## Design goals

There are lots of programs to project text on screen out there, but none offered the possibility to control the monitor remotely (at least none that I could find). This program lets you show text on the screen of a computer via WebSockets. The program gets a JSON object with information about the text, and the monitor and it just shows the desired text on the desired monitor. I had this project idea for church, when I had to show verses and lyrics while doing a live streaming. This project gives you the posibility to share the work with someone else as they can change the lyrics/verses via phone. 

**ATTENTION**: This project is only the backend, meaning it has no user interface, it just shows the text on the screen. I am currently working on a web intreface for this project. 

The goal of this project is to have a program that can show text on screen and be able to take commands from a WebSocket, also it is desired to have a streaming server to stream the text for a streaming software (like OBS).

## Platforms

Currently SimpleTextProjector only works on Windows (tested on Windows 10), but other platforms are on the way (specifically MacOS and Linux).

## Installation and Contribution

To install this project you need to download the release and double click on the .exe file. 

Any contribution is welcome. I've made this project as easy to build as it can get, you only have to open the project in Visual Studio (2022). Steps you have to follow:
1. Download and install Visual Studio Comunity Version (2022) from the Microsoft webpage
2. In the installer select "Desktop development with C++" then click install
3. Now you should be able to build and run the project.

## Documentation

This program listens for a WebSocket connection on port 80, and takes a JSON object with the commands.

Here are all the possible commands so far:

- ```text``` - ```string``` text that should be shown on the screen encoded with ```base64```, empty text for blank screen.
- ```monitor``` - ```int``` the index of the monitor, starting with 0 (1 for secondary monitor, 2 for the third and so on).
- ```font_size``` - ```int``` size in pixels of the font.
- ```font``` - ```string``` name of the ```.ttf``` file that must be present in the ```fonts``` folder.
- ```font_color``` - ```int array``` of size 3, with the first element representing the color RED (between 0.0 and 1.0), the second element representing the color GREEN (between 0.0 and 1.0), the third representing BLUE (between 0.0 and 1.0). You can use a combination of those to achieve your desired color (RGB).
- ```stream``` boolean value start or stop streaming
- ```get``` get different values from the server, possible values so far:
  - ```stream``` returns if the server is streaming or not or and if it's streaming, then it returns the offer for the WebRTC client. Example: ```{"isStreaming": true, "offer": {....}}```
  - ```ping``` returns ```{"pong": true}``` just to keep the WebSocket connection alive

## Example

Here's a JSON object example:

```
{
  "text": "SGVsbG8gV29ybGQ=",
  "monitor": 0,
  "font_size": 35,
  "font": "JandaAppleCobbler",
  "font_color": [
    1,
    0,
    0
  ]
}
```

## Used third-party tools/libraries

This software uses libraries from the FFmpeg project under the LGPLv2.1. I do *NOT* own FFmpeg!


- [freetype](https://freetype.org/)
- [glad](https://glad.dav1d.de/)
- [glfw](https://www.glfw.org/)
- [poco](https://pocoproject.org/)
- [FFmpeg](https://www.ffmpeg.org)
- [libwebrtc](https://github.com/webrtc-sdk/libwebrtc)

## License

See the ```LICENSE``` file