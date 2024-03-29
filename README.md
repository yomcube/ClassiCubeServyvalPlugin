# ClassiCubeServyvalPlugin
This is a WIP ClassiCube plugin that attempts to create a survival-like experience in ClassiCube.
## How to compile
### Linux
```bash
git clone https://github.com/UnknownShadow200/ClassiCube
ln -s ClassiCube/src/
gcc -shared -fPIC -o ServyvalPlugin.so ServyvalPlugin.c
```