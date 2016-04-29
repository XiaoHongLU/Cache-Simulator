#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <math.h>

typedef enum {dm, fa} cache_map_t;
typedef enum {uc, sc} cache_org_t;
typedef enum {instruction, data} access_t;

const unsigned int MEM_ADDR_BITS = 32;
const unsigned int BLOCK_SIZE = 64;

typedef struct {
    uint32_t address;
    access_t accesstype;
} mem_access_t;

// DECLARE CACHES AND COUNTERS FOR THE STATS HERE
struct block                                                                           //Structure of the type block
{                                                                                      //
	unsigned int valid;                                                                //valid bit
	unsigned int tag;                                                                  //tag bit
};                                                                                     //

struct cache                                                                           //Structure of the type cache
{                                                                                      //
	unsigned int cache_size;                                                           //cache size
	unsigned int block_size;                                                           //block size
	unsigned int block_number;                                                         //block number
	unsigned int block_offset_bits;                                                    //block offset bits
	unsigned int index_bits;                                                           //block index bits
	unsigned int tag_bits;                                                             //block tag bits
	unsigned int associate_number;                                                     //associate number
	unsigned int total_set_number;                                                     //total set number
	unsigned int access_number;                                                        //access number
	unsigned int hit_number;                                                           //hit number
	unsigned int next_update[100];                                                     //next update
	struct block cache_blocks[100];                                                    //struct block cache blocks
};                                                                                     //

uint32_t cache_size; 
cache_map_t cache_mapping;
cache_org_t cache_org;


/* Reads a memory access from the trace file and returns
 * 1) access type (instruction or data access)
 * 2) memory address
 */

//---------------------------------------------------------------------------
// Function
//---------------------------------------------------------------------------

mem_access_t read_transaction(FILE *ptr_file) {
    char buf[1000];
    char* token;
    char* string = buf;
    mem_access_t access;

    if (fgets(buf,1000, ptr_file)!=NULL) {

        /* Get the access type */
        token = strsep(&string, " \n");        
        if (strcmp(token,"I") == 0) {
            access.accesstype = instruction;
        } else if (strcmp(token,"D") == 0) {
            access.accesstype = data;
        } else {
            printf("Unkown access type\n");
            exit(0);
        }
        
        /* Get the access type */        
        token = strsep(&string, " \n");
        access.address = (uint32_t)strtol(token, NULL, 16);

        return access;
    }

    /* If there are no more entries in the file return an address 0 */
    access.address = 0;
    return access;
}

//---------------------------------------------------------------------------
// Function - Create Cache
//---------------------------------------------------------------------------

struct cache* create_cache(cache_map_t maps, unsigned int cache_size)                                           //
{                                                                                                               //
    unsigned int i;                                                                                             //
	assert(maps == dm || maps == fa);                                                                           //To ensure that the enum of maps is either uc or sc only
	struct cache* memory = (struct cache*)malloc(sizeof(struct cache));                                         //To malloc the size for the pointer memory
                                                                                                                //
                                                                                                                //
	if(maps==dm) memory->associate_number = 1;                                                                  //To set the associate_number is 1 for dm
	else memory->associate_number = cache_size / BLOCK_SIZE;                                                    //To set the associate_number as cache_size / BLOCK_SIZE(64) for fa
                                                                                                                //
	memory->block_number = cache_size / BLOCK_SIZE;                                                             //To set the block_number = cache_size / BLOCK_SIZE(64)
	memory->block_offset_bits = (unsigned int)log_2(BLOCK_SIZE);                                                //To set the block_offset_bits
	memory->block_size = BLOCK_SIZE;                                                                            //To set the block_size
	memory->cache_size = cache_size;                                                                            //To set the cache_size
	memory->index_bits = (unsigned int)log_2(memory->block_number / memory->associate_number);                  //To set the index_bits
	memory->tag_bits = MEM_ADDR_BITS - (memory->index_bits + memory->block_offset_bits);                        //To set the tag_bits
	memory->total_set_number = (memory->block_number) / (memory->associate_number);                             //To set the total_set_number
	memory->access_number = 0;                                                                                  //To set the access_number for the cache
	memory->hit_number = 0;                                                                                     //To set the hit_number for the cache
                                                                                                                //
	memset(memory->next_update, 0, sizeof(memory->next_update));                                                //To set the all the value in next_update to be 0
	for (i = 0; i < memory->block_number; i++)                                                                  //
	{                                                                                                           //
		memory->cache_blocks[i].tag = -1;                                                                       //To set the tag of the cache block to be -1
		memory->cache_blocks[i].valid = 0;                                                                      //To set the valid bit of the cache block to be 0
	}                                                                                                           //
	return memory;                                                                                              //To return the cache memory
}                                                                                                               //

