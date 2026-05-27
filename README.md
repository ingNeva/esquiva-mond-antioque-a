# 🎮 Esquiva Modaá Antioqueñas

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
| **Versión SDL** | SDL3 + SDL3_image + SDL3_ttf + SDL3_mixer |

---

## 📋 Descripción General

**Esquivar Botellas** es un juego de acción y supervivencia en 2D. El jugador controla un personaje animado que debe esquivar enemigos variados que aparecen desde los cuatro bordes de la pantalla. El juego cuenta con **5 niveles** con mecánicas únicas cada uno, un **sistema de progresión persistente**, un **machete** con animación de ataque orbital, un **jefe final** con pilares destructibles, y un sistema de **combo y multiplicador de puntos**.

---

## 🗂️ Estructura del Proyecto

```
esquivar-botellas/
│
├── main.cpp
├── Arial Black.ttf
│
├── core/
│   ├── Game.h / Game.cpp          # SDL, texturas, fuentes, helpers de texto
│   ├── AudioManager.h / .cpp      # Música por pista y estado
│   ├── InputManager.h / .cpp      # Teclado + gamepad
│   └── World.h / World.cpp        # Física, colisiones, mecánicas por nivel
│
├── entities/
│   ├── Player.h / Player.cpp      # Jugador, animación, HUD puntuación
│   ├── Enemy.h / Enemy.cpp        # Tipos de enemigo, movimiento, combo HUD
│   ├── Machete.h / Machete.cpp    # Ataque orbital, animación de estela, cooldown
│   ├── Boss.h / Boss.cpp          # Jefe final, pilares, proyectiles, transición de nivel
│   └── Llave.h / Llave.cpp        # Llave de nivel, spawn, colisión y transición
│
├── scenes/
│   ├── GameScene.h / .cpp         # Loop de juego, render, transición de nivel
│   ├── MenuScene.h / .cpp         # Menú principal
│   ├── CountdownScene.h / .cpp    # Cuenta regresiva + intro cinemática
│   ├── GameOverScene.h / .cpp     # Game over, pausa, victoria, ingreso de nombre
│   ├── OptionsScene.h / .cpp      # Resolución, pantalla completa, audio
│   └── LevelSelectScene.h / .cpp  # Selección de nivel con sistema de bloqueos
│
├── utils/
│   ├── Constants.h                # Todas las constantes del juego
│   ├── Types.h                    # Structs: Juego, Jugador, Enemigo, Machete, etc.
│   ├── ScoreManager.h / .cpp      # Sistema de puntos, combo, floating texts
│   └── SaveManager.h              # Progreso de niveles en disco (progreso.bin)
│
├── imagenes/
│   ├── bg_nivel1.png … bg_nivel5.png
│   ├── player_walk_right/left/up/down.png   # Spritesheets direccionales (8 frames)
│   ├── enemy.png, enemy_mini.png, enemy_boss.png
│   ├── enemy_azul.png, enemy_roja.png, enemy_maxima.png
│   ├── machete.png, llave.png, trofeo.png, pilar.png
│
├── musica/
│   ├── Map (basic version).wav    # Menú
│   ├── Map.wav                    # Pausa
│   ├── Mars.wav                   # Niveles 1-3
│   ├── BossIntro.wav              # Nivel 4 (intro jefe)
│   ├── BossMain.wav               # Nivel 5 (jefe activo)
│   ├── Warp Jingle.wav            # Game over
│   └── victoria.wav               # Victoria
│
└── saves/
    ├── puntajes.bin               # Top 5 (generado automáticamente)
    ├── progreso.bin               # Niveles desbloqueados
    └── config.bin                 # Resolución, pantalla completa, audio
```

---

## 🧠 Arquitectura del Sistema

### Máquina de Estados (FSM)

```
┌─────────────────────────────────────────────────────────────────────────┐
│                          ESTADO_MENU                                    │
│         ↑REINICIAR             │ JUGAR        │ SELECCIONAR NIVEL       │
│                                ↓              ↓                         │
│                         ESTADO_INTRO    ESTADO_SELECCION_NIVEL          │
│                                │              │ (nivel desbloqueado)    │
│                                ↓              ↓                         │
│                     ESTADO_CUENTA_REGRESIVA (sin uso actual)            │
│                                │                                        │
│                                ↓                                        │
│                         ESTADO_JUGANDO ──ESC──→ ESTADO_PAUSADO          │
│                                │                       │                │
│                         (colisión)              (Enter → menú)          │
│                                ↓                                        │
│                    ¿Top 5? → ESTADO_INGRESANDO_NOMBRE                   │
│                       No  → ESTADO_GAME_OVER                            │
│                                                                         │
│               ESTADO_TRANSICION_NIVEL (al recoger llave)                │
│               ESTADO_VICTORIA (al recoger trofeo del jefe)              │
│               ESTADO_OPCIONES (desde menú)                              │
│               ESTADO_INSTRUCCIONES (desde menú)                         │
└─────────────────────────────────────────────────────────────────────────┘
```

