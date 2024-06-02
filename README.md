# ClassiCubeServyvalPlugin
This is a WIP ClassiCube plugin that attempts to create a survival-like experience in ClassiCube.  
  
![Demo](demo.gif)
## Downloads
The latest version can be downloaded [here](https://github.com/yomcube/ClassiCubeServyvalPlugin/actions/workflows/main.yml?query=is%3Asuccess).
## How to compile
### Linux
```bash
git clone https://github.com/ClassiCube/ClassiCube
ln -s ClassiCube/src/
gcc -shared -fPIC -o ServyvalPlugin.so ServyvalPlugin.c
```
