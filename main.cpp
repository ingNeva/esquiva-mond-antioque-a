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
#else
    #include <unistd.h>
#endif

// ============================================
// Constantes del juego
// ============================================
#define MAX_ENEMIGOS 50
#define ANCHO_VENTANA 1400
#define ALTO_VENTANA 900
#define TAMANO_SPRITE 64
#define COOLDOWN_MACHETE 2000
#define RANGO_ATAQUE 150
#define OFFSET_MACHETE_X 40
#define OFFSET_MACHETE_Y 20
#define BARRA_COOLDOWN_ANCHO 200
#define BARRA_COOLDOWN_ALTO 30
#define BARRA_COOLDOWN_X 10
#define BARRA_COOLDOWN_Y (ALTO_VENTANA - 50)
#define DURACION_ANIMACION_ATAQUE 300
#define RADIO_GIRO_MACHETE 50
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================
// ESTADOS DEL JUEGO
// ============================================
enum EstadoJuego {
    ESTADO_MENU,
    ESTADO_JUGANDO,
    ESTADO_INSTRUCCIONES,
    ESTADO_GAME_OVER,
    ESTADO_PAUSADO          // ← NUEVO: pausa sin resetear estado
};

// ============================================
// ESTRUCTURAS
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
    bool recogido;
    bool activo;
    Uint64 ultimoUso;
    bool animandoAtaque;
    Uint64 inicioAnimacion;
    float anguloActual;
};

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
    SDL_Gamepad*        gamepad;
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
void  actualizarJugador(Jugador* jugador, SDL_Gamepad* gamepad);
void  actualizarEnemigos(Juego* juego);
bool  verificarColision(SDL_FRect* a, SDL_FRect* b);
void  dibujarJuego(Juego* juego);   // dibuja todo sin SDL_RenderPresent
void  renderizar(Juego* juego);
void  limpiarRecursos(Juego* juego);
void  actualizarAnimacionAtaque(Juego* juego);
void  calcularPosicionMacheteGirando(Juego* juego, float* posX, float* posY);
void  reiniciarJuego(Juego* juego);
void  renderizarMenu(Juego* juego);
void  manejarEventosMenu(Juego* juego);
void  renderizarInstrucciones(Juego* juego);
void  renderizarGameOver(Juego* juego);
void  renderizarPausa(Juego* juego);   // ← NUEVO

// ============================================
// FUNCIÓN PRINCIPAL
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
            case ESTADO_PAUSADO:             // ← NUEVO
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
// IMPLEMENTACIÓN DE FUNCIONES
// ============================================

