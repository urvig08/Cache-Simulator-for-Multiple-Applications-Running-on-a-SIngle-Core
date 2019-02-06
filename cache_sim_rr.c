#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <math.h>

typedef enum {dm, fa} cache_map_t;
typedef enum {uc} cache_org_t;
typedef enum {instruction, data} access_t;

typedef struct {
    uint32_t address;
    access_t accesstype;
} mem_access_t;

typedef struct {
    int valid_bit;
    int tag;
    int fifo_counter; 
                        /* This counter is only used for fully associative cache. It indicates the "oldest" block in the cache 
                        (a block which hasn't been updated for the longest). This allows us to point out where the next address
                        should be stored in cache. This counter is equal to 0 for the block in which the hit was recorded. It is
                        incremented by 1 for every block during every search in the cache. */
} block_t; 

typedef struct {
    block_t *block_array;
} cache_t; 


uint32_t cache_size; 
uint32_t block_size;
cache_map_t cache_mapping;
cache_org_t cache_org;

// for Unified Cache:
cache_t cache;
int cache_counter = 0;
int hit_counter = 0;


/* Reads a memory access from the trace file and returns
 * access type and
 * memory address
 */
mem_access_t read_line1(FILE *ptr_file1) {
    char buf[32];
    int* token;
    char* string = buf;
    mem_access_t access;

    if (fgets(buf,32, ptr_file1)!=NULL) {

        /* Get the access type */
        token = strtok(string, " \n");
        if (strcmp(token,"2") == 0) {
            access.accesstype = instruction;
        } 
	if (strcmp(token,"0") == 0) {
            access.accesstype = data;
        } 
	
	if (strcmp(token,"1") == 0) {
            access.accesstype = data;
        } 
	
        /* Get the access type */        
        token = strtok(NULL, " \n");
        access.address = (uint32_t)strtol(token, NULL, 16);
 

        return access;
    }

    access.address = 0;
    return access;
}


mem_access_t read_line2(FILE *ptr_file2) {
    char buf[32];
    int* token;
    char* string = buf;
    mem_access_t access;

    if (fgets(buf,32, ptr_file2)!=NULL) {

        /* Get the access type */
        token = strtok(string, " \n");
        if (strcmp(token,"2") == 0) {
            access.accesstype = instruction;
        } 
	if (strcmp(token,"0") == 0) {
            access.accesstype = data;
        } 
	
	if (strcmp(token,"1") == 0) {
            access.accesstype = data;
        } 
	
        /* Get the access type */        
        token = strtok(NULL, " \n");
        access.address = (uint32_t)strtol(token, NULL, 16);
 

        return access;
    }

    access.address = 0;
    return access;
}

mem_access_t read_line3(FILE *ptr_file3) {
    char buf[32];
    int* token;
    char* string = buf;
    mem_access_t access;

    if (fgets(buf,32, ptr_file3)!=NULL) {

        /* Get the access type */
        token = strtok(string, " \n");
        if (strcmp(token,"2") == 0) {
            access.accesstype = instruction;
        } 
	if (strcmp(token,"0") == 0) {
            access.accesstype = data;
        } 
	
	if (strcmp(token,"1") == 0) {
            access.accesstype = data;
        } 
	
	/* Get the access type */                
        token = strtok(NULL, " \n");
        access.address = (uint32_t)strtol(token, NULL, 16);
 

        return access;
    }

    access.address = 0;
    return access;
}

/*  This function is used to initialise the cache. It takes number of blocks and a pointer to the cache we want to initialise as arguments.
    Every block is initialised and all variables are initialised to 0. */
void cache_init (int block_number, cache_t * s_cache) {
    s_cache->block_array = malloc(block_number * 64);

    for (int i= 0; i< block_number; i++) {
        s_cache->block_array[i].tag = 0;
        s_cache->block_array[i].valid_bit = 0;
        s_cache->block_array[i].fifo_counter = 0;
    }
}

