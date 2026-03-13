#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <string>
#include <cmath>
#include <cstdio>
#ifdef _WIN32
    #include <windows.h>   // SetCurrentDirectoryA
#else
    #include <unistd.h>    // chdir
#endif      

// ============================================
// Constantes del juego
// ============================================
#define MAX_ENEMIGOS 50            // Número máximo de enemigos que pueden existir simultáneamente
#define ANCHO_VENTANA 1400         // Ancho de la ventana del juego en píxeles
#define ALTO_VENTANA 900           // Alto de la ventana del juego en píxeles
#define TAMANO_SPRITE 64           // Tamaño en píxeles de los sprites (jugador y enemigos)
#define COOLDOWN_MACHETE 2000      // Tiempo de espera entre ataques en milisegundos (2 segundos)
#define RANGO_ATAQUE 150           // Radio en píxeles del área de ataque del machete
#define OFFSET_MACHETE_X 40        // Desplazamiento horizontal del machete respecto al jugador
#define OFFSET_MACHETE_Y 20        // Desplazamiento vertical del machete respecto al jugador
#define BARRA_COOLDOWN_ANCHO 200   // Ancho de la barra de cooldown en píxeles
#define BARRA_COOLDOWN_ALTO 30     // Alto de la barra de cooldown en píxeles
#define BARRA_COOLDOWN_X 10        // Posición X de la barra (esquina inferior izquierda)
#define BARRA_COOLDOWN_Y (ALTO_VENTANA - 50)  // Posición Y (50 píxeles desde abajo)
#define DURACION_ANIMACION_ATAQUE 300  // Duración de la animación de ataque en milisegundos
#define RADIO_GIRO_MACHETE 50          // Radio del círculo que describe el machete al girar
#ifndef M_PI                           // M_PI: Constante matemática Pi (3.14159...)
#define M_PI 3.14159265358979323846    // Algunos compiladores no la definen por defecto
#endif

// ============================================
// ESTADOS DEL JUEGO
// ============================================
/**
 * Enum: EstadoJuego
 * Define los posibles estados en los que puede encontrarse el juego
 * en cada iteración del bucle principal.
 *
 * - ESTADO_MENU:          Pantalla principal con opciones Jugar, Instrucciones y Salir
 * - ESTADO_JUGANDO:       El juego está en curso, el jugador puede moverse y atacar
 * - ESTADO_INSTRUCCIONES: Pantalla que muestra los controles del teclado y del gamepad
 * - ESTADO_GAME_OVER:     El jugador colisionó con un enemigo; muestra puntuación final
 *
 * El estado se lee en el switch del game loop (main) para decidir
 * qué funciones de lógica y renderizado ejecutar en cada frame.
 */
enum EstadoJuego {
    ESTADO_MENU,
    ESTADO_JUGANDO,
    ESTADO_INSTRUCCIONES,
    ESTADO_GAME_OVER
};

// ============================================
// ESTRUCTURAS
// ============================================

/**
 * Estructura Jugador
 * Representa al personaje controlado por el usuario
 * - rect: Rectángulo SDL que define posición (x, y) y tamaño (w, h) del jugador
 * - velocidad: Cantidad de píxeles que se mueve el jugador por frame
 */
struct Jugador {
    SDL_Rect rect;
    int velocidad;
};

/**
 * Estructura Enemigo
 * Representa un obstáculo que se mueve automáticamente
 * - rect: Rectángulo SDL que define posición y tamaño del enemigo
 * - velX: Velocidad horizontal (positiva = derecha, negativa = izquierda)
 * - velY: Velocidad vertical (positiva = abajo, negativa = arriba)
 */
struct Enemigo {
    SDL_Rect rect;
    int velX;
    int velY;
};

/**
 * Estructura Machete
 * Representa el arma que el jugador puede recoger y usar
 * - rect:             Rectángulo SDL que define posición y tamaño del machete
 * - recogido:         Bandera que indica si el jugador ya recogió el machete
 * - activo:           Bandera que indica si el machete está siendo usado en este frame
 * - ultimoUso:        Timestamp del último ataque para controlar el cooldown
 * - animandoAtaque:   Bandera que indica si la animación de ataque está en progreso
 * - inicioAnimacion:  Timestamp del inicio de la animación de ataque
 * - anguloActual:     Ángulo actual de rotación del machete en grados (0-360)
 */
struct Machete {
    SDL_Rect rect;
    bool recogido;
    bool activo;
    Uint32 ultimoUso;
    bool animandoAtaque;
    Uint32 inicioAnimacion;
    float anguloActual;
};

/**
 * Estructura Juego
 * Contiene todos los elementos y el estado global del juego
 *
 * Recursos SDL:
 * - ventana:    Puntero a la ventana de SDL donde se dibuja el juego
 * - renderer:   Puntero al renderizador de SDL que dibuja gráficos
 * - texFondo:   Textura del fondo del escenario (fondo.png)
 * - texJugador: Textura (imagen) del sprite del jugador (player.png)
 * - texEnemigo: Textura (imagen) del sprite de los enemigos (enemy.png)
 * - texMachete: Textura (imagen) del sprite del machete (machete.png)
 * - fuente:     Fuente tipográfica TTF para mostrar texto en pantalla
 *
 * Entidades del juego:
 * - jugador:          Instancia de la estructura Jugador
 * - enemigos:         Array de MAX_ENEMIGOS enemigos
 * - enemigosActivos:  Cantidad actual de enemigos en juego
 * - machete:          Instancia de la estructura Machete
 * - macheteEquipado:  Indica si el jugador tiene el machete equipado
 * - macheteAparecido: Bandera que evita que el machete aparezca más de una vez por partida
 *
 * Estado del juego:
 * - puntuacion:             Puntos acumulados por el jugador
 * - ejecutando:             Bandera booleana que controla si el bucle del juego continúa
 * - estado:                 Estado actual del juego (menú, jugando, instrucciones, game over)
 * - opcionMenuSeleccionada: Índice de la opción resaltada en el menú (0 = Jugar, etc.)
 * - ultimoNivelDificultad:  Último nivel de dificultad aplicado; controla el spawn de enemigos
 *
 * Input:
 * - gamepad: Puntero al control de PS3/PS4/Xbox abierto con SDL_GameController
 *            Es nullptr si no hay ningún gamepad conectado
 */
struct Juego {
    SDL_Window*         ventana;
    SDL_Renderer*       renderer;
    SDL_Texture*        texFondo;
    SDL_Texture*        texJugador;
    SDL_Texture*        texEnemigo;
    SDL_Texture*        texMachete;
    TTF_Font*           fuente;
    Jugador             jugador;
    Enemigo             enemigos[MAX_ENEMIGOS];
    int                 enemigosActivos;
    int                 puntuacion;
    bool                ejecutando;
    Machete             machete;
    bool                macheteEquipado;
    EstadoJuego         estado;
    int                 opcionMenuSeleccionada;
    bool                macheteAparecido;
    int                 ultimoNivelDificultad;
    SDL_GameController* gamepad;
};

// ============================================
// DECLARACIÓN DE FUNCIONES
// ============================================
bool  inicializarSDL(Juego* juego);
bool  cargarTexturas(Juego* juego);
bool  cargarFuente(Juego* juego);
void  renderizarTexto(Juego* juego, const char* texto, int x, int y, SDL_Color color);
void  mostrarPuntuacionPantalla(Juego* juego);
void  inicializarJugador(Jugador* jugador);
void  inicializarMachete(Juego* juego);
void  aparecerMachete(Juego* juego);
void  verificarRecogidaMachete(Juego* juego);
void  usarMachete(Juego* juego);
float calcularProgresoCooldown(Juego* juego);
void  renderizarBarraCooldown(Juego* juego);
void  actualizarPosicionMacheteEquipado(Juego* juego);
void  inicializarEnemigos(Juego* juego);
void  generarEnemigo(Enemigo* enemigo);
void  manejarEventos(Juego* juego);
void  actualizarJugador(Jugador* jugador, SDL_GameController* gamepad);
void  actualizarEnemigos(Juego* juego);
bool  verificarColision(SDL_Rect* a, SDL_Rect* b);
void  renderizar(Juego* juego);
void  limpiarRecursos(Juego* juego);
void  actualizarAnimacionAtaque(Juego* juego);
void  calcularPosicionMacheteGirando(Juego* juego, int* posX, int* posY);
void  reiniciarJuego(Juego* juego);
void  renderizarMenu(Juego* juego);
void  manejarEventosMenu(Juego* juego);
void  renderizarInstrucciones(Juego* juego);
void  renderizarGameOver(Juego* juego);

// ============================================
// FUNCIÓN PRINCIPAL
// ============================================