bool inicializarSDL(Juego* juego) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        SDL_Log("Error SDL: %s", SDL_GetError());
        return false;
    }

    juego->ventana = SDL_CreateWindow("Esquivar Botellas",
        ANCHO_VENTANA, ALTO_VENTANA, 0);
    if (!juego->ventana) {
        SDL_Log("Error creando ventana: %s", SDL_GetError());
        return false;
    }

    juego->renderer = SDL_CreateRenderer(juego->ventana, NULL);
    if (!juego->renderer) {
        SDL_Log("Error creando renderer: %s", SDL_GetError());
        return false;
    }

    if (!TTF_Init()) {
        SDL_Log("Error SDL_ttf: %s", SDL_GetError());
        return false;
    }

    juego->gamepad = nullptr;
    int contadorGamepads = 0;
    SDL_JoystickID* ids = SDL_GetGamepads(&contadorGamepads);
    if (ids) {
        for (int i = 0; i < contadorGamepads; i++) {
            if (SDL_IsGamepad(ids[i])) {
                juego->gamepad = SDL_OpenGamepad(ids[i]);
                if (juego->gamepad) {
                    SDL_Log("Gamepad conectado: %s", SDL_GetGamepadName(juego->gamepad));
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
        fprintf(log, "texFondo:   %p\n", (void*)juego->texFondo);
        fprintf(log, "texJugador: %p\n", (void*)juego->texJugador);
        fprintf(log, "texEnemigo: %p\n", (void*)juego->texEnemigo);
        fprintf(log, "texMachete: %p\n", (void*)juego->texMachete);
        fprintf(log, "SDL_GetError: %s\n", SDL_GetError());
        fclose(log);
    }

    if (!juego->texJugador || !juego->texEnemigo || !juego->texMachete) {
        return false;
    }
    return true;
}

bool cargarFuente(Juego* juego) {
    juego->fuente = TTF_OpenFont("Arial Black.ttf", 36);
    if (!juego->fuente) {
        SDL_Log("Error cargando fuente: %s", SDL_GetError());
        return false;
    }
    return true;
}

void renderizarTexto(Juego* juego, const char* texto, int x, int y, SDL_Color color) {
    SDL_Surface* superficie = TTF_RenderText_Solid(juego->fuente, texto, 0, color);
    if (!superficie) return;

    SDL_Texture* textura = SDL_CreateTextureFromSurface(juego->renderer, superficie);
    if (!textura) { SDL_DestroySurface(superficie); return; }

    SDL_FRect rectDestino = {
        (float)x, (float)y,
        (float)superficie->w, (float)superficie->h
    };
    SDL_RenderTexture(juego->renderer, textura, NULL, &rectDestino);

    SDL_DestroyTexture(textura);
    SDL_DestroySurface(superficie);
}

void mostrarPuntuacionPantalla(Juego* juego) {
    SDL_Color colorBlanco = {255, 255, 255, 255};
    std::string textoScore = "Score: " + std::to_string(juego->puntuacion);
    renderizarTexto(juego, textoScore.c_str(), 10, 10, colorBlanco);
}

void inicializarJugador(Jugador* jugador) {
    jugador->rect.x   = (float)(ANCHO_VENTANA - TAMANO_SPRITE) / 2;
    jugador->rect.y   = (float)(ALTO_VENTANA  - TAMANO_SPRITE) / 2;
    jugador->rect.w   = (float)TAMANO_SPRITE;
    jugador->rect.h   = (float)TAMANO_SPRITE;
    jugador->velocidad = 4;
}

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
    juego->machete.rect.x = (float)(rand() % (ANCHO_VENTANA - TAMANO_SPRITE));
    juego->machete.rect.y = (float)(rand() % (ALTO_VENTANA  - TAMANO_SPRITE));
    juego->machete.recogido = false;
}

bool verificarColision(SDL_FRect* a, SDL_FRect* b) {
    return SDL_HasRectIntersectionFloat(a, b);
}

void verificarRecogidaMachete(Juego* juego) {
    if (!juego->machete.recogido) {
        if (verificarColision(&juego->jugador.rect, &juego->machete.rect)) {
            juego->machete.recogido = true;
            juego->macheteEquipado  = true;
            juego->machete.rect.w   = 48.0f;
            juego->machete.rect.h   = 48.0f;
            SDL_Log("¡Machete equipado! ESPACIO o X(PS3) para atacar.");
        }
    }
}

void usarMachete(Juego* juego) {
    Uint64 tiempoActual       = SDL_GetTicks();
    Uint64 tiempoTranscurrido = tiempoActual - juego->machete.ultimoUso;

    if (tiempoTranscurrido >= COOLDOWN_MACHETE) {
        juego->machete.activo          = true;
        juego->machete.ultimoUso       = tiempoActual;
        juego->machete.animandoAtaque  = true;
        juego->machete.inicioAnimacion = tiempoActual;
        juego->machete.anguloActual    = 0.0f;

        float centroJugadorX    = juego->jugador.rect.x + TAMANO_SPRITE / 2.0f;
        float centroJugadorY    = juego->jugador.rect.y + TAMANO_SPRITE / 2.0f;
        int enemigosDestruidos  = 0;

        for (int i = 0; i < juego->enemigosActivos; i++) {
            Enemigo* enemigo = &juego->enemigos[i];
            float deltaX = (enemigo->rect.x + TAMANO_SPRITE / 2.0f) - centroJugadorX;
            float deltaY = (enemigo->rect.y + TAMANO_SPRITE / 2.0f) - centroJugadorY;
            float distancia = sqrtf(deltaX * deltaX + deltaY * deltaY);

            if (distancia <= RANGO_ATAQUE) {
                juego->puntuacion++;
                generarEnemigo(enemigo);
                enemigosDestruidos++;
            }
        }

        if (enemigosDestruidos > 0)
            SDL_Log("¡Machete usado! %d enemigos destruidos.", enemigosDestruidos);
    }
}

float calcularProgresoCooldown(Juego* juego) {
    if (juego->machete.ultimoUso == 0) return 1.0f;

    Uint64 transcurrido = SDL_GetTicks() - juego->machete.ultimoUso;
    if (transcurrido >= COOLDOWN_MACHETE) return 1.0f;

    return (float)transcurrido / (float)COOLDOWN_MACHETE;
}

void renderizarBarraCooldown(Juego* juego) {
    if (!juego->macheteEquipado) return;

    float progreso = calcularProgresoCooldown(juego);

    SDL_FRect fondoBarra = {
        (float)(BARRA_COOLDOWN_X - 2), (float)(BARRA_COOLDOWN_Y - 2),
        (float)(BARRA_COOLDOWN_ANCHO + 4), (float)(BARRA_COOLDOWN_ALTO + 4)
    };
    SDL_SetRenderDrawColor(juego->renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(juego->renderer, &fondoBarra);

    SDL_FRect barraFondo = {
        (float)BARRA_COOLDOWN_X, (float)BARRA_COOLDOWN_Y,
        (float)BARRA_COOLDOWN_ANCHO, (float)BARRA_COOLDOWN_ALTO
    };
    SDL_SetRenderDrawColor(juego->renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(juego->renderer, &barraFondo);

    SDL_FRect barraProgreso = {
        (float)BARRA_COOLDOWN_X, (float)BARRA_COOLDOWN_Y,
        BARRA_COOLDOWN_ANCHO * progreso, (float)BARRA_COOLDOWN_ALTO
    };
    if (progreso >= 1.0f)
        SDL_SetRenderDrawColor(juego->renderer, 0, 255, 0, 255);
    else
        SDL_SetRenderDrawColor(juego->renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(juego->renderer, &barraProgreso);

    SDL_FRect iconoMachete = {
        (float)(BARRA_COOLDOWN_X + BARRA_COOLDOWN_ANCHO + 10),
        (float)BARRA_COOLDOWN_Y, 32.0f, 32.0f
    };
    SDL_RenderTexture(juego->renderer, juego->texMachete, NULL, &iconoMachete);

    SDL_Color colorTexto;
    std::string textoEstado;

    if (progreso >= 1.0f) {
        colorTexto  = {0, 255, 0, 255};
        textoEstado = "MACHETE LISTO";
    } else {
        colorTexto = {255, 255, 255, 255};
        float restante = (COOLDOWN_MACHETE - (float)(SDL_GetTicks() - juego->machete.ultimoUso)) / 1000.0f;
        char buffer[32];
        SDL_snprintf(buffer, sizeof(buffer), "MACHETE: %.1fs", restante);
        textoEstado = buffer;
    }

    renderizarTexto(juego, textoEstado.c_str(),
        BARRA_COOLDOWN_X, BARRA_COOLDOWN_Y + BARRA_COOLDOWN_ALTO + 5, colorTexto);
}

void calcularPosicionMacheteGirando(Juego* juego, float* posX, float* posY) {
    float centroX   = juego->jugador.rect.x + TAMANO_SPRITE / 2.0f;
    float centroY   = juego->jugador.rect.y + TAMANO_SPRITE / 2.0f;
    float anguloRad = juego->machete.anguloActual * (float)M_PI / 180.0f;

    *posX = centroX + cosf(anguloRad) * RADIO_GIRO_MACHETE - 24.0f;
    *posY = centroY + sinf(anguloRad) * RADIO_GIRO_MACHETE - 24.0f;
}

void actualizarPosicionMacheteEquipado(Juego* juego) {
    if (!juego->macheteEquipado) return;

    if (juego->machete.animandoAtaque) {
        float posX, posY;
        calcularPosicionMacheteGirando(juego, &posX, &posY);
        juego->machete.rect.x = posX;
        juego->machete.rect.y = posY;
    } else {
        juego->machete.rect.x = juego->jugador.rect.x + OFFSET_MACHETE_X;
        juego->machete.rect.y = juego->jugador.rect.y + OFFSET_MACHETE_Y;
    }
}

void inicializarEnemigos(Juego* juego) {
    juego->enemigosActivos = 1;
    juego->puntuacion      = 0;
    generarEnemigo(&juego->enemigos[0]);
}

void actualizarAnimacionAtaque(Juego* juego) {
    if (!juego->machete.animandoAtaque) return;

    Uint64 transcurrido = SDL_GetTicks() - juego->machete.inicioAnimacion;

    if (transcurrido >= DURACION_ANIMACION_ATAQUE) {
        juego->machete.animandoAtaque = false;
        juego->machete.anguloActual   = 0.0f;
    } else {
        float progreso = (float)transcurrido / (float)DURACION_ANIMACION_ATAQUE;
        juego->machete.anguloActual = progreso * 360.0f;
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

void manejarEventos(Juego* juego) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) {
            juego->ejecutando = false;
        }

        if (e.type == SDL_EVENT_KEY_DOWN) {
            if (e.key.key == SDLK_SPACE && juego->macheteEquipado)
                usarMachete(juego);
            if (e.key.key == SDLK_ESCAPE)
                juego->estado = ESTADO_PAUSADO;  // ← CAMBIADO: pausa en vez de menú
        }

        if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
            if (e.gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH && juego->macheteEquipado)
                usarMachete(juego);
            if (e.gbutton.button == SDL_GAMEPAD_BUTTON_START)
                juego->estado = ESTADO_PAUSADO;  // ← CAMBIADO: pausa en vez de menú
        }

        if (e.type == SDL_EVENT_GAMEPAD_ADDED && !juego->gamepad) {
            juego->gamepad = SDL_OpenGamepad(e.gdevice.which);
            if (juego->gamepad)
                SDL_Log("Gamepad conectado: %s", SDL_GetGamepadName(juego->gamepad));
        }

        if (e.type == SDL_EVENT_GAMEPAD_REMOVED) {
            SDL_Log("Gamepad desconectado.");
            if (juego->gamepad) {
                SDL_CloseGamepad(juego->gamepad);
                juego->gamepad = nullptr;
            }
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
        const int DEADZONE = 8000;

        int axisX = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX);
        int axisY = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY);

        if (axisX >  DEADZONE) jugador->rect.x += (float)jugador->velocidad;
        if (axisX < -DEADZONE) jugador->rect.x -= (float)jugador->velocidad;
        if (axisY >  DEADZONE) jugador->rect.y += (float)jugador->velocidad;
        if (axisY < -DEADZONE) jugador->rect.y -= (float)jugador->velocidad;

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

void actualizarEnemigos(Juego* juego) {
    int nivelActual = juego->puntuacion / 5;
    if (nivelActual > juego->ultimoNivelDificultad &&
        juego->enemigosActivos < MAX_ENEMIGOS) {
        generarEnemigo(&juego->enemigos[juego->enemigosActivos]);
        juego->enemigosActivos++;
        juego->ultimoNivelDificultad = nivelActual;
    }

    if (juego->puntuacion >= 20 && !juego->macheteAparecido && !juego->machete.recogido) {
        aparecerMachete(juego);
        juego->macheteAparecido = true;
        SDL_Log("¡Ha aparecido un MACHETE en el mapa!");
    }

    for (int i = 0; i < juego->enemigosActivos; i++) {
        Enemigo* enemigo = &juego->enemigos[i];

        enemigo->rect.x += enemigo->velX;
        enemigo->rect.y += enemigo->velY;

        if (enemigo->rect.x < -70 || enemigo->rect.x > ANCHO_VENTANA + 20 ||
            enemigo->rect.y < -70 || enemigo->rect.y > ALTO_VENTANA  + 20) {
            juego->puntuacion++;
            generarEnemigo(enemigo);
        }

        if (verificarColision(&juego->jugador.rect, &enemigo->rect)) {
            SDL_Log("COLISION - Juego terminado. Puntuacion: %d", juego->puntuacion);
            juego->ejecutando = false;
            return;
        }
    }

    verificarRecogidaMachete(juego);

    if (juego->machete.activo)
        juego->machete.activo = false;
}

// Dibuja todo el juego en el buffer pero NO hace SDL_RenderPresent.
// Usada tanto por renderizar() como por renderizarPausa() para evitar doble present.
void dibujarJuego(Juego* juego) {
    SDL_RenderClear(juego->renderer);

    if (juego->texFondo)
        SDL_RenderTexture(juego->renderer, juego->texFondo, NULL, NULL);

    SDL_RenderTexture(juego->renderer, juego->texJugador, NULL, &juego->jugador.rect);

    for (int i = 0; i < juego->enemigosActivos; i++)
        SDL_RenderTexture(juego->renderer, juego->texEnemigo, NULL, &juego->enemigos[i].rect);

    if (juego->puntuacion >= 20 || juego->macheteEquipado) {
        if (!juego->machete.recogido || juego->macheteEquipado)
            SDL_RenderTexture(juego->renderer, juego->texMachete, NULL, &juego->machete.rect);
    }

    if (juego->machete.activo || juego->machete.animandoAtaque) {
        float centroX = juego->jugador.rect.x + TAMANO_SPRITE / 2.0f;
        float centroY = juego->jugador.rect.y + TAMANO_SPRITE / 2.0f;

        SDL_SetRenderDrawColor(juego->renderer, 255, 50, 50, 255);
        SDL_FRect rangoVisual = {
            centroX - RANGO_ATAQUE, centroY - RANGO_ATAQUE,
            (float)(RANGO_ATAQUE * 2), (float)(RANGO_ATAQUE * 2)
        };
        SDL_RenderRect(juego->renderer, &rangoVisual);

        if (juego->machete.animandoAtaque) {
            SDL_SetRenderDrawColor(juego->renderer, 255, 255, 0, 255);
            float macheteX = juego->machete.rect.x + 24.0f;
            float macheteY = juego->machete.rect.y + 24.0f;
            SDL_RenderLine(juego->renderer, centroX, centroY, macheteX, macheteY);
        }
    }

    mostrarPuntuacionPantalla(juego);
    renderizarBarraCooldown(juego);
}

void renderizar(Juego* juego) {
    dibujarJuego(juego);
    SDL_RenderPresent(juego->renderer);  // unico present en el flujo normal
}

void limpiarRecursos(Juego* juego) {
    if (juego->gamepad)    SDL_CloseGamepad(juego->gamepad);
    if (juego->texJugador) SDL_DestroyTexture(juego->texJugador);
    if (juego->texEnemigo) SDL_DestroyTexture(juego->texEnemigo);
    if (juego->texMachete) SDL_DestroyTexture(juego->texMachete);
    if (juego->texFondo)   SDL_DestroyTexture(juego->texFondo);
    if (juego->fuente)     TTF_CloseFont(juego->fuente);
    if (juego->renderer)   SDL_DestroyRenderer(juego->renderer);
    if (juego->ventana)    SDL_DestroyWindow(juego->ventana);
    TTF_Quit();
    SDL_Quit();
}

void reiniciarJuego(Juego* juego) {
    inicializarJugador(&juego->jugador);
    inicializarEnemigos(juego);
    inicializarMachete(juego);
    juego->macheteAparecido      = false;
    juego->ultimoNivelDificultad = 0;
    juego->estado                = ESTADO_JUGANDO;
}

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

void manejarEventosMenu(Juego* juego) {
    const int totalOpciones = 3;
    const int inicioY       = 320;
    const int espaciado     = 80;
    const int altoFila      = 50;

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) { juego->ejecutando = false; return; }

        if (e.type == SDL_EVENT_KEY_DOWN) {
            switch (e.key.key) {
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

        if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
            switch (e.gbutton.button) {
                case SDL_GAMEPAD_BUTTON_DPAD_UP:
                    juego->opcionMenuSeleccionada--;
                    if (juego->opcionMenuSeleccionada < 0)
                        juego->opcionMenuSeleccionada = totalOpciones - 1;
                    break;
                case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
                    juego->opcionMenuSeleccionada++;
                    if (juego->opcionMenuSeleccionada >= totalOpciones)
                        juego->opcionMenuSeleccionada = 0;
                    break;
                case SDL_GAMEPAD_BUTTON_SOUTH:
                    switch (juego->opcionMenuSeleccionada) {
                        case 0: reiniciarJuego(juego);                break;
                        case 1: juego->estado = ESTADO_INSTRUCCIONES;  break;
                        case 2: juego->ejecutando = false;             break;
                    }
                    break;
                default: break;
            }
        }

        if (e.type == SDL_EVENT_GAMEPAD_ADDED && !juego->gamepad) {
            juego->gamepad = SDL_OpenGamepad(e.gdevice.which);
            if (juego->gamepad)
                SDL_Log("Gamepad conectado: %s", SDL_GetGamepadName(juego->gamepad));
        }

        if (e.type == SDL_EVENT_GAMEPAD_REMOVED) {
            if (juego->gamepad) {
                SDL_CloseGamepad(juego->gamepad);
                juego->gamepad = nullptr;
                SDL_Log("Gamepad desconectado.");
            }
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
                        case 0: reiniciarJuego(juego);                break;
                        case 1: juego->estado = ESTADO_INSTRUCCIONES;  break;
                        case 2: juego->ejecutando = false;             break;
                    }
                }
            }
        }
    }
}

