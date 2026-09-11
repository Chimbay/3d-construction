<!-- Improved compatibility of back to top link: See: https://github.com/othneildrew/Best-README-Template/pull/73 -->

<a id="readme-top"></a>

<!-- PROJECT SHIELDS -->

[![MIT License][license-shield]][license-url]
[![LinkedIn][linkedin-shield]][linkedin-url]

<!-- PROJECT LOGO -->

<br />
<div align="center">
  <h3 align="center">3D Gaussian Splatting Renderer</h3>

  <p align="center">
    A Vulkan-based renderer for visualizing trained 3D Gaussian Splatting models
    <br />
  </p>
</div>

<!-- TABLE OF CONTENTS -->

<details>
  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#about-the-project">About The Project</a>
      <ul>
        <li><a href="#built-with">Built With</a></li>
      </ul>
    </li>
    <li>
      <a href="#getting-started">Getting Started</a>
      <ul>
        <li><a href="#prerequisites">Prerequisites</a></li>
        <li><a href="#building">Building</a></li>
      </ul>
    </li>
    <li><a href="#usage">Usage</a></li>
    <li><a href="#roadmap">Roadmap</a></li>
    <li><a href="#license">License</a></li>
    <li><a href="#acknowledgments">Acknowledgments</a></li>
  </ol>
</details>

<!-- ABOUT THE PROJECT -->

## About The Project

This project focused on exploring **3D Gaussian Splatting (3DGS)** and **4D Gaussian Splatting (4DGS)**, with an emphasis on Gaussian-based scene representation, optimization, training, and novel view synthesis.

### Key Features

* **3D Gaussian Splatting**: Optimized Gaussian-based scene representations
* **Vulkan Renderer**: Custom renderer for loading and visualizing trained Gaussian models
* **PLY Loading**: Loads trained 3DGS models from binary PLY files
* **Spherical Harmonics**: Extracts and converts Gaussian color information to RGB
* **GPU Compute**: Uses Vulkan compute shaders for Gaussian projection
* **Interactive Viewer**: Orbit camera with rotation, movement, and zoom
* **ImGui**: Drag-and-drop model loading

<p align="right">(<a href="#readme-top">back to top</a>)</p>

## Motivation

The motivation for this project came from the challenge of reconstructing a 3D scene from multiple sources of footage. Each camera angle captures only a fragment of a scene, raising the question of whether these views can be combined into a single 3D reconstruction that can be explored from different viewpoints.

The project therefore focused on understanding **3D Gaussian Splatting**, its optimization process, and the development of a renderer for viewing trained Gaussian models from novel viewpoints.

### Built With

[![Vulkan][vulkan-shield]][vulkan-url]
[![PyTorch][pytorch-shield]][pytorch-url]
[![Python][python-shield]][python-url]
[![C++][cpp-shield]][cpp-url]

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- GETTING STARTED -->

## Getting Started

### Prerequisites

* **CMake 3.20+**
* **C++20 compiler**
* **Vulkan SDK**
* **Python**
* **PyTorch**
* A GPU with Vulkan support

### Building

#### Windows

```bash
cmake -B build -G "MinGW Makefiles"
cmake --build build
```

Run the renderer:

```bash
./build/renderer.exe
```

#### macOS

```bash
cmake -B build -DGIT_EXECUTABLE=/usr/bin/git
cmake --build build
```

Run the renderer:

```bash
./build/program
```

> **Note:** The macOS configuration uses the system Git executable to avoid conflicts with the Homebrew `libcurl` installation.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- USAGE EXAMPLES -->

## Usage

The project consists of a training pipeline and a Vulkan-based rendering application.

The training pipeline uses **PyTorch** to optimize the Gaussian representation of a scene. The resulting model can then be loaded by the Vulkan renderer for interactive visualization.

The renderer supports:

* Loading trained 3D Gaussian Splatting PLY models
* Extracting Gaussian position and spherical harmonic color data
* Converting spherical harmonic coefficients to RGB
* Uploading Gaussian data to GPU storage buffers
* GPU-based projection using Vulkan compute shaders
* Interactive camera rotation, movement, and zoom
* Drag-and-drop model loading through ImGui

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- RESEARCH BACKGROUND -->
## Research Background

This project was conducted under the supervision of Professor Joseph N. Gregg at Lawrence University. The research explores 3D Gaussian Splatting (3DGS) and 4D Gaussian Splatting (4DGS) for reconstructing and viewing 3D scenes from multiple viewpoints. The study examines the limitations of earlier implicit representations and Neural Radiance Fields (NeRF), the technical foundation of Gaussian Splatting, and the development of a Vulkan-based renderer.
### Key Research Insights

