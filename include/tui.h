#ifndef TUI_ENGINE_H_
#define TUI_ENGINE_H_

#include <stdio.h>
#include <ctype.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <termios.h>

#define PACK_RGB(r, g, b) \
	((r) << 24) + ((g) << 16) + ((b) << 8)

#define UNPACK_RGB(n) \
	(rgb_t) { ((n) >> 24) & 0xFF, ((n) >> 16) & 0xFF, ((n) >> 8) & 0xFF }

#define TUI_BUFF_SIZE 1024

#define TUI_RED     (rgb_t) { 255,   0,   0 }
#define TUI_GREEN   (rgb_t) {   0, 255,   0 }
#define TUI_BLUE    (rgb_t) {   0,   0, 255 }
#define TUI_WHITE   (rgb_t) { 255, 255, 255 }
#define TUI_BLACK   (rgb_t) {   0,   0,   0 }
#define TUI_MAGENTA (rgb_t) { 255,   0, 255 }
#define TUI_AQUA    (rgb_t) {   0, 255, 255 }
#define TUI_YELLOW  (rgb_t) { 255, 255,   0 }
#define TUI_ORANGE  (rgb_t) { 255, 165,   0 }
#define TUI_PURPLE  (rgb_t) { 128,   0, 128 }
#define TUI_GRAY    (rgb_t) {  36,  36,  36 }
#define TUI_GREY    (rgb_t) {  36,  36,  36 }

typedef struct {
	int x, y;
} vec2i_t;

typedef struct {
	unsigned char r, g, b;
} rgb_t;

typedef struct {
	int fg_color, bg_color;
	char c;
} pixel_t;

typedef struct {
	struct timespec last;	
	float elapsed;
	float dt;
} time_manager;

typedef struct {
	char *data;
	size_t size;
	size_t capacity;
} buf_t;

typedef struct {
	pixel_t *front_buf;
	pixel_t *back_buf;
	int width, height;
	volatile sig_atomic_t resized; 
} window_t;

typedef struct {
	vec2i_t size;
	vec2i_t pos;
	rgb_t color;
} rectangle_t;

typedef struct {
	const char *cstr;
	vec2i_t pos;
	rgb_t color;
} text_t;

typedef struct {
	int value; // you will rather use
	int raw; // in a specific case
} input_t;

typedef enum {
	TUI_A = 'A', 
	TUI_B,	
	TUI_C,
	TUI_D, 		 
	TUI_E,	
	TUI_F,
	TUI_G,		 
	TUI_H,	
	TUI_I,
	TUI_J,		 
	TUI_K,	
	TUI_L,
	TUI_M,		 
	TUI_N,	
	TUI_O,
	TUI_P,		 
	TUI_Q,	
	TUI_R,
	TUI_S,		 
	TUI_T,	
	TUI_U,
	TUI_V,		 
	TUI_W,	
	TUI_X,
	TUI_Y,		 
	TUI_Z, 

	TUI_UP = 1111,
	TUI_DOWN, 
	TUI_RIGHT, 
	TUI_LEFT,
	TUI_ESCAPE,

	TUI_ENTER = 13,	
	TUI_SPACE = 32,
	TUI_BACKSPACE = 127
} key_e;

buf_t buf_create(const size_t);
void  buf_str(buf_t *, const char *);

vec2i_t tui_termsize(void);
void    tui_clear_backbuf(void);
void 	tui_clear_frontbuf(void);
void 	tui_init(void);
void 	tui_exit(void);
void 	tui_draw(void);
void 	tui_rectangle(const rectangle_t);
void 	tui_text(const text_t);
void    tui_pixel(const int, const int, const char, const rgb_t);
void 	tui_resize(void);
void 	tui_update(void);
input_t tui_input(void);
float 	tui_elapsed(void);
float 	tui_dt(void);
void 	tui_signal(int);
void    tui_sleepms(const long);

#if defined(TUI_ENGINE_IMPLEMENTATION)

static window_t *window = NULL;
static time_manager tm = { 0 };
static struct termios orig_termios;
static struct sigaction sigact ;

buf_t buf_create(size_t capacity) {
	return (buf_t) {
		.data = malloc(capacity),
		.capacity = capacity,
		.size = 0
	};
}

void buf_str(buf_t *buf, const char *cstr) {
	while ( *cstr ) {
		if ( buf->size >= buf->capacity ) {

			if ( buf->capacity == 0 ) {
				buf->capacity = TUI_BUFF_SIZE;					
			} else {
				buf->capacity *= 2;	
			}

			char *tmp = realloc(buf->data, buf->capacity);

			if ( tmp == NULL ) {
				fprintf(stderr, 
					"realloc() failed: %s\n", strerror(errno));
				return;
			}

			buf->data = tmp;
		}

		buf->data[buf->size++] = *cstr++;
	}
}

