#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <string>
#include <cmath>
#include <cstdio>
#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>   // _mkdir
#else
    #include <unistd.h>
    #include <sys/stat.h> // mkdir
#endif

// ============================================
// Constantes del juego
// ============================================
#define MAX_ENEMIGOS              50
#define ANCHO_VENTANA             1400
#define ALTO_VENTANA              900
#define TAMANO_SPRITE             64
#define COOLDOWN_MACHETE          2000
#define RANGO_ATAQUE              150
#define OFFSET_MACHETE_X          40
#define OFFSET_MACHETE_Y          20
#define BARRA_COOLDOWN_ANCHO      200
#define BARRA_COOLDOWN_ALTO       20
#define BARRA_COOLDOWN_X          10
#define BARRA_COOLDOWN_Y          (ALTO_VENTANA - 80)
#define DURACION_ANIMACION_ATAQUE 300
#define RADIO_GIRO_MACHETE        50
#define MAX_PUNTAJES              5
#define RUTA_SAVES                "saves/puntajes.bin"
#define MAX_NOMBRE                32
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================
// ESTRUCTURAS DE PUNTAJES
// ============================================
struct EntradaPuntaje {
    char nombre[MAX_NOMBRE];
    int  puntuacion;
};

struct TablaPuntajes {
    EntradaPuntaje entradas[MAX_PUNTAJES];
    int            cantidad;  // entradas validas (0-5)
};

// ============================================
// ESTADOS DEL JUEGO
// ============================================
enum EstadoJuego {
    ESTADO_MENU,
    ESTADO_JUGANDO,
    ESTADO_INSTRUCCIONES,
    ESTADO_INGRESANDO_NOMBRE,  // captura nombre si entro al top 5
    ESTADO_GAME_OVER,
    ESTADO_PAUSADO
};

// ============================================
// ESTRUCTURAS DEL JUEGO
// ============================================
struct Jugador {
    SDL_FRect rect;
    int velocidad;
};

struct Enemigo {
    SDL_FRect rect;
    float velX;
    float velY;
};

struct Machete {
    SDL_FRect rect;
    bool   recogido;
    bool   activo;
    Uint64 ultimoUso;
    bool   animandoAtaque;
    Uint64 inicioAnimacion;
    float  anguloActual;
};

struct Juego {
    SDL_Window*    ventana;
    SDL_Renderer*  renderer;
    SDL_Texture*   texFondo;
    SDL_Texture*   texJugador;
    SDL_Texture*   texEnemigo;
    SDL_Texture*   texMachete;
    TTF_Font*      fuente;
    TTF_Font*      fuentePequena;        // para top 5 y etiquetas secundarias
    Jugador        jugador;
    Enemigo        enemigos[MAX_ENEMIGOS];
    int            enemigosActivos;
    int            puntuacion;
    bool           ejecutando;
    Machete        machete;
    bool           macheteEquipado;
    EstadoJuego    estado;
    int            opcionMenuSeleccionada;
    bool           macheteAparecido;
    int            ultimoNivelDificultad;
    SDL_Gamepad*   gamepad;
    TablaPuntajes  tablaPuntajes;
    char           nombreIngresado[MAX_NOMBRE];
    int            longitudNombre;
    int            posicionNuevoPuntaje; // -1 si no entro al top 5
};

// ============================================
// DECLARACION DE FUNCIONES
// ============================================
bool  inicializarSDL(Juego* juego);
bool  cargarTexturas(Juego* juego);
bool  cargarFuente(Juego* juego);
void  renderizarTexto(Juego* juego, const char* texto, int x, int y, SDL_Color color);
void  renderizarTextoPequeno(Juego* juego, const char* texto, int x, int y, SDL_Color color);
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
void  actualizarJugador(Jugador* jugador, SDL_Gamepad* gamepad);
void  actualizarEnemigos(Juego* juego);
bool  verificarColision(SDL_FRect* a, SDL_FRect* b);
void  dibujarJuego(Juego* juego);
void  renderizar(Juego* juego);
void  limpiarRecursos(Juego* juego);
void  actualizarAnimacionAtaque(Juego* juego);
void  calcularPosicionMacheteGirando(Juego* juego, float* posX, float* posY);
void  reiniciarJuego(Juego* juego);
void  renderizarMenu(Juego* juego);
void  manejarEventosMenu(Juego* juego);
void  renderizarInstrucciones(Juego* juego);
void  renderizarGameOver(Juego* juego);
void  renderizarPausa(Juego* juego);
// --- puntajes ---
void  crearDirectorioSaves();
void  cargarPuntajes(TablaPuntajes* tabla);
void  guardarPuntajes(const TablaPuntajes* tabla);
bool  calificaParaTop5(const TablaPuntajes* tabla, int puntuacion);
int   insertarPuntaje(TablaPuntajes* tabla, const char* nombre, int puntuacion);
void  renderizarTop5(Juego* juego, int x, int y, int posicionResaltada);
// --- ingreso de nombre ---
void  iniciarIngresoNombre(Juego* juego);
void  renderizarIngresoNombre(Juego* juego);

// ============================================
// FUNCION PRINCIPAL
// ============================================
int main(int argc, char* argv[]) {
    FILE* log = fopen("log.txt", "w");
    if (log) { fprintf(log, "Iniciando...\n"); fclose(log); }

    const char* basePath = SDL_GetBasePath();
    if (basePath) {
#ifdef _WIN32
        SetCurrentDirectoryA(basePath);
#else
        chdir(basePath);
#endif
        log = fopen("log.txt", "a");
        if (log) { fprintf(log, "BasePath: %s\n", basePath); fclose(log); }
    }

    srand((unsigned int)time(NULL));

    Juego juego = {};
    juego.estado                 = ESTADO_MENU;
    juego.opcionMenuSeleccionada = 0;
    juego.posicionNuevoPuntaje   = -1;

    crearDirectorioSaves();
    cargarPuntajes(&juego.tablaPuntajes);

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
                    // Si califica al top 5 primero pide el nombre
                    if (calificaParaTop5(&juego.tablaPuntajes, juego.puntuacion)) {
                        iniciarIngresoNombre(&juego);
                        juego.estado = ESTADO_INGRESANDO_NOMBRE;
                    } else {
                        juego.estado = ESTADO_GAME_OVER;
                    }
                }
                renderizar(&juego);
                break;
            case ESTADO_INGRESANDO_NOMBRE:
                renderizarIngresoNombre(&juego);
                break;
            case ESTADO_PAUSADO:
                renderizarPausa(&juego);
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
// SDL E INICIALIZACION
// ============================================

