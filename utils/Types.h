#pragma once

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "Constants.h"

// ============================================
// Structs de datos puros
// ============================================

// MAX_NIVELES debe estar definido en Constants.h.
// Si no existe, agrega: #define MAX_NIVELES 4

struct EntradaPuntaje {
    char nombre[MAX_NOMBRE];
    int  puntuacion;
};

struct TablaPuntajes {
    EntradaPuntaje entradas[MAX_PUNTAJES];
    int            cantidad;
};

enum DireccionJugador { DIR_DERECHA, DIR_IZQUIERDA, DIR_ABAJO, DIR_ARRIBA };

struct Jugador {
    SDL_FRect rect;
    int velocidad;
    // --- Animación de caminata ---
    DireccionJugador direccion  = DIR_ABAJO;
    int    frameAnim            = 0;
    Uint64 ultimoFrame          = 0;
    bool   enMovimiento         = false;
};

struct Enemigo {
    SDL_FRect   rect;
    float       velX;
    float       velY;
    TipoEnemigo tipo;
    int         vida;
    float       anguloZigzag;
    float       timerBomba;
    bool        apuntaAlJugador;       // true si fue generado apuntando al jugador
    bool        esquiveCercanoContado; // evita contar el mismo esquive dos veces
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

struct Pilar {
    SDL_FRect rect;
    bool      activo;
    float     pulsoTimer;
};

struct FloatingText {
    float  x, y;
    int    valor;
    int    timer;
    float  colorR, colorG, colorB;
    bool   activo;
};

struct Llave {
    SDL_FRect rect;
    bool      activa;
    float     pulsoTimer;
    int       nivelDestino;
};

struct Chancla {
    bool   activa;
    bool   regresando;
    float  x, y;
    float  velX, velY;
    float  angulo;
    float  xOrigen, yOrigen;
    Uint64 ultimoUso;
};
struct TransicionNivel {
    bool        activa;
    int         nivelNuevo;
    Uint64      inicio;
    EstadoJuego estadoAnterior;
};

// ============================================
// Configuracion de teclas reasignables
// ============================================
struct KeyConfig {
    SDL_Scancode moverArriba   = SDL_SCANCODE_W;
    SDL_Scancode moverAbajo    = SDL_SCANCODE_S;
    SDL_Scancode moverIzquierda= SDL_SCANCODE_A;
    SDL_Scancode moverDerecha  = SDL_SCANCODE_D;
    SDL_Scancode atacar        = SDL_SCANCODE_SPACE;
    SDL_Scancode pausa         = SDL_SCANCODE_ESCAPE;
};

// ============================================
// Estructura principal del juego
// ============================================
struct Juego {
    SDL_Window*    ventana;
    SDL_Renderer*  renderer;
    SDL_Texture*   texFondos[5];
    SDL_Texture*   texJugador;
    SDL_Texture*   texEnemigo;
    SDL_Texture*   texMachete;
    TTF_Font*      fuente;
    TTF_Font*      fuentePequena;
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
    int            posicionNuevoPuntaje;

    // Audio
    MIX_Mixer*   mixer;
    MIX_Track*   trackMusica;
    MIX_Audio*   musicaMenu;
    MIX_Audio*   musicaPausa;
    MIX_Audio*   musicaNiveles123;
    MIX_Audio*   musicaNivel4;
    MIX_Audio*   musicaNivel5;
    MIX_Audio*   musicaGameOver;
    MIX_Audio*   musicaVictoria;
    bool         musicaActiva;
    int          volumenMusica;
    EstadoPista  pistaSonando;
    bool         nivel4Reproducido;
    bool         gameOverReproducido;

    TransicionNivel transicion;

    SDL_Texture* texEnemigoRapido;
    SDL_Texture* texEnemigoTanque;
    SDL_Texture* texEnemigoZigzag;
    SDL_Texture* texEnemigoBombardero;
    SDL_Texture* texEnemigoEspejo;

    EstadoBoss   estadoBoss;
    int          bossHP;
    Uint64       bossUltimoDisparo;
    SDL_Texture* texBoss;
    SDL_Texture* texTrofeo;
    SDL_Texture* texPilar;
    Pilar        pilares[MAX_PILARES];
    int          pilaresActivos;
    bool         trofeoActivo;
    SDL_FRect    trofeoRect;
    bool         bossSpawneado;

    int          combo;
    int          mejorCombo;
    float        multiplicador;
    FloatingText floatingTexts[MAX_FLOATING_TEXT];

    SDL_Texture* texLlave;
    Llave        llave;

    SDL_Texture* texChancla = nullptr;
    Chancla      chancla;

    Uint64       inicioCuentaRegresiva;

    // Teclas reasignables
    KeyConfig keyConfig;

    // Opciones de pantalla
    int  opcionOpcionesSeleccionada;
    int  resolucionSeleccionada;
    bool pantallaCompleta;
    int  nivelActual;
    int  puntosEnNivel;

    // === Sistema de niveles y progreso ===
    bool nivelesDesbloqueados[MAX_NIVELES]; // true = nivel desbloqueado
    int  opcionLevelSelectSeleccionada;     // cursor en el menú de niveles
// Spritesheets del jugador (4 direcciones)
    SDL_Texture* texPlayerRight = nullptr;
    SDL_Texture* texPlayerLeft  = nullptr;
    SDL_Texture* texPlayerDown  = nullptr;
    SDL_Texture* texPlayerUp    = nullptr;

    // =============================================
    // Nivel 2 — Zonas de daño (suelo peligroso)
    // =============================================
    SDL_FRect zonasRiesgo[3];
    int       zonasRiesgoCount = 0;

    // =============================================
    // Nivel 3 — Niebla periódica
    // =============================================
    Uint64 nieblaSiguiente  = 0;  // ticks cuando empieza la próxima niebla
    Uint64 nieblaFin        = 0;  // ticks cuando termina la niebla activa
    bool   nieblaActiva     = false;

    // =============================================
    // Nivel 4 — Onda expansiva (empujón al jugador)
    // =============================================
    Uint64 ondaSiguiente     = 0;  // ticks cuando sale la próxima onda
    Uint64 ondaInicio        = 0;  // ticks de inicio de la onda actual
    bool   ondaActiva        = false;
    float  ondaRadio         = 0.0f;
    float  jugadorEmpujonX   = 0.0f;
    float  jugadorEmpujonY   = 0.0f;
    Uint64 jugadorEmpujonFin = 0;
};
