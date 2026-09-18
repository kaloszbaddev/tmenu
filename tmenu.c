#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

#include "tui.h"
#include "tmenu.h"
#include "tconfig.h"

static char buff[BUFF_SIZE] = {0};
static struct item *items = NULL;
static struct node *root = NULL;
static int bpos = 0, lbpos = 0, ipos = 0, icap = 0;
static int status = 0, cursor = 0, offset = 0;
static struct item target = {0};
static vec3i_t color_set[colors_count] = {0};

static int tmenu_usage(void) {
	fprintf(stderr, "<Usage> ./tmenu <?option>\n"
				"tmenu options:\n"
				"-nf <string>, --normal-foreground=<string> (change foreground color)\n" 
				"-nb <string>, --normal-background=<string> (change background color)\n" 
				"-cf <string>, --current-foreground=<string> (change current foreground color)\n" 
				"-cb <string>, --current-background=<string> (change current background color)\n" 
				"-w <number>, --width=<number> (change menu width)\n"
				"-l <number>, --lines=<number> (change lines count)\n"
				"-i, --ignore-case (treat uppercase and lowercase letters equal)\n"
				"-d, --dump, (outputs default config)\n"
				"-h, --help        (show help message)\n");
	return 0;
}

static int tmenu_dump(void) {
	fprintf(stdout,	"#ifndef TCONFIG_H_\n"
				"#define TCONFIG_H_\n"
				"static const char *color_scheme[] = {\n"
				"	\"#bbbbbb\", /* normal foreground */\n"
				"	\"#222222\", /* normal background */\n"
				"	\"#eeeeee\", /* current foreground */\n"
				"	\"#005577\"  /* current background */\n"
				"};\n"
				"static int menu_width = 30;\n"
				"static int lines_count = 10;\n"
				"static const char *ignored_characters = \" \";\n" 
				"static int ignore_case = 0;\n"
				"#endif");
	return 0;
}

int parse_color(vec3i_t *color, const char *cstr) {
	if ( *cstr++ != '#' )
		return 0;

	if ( *cstr == '\0' )
		return 0;

	int n, i, pos, value;	

	for (i = n = pos = 0; cstr[i] != '\0' && i < 6; ++i) {
		if ( cstr[i] >= '0' && cstr[i] <= '9' )
			value = cstr[i] - '0';
		else if ( cstr[i] >= 'a' && cstr[i] <= 'f' ) 
			value = cstr[i] - 'a' + 10;
		else if ( cstr[i] >= 'A' && cstr[i] <= 'F' )
			value = cstr[i] - 'A' + 10;
		else
			return 0;	
		
		if ( i % 2 == 0 )
			n = value * 16;
		else {
			n += value;
			color->raw[pos++] = n;
		}
	}

	return 1;
}

int get_int(const char *str) {
	char *endptr;	
	long num;

	num = strtol(str, &endptr, 10);
	if ( endptr == str ) {
		printf("no digits were found\n");
		return -1;
	} else if ( *endptr != '\0' ) {
		printf("invalid character: %c\n", *endptr);
		return -1;
	}
	
	return (int )num;
}

char *value_pos(char *args[], const int argc, int *index) {
	if ( *index >= argc )
		return NULL;

	char *p = strchr(args[*index], '='); 

	if ( p != NULL ) {
		return ++p;
	} else if ( *index < argc - 1 ) {
		*index += 1;
		return args[*index];
	}
	
	return NULL;
}

int cmp_weight(const void *a, const void *b) {
	const struct item * const item_a = a;	
	const struct item * const item_b = b;

	return item_a->weight < item_b->weight;
}

void push_node(struct node **root, char *value) {
	struct node *node = malloc(sizeof(struct node));

	node->item.value = value;
	node->item.sub = strrchr(value, '/');

	if (node->item.sub != NULL)
		node->item.sub++;

	node->next = *root;
	*root = node;
}

void sort_list(struct node *root) { 
	if ( root == NULL ) 
		return;

	struct node *indirect = root;
	struct node *curr = NULL; 
	struct node *lptr = NULL;

	int swapped = 0;

	while ( indirect->next != NULL ) {

		curr = root;
		lptr = indirect;

		swapped = 0;

		while ( curr->next != NULL && lptr->next != NULL ) {
			struct node *next = curr->next;
			
			if ( strcasecmp(curr->item.sub, next->item.sub) > 0 ) {
				struct item item = curr->item;
				curr->item = next->item;
				next->item = item;
				
				swapped = 1;
			}

			curr = curr->next;
			lptr = lptr->next;
		}

		indirect = indirect->next;
		if ( !swapped ) break;
	}
}

void print_list(struct node *root) {
	while (root != NULL) {
		if ( root->item.value != NULL )	
			printf("value = %s\n", root->item.value);	
		root = root->next;
	}
}

void free_list(struct node *root) {
	if (root == NULL ) 
		return;

	struct node *curr = NULL;

	while ((curr = root) != NULL) {
		root = root->next;	
		free(curr->item.value);
		free(curr);
	}
}

