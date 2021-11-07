# Cache Lab记录

<p style="text-align:right;">by TomatoEater<br>2021年11月</p>

实验分为两部分，Part A要求实现一个cache的模拟器，Part B要求实现一个为特定cache优化过的转置矩阵函数。

## 1. Part A

输入的trace文件都是由valgrind生成，如：`linux> valgrind --log-fd=1 --tool=lackey -v --trace-mem=yes ls -l`可以依次输出内存的访问。trace的格式为：

```
I 0400d7d4,8
 M 0421c7f0,4
 L 04f6b868,8
 S 7ff0005c8,8
```

“I” denotes an instruction load, “L” a data load, “S” a data store, and “M” a data modify (i.e., a data load followed by a data store). There is never a space before each “I”. There is always a space before each “M”, “L”, and “S”. The address field specifies a 64-bit hexadecimal memory address. The size field specifies the number of bytes accessed by the operation.

现在给出s、E、b这些参数以及trace文件，我们需要输出hit、miss以及eviction的次数。替换策略采用的是LRU（least-recently used）。用法具体如下：

```bash
Usage: ./csim-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile>
 -h: Optional help flag that prints usage info
 -v: Optional verbose flag that displays trace info
 -s <s>: Number of set index bits (S = 2^s is the number of sets)
 -E <E>: Associativity (number of lines per set)
 -b <b>: Number of block bits (B = 2^b is the block size)
 -t <tracefile>: Name of the valgrind trace to replay
```

输出的例子如下：

```bash
linux> ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace
hits:4 misses:5 evictions:3

linux> ./csim-ref -v -s 4 -E 1 -b 4 -t traces/yi.trace
L 10,1 miss
M 20,1 miss hit
L 22,1 hit
S 18,1 hit
L 110,1 miss eviction
L 210,1 miss eviction
M 12,1 miss eviction hit
hits:4 misses:5 evictions:3
```

注意到加了v参数就会多输出一些信息。

要模拟cache，其实参数b以及trace文件的I都是没有用的，而trace文件的M、L、S其实也是同质化的。M就是查两次cache有没有，L和S则是一次。我并没有采取队列来实现LRU，而是用了时间戳，感觉队列会比较麻烦。

话不多说，直接上代码：

```C
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
```

关键就是getopt和fscanf两个函数需要学习一下。~~sdyt，这是本人第一次正式写C程序~~

测试如下：

```bash
linux> ./test-csim
                        Your simulator     Reference simulator
Points (s,E,b)    Hits  Misses  Evicts    Hits  Misses  Evicts
     3 (1,1,1)       9       8       6       9       8       6  traces/yi2.trace
     3 (4,2,4)       4       5       2       4       5       2  traces/yi.trace
     3 (2,1,4)       2       3       1       2       3       1  traces/dave.trace
     3 (2,1,3)     167      71      67     167      71      67  traces/trans.trace
     3 (2,2,3)     201      37      29     201      37      29  traces/trans.trace
     3 (2,4,3)     212      26      10     212      26      10  traces/trans.trace
     3 (5,1,5)     231       7       0     231       7       0  traces/trans.trace
     6 (5,1,5)  265189   21775   21743  265189   21775   21743  traces/long.trace
    27

TEST_CSIM_RESULTS=27
```

得分27就对了。

***



## 2. Part B

Part B要求我们实现trans.c文件中的transpose_submit函数，对32\*32，64\*64，61\*67三个矩阵进行特殊的转置优化。cache大小为s = 5， E = 1, b = 5。即，直接映射Cache，有32组，每组一行，每行能放32个字节(8个int)。

评分要求为：

- 32 * 32，miss<300得8分，miss>600不得分
- 64 * 64，miss<1300得8分，miss>2000不得分
- 61 * 67，miss<2000得10分，miss>3000不得分

### 2.1 32\*32

**1）naive-plus solution**

最直接的理解为：

```c
int i, j, tmp;

for (i = 0; i < N; i++) 
{
     for (j = 0; j < M; j++) 
     {
        	tmp = A[i][j];
        	B[j][i] = tmp;
      }
}   
```

测试出miss数为1183，远远大于要求的300~~（为什么要运行这么久才能测出来）~~。

解释：

以i=0，j=0~31的例子说明。cache每个block放8个int，A矩阵按行读入，第一行产生32/8=4次miss。B矩阵按列写入，每次写都会产生miss，共有32次。在i=1的时候，写入B矩阵的第1列，仍会产生32次miss，这是由cache只有32行所导致的。这样算出来有36*32=1152次miss，测试却是1183。这是产生了冲突不命中（抖动）的原因。对于AB矩阵主对角线上的元素会映射到同一cache块上面，这是由AB的地址所导致的（这一点不查别人的资料很难想到）。也就是说读A\[0\]\[0\]的时候产生一次miss，然后把A\[0][0]到A\[0\]\[7\]的元素存到了cache的中；接下来把A\[0\]\[0\]写入B\[0\]\[0\]中，但是在写入的时候会把B\[0\]\[0\]到B\[0\]\[7\]的元素存到cache之前存了A\[0][0]到A\[0\]\[7\]的那一行中，接下来读取A\[0][1]的时候，又要把A\[0][0]到A\[0\]\[7\]存回该block中。这便产生了冲突不命中。