/*  This function is used for direct mapped cache. */
void execute_dm (mem_access_t access, int block_number, int index_size, cache_t * f_cache) {
    int cutOffset = (access.address >> ((int)log2(block_size))) % (block_number);     // get the offset
    int tag = (access.address >> (((int)log2(block_size)) + index_size));             // get the size of the tag (remaining bits after subtracting the size of the index and the offset (in bites))
    int f_counter = 0;                                          // initialising the counters
    int f_hit_counter = 0;

    for (int i= 0; i < block_number; i++) {                     // searching the cache
        if (i == cutOffset) {
            f_counter++;
            if (tag == f_cache->block_array[i].tag) {           // the tag is found in the cache 
                if (f_cache->block_array[i].valid_bit == 1) {   // valid bit is equal to 1
                    f_hit_counter++;                            // increment on hit
                } else {                                        // valid bit is not equal to 1
                    f_cache->block_array[i].tag = tag;          // updating the tag
                    f_cache->block_array[i].valid_bit = 1;      // changing the valid bit to 1
                }
            } else {                                            // the tag is not found in the cache
                f_cache->block_array[i].tag = tag;              // updating the tag
                f_cache->block_array[i].valid_bit = 1;          // changing the valid bit to 1
            }
        }
    }

    /* Updating the counters for unified cache */
    if (f_cache == &cache) {                                    
        cache_counter += f_counter;
        hit_counter += f_hit_counter;
    }
    
}

/*  This function is used for fully associative cache. */
void execute_fa (mem_access_t access, int block_number, cache_t * f_cache) {
    int tag = (access.address >> ((int)log2(block_size)));    // calculating the size of the tag (remaining bits after cutting the offset)
                                                              // Initialising the counters 
    int f_counter = 0;                                          
    int f_hit_counter = 0;
    int found = 0;                      // used to indicate if the cache block was updated (if the hit was recorded or the block was updated because it was previously empty)                                  
    int max_counter = 0;                // used to find the maximum fifo_counter 
    int x = 0;                          // used to update fifo counters of the ramaining cache blocks if the hit was recorded or the block was updated because it was previously empty

    for (int i= 0; i< block_number; i++) {                                                      // searching in cache
        if (tag == f_cache->block_array[i].tag && f_cache->block_array[i].valid_bit == 1) {     // the tag is found in the cache and the valid bit is equal to 1
            f_counter++;
            f_hit_counter++;                                                // hit
            f_cache->block_array[i].fifo_counter++;
            found = 1;
            x = i + 1;
            break;
        } else if (f_cache->block_array[i].tag == 0) {                      // if the cache block is empty
            f_cache->block_array[i].tag = tag;                              // updating the tag
            f_cache->block_array[i].valid_bit = 1;                          // changing the valid bit to 1
            f_cache->block_array[i].fifo_counter = 0;                       // changing fifo_counter to 0 
            found = 1;
            x = i + 1;
            f_counter++;
            break;
        } else {
            f_cache->block_array[i].fifo_counter++;
        }
    }

    if (found == 1) {                                                       // if the hit was recorded or the block was updated because it was previously empty
        for (int i = x; i < block_number; i++) {
            f_cache->block_array[i].fifo_counter++;
        }  
    }

    if (found == 0) {                                                       // if the tag wasn't matched 
        f_counter++;
        for (int a = 0; a < block_number; a++) {                            // find the maximum fifo_counter
            if (f_cache->block_array[a].fifo_counter > max_counter) {
                max_counter = f_cache->block_array[a].fifo_counter;
            }
        }
        for (int b = 0; b < block_number; b++) {
            if (f_cache->block_array[b].fifo_counter == max_counter) {      // update the cache block which had the highest fifo_counter 
                f_cache->block_array[b].tag = tag;
                f_cache->block_array[b].valid_bit = 1;
                f_cache->block_array[b].fifo_counter = 0;                   // changing fifo_counter to 0 
                break;
            }
        }               
    }

    /* Updating the counters for unified cache */
    if (f_cache == &cache) {                        
        cache_counter += f_counter;
        hit_counter += f_hit_counter;
    }
    
}