//---------------------------------------------------------------------------
// Function - Delete Cache
//---------------------------------------------------------------------------

void delete_cache(struct cache* memory)                                                //
{                                                                                      //
	if(memory) free(memory);                                                           //Free the memory when finish
}                                                                                      //

//---------------------------------------------------------------------------
// Function - Power Function
//---------------------------------------------------------------------------

int pow_xy(int x, unsigned int y)                                                      //To calculate Power 
{                                                                                      //
	int ret = 1;                                                                       //X is base, Y is power
	unsigned int i;                                                                    //
	for(i = 0; i < y; i++)                                                             //
	{                                                                                  //
		ret *= x;                                                                      //ret = ret * base for every time of the loop
	}                                                                                  //
	return ret;                                                                        //
}                                                                                      //

//---------------------------------------------------------------------------
// Function - Log 2 Function
//---------------------------------------------------------------------------

int log_2(unsigned int t)                                                              //To calculate log2
{                                                                                      //
	unsigned int ret = 0;                                                              //Variable t is the value
	for(ret = 0; ret < 32; ret++)                                                      //
	{                                                                                  //
		if((unsigned int)pow_xy(2, ret) == t)                                          //To compare the value of the power of base 2 with t 
			return ret;                                                                //
	}                                                                                  //
	return ret;                                                                        //To return 0 if the value of t is 1
}                                                                                      //

//---------------------------------------------------------------------------
// Function - Decode Index of the Cache
//---------------------------------------------------------------------------

unsigned int decode_index(unsigned int address, struct cache* mem)                     //
{                                                                                      //
	address = address >> (mem->block_offset_bits);                                     //To shift the address right according the value of block_offset_bits in mem
	unsigned int num = (unsigned int)pow_xy(2, mem->index_bits) - 1;                   //To calculate the block number according the value of index_bits in mem
	return address&num;                                                                //To use AND operator to get the index value
}                                                                                      //

//---------------------------------------------------------------------------
// Function - Decode Tag of the Cache
//---------------------------------------------------------------------------

unsigned int decode_tag(unsigned int address, struct cache* mem)                       //
{                                                                                      //
	return address >> (mem->block_offset_bits + mem->index_bits);                      //To shift the address right according the sum value of block_offset_bits and index_bits in mem
}                                                                                      //

//---------------------------------------------------------------------------
// Function - Check the tag with the tag in Cache Block
//---------------------------------------------------------------------------

unsigned int check_cache_hit(unsigned int index, unsigned int tag, struct cache* mem)                           //
{                                                                                                               //
	unsigned int current_row = index * mem->associate_number;                                                   //To calcualte the current position to chech with the tag
	unsigned int ret;                                                                                           //
	for (ret = 0; ret < mem->associate_number; ret++)                                                           //
		if (mem->cache_blocks[current_row + ret].valid == 1 && mem->cache_blocks[current_row+ret].tag==tag)     //If the valid bit of the cache block and the tag of the cache block
			return ret;                                                                                         //are true, return the value of ret
	return -1;                                                                                                  //If not, return -1 
}                                                                                                               //

//---------------------------------------------------------------------------
// Function - Update the tag in the Cache Block
//---------------------------------------------------------------------------

void update_cache(unsigned int index, unsigned int tag, unsigned int line, struct cache* mem)                   //
{                                                                                                               //
	unsigned int current_position = index * mem->associate_number + line;                                       //To calculate the next position for the tag to update
	mem->cache_blocks[current_position].valid = 1;                                                              //To set the valid bit of the cache block to be 1
	mem->cache_blocks[current_position].tag = tag;                                                              //To place the tag in the cache block
}                                                                                                               //

//---------------------------------------------------------------------------
// Function - Deal with the Cache access
//---------------------------------------------------------------------------

void dealwith_cache_access(unsigned int address, struct cache* mem)                                             //
{                                                                                                               //
	unsigned int index = decode_index(address, mem);                                                            //To decode the index for this address
	unsigned int tag = decode_tag(address, mem);                                                                //To decode the tag for this address
	if (check_cache_hit(index, tag, mem)!=-1)                                                                   //To check the cache hit
	{                                                                                                           //
		mem->hit_number++;                                                                                      //If the address is inside the cache, hit increases
	}                                                                                                           //
	else                                                                                                        //
	{                                                                                                           //If the address is not inside the cache,
		unsigned int next_update_line = mem->next_update[index];                                                //
		update_cache(index, tag, next_update_line, mem);                                                        //update the address to the specific block in the cache
		mem->next_update[index] = (mem->next_update[index] + 1) % (mem->associate_number);                      //
	}                                                                                                           //
	mem->access_number++;                                                                                       //total access number increases
}                                                                                                               //