### Game Loop Principal

```
while ejecutando:
  reproducirMusica() según estado actual

  ESTADO_MENU              → manejarEventosMenu() + renderizarMenu()
  ESTADO_INTRO             → renderizarIntro()
  ESTADO_JUGANDO           → manejarEventos()
                             actualizarJugador()
                             actualizarAnimacionJugador()
                             actualizarAnimacionAtaque()
                             actualizarPosicionMacheteEquipado()
                             mundoActualizar()        ← física + colisiones + mecánicas
                             renderizar()
  ESTADO_TRANSICION_NIVEL  → actualizarTransicionNivel() + renderizarTransicionNivel()
  ESTADO_PAUSADO           → dibujarJuego() + overlay semitransparente
  ESTADO_GAME_OVER         → renderizarGameOver()
  ESTADO_VICTORIA          → renderizarVictoria()
  ESTADO_OPCIONES          → renderizarOpciones()
  ESTADO_SELECCION_NIVEL   → manejarEventosSeleccionNivel() + renderizarSeleccionNivel()
  SDL_Delay(16)            ← ~60 FPS
```

---

## 🕹️ Mecánicas de Juego

### Jugador y Animación

El jugador se mueve en las cuatro direcciones con velocidad de **4 px/frame**. Dispone de **4 spritesheets direccionales** (`player_walk_right/left/up/down.png`), cada uno con **8 frames de 64×64 px** que se animan a ~12 FPS cuando el jugador está en movimiento. Al detenerse, el frame vuelve a 0 (pose de reposo).

La hitbox del jugador es reducida respecto al sprite: se recorta un **30% a cada lado** y el **35% superior** (sombrero), dejando solo el cuerpo visible para las colisiones.

### Tipos de Enemigo

| Tipo | Textura | Comportamiento |
|------|---------|---------------|
| **Básico** | `enemy.png` | Se mueve en línea recta desde un borde |
| **Rápido** | `enemy_mini.png` | Velocidad ×1.8, tamaño 48×48 px |
| **Zigzag** | `enemy_azul.png` | Trayectoria sinusoidal perpendicular a su avance |
| **Bombardero** | `enemy_roja.png` | Cada 2 s genera 2 enemigos básicos adicionales |
| **Espejo** | `enemy_maxima.png` | Persigue la posición espejada del jugador (opuesta en pantalla). Velocidad progresiva según nivel |

> El tipo **Tanque** fue eliminado y reemplazado por el Espejo en todos los niveles.

### Sistema de Combo y Multiplicador

Cada esquive o kill suma al combo. El multiplicador afecta directamente los puntos obtenidos:

| Combo | Multiplicador | Color floating text |
|-------|--------------|---------------------|
| 1-2   | ×1.0         | Blanco              |
| 3-4   | ×1.5         | Amarillo            |
| 5-9   | ×2.0         | Naranja             |
| 10-19 | ×3.0         | Rojo                |
| 20+   | ×5.0         | Púrpura             |

El combo se reinicia al morir. Un **esquive cercano** (enemigo que apuntaba al jugador y pasó a menos de 55 px) otorga +5 pts en color cian.

---

## ⚔️ El Machete

### Recogida y Equipamiento

En el nivel 1 el machete aparece en el centro de la pantalla durante la intro cinemática y se recoge al caminar sobre él. A partir del nivel 4 el jugador ya lo lleva equipado desde el inicio.

### Ataque Orbital con Estela

Al presionar `ESPACIO` (o botón Sur del gamepad) el machete gira **360° alrededor del jugador** en **300 ms** con un radio de **50 px**. La animación incluye:

1. **Estela de 7 copias fantasmas** detrás del sprite principal, cada una con alpha decreciente y rotación tangencial (perpendicular al radio orbital).
2. **Coloración dinámica**: naranja cálido en la fase de impacto (25%–75% del giro), azul frío al inicio y final.
3. **Destello luminoso** que viaja con el sprite principal durante la fase de impacto.
4. El sprite se rota para que la hoja **apunte tangencialmente** al círculo en cada posición.