int load(const char *path) {
	DIR *dir = opendir(path);

	if ( dir == NULL ) {
		fprintf(stderr, 
			"opendir failed: %s\n", strerror(errno));
		return 0;
	}

	struct dirent *entry = NULL;	
	while ( (entry = readdir(dir)) != NULL ) {
		char full_path[BUFF_SIZE] = { 0 };
		snprintf(full_path, BUFF_SIZE, "%s/%s", path, entry->d_name);

		if ( entry->d_type == DT_DIR ) {
			if ( strcmp(".", entry->d_name) != 0 && strcmp("..", entry->d_name) != 0 ) 
				load(full_path);
		} else {
			struct node *indirect = root;
			while ( indirect != NULL ) {
				char *str = strrchr(full_path, '/');
				if ( str != NULL && strcmp(++str, indirect->item.sub) == 0 )	
					break;
				indirect = indirect->next;
			}

			if ( indirect == NULL )
				push_node(&root, strdup(full_path));
		} 
			
	}

	closedir(dir);
	return 1;
}

void input(const input_t key) {
	int i = 0, len = strlen(ignored_characters);
 	if (key.raw >= ' ' && '~' >= key.raw && bpos < BUFF_SIZE - 1) {
		for (i = 0; i < len; ++i)
			if ( key.raw == ignored_characters[i] )
				break;
		if ( i == len )
			buff[bpos++] = (char )key.raw;
 	} else if (key.value == TUI_BACKSPACE && bpos > 0) 
		bpos--;
	
	if ( lbpos != bpos ) {
		cursor = 0;
		offset = 0;
	}

	buff[bpos] = '\0';
	lbpos = bpos;

	status = key.value != TUI_ESCAPE && key.value != TUI_ENTER;	
}

int init(void) {
	for (int i = 0; i < colors_count; ++i) {
		vec3i_t color = {0};
		if ( parse_color(&color, color_scheme[i]) != 1 ) {
			fprintf(stderr, 
				"failed to parse color: %s\n", color_scheme[i]);
			return 0;
		}
		color_set[i] = color;
	}

	const char *path_variable = NULL;	

	if ((path_variable = getenv("PATH")) != NULL ) {
		char path_buff[BUFF_SIZE] = {0};
		snprintf(path_buff, BUFF_SIZE, "%s", path_variable);
		
		char *token = strtok(path_buff, ":");

		while ( token != NULL ) {
			if ((status = load(token)) != 1 ) {
				fprintf(stderr, 
					"failed to load from: %s\n", token);
				return 0;
			}
			token = strtok(NULL, ":");
		}

		sort_list(root);
	}

	struct node *indirect = root;
	while ( indirect != NULL ) {
		int len = strlen(indirect->item.sub);
		if ( len > menu_width )
			menu_width = len;
		indirect = indirect->next;	
	}

	return 1;
}

void update(const input_t key) {
	struct node *indirect = root;	

	items = malloc(ITEMS_CAP * sizeof(struct item));
	icap = ITEMS_CAP, ipos = 0;

	while (indirect != NULL) {
		if ( ipos >= icap ) {
			icap += ITEMS_CAP;
			items = realloc(items, icap * sizeof(struct item));
		}

		char sub_buff[BUFF_SIZE];
		char input_buff[BUFF_SIZE];
		int i = 0;

		snprintf(sub_buff, BUFF_SIZE, "%s", indirect->item.sub);
		snprintf(input_buff, BUFF_SIZE, "%s", buff);

		for (; input_buff[i] != '\0'; ++i)
			if ( ignore_case )
				input_buff[i] = tolower(input_buff[i]);

		for (i = 0; sub_buff[i] != '\0'; ++i) 
			if ( ignore_case )
				sub_buff[i] = tolower(sub_buff[i]);

		if ( strcmp(sub_buff, input_buff) == 0 )
			indirect->item.weight = 3;
		else if ( strncmp(sub_buff, input_buff, bpos) == 0 )	
			indirect->item.weight = 2;
		else if ( strstr(sub_buff, input_buff) != NULL )
			indirect->item.weight = 1;
		else 		
			indirect->item.weight = 0;
		
		if ( indirect->item.weight > 0 ) 
			items[ipos++] = indirect->item;

		indirect = indirect->next;
	}

	if ( key.value == TUI_UP )   cursor--;
	if ( key.value == TUI_DOWN ) cursor++;

	if ( cursor < 0 ) {
		if ( offset > 0 )
			offset--;
		 cursor = 0;
	}

	if ( cursor >= ipos || cursor >= lines_count ) {
		if ( offset + lines_count < ipos )
			offset++;
		cursor = lines_count > ipos ? ipos - 1 : lines_count - 1;
	}

	qsort(items, ipos, sizeof(struct item), cmp_weight);

	for (int i = 0; i < ipos && i < lines_count; ++i) {
		struct item curr = items[i + offset];
		if ( cursor == i ) {
			if ( key.value == TUI_ENTER ) 
				target = curr;
			else if ( key.value == TUI_TAB ) {
				snprintf(buff, BUFF_SIZE, "%s", curr.sub);
				bpos = strlen(curr.sub);	
			}
		}
	}
}

