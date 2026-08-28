#ifndef TMENU_H_
#define TMENU_H_

#define BUFF_SIZE 1024
#define ITEMS_CAP 256

struct item {
	char *value;
	char *sub;
	int weight;
};

struct node {
	struct item item;
	struct node *next;	
};

struct data {
	int status;
	int cursor;
	int offset;
	struct item target;
};

/* HELPERS */
extern char *strstric(const char *, const char *); 
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