bool inicializarSDL(Juego* juego) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        SDL_Log("Error SDL: %s", SDL_GetError());
        return false;
    }
    juego->ventana = SDL_CreateWindow("Esquivar Botellas", ANCHO_VENTANA, ALTO_VENTANA, 0);
    if (!juego->ventana) { SDL_Log("Error ventana: %s", SDL_GetError()); return false; }

    juego->renderer = SDL_CreateRenderer(juego->ventana, NULL);
    if (!juego->renderer) { SDL_Log("Error renderer: %s", SDL_GetError()); return false; }

    if (!TTF_Init()) { SDL_Log("Error TTF: %s", SDL_GetError()); return false; }

    juego->gamepad = nullptr;
    int count = 0;
    SDL_JoystickID* ids = SDL_GetGamepads(&count);
    if (ids) {
        for (int i = 0; i < count; i++) {
            if (SDL_IsGamepad(ids[i])) {
                juego->gamepad = SDL_OpenGamepad(ids[i]);
                if (juego->gamepad) {
                    SDL_Log("Gamepad: %s", SDL_GetGamepadName(juego->gamepad));
                    break;
                }
            }
        }
        SDL_free(ids);
    }
    return true;
}

bool cargarTexturas(Juego* juego) {
    juego->texFondo   = IMG_LoadTexture(juego->renderer, "imagenes/fondo.png");
    juego->texJugador = IMG_LoadTexture(juego->renderer, "imagenes/player.png");
    juego->texEnemigo = IMG_LoadTexture(juego->renderer, "imagenes/enemy.png");
    juego->texMachete = IMG_LoadTexture(juego->renderer, "imagenes/machete.png");

    FILE* log = fopen("log.txt", "a");
    if (log) {
        fprintf(log, "texFondo:%p texJugador:%p texEnemigo:%p texMachete:%p\n",
            (void*)juego->texFondo, (void*)juego->texJugador,
            (void*)juego->texEnemigo, (void*)juego->texMachete);
        fclose(log);
    }
    return (juego->texJugador && juego->texEnemigo && juego->texMachete);
}

bool cargarFuente(Juego* juego) {
    juego->fuente = TTF_OpenFont("Arial Black.ttf", 36);
    if (!juego->fuente) { SDL_Log("Error fuente: %s", SDL_GetError()); return false; }

    juego->fuentePequena = TTF_OpenFont("Arial Black.ttf", 22);
    if (!juego->fuentePequena)
        juego->fuentePequena = juego->fuente;  // fallback

    return true;
}

// ============================================
// RENDERIZADO DE TEXTO
// ============================================

void renderizarTexto(Juego* juego, const char* texto, int x, int y, SDL_Color color) {
    SDL_Surface* sup = TTF_RenderText_Solid(juego->fuente, texto, 0, color);
    if (!sup) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(juego->renderer, sup);
    if (tex) {
        SDL_FRect dst = {(float)x, (float)y, (float)sup->w, (float)sup->h};
        SDL_RenderTexture(juego->renderer, tex, NULL, &dst);
        SDL_DestroyTexture(tex);
    }
    SDL_DestroySurface(sup);
}

void renderizarTextoPequeno(Juego* juego, const char* texto, int x, int y, SDL_Color color) {
    SDL_Surface* sup = TTF_RenderText_Solid(juego->fuentePequena, texto, 0, color);
    if (!sup) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(juego->renderer, sup);
    if (tex) {
        SDL_FRect dst = {(float)x, (float)y, (float)sup->w, (float)sup->h};
        SDL_RenderTexture(juego->renderer, tex, NULL, &dst);
        SDL_DestroyTexture(tex);
    }
    SDL_DestroySurface(sup);
}

// ============================================
// SISTEMA DE PUNTAJES — ARCHIVO BINARIO
// Formato: [int cantidad][EntradaPuntaje x cantidad]
// Tamaño máximo: 4 + 5*(32+4) = 184 bytes
// ============================================

void crearDirectorioSaves() {
#ifdef _WIN32
    _mkdir("saves");
#else
    mkdir("saves", 0755);
#endif
}

void cargarPuntajes(TablaPuntajes* tabla) {
    tabla->cantidad = 0;
    for (int i = 0; i < MAX_PUNTAJES; i++) {
        tabla->entradas[i].puntuacion = 0;
        tabla->entradas[i].nombre[0]  = '\0';
    }
    FILE* f = fopen(RUTA_SAVES, "rb");
    if (!f) return;

    fread(&tabla->cantidad, sizeof(int), 1, f);
    if (tabla->cantidad < 0 || tabla->cantidad > MAX_PUNTAJES)
        tabla->cantidad = 0;

    for (int i = 0; i < tabla->cantidad; i++)
        fread(&tabla->entradas[i], sizeof(EntradaPuntaje), 1, f);

    fclose(f);
}

void guardarPuntajes(const TablaPuntajes* tabla) {
    crearDirectorioSaves();
    FILE* f = fopen(RUTA_SAVES, "wb");
    if (!f) { SDL_Log("Error: no se pudo guardar puntajes."); return; }
    fwrite(&tabla->cantidad, sizeof(int), 1, f);
    for (int i = 0; i < tabla->cantidad; i++)
        fwrite(&tabla->entradas[i], sizeof(EntradaPuntaje), 1, f);
    fclose(f);
}

