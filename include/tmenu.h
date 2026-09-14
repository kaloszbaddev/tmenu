#ifndef TMENU_H_
#define TMENU_H_

#define BUFF_SIZE 1024
#define ITEMS_CAP 256
#define TARGET_FPS 60

#define MAX(a, b) ((a) > (b) ? (a) : (b))

enum { normal_fg, normal_bg, curr_fg, curr_bg, colors_count };

struct item {
	char *value;
	char *sub;
	int weight;
};

struct node {
	struct item item;
	struct node *next;	
};

/* HELPERS */
extern int parse_color(vec3i_t *, const char *);
extern int get_int(const char *);
extern char *value_pos(char **args, const int, int *);
extern int cmp_weight(const void *, const void *); 

/* LINKED LIST */
extern void push_node(struct node **, char *); 
extern void sort_list(struct node *);
extern void print_list(struct node *);
extern void free_list(struct node *);
extern void print_list(struct node *); 
extern void free_list(struct node *);

/* MAIN FUNCTIONS */
extern int  load(const char *); 
extern void input(const input_t); 
extern int  init(void); 
extern void update(const input_t); 
extern void draw(void); 
extern void run(void);

#endif
