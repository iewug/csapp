/**
 * by TomatoEater 2021/10/31
 */ 
#include "cachelab.h" //for printSummary
#include <stdio.h> //for printf fscanf
#include <unistd.h> // for getopt
#include <stdlib.h> // for atoi
#include <string.h> // for strcpy memset

int hit_count, miss_count, eviction_count;
void printUsage()
{
    printf("Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n"
            "Options:\n"
            "  -h         Print this help message.\n"
            "  -v         Optional verbose flag.\n"
            "  -s <num>   Number of set index bits.\n"
            "  -E <num>   Number of lines per set.\n"
            "  -b <num>   Number of block offset bits.\n"
            "  -t <file>  Trace file.\n\n"
            "Examples:\n"
            "  linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n"
            "  linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}
typedef struct{
    int valid;
    int tag;
    int stamp;
}cache_line;

int main(int argc, char *argv[])
{
    /**
     * 利用getopt函数获取参数
     */ 
    int opt, h, v, s, E, b, invalidOption;
    opt = h = v = s = E = b = invalidOption = 0;
    char t[1000];
    while (-1 != (opt = getopt(argc, argv, "hvs:E:b:t:")))
    {
        switch (opt)
        {
        case 'h':
            h = 1;
            break;
        case 'v':
            v = 1;
            break;
        case 's':
            s = atoi(optarg);
            break;
        case 'E':
            E = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 't':
            strcpy(t, optarg);
            break;
        case '?':
            invalidOption = 1;
            break;
        }
    }
    if (h)
    {
        printUsage();
        return 0;
    }
    if (invalidOption || s <= 0 || E <= 0 || b <= 0 || t == NULL)
    {
        printf("OOPS! Something is wrong! Check your input.\n");
        printUsage();
        return -1;
    }
    FILE* pFile = fopen(t, "r");
	if(pFile == NULL)
	{
		printf("%s: No such file or directory\n", t);
	    return -1;
	}


    int S = 1 << s;
    // init my cache & queue, C99 supports VLA, no need for malloc & free
    cache_line cache[S][E];
    memset(cache, 0, sizeof(cache));

    /**
     * 核心代码
     */ 
    char identifier;
    unsigned long long address;
    int size;
    while (fscanf(pFile, " %c %llx,%d", &identifier, &address, &size) != -1)
    {
        if (identifier == 'I') continue; // no operation for instruction load
        unsigned int tag = address >> (s+b);
        unsigned int set_index = address << (64-s-b) >> (64-s);
        if (v) printf("%c %llx,%d", identifier, address, size);
        int cnt;
        if (identifier == 'M')  cnt = 2;
        else cnt = 1;
        for (int j = 0; j < cnt; j++)
        {
            //hit!
            int isHit = 0;
            int hasEmpty = 0;
            for (int i = 0; i < E; i++)
            {
                if (cache[set_index][i].valid == 1 && cache[set_index][i].tag == tag)
                {
                    for (int k = 0; k < E; k++)
                        if (cache[set_index][k].valid)
                            cache[set_index][k].stamp++;
                    cache[set_index][i].stamp = 0;
                    hit_count++;
                    isHit = 1;
                    if (v) printf(" hit");
                    break;
                }
            }
            if (isHit) continue;

            //miss!
            miss_count++;
            if (v) printf(" miss");
            for (int i = 0; i < E; i++)
            {
                if (cache[set_index][i].valid == 0)
                {
                    cache[set_index][i].stamp = 0;
                    cache[set_index][i].tag = tag;
                    for (int k = 0; k < E; k++)
                        if (cache[set_index][k].valid)
                            cache[set_index][k].stamp++;
                    cache[set_index][i].valid = 1;
                    hasEmpty = 1;
                    break;
                }
            }
            if (hasEmpty) continue;

            //eviction!
            eviction_count++;
            int maxStamp = -1;
            int maxStampIndex = 0;
            for (int i = 0; i < E; i++)
            {
                if (maxStamp < cache[set_index][i].stamp)
                {
                    maxStamp = cache[set_index][i].stamp;
                    maxStampIndex = i;
                }
                cache[set_index][i].stamp++;
            }
            cache[set_index][maxStampIndex].tag = tag;
            cache[set_index][maxStampIndex].stamp = 0;
            if (v) printf(" eviction");
        }
        if (v) printf("\n");
    }
    

    fclose(pFile);
    
    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}
