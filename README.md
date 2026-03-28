# 🎮 Esquivar Botellas

<div align="center">

![C++](https://img.shields.io/badge/Lenguaje-C++-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)
![SDL3](https://img.shields.io/badge/Framework-SDL3-1C4C96?style=for-the-badge&logo=libsdl&logoColor=white)
![Status](https://img.shields.io/badge/Estado-En%20Desarrollo-FFA500?style=for-the-badge)
![Players](https://img.shields.io/badge/Jugadores-1%20Local-blue?style=for-the-badge)
![License](https://img.shields.io/badge/Licencia-MIT-green?style=for-the-badge)

**Juego de esquiva y supervivencia desarrollado en C++ con SDL3.**  
Muévete, esquiva enemigos, recoge el machete y supera tu récord.

---

</div>

## 👤 Autor

| Campo | Detalle |
|-------|---------|
| **Nombre** | Diego Alexander Neva Patiño |
| **Versión SDL** | SDL3 + SDL3_image + SDL3_ttf |

---

## 📋 Descripción General

**Esquivar Botellas** es un juego de acción y supervivencia en 2D. El jugador controla un personaje en el centro de la pantalla y debe esquivar enemigos que aparecen desde los cuatro bordes. Cada enemigo que cruza la pantalla suma un punto. A partir de los 20 puntos aparece un **machete** que el jugador puede recoger y usar para destruir enemigos cercanos. La dificultad aumenta progresivamente añadiendo un nuevo enemigo cada 5 puntos.

El juego incluye un **sistema de ranking Top 5** persistente guardado en un archivo binario local, con ingreso del nombre del jugador al superar un récord.

---

## 🗂️ Estructura del Proyecto

```
esquivar-botellas/
│
├── main.cpp                    # Código fuente principal
├── Arial Black.ttf             # Fuente usada en la UI
│
├── imagenes/
│   ├── fondo.png               # Imagen de fondo del juego
│   ├── player.png              # Sprite del jugador (64×64)
│   ├── enemy.png               # Sprite del enemigo (64×64)
│   └── machete.png             # Sprite del machete (64×64)
│
└── saves/
    └── puntajes.bin            # Top 5 (generado automáticamente)
```

> ⚠️ **Importante:** Las carpetas `imagenes/` y `saves/` deben estar en el mismo directorio que el ejecutable. La carpeta `saves/` se crea automáticamente al ejecutar el juego por primera vez.

---

## 🧠 Arquitectura del Sistema

### Máquina de Estados (FSM)

```
                    ┌──────────────┐
          ┌────────▶│  ESTADO_MENU │◀──────────────────────┐
          │         └──────┬───────┘                       │
          │           JUGAR│              INSTRUCCIONES     │
          │                │           ┌──────────────────┐ │
          │                │      ────▶│ESTADO_INSTRUCCIO-│ │
          │                │      │    │      NES         │ │
          │                │      │    └──────────────────┘ │
          │         ┌──────▼──────────┐                     │
   Enter  │         │ ESTADO_JUGANDO  │──── ESC/START ────┐  │
   +reinic│         └──────┬──────────┘                   │  │
          │         Colisión│                             ▼  │
          │                │                ┌──────────────────┐
          │    Top 5?       ▼      No        │ ESTADO_PAUSADO   │
          │   ┌───────────────────────┐      └──────┬───────────┘
          │   │ ESTADO_INGRESANDO_    │   ESC/START │ continuar
          │   │       NOMBRE         │             │
          │   └──────────┬───────────┘             │
          │        Enter │                         │
          │              ▼                         │
          │   ┌──────────────────┐                 │
          └───│ ESTADO_GAME_OVER │◀────────────────┘
              └──────────────────┘
```

### Flujo del Game Loop

```
while ejecutando:
  MENU              → manejarEventosMenu() + renderizarMenu()
  INSTRUCCIONES     → renderizarInstrucciones()
  JUGANDO           → manejarEventos()
                      actualizarJugador()
                      actualizarAnimacionAtaque()
                      actualizarPosicionMacheteEquipado()
                      actualizarEnemigos()
                      renderizar()             ← un solo RenderPresent
  INGRESANDO_NOMBRE → renderizarIngresoNombre() ← SDL_StartTextInput activo
  PAUSADO           → dibujarJuego() + overlay  ← un solo RenderPresent
  GAME_OVER         → renderizarGameOver()
  SDL_Delay(16)                                 ← ~60 FPS
```

---

## 🕹️ Mecánicas de Juego

### Enemigos
- Aparecen desde los 4 bordes en dirección horizontal o vertical
- Velocidad fija de **5 px/frame**
- Al salir por el borde opuesto: **+1 punto** y se regeneran
- Al colisionar con el jugador: **Game Over**
- Cada **5 puntos** se añade un nuevo enemigo (máximo 50)

### Machete
- Aparece en posición aleatoria al alcanzar **20 puntos**
- Se recoge por colisión directa
- Al activarlo (ESPACIO / Botón Sur del gamepad):
  - Gira 360° alrededor del jugador en **300 ms**
  - Destruye todos los enemigos dentro de **radio 150 px**
  - Cada enemigo destruido suma **+1 punto**
- **Cooldown de 2 segundos**, visualizado con barra de carga gradiente

### Dificultad Progresiva

| Puntaje | Enemigos activos |
|---------|-----------------|
| 0–4     | 1               |
| 5–9     | 2               |
| 10–14   | 3               |
| 15–19   | 4               |
| 20+     | 5+ (machete disponible) |

---

## 🏆 Sistema de Ranking Top 5

Los 5 mejores puntajes se guardan en `saves/puntajes.bin` como archivo binario estructurado.

### Formato del archivo binario

```
[int cantidad]           ← número de entradas válidas (0–5), 4 bytes
[EntradaPuntaje × N]     ← cada entrada: 36 bytes
  └── char nombre[32]    ← nombre del jugador (max 31 chars + null)
      int  puntuacion    ← puntaje entero
```

**Tamaño máximo del archivo:** `4 + 5 × 36 = 184 bytes`

### Flujo al terminar la partida

```
Colisión
  └── ¿puntuacion entra al top 5?
        │
        ├── SÍ → ESTADO_INGRESANDO_NOMBRE
        │          Ingresa nombre + Enter
        │          └── insertarPuntaje() → guardarPuntajes()
        │          └── ESTADO_GAME_OVER (entrada resaltada en verde)
        │
        └── NO → ESTADO_GAME_OVER directamente
```

### Colores en el ranking

| Color | Significado |
|-------|-------------|
| 🟡 Dorado | Primer lugar |
| ⚪ Blanco | Puestos 2–5 |
| 🟢 Verde | Entrada recién ingresada en esta partida |
| ⚫ Gris | Puesto vacío (sin récord) |

---

## ⚙️ Funciones Principales

### Gestión de puntajes

| Función | Descripción |
|---------|-------------|
| `crearDirectorioSaves()` | Crea la carpeta `saves/` si no existe (cross-platform) |
| `cargarPuntajes(tabla)` | Lee el archivo binario y llena la `TablaPuntajes` |
| `guardarPuntajes(tabla)` | Escribe la tabla ordenada en el archivo binario |
| `calificaParaTop5(tabla, pts)` | Retorna `true` si el puntaje entra al top 5 |
| `insertarPuntaje(tabla, nombre, pts)` | Inserta ordenado desc., retorna índice donde quedó |
| `renderizarTop5(juego, x, y, resaltar)` | Dibuja el ranking en pantalla con colores |

### Renderizado

| Función | Descripción |
|---------|-------------|
| `dibujarJuego(juego)` | Dibuja el estado completo del juego **sin** `SDL_RenderPresent` |
| `renderizar(juego)` | Llama `dibujarJuego()` + un único `SDL_RenderPresent()` |
| `renderizarPausa(juego)` | `dibujarJuego()` + overlay semitransparente + `SDL_RenderPresent()` |
| `renderizarBarraCooldown(juego)` | Barra con icono, gradiente rojo→verde y etiqueta |
| `renderizarTexto(juego, ...)` | Fuente 36px — títulos y opciones de menú |
| `renderizarTextoPequeno(juego, ...)` | Fuente 22px — top 5 y etiquetas secundarias |

> **Nota de diseño:** `dibujarJuego` y `renderizar` están separados intencionalmente para evitar el doble `SDL_RenderPresent` que causaba parpadeo en la pantalla de pausa.

### Machete

| Función | Descripción |
|---------|-------------|
| `usarMachete(juego)` | Activa el ataque si el cooldown está completo |
| `calcularProgresoCooldown(juego)` | Retorna `float` 0.0–1.0 del progreso de carga |
| `actualizarAnimacionAtaque(juego)` | Actualiza ángulo de giro (0°→360° en 300 ms) |
| `calcularPosicionMacheteGirando(juego, x, y)` | Posición orbital del machete mientras ataca |

---

## 🖥️ Interfaz de Usuario

### Menú Principal
```
┌─────────────────────────────────────────────────────────────────┐
│                              │                                   │
│   ESQUIVAR BOTELLAS          │      --- TOP 5 ---               │
│                              │                                   │
│   > JUGAR                    │   #1 Juan          42            │
│     INSTRUCCIONES            │   #2 Ana           35            │
│     SALIR                    │   #3 Pedro         28            │
│                              │   #4 ---                         │
│                              │   #5 ---                         │
│                              │                                   │
│  Flechas/DPad: navegar    Enter/Cruz: seleccionar               │
└─────────────────────────────────────────────────────────────────┘
```

### Durante el Juego
```
┌─────────────────────────────────────────────────────────────────┐
│ Score: 23                                                        │
│                                                                  │
│          [enemigos aparecen y cruzan la pantalla]                │
│                                                                  │
│                        [jugador]                                 │
│                                                                  │
│  🗡 [████████████░░░] LISTO [ESPACIO]                           │
└─────────────────────────────────────────────────────────────────┘
```

### Barra de Cooldown del Machete

```
  [🗡] [██████████████████░░░░░]   LISTO [ESPACIO]
           rojo → amarillo → verde     etiqueta de estado
```

- **Rojo (0–50%):** recargando
- **Amarillo (50–99%):** casi listo
- **Verde + "LISTO [ESPACIO]":** disponible para atacar

### Game Over
```
┌─────────────────────────────────────────────────────────────────┐
│                              │                                   │
│   GAME OVER                  │      --- TOP 5 ---               │
│                              │                                   │
│   Puntuacion: 42             │  #1 Juan           42  ← verde   │
│   TOP 5! Puesto #1           │  #2 Ana            35            │
│                              │  #3 Pedro          28            │
│   Enter / Cruz   Menu        │  #4 ---                         │
│   ESC / START    Salir       │  #5 ---                         │
│                              │                                   │
└─────────────────────────────────────────────────────────────────┘
```

---

## 🎮 Controles

### Teclado

| Tecla | Acción |
|-------|--------|
| `W` / `A` / `S` / `D` | Mover al jugador |
| `ESPACIO` | Usar machete (equipado y listo) |
| `ESC` | Pausar el juego |
| `ESC` en pausa | Continuar jugando |
| `Enter` en pausa | Volver al menú (reinicia la partida) |
| `Q` en pausa | Salir del juego |
| `Enter` en Game Over | Volver al menú |

### Control PS3 / PS4 / Xbox

| Botón | Acción |
|-------|--------|
| Stick izquierdo / D-pad | Mover al jugador |
| Botón Sur (Cruz / A) | Usar machete |
| START | Pausar / Continuar |
| Botón Sur en pausa | Volver al menú |

---

## 🚀 Compilación y Ejecución

### Requisitos

- Compilador C++17 o superior (`g++`, `clang++`, MSVC)
- [SDL3](https://github.com/libsdl-org/SDL)
- [SDL3_image](https://github.com/libsdl-org/SDL_image)
- [SDL3_ttf](https://github.com/libsdl-org/SDL_ttf)

### Linux / macOS

```bash
g++ main.cpp -o esquivar_botellas \
    $(sdl3-config --cflags --libs) \
    -lSDL3_image -lSDL3_ttf \
    -std=c++17 -O2

./esquivar_botellas
```

### Windows (MinGW)

```bash
g++ main.cpp -o esquivar_botellas.exe \
    -I"path/to/SDL3/include" \
    -L"path/to/SDL3/lib" \
    -lSDL3 -lSDL3_image -lSDL3_ttf \
    -std=c++17 -O2 -mwindows
```

### CMake

```cmake
cmake_minimum_required(VERSION 3.20)
project(EsquivarBotellas)
set(CMAKE_CXX_STANDARD 17)

find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(SDL3_ttf REQUIRED)

add_executable(esquivar_botellas main.cpp)
target_link_libraries(esquivar_botellas
    SDL3::SDL3 SDL3_image::SDL3_image SDL3_ttf::SDL3_ttf)
```

---

## 🔄 Migración SDL2 → SDL3

Este proyecto está escrito íntegramente para **SDL3**. Diferencias clave:

| SDL2 | SDL3 |
|------|------|
| `SDL_Rect` | `SDL_FRect` (coordenadas `float`) |
| `SDL_RenderCopy` | `SDL_RenderTexture` |
| `SDL_RenderDrawRect` | `SDL_RenderRect` |
| `SDL_RenderDrawLine` | `SDL_RenderLine` (parámetros `float`) |
| `SDL_FreeSurface` | `SDL_DestroySurface` |
| `SDL_HasIntersection` | `SDL_HasRectIntersectionFloat` |
| `SDL_GetTicks` → `Uint32` | `SDL_GetTicks` → `Uint64` |
| `SDL_GameController*` | `SDL_Gamepad*` |
| `SDL_CONTROLLERBUTTONDOWN` | `SDL_EVENT_GAMEPAD_BUTTON_DOWN` |
| `SDL_CONTROLLER_BUTTON_A` | `SDL_GAMEPAD_BUTTON_SOUTH` |
| `e.key.keysym.sym` | `e.key.key` |
| `const Uint8* SDL_GetKeyboardState` | `const bool*` |
| `SDL_CreateRenderer(w, -1, 0)` | `SDL_CreateRenderer(w, NULL)` |
| `SDL_NumJoysticks` | `SDL_GetGamepads(&count)` |
| `SDL_StartTextInput()` | `SDL_StartTextInput(window)` |

---

## 📊 Estructuras de Datos

```cpp
struct EntradaPuntaje {
    char nombre[32];   // nombre del jugador (max 31 chars + null)
    int  puntuacion;   // puntaje obtenido
};                     // 36 bytes por entrada

struct TablaPuntajes {
    EntradaPuntaje entradas[5];  // ordenadas descendentemente
    int            cantidad;     // entradas válidas (0–5)
};

struct Machete {
    SDL_FRect rect;
    bool      recogido;
    bool      activo;
    Uint64    ultimoUso;        // timestamp de SDL_GetTicks()
    bool      animandoAtaque;
    Uint64    inicioAnimacion;
    float     anguloActual;     // 0.0–360.0 durante el giro
};
```

---

## 🐛 Limitaciones Conocidas

| Limitación | Descripción |
|-----------|-------------|
| Velocidad de enemigos | Fija en 5 px/frame, no escala con el tiempo |
| Dirección de enemigos | Solo horizontal o vertical, no diagonal |
| Sin sonido | No implementado (`SDL3_mixer` no incluido) |
| Fuente requerida | `Arial Black.ttf` debe estar en el directorio raíz |
| Resolución fija | Ventana de 1400×900 px, sin soporte de redimensionado |

---

## 📄 Licencia

Proyecto personal de **Diego Alexander Neva Patiño**.  
Libre para uso personal. No distribuir con fines comerciales sin autorización del autor.

---

<div align="center">

Hecho con 🎮 y mucho `SDL_RenderPresent` por **Diego Alexander Neva Patiño**  
2024/2025

</div>