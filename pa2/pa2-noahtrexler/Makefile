# define CPPFLAGS=-I... for other (system) includes
# define LDFLAGS=-L... for other (system) libs to link

CC = g++ -g -Wno-float-conversion -Wno-narrowing -Wreturn-type -Wunused-function -Wreorder -Wunused-variable

CC_DEBUG = @$(CC) -std=c++11
CC_RELEASE = @$(CC) -std=c++11 -O3 -DNDEBUG

G_SRC = src/*.cpp *.cpp
G_DEPS = *.cpp Makefile

G_INC = -Iinclude -Iapps $(CPPFLAGS)

G_LINK = $(LDFLAGS)

all: image bench

image : $(G_SRC) $(G_DEPS) apps/image.cpp apps/image_recs.cpp
	$(CC_DEBUG) $(G_INC) $(G_SRC) apps/image.cpp apps/image_recs.cpp -o image

bench : $(G_SRC) $(G_DEPS) apps/bench.cpp apps/bench_recs.cpp apps/GTime.cpp
	$(CC_RELEASE) $(G_INC) $(G_SRC) apps/GTime.cpp apps/bench.cpp apps/bench_recs.cpp -o bench

DRAW_SRC = apps/draw.cpp apps/GWindow.cpp apps/GTime.cpp
draw: $(DRAW_SRC) $(G_SRC) $(G_DEPS)
	$(CC_RELEASE) $(G_INC) $(G_SRC) $(DRAW_SRC) $(G_LINK) -lSDL2 -o draw

PAINT_SRC = apps/paint.cpp apps/GWindow.cpp apps/GTime.cpp
paint: $(PAINT_SRC) $(G_SRC) $(G_DEPS)
	$(CC_RELEASE) $(G_INC) $(G_SRC) $(PAINT_SRC) $(G_LINK) -lSDL2 -o paint

VIEWER_SRC = apps/viewer.cpp apps/GWindow.cpp apps/GTime.cpp apps/image_recs.cpp
viewer: $(VIEWER_SRC) $(G_SRC) $(G_DEPS)
	$(CC_RELEASE) $(G_INC) $(G_SRC) $(VIEWER_SRC) $(G_LINK) -lSDL2 -o viewer


BOUNCE_SRC = apps/bounce.cpp apps/GWindow.cpp apps/GTime.cpp
bounce: $(BOUNCE_SRC) $(G_SRC) $(G_DEPS)
	$(CC_RELEASE) $(G_INC) $(G_SRC) $(BOUNCE_SRC) $(G_LINK) -lSDL2 -o bounce


clean:
	@rm -rf image draw paint viewer bounce bench *.png *.dSYM