/**
 * Función: main
 * Punto de entrada del programa
 *
 * Parámetros:
 * - argc: Número de argumentos de línea de comandos
 * - argv: Array de argumentos de línea de comandos
 *
 * Retorna:
 * - 0 si el programa terminó correctamente
 * - 1 si hubo errores durante la inicialización
 *
 * Funcionamiento:
 * 1. Inicializa el generador de números aleatorios con srand
 * 2. Crea e inicializa la estructura Juego con todos los campos en 0
 * 3. Establece el estado inicial en ESTADO_MENU
 * 4. Inicializa SDL y sus subsistemas (video, gamepad, image, ttf)
 * 5. Carga texturas e imágenes
 * 6. Carga la fuente tipográfica
 * 7. Inicializa jugador, enemigos y machete
 * 8. Ejecuta el bucle principal del juego mientras ejecutando sea true
 * 9. Al terminar, limpia recursos y espera Enter antes de cerrar
 *
 * GAME LOOP - Bucle principal (switch por estado):
 *
 * ESTADO_MENU:
 *   - manejarEventosMenu: procesa teclado, gamepad y mouse en el menú
 *   - renderizarMenu: dibuja el título y las opciones con el selector ">"
 *
 * ESTADO_INSTRUCCIONES:
 *   - renderizarInstrucciones: muestra controles; procesa ESC/B para volver
 *
 * ESTADO_JUGANDO (orden crítico para evitar glitches visuales):
 *   1. manejarEventos:               procesa ESPACIO, ESC, botones del gamepad
 *   2. actualizarJugador:            mueve al jugador por teclado o gamepad
 *   3. actualizarAnimacionAtaque:    calcula el ángulo de giro del machete
 *   4. actualizarPosicionMacheteEquipado: posiciona el machete (normal o girando)
 *   5. actualizarEnemigos:           mueve enemigos, detecta colisiones y puntuación
 *   6. Intercepción de game over:    si ejecutando quedó en false, cambia a ESTADO_GAME_OVER
 *   7. renderizar:                   dibuja todo en pantalla
 *
 * ESTADO_GAME_OVER:
 *   - renderizarGameOver: muestra puntuación final; procesa Enter/Cruz para volver al menú
 *
 * SDL_Delay(16) al final de cada iteración mantiene aproximadamente 60 FPS
 */
int main(int argc, char* argv[]) {
    // Log para diagnosticar qué falla al lanzar desde Steam
    FILE* log = fopen("log.txt", "w");
    if (log) {
        fprintf(log, "Iniciando...\n");
        fclose(log);
    }

    char* basePath = SDL_GetBasePath();
    if (basePath) {
        #ifdef _WIN32
            SetCurrentDirectoryA(basePath);
        #else
            chdir(basePath);
        #endif
        log = fopen("log.txt", "a");
        if (log) { fprintf(log, "BasePath: %s\n", basePath); fclose(log); }
        SDL_free(basePath);
    }

    srand(time(NULL));

    Juego juego = {0};
    juego.estado                 = ESTADO_MENU;
    juego.opcionMenuSeleccionada = 0;

    if (!inicializarSDL(&juego)) {
        log = fopen("log.txt", "a");
        if (log) { fprintf(log, "FALLO: inicializarSDL\n"); fclose(log); }
        return 1;
    }
    if (!cargarTexturas(&juego)) {
        log = fopen("log.txt", "a");
        if (log) { fprintf(log, "FALLO: cargarTexturas\n"); fclose(log); }
        limpiarRecursos(&juego); return 1;
    }
    if (!cargarFuente(&juego)) {
        log = fopen("log.txt", "a");
        if (log) { fprintf(log, "FALLO: cargarFuente\n"); fclose(log); }
        limpiarRecursos(&juego); return 1;
    }

    log = fopen("log.txt", "a");
    if (log) { fprintf(log, "Todo OK, entrando al game loop\n"); fclose(log); }

    inicializarJugador(&juego.jugador);
    inicializarEnemigos(&juego);
    inicializarMachete(&juego);
    juego.macheteAparecido      = false;
    juego.ultimoNivelDificultad = 0;
    juego.ejecutando            = true;

    while (juego.ejecutando) {
        switch (juego.estado) {
            case ESTADO_MENU:
                manejarEventosMenu(&juego);
                if (juego.ejecutando) renderizarMenu(&juego);
                break;
            case ESTADO_INSTRUCCIONES:
                renderizarInstrucciones(&juego);
                break;
            case ESTADO_JUGANDO:
                manejarEventos(&juego);
                actualizarJugador(&juego.jugador, juego.gamepad);
                actualizarAnimacionAtaque(&juego);
                actualizarPosicionMacheteEquipado(&juego);
                actualizarEnemigos(&juego);
                if (!juego.ejecutando) {
                    juego.ejecutando = true;
                    juego.estado     = ESTADO_GAME_OVER;
                }
                renderizar(&juego);
                break;
            case ESTADO_GAME_OVER:
                renderizarGameOver(&juego);
                break;
        }
        SDL_Delay(16);
    }

    limpiarRecursos(&juego);
    return 0;
}
// ============================================
// IMPLEMENTACIÓN DE FUNCIONES
// ============================================

/**
 * Función: inicializarSDL
 * Inicializa SDL y todos sus subsistemas necesarios, incluyendo el gamepad
 *
 * Parámetros:
 * - juego: Puntero a la estructura Juego
 *
 * Retorna:
 * - true si todo se inicializó correctamente
 * - false si hubo algún error
 *
 * Funcionamiento:
 * 1. Inicializa SDL con los subsistemas de video Y gamepad en una sola llamada
 *    (SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER).
 *    IMPORTANTE: SDL_Init debe llamarse una sola vez con todos los flags juntos;
 *    llamarlo dos veces puede causar comportamiento indefinido.
 * 2. Inicializa SDL_image para cargar archivos PNG
 * 3. Crea la ventana del juego centrada en pantalla
 * 4. Crea el renderer con aceleración por hardware
 * 5. Inicializa SDL_ttf para manejar fuentes
 * 6. Busca gamepads conectados con SDL_NumJoysticks() y abre el primero válido.
 *    Si no hay ninguno, muestra un aviso y continúa solo con teclado.
 * 7. Si cualquier paso falla, muestra el error y retorna false
 */
bool inicializarSDL(Juego* juego) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
        std::cout << "Error SDL: " << SDL_GetError() << "\n";
        return false;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cout << "Error SDL_image: " << IMG_GetError() << "\n";
        SDL_Quit();
        return false;
    }

    juego->ventana = SDL_CreateWindow("Esquivar Botellas",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        ANCHO_VENTANA, ALTO_VENTANA, SDL_WINDOW_SHOWN);

    if (!juego->ventana) {
        std::cout << "Error creando ventana: " << SDL_GetError() << "\n";
        return false;
    }

    juego->renderer = SDL_CreateRenderer(juego->ventana, -1,
        SDL_RENDERER_ACCELERATED);

    if (!juego->renderer) {
        std::cout << "Error creando renderer: " << SDL_GetError() << "\n";
        return false;
    }

    if (TTF_Init() < 0) {
        std::cout << "Error SDL_ttf: " << TTF_GetError() << "\n";
        return false;
    }

    // Buscar y conectar el primer gamepad disponible al iniciar
    juego->gamepad = nullptr;
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            juego->gamepad = SDL_GameControllerOpen(i);
            if (juego->gamepad) {
                std::cout << "Gamepad conectado: "
                          << SDL_GameControllerName(juego->gamepad) << "\n";
                break;
            }
        }
    }
    if (!juego->gamepad) {
        std::cout << "No se detectó gamepad. Usando solo teclado.\n";
    }

    return true;
}

/**
 * Función: cargarTexturas
 * Carga las imágenes PNG del fondo, jugador, enemigos y machete en memoria
 *
 * Parámetros:
 * - juego: Puntero a la estructura Juego
 *
 * Retorna:
 * - true si las texturas obligatorias se cargaron exitosamente
 * - false si alguna falló
 *
 * Funcionamiento:
 * 1. Carga "fondo.png"   como textura del fondo (puede ser nullptr sin error fatal)
 * 2. Carga "player.png"  como textura del jugador
 * 3. Carga "enemy.png"   como textura de enemigos
 * 4. Carga "machete.png" como textura del machete
 * 5. Verifica que jugador, enemigo y machete se cargaron; si alguno falla retorna false
 *
 * Nota: texFondo puede ser nullptr si no existe el archivo; en ese caso el fondo
 * se omite en renderizar() sin causar un crash.
 */
bool cargarTexturas(Juego* juego) {
    juego->texFondo   = IMG_LoadTexture(juego->renderer, "fondo.png");
    juego->texJugador = IMG_LoadTexture(juego->renderer, "player.png");
    juego->texEnemigo = IMG_LoadTexture(juego->renderer, "enemy.png");
    juego->texMachete = IMG_LoadTexture(juego->renderer, "machete.png");

    if (!juego->texJugador || !juego->texEnemigo || !juego->texMachete) {
        std::cout << "Error cargando imagenes: " << IMG_GetError() << "\n";
        return false;
    }
    return true;
}