bool calificaParaTop5(const TablaPuntajes* tabla, int puntuacion) {
    if (puntuacion <= 0) return false;
    if (tabla->cantidad < MAX_PUNTAJES) return true;
    return puntuacion > tabla->entradas[tabla->cantidad - 1].puntuacion;
}

// Inserta ordenado desc. Retorna indice donde quedo (0 = primer lugar).
int insertarPuntaje(TablaPuntajes* tabla, const char* nombre, int puntuacion) {
    int pos = tabla->cantidad;
    for (int i = 0; i < tabla->cantidad; i++) {
        if (puntuacion > tabla->entradas[i].puntuacion) { pos = i; break; }
    }
    int hasta = (tabla->cantidad < MAX_PUNTAJES) ? tabla->cantidad : MAX_PUNTAJES - 1;
    for (int i = hasta; i > pos; i--)
        tabla->entradas[i] = tabla->entradas[i - 1];

    SDL_snprintf(tabla->entradas[pos].nombre, MAX_NOMBRE, "%s", nombre);
    tabla->entradas[pos].puntuacion = puntuacion;
    if (tabla->cantidad < MAX_PUNTAJES) tabla->cantidad++;

    guardarPuntajes(tabla);
    return pos;
}

// Dibuja el top 5. posicionResaltada=-1 para ninguno, verde para el nuevo.
void renderizarTop5(Juego* juego, int x, int y, int posicionResaltada) {
    SDL_Color amarillo = {255, 220,   0, 255};
    SDL_Color blanco   = {255, 255, 255, 255};
    SDL_Color gris     = {130, 130, 130, 255};
    SDL_Color dorado   = {255, 180,   0, 255};
    SDL_Color verde    = { 80, 255, 120, 255};

    const char* medallas[] = {"#1", "#2", "#3", "#4", "#5"};
    const int espaciado = 38;

    renderizarTextoPequeno(juego, "--- TOP 5 ---", x, y, amarillo);

    if (juego->tablaPuntajes.cantidad == 0) {
        renderizarTextoPequeno(juego, "Sin records aun", x, y + espaciado, gris);
        return;
    }

    for (int i = 0; i < MAX_PUNTAJES; i++) {
        char linea[72];
        SDL_Color color;

        if (i < juego->tablaPuntajes.cantidad) {
            if (i == posicionResaltada)
                color = verde;
            else if (i == 0)
                color = dorado;
            else
                color = blanco;

            SDL_snprintf(linea, sizeof(linea), "%s %-14s %4d",
                medallas[i],
                juego->tablaPuntajes.entradas[i].nombre,
                juego->tablaPuntajes.entradas[i].puntuacion);
        } else {
            color = gris;
            SDL_snprintf(linea, sizeof(linea), "%s ---", medallas[i]);
        }

        renderizarTextoPequeno(juego, linea, x, y + (i + 1) * espaciado, color);
    }
}

// ============================================
// INGRESO DE NOMBRE (nuevo estado)
// ============================================

void iniciarIngresoNombre(Juego* juego) {
    SDL_memset(juego->nombreIngresado, 0, MAX_NOMBRE);
    juego->longitudNombre       = 0;
    juego->posicionNuevoPuntaje = -1;
    SDL_StartTextInput(juego->ventana);
}

void renderizarIngresoNombre(Juego* juego) {
    SDL_RenderClear(juego->renderer);

    SDL_Color amarillo = {255, 220,   0, 255};
    SDL_Color blanco   = {255, 255, 255, 255};
    SDL_Color verde    = { 80, 255, 120, 255};
    SDL_Color gris     = {130, 130, 130, 255};

    // Titulo
    renderizarTexto(juego, "NUEVO RECORD!", ANCHO_VENTANA / 2 - 175, 110, amarillo);

    // Puntaje
    char msgPuntaje[64];
    SDL_snprintf(msgPuntaje, sizeof(msgPuntaje), "Puntuacion: %d", juego->puntuacion);
    renderizarTexto(juego, msgPuntaje, ANCHO_VENTANA / 2 - 140, 190, blanco);

    // Instruccion
    renderizarTextoPequeno(juego, "Ingresa tu nombre (max 31 caracteres) y presiona Enter",
        ANCHO_VENTANA / 2 - 300, 270, gris);

    // Caja de texto
    SDL_SetRenderDrawColor(juego->renderer, 50, 50, 50, 255);
    SDL_FRect caja = {(float)(ANCHO_VENTANA / 2 - 220), 320.0f, 440.0f, 54.0f};
    SDL_RenderFillRect(juego->renderer, &caja);
    SDL_SetRenderDrawColor(juego->renderer, 255, 220, 0, 255);
    SDL_RenderRect(juego->renderer, &caja);

    // Texto ingresado + cursor parpadeante
    char textoMostrado[MAX_NOMBRE + 2];
    bool cursorVisible = (SDL_GetTicks() / 500) % 2 == 0;
    SDL_snprintf(textoMostrado, sizeof(textoMostrado), "%s%s",
        juego->nombreIngresado, cursorVisible ? "_" : " ");
    renderizarTexto(juego, textoMostrado,
        ANCHO_VENTANA / 2 - 205, 330, verde);

    // Top 5 a la derecha
    renderizarTop5(juego, ANCHO_VENTANA - 430, 110, -1);

    // Ayuda inferior
    renderizarTextoPequeno(juego,
        "Enter: confirmar    ESC: cancelar (puntaje no guardado)",
        ANCHO_VENTANA / 2 - 310, ALTO_VENTANA - 55, gris);

    SDL_RenderPresent(juego->renderer);

    // Eventos
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) {
            SDL_StopTextInput(juego->ventana);
            juego->ejecutando = false;
            return;
        }

        if (e.type == SDL_EVENT_TEXT_INPUT) {
            int espacio = MAX_NOMBRE - 1 - juego->longitudNombre;
            if (espacio > 0) {
                int len    = (int)SDL_strlen(e.text.text);
                int copiar = (len < espacio) ? len : espacio;
                SDL_strlcat(juego->nombreIngresado, e.text.text, MAX_NOMBRE);
                juego->longitudNombre += copiar;
            }
        }

        if (e.type == SDL_EVENT_KEY_DOWN) {
            if (e.key.key == SDLK_BACKSPACE && juego->longitudNombre > 0) {
                juego->longitudNombre--;
                juego->nombreIngresado[juego->longitudNombre] = '\0';
            }
            if ((e.key.key == SDLK_RETURN || e.key.key == SDLK_KP_ENTER)
                && juego->longitudNombre > 0) {
                SDL_StopTextInput(juego->ventana);
                juego->posicionNuevoPuntaje = insertarPuntaje(
                    &juego->tablaPuntajes,
                    juego->nombreIngresado,
                    juego->puntuacion);
                juego->estado = ESTADO_GAME_OVER;
            }
            if (e.key.key == SDLK_ESCAPE) {
                SDL_StopTextInput(juego->ventana);
                juego->posicionNuevoPuntaje = -1;
                juego->estado = ESTADO_GAME_OVER;
            }
        }
    }
}