void renderizarInstrucciones(Juego* juego) {
    SDL_RenderClear(juego->renderer);

    SDL_Color amarillo = {255, 220,   0, 255};
    SDL_Color blanco   = {255, 255, 255, 255};
    SDL_Color gris     = {160, 160, 160, 255};

    renderizarTexto(juego, "INSTRUCCIONES", ANCHO_VENTANA / 2 - 150, 80, amarillo);

    renderizarTexto(juego, "TECLADO",                                          200, 170, amarillo);
    renderizarTexto(juego, "W / A / S / D       Mover al jugador",             200, 220, blanco);
    renderizarTexto(juego, "ESPACIO             Atacar con el machete",         200, 270, blanco);
    renderizarTexto(juego, "ESC                 Pausar el juego",               200, 320, blanco);  // ← actualizado

    renderizarTexto(juego, "CONTROL PS3/PS4/XBOX",                             200, 400, amarillo);
    renderizarTexto(juego, "Stick izq / D-pad   Mover al jugador",             200, 450, blanco);
    renderizarTexto(juego, "Boton Sur (Cruz/A)  Atacar con el machete",        200, 500, blanco);
    renderizarTexto(juego, "START               Pausar el juego",              200, 550, blanco);  // ← actualizado

    renderizarTexto(juego, "Cada 5 puntos aparece un enemigo nuevo",           200, 630, blanco);
    renderizarTexto(juego, "Con 20 puntos aparece el machete en el mapa",      200, 680, blanco);

    renderizarTexto(juego, "ESC / Circulo(PS3) para volver",
        ANCHO_VENTANA / 2 - 230, ALTO_VENTANA - 55, gris);

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
        if (e.type == SDL_EVENT_QUIT) { juego->ejecutando = false; return; }
        if (e.type == SDL_EVENT_KEY_DOWN) {
            if (e.key.key == SDLK_RETURN || e.key.key == SDLK_KP_ENTER)
                juego->estado = ESTADO_MENU;
            if (e.key.key == SDLK_ESCAPE)
                juego->ejecutando = false;
        }
        if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
            if (e.gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH)
                juego->estado = ESTADO_MENU;
            if (e.gbutton.button == SDL_GAMEPAD_BUTTON_START)
                juego->ejecutando = false;
        }
    }
}

