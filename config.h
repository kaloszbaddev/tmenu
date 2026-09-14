#ifndef CONFIG_H_
#define CONFIG_H_

static const char *color_scheme[] = {
	"#bbbbbb", /* normal foreground */
	"#222222", /* normal background */
	"#eeeeee", /* current foreground */
	"#005577"  /* current background */
};

static int menu_width  = 30;
static int lines_count = 10;

static const char *ignored_characters = " "; 
static int ignore_case = 0;

#endif
