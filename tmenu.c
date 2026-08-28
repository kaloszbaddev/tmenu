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

#include "tui.h"
#include "tmenu.h"
#include "config.h"

static struct data data = {0};
static char buff[BUFF_SIZE] = {0};
static struct item *items = NULL;
static struct node *root = NULL;
static int bpos = 0, lbpos = 0, ipos = 0, icap = 0;

char *strstric(const char *str, const char *sub) {
	int len;
	for (len = strlen(sub); *str; str++) {
		if (strncasecmp(str, sub, len) == 0) 
			return (char *)str;
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
			push_node(&root, strdup(full_path));
		} 
			
	}

	closedir(dir);
	return 1;
}

void input(const input_t key) {
 	if (key.raw >= ' ' && '~' >= key.raw && bpos < BUFF_SIZE - 1) 
		buff[bpos++] = (char )key.raw;
 	else if (key.value == TUI_BACKSPACE && bpos > 0) 
		bpos--;
	
	if ( lbpos != bpos ) {
		data.cursor = 0;
		data.offset = 0;
	}

	buff[bpos] = '\0';
	lbpos = bpos;

	data.status = key.value != TUI_ESCAPE && key.value != TUI_ENTER;	
}

int init(void) {
	const char *path_variable = NULL;	

	if ((path_variable = getenv("PATH")) != NULL ) {
		char path_buff[BUFF_SIZE] = {0};
		snprintf(path_buff, BUFF_SIZE, "%s", path_variable);
		
		char *tok = strtok(path_buff, ":");

		while ( tok != NULL ) {
			if ((data.status = load(tok)) != 1 ) {
				fprintf(stderr, 
					"failed to load from: %s\n", tok);
				return 0;
			}
			tok = strtok(NULL, ":");
		}

		sort_list(root);
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
	
		if ( strcasecmp(indirect->item.sub, buff) == 0 )
			indirect->item.weight = 3;
		else if ( strncasecmp(indirect->item.sub, buff, bpos) == 0 )	
			indirect->item.weight = 2;
		else if ( strstric(indirect->item.sub, buff) != NULL )
			indirect->item.weight = 1;
		else 		
			indirect->item.weight = 0;
		
		if ( indirect->item.weight > 0 ) 
			items[ipos++] = indirect->item;

		indirect = indirect->next;
	}

	if ( key.value == TUI_UP )   data.cursor--;
	if ( key.value == TUI_DOWN ) data.cursor++;

	if ( data.cursor < 0 ) {
		if ( data.offset > 0 )
			data.offset--;
		 data.cursor = 0;
	}

	if ( data.cursor >= ipos || data.cursor >= MAX_LINES ) {
		if ( data.offset + MAX_LINES < ipos )
			data.offset++;
		data.cursor = MAX_LINES > ipos ? ipos - 1 : MAX_LINES - 1;
	}

	qsort(items, ipos, sizeof(struct item), cmp_weight);

	for (int i = 0; i < ipos; ++i) {
		if ( data.cursor == i && key.value == TUI_ENTER )
			data.target = items[i + data.offset];
	}
}

void draw(void) {
	/* SEARCH BAR */
	char search_bar[BUFF_SIZE];
	snprintf(search_bar, BUFF_SIZE > BAR_WIDTH ? BAR_WIDTH : BUFF_SIZE, 
				"Search: %s", buff);

	const text_t text = (text_t) {
		.cstr = search_bar,	
		.color = FG_NORMAL,
		.pos  = (vec2i_t) { 0, 0 }
	};

	const rectangle_t rec = (rectangle_t) {
		.size = (vec2i_t) { BAR_WIDTH, 1 },
		.pos  = (vec2i_t) { 0, 0 },
		.color = BG_NORMAL
	};

	tui_rectangle(rec);
	tui_text(text);

	/* EXECS BARS */
	for (int i = 0; i < ipos && i < MAX_LINES; ++i) {
		char item_buff[BUFF_SIZE];
		snprintf(item_buff, BUFF_SIZE > BAR_WIDTH ? BAR_WIDTH : BUFF_SIZE, 
					"%s", items[i + data.offset].sub);

		const text_t text = (text_t) {
			.cstr = item_buff,				
			.color = data.cursor == i ? FG_CURR : FG_NORMAL,
			.pos = (vec2i_t) { 0, i + 1 }
		};

		const rectangle_t rec = (rectangle_t) {
			.size = (vec2i_t) { BAR_WIDTH, 1 },
			.pos  = (vec2i_t) { 0, i + 1 },
			.color = data.cursor == i ? BG_CURR : BG_NORMAL
		};

		tui_rectangle(rec);
		tui_text(text);
	}

	tui_draw();
	free(items);
}

void run(void) {
	pid_t pid = fork();

	if ( pid == -1 ) {
		fprintf(stderr, 
			"fork failed: %s\n", strerror(errno));	
		data.status = 1;
	} else if ( pid == 0 ) {
		char *args[] = { data.target.sub, NULL };
		if ( execv(data.target.value, args) == -1 ) {
			fprintf(stderr,
				"cant run: %s\n", data.target.sub);		
			exit(1);
		}
	} else 
		waitpid(pid, 0, 0);	
}

int main(int argc, char **argv) {

	if ( init() != 1 ) {
		free_list(root);
		return 1;
	}

	tui_init();

	while ( data.status ) {
		tui_update();

		input_t key = tui_input();

		input(key);
		update(key);
		draw();

		tui_sleepms(1000 / 60);
	}

	tui_exit();

	if ( data.target.value != NULL )
		run();

	free_list(root);

	return 0;
}
