#pragma once

// ============================================
// Constantes del juego
// ============================================
#define MAX_ENEMIGOS              80
#define ANCHO_VENTANA             1920
#define ALTO_VENTANA              1080
#define TAMANO_SPRITE             64
#define COOLDOWN_MACHETE          2000
#define RANGO_ATAQUE              150
#define OFFSET_MACHETE_X          40
#define OFFSET_MACHETE_Y          20
#define DURACION_ANIMACION_ATAQUE 300
#define RADIO_GIRO_MACHETE        50
#define MAX_PUNTAJES              5
#define MAX_NIVELES               5
#define RUTA_SAVES                "saves/puntajes.bin"
#define MAX_NOMBRE                32
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================
// Intro cinemática
// ============================================
#define INTRO_DURACION_CAMINAR_MS   1400   // tiempo que tarda en llegar al centro
#define INTRO_DURACION_RECOGER_MS    400   // pausa al recoger el machete
#define INTRO_DURACION_TEXTO_MS     1200   // tiempo que se muestra el mensaje
#define INTRO_DURACION_TOTAL_MS  (INTRO_DURACION_CAMINAR_MS + INTRO_DURACION_RECOGER_MS + INTRO_DURACION_TEXTO_MS)

// ============================================
// Umbrales de nivel
// ============================================
#define UMBRAL_NIVEL_1   0
#define UMBRAL_NIVEL_2   50
#define UMBRAL_NIVEL_3   150
#define UMBRAL_NIVEL_4   350
#define UMBRAL_NIVEL_5   700
#define PUNTOS_LLAVE_NIVEL_1   1800
#define PUNTOS_LLAVE_NIVEL_2   2200
#define PUNTOS_LLAVE_NIVEL_3   2800
#define PUNTOS_LLAVE_NIVEL_4   3500
#define DURACION_TRANSICION       2500

// ============================================
// Audio
// ============================================
#define VOLUMEN_MUSICA_DEFAULT  64

#define RUTA_MUSICA_MENU        "musica/Map (basic version).wav"
#define RUTA_MUSICA_PAUSA       "musica/Map.wav"
#define RUTA_MUSICA_NIVELES123  "musica/Mars.wav"
#define RUTA_MUSICA_NIVEL4      "musica/BossIntro.wav"
#define RUTA_MUSICA_NIVEL5      "musica/BossMain.wav"
#define RUTA_MUSICA_GAMEOVER    "musica/Warp Jingle.wav"

// ============================================
// Fondos
// ============================================
#define RUTA_FONDO_NIVEL1  "imagenes/bg_nivel1.png"
#define RUTA_FONDO_NIVEL2  "imagenes/bg_nivel2.png"
#define RUTA_FONDO_NIVEL3  "imagenes/bg_nivel3.png"
#define RUTA_FONDO_NIVEL4  "imagenes/bg_nivel4.png"
#define RUTA_FONDO_NIVEL5  "imagenes/bg_nivel5.png"

// ============================================
// Boss
// ============================================
#define BOSS_TAMANO               96
#define BOSS_HP_MAX               5
#define BOSS_CADENCIA_NORMAL      2500
#define BOSS_CADENCIA_RABIA       1200
#define BOSS_VEL_PROYECTIL        7.0f
#define MAX_PILARES               5
#define MAX_FLOATING_TEXT         20
#define LLAVE_TAMANO              48
#define PTS_LLAVE_BONUS           20
#define FLOATING_TEXT_DURACION    50

// ============================================
// Puntos
// ============================================
#define PTS_ESQUIVAR_NORMAL       3
#define PTS_ESQUIVAR_AZUL         6
#define PTS_ESQUIVAR_BOSS         15
#define PTS_MATAR_NORMAL          5
#define PTS_MATAR_TANQUE          15
#define PTS_MATAR_AZUL            8
#define PTS_MATAR_BOSS_MACHETE    10
#define PTS_TROFEO_BONUS          200

// ============================================
// Resoluciones disponibles
// ============================================
struct ResolucionDisponible {
    int ancho;
    int alto;
    const char* etiqueta;
};

static const ResolucionDisponible RESOLUCIONES[] = {
    {  800,  600,  "800x600"   },
    { 1024,  600,  "1024x600"  },
    { 1024,  768,  "1024x768"  },
    { 1280,  720,  "1280x720"  },
    { 1280,  768,  "1280x768"  },
    { 1280,  800,  "1280x800"  },
    { 1280, 1024,  "1280x1024" },
    { 1360,  768,  "1360x768"  },
    { 1366,  768,  "1366x768"  },
    { 1440,  900,  "1440x900"  },
    { 1600,  900,  "1600x900"  },
    { 1680, 1050,  "1680x1050" },
    { 1920, 1080,  "1920x1080" },
    { 1920, 1200,  "1920x1200" },
    { 2560, 1080,  "2560x1080" },
    { 2560, 1440,  "2560x1440" },
    { 3440, 1440,  "3440x1440" },
    { 3840, 2160,  "3840x2160" },
};
static const int NUM_RESOLUCIONES = (int)(sizeof(RESOLUCIONES) / sizeof(RESOLUCIONES[0]));

// ============================================
// Enums de estado
// ============================================
enum EstadoJuego {
    ESTADO_MENU,
    ESTADO_JUGANDO,
    ESTADO_INSTRUCCIONES,
    ESTADO_INGRESANDO_NOMBRE,
    ESTADO_GAME_OVER,
    ESTADO_PAUSADO,
    ESTADO_TRANSICION_NIVEL,
    ESTADO_CUENTA_REGRESIVA,
    ESTADO_VICTORIA,
    ESTADO_OPCIONES,
    ESTADO_INTRO,             // cinemática de entrada al juego
    ESTADO_SELECCION_NIVEL    // menú de selección de nivel con bloqueos
};

enum TipoEnemigo {
    ENEMIGO_BASICO,
    ENEMIGO_RAPIDO,
    ENEMIGO_TANQUE,
    ENEMIGO_ZIGZAG,
    ENEMIGO_BOMBARDERO,
    ENEMIGO_ESPEJO,
};

enum EstadoBoss {
    BOSS_INACTIVO,
    BOSS_ENTRANDO,
    BOSS_ACTIVO,
    BOSS_ENFURECIDO,
    BOSS_MUERTO
};

enum EstadoPista {
    PISTA_NINGUNA,
    PISTA_MENU,
    PISTA_PAUSA,
    PISTA_NIVELES123,
    PISTA_NIVEL4,
    PISTA_NIVEL5,
    PISTA_GAMEOVER
};