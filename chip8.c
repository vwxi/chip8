/*
 * chip-8 emulator without sound
 * written by pala <pala@tilde.institute>
 */

#if defined(_WIN32) || defined(_WIN64)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <SDL.h>

/* macros */
#define WIDTH    640
#define HEIGHT   480
#define ROWS      64
#define COLS      32
#define MAXSZ  0xE00

#define NNN(i) ( i & 0xFFF      )
#define N(i)   ( i & 0xF        )
#define X(i)   ((i & 0xF00) >> 8)
#define Y(i)   ((i & 0xF0 ) >> 4)
#define KK(i)  ( i & 0xFF       )

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef unsigned long long u64;

/* graphics */

const u8 keys[16] = {
	SDLK_1, SDLK_2, SDLK_3, SDLK_4,
	SDLK_q, SDLK_w, SDLK_e, SDLK_r,
	SDLK_a, SDLK_s, SDLK_d, SDLK_f,
	SDLK_z, SDLK_x, SDLK_c, SDLK_v
};

struct {
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* texture;
} gfx;

u8   gfx_init();
void gfx_destroy();
void gfx_draw();
void gfx_loop();

/* system */

const u8 font[80] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0,
	0x20, 0x60, 0x20, 0x20, 0x70,
	0xF0, 0x10, 0xF0, 0x80, 0xF0,
	0xF0, 0x10, 0xF0, 0x10, 0xF0,
	0x90, 0x90, 0xF0, 0x10, 0x10,
	0xF0, 0x80, 0xF0, 0x10, 0xF0,
	0xF0, 0x80, 0xF0, 0x90, 0xF0,
	0xF0, 0x10, 0x20, 0x40, 0x40,
	0xF0, 0x90, 0xF0, 0x90, 0xF0,
	0xF0, 0x90, 0xF0, 0x10, 0xF0,
	0xF0, 0x90, 0xF0, 0x90, 0x90,
	0xE0, 0x90, 0xE0, 0x90, 0xE0,
	0xF0, 0x80, 0x80, 0x80, 0xF0,
	0xE0, 0x90, 0x90, 0x90, 0xE0,
	0xF0, 0x80, 0xF0, 0x80, 0xF0,
	0xF0, 0x80, 0xF0, 0x80, 0x80
};

struct {
	u16  I;
	u8   DT;
	u8   ST;
	u16  PC;
	u8   SP;
	u8   RAM[0x1000];
	u16  op;
	u8   V[0x10];
	u16  stack[0x10];
	u32  display[0x800];
	u8   keys[0x10];
	u8   draw;
} c8;

void c8_init();
void c8_tick();

/* insns */

void op_0000();
void op_1NNN();
void op_2NNN();
void op_3XNN();
void op_4XNN();
void op_5XY0();
void op_9XY0();
void op_6XNN();
void op_7XNN();
void op_8XYN();
void op_ANNN();
void op_BNNN();
void op_CXNN();
void op_DXYN();
void op_EXNN();
void op_FXNN();
void op_UNKN();

void (*insn_table[16])() = {
	op_0000,
	op_1NNN,
	op_2NNN,
	op_3XNN,
	op_4XNN,
	op_5XY0,
	op_6XNN,
	op_7XNN,
	op_8XYN,
	op_9XY0,
	op_ANNN,
	op_BNNN,
	op_CXNN,
	op_DXYN,
	op_EXNN,
	op_FXNN
};

/* routines */

/* insns */ 
void
op_UNKN()
{
	fprintf(stderr,
		"WARNING: "
		"Unknown operation\n");
}

void
op_0000()
{	
	switch (c8.op) {
	case 0x0000:
		op_UNKN();
		break;

	case 0x00e0:
		memset(c8.display, 0, sizeof(c8.display));
		break;

	case 0x00ee:
		c8.PC = c8.stack[--c8.SP % 16];
		break;
	}
}

void
op_1NNN()
{
	c8.PC = NNN(c8.op);
}

void
op_2NNN()
{
	c8.stack[c8.SP++ % 16] = c8.PC;
	c8.PC = NNN(c8.op);
}

void
op_3XNN()
{
	if (c8.V[X(c8.op)] == KK(c8.op))
		c8.PC += 2;
}

void
op_4XNN()
{
	if (c8.V[X(c8.op)] != KK(c8.op))
		c8.PC += 2;
}

void
op_5XY0()
{
	if (c8.V[X(c8.op)] == c8.V[Y(c8.op)])
		c8.PC += 2;
}

void
op_6XNN()
{
	c8.V[X(c8.op)] = KK(c8.op);
}

void
op_7XNN()
{
	c8.V[X(c8.op)] += KK(c8.op);
}