void draw(void) {
	/* SEARCH BAR */
	char search_bar[BUFF_SIZE];
	snprintf(search_bar, BUFF_SIZE > menu_width ? menu_width : BUFF_SIZE, 
				"Search: %s", buff);

	tui_rectangle((rectangle_t) {
		.size = (vec2i_t) { menu_width, 1 },
		.pos  = (vec2i_t) { 0, 0 },
		.color = color_set[normal_bg]
	});

	tui_text((text_t) {
		.cstr = search_bar,	
		.color = color_set[normal_fg],
		.pos  = (vec2i_t) { 0, 0 }
	});

	/* EXECS BARS */
	for (int i = 0; i < ipos && i < lines_count; ++i) {
		char item_buff[BUFF_SIZE];
		snprintf(item_buff, BUFF_SIZE > menu_width ? menu_width : BUFF_SIZE, 
					"%s", items[i + offset].sub);

		tui_rectangle((rectangle_t) {
			.size = (vec2i_t) { menu_width, 1 },
			.pos  = (vec2i_t) { 0, i + 1 },
			.color = cursor == i ? color_set[curr_bg] : color_set[normal_bg]
		});

		tui_text((text_t) {
			.cstr = item_buff,				
			.color = cursor == i ? color_set[curr_fg] : color_set[normal_fg],
			.pos = (vec2i_t) { 0, i + 1 }
		});
	}

	tui_draw();
	free(items);
}

void launch(void) {
	struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
       
    sigaction(SIGWINCH, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    sigset_t empty_mask;
    sigemptyset(&empty_mask);
    sigprocmask(SIG_SETMASK, &empty_mask, NULL);	

	pid_t pid = fork();

	if ( pid == -1 ) {
		fprintf(stderr, 
			"fork failed: %s\n", strerror(errno));	
		status = 1;
	} else if ( pid == 0 ) {
		char *args[] = { target.sub, NULL };
		if ( execv(target.value, args) == -1 ) {
			fprintf(stderr,
				"cant launch: %s\n", target.sub);		
		}
		exit(0);
	} else 
		waitpid(pid, 0, 0);
}

int main(int argc, char *argv[]) {

	char *token;
	int value;

	for (int i = 1; i < argc; ++i) {
		if ( strncmp("-", argv[i], 1) != 0 && strncmp("--", argv[i], 2) != 0 ) {
			fprintf(stderr,
				"invalid option: '%s' must start with '-' or '--'\n", argv[i]);
			return 1;
		}

		if ( strcmp("-h", argv[i]) == 0 || strcmp("--help", argv[i]) == 0 )
			return tmenu_usage();
		else if ( strcmp("-d", argv[i]) == 0 || strcmp("--dump", argv[i]) == 0 )
			return tmenu_dump();	
		else if ( strcmp("-i", argv[i]) == 0 || strcmp("--ignore-case", argv[i]) == 0 ) 
			ignore_case = 1;
		else if ( strcmp("-w", argv[i]) == 0 || strncmp("--width=", argv[i], 8) == 0 ) {
			if ((token = value_pos(argv, argc, &i)) == NULL || (value = get_int(token)) == -1)
				return 1;
			menu_width = MAX(menu_width, value);		
		} else if ( strcmp("-l", argv[i]) == 0 || strncmp("--lines=", argv[i], 8) == 0 ) {
			if ((token = value_pos(argv, argc, &i)) == NULL || (value = get_int(token)) == -1)
				return 1;
			lines_count = MAX(1, value);		
		} else if ( strcmp("-nf", argv[i]) == 0 || strncmp("--normal-foreground=", argv[i], 20) == 0 ) {
			if ((token = value_pos(argv, argc, &i)) == NULL) 
				return 1;
			color_scheme[normal_fg] = token;
		} else if ( strcmp("-nb", argv[i]) == 0 || strncmp("--normal-background=", argv[i], 20) == 0 ) {
			if ((token = value_pos(argv, argc, &i)) == NULL) 
				return 1;
			color_scheme[normal_bg] = token;
		} else if ( strcmp("-cf", argv[i]) == 0 || strncmp("--current-foreground=", argv[i], 21) == 0 ) {
			if ((token = value_pos(argv, argc, &i)) == NULL) 
				return 1;
			color_scheme[curr_fg] = token;
		} else if ( strcmp("-cb", argv[i]) == 0 || strncmp("--curent-background=", argv[i], 21) == 0 ) {
			if ((token = value_pos(argv, argc, &i)) == NULL) 
				return 1;
			color_scheme[curr_bg] = token;
		} else {
			printf("unknown option: '%s'\n", argv[i]);
			return 1;
		}
	}

	if ( init() != 1 ) {
		free_list(root);
		return 1;
	}

	tui_init();

	while ( status ) {
		tui_update();

		input_t key = tui_input();

		input(key);
		update(key);
		draw();

		tui_sleepms(
			1000 / TARGET_FPS);
	}

	tui_exit();

	if ( target.value )
		launch();

	free_list(root);

	return 0;
}