/**
 * Función: cargarFuente
 * Carga un archivo de fuente TTF para renderizar texto en pantalla
 *
 * Parámetros:
 * - juego: Puntero a la estructura Juego donde se guardará la fuente
 *
 * Retorna:
 * - true si la fuente se cargó exitosamente
 * - false si hubo un error al cargar la fuente
 *
 * Funcionamiento:
 * 1. Intenta abrir el archivo "Arial Black.ttf" con tamaño 36
 * 2. Si falla, muestra el error y retorna false
 * 3. Si tiene éxito, guarda el puntero en juego->fuente y retorna true
 */
bool cargarFuente(Juego* juego) {
    juego->fuente = TTF_OpenFont("Arial Black.ttf", 36);
    if (!juego->fuente) {
        std::cout << "Error cargando fuente: " << TTF_GetError() << "\n";
        return false;
    }
    return true;
}

/**
 * Función: renderizarTexto
 * Dibuja texto en pantalla en una posición específica
 *
 * Parámetros:
 * - juego: Puntero a la estructura Juego (contiene renderer y fuente)
 * - texto: Cadena de caracteres a mostrar
 * - x:     Posición horizontal en píxeles
 * - y:     Posición vertical en píxeles
 * - color: Color SDL del texto (RGBA)
 *
 * Funcionamiento:
 * 1. Crea una superficie SDL con el texto renderizado usando TTF_RenderText_Solid
 * 2. Convierte la superficie en textura con SDL_CreateTextureFromSurface
 * 3. Define un rectángulo destino con la posición y el tamaño real del texto
 * 4. Copia la textura al renderer con SDL_RenderCopy
 * 5. Libera la textura y la superficie de memoria para evitar fugas
 */
void renderizarTexto(Juego* juego, const char* texto, int x, int y, SDL_Color color) {
    SDL_Surface* superficie = TTF_RenderText_Solid(juego->fuente, texto, color);
    if (!superficie) return;

    SDL_Texture* textura = SDL_CreateTextureFromSurface(juego->renderer, superficie);
    if (!textura) { SDL_FreeSurface(superficie); return; }

    SDL_Rect rectDestino = {x, y, superficie->w, superficie->h};
    SDL_RenderCopy(juego->renderer, textura, NULL, &rectDestino);

    SDL_DestroyTexture(textura);
    SDL_FreeSurface(superficie);
}

/**
 * Función: mostrarPuntuacionPantalla
 * Muestra la puntuación actual del jugador en la esquina superior izquierda
 *
 * Parámetros:
 * - juego: Puntero a la estructura Juego
 *
 * Funcionamiento:
 * 1. Define el color blanco (RGB: 255, 255, 255)
 * 2. Crea una cadena con formato "Score: [número]"
 * 3. Llama a renderizarTexto en posición (10, 10)
 */
void mostrarPuntuacionPantalla(Juego* juego) {
    SDL_Color colorBlanco = {255, 255, 255, 255};
    std::string textoScore = "Score: " + std::to_string(juego->puntuacion);
    renderizarTexto(juego, textoScore.c_str(), 10, 10, colorBlanco);
}

/**
 * Función: inicializarJugador
 * Establece la posición inicial y las propiedades del jugador
 *
 * Parámetros:
 * - jugador: Puntero a la estructura Jugador
 *
 * Funcionamiento:
 * 1. Calcula posición X centrada: (ANCHO_VENTANA - TAMANO_SPRITE) / 2
 * 2. Calcula posición Y centrada: (ALTO_VENTANA  - TAMANO_SPRITE) / 2
 * 3. Establece ancho y alto del sprite en TAMANO_SPRITE (64 píxeles)
 * 4. Establece velocidad de movimiento en 4 píxeles por frame
 */
void inicializarJugador(Jugador* jugador) {
    jugador->rect.x   = (ANCHO_VENTANA - TAMANO_SPRITE) / 2;
    jugador->rect.y   = (ALTO_VENTANA  - TAMANO_SPRITE) / 2;
    jugador->rect.w   = TAMANO_SPRITE;
    jugador->rect.h   = TAMANO_SPRITE;
    jugador->velocidad = 4;
}

/**
 * Función: inicializarMachete
 * Establece el estado inicial del machete al comenzar o reiniciar una partida
 *
 * Parámetros:
 * - juego: Puntero a la estructura Juego
 *
 * Funcionamiento:
 * 1. Marca el machete como NO recogido y NO activo
 * 2. Inicializa ultimoUso en 0 (el cooldown empieza completamente listo)
 * 3. Define el tamaño del sprite del machete en 64x64 píxeles
 * 4. Marca macheteEquipado como false
 * 5. Inicializa las variables de animación:
 *    - animandoAtaque = false (sin animación en curso)
 *    - inicioAnimacion = 0 (sin timestamp)
 *    - anguloActual = 0.0 (machete en posición inicial)
 *
 * Nota: Las coordenadas X e Y del machete se establecen en aparecerMachete(),
 * que se llama cuando la puntuación llega a 20.
 */
void inicializarMachete(Juego* juego) {
    juego->machete.recogido        = false;
    juego->machete.activo          = false;
    juego->machete.ultimoUso       = 0;
    juego->machete.rect.w          = TAMANO_SPRITE;
    juego->machete.rect.h          = TAMANO_SPRITE;
    juego->macheteEquipado         = false;
    juego->machete.animandoAtaque  = false;
    juego->machete.inicioAnimacion = 0;
    juego->machete.anguloActual    = 0.0f;
}

/**
 * Función: aparecerMachete
 * Hace que el machete aparezca en una posición aleatoria de la pantalla
 *
 * Parámetros:
 * - juego: Puntero a la estructura Juego
 *
 * Funcionamiento:
 * 1. Genera posición X aleatoria dentro de los límites de la pantalla
 * 2. Genera posición Y aleatoria dentro de los límites de la pantalla
 * 3. Establece el machete como NO recogido para que sea visible en el suelo
 *
 * Esta función es llamada desde actualizarEnemigos() cuando
 * juego->puntuacion >= 20 y macheteAparecido es false.
 */
void aparecerMachete(Juego* juego) {
    juego->machete.rect.x = rand() % (ANCHO_VENTANA - TAMANO_SPRITE);
    juego->machete.rect.y = rand() % (ALTO_VENTANA  - TAMANO_SPRITE);
    juego->machete.recogido = false;
}

/**
 * Función: verificarColision
 * Determina si dos rectángulos SDL se están superponiendo
 *
 * Parámetros:
 * - a: Puntero al primer rectángulo SDL
 * - b: Puntero al segundo rectángulo SDL
 *
 * Retorna:
 * - true si los rectángulos se intersectan
 * - false si no hay intersección
 *
 * Funcionamiento:
 * Utiliza SDL_HasIntersection, que verifica si hay solapamiento
 * en ambos ejes (X e Y) simultáneamente.
 */
bool verificarColision(SDL_Rect* a, SDL_Rect* b) {
    return SDL_HasIntersection(a, b);
}

/**
 * Función: verificarRecogidaMachete
 * Comprueba si el jugador está tocando el machete y lo recoge
 *
 * Parámetros:
 * - juego: Puntero a la estructura Juego
 *
 * Funcionamiento:
 * 1. Verifica que el machete NO esté ya recogido
 * 2. Verifica si hay colisión entre el rectángulo del jugador y el del machete
 * 3. Si ambas condiciones son verdaderas:
 *    - Marca el machete como recogido y como equipado
 *    - Reduce el sprite a 48x48 píxeles para que se vea proporcional al jugador
 *    - Muestra mensaje en consola indicando los controles de ataque
 *
 * Nota: El tamaño reducido (48x48) hace que el machete equipado se vea
 * como si el jugador lo estuviera sosteniendo en la mano.
 */
void verificarRecogidaMachete(Juego* juego) {
    if (!juego->machete.recogido) {
        if (verificarColision(&juego->jugador.rect, &juego->machete.rect)) {
            juego->machete.recogido = true;
            juego->macheteEquipado  = true;
            juego->machete.rect.w   = 48;
            juego->machete.rect.h   = 48;
            std::cout << "\n¡Machete equipado! ESPACIO o X(PS3) para atacar.\n";
        }
    }
}

/**
 * Función: usarMachete
 * Activa el ataque del machete, inicia la animación y destruye enemigos cercanos
 *
 * Parámetros:
 * - juego: Puntero a la estructura Juego
 *
 * Funcionamiento:
 * 1. Obtiene el tiempo actual en milisegundos con SDL_GetTicks
 * 2. Calcula el tiempo transcurrido desde el último uso
 * 3. Si pasaron más de COOLDOWN_MACHETE (2000ms):
 *    a) Marca el machete como activo (para efectos visuales de un frame)
 *    b) Actualiza ultimoUso con el tiempo actual (reinicia el cooldown)
 *    c) Inicia la animación circular: animandoAtaque = true, guarda timestamp,
 *       resetea anguloActual a 0
 *    d) Calcula el centro del jugador como punto de origen del ataque
 *    e) Itera sobre todos los enemigos activos:
 *       - Calcula el centro de cada enemigo
 *       - Calcula la distancia al jugador usando el teorema de Pitágoras
 *       - Si la distancia <= RANGO_ATAQUE: suma 1 punto y regenera el enemigo
 *    f) Muestra en consola cuántos enemigos fueron destruidos
 * 4. Si el cooldown no ha terminado, la función no hace nada
 */