具体查看生成的trace.f0文件有：

```
S 18c08c,1 miss 
L 18c0a0,8 miss 
L 18c084,4 hit 
L 18c080,4 hit //不知道前四行在干啥，可能是调用函数之类的
L 10c080,4 miss eviction //load A[0][0] in set4
S 14c080,4 miss eviction //store B[0][0] in set 4, evict A[0][0]~A[0][7] from set4
L 10c084,4 miss eviction //load A[0][1] int set4 thrash!!!
S 14c100,4 miss //store B[1][0] in set8
L 10c088,4 hit //load A[0][2] in set4
S 14c180,4 miss //store B[2][0] in set12
L 10c08c,4 hit //load A[0][3] in set4
S 14c200,4 miss //store B[3][0] in set16
L 10c090,4 hit //load A[0][4] in set4
S 14c280,4 miss //store B[4][0] in set20
L 10c094,4 hit //load A[0][5] in set4
S 14c300,4 miss //store B[5][0] in set24
L 10c098,4 hit //load A[0][6] in set4
S 14c380,4 miss //store B[6][0] in set28
L 10c09c,4 hit //load A[0][7] in set4
S 14c400,4 miss //load B[7][0] in set0
L 10c0a0,4 miss eviction //load A[0][8] in set5
S 14c480,4 miss eviction //store B[8][0] in set4, evict A[0][0]~A[0][7] from set4
...
```

从第五行开始看，A首地址在0x10c080、B首地址在0x14c080。这样对角线的元素会产生抖动，增加一次miss，对角线共32个元素，那么1152+32=1184，和实际结果的1183十分接近。算是一个不错的估算了，可能中间还有些细节没有考虑到。

**2) naive solution**

方法1每次写入B矩阵都产生了miss，而方法2利用分块技术来避免写入B矩阵时产生的大量miss。采取的分块是8\*8的，一方面，每个cache块存8个int；另一方面两个8\*8的块用16个cache块，不会产生容量不命中。而且8又是32的约数，对编码来说比较简单。于是有：

```c
int i, j, m, n;
for (i = 0; i < N; i += 8)
    for (j = 0; j < M; j += 8)
        for (m = i; m < i + 8; ++m)
            for (n = j; n < j + 8; ++n)
            {
                B[n][m] = A[m][n];
            }
```

测试出miss数为343。还是不能达到满分。

解释：

对于主对角线上的8*8分块，仍然会产生冲突不命中。对于不在主对角线上的12个块共有16\*12次miss。对于在主对角线上的四个块，第0行10次miss、第1\~6行4次，第7行3次，这样共有37\*4次miss。算出来是16\*12+4\*37=340。与343很接近。~~不排除算错~~

**3) acceptable solution**

我们知道在主对角线上的块会产生冲突也就是：当把A\[0\]\[0\]写入B\[0\]\[0\]时，会把B\[0\]\[0\]到B\[0\]\[7\]的元素存到cache之前存了A\[0][0]到A\[0\]\[7\]的那一行中，接下来读取A\[0][1]的时候，又要把A\[0][0]到A\[0\]\[7\]存回该block中。那么直接在读A\[0\]\[0\]的时候顺便把A\[0][1\]到A\[0]\[7]都读了不久可以解决问题了吗。于是有：

```c
for (int i = 0; i < N; i += 8)
{
    for (int j = 0; j < M; j += 8)
    {
        for (int k = i; k < i + 8; ++k)
        {
            int v0 = A[k][j];
            int v1 = A[k][j + 1];
            int v2 = A[k][j + 2];
            int v3 = A[k][j + 3];
            int v4 = A[k][j + 4];
            int v5 = A[k][j + 5];
            int v6 = A[k][j + 6];
            int v7 = A[k][j + 7];

            B[j][k] = v0;
            B[j + 1][k] = v1;
            B[j + 2][k] = v2;
            B[j + 3][k] = v3;
            B[j + 4][k] = v4;
            B[j + 5][k] = v5;
            B[j + 6][k] = v6;
            B[j + 7][k] = v7;
        }
    }
}
```

测试结果有miss数为287。

解释：$$12*16+4*24=288$$。优化掉了读取A时候的冲突不命中，但是在写入B的时候还是有8\*4次额外的冲突不命中。但是已经满分了。

理论上最小的miss数为16\*16=256次，但是没有必要再去琢磨了。

### 2.2 64*64

上来就高兴地用刚刚的8\*8分块一跑，miss数高达4611次。朴素的做法也只有4723次miss。。。问题出在写入B时产生了冲突。比如说写入B\[0\]\[0\]\~B\[0\]\[7\]的时候依次写入了set0、set8、set16、set24、set0、set8、set16、set24这样便产生了冲突。

那用4\*4分块跑，如下：