// ============================================
// JUGADOR
// ============================================

void mostrarPuntuacionPantalla(Juego* juego) {
    SDL_Color blanco = {255, 255, 255, 255};
    std::string txt = "Score: " + std::to_string(juego->puntuacion);
    renderizarTexto(juego, txt.c_str(), 10, 10, blanco);
}

void inicializarJugador(Jugador* jugador) {
    jugador->rect.x    = (float)(ANCHO_VENTANA - TAMANO_SPRITE) / 2;
    jugador->rect.y    = (float)(ALTO_VENTANA  - TAMANO_SPRITE) / 2;
    jugador->rect.w    = (float)TAMANO_SPRITE;
    jugador->rect.h    = (float)TAMANO_SPRITE;
    jugador->velocidad = 4;
}

// ============================================
// MACHETE
// ============================================

void inicializarMachete(Juego* juego) {
    juego->machete.recogido        = false;
    juego->machete.activo          = false;
    juego->machete.ultimoUso       = 0;
    juego->machete.rect.w          = (float)TAMANO_SPRITE;
    juego->machete.rect.h          = (float)TAMANO_SPRITE;
    juego->macheteEquipado         = false;
    juego->machete.animandoAtaque  = false;
    juego->machete.inicioAnimacion = 0;
    juego->machete.anguloActual    = 0.0f;
}

void aparecerMachete(Juego* juego) {
    juego->machete.rect.x   = (float)(rand() % (ANCHO_VENTANA - TAMANO_SPRITE));
    juego->machete.rect.y   = (float)(rand() % (ALTO_VENTANA  - TAMANO_SPRITE));
    juego->machete.recogido = false;
}

void verificarRecogidaMachete(Juego* juego) {
    if (!juego->machete.recogido &&
        verificarColision(&juego->jugador.rect, &juego->machete.rect)) {
        juego->machete.recogido = true;
        juego->macheteEquipado  = true;
        juego->machete.rect.w   = 48.0f;
        juego->machete.rect.h   = 48.0f;
        SDL_Log("Machete equipado!");
    }
}

void usarMachete(Juego* juego) {
    Uint64 tiempoActual = SDL_GetTicks();
    if (tiempoActual - juego->machete.ultimoUso < COOLDOWN_MACHETE) return;

    juego->machete.activo          = true;
    juego->machete.ultimoUso       = tiempoActual;
    juego->machete.animandoAtaque  = true;
    juego->machete.inicioAnimacion = tiempoActual;
    juego->machete.anguloActual    = 0.0f;

    float cx = juego->jugador.rect.x + TAMANO_SPRITE / 2.0f;
    float cy = juego->jugador.rect.y + TAMANO_SPRITE / 2.0f;
    int   destruidos = 0;

    for (int i = 0; i < juego->enemigosActivos; i++) {
        float dx  = (juego->enemigos[i].rect.x + TAMANO_SPRITE / 2.0f) - cx;
        float dy  = (juego->enemigos[i].rect.y + TAMANO_SPRITE / 2.0f) - cy;
        float dis = sqrtf(dx*dx + dy*dy);
        if (dis <= RANGO_ATAQUE) {
            juego->puntuacion++;
            generarEnemigo(&juego->enemigos[i]);
            destruidos++;
        }
    }
    if (destruidos > 0) SDL_Log("Machete: %d destruidos.", destruidos);
}

float calcularProgresoCooldown(Juego* juego) {
    if (juego->machete.ultimoUso == 0) return 1.0f;
    Uint64 t = SDL_GetTicks() - juego->machete.ultimoUso;
    if (t >= COOLDOWN_MACHETE) return 1.0f;
    return (float)t / (float)COOLDOWN_MACHETE;
}