void
op_8XYN()
{
	u16 s = 0;
	switch (c8.op & 0xf) {
	case 0:
		c8.V[X(c8.op)] = c8.V[Y(c8.op)];
		break;

	case 1:
		c8.V[X(c8.op)] |= c8.V[Y(c8.op)];
		break;

	case 2:
		c8.V[X(c8.op)] &= c8.V[Y(c8.op)];
		break;

	case 3:
		c8.V[X(c8.op)] ^= c8.V[Y(c8.op)];
		break;

	case 4:
		s = c8.V[X(c8.op)] + c8.V[Y(c8.op)];
		c8.V[0xF] = (s > 255);
		c8.V[X(c8.op)] = s & 0xff;
		break;

	case 5:
		c8.V[0xF] = (c8.V[X(c8.op)] > c8.V[Y(c8.op)]);
		c8.V[X(c8.op)] -= c8.V[Y(c8.op)];
		break;

	case 6:
		c8.V[X(c8.op)] = c8.V[Y(c8.op)];
		c8.V[0xF] = (c8.V[X(c8.op)] & 1);
		c8.V[X(c8.op)] >>= 1;
		break;

	case 7:
		c8.V[0xF] = (c8.V[Y(c8.op)] > c8.V[X(c8.op)]);
		c8.V[X(c8.op)] = c8.V[Y(c8.op)] - c8.V[X(c8.op)];
		break;

	case 0xE:
		c8.V[0xF] = (c8.V[X(c8.op)] & 0x80) >> 7;
		c8.V[X(c8.op)] <<= 1;
		break;
	}
}

void
op_9XY0()
{
	if (c8.V[X(c8.op)] != c8.V[Y(c8.op)])
		c8.PC += 2;
}

void
op_ANNN()
{
	c8.I = NNN(c8.op);
}

void
op_BNNN()
{
	c8.PC = NNN(c8.op) + c8.V[0];
}

void
op_CXNN()
{
	c8.V[X(c8.op)] = (rand() % 255) & KK(c8.op);
}

void
op_DXYN()
{
	u64 x, y;
	u8 xpos, ypos, height, sprite;
	u32 *pixel = NULL;
	
	c8.draw = 1;

	xpos = c8.V[X(c8.op)] % ROWS;
	ypos = c8.V[Y(c8.op)] % COLS;
	height = N(c8.op);

	c8.V[0xF] = 0;

	for (y = 0; y < height; y++) {
		sprite = c8.RAM[c8.I + y];
		for (x = 0; x < 8; x++) {
			pixel = &c8.display[(ypos + y) * ROWS + (xpos + x)];
			if (sprite & (0x80 >> x)) {
				if (*pixel)
					c8.V[0xF] = 1;

				*pixel ^= 1;
			}
		}
	}
}

void
op_EXNN()
{
	switch (c8.op & 0xf0ff) {
	case 0xE09E:
		if (c8.keys[c8.V[X(c8.op)]])
			c8.PC += 2;
		break;

	case 0xE0A1:
		if (!c8.keys[c8.V[X(c8.op)]])
			c8.PC += 2;
		break;
	}
}

void
op_FXNN()
{
	u8 i;
	
	switch (c8.op & 0xff) {
	case 0x07:
		c8.V[X(c8.op)] = c8.DT;
		break;

	case 0x15:
		c8.DT = c8.V[X(c8.op)];
		break;

	case 0x18:
		c8.ST = c8.V[X(c8.op)];
		break;

	case 0x1E:
		c8.I += c8.V[X(c8.op)];
		c8.V[0xF] = c8.I > 0xFFF;
		break;

	case 0x0A:
		for (i = 0; i < 16; i++) {
			if (c8.keys[i]) {
				c8.V[X(c8.op)] = i;
				c8.PC += 4;
				break;
			}
		}

		c8.PC -= 2;
		break;

	case 0x29:
		c8.I = c8.V[X(c8.op)] * 5;
		break;

	case 0x33:
		c8.RAM[c8.I] = c8.V[X(c8.op)] / 100;
		c8.RAM[c8.I + 1] = (c8.V[X(c8.op)] / 10) % 10;
		c8.RAM[c8.I + 2] = (c8.V[X(c8.op)] % 100) % 10;
		break;

	case 0x55:
		for (i = 0; i < X(c8.op); i++) {
			c8.RAM[c8.I + i] = c8.V[i];
		}

		break;

	case 0x65:
		for (i = 0; i < X(c8.op); i++) {
			c8.V[i] = c8.RAM[c8.I + i];
		}

		break;
	}
}

/* system */

void
c8_init()
{
	u8 i;
	
	c8.PC = 0x200;
	c8.DT = 0;
	c8.ST = 0;
	c8.I = 0;
	c8.SP = 0;
	c8.draw = 0;

	memset(c8.display, 0, sizeof(c8.display));
	memset(c8.RAM, 0, sizeof(c8.RAM));
	memset(c8.V, 0, sizeof(c8.V));
	memset(c8.keys, 0, sizeof(c8.keys));
	memset(c8.stack, 0, sizeof(c8.stack));

	for (i = 0; i < 80; i++)
		c8.RAM[i] = font[i];

	srand((unsigned int)time(NULL));
}

