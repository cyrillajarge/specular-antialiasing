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
![result-without-specular-antialiasing](/uploads/4a86edebf66a9b9f5e8621fbedbc2b2d/result-without-specular-antialiasing.png)
**With specular antialiasing:**
![result-with-specular-antialiasing](/uploads/14d46fc2c4f885a3f974bc09f3923acb/result-with-specular-antialiasing.png)


## References
<a id="1">[1]</a> 
Kaplanyan, A. S. and Hill, S. and Patney, A. and Lefohn, A. , **Filtering Distributions of Normals for Shading Antialiasing**, 2016    
<a id="1">[2]</a> 
https://bkaradzic.github.io/bgfx/index.html