// Barra de cooldown con icono, gradiente de color y etiqueta
void renderizarBarraCooldown(Juego* juego) {
    if (!juego->macheteEquipado) return;

    float progreso = calcularProgresoCooldown(juego);

    const int barX     = BARRA_COOLDOWN_X;
    const int barY     = BARRA_COOLDOWN_Y;
    const int barW     = BARRA_COOLDOWN_ANCHO;
    const int barH     = BARRA_COOLDOWN_ALTO;
    const int offsetX  = 34;   // espacio para el icono

    // Icono del machete
    SDL_FRect icono = {(float)barX, (float)(barY - 4), 28.0f, 28.0f};
    SDL_RenderTexture(juego->renderer, juego->texMachete, NULL, &icono);

    // Borde negro
    SDL_SetRenderDrawColor(juego->renderer, 0, 0, 0, 255);
    SDL_FRect borde = {
        (float)(barX + offsetX - 2), (float)(barY - 2),
        (float)(barW + 4), (float)(barH + 4)
    };
    SDL_RenderFillRect(juego->renderer, &borde);

    // Fondo oscuro
    SDL_SetRenderDrawColor(juego->renderer, 35, 35, 35, 255);
    SDL_FRect fondo = {(float)(barX + offsetX), (float)barY, (float)barW, (float)barH};
    SDL_RenderFillRect(juego->renderer, &fondo);

    // Gradiente rojo → amarillo → verde segun carga
    Uint8 r, g, b;
    if (progreso < 0.5f) {
        r = 255; g = (Uint8)(progreso * 2.0f * 200); b = 0;
    } else {
        r = (Uint8)((1.0f - (progreso - 0.5f) * 2.0f) * 255); g = 200; b = 0;
    }
    SDL_SetRenderDrawColor(juego->renderer, r, g, b, 255);
    SDL_FRect relleno = {
        (float)(barX + offsetX), (float)barY,
        barW * progreso, (float)barH
    };
    SDL_RenderFillRect(juego->renderer, &relleno);

    // Brillo superior semitransparente
    SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(juego->renderer, 255, 255, 255, 55);
    SDL_FRect brillo = {
        (float)(barX + offsetX), (float)barY,
        barW * progreso, (float)(barH / 2)
    };
    SDL_RenderFillRect(juego->renderer, &brillo);
    SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_NONE);

    // Etiqueta de estado a la derecha de la barra
    char etiqueta[48];
    SDL_Color colorEtiqueta;
    if (progreso >= 1.0f) {
        SDL_snprintf(etiqueta, sizeof(etiqueta), "LISTO  [ESPACIO]");
        colorEtiqueta = {0, 230, 80, 255};
    } else {
        float restante = (COOLDOWN_MACHETE
            - (float)(SDL_GetTicks() - juego->machete.ultimoUso)) / 1000.0f;
        SDL_snprintf(etiqueta, sizeof(etiqueta), "%.1fs", restante);
        colorEtiqueta = {200, 200, 200, 255};
    }
    renderizarTextoPequeno(juego, etiqueta,
        barX + offsetX + barW + 8, barY - 1, colorEtiqueta);
}

void calcularPosicionMacheteGirando(Juego* juego, float* posX, float* posY) {
    float cx  = juego->jugador.rect.x + TAMANO_SPRITE / 2.0f;
    float cy  = juego->jugador.rect.y + TAMANO_SPRITE / 2.0f;
    float rad = juego->machete.anguloActual * (float)M_PI / 180.0f;
    *posX = cx + cosf(rad) * RADIO_GIRO_MACHETE - 24.0f;
    *posY = cy + sinf(rad) * RADIO_GIRO_MACHETE - 24.0f;
}

void actualizarPosicionMacheteEquipado(Juego* juego) {
    if (!juego->macheteEquipado) return;
    if (juego->machete.animandoAtaque) {
        float px, py;
        calcularPosicionMacheteGirando(juego, &px, &py);
        juego->machete.rect.x = px;
        juego->machete.rect.y = py;
    } else {
        juego->machete.rect.x = juego->jugador.rect.x + OFFSET_MACHETE_X;
        juego->machete.rect.y = juego->jugador.rect.y + OFFSET_MACHETE_Y;
    }
}

// ============================================
// ENEMIGOS
// ============================================

void inicializarEnemigos(Juego* juego) {
    juego->enemigosActivos = 1;
    juego->puntuacion      = 0;
    generarEnemigo(&juego->enemigos[0]);
}

void actualizarAnimacionAtaque(Juego* juego) {
    if (!juego->machete.animandoAtaque) return;
    Uint64 t = SDL_GetTicks() - juego->machete.inicioAnimacion;
    if (t >= DURACION_ANIMACION_ATAQUE) {
        juego->machete.animandoAtaque = false;
        juego->machete.anguloActual   = 0.0f;
    } else {
        juego->machete.anguloActual = ((float)t / DURACION_ANIMACION_ATAQUE) * 360.0f;
    }
}

void generarEnemigo(Enemigo* enemigo) {
    int lado = rand() % 4;
    enemigo->rect.w = (float)TAMANO_SPRITE;
    enemigo->rect.h = (float)TAMANO_SPRITE;
    if (lado == 0) {
        enemigo->rect.x = -(float)TAMANO_SPRITE;
        enemigo->rect.y = (float)(rand() % (ALTO_VENTANA - TAMANO_SPRITE));
        enemigo->velX = 5.0f; enemigo->velY = 0.0f;
    } else if (lado == 1) {
        enemigo->rect.x = (float)ANCHO_VENTANA;
        enemigo->rect.y = (float)(rand() % (ALTO_VENTANA - TAMANO_SPRITE));
        enemigo->velX = -5.0f; enemigo->velY = 0.0f;
    } else if (lado == 2) {
        enemigo->rect.x = (float)(rand() % (ANCHO_VENTANA - TAMANO_SPRITE));
        enemigo->rect.y = -(float)TAMANO_SPRITE;
        enemigo->velX = 0.0f; enemigo->velY = 5.0f;
    } else {
        enemigo->rect.x = (float)(rand() % (ANCHO_VENTANA - TAMANO_SPRITE));
        enemigo->rect.y = (float)ALTO_VENTANA;
        enemigo->velX = 0.0f; enemigo->velY = -5.0f;
    }
}

// ============================================
// EVENTOS Y ACTUALIZACION
// ============================================

void manejarEventos(Juego* juego) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) { juego->ejecutando = false; return; }

        if (e.type == SDL_EVENT_KEY_DOWN) {
            if (e.key.key == SDLK_SPACE && juego->macheteEquipado)
                usarMachete(juego);
            if (e.key.key == SDLK_ESCAPE)
                juego->estado = ESTADO_PAUSADO;
        }

        if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
            if (e.gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH && juego->macheteEquipado)
                usarMachete(juego);
            if (e.gbutton.button == SDL_GAMEPAD_BUTTON_START)
                juego->estado = ESTADO_PAUSADO;
        }

        if (e.type == SDL_EVENT_GAMEPAD_ADDED && !juego->gamepad)
            juego->gamepad = SDL_OpenGamepad(e.gdevice.which);

        if (e.type == SDL_EVENT_GAMEPAD_REMOVED && juego->gamepad) {
            SDL_CloseGamepad(juego->gamepad);
            juego->gamepad = nullptr;
        }
    }
}

