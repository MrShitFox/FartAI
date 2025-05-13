# FartAI++

## Description
This is an aimbot project for all games, written entirely in C++, using YOLO family models with DirectML hardware acceleration as the recognition core. The development is very raw.

## Historical Background and Thoughts
In early 2024, piss hit my head and I realized I wanted to write a neural network aimbot no matter what. Looking at other developments in this area made me want to cry; they were crooked Python crafts. I scratched my head and thought... "how am I any worse?". And I started writing my own shit. At first, nothing worked out, then I got ONNX Runtime with YOLOv8 running, then the screen capture fucked up, but the FPS on my GTX 770 left much to be desired.

Looking at this, I trained YOLOv8 Nano on a test dataset with 2 classes and ran it. Surprisingly, the FPS increased manifold, as the model now had only 2 classes instead of 80.

Great, I thought, and decided to make an overlay to draw detected enemies. And that... was... a mistake. After I implemented it, everything started to lag and desynchronize. Python and its divine multithreading (ahem, sorry, multiprocessing) made itself known.

After such a performance, I thought about porting the project to C++, and after a few days of torment, I shat out the first version of FartAI++. I rewrote all modules and mechanisms from scratch. Everything that was in Python, in fact, had to be written from scratch. I wrote screen capture modules `DesktopDuplicationCapture`, a module or even a core for fast Yolo inference on DirectML - `yoloLib`.

Everything more or less worked, but still crookedly. I continued to "improve" the code. In the end, so much shit appeared there that I myself barely remember what I wrote and where. `aimLogic` and `aimRender` are a complete ass, but it works! Screen capture is a crutch, well, it's a program for Windows, what did you expect. In principle, the aiming and calculation mechanisms are complete darkness, I don't advise reading it, but if you want to, please do.

Now, on May 13, 2025, I am uploading this to GitHub because I will hardly ever touch it again, although who knows.

## Features
*   Recognition core: YOLO ONNX converted models (v8, v11 tested)
*   Screen capture: Desktop Duplication API, there is also a Legacy version using BitBlt in the code
*   Custom aiming mechanisms
*   Overlay in pure WinAPI that will drop your FPS below the floorboards (don't worry, it's commented out)
*   GUI part in ImGui with a layer from https://github.com/SamuelTulach/ImGui-AppKit, half of it is taken without changes, looks like shit, but works somehow, basically like this whole project

## What's needed for compilation?
*   Windows 10+
*   Visual Studio
*   Libraries from `/FartAIv2/packages.config` (there's OpenCV and shit for DirectML)
*   MFC latest (installed via Visual Studio installer)
*   And patience

## How to train the model?
1.  Create a dataset on Roboflow.
2.  Train YOLO with the Ultralytics library on this dataset.
3.  Convert to ONNX using the same library.
4.  Rename the file to `model.onnx` and put it in the root directory with the `.exe`.
5.  Congratulations, you are happy, probably...

## End
Overall, that's all. This project took quite a lot of my time; overall I minimally did what I wanted and I am minimally satisfied with what I did. Do whatever you want with it. In the release, I've attached a working build with a trained model (YOLOv11n, trained on 4k images).