```c
for (i = 0; i < N; i+=4)
{
    for (j = 0; j < M; j+=4)
    {
        for (k = i; k < i + 4; k++)
        {
            v0 = A[k][j];
            v1 = A[k][j+1];
            v2 = A[k][j+2];
            v3 = A[k][j+3];
            B[j][k] = v0;
            B[j+1][k] = v1;
            B[j+2][k] = v2;
            B[j+3][k] = v3;
        }
    }
}
```

miss数还是达1699。这主要由于对块的利用不高导致，一个cache块可以存8个int，但是每次只用4个。

*于是就gg了。百度 启动！*

```c
for (int i = 0; i < N; i += 8)
{
    for (int j = 0; j < M; j += 8)
    {
        for (int k = i; k < i + 4; ++k)
        {
        /* 读取1 2，暂时放在左下角1 2 */
            int temp_value0 = A[k][j];
            int temp_value1 = A[k][j+1];
            int temp_value2 = A[k][j+2];
            int temp_value3 = A[k][j+3];
            int temp_value4 = A[k][j+4];
            int temp_value5 = A[k][j+5];
            int temp_value6 = A[k][j+6];
            int temp_value7 = A[k][j+7];
          
            B[j][k] = temp_value0;
            B[j+1][k] = temp_value1;
            B[j+2][k] = temp_value2;
            B[j+3][k] = temp_value3;
          /* 逆序放置 */
            B[j][k+4] = temp_value7;
            B[j+1][k+4] = temp_value6;
            B[j+2][k+4] = temp_value5;
            B[j+3][k+4] = temp_value4;
        }
         for (int l = 0; l < 4; ++l)
        {
           /* 按列读取 */
            int temp_value0 = A[i+4][j+3-l];
            int temp_value1 = A[i+5][j+3-l];
            int temp_value2 = A[i+6][j+3-l];
            int temp_value3 = A[i+7][j+3-l];
            int temp_value4 = A[i+4][j+4+l];
            int temp_value5 = A[i+5][j+4+l];
            int temp_value6 = A[i+6][j+4+l];
            int temp_value7 = A[i+7][j+4+l];
           
           /* 从下向上按行转换2到3 */
            B[j+4+l][i] = B[j+3-l][i+4];
            B[j+4+l][i+1] = B[j+3-l][i+5];
            B[j+4+l][i+2] = B[j+3-l][i+6];
            B[j+4+l][i+3] = B[j+3-l][i+7];
           /* 将3 4放到正确的位置 */
            B[j+3-l][i+4] = temp_value0;
            B[j+3-l][i+5] = temp_value1;
            B[j+3-l][i+6] = temp_value2;
            B[j+3-l][i+7] = temp_value3;
            B[j+4+l][i+4] = temp_value4;
            B[j+4+l][i+5] = temp_value5;
            B[j+4+l][i+6] = temp_value6;
            B[j+4+l][i+7] = temp_value7;
        } 
    }
}
```

这个miss数是1243，已经满分了。相当tricky的解决办法。大体思路：通过把8\*8的块分成四个4\*4的块进行处理，即充分利用了cache块可以存八个int的能力，又避免了写入B产生的冲突。本代码来自[CS:APP3e 深入理解计算机系统_3e CacheLab实验 - 李秋豪 - 博客园 (cnblogs.com)](https://www.cnblogs.com/liqiuhao/p/8026100.html?utm_source=debugrun&utm_medium=referral)，博客里面有详细的图片解释，可以学习一下。~~本人只是意会了一下~~

### 2.3 61*67

这里要求放的很宽，只要小于2000就行，下面这样的16*16分块就行了。

```c
for (i = 0; i < N; i+=16)
{
    for (j = 0; j < M; j+=16)
    {
        for (k = i; k < i + 16 && k < N; k++)
        {
            for (l = j; l < j + 16 && l < M; l++)
            {
                B[l][k] = A[k][l];
            }
        }
    }
}
```

miss数1992，刚刚够！

***



## 3. 写在后面

本人于2021/11/2完成了Cache Lab，差不多用了四天时间吧。Cache Lab算是目前最有代码量的一个lab了。Part A要求实现cache simulator，完全是自己写的代码（不像arch lab那样），整个完成的思路还是比较清晰的。Part B很多还是要参考网上的写法，尤其是64*64的分块还是可以继续深入下去玩味的。总的来说，Cache Lab也挺不错的！

个人主要强化了一下知识：

- main memory中的数据如何缓存到cache之中

  （什么t、s、b、E呀乱七八糟的东西应该真的记住了）

- main函数如何优雅地处理argv的参数

- c programming! （学校里都是c++

- 分块技术降低miss数

有意思的是：

- wsl2测试part B的速度十分慢，会产生超时的情况。

- Windows下mingw缺少<sys/wait.h>库文件，却可以编译test-trans.c文件。

  妙啊，原来编辑器是在windows环境下所以找不到这个库文件，但是终端是在linux环境下，所以可以成功编译

预计接下来：

- shell lab启动？
- 吃晚饭！