void actualizarJugador(Jugador* jugador, SDL_Gamepad* gamepad) {
    const bool* teclas = SDL_GetKeyboardState(NULL);
    if (teclas[SDL_SCANCODE_W]) jugador->rect.y -= (float)jugador->velocidad;
    if (teclas[SDL_SCANCODE_S]) jugador->rect.y += (float)jugador->velocidad;
    if (teclas[SDL_SCANCODE_A]) jugador->rect.x -= (float)jugador->velocidad;
    if (teclas[SDL_SCANCODE_D]) jugador->rect.x += (float)jugador->velocidad;

    if (gamepad) {
        const int DZ = 8000;
        int ax = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX);
        int ay = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY);
        if (ax >  DZ) jugador->rect.x += (float)jugador->velocidad;
        if (ax < -DZ) jugador->rect.x -= (float)jugador->velocidad;
        if (ay >  DZ) jugador->rect.y += (float)jugador->velocidad;
        if (ay < -DZ) jugador->rect.y -= (float)jugador->velocidad;
        if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP))
            jugador->rect.y -= (float)jugador->velocidad;
        if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN))
            jugador->rect.y += (float)jugador->velocidad;
        if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT))
            jugador->rect.x -= (float)jugador->velocidad;
        if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT))
            jugador->rect.x += (float)jugador->velocidad;
    }

    if (jugador->rect.x < 0.0f) jugador->rect.x = 0.0f;
    if (jugador->rect.x > ANCHO_VENTANA - TAMANO_SPRITE)
        jugador->rect.x = (float)(ANCHO_VENTANA - TAMANO_SPRITE);
    if (jugador->rect.y < 0.0f) jugador->rect.y = 0.0f;
    if (jugador->rect.y > ALTO_VENTANA - TAMANO_SPRITE)
        jugador->rect.y = (float)(ALTO_VENTANA - TAMANO_SPRITE);
}

bool verificarColision(SDL_FRect* a, SDL_FRect* b) {
    return SDL_HasRectIntersectionFloat(a, b);
}

void actualizarEnemigos(Juego* juego) {
    int nivel = juego->puntuacion / 5;
    if (nivel > juego->ultimoNivelDificultad && juego->enemigosActivos < MAX_ENEMIGOS) {
        generarEnemigo(&juego->enemigos[juego->enemigosActivos]);
        juego->enemigosActivos++;
        juego->ultimoNivelDificultad = nivel;
    }

    if (juego->puntuacion >= 20 && !juego->macheteAparecido && !juego->machete.recogido) {
        aparecerMachete(juego);
        juego->macheteAparecido = true;
        SDL_Log("Machete aparecio!");
    }

    for (int i = 0; i < juego->enemigosActivos; i++) {
        Enemigo* en = &juego->enemigos[i];
        en->rect.x += en->velX;
        en->rect.y += en->velY;

        if (en->rect.x < -70 || en->rect.x > ANCHO_VENTANA + 20 ||
            en->rect.y < -70 || en->rect.y > ALTO_VENTANA  + 20) {
            juego->puntuacion++;
            generarEnemigo(en);
        }

        if (verificarColision(&juego->jugador.rect, &en->rect)) {
            SDL_Log("Colision! Puntuacion: %d", juego->puntuacion);
            juego->ejecutando = false;
            return;
        }
    }

    verificarRecogidaMachete(juego);
    if (juego->machete.activo) juego->machete.activo = false;
}

// ============================================
// RENDERIZADO
// ============================================

void dibujarJuego(Juego* juego) {
    SDL_RenderClear(juego->renderer);

    if (juego->texFondo)
        SDL_RenderTexture(juego->renderer, juego->texFondo, NULL, NULL);

    SDL_RenderTexture(juego->renderer, juego->texJugador, NULL, &juego->jugador.rect);

    for (int i = 0; i < juego->enemigosActivos; i++)
        SDL_RenderTexture(juego->renderer, juego->texEnemigo, NULL, &juego->enemigos[i].rect);

    if (juego->puntuacion >= 20 || juego->macheteEquipado)
        if (!juego->machete.recogido || juego->macheteEquipado)
            SDL_RenderTexture(juego->renderer, juego->texMachete, NULL, &juego->machete.rect);

    if (juego->machete.activo || juego->machete.animandoAtaque) {
        float cx = juego->jugador.rect.x + TAMANO_SPRITE / 2.0f;
        float cy = juego->jugador.rect.y + TAMANO_SPRITE / 2.0f;

        SDL_SetRenderDrawColor(juego->renderer, 255, 50, 50, 255);
        SDL_FRect rv = {cx - RANGO_ATAQUE, cy - RANGO_ATAQUE,
                        (float)(RANGO_ATAQUE*2), (float)(RANGO_ATAQUE*2)};
        SDL_RenderRect(juego->renderer, &rv);

        if (juego->machete.animandoAtaque) {
            SDL_SetRenderDrawColor(juego->renderer, 255, 255, 0, 255);
            SDL_RenderLine(juego->renderer, cx, cy,
                juego->machete.rect.x + 24.0f, juego->machete.rect.y + 24.0f);
        }
    }

    mostrarPuntuacionPantalla(juego);
    renderizarBarraCooldown(juego);
}

void renderizar(Juego* juego) {
    dibujarJuego(juego);
    SDL_RenderPresent(juego->renderer);
}

// ============================================
// LIMPIEZA
// ============================================

void limpiarRecursos(Juego* juego) {
    if (juego->gamepad) SDL_CloseGamepad(juego->gamepad);
    if (juego->texJugador) SDL_DestroyTexture(juego->texJugador);
    if (juego->texEnemigo) SDL_DestroyTexture(juego->texEnemigo);
    if (juego->texMachete) SDL_DestroyTexture(juego->texMachete);
    if (juego->texFondo)   SDL_DestroyTexture(juego->texFondo);
    if (juego->fuentePequena && juego->fuentePequena != juego->fuente)
        TTF_CloseFont(juego->fuentePequena);
    if (juego->fuente)     TTF_CloseFont(juego->fuente);
    if (juego->renderer)   SDL_DestroyRenderer(juego->renderer);
    if (juego->ventana)    SDL_DestroyWindow(juego->ventana);
    TTF_Quit();
    SDL_Quit();
}

