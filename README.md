# esquiva-mond-antioque-a
# esquiva mondá antioqueña

**esquiva mondá antioqueña** es un juego 2D desarrollado en **C++** utilizando las librerías **SDL2**, **SDL2_image** y **SDL2_ttf**. El objetivo del juego es sobrevivir el mayor tiempo posible esquivando enemigos que aparecen desde los bordes de la pantalla.

A medida que el jugador obtiene puntos, la dificultad aumenta progresivamente con más enemigos en pantalla.

---

##  Vista general

* Juego 2D en tiempo real
* Movimiento libre en pantalla
* Enemigos generados de forma aleatoria
* Incremento dinámico de dificultad

---

##  Controles

|    Tecla    | Acción          |
| ----------- | --------------- |
|    **W**    | Mover arriba    |
|    **S**    | Mover abajo     |
|    **A**    | Mover izquierda |
|    **D**    | Mover derecha   |
| **espace**  | Atacar          |
---

##  Objetivo del juego

Evitar colisionar con las botellas enemigas el mayor tiempo posible. Cada enemigo que sale de la pantalla sin colisionar aumenta el puntaje del jugador. Cada 5 puntos, aparece un nuevo enemigo, incrementando la dificultad.

El juego termina cuando el jugador colisiona con un enemigo.

---

##  Características principales

* Movimiento fluido del jugador
* Generación aleatoria de enemigos desde los bordes
* Sistema de puntaje
* Aumento progresivo de dificultad
* Detección de colisiones
* Control de FPS (~60 FPS)
* Uso de texturas PNG

---

##  Tecnologías utilizadas

* **Lenguaje:** C++
* **Librerías:**

  * SDL2
  * SDL2_image
  * SDL2_ttf
  * Archivo de fuente .ttf (Arial Black.ttf recomendado)
* **Renderizado:** Acelerado por hardware

---

##  Estructura básica del proyecto

```
Esquivar-Botellas/
│
├── main.cpp          # Código fuente principal
├── player.png        # Textura del jugador
├── enemy.png         # Textura de los enemigos
├── README.md         # Documentación del proyecto
├── Arial Black.ttf   # Fuente para texto
├── fondo.png         # Textura del Fondo del juego
├── machete.png       # Textura del arma principal del juego
```

---

##  Compilación y ejecución

###  Requisitos

* Compilador C++ (g++, clang o MSVC)
* SDL2
* SDL2_image
* SDL2_ttf
* Archivo de fuente .ttf (arial.ttf recomendado)

###  Compilar (ejemplo en Linux / MinGW)

```bash
g++ main.cpp -o Esquiva modaa antioqueña \
    -lSDL2 -lSDL2_image -lSDL2_ttf
```

###  Ejecutar

```bash
./Esquiva modaa antioqueña
```

> Asegúrate de que `player.png` , `fondo.png`, `machete.png`, `Arial Black.ttf` y `enemy.png` estén en la misma carpeta que el ejecutable.

---

##  Lógica del juego (resumen)

* El jugador se mueve con el teclado dentro de los límites de la pantalla
* Los enemigos aparecen desde un borde aleatorio
* Cada enemigo tiene una velocidad dirigida hacia el interior
* Al salir de la pantalla, el enemigo se reutiliza
* El puntaje aumenta y se generan más enemigos

---

##  Posibles mejoras futuras

* Sistema de vidas
* Menú principal
* Sonidos y música
* Pantalla de Game Over
* Niveles o dificultad seleccionable
* Mostrar el puntaje en pantalla
* Uso de clases (`Player`, `Enemy`)
* Nuevos objetos
* Nuevas habilidades
* Mas niveles

---

##  Autor Ing (c) Diego Neva .

Proyecto desarrollado con fines educativos para aprender los fundamentos de **SDL2**, manejo de eventos, renderizado y lógica básica de videojuegos en C++.

---

¡Siéntete libre de clonar, modificar y mejorar este proyecto! 🚀