El machete daña todos los enemigos en un **radio de 150 px**. Los enemigos tipo Espejo tienen `vida = 1`; los Tanques (si existen) requerían varios golpes.

### Barra de Cooldown

El cooldown es de **2 segundos**. La barra se muestra en la esquina inferior izquierda:

- **Rojo → Amarillo → Verde** según el progreso
- Icono del machete a la izquierda
- Etiqueta `"X.Xs"` mientras recarga, `"LISTO [ESPACIO]"` cuando está disponible

---

## 🗺️ Sistema de Niveles

### Progresión

El juego tiene **5 niveles** numerados base-1. El nivel actual se guarda en `juego.nivelActual` y no depende de la puntuación global (a diferencia del sistema legacy `nivelActual(puntuacion)`).

| Nivel | Nombre | Mecánica especial |
|-------|--------|-------------------|
| 1 | El Barrio | Solo enemigos básicos. Intro cinemática de recogida de machete |
| 2 | La Cantina | Enemigos rápidos y zigzag + **zonas de riesgo** |
| 3 | El Monte | Tanques (espejo) y bombarderos + **niebla periódica** |
| 4 | El Jefe Final | Machete ya equipado + **onda expansiva** del jefe |
| 5 | Máxima Dificultad | Jefe activo con pilares + máxima variedad de enemigos |

### Mecánicas Especiales por Nivel

**Nivel 2 — Zonas de Riesgo:**  
Tres zonas rectangulares fijas (proporcionales a la resolución) con relleno rojo semitransparente y borde pulsante. Al pisarlas, el jugador muere instantáneamente.

**Nivel 3 — Niebla Periódica:**  
Cada 5 s aparece una capa oscura semitransparente (alpha máx. 200) durante 2.5 s, con fade-in de 600 ms y fade-out de 600 ms. Obliga a memorizar las posiciones de los enemigos.

**Nivel 4 — Onda Expansiva:**  
Cada 4 s sale una onda anular desde el centro de la pantalla que crece a 6 px/frame. Al tocar al jugador lo empuja radialmente con una fuerza de 180 px que se desacelera en 220 ms. Visualmente se muestra como un anillo amarillo-eléctrico de grosor 8 px.

### Llave de Nivel (Transición)

Al acumular suficientes `puntosEnNivel` en cada nivel aparece una **llave dorada pulsante**:

| Nivel | Puntos requeridos |
|-------|------------------|
| 1 | 1800 |
| 2 | 2200 |
| 3 | 2800 |
| 4 | 3500 |

El spawn intenta 50 posiciones aleatorias a más de 150 px del jugador. Si los 50 intentos fallan, la llave aparece garantizada en el centro de pantalla (nunca falla). Al recogerla se inicia la transición al siguiente nivel y se guarda el progreso en disco.

### Transición de Nivel

La transición dura **2.5 s** con fundido cruzado entre los fondos del nivel anterior y el nuevo, más un overlay pulsante de color temático (púrpura para niveles normales, naranja para el nivel 4) y una barra de progreso centrada.

---

## 👾 Jefe Final (Nivel 5)

### Pilares

Al entrar al nivel 5 aparecen **5 pilares** en posiciones aleatorias (mínimo 160 px del jefe). Cada pilar tiene un efecto de pulso de color púrpura.

### Comportamiento del Jefe

El jefe dispara proyectiles (tipo `ENEMIGO_RAPIDO`) hacia el jugador a cadencia regular:

| HP restante | Disparos por salva | Cadencia |
|-------------|-------------------|---------|
| 5-3 | 1 | 2500 ms |
| 2 | 2 (abanico ±30°) | 1200 ms (enfurecido) |
| 1 | 3 (abanico ±35°) | 1200 ms (enfurecido) |

### Destrucción de Pilares

El machete destruye los pilares al alcance durante el ataque. Cada pilar destruido reduce 1 HP al jefe y otorga 25 pts. Al llegar a 0 HP el jefe muere y aparece un **trofeo** en su posición. Al recogerlo se otorgan 200 pts y se pasa al estado `ESTADO_VICTORIA`.

**Barra de vida del jefe**: barra centrada en la parte superior, segmentada en 5 secciones, con parpadeo rojo en los últimos 2 HP.

---

## 🎬 Intro Cinemática

