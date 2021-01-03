# Specular Antialiasing using Kaplanyan et. al method [[1]](#1) with BGFX rendering library [[2]](#2)

## BGFX Dependencies
- https://github.com/bkaradzic/bx
- https://github.com/bkaradzic/bimg

## How to use

1. Get **bg** as well as **bimg** and put them in the same directory as this repository.
2. Go to the root of bgfx directory and run the command ``` ..\bx\tools\bin\windows\genie.exe --platform=x64 --with-tools --with-examples --with-windows=10 vs2019 ``` to build all examples for Windows with Visual Studio 2019. Consult the help of ```genie``` to see how to build the project if you are on another platform.
3. The Visual Studio project is then available under the ```.build``` directory.

The implementation is done in an example called **xx-specular-antialiasing** in ```examples```.

## Results
**Without specular antialiasing:**   
![result-without-specular-antialiasing](https://user-images.githubusercontent.com/32383570/103460990-2bd88d00-4d1b-11eb-943a-c8f1391510b8.png)
**With specular antialiasing:**
![result-with-specular-antialiasing](https://user-images.githubusercontent.com/32383570/103460992-2c712380-4d1b-11eb-855c-120a0fa92c35.png)

## References
<a id="1">[1]</a> 
Kaplanyan, A. S. and Hill, S. and Patney, A. and Lefohn, A. , **Filtering Distributions of Normals for Shading Antialiasing**, 2016    
<a id="1">[2]</a> 
https://bkaradzic.github.io/bgfx/index.html
