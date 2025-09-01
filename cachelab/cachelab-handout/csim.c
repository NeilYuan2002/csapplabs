#include "cachelab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAXSIZE 1024
#define MAXLINE 100
int how_many_sets;
int how_many_lines_per_set;
int how_many_blocks;

int how_many_sets_bit;
int how_many_blocks_bit;
char *trace_path;

typedef struct{  
  int valid_bit;
  unsigned int tag;
  char *data;
  int priority;
}cache_line;

typedef struct{
  int id;
  int lines_per_set;
  cache_line *lines;
}cache_set;

typedef struct{
  int total_sets;
  cache_set *sets;
}cache;

cache my_cache;

/* initialize and free */
void init_cache(cache *c,int total_sets,int lines_per_set,int block_num);
void free_cache(cache *c);
/* functions that resolve traces */
int get_trace_op(char *);/* return 0 for I,1 for L,2 for S,3 for M */
unsigned int get_trace_address(char *);
/* functions that resolve address */
unsigned int get_tag(int);
unsigned int get_index(int);
unsigned int get_offset(int);
/* cache operation related funcions */
int hit_check(cache c,int address);
void LRU_line_replace(cache *c,int address);
/* address mapping functions */
/* cache_set *address_which_set(cache c,int address); */
cache_line *address_which_line(cache s,int address);  
cache_set *address_which_set(cache c,int address);
unsigned int atoh(char *str);
int main(int argc, char *argv[])
{
  /* command arguments*/
  if(argc < 9)
    printf("not enough arguments\n");
  while(--argc > 0){
    argv++;
    /* printf("argc:%d\n",argc); */
    if(argv[0][0] == '-'){
      char c = argv[0][1];
      switch (c) {
      case 's':
        how_many_sets = (1 << atoi(argv[1]));
        how_many_sets_bit = atoi(argv[1]);
        printf("Option: -%c\n",c);
        printf("how_many_sets = %d\n",how_many_sets);
        break;
      case 'E':
        how_many_lines_per_set = atoi(argv[1]);
        printf("Option: -%c\n",c);
        printf("how_many_lines_per_set = %d\n",how_many_lines_per_set);
        break;
      case 'b':
        how_many_blocks = (1 << atoi(argv[1]));
        how_many_blocks_bit = atoi(argv[1]);
        printf("Option: -%c\n",c);
        printf("how_many_block_num = %d\n",how_many_blocks);        
        break;
      case 't':
        trace_path = argv[1];
        printf("Option: -%c\n",c);
        printf("trace_path is %s\n",trace_path);
        break;
      default:
        printf("Unknown option: -%c\n",c);
      }
    }
  }
  /* open trace file */
  FILE *fp;
  char *buffer[MAXLINE];
  char trace_line[MAXSIZE];
  int buffer_count = 0;
  
  fp = fopen(trace_path,"r");
  if(fp == NULL){
    printf("Error opening trace file\n");
    return 1;
  }
  while(fgets(trace_line, MAXSIZE, fp) != NULL){
    buffer[buffer_count] = (char *)malloc((strlen(trace_line) + 1) * sizeof(char));
    if(buffer[buffer_count] == NULL){
      printf("malloc error\n");
      return 1;
    }
    strcpy(buffer[buffer_count],trace_line);
    buffer_count++;        
  }
  /* for (int i = 0; i < buffer_count; i++) { */
  /*   printf("Line %d: %s", i + 1, buffer[i]); */
  /* } */
  
  /* cache initialization */
    
  init_cache(&my_cache,how_many_sets,how_many_lines_per_set,how_many_blocks);
  
  /* cache operation */
  
  for (int i = 0; i < buffer_count; i++){    
    int op;
    unsigned int address;
    address = get_trace_address(buffer[i]);
    printf("[%d]\n",i);
    printf("trace read: %s\n",buffer[i]);    
    printf("trace set index: %d\n",get_index(address));
    printf("trace tag: %d\n",get_tag(address));        
    op = get_trace_op(buffer[i]);
    /* printf("Trace line:%s -> Address: %d\n", buffer[i], address); */
    /* printf("Trace op id: %d\n",op); */

    /*operation: Load or Store */
    if(op == 1 || op == 2){
      
      if(hit_check(my_cache,address)){
        cache_set *pset = address_which_set(my_cache,address);
        cache_line *pline = address_which_line(my_cache,address);
        for(int i = 0;i < how_many_lines_per_set; i++){
          pset->lines[i].priority++;
        }
        pline->priority = 0;
        printf("hit\n");
      }    
      else{
        printf("miss\n");
        LRU_line_replace(&my_cache,address); /* LRU */          
      }
    }
    /* operation: Modify */
    else if(op == 3){
      
      /* Load */
      if(hit_check(my_cache,address)){
        cache_set *pset = address_which_set(my_cache,address);
        cache_line *pline = address_which_line(my_cache,address);
        for(int i = 0;i < how_many_lines_per_set; i++){
          pset->lines[i].priority++;
        }
        pline->priority = 0;
        printf("hit ");
      }    
      else{
        printf("miss ");
        LRU_line_replace(&my_cache,address); /* LRU */          
      }
     
      /* Store */
      if(hit_check(my_cache,address)){
        cache_set *pset = address_which_set(my_cache,address);
        cache_line *pline = address_which_line(my_cache,address);
        for(int i = 0;i < how_many_lines_per_set; i++){
          pset->lines[i].priority++;
        }
        pline->priority = 0;
        printf("hit ");
      }
      else{
        printf("miss ");
        LRU_line_replace(&my_cache,address); /* LRU */          
      }
      printf("\n");
    }
  
    /*operation: Instruction*/
    else{
      printf("operation 'I' ignored\n");
    }
  }       
  free_cache(&my_cache);
/* printSummary(0, 0, 0); */
return 0;
}