Al iniciar o reiniciar una partida se reproduce una secuencia cinemática de **3 fases** (`ESTADO_INTRO`):

1. **Caminata** (~1.4 s): El jugador entra desde el borde izquierdo hasta el centro con easing `(1 - (1-t)²)`. El machete pulsa dorado en el suelo esperando (solo nivel 1).
2. **Recogida** (~0.4 s): El machete sube y se agranda con alpha decreciente; destello amarillo expansivo al contacto.
3. **Texto** (~1.2 s): Aparece el mensaje `"¡Destruye las botellas y esquívalas!"` con fade-in/out centrado en pantalla.

Cualquier tecla (salvo ESC) o botón del gamepad salta la intro. ESC vuelve al menú.

---

## 📂 Sistema de Progreso Persistente

### Archivos en `saves/`

| Archivo | Contenido |
|---------|-----------|
| `puntajes.bin` | Top 5: nombre (32 chars) + puntuación (int) por entrada |
| `progreso.bin` | Array `bool nivelesDesbloqueados[5]` + nivel jugado + versión |
| `config.bin` | Resolución, pantalla completa, música activa, volumen |

### Desbloqueo de Niveles

Solo el nivel 1 está disponible al iniciar por primera vez. Recoger la llave de un nivel desbloquea el siguiente y guarda el progreso automáticamente. El menú **Seleccionar Nivel** muestra niveles bloqueados en gris con mensaje `"[BLOQUEADO]"`.

---

## 🖥️ Multi-Resolución

Todo el UI y la lógica de posicionamiento usa `VW(juego)` y `VH(juego)` (que consultan `SDL_GetWindowSize()` en tiempo de ejecución) **en lugar de las constantes fijas** `ANCHO_VENTANA`/`ALTO_VENTANA`. Esto garantiza que el juego se vea correctamente en cualquier resolución.

Los tamaños de fuente escalan con el patrón `VH(juego) * px / 1080`:
- **Fuente normal**: `VH * 36 / 1080` (mín. 14 px)
- **Fuente pequeña**: `VH * 22 / 1080` (mín. 10 px)

Al cambiar la resolución o alternar pantalla completa se llama a `recargarFuentes()`, que cierra y reabre las fuentes TTF con el nuevo tamaño.

### Resoluciones disponibles

Desde 800×600 hasta 3840×2160, incluyendo formatos ultrawide (2560×1080, 3440×1440). 18 opciones en total, seleccionables desde el menú de opciones.

---

## 🔊 Sistema de Audio

Cada estado del juego tiene su pista asignada. La transición entre pistas es automática:

| Estado | Pista |
|--------|-------|
| Menú, Instrucciones, Opciones | `Map (basic version).wav` |
| Pausa | `Map.wav` |
| Niveles 1-3 | `Mars.wav` |
| Nivel 4 (intro jefe) | `BossIntro.wav` (sin loop) |
| Nivel 5 | `BossMain.wav` |
| Game Over | `Warp Jingle.wav` (sin loop) |
| Victoria | `victoria.wav` |
| Transición de nivel | Silencio |

---

## 🏆 Ranking Top 5

Los 5 mejores puntajes se guardan en `saves/puntajes.bin`. Al terminar la partida, si el puntaje califica, se pide el nombre del jugador (máx. 31 caracteres). La entrada nueva se resalta en verde en la tabla.

**Formato binario**: `[int cantidad] + [EntradaPuntaje × N]` donde cada entrada tiene 36 bytes (32 nombre + 4 puntuación). Tamaño máximo: 184 bytes.

---

## 🎮 Controles

### Teclado

| Tecla | Acción |
|-------|--------|
| `W A S D` / Flechas | Mover al jugador |
| `ESPACIO` | Usar machete |
| `ESC` | Pausar / volver al menú |
| `R` en Game Over | Reintentar el mismo nivel |
| `Enter` | Confirmar / volver al menú |
| `M` | Silenciar / activar música |
| `+ / -` | Subir / bajar volumen |
| `F` en opciones | Alternar pantalla completa |

### Control PS3 / PS4 / Xbox

| Botón | Acción |
|-------|--------|
| Stick izq. / D-pad | Mover al jugador |
| Botón Sur (Cruz/A) | Usar machete / confirmar |
| Botón Norte (Triángulo/Y) | Reintentar nivel (en Game Over) |
| Botón Este (Círculo/B) | Volver / cancelar |
| START | Pausar / continuar |

