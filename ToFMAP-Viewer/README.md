# ToFMAP Viewer
Software de visionado de superficies con vehiculo y ToF

Restricciones
- Probado sobre Linux, se necesita: ultimas versiones de Windows / MinGW o similares para hacer uso de este software (UNIX sockets)

Dependencias
- OpenGL freeglut

Compilación

```sh
make .
```

Ejecución
./binario [Dimensión tablero cuadrado] [N bloques (ignorado por ahora)] [Altura máxima (ignorado por ahora)]
```sh
./binario 150 5 4
```