vec2i_t tui_termsize(void) {
	struct winsize ws;	

	if ( ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) != 0 &&
	    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0 &&
		ioctl(STDERR_FILENO, TIOCGWINSZ, &ws) != 0 ) {
		fprintf(stderr,
			"ioctl() failed: %s\n", strerror(errno));
		return (vec2i_t) { 0, 0 };
	}
	
	return (vec2i_t) {
		.x = ws.ws_col,	
		.y = ws.ws_row
	};
}

void tui_clear_backbuf(void) {
	for (int y = 0; y < window->height; ++y) {
		for (int x = 0; x < window->width; ++x) {
			window->back_buf[y * window->width + x] = (pixel_t) {
				.fg_color = -1,
				.bg_color = -1,
				.c = ' '
			}; 
		}
	}
}

void tui_clear_frontbuf(void) {
	for (int y = 0; y < window->height; ++y) {
		for (int x = 0; x < window->width; ++x) {
			window->front_buf[y * window->width + x] = (pixel_t) {
				.fg_color = -1,
				.bg_color = -1,
				.c = 0
			}; 
		}
	}
}

void tui_init(void) {
	printf("\x1b[?25l");
	printf("\x1b[?7l");
	printf("\x1b[1;1H\x1b[2J");
	
	fflush(stdout);

	vec2i_t size = tui_termsize();

	if ( size.x <= 0 || size.y <= 0 ) return;

	window = malloc(sizeof(window_t));

	window->width = size.x;
	window->height = size.y;

	window->resized = 0;

	const int buf_size = sizeof(pixel_t) * size.x * size.y;

	window->front_buf = malloc(buf_size);
	window->back_buf = malloc(buf_size);

	tui_clear_frontbuf();
	tui_clear_backbuf();

	timespec_get(&tm.last, TIME_MONOTONIC);

	sigact.sa_handler = tui_signal;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigaction(SIGWINCH, &sigact, (struct sigaction *)NULL);

	tcgetattr(STDIN_FILENO, &orig_termios);

	struct termios raw = orig_termios;
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 0;

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void tui_exit(void) {
	sigemptyset(&sigact.sa_mask);

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);

	printf("\x1b[?25h");
	printf("\x1b[?7h");
	printf("\x1b[0m");
	printf("\x1b[1;1H\x1b[2J");
	fflush(stdout);

	free(window->front_buf);
	free(window->back_buf);
	free(window);
}

void tui_draw(void) {
	const int buf_size = sizeof(pixel_t) * window->width * window->height;

	int term_bg = -2;
	int term_fg = -2;

	buf_t buf = buf_create(TUI_BUFF_SIZE);

	char to_write[TUI_BUFF_SIZE] = { 0 };

	for (int y = 0; y < window->height; ++y) {
		for (int x = 0; x < window->width; ++x) {

			const int index = y * window->width + x;
		
			const pixel_t front_pixel = window->front_buf[index];
			const pixel_t back_pixel = window->back_buf[index];

			if ( front_pixel.c != back_pixel.c ||
				 front_pixel.bg_color != back_pixel.bg_color ||
				 front_pixel.fg_color != back_pixel.fg_color ) {

				if ( back_pixel.bg_color != term_bg ) {

					if ( back_pixel.bg_color == -1 ) {
						buf_str(&buf, "\x1b[0m");	
					} else {
						const rgb_t rgb = UNPACK_RGB(back_pixel.bg_color);	

						snprintf(to_write, TUI_BUFF_SIZE, 
							"\x1b[48;2;%d;%d;%dm", rgb.r, rgb.g, rgb.b);
						buf_str(&buf, to_write);
					}

					term_bg = back_pixel.bg_color;
				}	

				if ( back_pixel.fg_color != term_fg ) {
					if ( back_pixel.fg_color == -1 && 
					     back_pixel.bg_color == -1 ) {
						buf_str(&buf, "\x1b[0m");
					} else {
						const rgb_t rgb = UNPACK_RGB(back_pixel.fg_color);	

						snprintf(to_write, TUI_BUFF_SIZE, 
							"\x1b[38;2;%d;%d;%dm", rgb.r, rgb.g, rgb.b);
						buf_str(&buf, to_write);
					}
				
					term_fg = back_pixel.fg_color;
				}

				snprintf(to_write, TUI_BUFF_SIZE, 
						"\x1b[%d;%dH%c", y + 1, x + 1, back_pixel.c);	
				buf_str(&buf, to_write);
			}

		}
	}

	write(STDOUT_FILENO, buf.data, buf.size);
	free(buf.data);

	memcpy(window->front_buf, window->back_buf, buf_size);
}