// ============================================
// REINICIO
// ============================================

void reiniciarJuego(Juego* juego) {
    inicializarJugador(&juego->jugador);
    inicializarEnemigos(juego);
    inicializarMachete(juego);
    juego->macheteAparecido      = false;
    juego->ultimoNivelDificultad = 0;
    juego->posicionNuevoPuntaje  = -1;
    juego->estado                = ESTADO_JUGANDO;
}

// ============================================
// MENU PRINCIPAL (top 5 lado derecho)
// ============================================

void renderizarMenu(Juego* juego) {
    SDL_RenderClear(juego->renderer);

    SDL_Color amarillo = {255, 220,   0, 255};
    SDL_Color blanco   = {255, 255, 255, 255};
    SDL_Color gris     = {130, 130, 130, 255};

    renderizarTexto(juego, "ESQUIVAR BOTELLAS", 80, 160, amarillo);

    const char* opciones[]  = {"JUGAR", "INSTRUCCIONES", "SALIR"};
    const int totalOpciones = 3;
    const int inicioY = 320, espaciado = 80;

    for (int i = 0; i < totalOpciones; i++) {
        SDL_Color color = (juego->opcionMenuSeleccionada == i) ? amarillo : blanco;
        std::string linea = (juego->opcionMenuSeleccionada == i)
            ? std::string("> ") + opciones[i]
            : std::string("  ") + opciones[i];
        renderizarTexto(juego, linea.c_str(), 100, inicioY + i * espaciado, color);
    }

    renderizarTextoPequeno(juego,
        "Flechas/DPad: navegar    Enter/Cruz: seleccionar",
        80, ALTO_VENTANA - 50, gris);

    // Divisor vertical central
    SDL_SetRenderDrawColor(juego->renderer, 70, 70, 70, 255);
    SDL_RenderLine(juego->renderer,
        (float)(ANCHO_VENTANA/2), 80.0f,
        (float)(ANCHO_VENTANA/2), (float)(ALTO_VENTANA - 70));

    // Top 5 lado derecho
    renderizarTop5(juego, ANCHO_VENTANA/2 + 60, 100, -1);

    SDL_RenderPresent(juego->renderer);
}

void manejarEventosMenu(Juego* juego) {
    const int totalOpciones = 3;
    const int inicioY = 320, espaciado = 80, altoFila = 50;

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) { juego->ejecutando = false; return; }

        if (e.type == SDL_EVENT_KEY_DOWN) {
            switch (e.key.key) {
                case SDLK_UP:
                    juego->opcionMenuSeleccionada =
                        (juego->opcionMenuSeleccionada - 1 + totalOpciones) % totalOpciones;
                    break;
                case SDLK_DOWN:
                    juego->opcionMenuSeleccionada =
                        (juego->opcionMenuSeleccionada + 1) % totalOpciones;
                    break;
                case SDLK_RETURN: case SDLK_KP_ENTER:
                    switch (juego->opcionMenuSeleccionada) {
                        case 0: reiniciarJuego(juego);               break;
                        case 1: juego->estado = ESTADO_INSTRUCCIONES; break;
                        case 2: juego->ejecutando = false;            break;
                    }
                    break;
                default: break;
            }
        }

        if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
            switch (e.gbutton.button) {
                case SDL_GAMEPAD_BUTTON_DPAD_UP:
                    juego->opcionMenuSeleccionada =
                        (juego->opcionMenuSeleccionada - 1 + totalOpciones) % totalOpciones;
                    break;
                case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
                    juego->opcionMenuSeleccionada =
                        (juego->opcionMenuSeleccionada + 1) % totalOpciones;
                    break;
                case SDL_GAMEPAD_BUTTON_SOUTH:
                    switch (juego->opcionMenuSeleccionada) {
                        case 0: reiniciarJuego(juego);               break;
                        case 1: juego->estado = ESTADO_INSTRUCCIONES; break;
                        case 2: juego->ejecutando = false;            break;
                    }
                    break;
                default: break;
            }
        }

        if (e.type == SDL_EVENT_GAMEPAD_ADDED && !juego->gamepad)
            juego->gamepad = SDL_OpenGamepad(e.gdevice.which);

        if (e.type == SDL_EVENT_GAMEPAD_REMOVED && juego->gamepad) {
            SDL_CloseGamepad(juego->gamepad);
            juego->gamepad = nullptr;
        }

        if (e.type == SDL_EVENT_MOUSE_MOTION) {
            float my = e.motion.y;
            for (int i = 0; i < totalOpciones; i++) {
                float fy = (float)(inicioY + i * espaciado);
                if (my >= fy && my <= fy + altoFila)
                    juego->opcionMenuSeleccionada = i;
            }
        }

        if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT) {
            float my = e.button.y;
            for (int i = 0; i < totalOpciones; i++) {
                float fy = (float)(inicioY + i * espaciado);
                if (my >= fy && my <= fy + altoFila) {
                    switch (i) {
                        case 0: reiniciarJuego(juego);               break;
                        case 1: juego->estado = ESTADO_INSTRUCCIONES; break;
                        case 2: juego->ejecutando = false;            break;
                    }
                }
            }
        }
    }
}

// ============================================
// INSTRUCCIONES
// ============================================