---

## 🚀 Compilación y Ejecución

### Requisitos

- Compilador C++17 o superior
- [SDL3](https://github.com/libsdl-org/SDL)
- [SDL3_image](https://github.com/libsdl-org/SDL_image)
- [SDL3_ttf](https://github.com/libsdl-org/SDL_ttf)
- [SDL3_mixer](https://github.com/libsdl-org/SDL_mixer)

### Linux / macOS

```bash
g++ $(find . -name "*.cpp") -o esquivar_botellas \
    $(sdl3-config --cflags --libs) \
    -lSDL3_image -lSDL3_ttf -lSDL3_mixer \
    -std=c++17 -O2

./esquivar_botellas
```

### Windows (MinGW)

```bash
g++ $(Get-ChildItem -Recurse -Filter *.cpp | % { $_.FullName }) `
    -o esquivar_botellas.exe `
    -I"path/to/SDL3/include" -L"path/to/SDL3/lib" `
    -lSDL3 -lSDL3_image -lSDL3_ttf -lSDL3_mixer `
    -std=c++17 -O2 -mwindows
```

### CMake

```cmake
cmake_minimum_required(VERSION 3.20)
project(EsquivarBotellas)
set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SOURCES "*.cpp")

find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(SDL3_ttf REQUIRED)
find_package(SDL3_mixer REQUIRED)

add_executable(esquivar_botellas ${SOURCES})
target_link_libraries(esquivar_botellas
    SDL3::SDL3 SDL3_image::SDL3_image
    SDL3_ttf::SDL3_ttf SDL3_mixer::SDL3_mixer)
```

---

## 🔄 Migración SDL2 → SDL3

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
| `SDL_StartTextInput()` | `SDL_StartTextInput(window)` |
| `Mix_*` (SDL2_mixer) | `MIX_*` (SDL3_mixer) |

---

## 📊 Estructuras Principales

```cpp
struct Juego {
    // SDL
    SDL_Window* ventana; SDL_Renderer* renderer;
    SDL_Gamepad* gamepad;

    // Estado
    EstadoJuego estado;
    bool ejecutando;
    int nivelActual;        // base-1 (1..5)
    int puntosEnNivel;      // puntos acumulados en el nivel actual
    int puntuacion;         // puntuación total acumulada

    // Entidades
    Jugador jugador;
    Enemigo enemigos[MAX_ENEMIGOS]; // hasta 80
    int enemigosActivos;
    Machete machete;
    bool macheteEquipado;
    Llave llave;

    // Jefe
    EstadoBoss estadoBoss;
    int bossHP;
    Pilar pilares[MAX_PILARES];
    bool trofeoActivo;

    // Combo
    int combo; int mejorCombo; float multiplicador;
    FloatingText floatingTexts[MAX_FLOATING_TEXT];

    // Mecánicas por nivel
    SDL_FRect zonasRiesgo[3]; int zonasRiesgoCount;  // nivel 2
    bool nieblaActiva; Uint64 nieblaSiguiente;        // nivel 3
    bool ondaActiva; float ondaRadio;                 // nivel 4

    // Progresión
    bool nivelesDesbloqueados[MAX_NIVELES];

    // Audio, fuentes, texturas...
};
```

---

## ⚠️ Notas de Diseño

**Regla de posicionamiento**: Todo el código de UI y spawn de entidades usa `VW(juego)`/`VH(juego)` en tiempo de ejecución. **Nunca** usar `ANCHO_VENTANA`/`ALTO_VENTANA` directamente en escenas o lógica de posición.

**Spawn de llave**: Si los 50 intentos de posición alejada del jugador fallan, la llave aparece garantizada en el centro de pantalla. Esto evita el bug donde la llave nunca aparecía.

**Separación render/lógica**: `dibujarJuego()` dibuja sin llamar a `SDL_RenderPresent()`. `renderizar()` la envuelve y agrega el `Present`. La pantalla de pausa usa esta separación para pintar el juego como fondo antes del overlay.

**Fuentes**: `texJugador` es un alias de `texPlayerDown` y no se destruye por separado en `limpiarRecursos()`.

---

## 📄 Licencia

Proyecto personal de **Diego Alexander Neva Patiño**.  
Libre para uso personal. No distribuir con fines comerciales sin autorización del autor.

---

<div align="center">

Hecho con 🎮 y muchos `SDL_RenderPresent` por **Diego Alexander Neva Patiño**  
2024 / 2025

</div>