void main(int argc, char** argv) {

    /* Read command-line parameters and initialize:
     * cache_size, block_size, cache_mapping and cache_org variables
     */

    if ( argc != 5 ) { 
        printf("Usage: ./cache_sim_rr [cache size: 32K] [block size: 8|32|128] [cache mapping: dm|fa] [cache organization: uc]\n");
        exit(0);
    } else  {

        /* Setting cache size and block size */
        cache_size = atoi(argv[1]);
	block_size = atoi(argv[2]);

        /* Setting Cache Mapping */
        if (strcmp(argv[3], "dm") == 0) {
            cache_mapping = dm;
        } else if (strcmp(argv[3], "fa") == 0) {
            cache_mapping = fa;
        } else {
            printf("Unknown cache mapping\n");
            exit(0);
        }

        /* Setting Cache Organization */
        if (strcmp(argv[4], "uc") == 0) {
            cache_org = uc;
        } else {
            printf("Unknown cache organization\n");
            exit(0);
        }
    }


    FILE *ptr_file1, *ptr_file2, *ptr_file3;
    ptr_file2 =fopen("ccompiler.trace","r");
    if (!ptr_file2) {
        printf("Unable to open the trace file\n");
        exit(1);
    }

    ptr_file3 =fopen("spice.trace","r");
    if (!ptr_file3) {
        printf("Unable to open the trace file\n");
        exit(1);
    }

    ptr_file1 =fopen("trace.din","r");
    if (!ptr_file1) {
        printf("Unable to open the trace file\n");
        exit(1);
    }

    // Cache initialisation:    
    if (cache_mapping == fa || cache_mapping == dm) {
        if (cache_org == uc) {
            int block_number = cache_size / block_size;
            cache_init(block_number, &cache);
        } 
    }

    /* Loop until whole trace file has been read */
    mem_access_t access;
    while(1) {
        access = read_line1(ptr_file1);
        
        if (access.address == 0)
            goto x;

	   /* Accessing the cache */

        else if (cache_mapping == dm) {                              // for Direct mapping cache
            if (cache_org == uc) {
                int block_number = cache_size / block_size;
                int index_size = (int)log2(block_number);
                execute_dm (access, block_number, index_size, &cache);
            } 
        } else {                                                     // for Fully Associative cache mapping
            if (cache_org == uc) {
                int block_number = cache_size / block_size;
                execute_fa (access, block_number, &cache);
            } 
        }

x:
	access = read_line2(ptr_file2);
                                                                      // If EOF, break out of loop
        if (access.address == 0)
            goto y;

	   /* Accessing the cache */

        else if (cache_mapping == dm) {                                // for Direct mapping cache
            if (cache_org == uc) {
                int block_number = cache_size / block_size;
                int index_size = (int)log2(block_number);
                execute_dm (access, block_number, index_size, &cache);
            } 
        } else {                                                        // for Fully Associative cache mapping
            if (cache_org == uc) {
                int block_number = cache_size / block_size;
                execute_fa (access, block_number, &cache);
            } 
        }

y:
	access = read_line3(ptr_file3);
                                                                         //If EOF,break out of loop
        if (access.address == 0)
            break;

	   
        /* Accessing the cache */

        else if (cache_mapping == dm) {                                  // for Direct mapping cache
            if (cache_org == uc) {
                int block_number = cache_size / block_size;
                int index_size = (int)log2(block_number);
                execute_dm (access, block_number, index_size, &cache);
            } 
        } else {                                                         // for Fully associative cache mapping
            if (cache_org == uc) {
                int block_number = cache_size / block_size;
                execute_fa (access, block_number, &cache);
            } 
        }

    }

    /* Print the total accesses and total hits recorded */

    if (cache_org == uc) {
        printf("U.accesses: %d\n", cache_counter);
        printf("U.hits: %d\n", hit_counter);
        printf("U.hit rate: %1.3f\n", ((double)hit_counter)/cache_counter);
    } 

    /* Close the trace file */
    fclose(ptr_file1);
    fclose(ptr_file2);
    fclose(ptr_file3);
}