* Neural Radiance Fields represent scenes as continuous functions but can be limited by sampling time and space complexity
* 3D Gaussian Splatting represents scenes using collections of semi-transparent 3D Gaussians
* Each Gaussian contains properties such as position, size, shape, color, and transparency
* Gaussian Splatting provides a differentiable, localized, and view-consistent representation of 3D scenes
* Extending Gaussian Splatting with temporal evolution enables the representation of dynamic scenes through 4D Gaussian Splatting

### Research Objectives

The thirteen-week study focused on two primary objectives:

* Develop an understanding of 3D Gaussian representations and their optimization process
* Develop a renderer capable of displaying trained Gaussian models and progressing toward novel view synthesis

### Results and Limitations

The Vulkan renderer successfully loads trained 3D Gaussian models and displays them as navigable point clouds. The system supports scene rotation, movement, zooming, and drag-and-drop model loading.

However, the current renderer displays each Gaussian as a colored point rather than a fully projected and alpha-composited splat. As a result, it provides a functional point-cloud preview rather than the photorealistic rendering achieved by full 3D Gaussian Splatting.

The remaining steps toward full novel view synthesis include:

* Gaussian sorting
* Ellipsoid projection
* Alpha compositing
* Extending from static scenes toward dynamic, multi-angle 4D capture


<!-- ROADMAP -->

## Roadmap

* [x] Train and optimize 3D Gaussian models
* [x] Load trained Gaussian models
* [x] Parse binary 3DGS PLY files
* [x] Extract Gaussian position and color information
* [x] Convert spherical harmonics to RGB
* [x] Upload Gaussian data to GPU storage buffers
* [x] Implement Vulkan compute-shader projection
* [x] Implement interactive orbit camera
* [x] Implement interactive model loading

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- LICENSE -->

## License
Distributed under the MIT License. See `LICENSE.txt` for more information.
<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- ACKNOWLEDGMENTS -->
## Acknowledgments
### Research Support  

Joseph N. Gregg — Faculty Mentor  

Lawrence University, Computer Science Department

---

### Technology & Software  

PyTorch — Used as the foundation for the Gaussian optimization and training pipeline.

Vulkan — Used to develop the renderer for loading and displaying trained 3D Gaussian models.

3D Gaussian Splatting — Used as the primary representation for reconstructing and viewing 3D scenes.

ImGui — Used to provide interactive controls and drag-and-drop model loading.

COLMAP — Used as part of the training pipeline and camera/coordinate conventions.

---

### Research References  
**Gordon, V. Scott, & Clevenger, John L.** (2024). *Computer Graphics Programming in OpenGL with C++* (3rd ed.). Mercury Learning & Information.

**Kerbl, B., Kopanas, G., Leimkühler, T., & Drettakis, G.** (2023). "3D Gaussian Splatting for Real-Time Radiance Field Rendering." *ACM Transactions on Graphics*, 42, 1–14.

**Mildenhall, B., Srinivasan, P. P., Tancik, M., Barron, J. T., Ramamoorthi, R., & Ng, R.** (2020). "NeRF: Representing Scenes as Neural Radiance Fields for View Synthesis." In *Computer Vision – ECCV 2020*, pp. 405–421. Springer.

**Vulkan Guide.** (2023–present). *Vulkan Guide: A Hands-on Vulkan Tutorial*. vkguide.dev.

<p align="right">(<a href="#readme-top">back to top</a>)</p>


<!-- MARKDOWN LINKS & IMAGES -->

[license-shield]: https://img.shields.io/github/license/Chimbay/SocialCueWebapp.svg?style=for-the-badge
[license-url]: https://github.com/Chimbay/Capstone/blob/main/LICENSE
[linkedin-shield]: https://img.shields.io/badge/-LinkedIn-black.svg?style=for-the-badge&logo=linkedin&colorB=555
[linkedin-url]: https://www.linkedin.com/in/chimbay

[vulkan-shield]: https://img.shields.io/badge/Vulkan-000000?style=for-the-badge&logo=vulkan&logoColor=white
[vulkan-url]: https://www.vulkan.org/

[pytorch-shield]: https://img.shields.io/badge/PyTorch-000000?style=for-the-badge&logo=pytorch&logoColor=white
[pytorch-url]: https://pytorch.org/

[python-shield]: https://img.shields.io/badge/Python-000000?style=for-the-badge&logo=python&logoColor=white
[python-url]: https://www.python.org/

[cpp-shield]: https://img.shields.io/badge/C%2B%2B-000000?style=for-the-badge&logo=c%2B%2B&logoColor=white
[cpp-url]: https://isocpp.org/
