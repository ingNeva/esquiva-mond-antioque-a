#include "ScoreManager.h"
#include <cstdio>
#include <cmath>

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
    juego->puntuacion += puntosReales;
    juego->puntosEnNivel += puntosReales;

    for (int i = 0; i < MAX_FLOATING_TEXT; i++) {
        if (!juego->floatingTexts[i].activo) {
            FloatingText& ft = juego->floatingTexts[i];
            ft.x = x; ft.y = y; ft.valor = puntosReales;
            ft.timer = FLOATING_TEXT_DURACION; ft.activo = true;
            if      (juego->combo >= 20) { ft.colorR=0.8f; ft.colorG=0.2f; ft.colorB=1.0f; }
            else if (juego->combo >= 10) { ft.colorR=1.0f; ft.colorG=0.1f; ft.colorB=0.1f; }
            else if (juego->combo >=  5) { ft.colorR=1.0f; ft.colorG=0.5f; ft.colorB=0.0f; }
            else if (juego->combo >=  3) { ft.colorR=1.0f; ft.colorG=0.9f; ft.colorB=0.0f; }
            else                         { ft.colorR=1.0f; ft.colorG=1.0f; ft.colorB=1.0f; }
            break;
        }
    }
}

void actualizarFloatingTexts(Juego* juego) {
    for (int i = 0; i < MAX_FLOATING_TEXT; i++) {
        if (!juego->floatingTexts[i].activo) continue;
        juego->floatingTexts[i].y -= 0.6f;
        juego->floatingTexts[i].timer--;
        if (juego->floatingTexts[i].timer <= 0) juego->floatingTexts[i].activo = false;
    }
}
