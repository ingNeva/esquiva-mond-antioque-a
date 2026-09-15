# IA_DEVLOG — EsquivarBotellas / Esquiva Mondá Antioqueña

> **Propósito de este archivo:** que cualquier IA (o Claude en una sesión nueva) entienda el
> estado actual del código SIN tener que releer todo el repo. Cuando mi ñema diga
> **"actualiza"**, esta IA debe leer los commits recientes del repo
> (https://github.com/ingNeva/esquiva-mond-antioque-a) y actualizar las secciones que
> cambiaron: árbol de archivos, sistemas, pendientes y aprendizajes.
>
> ⚠️ **Estado de sincronía en la última auditoría:** el código descrito acá (fix de Machete,
> fix de `cmake/windows.cmake`, rediseño de ENEMIGO_TANQUE) **compila y corre bien en la
> máquina de mi ñema (Windows/MSYS2), pero todavía NO estaba commiteado/pusheado al repo
> remoto** en el momento de esta auditoría — el remoto seguía en el commit `9759ddc`
> (con el bug del machete y el tanque sin implementar). **Antes de seguir planeando nuevos
> elementos, hacer `git add` + `commit` + `push` de los archivos modificados**, o el próximo
> `git clone` de una sesión de IA va a traer el código viejo y este devlog va a quedar
> desincronizado del repo real.
>
> Última auditoría de código hecha sobre: fixes de Machete/windows.cmake + rediseño de
> ENEMIGO_TANQUE (post commit `9759ddc`, aún sin commitear).

---

## 1. Qué es el proyecto

Juego 2D de esquiva y supervivencia en C++ con **SDL3** (+ SDL3_image, SDL3_ttf, SDL3_mixer).
El jugador esquiva enemigos que entran desde los bordes, recoge un **machete** (ataque circular
orbital) y una **chancla** (arma bumerán, disponible desde el inicio), sube de nivel recogiendo
una **llave** por nivel, y en el nivel 5 enfrenta un **boss** con pilares destructibles. Hay
ranking Top-5 persistente, progreso de niveles persistente, configuración persistente
(resolución/audio/teclas), soporte de gamepad (PS3/PS4/Xbox), y un sistema de combo/multiplicador
de puntos con floating texts y frases colombianas al matar enemigos. Los enemigos son botellas de
licor estilizadas (aguardiente/ron), coherente con el nombre del juego.

- **Build real usado por mi ñema:** el `CMakeLists.txt` de la **raíz** (el mismo que Linux),
  vía `pkg-config` — en su MSYS2 UCRT64 sí están los `.pc` de SDL3 instalados, así que
  **no usa `cmake/windows.cmake`** en la práctica. Ese archivo queda como alternativa manual
  (hardcodea `C:/msys64/ucrt64`) para el caso de que pkg-config no esté disponible en otra
  máquina — se le corrigió el mismo bug de fuentes faltantes por consistencia, pero no es la
  ruta de build activa.
- **Flujo de compilación real (Windows, carpeta `build/` ya configurada una vez):**
  ```
  cd build
  cmake ..
  cmake --build .
  ```
  Configurado originalmente con `-G "MinGW Makefiles" -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=ejecutable/windows`.
- **Ejecutable:** sale a `build/ejecutable/windows/` (por el flag de arriba) — copiar
  manualmente (junto con las DLLs de `copy_dlls.ps1`) a `ejecutable/windows/` en la raíz del
  repo antes de correrlo, porque ahí es donde SDL_GetBasePath()/chdir espera encontrar
  `imagenes/`, `musica/` y `saves/` como carpetas hermanas.
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
                                TrofeoRecogido/PilarDestruido. hitboxJugador() reducida. Salta
                                colisión/esquive para enemigos ENEMIGO_TANQUE que están "explotando".
  InputManager.h/.cpp        → manejarEventos() (teclado+gamepad durante partida), actualizarJugador()
                                (movimiento + animación, usa KeyConfig remapeable)
  AudioManager.h/.cpp        → música por estado/nivel, mute, volumen

entities/
  Player.h/.cpp              → inicializarJugador(), animación, mostrarPuntuacionPantalla() (HUD nivel/llave/audio)
  Enemy.h/.cpp                → spawn de enemigos (probabilidades por nivel), movimiento por tipo
                                (zigzag, bombardero que se reproduce, espejo que persigue posición
                                simétrica, tanque que persigue directo y explota al morir),
                                floating texts, HUD de combo, iniciarExplosionTanque()
  Machete.h/.cpp              → arma cuerpo a cuerpo, ataque circular orbital con estela, cooldown,
                                recogida, animación con destello. Usa mundoOnEnemigoMuertoMachete
                                (frase + puntos correctos). Ignora enemigos ya "explotando".
  Chancla.h/.cpp              → arma bumerán: ida recta → vuelta homing al jugador, rotación visual,
                                colisión con enemigos y pilares del boss, cooldown propio. Ignora
                                enemigos ya "explotando".
  Boss.h/.cpp                 → pilares destructibles, disparo de proyectiles (1/2/3 según HP),
                                barra de vida, transición de nivel con fade entre fondos
  Llave.h/.cpp                → llave que aparece al alcanzar umbral de puntos del nivel, teletransporta
                                a posición aleatoria lejos del jugador, dispara transición de nivel

scenes/
  GameScene.h/.cpp            → dibujarJuego(): renderiza TODO el frame de juego (fondo, boss, pilares,
                                trofeo, jugador animado, enemigos —incl. parpadeo rojo del tanque
                                "explotando"—, machete, chancla, llave, HUD, overlays de nivel 2/3/4)
  MenuScene.h/.cpp            → menú principal
  CountdownScene.h/.cpp       → cuenta regresiva, instrucciones, intro cinemática (personaje camina
                                y recoge el machete)
  GameOverScene.h/.cpp        → game over, pausa, victoria, ingreso de nombre para Top5
  OptionsScene.h/.cpp         → resolución, pantalla completa, audio
  LevelSelectScene.h/.cpp     → selección de nivel con bloqueo/desbloqueo
  KeybindScene.h/.cpp         → remapeo de teclas (WASD/atacar/pausa)

utils/
  Types.h                     → TODOS los structs y enums del juego (Jugador, Enemigo —con campos
                                explotando/tiempoExplosion—, Machete, Chancla, Llave, Pilar,
                                FloatingText, TransicionNivel, KeyConfig, y el struct Juego
                                "god object" que contiene todo el estado global)
  Constants.h                 → todas las macros de balance (puntos, cooldowns, umbrales de nivel,
                                constantes de mecánicas por nivel, bloque TANQUE_*), enums
                                EstadoJuego/TipoEnemigo/EstadoBoss/EstadoPista, tabla de resoluciones
  ScoreManager.h/.cpp          → Top5 en binario, sistema de puntos+combo+multiplicador, floating
                                texts, frases colombianas (machete=dorado, chancla=cyan), callback
                                mundoOnEsquiveCercano (bonus por esquive cercano, incl. TANQUE)
  SaveManager.h                → progreso de niveles desbloqueados (saves/progreso.bin), todo inline
                                en el header

imagenes/, musica/            → assets (backgrounds por nivel, spritesheets del jugador, sprites de
                                enemigos por tipo —botellas de licor—, machete, chancla, llave,
                                trofeo, pilar, música .wav)
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
- **Patrón "enemigo inerte" (nuevo, con el tanque):** un enemigo puede quedar "muerto pero
  presente" (`en->explotando = true`, `en->vida = 0`) durante una ventana de tiempo, ignorado
  por colisión/esquive/re-hit, hasta que su lógica en `moverEnemigo()` decide qué pasa al
  cumplirse el timer. Reutilizable para futuros enemigos con secuencias de muerte diferidas.

---

## 4. Sistemas clave (comportamiento actual)

### 4.1 Enemigos (`Enemy.cpp`)
- Tipos: `ENEMIGO_BASICO`, `RAPIDO`, `ZIGZAG`, `BOMBARDERO` (se duplica cada 2s mientras vive),
  `ESPEJO` (persigue la posición simétrica del jugador respecto al centro de pantalla, no ataca
  directo), `TANQUE` (ver 4.1.1).
- Probabilidades de spawn por nivel (1 a 5), ahora incluyen TANQUE desde nivel 2:
  - Nivel 1: solo BASICO.
  - Nivel 2: TANQUE 15%, RAPIDO 35%, BASICO 50%.
  - Nivel 3: TANQUE 12%, ZIGZAG 25%, ESPEJO 20%, RAPIDO 23%, BASICO 20%.
  - Nivel 4: TANQUE 10%, BOMBARDERO 25%, ZIGZAG 20%, ESPEJO 20%, RAPIDO 15%, BASICO 10%.
  - Nivel 5: TANQUE 10%, ESPEJO 30%, BOMBARDERO 20%, ZIGZAG 20%, RAPIDO 20%.
- **Rayos de proximidad / esquive cercano** (en `World.cpp`): trackea `distMinAlcanzada` por
  enemigo dentro de `RADIO_BURBUJA_ESQUIVE`; cuenta esquive cuando se aleja
  `UMBRAL_ALEJAMIENTO` px desde el mínimo, o al salir del radio si llegó a estar muy cerca
  (`UMBRAL_SALIDA_ESQUIVE`). Da bonus de puntos según el tipo (`mundoOnEsquiveCercano`).

#### 4.1.1 ENEMIGO_TANQUE — rediseñado (antes era código muerto, nunca se generaba)
Diseño acordado con mi ñema: persigue al jugador y, al morir, explota liberando 2 básicos.
- **Persecución continua**: cada frame recalcula dirección hacia el jugador y avanza a
  `TANQUE_VELOCIDAD` (2.3, más lento que el jugador a 4; sube un poco en niveles 4-5). No usa
  velocidad de entrada por borde como los demás tipos — se excluye de `orientarHaciaJugador()`.
- **Resistencia**: `TANQUE_VIDA` = 3 golpes de machete o chancla para matarlo.
- **Tamaño**: `TANQUE_TAMANO` = 84px (más grande que el resto, se siente "pesado").
- **Secuencia de muerte**: al llegar a 0 vida, en vez de regenerarse de inmediato
  (`iniciarExplosionTanque()`) queda **inerte 2000ms** (`TANQUE_TIEMPO_EXPLOSION`):
  no colisiona con el jugador, no cuenta esquive, no se le puede volver a pegar (evita
  farmear puntos re-golpeándolo mientras espera). Parpadea en rojo (`GameScene.cpp`).
  Al cumplirse el timer, libera **2 ENEMIGO_BASICO** a los lados de su posición de muerte
  (con velocidad de salida aleatoria) y se regenera a sí mismo desde borde.
- **Puntos**: matar = `PTS_MATAR_TANQUE` (15), esquivar de cerca = `PTS_ESQUIVAR_TANQUE` (7).
- **Sprite**: usa `enemy_boss.png` (botella morada) — ya existía en el repo pero estaba
  huérfana porque antes el tipo nunca se generaba; ya no hay hack de "usar textura de espejo
  en nivel 5".

### 4.2 Machete (`Machete.cpp`)
- Aparece en el mapa a partir de nivel 4 (`aparecerMachete`), se recoge por colisión.
- Ataque circular orbital de 360° en `DURACION_ANIMACION_ATAQUE` (300ms), con estela de 7 copias
  semitransparentes (`renderizarMacheteGirando`) y destello en la fase de impacto.
- Cooldown `COOLDOWN_MACHETE` (2000ms), radio de golpe `RANGO_ATAQUE` (150px) desde el centro
  del jugador — golpea todos los enemigos y pilares del boss dentro de ese radio en un solo uso.
- ✅ **Corregido:** ahora llama a `mundoOnEnemigoMuertoMachete` (antes llamaba al callback
  genérico sin frase/puntos correctos por tipo). Ignora enemigos con `en->explotando == true`.

### 4.3 Chancla (`Chancla.cpp`)
- Arma bumerán: se lanza con **tecla `C` fija (hardcodeada, NO remapeable vía KeyConfig)** o
  botón `WEST` del gamepad, en la dirección que mira el jugador (`Jugador.direccion`).
  Disponible desde el arranque de la partida (no requiere recogerla como el machete).
- Fase ida: recta hasta `CHANCLA_MAX_DISTANCIA` (350px) → fase vuelta: homing hacia el jugador
  hasta `CHANCLA_RADIO_RECOGIDA` (30px), con rotación visual continua y estela de color
  (naranja=ida, cyan=vuelta).
- Cooldown propio `COOLDOWN_CHANCLA` (3500ms), independiente del machete.
- Colisión: `break` tras el primer golpe por frame; ignora enemigos con `en->explotando == true`.
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
- Puntos por tipo de enemigo ahora cubren los 5 tipos jugables (BASICO/RAPIDO/ZIGZAG/
  BOMBARDERO/ESPEJO/TANQUE) tanto en muerte por machete/chancla como en esquive cercano.

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

1. ✅ **RESUELTO** — Machete usa `mundoOnEnemigoMuertoMachete` (frase + puntos correctos).
2. **`imagenes/enemy_maxima.png` sigue sin existir** — el enemigo ESPEJO usa el sprite genérico
   `enemy.png` por fallback en `cargarTexturas()`. Ya se armó un prompt para Gemini (motivo
   plateado/espejo, mismo estilo fotográfico que los demás enemigos) pero no se ha confirmado
   si ya se generó y colocó el archivo — **verificar en la próxima sesión**.
3. ✅ **RESUELTO (rediseñado)** — `ENEMIGO_TANQUE` ya no es código muerto: persigue al jugador,
   aguanta 3 golpes, y al morir queda inerte 2s y explota en 2 básicos. Ver 4.1.1.
4. ✅ **RESUELTO** — `cmake/windows.cmake` ya incluye `entities/Chancla.cpp` y
   `scenes/KeybindScene.cpp`. (Nota: mi ñema en la práctica compila con el `CMakeLists.txt`
   raíz vía pkg-config, no con este archivo — ver sección 1).
5. **Chancla no es remapeable** — sigue en `SDLK_C` fijo en `InputManager.cpp`, fuera del sistema
   `KeyConfig`. Si se quiere consistencia, habría que añadir un campo `lanzarChancla` a
   `KeyConfig` y a `KeybindScene`.
6. **⚠️ Sincronía repo remoto pendiente** — los fixes de Machete/windows.cmake y el rediseño
   del tanque están validados (compilan y corren) en la máquina de mi ñema pero **no estaban
   commiteados al remoto** al momento de esta auditoría. Hacer commit+push antes de seguir.
7. **Código muerto menor** — `mundoOnEnemigoMuerto` (el callback genérico en `World.cpp`, sin
   sufijo de arma) quedó sin ningún caller tras el fix del punto 1. No rompe nada, se puede
   limpiar cuando convenga.

---

## 6. Aprendizajes / trampas ya conocidas (no repetir)

- **Símbolos duplicados en linker:** definir la misma función (ej. `mundoOnEsquiveCercano`) en
  más de un `.cpp` rompe el link. Cada función va en UNA sola unidad de traducción; su
  declaración va en el `.h` correspondiente.
- **Campos de struct sin inicializar:** campos como `distMinAlcanzada` en `Enemigo` deben tener
  valor por defecto explícito (`= 9999.0f`) — ya está así en `Types.h` y se reinicializa también
  en `generarEnemigoConJugador()`. Mismo criterio aplicado a los nuevos `explotando`/`tiempoExplosion`.
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
- **Estados "muerto pero presente" necesitan blindaje contra re-hit:** al introducir el patrón
  de enemigo inerte (tanque explotando), hubo que agregar explícitamente `if (en->explotando)
  continue;` en los loops de colisión de Machete y Chancla — si no, se puede re-triggerar el
  callback de muerte en cada frame mientras espera explotar (exploit de puntos infinitos).
- **CMakeLists.txt real vs. alternativo:** el repo tiene DOS configuraciones de CMake para
  Windows: la raíz (vía pkg-config, la que realmente usa mi ñema porque su MSYS2 la resuelve
  bien) y `cmake/windows.cmake` (hardcodeada, de respaldo). Un bug en una NO implica que afecte
  el build real — siempre confirmar cuál usa la persona antes de diagnosticar un "build roto".
- **No asumir que un fix local ya está en el repo remoto** — validar con `git log`/`git diff`
  contra GitHub antes de dar por sentado que un cambio "ya quedó". Los archivos que se entregan
  como descarga en esta plataforma NO se auto-commitean al repo de mi ñema.

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
- Enemigos con secuencias de muerte diferidas (patrón tanque) usan un flag booleano de estado
  (`explotando`) + timestamp objetivo (`tiempoExplosion`, con `SDL_GetTicks()`), y se excluyen
  explícitamente de: colisión con jugador, conteo de esquive, y re-hit de armas.

---

## 8. Protocolo para futuras sesiones con IA

1. Mi ñema describe qué necesita.
2. La IA lee este archivo primero (evita releer todo el repo).
3. Si mi ñema dice **"actualiza"**: la IA revisa los commits recientes en
   https://github.com/ingNeva/esquiva-mond-antioque-a y actualiza las secciones 2 (árbol),
   4 (sistemas), 5 (pendientes) y 6 (aprendizajes) de este mismo archivo. Si el remoto no
   refleja cambios locales ya validados, señalarlo explícitamente (ver sección 5, punto 6).
4. Si el cambio requiere ver un archivo puntual no resumido aquí en detalle línea por línea
   (ej. una escena completa), la IA lo pide o lo lee del repo — este documento es un mapa, no
   reemplaza el código fuente para cambios quirúrgicos.
5. Implementación: pase completo integrando todos los archivos afectados, entrega vía archivos
   generados para reemplazo directo en el árbol del proyecto, y recordatorio de hacer
   commit+push antes de cerrar la sesión.

---

## 9. Backlog de nuevos elementos (a definir)

> Sección abierta para planear features nuevas. Se completa a medida que se decide cada una;
> cada entrada debería terminar con: diseño acordado → archivos a tocar → estado (pendiente/
> en progreso/hecho).

- _(vacío por ahora — arrancar a planear acá)_