void usarMachete(Juego* juego) {
    Uint32 tiempoActual       = SDL_GetTicks();
    Uint32 tiempoTranscurrido = tiempoActual - juego->machete.ultimoUso;

    if (tiempoTranscurrido >= COOLDOWN_MACHETE) {
        juego->machete.activo          = true;
        juego->machete.ultimoUso       = tiempoActual;
        juego->machete.animandoAtaque  = true;
        juego->machete.inicioAnimacion = tiempoActual;
        juego->machete.anguloActual    = 0.0f;

        int centroJugadorX     = juego->jugador.rect.x + TAMANO_SPRITE / 2;
        int centroJugadorY     = juego->jugador.rect.y + TAMANO_SPRITE / 2;
        int enemigosDestruidos = 0;

        for (int i = 0; i < juego->enemigosActivos; i++) {
            Enemigo* enemigo = &juego->enemigos[i];
            int deltaX = (enemigo->rect.x + TAMANO_SPRITE / 2) - centroJugadorX;
            int deltaY = (enemigo->rect.y + TAMANO_SPRITE / 2) - centroJugadorY;
            float distancia = sqrt((float)(deltaX * deltaX + deltaY * deltaY));

            if (distancia <= RANGO_ATAQUE) {
                juego->puntuacion++;
                generarEnemigo(enemigo);
                enemigosDestruidos++;
            }
        }

        if (enemigosDestruidos > 0)
            std::cout << "¡Machete usado! " << enemigosDestruidos << " enemigos destruidos.\n";
    }
}

/**
 * Función: calcularProgresoCooldown
 * Calcula el porcentaje de cooldown completado del machete
 *
 * Parámetros:
 * - juego: Puntero a la estructura Juego
 *
 * Retorna:
 * - float entre 0.0 y 1.0 representando el progreso del cooldown
 *   * 0.0 = recién usado (0% listo)
 *   * 1.0 = completamente listo para usar (100%)
 *
 * Funcionamiento:
 * 1. Si ultimoUso == 0 (nunca usado), retorna 1.0 directamente
 * 2. Calcula el tiempo transcurrido desde el último uso
 * 3. Si el tiempo >= COOLDOWN_MACHETE, retorna 1.0 (listo)
 * 4. Divide el tiempo transcurrido entre el cooldown total para obtener
 *    un valor proporcional entre 0.0 y 1.0
 *
 * Ejemplo:
 * - Si COOLDOWN = 2000ms y han pasado 1000ms → retorna 0.5 (50%)
 * - Si han pasado 500ms → retorna 0.25 (25%)
 */
float calcularProgresoCooldown(Juego* juego) {
    if (juego->machete.ultimoUso == 0) return 1.0f;

    Uint32 transcurrido = SDL_GetTicks() - juego->machete.ultimoUso;
    if (transcurrido >= COOLDOWN_MACHETE) return 1.0f;

    return (float)transcurrido / (float)COOLDOWN_MACHETE;
}

/**
 * Función: renderizarBarraCooldown
 * Dibuja una barra visual que muestra el estado del cooldown del machete
 *
 * Parámetros:
 * - juego: Puntero a la estructura Juego
 *
 * Funcionamiento:
 * 1. Solo se dibuja si el machete está equipado (macheteEquipado == true)
 * 2. Calcula el progreso actual del cooldown (0.0 a 1.0)
 * 3. Dibuja un borde negro alrededor de la barra (2 píxeles de margen)
 * 4. Dibuja el fondo interior en gris oscuro (50, 50, 50)
 * 5. Dibuja el relleno proporcional al progreso:
 *    - Rojo  (255, 0, 0)  si el machete está en cooldown (progreso < 1.0)
 *    - Verde (0, 255, 0)  si el machete está listo para usar (progreso = 1.0)
 * 6. Dibuja un mini icono del machete (32x32) a la derecha de la barra
 * 7. Dibuja texto de estado debajo de la barra:
 *    - "MACHETE LISTO" en verde si está disponible
 *    - "MACHETE: X.Xs" en blanco con el tiempo restante si está en cooldown
 */