//---------------------------------------------------------------------------
// Function - Print Statistics
//---------------------------------------------------------------------------

void print_statistics(struct cache* mem, char type)                                    //
{                                                                                      //
	double hit_rate = (mem->hit_number) / (1.0 * mem->access_number);                  //To calculate hit_rate
	printf("%c.accesses: %u\n", type, mem->access_number);                             //To print out access_number in the mem
	printf("%c.hits: %u\n", type, mem->hit_number);                                    //To print out hit_number in the mem
	printf("%c.hit rate: %1.3lf\n", type, hit_rate);                                   //To print out hit_rate for the mem
}                                                                                      //


//---------------------------------------------------------------------------
// MAIN function
//---------------------------------------------------------------------------

int main(int argc, char** argv)
{

    /* Read command-line parameters and initialize:
     * cache_size, cache_mapping and cache_org variables
     */

    if ( argc != 4 ) { /* argc should be 2 for correct execution */
        printf("Usage: ./cache_sim [cache size: 128-4096] [cache mapping: dm|fa] [cache organization: uc|sc]\n");
        exit(0);
    } else  {
        /* argv[0] is program name, parameters start with argv[1] */

        /* Set cache size */
        cache_size = atoi(argv[1]);

        /* Set Cache Mapping */
        if (strcmp(argv[2], "dm") == 0) {
            cache_mapping = dm;
        } else if (strcmp(argv[2], "fa") == 0) {
            cache_mapping = fa;
        } else {
            printf("Unknown cache mapping\n");
            exit(0);
        }

        /* Set Cache Organization */
        if (strcmp(argv[3], "uc") == 0) {
            cache_org = uc;
        } else if (strcmp(argv[3], "sc") == 0) {
            cache_org = sc;
        } else {
            printf("Unknown cache organization\n");
            exit(0);
        }
    }
	


    /* Open the file mem_trace.txt to read memory accesses */
    FILE *ptr_file;
    ptr_file =fopen("mem_trace.txt","r");
    if (!ptr_file) {
        printf("Unable to open the trace file\n");
        exit(1);
    }

    unsigned int actual_cache_size;                                                    //Create Variable to calculate actual cache size depending on UC or SC
    if(cache_org==uc) actual_cache_size = cache_size;                                  //Assign the cache size for UC
    else actual_cache_size = cache_size / 2;                                           //Assign the cache size for SC
    struct cache* inst_mem = create_cache(cache_mapping, actual_cache_size);           //Create inst_mem for Instruction Cache
    struct cache* data_mem = create_cache(cache_mapping, actual_cache_size);           //Create data_mem for Data Cache

    /* Loop until whole trace file has been read */
    mem_access_t access;
    while(1) {
        access = read_transaction(ptr_file);
        //If no transactions left, break out of loop
        if (access.address == 0)
            break;

		/* Do a cache access */
		// ADD YOUR CODE HERE
		assert(cache_org == uc || cache_org == sc);                                    //To ensure that the enum of cache_org is either uc or sc only
		if (cache_org == uc)                                                           //If cache_org is uc
			dealwith_cache_access(access.address, inst_mem);                           //dealwith cache access only with one Cache for both Instruction and Data
		else                                                                           //
		{                                                                              //If cache_org is sc
			if (access.accesstype == instruction)                                      //If the access.accesstype is instruction
				dealwith_cache_access(access.address, inst_mem);                       //dealwith cache access with Instruction Cache for Instruction
			else if (access.accesstype == data)                                        //If the access.accesstype is data
				dealwith_cache_access(access.address, data_mem);                       //dealwith cache access with Data Cache for Data
		}                                                                              //
    }                                                                                  //

    /* Print the statistics */
    // ADD YOUR CODE HERE
	assert(cache_org == uc || cache_org == sc);                                        //To ensure that the enum of cache_org is either uc or sc only
	if (cache_org == uc)                                                               //If cache_org is uc,
		print_statistics(inst_mem, 'U');                                               //The statistics starts with U
	else                                                                               //
	{                                                                                  //If cache_org is sc,
		print_statistics(inst_mem, 'I');                                               //The statistics starts with I for Instruction Cache
		printf("\n");                                                                  //
		print_statistics(data_mem, 'D');                                               //The statistics starts with D for Data Cache
	}                                                                                  //

	delete_cache(inst_mem);                                                            //Free the inst_mem
	delete_cache(data_mem);                                                            //Free the data_mem
    
    /* Close the trace file */
    fclose(ptr_file);
    return 0;
}