void init_cache(cache *c,int total_sets,int lines_per_set,int block_num){  
  c->total_sets = total_sets;
  
  c->sets = (cache_set *)malloc(total_sets * sizeof(cache_set));
  if(c->sets == NULL){
    printf("malloc error\n");
    exit(1);
  }
  /*init sets*/
  for(int i = 0; i < total_sets; i++){
    c->sets[i].lines = (cache_line *)malloc(lines_per_set * sizeof(cache_line));
    if(c->sets[i].lines == NULL){
      printf("malloc error\n");
      exit(1);
    }
    c->sets[i].lines_per_set = lines_per_set;
    
    /*init lines*/
    for(int j = 0; j < lines_per_set; j++){
      c->sets[i].lines[j].valid_bit = 0;
      c->sets[i].lines[j].tag = 0xFFFFFFFF;
      c->sets[i].lines[j].data = (char *)malloc(block_num * sizeof(char));
      if(c->sets[i].lines[j].data == NULL){
        printf("malloc error\n");
        exit(1);
      }
      c->sets[i].lines[j].priority = 0;
    }
  }
  printf("cache initialized!\n");
}

void free_cache(cache *c){
  for(int i = 0;i < c->total_sets; i++){
    for(int j = 0;j < c->sets[i].lines_per_set;j++){
      free(c->sets[i].lines[j].data);
    }
    free(c->sets[i].lines);
  }
  free(c->sets);
  printf("cache is now free\n");
}

int get_trace_op(char *trace_line){
  char op_char = trace_line[1];
  switch(op_char){
  case 'I':
    return 0;    
  case 'L':
    return 1;    
  case 'S':
    return 2;
  case 'M':
    return 3;
  default:
    printf("Unkown memory operation\n");
    return -1;
  }
}
unsigned int get_trace_address(char *trace_line){
  if (trace_line == NULL || trace_line[0] == '\0') {
        printf("Error: Empty trace line\n");
        return 0xFFFFFFFF; 
    }
  char *address_char = trace_line + 3;
  unsigned int address = atoh(address_char);/*invalid ',' causes atoh() stop*/
  return address;
}
unsigned int get_tag(int address){
  unsigned int tag_offset = how_many_sets_bit + how_many_blocks_bit;
  return address >> tag_offset;
}
unsigned int get_index(int address){
  unsigned int index_offset = how_many_blocks_bit;    
  return (address >> index_offset) & ((1 << how_many_sets_bit) - 1);
}
unsigned int get_offset(int address){
  
  return address & ((1 << how_many_blocks_bit) - 1);
}


int hit_check(cache c,int address){
  unsigned int valid,tag,index;
  
  index = get_index(address);
  tag = get_tag(address);

  if(index < 0 || index >= c.total_sets) {
    printf("Error: Invalid index %d (total_sets=%d)\n", index, c.total_sets);
        return 0; 
    }
  for(int i = 0; i < c.sets[index].lines_per_set; i++){
    if(tag == c.sets[index].lines[i].tag && c.sets[index].lines[i].valid_bit == 1)
      /* printf("hit lint info: valid_bit: %d,tag %d",c.sets[index].lines[i].valid_bit,c.sets[index].lines[i].tag); */
      return 1;     
  }
  return 0;
  
}
void LRU_line_replace(cache *c,int address){
  unsigned int index = get_index(address);
  unsigned int tag = get_tag(address);  
  int biggest_priority = c->sets[index].lines[0].priority;
  int bpi = 0;
  
  /* search for the LRU line */
  for(int i = 0; i < c->sets[index].lines_per_set; i++){    
    if(biggest_priority < c->sets[index].lines[i].priority){
      biggest_priority = c->sets[index].lines[i].priority;
      bpi = i;
    }
  }
  /*replacement*/
  c->sets[index].lines[bpi].tag = tag;
  c->sets[index].lines[bpi].valid_bit = 1;
  c->sets[index].lines[bpi].priority = 0;
}
cache_line *address_which_line(cache c,int address){
  unsigned int tag = get_tag(address);
  unsigned int index = get_index(address);
  cache_line *pline = NULL;

  for(int i = 0; i < c.sets[index].lines_per_set; i++){
    if(c.sets[index].lines[i].tag == tag){
      return pline = &c.sets[index].lines[i];      
    }
  }
  return pline;
}
cache_set *address_which_set(cache c,int address){
  unsigned int index = get_index(address);

  return &c.sets[index];
}
unsigned int atoh(char *str) {
    unsigned int result = 0;
    while (*str) {
        char c = *str++;
        if (c >= '0' && c <= '9') {
            result = result * 16 + (c - '0');
        } else if (c >= 'a' && c <= 'f') {
            result = result * 16 + (c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
            result = result * 16 + (c - 'A' + 10);
        } else {
           
            break;
        }
    }
    return result;
}
