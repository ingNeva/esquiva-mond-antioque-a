# ============================================
# Makefile - Esquivar Botellas
# Uso: make          -> compila debug
#      make release  -> compila optimizado
#      make clean    -> borra objetos y binario
# ============================================

CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -MMD -MP \
            $(shell pkg-config --cflags sdl3 sdl3-image sdl3-ttf sdl3-mixer)
LIBS     := $(shell pkg-config --libs   sdl3 sdl3-image sdl3-ttf sdl3-mixer)
TARGET   := EsquivarBotellas

# Modo debug por defecto
DEBUG_FLAGS   := -g -O0 -DDEBUG
RELEASE_FLAGS := -O2 -DNDEBUG

OBJDIR := build

SRCS := main.cpp \
        utils/ScoreManager.cpp \
        core/AudioManager.cpp \
        core/Game.cpp \
        core/InputManager.cpp \
        core/World.cpp \
        entities/Player.cpp \
        entities/Machete.cpp \
        entities/Enemy.cpp \
        entities/Boss.cpp \
        entities/Llave.cpp \
        scenes/GameScene.cpp \
        scenes/MenuScene.cpp \
        scenes/CountdownScene.cpp \
        scenes/GameOverScene.cpp \
        scenes/OptionsScene.cpp

OBJS := $(patsubst %.cpp, $(OBJDIR)/%.o, $(SRCS))
DEPS := $(OBJS:.o=.d)

# ── Regla por defecto: debug ─────────────────
all: CXXFLAGS += $(DEBUG_FLAGS)
all: $(TARGET)

# ── Release ──────────────────────────────────
release: CXXFLAGS += $(RELEASE_FLAGS)
release: $(TARGET)

# ── Enlazado ─────────────────────────────────
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) $(LIBS) -o $@
	@echo "==> Compilado: $@"

# ── Compilacion de cada .cpp ─────────────────
$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ── Incluir dependencias generadas (-MMD) ────
-include $(DEPS)

# ── Limpieza ─────────────────────────────────
clean:
	rm -rf $(OBJDIR) $(TARGET)
	@echo "==> Limpio"

.PHONY: all release clean