void renderizarBarraCooldown(Juego* juego) {
    if (!juego->macheteEquipado) return;

    float progreso = calcularProgresoCooldown(juego);

    // Borde negro
    SDL_Rect fondoBarra = { BARRA_COOLDOWN_X - 2, BARRA_COOLDOWN_Y - 2,
                            BARRA_COOLDOWN_ANCHO + 4, BARRA_COOLDOWN_ALTO + 4 };
    SDL_SetRenderDrawColor(juego->renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(juego->renderer, &fondoBarra);

    // Fondo gris oscuro
    SDL_Rect barraFondo = { BARRA_COOLDOWN_X, BARRA_COOLDOWN_Y,
                            BARRA_COOLDOWN_ANCHO, BARRA_COOLDOWN_ALTO };
    SDL_SetRenderDrawColor(juego->renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(juego->renderer, &barraFondo);

    // Relleno de progreso (rojo = en cooldown, verde = listo)
    SDL_Rect barraProgreso = { BARRA_COOLDOWN_X, BARRA_COOLDOWN_Y,
                               (int)(BARRA_COOLDOWN_ANCHO * progreso), BARRA_COOLDOWN_ALTO };
    if (progreso >= 1.0f)
        SDL_SetRenderDrawColor(juego->renderer, 0, 255, 0, 255);
    else
        SDL_SetRenderDrawColor(juego->renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(juego->renderer, &barraProgreso);

    // Mini icono del machete a la derecha de la barra
    SDL_Rect iconoMachete = { BARRA_COOLDOWN_X + BARRA_COOLDOWN_ANCHO + 10,
                              BARRA_COOLDOWN_Y, 32, 32 };
    SDL_RenderCopy(juego->renderer, juego->texMachete, NULL, &iconoMachete);

    // Texto de estado debajo de la barra
    SDL_Color colorTexto;
    std::string textoEstado;

    if (progreso >= 1.0f) {
        colorTexto  = {0, 255, 0, 255};
        textoEstado = "MACHETE LISTO";
    } else {
        colorTexto = {255, 255, 255, 255};
        float restante = (COOLDOWN_MACHETE - (SDL_GetTicks() - juego->machete.ultimoUso)) / 1000.0f;
        char buffer[32];
        sprintf(buffer, "MACHETE: %.1fs", restante);
        textoEstado = buffer;
    }

    renderizarTexto(juego, textoEstado.c_str(),
        BARRA_COOLDOWN_X, BARRA_COOLDOWN_Y + BARRA_COOLDOWN_ALTO + 5, colorTexto);
}

/**
 * Función: calcularPosicionMacheteGirando
 * Calcula la posición X,Y del machete durante la animación de giro circular
 *
 * Parámetros:
 * - juego: Puntero a la estructura Juego
 * - posX:  Puntero donde se guardará la posición X calculada
 * - posY:  Puntero donde se guardará la posición Y calculada
 *
 * Funcionamiento:
 * 1. Calcula el centro del jugador sumando la mitad de TAMANO_SPRITE a su rect
 * 2. Convierte anguloActual de grados a radianes (ángulo * PI / 180)
 * 3. Aplica trigonometría circular para obtener la posición:
 *    - posX = centroJugador + cos(ángulo) * RADIO_GIRO_MACHETE
 *    - posY = centroJugador + sin(ángulo) * RADIO_GIRO_MACHETE
 * 4. Resta 24 píxeles (mitad de 48, el tamaño del machete equipado)
 *    para centrar el sprite del machete sobre el punto calculado
 *
 * Matemática:
 * - cos(ángulo) da la componente horizontal del círculo (rango -1 a 1)
 * - sin(ángulo) da la componente vertical del círculo  (rango -1 a 1)
 * - Multiplicar por RADIO_GIRO_MACHETE escala al tamaño visual deseado
 */
void calcularPosicionMacheteGirando(Juego* juego, int* posX, int* posY) {
    int centroX     = juego->jugador.rect.x + TAMANO_SPRITE / 2;
    int centroY     = juego->jugador.rect.y + TAMANO_SPRITE / 2;
    float anguloRad = juego->machete.anguloActual * (float)M_PI / 180.0f;

    *posX = centroX + (int)(cos(anguloRad) * RADIO_GIRO_MACHETE) - 24;
    *posY = centroY + (int)(sin(anguloRad) * RADIO_GIRO_MACHETE) - 24;
}

/**
 * Función: actualizarPosicionMacheteEquipado
 * Actualiza la posición del machete para que siga al jugador cuando está equipado
 *
 * Parámetros:
 * - juego: Puntero a la estructura Juego
 *
 * Funcionamiento:
 * 1. Retorna inmediatamente si el machete no está equipado
 * 2. Si está en animación de ataque (animandoAtaque == true):
 *    - Llama a calcularPosicionMacheteGirando para obtener la posición circular
 *    - Asigna esa posición al rect del machete
 * 3. Si NO está en animación (estado de reposo):
 *    - Posiciona el machete al lado derecho del jugador usando los offsets fijos
 *      (OFFSET_MACHETE_X, OFFSET_MACHETE_Y)
 *
 * Nota: Esta función debe llamarse DESPUÉS de actualizarJugador y ANTES
 * de actualizarEnemigos para evitar desfase de un frame en la posición.
 */
void actualizarPosicionMacheteEquipado(Juego* juego) {
    if (!juego->macheteEquipado) return;

    if (juego->machete.animandoAtaque) {
        int posX, posY;
        calcularPosicionMacheteGirando(juego, &posX, &posY);
        juego->machete.rect.x = posX;
        juego->machete.rect.y = posY;
    } else {
        juego->machete.rect.x = juego->jugador.rect.x + OFFSET_MACHETE_X;
        juego->machete.rect.y = juego->jugador.rect.y + OFFSET_MACHETE_Y;
    }
}

/**
 * Función: inicializarEnemigos
 * Configura el estado inicial del sistema de enemigos al comenzar o reiniciar
 *
 * Parámetros:
 * - juego: Puntero a la estructura Juego
 *
 * Funcionamiento:
 * 1. Establece enemigosActivos en 1 (la partida comienza con un solo enemigo)
 * 2. Inicializa la puntuación en 0
 * 3. Genera el primer enemigo llamando a generarEnemigo
 */
void inicializarEnemigos(Juego* juego) {
    juego->enemigosActivos = 1;
    juego->puntuacion      = 0;
    generarEnemigo(&juego->enemigos[0]);
}

/**
 * Función: actualizarAnimacionAtaque
 * Actualiza el ángulo de rotación del machete durante la animación de ataque
 *
 * Parámetros:
 * - juego: Puntero a la estructura Juego
 *
 * Funcionamiento:
 * 1. Retorna inmediatamente si no hay animación en progreso
 * 2. Calcula el tiempo transcurrido desde el inicio de la animación
 * 3. Si el tiempo >= DURACION_ANIMACION_ATAQUE (300ms):
 *    - Detiene la animación: animandoAtaque = false
 *    - Resetea el ángulo a 0 (posición inicial)
 * 4. Si la animación continúa:
 *    - Calcula el progreso como fracción del tiempo (0.0 a 1.0)
 *    - Calcula el ángulo: progreso * 360 grados
 *    - El machete completa una vuelta completa en DURACION_ANIMACION_ATAQUE ms
 *
 * La rotación es en sentido horario (ángulo creciente en sistema SDL).
 */
void actualizarAnimacionAtaque(Juego* juego) {
    if (!juego->machete.animandoAtaque) return;

    Uint32 transcurrido = SDL_GetTicks() - juego->machete.inicioAnimacion;

    if (transcurrido >= DURACION_ANIMACION_ATAQUE) {
        juego->machete.animandoAtaque = false;
        juego->machete.anguloActual   = 0.0f;
    } else {
        float progreso = (float)transcurrido / (float)DURACION_ANIMACION_ATAQUE;
        juego->machete.anguloActual = progreso * 360.0f;
    }
}

/**
 * Función: generarEnemigo
 * Crea un enemigo en un borde aleatorio de la pantalla con dirección hacia dentro
 *
 * Parámetros:
 * - enemigo: Puntero a la estructura Enemigo a generar
 *
 * Funcionamiento:
 * 1. Genera un número aleatorio 0-3 para determinar el lado de aparición
 * 2. Establece el tamaño del enemigo en TAMANO_SPRITE x TAMANO_SPRITE (64x64)
 * 3. Según el lado elegido:
 *    - lado 0 (izquierda): aparece fuera por la izquierda, se mueve a la derecha  (velX=+5)
 *    - lado 1 (derecha):   aparece fuera por la derecha,   se mueve a la izquierda (velX=-5)
 *    - lado 2 (arriba):    aparece fuera por arriba,       se mueve hacia abajo    (velY=+5)
 *    - lado 3 (abajo):     aparece fuera por abajo,        se mueve hacia arriba   (velY=-5)
 * 4. La posición perpendicular al movimiento es aleatoria dentro de los límites de pantalla
 * 5. La velocidad es fija en 5 píxeles por frame en la dirección del movimiento
 */
void generarEnemigo(Enemigo* enemigo) {
    int lado = rand() % 4;
    enemigo->rect.w = TAMANO_SPRITE;
    enemigo->rect.h = TAMANO_SPRITE;

    if (lado == 0) {        // Izquierda → derecha
        enemigo->rect.x = -TAMANO_SPRITE;
        enemigo->rect.y = rand() % (ALTO_VENTANA - TAMANO_SPRITE);
        enemigo->velX = 5;  enemigo->velY = 0;
    } else if (lado == 1) { // Derecha → izquierda
        enemigo->rect.x = ANCHO_VENTANA;
        enemigo->rect.y = rand() % (ALTO_VENTANA - TAMANO_SPRITE);
        enemigo->velX = -5; enemigo->velY = 0;
    } else if (lado == 2) { // Arriba → abajo
        enemigo->rect.x = rand() % (ANCHO_VENTANA - TAMANO_SPRITE);
        enemigo->rect.y = -TAMANO_SPRITE;
        enemigo->velX = 0;  enemigo->velY = 5;
    } else {                // Abajo → arriba
        enemigo->rect.x = rand() % (ANCHO_VENTANA - TAMANO_SPRITE);
        enemigo->rect.y = ALTO_VENTANA;
        enemigo->velX = 0;  enemigo->velY = -5;
    }
}

/**
 * Función: manejarEventos
 * Procesa todos los eventos de entrada del sistema durante el juego activo
 *
 * Parámetros:
 * - juego: Puntero a la estructura Juego
 *
 * Funcionamiento:
 * 1. Itera sobre todos los eventos pendientes en la cola de SDL
 * 2. SDL_QUIT (usuario cierra la ventana): pone ejecutando = false
 * 3. SDL_KEYDOWN (tecla presionada):
 *    - ESPACIO: llama a usarMachete si el machete está equipado
 *    - ESC:     regresa al menú principal (estado = ESTADO_MENU)
 * 4. SDL_CONTROLLERBUTTONDOWN (botón del gamepad presionado):
 *    - BUTTON_A (Cruz PS3):    llama a usarMachete si el machete está equipado
 *    - BUTTON_START:           regresa al menú principal
 * 5. SDL_CONTROLLERDEVICEADDED (gamepad conectado en caliente):
 *    - Si no hay gamepad activo, abre el nuevo dispositivo
 * 6. SDL_CONTROLLERDEVICEREMOVED (gamepad desconectado):
 *    - Cierra el gamepad y pone el puntero en nullptr
 *    - El juego continúa funcionando con solo teclado
 *
 * Nota sobre el control PS3 en SDL2:
 * El botón Cruz (X) del DualShock 3 es mapeado por SDL2 como
 * SDL_CONTROLLER_BUTTON_A, que es el botón de acción principal.
 */
void manejarEventos(Juego* juego) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            juego->ejecutando = false;
        }

        if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_SPACE && juego->macheteEquipado)
                usarMachete(juego);
            if (e.key.keysym.sym == SDLK_ESCAPE)
                juego->estado = ESTADO_MENU;
        }

        if (e.type == SDL_CONTROLLERBUTTONDOWN) {
            // Cruz (X) PS3 = BUTTON_A en SDL → atacar con el machete
            if (e.cbutton.button == SDL_CONTROLLER_BUTTON_A && juego->macheteEquipado)
                usarMachete(juego);
            // START → volver al menú principal
            if (e.cbutton.button == SDL_CONTROLLER_BUTTON_START)
                juego->estado = ESTADO_MENU;
        }

        // Hot-plug: gamepad conectado en caliente durante la partida
        if (e.type == SDL_CONTROLLERDEVICEADDED && !juego->gamepad) {
            juego->gamepad = SDL_GameControllerOpen(e.cdevice.which);
            if (juego->gamepad)
                std::cout << "Gamepad conectado: "
                          << SDL_GameControllerName(juego->gamepad) << "\n";
        }

        // Hot-plug: gamepad desconectado durante la partida
        if (e.type == SDL_CONTROLLERDEVICEREMOVED) {
            std::cout << "Gamepad desconectado.\n";
            if (juego->gamepad) {
                SDL_GameControllerClose(juego->gamepad);
                juego->gamepad = nullptr;
            }
        }
    }
}