void renderizarInstrucciones(Juego* juego) {
    SDL_RenderClear(juego->renderer);

    SDL_Color amarillo = {255, 220,   0, 255};
    SDL_Color blanco   = {255, 255, 255, 255};
    SDL_Color gris     = {130, 130, 130, 255};

    renderizarTexto(juego, "INSTRUCCIONES", ANCHO_VENTANA/2 - 150, 80, amarillo);

    renderizarTexto(juego, "TECLADO",                                     200, 170, amarillo);
    renderizarTexto(juego, "W / A / S / D       Mover al jugador",        200, 220, blanco);
    renderizarTexto(juego, "ESPACIO             Atacar con el machete",    200, 270, blanco);
    renderizarTexto(juego, "ESC                 Pausar el juego",          200, 320, blanco);

    renderizarTexto(juego, "CONTROL PS3/PS4/XBOX",                        200, 400, amarillo);
    renderizarTexto(juego, "Stick izq / D-pad   Mover al jugador",        200, 450, blanco);
    renderizarTexto(juego, "Boton Sur (Cruz/A)  Atacar con el machete",   200, 500, blanco);
    renderizarTexto(juego, "START               Pausar el juego",         200, 550, blanco);

    renderizarTexto(juego, "Cada 5 puntos aparece un enemigo nuevo",      200, 630, blanco);
    renderizarTexto(juego, "Con 20 puntos aparece el machete en el mapa", 200, 680, blanco);

    renderizarTextoPequeno(juego, "ESC / Circulo(PS3) para volver",
        ANCHO_VENTANA/2 - 200, ALTO_VENTANA - 50, gris);

    SDL_RenderPresent(juego->renderer);

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) { juego->ejecutando = false; return; }
        if (e.type == SDL_EVENT_KEY_DOWN &&
           (e.key.key == SDLK_ESCAPE || e.key.key == SDLK_BACKSPACE))
            juego->estado = ESTADO_MENU;
        if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN &&
            e.gbutton.button == SDL_GAMEPAD_BUTTON_EAST)
            juego->estado = ESTADO_MENU;
    }
}

// ============================================
// GAME OVER (top 5 lado derecho, entrada resaltada)
// ============================================

void renderizarGameOver(Juego* juego) {
    SDL_RenderClear(juego->renderer);

    SDL_Color rojo     = {220,  50,  50, 255};
    SDL_Color blanco   = {255, 255, 255, 255};
    SDL_Color amarillo = {255, 220,   0, 255};
    SDL_Color verde    = { 80, 255, 120, 255};

    renderizarTexto(juego, "GAME OVER", 100, 150, rojo);

    char scoreTexto[64];
    SDL_snprintf(scoreTexto, sizeof(scoreTexto), "Puntuacion: %d", juego->puntuacion);
    renderizarTexto(juego, scoreTexto, 100, 250, blanco);

    if (juego->posicionNuevoPuntaje >= 0) {
        char msgPos[64];
        SDL_snprintf(msgPos, sizeof(msgPos),
            "TOP 5! Puesto #%d", juego->posicionNuevoPuntaje + 1);
        renderizarTexto(juego, msgPos, 100, 320, verde);
    }

    renderizarTexto(juego, "Enter / Cruz    Menu principal", 100, 470, amarillo);
    renderizarTexto(juego, "ESC / START     Salir",          100, 545, amarillo);

    // Divisor vertical
    SDL_SetRenderDrawColor(juego->renderer, 70, 70, 70, 255);
    SDL_RenderLine(juego->renderer,
        (float)(ANCHO_VENTANA/2), 80.0f,
        (float)(ANCHO_VENTANA/2), (float)(ALTO_VENTANA - 70));

    // Top 5 lado derecho, con la nueva entrada resaltada en verde
    renderizarTop5(juego, ANCHO_VENTANA/2 + 60, 100, juego->posicionNuevoPuntaje);

    SDL_RenderPresent(juego->renderer);

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) { juego->ejecutando = false; return; }
        if (e.type == SDL_EVENT_KEY_DOWN) {
            if (e.key.key == SDLK_RETURN || e.key.key == SDLK_KP_ENTER) {
                reiniciarJuego(juego);
                juego->estado = ESTADO_MENU;
            }
            if (e.key.key == SDLK_ESCAPE)
                juego->ejecutando = false;
        }
        if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
            if (e.gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH) {
                reiniciarJuego(juego);
                juego->estado = ESTADO_MENU;
            }
            if (e.gbutton.button == SDL_GAMEPAD_BUTTON_START)
                juego->ejecutando = false;
        }
    }
}

// ============================================
// PAUSA
// ============================================

void renderizarPausa(Juego* juego) {
    dibujarJuego(juego);   // juego congelado de fondo

    SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(juego->renderer, 0, 0, 0, 150);
    SDL_FRect overlay = {0.0f, 0.0f, (float)ANCHO_VENTANA, (float)ALTO_VENTANA};
    SDL_RenderFillRect(juego->renderer, &overlay);
    SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_NONE);

    SDL_Color amarillo = {255, 220,   0, 255};
    SDL_Color blanco   = {255, 255, 255, 255};
    SDL_Color rojo     = {220,  50,  50, 255};

    renderizarTexto(juego, "PAUSADO",
        ANCHO_VENTANA/2 - 85, 220, amarillo);
    renderizarTexto(juego, "ESC / START         Continuar",
        ANCHO_VENTANA/2 - 210, 340, blanco);
    renderizarTexto(juego, "Enter / Cruz(PS3)   Volver al menu",
        ANCHO_VENTANA/2 - 210, 410, blanco);
    renderizarTexto(juego, "Q                   Salir del juego",
        ANCHO_VENTANA/2 - 210, 480, rojo);

    SDL_RenderPresent(juego->renderer);

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) { juego->ejecutando = false; return; }
        if (e.type == SDL_EVENT_KEY_DOWN) {
            if (e.key.key == SDLK_ESCAPE)
                juego->estado = ESTADO_JUGANDO;
            if (e.key.key == SDLK_RETURN || e.key.key == SDLK_KP_ENTER) {
                reiniciarJuego(juego);
                juego->estado = ESTADO_MENU;
            }
            if (e.key.key == SDLK_Q)
                juego->ejecutando = false;
        }
        if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
            if (e.gbutton.button == SDL_GAMEPAD_BUTTON_START)
                juego->estado = ESTADO_JUGANDO;
            if (e.gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH) {
                reiniciarJuego(juego);
                juego->estado = ESTADO_MENU;
            }
        }
    }
}