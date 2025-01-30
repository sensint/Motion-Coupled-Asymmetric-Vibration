# Motion-Coupled Asymmetric Vibration

<div id="top"></div>



<!-- PROJECT SHIELDS -->
<!--
*** I'm using markdown "reference style" links for readability.
*** Reference links are enclosed in brackets [ ] instead of parentheses ( ).
*** See the bottom of this document for the declaration of the reference variables
*** for contributors-url, forks-url, etc. This is an optional, concise syntax you may use.
*** https://www.markdownguide.org/basic-syntax/#reference-style-links
-->
[![Contributors][contributors-shield]][contributors-url]
[![Forks][forks-shield]][forks-url]
[![Stargazers][stars-shield]][stars-url]
[![Issues][issues-shield]][issues-url]
[![MIT License][license-shield]][license-url]

<!-- PROJECT LOGO -->
<br />
<div align="center">
  <a href="https://sensint.mpi-inf.mpg.de/">
    <img src="assets/img/sensint_logo.png" alt="Logo" width="121" height="100">
  </a>

<h3 align="center">Motion-Coupled Asymmetric Vibration for Pseudo Force Rendering in Virtual Reality</h3>

  <p align="center">
    <b>We'd love to get your feedback and know if you want to explore this research further.</b>
    <br />
<!--     <br />
    <a href="https://github.com/sensint/Motion-Coupled-Asymmetric-Vibration/issues">Report Bug</a>
    ·
    <a href="https://github.com/sensint/Motion-Coupled-Asymmetric-Vibration/issues">Request Feature</a> -->
  </p>
</div>



## About The Project

![Banner images][banner-image]
In Virtual Reality (VR), rendering realistic forces is crucial for immersion, but traditional vibrotactile feedback fails to convey force sensations effectively. Studies of asymmetric vibrations that elicit pseudo forces show promise but are inherently tied to unwanted vibrations, reducing realism. Leveraging sensory attenuation to reduce the perceived intensity of self-generated vibrations during user movement, we present a novel algorithm that couples asymmetric vibrations with user motion, which mimics self-generated sensations. Our psychophysics study with 12 participants shows that motion-coupled asymmetric vibration attenuates the experience of vibration (equivalent to a \textasciitilde 30\% reduction in vibration-amplitude) while preserving the experience of force, compared to continuous asymmetric vibrations (state-of-the-art). We demonstrate the effectiveness of our approach in VR through three scenarios: shooting arrows, lifting weights, and simulating haptic magnets. Results revealed that participants preferred forces elicited by motion-coupled asymmetric vibration for tasks like shooting arrows and lifting weights. This research highlights the potential of motion-coupled asymmetric vibrations, offers new insights into sensory attenuation, and advances force rendering in VR.

<p align="right">(<a href="#top">back to top</a>)</p>



### Built With