/**
 * Función: actualizarJugador
 * Actualiza la posición del jugador según las teclas del teclado o el gamepad
 *
 * Parámetros:
 * - jugador: Puntero a la estructura Jugador
 * - gamepad: Puntero al gamepad abierto (puede ser nullptr si no hay gamepad)
 *
 * Funcionamiento:
 * 1. TECLADO — Lee el estado actual con SDL_GetKeyboardState:
 *    - W: mueve arriba (resta Y)
 *    - S: mueve abajo  (suma Y)
 *    - A: mueve izquierda (resta X)
 *    - D: mueve derecha   (suma X)
 *
 * 2. GAMEPAD — Solo si el puntero gamepad no es nullptr:
 *    a) Stick analógico izquierdo:
 *       - Lee los ejes LEFTX y LEFTY con SDL_GameControllerGetAxis
 *       - Aplica una zona muerta (DEADZONE = 8000) para ignorar el drift
 *         natural del joystick del PS3. El rango completo es -32768 a 32767.
 *       - Si el valor supera la deadzone en cualquier dirección, mueve el jugador
 *    b) D-pad (cruceta digital):
 *       - Lee cada botón de dirección con SDL_GameControllerGetButton
 *       - Permite movimiento digital preciso como alternativa al stick
 *
 * 3. LÍMITES — Después de mover, restringe la posición dentro de la pantalla:
 *    - Límite izquierdo:  X >= 0
 *    - Límite derecho:    X <= ANCHO_VENTANA - TAMANO_SPRITE
 *    - Límite superior:   Y >= 0
 *    - Límite inferior:   Y <= ALTO_VENTANA  - TAMANO_SPRITE
 *
 * Nota: El teclado y el gamepad pueden usarse simultáneamente sin conflictos.
 */
void actualizarJugador(Jugador* jugador, SDL_GameController* gamepad) {
    const Uint8* teclas = SDL_GetKeyboardState(NULL);

    if (teclas[SDL_SCANCODE_W]) jugador->rect.y -= jugador->velocidad;
    if (teclas[SDL_SCANCODE_S]) jugador->rect.y += jugador->velocidad;
    if (teclas[SDL_SCANCODE_A]) jugador->rect.x -= jugador->velocidad;
    if (teclas[SDL_SCANCODE_D]) jugador->rect.x += jugador->velocidad;

    if (gamepad) {
        const int DEADZONE = 8000;  // Umbral para ignorar el drift del stick del PS3

        int axisX = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_LEFTX);
        int axisY = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_LEFTY);

        if (axisX >  DEADZONE) jugador->rect.x += jugador->velocidad;
        if (axisX < -DEADZONE) jugador->rect.x -= jugador->velocidad;
        if (axisY >  DEADZONE) jugador->rect.y += jugador->velocidad;
        if (axisY < -DEADZONE) jugador->rect.y -= jugador->velocidad;

        if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_DPAD_UP))
            jugador->rect.y -= jugador->velocidad;
        if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_DPAD_DOWN))
            jugador->rect.y += jugador->velocidad;
        if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_DPAD_LEFT))
            jugador->rect.x -= jugador->velocidad;
        if (SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_DPAD_RIGHT))
            jugador->rect.x += jugador->velocidad;
    }

    if (jugador->rect.x < 0) jugador->rect.x = 0;
    if (jugador->rect.x > ANCHO_VENTANA - TAMANO_SPRITE)
        jugador->rect.x = ANCHO_VENTANA - TAMANO_SPRITE;
    if (jugador->rect.y < 0) jugador->rect.y = 0;
    if (jugador->rect.y > ALTO_VENTANA - TAMANO_SPRITE)
        jugador->rect.y = ALTO_VENTANA - TAMANO_SPRITE;
}

/**
 * Función: actualizarEnemigos
 * Actualiza la posición de todos los enemigos activos y maneja colisiones,
 * puntuación, dificultad progresiva y aparición del machete
 *
 * Parámetros:
 * - juego: Puntero a la estructura Juego
 *
 * Funcionamiento:
 * 1. DIFICULTAD — Cada 5 puntos se añade un enemigo nuevo al campo:
 *    - Calcula nivelActual = puntuacion / 5
 *    - Si es mayor que ultimoNivelDificultad y hay espacio en el array,
 *      genera un nuevo enemigo y aumenta enemigosActivos
 *    - Actualiza ultimoNivelDificultad para no repetir el spawn
 *
 * 2. MACHETE — Al llegar a 20 puntos aparece el machete en el mapa:
 *    - Solo si macheteAparecido == false y el machete no fue recogido aún
 *    - Llama a aparecerMachete() y marca macheteAparecido = true
 *
 * 3. MOVIMIENTO Y COLISIONES — Itera sobre todos los enemigos activos:
 *    a) Mueve cada enemigo sumando velX y velY a su posición
 *    b) Si el enemigo salió de la pantalla (margen de 70 píxeles):
 *       - Suma 1 punto y regenera el enemigo en un borde aleatorio
 *    c) Si hay colisión con el jugador:
 *       - Muestra el mensaje de fin de juego en consola
 *       - Pone ejecutando = false y hace return inmediato para evitar
 *         accesos a memoria inválida en el mismo frame
 *
 * 4. RECOGER MACHETE — Llama a verificarRecogidaMachete al final del bucle
 *
 * 5. DESACTIVAR EFECTO — Si machete.activo está en true, lo pone en false
 *    (el efecto visual de ataque solo dura un frame)
 */
void actualizarEnemigos(Juego* juego) {
    // Aumentar dificultad cada 5 puntos
    int nivelActual = juego->puntuacion / 5;
    if (nivelActual > juego->ultimoNivelDificultad &&
        juego->enemigosActivos < MAX_ENEMIGOS) {
        generarEnemigo(&juego->enemigos[juego->enemigosActivos]);
        juego->enemigosActivos++;
        juego->ultimoNivelDificultad = nivelActual;
    }

    // Hacer aparecer el machete al llegar a 20 puntos
    if (juego->puntuacion >= 20 && !juego->macheteAparecido && !juego->machete.recogido) {
        aparecerMachete(juego);
        juego->macheteAparecido = true;
        std::cout << "\n¡Ha aparecido un MACHETE en el mapa!\n";
    }

    // Mover enemigos y detectar colisiones
    for (int i = 0; i < juego->enemigosActivos; i++) {
        Enemigo* enemigo = &juego->enemigos[i];

        enemigo->rect.x += enemigo->velX;
        enemigo->rect.y += enemigo->velY;

        // Salió de la pantalla → sumar punto y regenerar
        if (enemigo->rect.x < -70 || enemigo->rect.x > ANCHO_VENTANA + 20 ||
            enemigo->rect.y < -70 || enemigo->rect.y > ALTO_VENTANA  + 20) {
            juego->puntuacion++;
            generarEnemigo(enemigo);
        }

        // Colisión con el jugador → fin de la partida
        if (verificarColision(&juego->jugador.rect, &enemigo->rect)) {
            std::cout << "\nCOLISION - Juego terminado.\n";
            std::cout << "Puntuacion final: " << juego->puntuacion << "\n";
            juego->ejecutando = false;
            return;  // Salir inmediatamente para evitar accesos inválidos
        }
    }

    // Verificar si el jugador recoge el machete
    verificarRecogidaMachete(juego);

    // El efecto visual de ataque solo dura un frame
    if (juego->machete.activo)
        juego->machete.activo = false;
}

/**
 * Función: renderizar
 * Dibuja todos los elementos visuales del juego en pantalla durante ESTADO_JUGANDO
 *
 * Parámetros:
 * - juego: Puntero a la estructura Juego
 *
 * Orden de renderizado (de fondo a frente):
 * 1. SDL_RenderClear: limpia el buffer con el color de fondo
 * 2. texFondo:        dibuja la imagen de fondo si está cargada
 * 3. texJugador:      dibuja el sprite del jugador en su posición actual
 * 4. texEnemigo:      dibuja todos los enemigos activos
 * 5. Machete:
 *    - Si puntuacion >= 20 o el machete está equipado:
 *      * Si no está recogido: dibuja el machete en el suelo
 *      * Si está equipado:    dibuja el machete siguiendo al jugador
 * 6. Efecto de ataque (si machete.activo o machete.animandoAtaque):
 *    - Dibuja un rectángulo rojo que muestra el rango de ataque (RANGO_ATAQUE)
 *    - Durante la animación dibuja además una línea amarilla del centro
 *      del jugador al centro del machete (rastro visual del giro)
 * 7. Puntuación:      llama a mostrarPuntuacionPantalla (esquina superior izquierda)
 * 8. Barra cooldown:  llama a renderizarBarraCooldown (esquina inferior izquierda)
 * 9. SDL_RenderPresent: muestra en pantalla todo lo dibujado en el buffer
 */
