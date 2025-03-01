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
- ```font_color``` - ```JSON Object``` This object must define values (between 0.0 and 1.0) for each colors: R (RED), G (GREEN), B(BLUE) and A(ALPHA). Example: ```{"font_color": {"R": 1.0, "G": 0.0, "B": 0.0, "A": 1.0 } }```
- ```background_color``` - ```JSON Object``` This object must define values (between 0.0 and 1.0) for each colors: R (RED), G (GREEN), B(BLUE) and A(ALPHA). Example: ```{"background_color": {"R": 1.0, "G": 0.0, "B": 0.0, "A": 1.0 } }```
- ```stream``` boolean value start or stop streaming
- ```get``` get different values from the server, possible values so far:
  - ```stream``` returns if the server is streaming or not or and if it's streaming, then it returns the offer for the WebRTC client. Example: ```{"isStreaming": true, "offer": {....}}```
  - ```ping``` returns ```{"pong": true}``` just to keep the WebSocket connection alive
  - ```monitors``` returns a JSON array with the IDs of the monitors and their names (names are not guaranteed to be unique). Example output: ```[{"0":"Generic PnP Monitor 1920 x 1080 60hz"},{"1":"Generic PnP Monitor 2560 x 1440 59hz"},{"2":"Generic PnP Monitor 1920 x 1080 60hz"}]```
- ```set``` set different values for this WebRTC connection - usually used to set the offer. Possible values so far:
  - ```answer``` - sets the answer for the RTC connection. When ```"set": "answer"``` is present, the ```answer``` key must also be present. Example:
  ```
  {
    "set": "answer",
	"answer": {
		"sdp": "v=0\r\no=mozilla...THIS_IS_SDPARTA ... ",
		"type": "answer"
	}
  }
  ```

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
- [libdatachannel](https://github.com/paullouisageneau/libdatachannel)
- [utfcpp](https://github.com/nemtrif/utfcpp)
- [glm](https://github.com/g-truc/glm)

## License

See the ```LICENSE``` file