//========================================================//
//  CSE 240a Cache  Lab                                   //
//                                                        //
//  Students need to implement L1 Cache                   //
//========================================================//

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ASSOCIATIVITY         4
#define BLOCK_SIZE            4 //in Bytes
#define CACHE_SIZE          128 //In Locations
#define L1_MISS_PENALTY      35 //In ns
#define L1_HIT_ACCESS_TIME    1
#define SETS                  CACHE_SIZE/ASSOCIATIVITY

//Declaring all global variables here
FILE *stream;
char *buf = NULL;
size_t len = 0;
// Set defaults
uint32_t address=0;
uint8_t cycle=0, loadnotstore=0,miss=0;
uint32_t num_accesses=0,misses=0, hits = 0,indexBits=0,indexMask=0,writehits=0,writemisses=0,readhits=0, readmisses=0;
uint32_t numOfSets = SETS;
uint32_t verbose = 0,replacement=1;
long cycle_access_time = 0;
int blockOffsetBits = 0 ;
char marker;

//The Below structure can be used for most Caches... Differences will come when we need different versions of cache or different replacement policies
//We dont need Data Currently because we are just Modelling the Cache Access Pattern
typedef struct cacheBlk
{
  uint8_t  valid[ASSOCIATIVITY];
  uint32_t tag[ASSOCIATIVITY];
  uint8_t  firstin;
  uint8_t  lastin;
} CacheImpl;
CacheImpl Cache[SETS];
//FIXME - Add more documentation here later on
void usage()
{
  fprintf(stderr,"Usage: simulator <options> [<trace>]\n");
  fprintf(stderr,"Usage: bunzip2 -kc gcc.trace.bz2 | ./main\n");
  fprintf(stderr," --verbose:0/1/2/3/4  (0:Default) Print different detailed debug message on stdout\n");
  fprintf(stderr," --replacement:0/1    Use (0:default) for FIFO replacement policy, use (1) for LIFO replacement policy\n");
}
//FIXME - Add more processing here
int handle_option(char const *arg)
{
  if (!strncmp(arg,"--verbose:",10)) {
    sscanf(arg+10,"%d", &verbose);
    printf("VERBOSE Setting : %d\n", verbose);
    return 1;
  }
  else if (!strncmp(arg,"--replacement:",14)) {
    sscanf(arg+14,"%d", &replacement);
    printf("replacement Setting : %d\n", replacement);
    return 1;
  }
  else
    return 0;
}
//This function extracts information from the trace file
int read_access(char *marker, uint8_t *loadnotstore, uint32_t *pc, uint8_t *cycle)
{
  if (getline(&buf, &len, stream) == -1) {
    return 0;
  }
  uint32_t cycle_temp;
  uint32_t loadnotstore_temp;

  sscanf(buf,"%c %d %x %d\n",marker,&loadnotstore_temp,pc,&cycle_temp);
  //printf("%s\n", buf);
  *cycle = cycle_temp;
  *loadnotstore = loadnotstore_temp;
  if(verbose > 2) printf("READ_ACCESS_FUNCTION : loadnotstore : %d\n", *loadnotstore);
  if(verbose > 2) printf("READ_ACCESS_FUNCTION : Address : %x\n", *pc);
  if(verbose > 2) printf("READ_ACCESS_FUNCTION : Cycle : %d\n", *cycle);
  return 1;
}