void renderizar(Juego* juego) {
    SDL_RenderClear(juego->renderer);

    if (juego->texFondo)
        SDL_RenderCopy(juego->renderer, juego->texFondo, NULL, NULL);

    SDL_RenderCopy(juego->renderer, juego->texJugador, NULL, &juego->jugador.rect);

    for (int i = 0; i < juego->enemigosActivos; i++)
        SDL_RenderCopy(juego->renderer, juego->texEnemigo, NULL, &juego->enemigos[i].rect);

    // Renderizar machete en el suelo o equipado
    if (juego->puntuacion >= 20 || juego->macheteEquipado) {
        if (!juego->machete.recogido || juego->macheteEquipado)
            SDL_RenderCopy(juego->renderer, juego->texMachete, NULL, &juego->machete.rect);
    }

    // Efecto visual de ataque: rectángulo de rango + línea de rastro
    if (juego->machete.activo || juego->machete.animandoAtaque) {
        int centroX = juego->jugador.rect.x + TAMANO_SPRITE / 2;
        int centroY = juego->jugador.rect.y + TAMANO_SPRITE / 2;

        SDL_SetRenderDrawColor(juego->renderer, 255, 50, 50, 255);
        SDL_Rect rangoVisual = { centroX - RANGO_ATAQUE, centroY - RANGO_ATAQUE,
                                 RANGO_ATAQUE * 2, RANGO_ATAQUE * 2 };
        SDL_RenderDrawRect(juego->renderer, &rangoVisual);

        // Línea amarilla del centro del jugador al centro del machete
        if (juego->machete.animandoAtaque) {
            SDL_SetRenderDrawColor(juego->renderer, 255, 255, 0, 255);
            int macheteX = juego->machete.rect.x + 24;  // Centro del sprite del machete
            int macheteY = juego->machete.rect.y + 24;
            SDL_RenderDrawLine(juego->renderer, centroX, centroY, macheteX, macheteY);
        }
    }

    mostrarPuntuacionPantalla(juego);
    renderizarBarraCooldown(juego);
    SDL_RenderPresent(juego->renderer);
}

/**
 * Función: limpiarRecursos
 * Libera toda la memoria y los recursos del juego antes de cerrar el programa
 *
 * Parámetros:
 * - juego: Puntero a la estructura Juego
 *
 * Funcionamiento (en orden):
 * 1. Cierra el gamepad si está abierto (SDL_GameControllerClose)
 * 2. Destruye las texturas: jugador, enemigo, machete y fondo
 * 3. Cierra la fuente TTF
 * 4. Destruye el renderer
 * 5. Destruye la ventana
 * 6. Cierra el subsistema SDL_ttf  (TTF_Quit)
 * 7. Cierra el subsistema SDL_image (IMG_Quit)
 * 8. Cierra SDL completamente      (SDL_Quit)
 *
 * Cada recurso se verifica con un if antes de liberarlo para evitar
 * errores si la función se llama antes de que todos los recursos
 * hayan sido inicializados (por ejemplo, cuando inicializarSDL falla a medias).
 *
 * IMPORTANTE: Esta función debe llamarse siempre antes de terminar el programa
 * para evitar fugas de memoria.
 */
void limpiarRecursos(Juego* juego) {
    if (juego->gamepad)    SDL_GameControllerClose(juego->gamepad);
    if (juego->texJugador) SDL_DestroyTexture(juego->texJugador);
    if (juego->texEnemigo) SDL_DestroyTexture(juego->texEnemigo);
    if (juego->texMachete) SDL_DestroyTexture(juego->texMachete);
    if (juego->texFondo)   SDL_DestroyTexture(juego->texFondo);
    if (juego->fuente)     TTF_CloseFont(juego->fuente);
    if (juego->renderer)   SDL_DestroyRenderer(juego->renderer);
    if (juego->ventana)    SDL_DestroyWindow(juego->ventana);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

/**
 * Función: reiniciarJuego
 * Reinicia todos los elementos del juego para comenzar una nueva partida
 *
 * Parámetros:
 * - juego: Puntero a la estructura Juego
 *
 * Funcionamiento:
 * 1. Reinicia la posición y propiedades del jugador al centro de la pantalla
 * 2. Reinicia el sistema de enemigos (1 enemigo, puntuación 0)
 * 3. Reinicia el estado del machete (no recogido, no equipado, cooldown limpio)
 * 4. Pone macheteAparecido en false para que pueda volver a aparecer al llegar a 20 puntos
 * 5. Pone ultimoNivelDificultad en 0 para que la dificultad escale desde el principio
 * 6. Cambia el estado a ESTADO_JUGANDO para iniciar la partida
 *
 * Esta función es llamada tanto al seleccionar "JUGAR" en el menú
 * como al reiniciar desde la pantalla de Game Over.
 */
void reiniciarJuego(Juego* juego) {
    inicializarJugador(&juego->jugador);
    inicializarEnemigos(juego);
    inicializarMachete(juego);
    juego->macheteAparecido      = false;
    juego->ultimoNivelDificultad = 0;
    juego->estado                = ESTADO_JUGANDO;
}

/**
 * Función: renderizarMenu
 * Dibuja la pantalla del menú principal con el título y las opciones disponibles
 *
 * Parámetros:
 * - juego: Puntero a la estructura Juego
 *
 * Funcionamiento:
 * 1. Limpia la pantalla con SDL_RenderClear
 * 2. Dibuja el título "ESQUIVAR BOTELLAS" en amarillo centrado en la parte superior
 * 3. Itera sobre las 3 opciones del menú (JUGAR, INSTRUCCIONES, SALIR):
 *    - La opción seleccionada (opcionMenuSeleccionada) se muestra en amarillo
 *      con el prefijo ">" como indicador visual de selección
 *    - Las opciones no seleccionadas se muestran en blanco con prefijo "  " (dos espacios)
 * 4. Dibuja un texto de ayuda en la parte inferior con los controles de navegación
 * 5. Presenta el frame con SDL_RenderPresent
 *
 * Posicionamiento:
 * - Título centrado en Y = 160
 * - Primera opción en Y = 320, con espaciado de 80 píxeles entre opciones
 * - Texto de ayuda a 55 píxeles del borde inferior
 */
void renderizarMenu(Juego* juego) {
    SDL_RenderClear(juego->renderer);

    SDL_Color amarillo = {255, 220,   0, 255};
    SDL_Color blanco   = {255, 255, 255, 255};
    SDL_Color gris     = {160, 160, 160, 255};

    renderizarTexto(juego, "ESQUIVAR BOTELLAS",
        ANCHO_VENTANA / 2 - 190, 160, amarillo);

    const char* opciones[]  = {"JUGAR", "INSTRUCCIONES", "SALIR"};
    const int totalOpciones = 3;
    const int inicioY       = 320;
    const int espaciado     = 80;

    for (int i = 0; i < totalOpciones; i++) {
        SDL_Color color = (juego->opcionMenuSeleccionada == i) ? amarillo : blanco;
        std::string linea = (juego->opcionMenuSeleccionada == i)
            ? std::string("> ") + opciones[i]
            : std::string("  ") + opciones[i];
        renderizarTexto(juego, linea.c_str(),
            ANCHO_VENTANA / 2 - 130, inicioY + i * espaciado, color);
    }

    renderizarTexto(juego,
        "Flechas/DPad para navegar   Enter/Cruz/Click para seleccionar",
        ANCHO_VENTANA / 2 - 400, ALTO_VENTANA - 55, gris);

    SDL_RenderPresent(juego->renderer);
}

/**
 * Función: manejarEventosMenu
 * Procesa todos los eventos de entrada mientras el juego está en ESTADO_MENU
 *
 * Parámetros:
 * - juego: Puntero a la estructura Juego
 *
 * Funcionamiento:
 * 1. SDL_QUIT: pone ejecutando = false y retorna inmediatamente
 *
 * 2. TECLADO (SDL_KEYDOWN):
 *    - Flecha arriba:  decrementa opcionMenuSeleccionada, con wrap-around al final
 *    - Flecha abajo:   incrementa opcionMenuSeleccionada, con wrap-around al inicio
 *    - Enter / KP_Enter: ejecuta la opción seleccionada:
 *      * 0 → reiniciarJuego (inicia partida)
 *      * 1 → estado = ESTADO_INSTRUCCIONES
 *      * 2 → ejecutando = false (cierra el programa)
 *
 * 3. GAMEPAD (SDL_CONTROLLERBUTTONDOWN):
 *    - DPAD_UP:        igual que flecha arriba del teclado
 *    - DPAD_DOWN:      igual que flecha abajo del teclado
 *    - BUTTON_A (Cruz PS3): igual que Enter del teclado
 *    También maneja hot-plug (conexión y desconexión del gamepad)
 *
 * 4. MOUSE hover (SDL_MOUSEMOTION):
 *    - Si el cursor está sobre una fila de opción, actualiza opcionMenuSeleccionada
 *    - Cada fila ocupa altoFila (50) píxeles a partir de su posición Y
 *
 * 5. MOUSE click (SDL_MOUSEBUTTONDOWN, botón izquierdo):
 *    - Si se hace click sobre una fila, ejecuta la acción correspondiente
 *
 * Nota: El wrap-around permite navegar de la última opción a la primera
 * y viceversa en forma circular tanto con teclado como con gamepad.
 */
void manejarEventosMenu(Juego* juego) {
    const int totalOpciones = 3;
    const int inicioY       = 320;
    const int espaciado     = 80;
    const int altoFila      = 50;

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) { juego->ejecutando = false; return; }

        // --- Teclado ---
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_UP:
                    juego->opcionMenuSeleccionada--;
                    if (juego->opcionMenuSeleccionada < 0)
                        juego->opcionMenuSeleccionada = totalOpciones - 1;
                    break;
                case SDLK_DOWN:
                    juego->opcionMenuSeleccionada++;
                    if (juego->opcionMenuSeleccionada >= totalOpciones)
                        juego->opcionMenuSeleccionada = 0;
                    break;
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                    switch (juego->opcionMenuSeleccionada) {
                        case 0: reiniciarJuego(juego);                break;
                        case 1: juego->estado = ESTADO_INSTRUCCIONES;  break;
                        case 2: juego->ejecutando = false;             break;
                    }
                    break;
                default: break;
            }
        }

        // --- Gamepad ---
        if (e.type == SDL_CONTROLLERBUTTONDOWN) {
            switch (e.cbutton.button) {
                case SDL_CONTROLLER_BUTTON_DPAD_UP:
                    juego->opcionMenuSeleccionada--;
                    if (juego->opcionMenuSeleccionada < 0)
                        juego->opcionMenuSeleccionada = totalOpciones - 1;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                    juego->opcionMenuSeleccionada++;
                    if (juego->opcionMenuSeleccionada >= totalOpciones)
                        juego->opcionMenuSeleccionada = 0;
                    break;
                case SDL_CONTROLLER_BUTTON_A:  // Cruz (X) PS3 = confirmar selección
                    switch (juego->opcionMenuSeleccionada) {
                        case 0: reiniciarJuego(juego);                break;
                        case 1: juego->estado = ESTADO_INSTRUCCIONES;  break;
                        case 2: juego->ejecutando = false;             break;
                    }
                    break;
                default: break;
            }
        }

        // Hot-plug: gamepad conectado mientras se navega el menú
        if (e.type == SDL_CONTROLLERDEVICEADDED && !juego->gamepad) {
            juego->gamepad = SDL_GameControllerOpen(e.cdevice.which);
            if (juego->gamepad)
                std::cout << "Gamepad conectado: "
                          << SDL_GameControllerName(juego->gamepad) << "\n";
        }

        // Hot-plug: gamepad desconectado mientras se navega el menú
        if (e.type == SDL_CONTROLLERDEVICEREMOVED) {
            if (juego->gamepad) {
                SDL_GameControllerClose(juego->gamepad);
                juego->gamepad = nullptr;
                std::cout << "Gamepad desconectado.\n";
            }
        }

        // --- Mouse hover: resaltar la opción bajo el cursor ---
        if (e.type == SDL_MOUSEMOTION) {
            int my = e.motion.y;
            for (int i = 0; i < totalOpciones; i++) {
                int fy = inicioY + i * espaciado;
                if (my >= fy && my <= fy + altoFila)
                    juego->opcionMenuSeleccionada = i;
            }
        }

        // --- Mouse click: ejecutar la opción bajo el cursor ---
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            int my = e.button.y;
            for (int i = 0; i < totalOpciones; i++) {
                int fy = inicioY + i * espaciado;
                if (my >= fy && my <= fy + altoFila) {
                    switch (i) {
                        case 0: reiniciarJuego(juego);                break;
                        case 1: juego->estado = ESTADO_INSTRUCCIONES;  break;
                        case 2: juego->ejecutando = false;             break;
                    }
                }
            }
        }
    }
}

