# ToFMAP Viewer
Custom "from-scratch" computer vision software to be used as part of ToFMAP project

Tested under Linux environment, you can compile and test it under windows using MinGW or similar since it uses UNIX sockets

Dependencies
- Free OpenGL Utility Toolkit (freeglut)


Compilation

```sh
make .
```

Testing (using "run" as name for the compiled & linked object(s))

./run [Board Dimension (in blocks)] [Block N (unused feature)] [Max Height (unused feature)]
```sh
./run 150 5 4
```
