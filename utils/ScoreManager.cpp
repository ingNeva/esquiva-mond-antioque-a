#include "ScoreManager.h"
#include "../entities/Enemy.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <cstdlib>

#ifdef _WIN32
    #include <direct.h>
#else
    #include <sys/stat.h>
#endif

// ============================================
// Directorio de saves
// ============================================
void crearDirectorioSaves() {
#ifdef _WIN32
    _mkdir("saves");
#else
    mkdir("saves", 0755);
#endif
}

// ============================================
// Carga / guardado de puntajes
// ============================================
void cargarPuntajes(TablaPuntajes* tabla) {
    tabla->cantidad = 0;
    for (int i = 0; i < MAX_PUNTAJES; i++) {
        tabla->entradas[i].puntuacion = 0;
        tabla->entradas[i].nombre[0]  = '\0';
    }
    FILE* f = fopen(RUTA_SAVES, "rb");
    if (!f) return;
    fread(&tabla->cantidad, sizeof(int), 1, f);
    if (tabla->cantidad < 0 || tabla->cantidad > MAX_PUNTAJES) tabla->cantidad = 0;
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

// ============================================
// Frases colombianas por arma
// ============================================
static const char* FRASES_MACHETE[] = {
    "!Toma, malparido!",
    "!Eso es pa' que aprenda!",
    "!Juepucha, que hachazo!",
    "!No joda mas, mijo!",
    "!Le doy con todo, parce!",
    "!Esto es un machete paisa!",
    "!A la orden, hermano!",
    "!Que viva el campo!",
};
static const int NUM_FRASES_MACHETE = 8;

static const char* FRASES_CHANCLA[] = {
    "!Chancletazo de mi mama!",
    "!La chancla no falla, mijo!",
    "!Pa' que no vuelva, maluco!",
    "!Ay, venga esa chancleta!",
    "!Ni el diablo escapa!",
    "!Chancleta bendita, parce!",
    "!Eso le pasa por bobo!",
    "!Que cuero tan duro!",
};
static const int NUM_FRASES_CHANCLA = 8;

// ============================================
// Helper: spawna floating text de frase
// ============================================
static void spawnFrase(Juego* juego, const char* frase, float x, float y,
                       float r, float g, float b) {
    for (int i = MAX_FLOATING_TEXT - 1; i >= MAX_FLOATING_TEXT * 2 / 3; i--) {
        if (!juego->floatingTexts[i].activo) {
            FloatingText& ft = juego->floatingTexts[i];
            ft.x      = x;
            ft.y      = y - 48.0f;
            ft.valor  = 0;          // 0 indica "mostrar frase, no número"
            ft.timer  = (int)(FLOATING_TEXT_DURACION * 2.0f);
            ft.activo = true;
            ft.colorR = r;
            ft.colorG = g;
            ft.colorB = b;
            SDL_snprintf(ft.frase, sizeof(ft.frase), "%s", frase);
            break;
        }
    }
}

// ============================================
// Sistema de puntuacion con combo
// ============================================
void agregarPuntos(Juego* juego, int base, float x, float y) {
    juego->combo++;
    if (juego->combo > juego->mejorCombo) juego->mejorCombo = juego->combo;
    if      (juego->combo >= 20) juego->multiplicador = 5.0f;
    else if (juego->combo >= 10) juego->multiplicador = 3.0f;
    else if (juego->combo >=  5) juego->multiplicador = 2.0f;
    else if (juego->combo >=  3) juego->multiplicador = 1.5f;
    else                         juego->multiplicador = 1.0f;

    int puntosReales = (int)(base * juego->multiplicador);
    juego->puntuacion    += puntosReales;
    juego->puntosEnNivel += puntosReales;

    // Floating text principal — puntos
    int slotPuntos = -1;
    for (int i = 0; i < MAX_FLOATING_TEXT * 2 / 3; i++) {
        if (!juego->floatingTexts[i].activo) {
            FloatingText& ft = juego->floatingTexts[i];
            ft.x      = x;
            ft.y      = y;
            ft.valor  = puntosReales;
            ft.timer  = FLOATING_TEXT_DURACION;
            ft.activo = true;
            ft.frase[0] = '\0';
            if      (juego->combo >= 20) { ft.colorR=0.8f; ft.colorG=0.2f; ft.colorB=1.0f; }
            else if (juego->combo >= 10) { ft.colorR=1.0f; ft.colorG=0.1f; ft.colorB=0.1f; }
            else if (juego->combo >=  5) { ft.colorR=1.0f; ft.colorG=0.5f; ft.colorB=0.0f; }
            else if (juego->combo >=  3) { ft.colorR=1.0f; ft.colorG=0.9f; ft.colorB=0.0f; }
            else                         { ft.colorR=1.0f; ft.colorG=1.0f; ft.colorB=1.0f; }
            slotPuntos = i;
            break;
        }
    }

    // Floating text secundario — racha si hay combo >= 2
    if (slotPuntos >= 0 && juego->combo >= 2) {
        for (int i = slotPuntos + 1; i < MAX_FLOATING_TEXT * 2 / 3; i++) {
            if (!juego->floatingTexts[i].activo) {
                FloatingText& fr = juego->floatingTexts[i];
                fr.x      = x;
                fr.y      = y + 30.0f;  // debajo del número
                fr.valor  = -(juego->combo); // negativo = mostrar "racha X"
                fr.timer  = FLOATING_TEXT_DURACION;
                fr.activo = true;
                fr.frase[0] = '\0';
                // Color más oscuro que el de puntos
                FloatingText& fp = juego->floatingTexts[slotPuntos];
                fr.colorR = fp.colorR * 0.75f;
                fr.colorG = fp.colorG * 0.75f;
                fr.colorB = fp.colorB * 0.75f;
                break;
            }
        }
    }
}

void actualizarFloatingTexts(Juego* juego) {
    for (int i = 0; i < MAX_FLOATING_TEXT; i++) {
        if (!juego->floatingTexts[i].activo) continue;
        juego->floatingTexts[i].y -= 0.8f;
        juego->floatingTexts[i].timer--;
        if (juego->floatingTexts[i].timer <= 0)
            juego->floatingTexts[i].activo = false;
    }
}

// ============================================
// Esquive cercano — bonus dopamínico
// ============================================
void mundoOnEsquiveCercano(Juego* juego, Enemigo* en) {
    int pts;
    switch (en->tipo) {
        case ENEMIGO_ESPEJO:     pts = 8; break;
        case ENEMIGO_BOMBARDERO: pts = 6; break;
        case ENEMIGO_ZIGZAG:     pts = 5; break;
        case ENEMIGO_RAPIDO:     pts = 4; break;
        default:                 pts = PTS_ESQUIVAR_NORMAL; break;
    }
    if      (en->distMinAlcanzada < 25.0f) pts += 4;
    else if (en->distMinAlcanzada < 50.0f) pts += 2;
    else if (en->distMinAlcanzada < 80.0f) pts += 1;

    float fx = en->rect.x + en->rect.w * 0.5f;
    float fy = en->rect.y + en->rect.h * 0.5f;

    agregarPuntos(juego, pts, fx, fy);

    // Sobreescribir color del número a cyan para distinguir de kills
    for (int i = MAX_FLOATING_TEXT * 2 / 3 - 1; i >= 0; i--) {
        if (juego->floatingTexts[i].activo && juego->floatingTexts[i].valor > 0) {
            juego->floatingTexts[i].colorR = 0.2f;
            juego->floatingTexts[i].colorG = 0.9f;
            juego->floatingTexts[i].colorB = 1.0f;
            break;
        }
    }
}

// ============================================
// Kill con machete — puntos + frase paisa
// ============================================
void mundoOnEnemigoMuertoMachete(Juego* juego, int idx, float x, float y) {
    Enemigo* en = &juego->enemigos[idx];
    int pts;
    switch (en->tipo) {
        case ENEMIGO_ESPEJO:     pts = 10; break;
        case ENEMIGO_BOMBARDERO: pts = 8;  break;
        case ENEMIGO_ZIGZAG:     pts = 6;  break;
        case ENEMIGO_RAPIDO:     pts = 4;  break;
        default:                 pts = PTS_MATAR_NORMAL; break;
    }
    agregarPuntos(juego, pts, x, y);
    // Frase aleatoria machete — amarillo dorado
    int idx_frase = rand() % NUM_FRASES_MACHETE;
    spawnFrase(juego, FRASES_MACHETE[idx_frase], x, y, 1.0f, 0.85f, 0.1f);
    generarEnemigoConJugador(en, juego->nivelActual, &juego->jugador);
}

// ============================================
// Kill con chancla — puntos + frase paisa
// ============================================
void mundoOnEnemigoMuertoChancla(Juego* juego, int idx, float x, float y) {
    Enemigo* en = &juego->enemigos[idx];
    int pts;
    switch (en->tipo) {
        case ENEMIGO_ESPEJO:     pts = 10; break;
        case ENEMIGO_BOMBARDERO: pts = 8;  break;
        case ENEMIGO_ZIGZAG:     pts = 6;  break;
        case ENEMIGO_RAPIDO:     pts = 4;  break;
        default:                 pts = PTS_MATAR_NORMAL; break;
    }
    agregarPuntos(juego, pts, x, y);
    // Frase aleatoria chancla — cian
    int idx_frase = rand() % NUM_FRASES_CHANCLA;
    spawnFrase(juego, FRASES_CHANCLA[idx_frase], x, y, 0.1f, 0.9f, 1.0f);
    generarEnemigoConJugador(en, juego->nivelActual, &juego->jugador);
}