/**
 * Función: renderizarInstrucciones
 * Muestra la pantalla de instrucciones con los controles del teclado y del gamepad PS3
 *
 * Parámetros:
 * - juego: Puntero a la estructura Juego
 *
 * Funcionamiento:
 * 1. Limpia la pantalla y dibuja el título "INSTRUCCIONES" en amarillo
 * 2. Muestra la sección "TECLADO" con sus controles:
 *    - W/A/S/D → mover al jugador
 *    - ESPACIO → atacar con el machete
 *    - ESC     → volver al menú
 * 3. Muestra la sección "CONTROL PS3" con sus controles:
 *    - Stick izquierdo / D-pad → mover al jugador
 *    - Cruz (X) = BUTTON_A    → atacar con el machete
 *    - START                  → volver al menú
 * 4. Muestra reglas del juego (dificultad y machete) en blanco
 * 5. Muestra el hint para volver en la parte inferior de la pantalla
 * 6. Presenta el frame y procesa eventos de salida:
 *    - ESC o BACKSPACE (teclado)       → estado = ESTADO_MENU
 *    - BUTTON_B / Círculo (PS3)        → estado = ESTADO_MENU
 *    - SDL_QUIT                        → ejecutando = false
 */
void renderizarInstrucciones(Juego* juego) {
    SDL_RenderClear(juego->renderer);

    SDL_Color amarillo = {255, 220,   0, 255};
    SDL_Color blanco   = {255, 255, 255, 255};
    SDL_Color gris     = {160, 160, 160, 255};

    renderizarTexto(juego, "INSTRUCCIONES", ANCHO_VENTANA / 2 - 150, 80, amarillo);

    renderizarTexto(juego, "TECLADO",                                          200, 170, amarillo);
    renderizarTexto(juego, "W / A / S / D       Mover al jugador",             200, 220, blanco);
    renderizarTexto(juego, "ESPACIO             Atacar con el machete",         200, 270, blanco);
    renderizarTexto(juego, "ESC                 Volver al menu",                200, 320, blanco);

    renderizarTexto(juego, "CONTROL PS3",                                       200, 400, amarillo);
    renderizarTexto(juego, "Stick izq / D-pad   Mover al jugador",              200, 450, blanco);
    renderizarTexto(juego, "Cruz (X)            Atacar con el machete",         200, 500, blanco);
    renderizarTexto(juego, "START               Volver al menu",                200, 550, blanco);

    renderizarTexto(juego, "Cada 5 puntos aparece un enemigo nuevo",            200, 630, blanco);
    renderizarTexto(juego, "Con 20 puntos aparece el machete en el mapa",       200, 680, blanco);

    renderizarTexto(juego, "ESC / Circulo(PS3) para volver",
        ANCHO_VENTANA / 2 - 230, ALTO_VENTANA - 55, gris);

    SDL_RenderPresent(juego->renderer);

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) { juego->ejecutando = false; return; }
        if (e.type == SDL_KEYDOWN &&
           (e.key.keysym.sym == SDLK_ESCAPE || e.key.keysym.sym == SDLK_BACKSPACE))
            juego->estado = ESTADO_MENU;
        // Botón B (círculo PS3) para volver al menú
        if (e.type == SDL_CONTROLLERBUTTONDOWN &&
            e.cbutton.button == SDL_CONTROLLER_BUTTON_B)
            juego->estado = ESTADO_MENU;
    }
}

/**
 * Función: renderizarGameOver
 * Muestra la pantalla de fin de juego con la puntuación final y opciones para continuar
 *
 * Parámetros:
 * - juego: Puntero a la estructura Juego
 *
 * Funcionamiento:
 * 1. Limpia la pantalla y muestra "GAME OVER" en rojo centrado en la parte superior
 * 2. Muestra la puntuación final del jugador en blanco
 * 3. Muestra las opciones disponibles en amarillo:
 *    - Enter / Cruz(PS3) → volver al menú principal
 *    - ESC / START       → cerrar el programa
 * 4. Presenta el frame y procesa eventos:
 *    - SDL_QUIT                        → ejecutando = false, retorna
 *    - Enter o KP_Enter (teclado)      → estado = ESTADO_MENU
 *    - ESC (teclado)                   → ejecutando = false
 *    - BUTTON_A / Cruz (PS3)           → estado = ESTADO_MENU
 *    - BUTTON_START (PS3)              → ejecutando = false
 *
 * Nota: Esta pantalla se mantiene activa hasta que el usuario tome una decisión,
 * ya que renderizarGameOver se llama en cada iteración del bucle principal
 * mientras el estado sea ESTADO_GAME_OVER.
 */
void renderizarGameOver(Juego* juego) {
    SDL_RenderClear(juego->renderer);

    SDL_Color rojo     = {220,  50,  50, 255};
    SDL_Color blanco   = {255, 255, 255, 255};
    SDL_Color amarillo = {255, 220,   0, 255};

    renderizarTexto(juego, "GAME OVER",
        ANCHO_VENTANA / 2 - 110, 200, rojo);

    std::string scoreTexto = "Puntuacion final: " + std::to_string(juego->puntuacion);
    renderizarTexto(juego, scoreTexto.c_str(),
        ANCHO_VENTANA / 2 - 160, 310, blanco);

    renderizarTexto(juego, "Enter / Cruz(PS3)   Volver al menu",
        ANCHO_VENTANA / 2 - 200, 430, amarillo);
    renderizarTexto(juego, "ESC / START         Salir",
        ANCHO_VENTANA / 2 - 200, 500, amarillo);

    SDL_RenderPresent(juego->renderer);

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) { juego->ejecutando = false; return; }
        if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER)
                juego->estado = ESTADO_MENU;
            if (e.key.keysym.sym == SDLK_ESCAPE)
                juego->ejecutando = false;
        }
        if (e.type == SDL_CONTROLLERBUTTONDOWN) {
            if (e.cbutton.button == SDL_CONTROLLER_BUTTON_A)     // Cruz (X) → volver al menú
                juego->estado = ESTADO_MENU;
            if (e.cbutton.button == SDL_CONTROLLER_BUTTON_START) // START → salir del programa
                juego->ejecutando = false;
        }
    }
}