void
c8_tick()
{
	c8.op = (c8.RAM[c8.PC] << 8) | c8.RAM[c8.PC + 1];
	
	c8.PC += 2;

	insn_table[(c8.op & 0xf000) >> 12]();

	if (c8.DT > 0) c8.DT -= 1;
	if (c8.ST > 0) c8.ST -= 1;
}

/* graphics */

u8 
gfx_init()
{
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		fprintf(stderr,
			"ERROR: "
			"Cannot initialize SDL, "
			"reason: %s\n",
			SDL_GetError());
		return 0;
	}

	gfx.window = SDL_CreateWindow(
		"chip8 interpreter",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		WIDTH,
		HEIGHT,
		0
	);

	if (!gfx.window) {
		fprintf(stderr,
			"ERROR: "
			"Cannot create window, "
			"reason: %s\n",
			SDL_GetError());
		return 0;
	}

	gfx.renderer = SDL_CreateRenderer(
		gfx.window,
		-1,
		SDL_RENDERER_ACCELERATED |
		SDL_RENDERER_PRESENTVSYNC
	);

	if (!gfx.renderer) {
		fprintf(stderr,
			"ERROR: "
			"Cannot create renderer, "
			"reason: %s\n",
			SDL_GetError());
		return 0;
	}

	SDL_SetRenderDrawColor(gfx.renderer, 50, 255, 155, 255);
	SDL_RenderClear(gfx.renderer);

	return 1;
}

void
gfx_destroy()
{
	SDL_DestroyRenderer(gfx.renderer);
	SDL_DestroyWindow(gfx.window);
	SDL_Quit();
}

void
gfx_draw()
{
	int i, j;
	SDL_Rect rect;

	SDL_RenderClear(gfx.renderer);
	SDL_SetRenderDrawColor(
		gfx.renderer, 
		50, 
		255, 
		155,
		255);

	c8.draw = 0;

	rect.w = 10;
	rect.h = 10;

	for (j = 0; j < COLS; j++) {
		for (i = 0; i < ROWS; i++) {
			rect.x = i * 10;
			rect.y = j * 10;

			if (c8.display[i + j * ROWS]) {
				SDL_SetRenderDrawColor(
					gfx.renderer,
					255,
					255,
					255,
					255
				);
			}
			else {
				SDL_SetRenderDrawColor(
					gfx.renderer,
					0,
					0,
					0,
					255
				);
			}

			SDL_RenderFillRect(
				gfx.renderer,
				&rect
			);
		}
	}

	SDL_RenderPresent(gfx.renderer);
}

void
gfx_loop()
{
	int i;
	SDL_Event e;

	SDL_SetRenderDrawColor(
		gfx.renderer, 
		0,
		0, 
		0,
		255
		);

	SDL_RenderClear(gfx.renderer);

	for (;;) {
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
			case SDL_QUIT: return;
			case SDL_KEYDOWN:
				for (i = 0; i < 16; i++) {
					if ((c8.keys[i] = (e.key.keysym.sym == keys[i])))
						break;
				}

				break;

			case SDL_KEYUP:
				for (i = 0; i < 16; i++) {
					if (keys[i] == e.key.keysym.sym) {
						if (c8.keys[i]) c8.keys[i] = 0;
					}
				}

				break;
			}
		}

		c8_tick();
		
		if (c8.draw) gfx_draw();

		SDL_Delay(10);
	}
}

/* main */

int 
main(int argc, char** argv)
{
	long fpsz;
	FILE* fp = NULL;

	if (argc != 2) {
		fprintf(stderr,
			"USAGE: "
			"%s [ROM file]\n",
			argv[0]);
		return 1;
	}

	c8_init();

	fp = fopen(argv[1], "rb");
	if (!fp) {
		fprintf(stderr,
			"ERROR: "
			"Could not open ROM file.\n");
		return 1;
	}

	fseek(fp, 0, SEEK_END);

	fpsz = ftell(fp);
	if (fpsz > MAXSZ) {
		fprintf(stderr,
			"ERROR: "
			"ROM too big.\n");
		fclose(fp);
		return 1;
	}

	fseek(fp, 0, SEEK_SET);
	
	if (!fread(&c8.RAM[0x200], 1, fpsz, fp)) {
		fprintf(stderr,
			"ERROR: "
			"Could not read ROM.\n");
		fclose(fp);
		return 1;
	}

	fclose(fp);

	if (!gfx_init()) {
		fprintf(stderr,
			"ERROR: "
			"Could not create gfx.\n");
		return 1;
	}

	gfx_loop();

	gfx_destroy();

	return 0;
}
