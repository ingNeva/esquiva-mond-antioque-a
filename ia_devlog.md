# IA_DEVLOG — EsquivarBotellas / Esquiva Mondá Antioqueña

> **Propósito de este archivo:** que cualquier IA (o Claude en una sesión nueva) entienda el
> estado actual del código SIN tener que releer todo el repo. Cuando mi ñema diga
> **"actualiza"**, esta IA debe leer los commits recientes del repo
> (https://github.com/ingNeva/esquiva-mond-antioque-a) y actualizar las secciones que
> cambiaron: árbol de archivos, sistemas, pendientes y aprendizajes.
>
> Última actualización basada en el commit: `f63e38d` (rama `main`).

---

## 1. Qué es el proyecto

Juego 2D de esquiva y supervivencia en C++ con **SDL3** (+ SDL3_image, SDL3_ttf, SDL3_mixer).
El jugador esquiva enemigos que entran desde los bordes, recoge un **machete** (ataque circular
orbital) y una **chancla** (arma bumerán), sube de nivel recogiendo una **llave** por nivel,
y en el nivel 5 enfrenta un **boss** con pilares destructibles. Hay ranking Top-5 persistente,
progreso de niveles persistente, configuración persistente (resolución/audio/teclas), soporte
de gamepad (PS3/PS4/Xbox), y un sistema de combo/multiplicador de puntos con floating texts y
frases colombianas al matar enemigos.

- **Build:** CMake. `CMakeLists.txt` = build principal (Linux, vía pkg-config). `cmake/windows.cmake`
  = build específico para Windows/MSYS2-UCRT64 (mi ñema compila en Windows).
- **Ejecutable:** sale a `build/ejecutable/<linux|windows>/` — copiar manualmente a
  `ejecutable/<linux|windows>/` antes de correr, o correr desde el output de build directamente.
- **Saves binarios:** `saves/puntajes.bin` (Top5), `saves/config.bin` (resolución/audio/teclas),
  `saves/progreso.bin` (niveles desbloqueados).

---

## 2. Árbol de archivos y rol de cada uno

```
main.cpp                     → entry point, máquina de estados (switch sobre EstadoJuego), game loop a 16ms/frame

core/
  Game.h / Game.cpp          → struct Juego global, init SDL, carga de texturas/fuentes, limpieza,
                                reiniciarJuego(), config persistente (saves/config.bin), helpers de texto
                                (renderizarTexto*), VW()/VH() (tamaño real de ventana), renderizarTop5()
  World.h / World.cpp        → mundoActualizar() = loop de física/colisiones por frame. Mecánicas
                                especiales de nivel 2/3/4. Sistema de "rayos de proximidad" (esquive
                                cercano). Callbacks mundoOnEnemigoMuerto/Esquivado/ColisionJugador/
                                TrofeoRecogido/PilarDestruido. hitboxJugador() reducida.
  InputManager.h/.cpp        → manejarEventos() (teclado+gamepad durante partida), actualizarJugador()
                                (movimiento + animación, usa KeyConfig remapeable)
  AudioManager.h/.cpp        → música por estado/nivel, mute, volumen

entities/
  Player.h/.cpp              → inicializarJugador(), animación, mostrarPuntuacionPantalla() (HUD nivel/llave/audio)
  Enemy.h/.cpp                → spawn de enemigos (probabilidades por nivel), movimiento por tipo
                                (zigzag, bombardero que se reproduce, espejo que persigue posición
                                simétrica), floating texts, HUD de combo
  Machete.h/.cpp              → arma cuerpo a cuerpo, ataque circular orbital con estela, cooldown,
                                recogida, animación con destello
  Chancla.h/.cpp              → arma bumerán: ida recta → vuelta homing al jugador, rotación visual,
                                colisión con enemigos y pilares del boss, cooldown propio
  Boss.h/.cpp                 → pilares destructibles, disparo de proyectiles (1/2/3 según HP),
                                barra de vida, transición de nivel con fade entre fondos
  Llave.h/.cpp                → llave que aparece al alcanzar umbral de puntos del nivel, teletransporta
                                a posición aleatoria lejos del jugador, dispara transición de nivel

scenes/
  GameScene.h/.cpp            → dibujarJuego(): renderiza TODO el frame de juego (fondo, boss, pilares,
                                trofeo, jugador animado, enemigos, machete, chancla, llave, HUD,
                                overlays de nivel 2/3/4)
  MenuScene.h/.cpp            → menú principal
  CountdownScene.h/.cpp       → cuenta regresiva, instrucciones, intro cinemática (personaje camina
                                y recoge el machete)
  GameOverScene.h/.cpp        → game over, pausa, victoria, ingreso de nombre para Top5
  OptionsScene.h/.cpp         → resolución, pantalla completa, audio
  LevelSelectScene.h/.cpp     → selección de nivel con bloqueo/desbloqueo
  KeybindScene.h/.cpp         → remapeo de teclas (WASD/atacar/pausa)

utils/
  Types.h                     → TODOS los structs y enums del juego (Jugador, Enemigo, Machete, Chancla,
                                Llave, Pilar, FloatingText, TransicionNivel, KeyConfig, y el struct
                                Juego "god object" que contiene todo el estado global)
  Constants.h                 → todas las macros de balance (puntos, cooldowns, umbrales de nivel,
                                constantes de mecánicas por nivel), enums EstadoJuego/TipoEnemigo/
                                EstadoBoss/EstadoPista, tabla de resoluciones disponibles
  ScoreManager.h/.cpp          → Top5 en binario, sistema de puntos+combo+multiplicador, floating
                                texts, frases colombianas (machete=dorado, chancla=cyan), callback
                                mundoOnEsquiveCercano (bonus por esquive cercano)
  SaveManager.h                → progreso de niveles desbloqueados (saves/progreso.bin), todo inline
                                en el header

imagenes/, musica/            → assets (backgrounds por nivel, spritesheets del jugador, sprites de
                                enemigos por tipo, machete, chancla, llave, trofeo, pilar, música .wav)
```

---

## 3. Arquitectura general

- **Patrón "god object":** todo el estado del juego vive en un único `struct Juego` (en `Types.h`),
  pasado por puntero a casi todas las funciones. No hay clases; es C con SDL, estilo procedural.
- **Máquina de estados:** `enum EstadoJuego` (`ESTADO_MENU`, `ESTADO_JUGANDO`, `ESTADO_PAUSADO`,
  `ESTADO_TRANSICION_NIVEL`, `ESTADO_INTRO`, `ESTADO_TECLAS`, etc.) controla qué función de
  render/update se llama en el `switch` de `main.cpp`. Cada escena tiene su propio par
  `manejarEventosX()` / `renderizarX()`.
- **Nivel actual (`juego->nivelActual`)** es la fuente de verdad para dificultad/mecánicas,
  distinto de `nivelActual(puntuacion)` en `Game.cpp` que es solo un helper legacy por umbral
  de puntuación (ya no dirige el gameplay real; el avance de nivel ahora es por la **llave**).
- **Progresión de nivel:** puntos en nivel (`puntosEnNivel`) → llave aparece en umbral
  (`PUNTOS_LLAVE_NIVEL_X`) → jugador la recoge → `desbloquearSiguienteNivel()` +
  `iniciarTransicionNivel()` → fade visual de 2.5s → `nivelActual++`.

---

## 4. Sistemas clave (comportamiento actual)

### 4.1 Enemigos (`Enemy.cpp`)
- Tipos: `ENEMIGO_BASICO`, `RAPIDO`, `TANQUE` (⚠️ ver pendientes — nunca se genera), `ZIGZAG`,
  `BOMBARDERO` (se duplica cada 2s mientras vive), `ESPEJO` (persigue la posición simétrica del
  jugador respecto al centro de pantalla, no ataca directo; implementado y con textura dedicada
  `enemy_maxima.png` — **el archivo no existe en `imagenes/`**, así que cae al fallback
  `texEnemigo` en `cargarTexturas()`).
- Probabilidades de spawn varían por nivel (1 a 5); a partir de nivel 3 aparece ESPEJO, nivel 4
  BOMBARDERO, nivel 5 mezcla más agresiva.
- **Rayos de proximidad / esquive cercano** (en `World.cpp`): trackea `distMinAlcanzada` por
  enemigo dentro de `RADIO_BURBUJA_ESQUIVE`; cuenta esquive cuando se aleja
  `UMBRAL_ALEJAMIENTO` px desde el mínimo, o al salir del radio si llegó a estar muy cerca
  (`UMBRAL_SALIDA_ESQUIVE`). Da bonus de puntos según qué tan cerca llegó
  (`mundoOnEsquiveCercano` en `ScoreManager.cpp`).

### 4.2 Machete (`Machete.cpp`)
- Aparece en el mapa a partir de nivel 4 (`aparecerMachete`), se recoge por colisión.
- Ataque circular orbital de 360° en `DURACION_ANIMACION_ATAQUE` (300ms), con estela de 7 copias
  semitransparentes (`renderizarMacheteGirando`) y destello en la fase de impacto.
- Cooldown `COOLDOWN_MACHETE` (2000ms), radio de golpe `RANGO_ATAQUE` (150px) desde el centro
  del jugador — golpea todos los enemigos y pilares del boss dentro de ese radio en un solo uso.
- ⚠️ **Pendiente:** `usarMachete()` llama a `mundoOnEnemigoMuerto` (genérico, sin frase/puntos por
  tipo de arma), en vez de `mundoOnEnemigoMuertoMachete` (que sí da frase colombiana dorada y
  puntos por tipo de enemigo). La Chancla sí usa correctamente su callback dedicado.

### 4.3 Chancla (`Chancla.cpp`)
- Arma bumerán: se lanza con **tecla `C` fija (hardcodeada, NO remapeable vía KeyConfig)** o
  botón `WEST` del gamepad, en la dirección que mira el jugador (`Jugador.direccion`).
- Fase ida: recta hasta `CHANCLA_MAX_DISTANCIA` (350px) → fase vuelta: homing hacia el jugador
  hasta `CHANCLA_RADIO_RECOGIDA` (30px), con rotación visual continua y estela de color
  (naranja=ida, cyan=vuelta).
- Cooldown propio `COOLDOWN_CHANCLA` (3500ms), independiente del machete.
- Colisión: `break` tras el primer golpe por frame (fix aplicado para el bug de freeze por
  re-colisión con enemigo regenerado en el mismo sitio).
- Usa correctamente `mundoOnEnemigoMuertoChancla` → frase cyan + puntos.

### 4.4 Boss / nivel 5 (`Boss.cpp`)
- 5 pilares destructibles (`MAX_PILARES`) spawneados lejos del centro del boss.
- Dispara proyectiles (reutilizando `Enemigo` con `tipo = ENEMIGO_RAPIDO`) en 1/2/3 direcciones
  según HP restante; cadencia normal vs. "enfurecido" (`bossHP <= 2`).
- Al morir el último pilar (`bossHP <= 0`) aparece el trofeo → recogerlo dispara `ESTADO_VICTORIA`
  y cambia la música.

### 4.5 Combo / puntaje / floating text (`ScoreManager.cpp`)
- Multiplicador escalonado por racha: x1 → x1.5 (combo≥3) → x2 (≥5) → x3 (≥10) → x5 (≥20).
- Se resetea a 0 en `mundoOnColisionJugador` (game over).
- Floating texts en dos "carriles" del array (`MAX_FLOATING_TEXT`): primeros 2/3 para
  puntos/racha, últimos slots reservados para frases colombianas (`spawnFrase`).
- Frases: 8 de machete (doradas) + 8 de chancla (cyan), elegidas con `rand()`.

### 4.6 Input / Keybindings
- `KeyConfig` remapeable cubre: mover (4 direcciones), atacar (machete), pausa. **No incluye la
  chancla** (sigue hardcodeada a `SDLK_C`).
- Config binaria v2 con compatibilidad hacia v1 (`ConfigGuardada` en `Game.cpp`).

### 4.7 Mecánicas especiales por nivel (`World.cpp` + overlay en `GameScene.cpp`)
- **Nivel 2:** 3 zonas de riesgo fijas — tocarlas cuenta como colisión (game over instantáneo).
- **Nivel 3:** niebla cíclica (aparece/desaparece cada `NIEBLA_INTERVALO_MS`, dura
  `NIEBLA_DURACION_MS`), solo overlay visual, no afecta colisiones directamente.
- **Nivel 4:** onda expansiva periódica desde el centro que empuja al jugador
  (`ONDA_EMPUJON_FUERZA`) si está cerca del anillo cuando pasa.

---

## 5. Pendientes / bugs conocidos

1. **`Machete.cpp` usa el callback genérico** (`mundoOnEnemigoMuerto`) en vez de
   `mundoOnEnemigoMuertoMachete` → los kills a machete no muestran frase colombiana ni respetan
   el mismo criterio de puntos por tipo que sí tiene el callback dedicado (aunque los puntos base
   coinciden actualmente, están duplicados en dos lugares distintos — riesgo de que diverjan).
2. **`imagenes/enemy_maxima.png` no existe** — el enemigo ESPEJO (`ENEMIGO_ESPEJO`) sí está
   totalmente implementado (movimiento, spawn, puntos), pero visualmente usa el sprite genérico
   `enemy.png` por el fallback en `cargarTexturas()`. No es un crash, es un pendiente de arte.
3. **`ENEMIGO_TANQUE` es un tipo muerto:** existe en el enum, tiene lógica de render (barra de
   vida con puntos, textura dedicada `enemy_boss.png`), pero **nunca se asigna** en
   `generarEnemigoConJugador()` — ningún enemigo llega a spawnear como TANQUE. Decidir si se
   implementa su probabilidad de spawn o se elimina el código muerto.
4. **⚠️ CRÍTICO — `cmake/windows.cmake` tiene la lista `SOURCES` desactualizada:** le faltan
   `entities/Chancla.cpp` y `scenes/KeybindScene.cpp` (sí están en el `CMakeLists.txt` principal
   de Linux). Como `main.cpp` sí llama a funciones de esos dos archivos (`lanzarChancla`,
   `actualizarChancla`, `renderizarChancla`, `renderizarTeclas`, etc.), **el build de Windows
   debería fallar en el linker** con errores de referencia indefinida, a menos que se esté
   compilando de otra forma no reflejada en el repo. Hay que agregar ambos archivos a
   `cmake/windows.cmake` → `SOURCES`.
5. **Chancla no es remapeable** — sigue en `SDLK_C` fijo en `InputManager.cpp`, fuera del sistema
   `KeyConfig`. Si se quiere consistencia, habría que añadir un campo `lanzarChancla` a
   `KeyConfig` y a `KeybindScene`.

---

## 6. Aprendizajes / trampas ya conocidas (no repetir)

- **Símbolos duplicados en linker:** definir la misma función (ej. `mundoOnEsquiveCercano`) en
  más de un `.cpp` rompe el link. Cada función va en UNA sola unidad de traducción; su
  declaración va en el `.h` correspondiente.
- **Campos de struct sin inicializar:** campos como `distMinAlcanzada` en `Enemigo` deben tener
  valor por defecto explícito (`= 9999.0f`) — ya está así en `Types.h` y se reinicializa también
  en `generarEnemigoConJugador()`.
- **Ejecutable "stale":** el binario compilado sale a `build/ejecutable/windows/`, hay que
  copiarlo a `ejecutable/windows/` (o correr directo desde el output de build) antes de probar
  cambios — si no, se prueba una versión vieja sin darse cuenta.
- **Logs en build de Windows (subsystem):** `SDL_Log`/`fprintf` no se ven en la terminal MSYS2
  para builds de subsistema Windows; redirigir con `2>&1 > debug.txt` y leer después de cerrar
  el juego. (`main.cpp` ya escribe algo de esto a mano en `log.txt`.)
- **Aliasing de texturas:** `texJugador` es un alias de `texPlayerDown` (NO se carga por
  separado) para evitar fallos de validación por null; `limpiarRecursos()` solo debe destruir
  los 4 spritesheets independientes (`texPlayerRight/Left/Down/Up`), nunca `texJugador` aparte.
- **Orden de funciones en C++:** funciones usadas antes de su definición necesitan forward
  declaration o reordenar el archivo (pasó con `hitboxJugador()` antes de `verificarColision()`
  en `World.cpp`).

---

## 7. Convenciones de código observadas

- Todo en español (nombres de funciones, variables, comentarios). Mantener consistencia.
- Constantes de balance SIEMPRE en `Constants.h`, nunca hardcodeadas en la lógica (excepción
  actual: la tecla `C` de la chancla en `InputManager.cpp`, y algunos números mágicos de UI en
  las escenas).
- Coordenadas de UI casi siempre relativas a `VW(juego)`/`VH(juego)` (tamaño real de ventana),
  no a `ANCHO_VENTANA`/`ALTO_VENTANA` (que son la resolución de diseño base 1920x1080). Al tocar
  UI, seguir ese patrón para que escale bien con resoluciones distintas.
- Callbacks de "algo pasó" (`mundoOnX`) centralizan efectos secundarios (puntos, sonido, cambio
  de estado) y viven repartidos entre `World.cpp` (colisión/movimiento) y `ScoreManager.cpp`
  (puntaje/frases) — al añadir un nuevo evento de gameplay, seguir ese mismo patrón de callback.

---

## 8. Protocolo para futuras sesiones con IA

1. Mi ñema describe qué necesita.
2. La IA lee este archivo primero (evita releer todo el repo).
3. Si mi ñema dice **"actualiza"**: la IA revisa los commits recientes en
   https://github.com/ingNeva/esquiva-mond-antioque-a y actualiza las secciones 2 (árbol),
   4 (sistemas), 5 (pendientes) y 6 (aprendizajes) de este mismo archivo.
4. Si el cambio requiere ver un archivo puntual no resumido aquí en detalle línea por línea
   (ej. una escena completa), la IA lo pide o lo lee del repo — este documento es un mapa, no
   reemplaza el código fuente para cambios quirúrgicos.
5. Implementación: igual que siempre — pase completo integrando todos los archivos afectados,
   y entrega final vía archivos generados para reemplazo directo en el árbol del proyecto.