// ============================================
// NUEVO: Pantalla de pausa
// No toca ningún dato del juego — solo congela y muestra opciones
// ============================================
void renderizarPausa(Juego* juego) {
    // Dibuja el juego congelado en el buffer (SIN RenderPresent todavia)
    dibujarJuego(juego);

    // Overlay oscuro semitransparente encima
    SDL_SetRenderDrawBlendMode(juego->renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(juego->renderer, 0, 0, 0, 150);
    SDL_FRect overlay = {0.0f, 0.0f, (float)ANCHO_VENTANA, (float)ALTO_VENTANA};
    SDL_RenderFillRect(juego->renderer, &overlay);

    SDL_Color amarillo = {255, 220,   0, 255};
    SDL_Color blanco   = {255, 255, 255, 255};
    SDL_Color rojo     = {220,  50,  50, 255};

    renderizarTexto(juego, "PAUSADO",
        ANCHO_VENTANA / 2 - 85, 220, amarillo);
    renderizarTexto(juego, "ESC / START         Continuar",
        ANCHO_VENTANA / 2 - 210, 340, blanco);
    renderizarTexto(juego, "Enter / Cruz(PS3)   Volver al menu",
        ANCHO_VENTANA / 2 - 210, 410, blanco);
    renderizarTexto(juego, "Q                   Salir del juego",
        ANCHO_VENTANA / 2 - 210, 480, rojo);

    SDL_RenderPresent(juego->renderer);

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) { juego->ejecutando = false; return; }

        if (e.type == SDL_EVENT_KEY_DOWN) {
            if (e.key.key == SDLK_ESCAPE)
                juego->estado = ESTADO_JUGANDO;          // continuar sin tocar nada
            if (e.key.key == SDLK_RETURN || e.key.key == SDLK_KP_ENTER) {
                reiniciarJuego(juego);                   // reinicio explícito al ir al menú
                juego->estado = ESTADO_MENU;
            }
            if (e.key.key == SDLK_Q)
                juego->ejecutando = false;
        }

        if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
            if (e.gbutton.button == SDL_GAMEPAD_BUTTON_START)
                juego->estado = ESTADO_JUGANDO;          // continuar
            if (e.gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH) {
                reiniciarJuego(juego);
                juego->estado = ESTADO_MENU;
            }
        }
    }
}