* [Teensyduino](https://www.pjrc.com/teensy/teensyduino.html)

<p align="right">(<a href="#top">back to top</a>)</p>

## Getting Started

Download the code from the GitHub website or clone repo using your favorite git-client software or with the following command:

   ```sh
   git clone https://github.com/sensint/Motion-Coupled-Asymmetric-Vibration.git
   ```

##### Experiment 1
+ The processing code: CHI_25_Exp1.pde (processing/CHI_25_Exp1) can be run by selecting the appropriate serial port.
+ Further, the sequence of (stimuli)(intensity) needs to be provided to have the code run in sequence.
+ For the CHI_25_Exp1.ino (Firmware/ Teensy/ CHI_25_Exp1) code, the following letters enter through the serial monitor will play the corresponding stimuli at the corresponding amplitude: For instance, a1 will play the stimuli a with 70% of max amplitude. The stimuli are as follows:
	- a: motion-coupled asymmetric vibration
	- b: motion-decoupled asymmetric vibration
	- c: continuous asymmetric vibration (user moves)
	- d: continuous asymmetric vibration (user stationary)

  The Intensity_Level refers to the three levels of intensity:
	- 0: amplitude at 40% the maximum level
	- 1: amplitude at 70% the maximum level
	- 2: amplitude at 100% the maximum level

+ Each combination of a character and number would be played for 15 seconds by default.
+ Playing condition 'a' is a prerequisite for playing the conditions 'b', 'c', 'd'

##### Experiment 2
+ Open the Exp2_Unity directory in Unity version 2022.3.34f1 or higher.
+ You should be able to see the different scenes (Bow-Arrow, Weights, Magnets). In each scene, you can choose the mapping type, vibration type.
+ Key Press 'L' should toggle the visual On and Off.
+ Upload the CHI_25_Exp2.ino (Firmware/Teensy/CHI_25_Exp2) code to the teensy 4.1 microcontroller.
+ Changing things in Unity would reflect on the actuator once all the physical connections are made.

### Firmware

We provide the firmware for one build system - Teensyduino (based on Arduino IDE).

The firmware was tested with the following microcontrollers:

- Teensy 4.1



#### Teensyduino

The easiest way to get up and running with the firmware is Teensyduino.

##### Prerequisites

Download and install the [Teensyduino](https://www.pjrc.com/teensy/td_download.html) software for your operating system. All needed libraries are included in the basic installation.


### Hardware

This project is based on the Teensy microcontroller and the [PT8211 Audio Kit](https://www.pjrc.com/store/pt8211_kit.html). For understanding the hardware and the underlying principles, please refer to [Haptic Servos](https://dl.acm.org/doi/full/10.1145/3544548.3580716). The amplifier used for rendering pseudo forces was [Visaton 2.2](https://www.visaton.de/en/products/accessories/amplifiers/amp-22)

<p align="right">(<a href="#top">back to top</a>)</p>


## Contributing

Contributions are what make the open source community such an amazing place to learn, inspire, and create. Any contributions you make are **greatly appreciated**.

If you have a suggestion that would make this better, please fork the repo and create a pull request. You can also simply open an issue with the tag "enhancement".
Don't forget to give the project a star! Thanks again!

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

<p align="right">(<a href="#top">back to top</a>)</p>


## License

Distributed under the MIT License. See `LICENSE.txt` for more information.

<p align="right">(<a href="#top">back to top</a>)</p>



## Contact

Sensorimotor Interaction Group - [website](https://sensint.mpi-inf.mpg.de/) - [@sensintgroup](https://twitter.com/sensintgroup)

Project Link: [https://github.com/sensint/HapticGasPedal](https://github.com/sensint/HapticGasPedal)

<p align="right">(<a href="#top">back to top</a>)</p>





## Acknowledgments

* Othneil Drew for [Best-README-Template](https://github.com/othneildrew/Best-README-Template)

<p align="right">(<a href="#top">back to top</a>)</p>






<!-- MARKDOWN LINKS & IMAGES -->
<!-- https://www.markdownguide.org/basic-syntax/#reference-style-links -->
[contributors-shield]: https://img.shields.io/github/contributors/sensint/Motion-Coupled-Asymmetric-Vibration.svg?style=for-the-badge
[contributors-url]: https://github.com/sensint/Motion-Coupled-Asymmetric-Vibration/graphs/contributors
[forks-shield]: https://img.shields.io/github/forks/sensint/Motion-Coupled-Asymmetric-Vibration.svg?style=for-the-badge
[forks-url]: https://github.com/sensint/Motion-Coupled-Asymmetric-Vibration/network/members
[stars-shield]: https://img.shields.io/github/stars/sensint/Motion-Coupled-Asymmetric-Vibration.svg?style=for-the-badge
[stars-url]: https://github.com/sensint/Motion-Coupled-Asymmetric-Vibration/stargazers
[issues-shield]: https://img.shields.io/github/issues/sensint/Motion-Coupled-Asymmetric-Vibration.svg?style=for-the-badge
[issues-url]: https://github.com/sensint/Motion-Coupled-Asymmetric-Vibration/issues
[license-shield]: https://img.shields.io/github/license/sensint/Motion-Coupled-Asymmetric-Vibration.svg?style=for-the-badge
[license-url]: https://github.com/sensint/Motion-Coupled-Asymmetric-Vibration/blob/master/LICENSE
[banner-image]: assets/img/Banner_MCAV.png