//This function simulates our cache
int is_cache_miss(uint8_t loadnotstore, uint32_t address){
  int idx,i;
  int valid;
  int tag, tagAddr;
  int tagMatch = 0;
  int allocateMatch = 0;
  int firstin,lastin;
  if(verbose > 2) printf("IS_CACHE_MISS : loadnotstore : %d\n", loadnotstore);
  if(verbose > 2) printf("IS_CACHE_MISS : Address : %x\n", address);
  uint32_t address_temp;
  address_temp = address >> blockOffsetBits;
  idx = address_temp & indexMask;

  address_temp = address_temp >> indexBits;
  tagAddr = address_temp;
  if(verbose > 2) printf("IS_CACHE_MISS idx : %x, TAG to look for : %x\n", idx, tagAddr);
  firstin = Cache[idx].firstin;
  lastin  = Cache[idx].lastin;
  
  //Check for match in Cache 
  for(i =0; i < ASSOCIATIVITY; i++){
    if((tagAddr == Cache[idx].tag[i]) && (Cache[idx].valid[i] == 1)){
      tagMatch = 1;
      if(verbose > 3) printf("Found a Tag Match\n");
      break;
    }
  }
  //Do Allocation Here
  if(tagMatch == 0){
    for(i = 0; i <ASSOCIATIVITY; i++){
      //Allocate Entry if place found.
      if(Cache[idx].valid[i] == 0){
        Cache[idx].valid[i] = 1;
        Cache[idx].tag[i] = tagAddr;
        allocateMatch = 1;
        if(verbose > 3) printf("Allocated an Entry here at idx %x, entry %x with Tag : %x, with Valid %d\n",idx,i,tagAddr,Cache[idx].valid[i]);
        break;
      }
    }
  } 
  //If there is a conflict, then find the place to remove
  if(allocateMatch == 0){
    if(replacement == 0){
      Cache[idx].valid[firstin] = 1;
      Cache[idx].tag[firstin]   = tagAddr;
      if(verbose > 3) printf("ALlocated an entry on a conflict entry no : %d using FIFO\n",firstin);
    } 
    else if (replacement == 1){
      Cache[idx].valid[lastin] = 1;
      Cache[idx].tag[lastin]   = tagAddr;
      if(verbose > 3) printf("ALlocated an entry on a conflict entry no : %d using LIFO\n",lastin);
    }
  }
  //For Caches with Conflicts check how to increment counter 
  if(ASSOCIATIVITY > 1 && allocateMatch == 0){
    if(replacement == 0){
      if(Cache[idx].firstin <= (ASSOCIATIVITY-2)){
        Cache[idx].firstin++;
      }
      else{
        Cache[idx].firstin = 0;
      }
    }       
    else if (replacement == 1){
      if(Cache[idx].lastin > 0){
        Cache[idx].lastin--;
      }
      else{
        Cache[idx].lastin = ASSOCIATIVITY-1;
      }
    }                
  }
  //Return Miss if block wasn't found
  if(tagMatch == 0){
    if(verbose > 4) printf("Missed in Cache\n");
    return 1;
  }
  else {
    if(verbose > 4) printf("Hit in Cache\n");
    return 0;
  }
}
//Function to calculate Log2
int log2n(int n)
{
    return (n > 1) ? 1 + log2n(n / 2) : 0;
}

int main(int argc, char const *argv[])
{

  int i;
  stream = stdin;
  // Process cmdline Arguments
  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i],"--help")) {
      usage();
      exit(0);
    } 
    else if (!strncmp(argv[i],"--",2)) {
      if (!handle_option(argv[i])) {
        printf("Unrecognized option %s\n", argv[i]);
        usage();
        exit(1);
      }
    } 
  }
  //Initialize Cache to be Empty initially
  for (int i = 0; i < SETS; i++)
  {
    for(int j = 0; j < ASSOCIATIVITY; j++){
      Cache[i].valid[j] = 0;
      Cache[i].tag[j]   = 0;
    }
    Cache[i].firstin = 0;
    Cache[i].lastin = ASSOCIATIVITY-1;
  }

  blockOffsetBits = log2n(BLOCK_SIZE);
  //printf("%d\n", blockOffsetBits);
  indexBits       = log2n(SETS);
  indexMask       = (1 << indexBits) - 1;
  //printf("Index Bits %d\n", indexBits);
  // Reach each branch from the trace
  while (read_access(&marker,&loadnotstore,&address, &cycle)) {
    num_accesses++;
    //printf("Here\n");
    // Make a prediction and compare with actual outcome
    miss = is_cache_miss(loadnotstore,address);
    if (miss) {
      misses++;
      if(loadnotstore){
        writemisses++;
      }
      else{
        readmisses++;
      }
      cycle_access_time += L1_MISS_PENALTY;
    }
    else{
      hits++;
      if(loadnotstore){
        writehits++;
      }
      else{
        readhits++;
      }
      cycle_access_time += L1_HIT_ACCESS_TIME;
    }
  }
  printf("Simulation results:\n");
  printf("\texecution cycles           %ld cycles\n",cycle_access_time);
  printf("\tmemory accesses            %d\n", hits+misses);
  printf("\toverall miss rate          %.2f%%\n", 100.0 * (float) misses / ((float) (hits + misses)) );
  printf("\tread miss rate             %.2f%%\n", 100.0 * (float) readmisses / ((float) (readhits + readmisses)) );
  printf("\toverall hit  rate          %.2f%%\n", 100.0 * (float) hits / ((float) (hits + misses)) );
  printf("\tread hit  rate             %.2f%%\n", 100.0 * (float) readhits / ((float) (readhits + readmisses)) );
  printf("\taverage memory access time %.2f cycles\n",  (float) (cycle_access_time) / (float) (hits + misses));
  printf("\tload_misses                'd%d\n", readmisses);
  printf("\tstore_misses               'd%d\n", misses - readmisses);
  printf("\tload_hits                  'd%d\n", readhits);
  printf("\tstore_hits                 'd%d\n", hits - readhits);
  // Cleanup
  fclose(stream);
  free(buf);
  return 0;
}