void tui_rectangle(const rectangle_t rec) {
	int pos_x = rec.pos.x > 0 ? rec.pos.x : 0 ;	
	int pos_y = rec.pos.y > 0 ? rec.pos.y : 0 ;

	int end_x = pos_x + rec.size.x;
	int end_y = pos_y + rec.size.y;

	for (int y = pos_y; y < end_y && y < window->height; ++y) {
		for (int x = pos_x; x < end_x && x < window->width; ++x) {
			pixel_t *pixel  = &window->back_buf[y * window->width + x];
			pixel->bg_color = PACK_RGB(rec.color.r, rec.color.g, rec.color.b);
			pixel->c = ' ';
		}
	}
}

void tui_text(const text_t text) {
	int pos_x = text.pos.x > 0 ? text.pos.x : 0 ;
	int pos_y = text.pos.y > 0 ? text.pos.y : 0 ;
	
	if ( pos_x >= window->width || pos_y >= window->height ) return;	

	const char *cstr = text.cstr;

	int n = strlen(cstr);

	int end_x = pos_x + n;

	for (int x = pos_x; x < end_x && x < window->width; ++x) {
		pixel_t *pixel = &window->back_buf[pos_y * window->width + x];
		pixel->fg_color = PACK_RGB(text.color.r, text.color.g, text.color.b);
		pixel->c = *cstr++;
	}
}

void tui_pixel(const int x, const int y, const char c, rgb_t color) {
	int pos_x = x > 0 ? x : 0 ;	
	int pos_y = y > 0 ? y : 0 ;

	if ( pos_x >= window->width || pos_y >= window->height ) return;	

	pixel_t *pixel = &window->back_buf[pos_y * window->width + pos_x];

	if ( c == ' ' ) {
		pixel->bg_color = PACK_RGB(color.r, color.g, color.b);
	} else {
		pixel->fg_color = PACK_RGB(color.r, color.g, color.b);	
	}
	
	pixel->c = c;
}

void tui_resize(void) {
	vec2i_t size = tui_termsize();

	if ( size.x <= 0 || size.y <= 0 ) return;

	window->width = size.x;
	window->height = size.y;

	const int buf_size = sizeof(pixel_t) * window->width * window->height;
	
	free(window->front_buf);
	free(window->back_buf);

	window->front_buf = malloc(buf_size);
	window->back_buf = malloc(buf_size);

	tui_clear_frontbuf();
	tui_clear_backbuf();

	printf("\x1b[1;1H\x1b[2J"); 
    fflush(stdout);
	
	window->resized = 0;
}

void tui_update(void) {
	if ( window->resized ) {
		tui_resize();
	} else {
		tui_clear_backbuf();
	}
	
	struct timespec now;
	timespec_get(&now, TIME_MONOTONIC);

	tm.dt = (now.tv_sec - tm.last.tv_sec) +
		    (now.tv_nsec - tm.last.tv_nsec) / 1e9;

	tm.elapsed += tm.dt;		

	tm.last = now;
}

input_t tui_input(void) {

	int raw = 0 ;
	read(STDIN_FILENO, &raw, 1);
	
	int value = toupper(raw);

	if ( value == 27 ) {
		read(STDIN_FILENO, &value, 1);

		if ( value == '[' ) {
			read(STDIN_FILENO, &value, 1);

			switch ( value ) {
				case 'A': value = TUI_UP; 	 break;
				case 'B': value = TUI_DOWN;  break;
				case 'C': value = TUI_RIGHT; break;
				case 'D': value = TUI_LEFT;  break;
			}
		} else {
			value = TUI_ESCAPE;
		} 
	}
	
	return (input_t) {
		.value = value,
		.raw = raw
	};
}

float tui_elapsed(void)   { return tm.elapsed; } 
float tui_dt(void)        { return tm.dt;      }

void tui_signal(int sig) {
	if ( SIGWINCH == sig ) {
		window->resized = 1;
	}
}

void tui_sleepms(const long ms) {
	struct timespec t1, t2;	

	t1.tv_sec  = (int)ms / 1000;
	t1.tv_nsec = (ms % 1000) * 1e6;

	nanosleep(&t1, &t2);	
}

#endif // TUI_ENGINE_IMPLEMENTATION

#endif
