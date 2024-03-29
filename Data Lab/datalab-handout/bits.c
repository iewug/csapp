/* 
 * CS:APP Data Lab 
 * 
 * <Please put your name and userid here>
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting if the shift amount
     is less than 0 or greater than 31.


EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implement floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants. You can use any arithmetic,
logical, or comparison operations on int or unsigned data.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operations (integer, logical,
     or comparison) that you are allowed to use for your implementation
     of the function.  The max operator count is checked by dlc.
     Note that assignment ('=') is not counted; you may use as many of
     these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif
//1
/* 
 * bitXor - x^y using only ~ and & 
 *   Example: bitXor(4, 5) = 1
 *   Legal ops: ~ &
 *   Max ops: 14
 *   Rating: 1
 */
/*实现异或运算，用离散知识易得*/
int bitXor(int x, int y) {
  return ~(~(~x&y)&~(x&~y));
}
/* 
 * tmin - return minimum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
/*最小的二进制补码就是100...0*/
int tmin(void) {
  return 1 << 31;
}
//2
/*
 * isTmax - returns 1 if x is the maximum, two's complement number,
 *     and 0 otherwise 
 *   Legal ops: ! ~ & ^ | +
 *   Max ops: 10
 *   Rating: 1
 */
/**
 * 题意：判断x是否为最大的二进制补码数
 * 相反数等于自己的数有min以及0，这里利用该性质实现函数
 * 若x是max，x+1后成了min
 * 又利用-x == ~x+1 （这是显然的
 * 通过异或^运算判断是否相等：a^b == 0 iff a == b
 * 最后还要排除x==0的情况
*/
int isTmax(int x) {
  x += 1;
  return (!(x^(~x+1)))&(!!x);
}
/* 
 * allOddBits - return 1 if all odd-numbered bits in word set to 1
 *   where bits are numbered from 0 (least significant) to 31 (most significant)
 *   Examples allOddBits(0xFFFFFFFD) = 0, allOddBits(0xAAAAAAAA) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 */
/**
 * 题意：判读x的奇数位是否全为1
 * 照着移移就知道了
 */
int allOddBits(int x) {
  x = x & (x >> 2);
  x = x & (x >> 4);
  x = x & (x >> 8);
  x = x & (x >> 16);
  return (x & 2) >> 1;
}
/* 
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */

/**
 * 题意：实现相反数
 * 这为什么会是rating2？这不是简单的结论吗，甚至之前的isTmax也用到了该性质
 */
int negate(int x) {
  return ~x + 1;
}
//3
/* 
 * isAsciiDigit - return 1 if 0x30 <= x <= 0x39 (ASCII codes for characters '0' to '9')
 *   Example: isAsciiDigit(0x35) = 1.
 *            isAsciiDigit(0x3a) = 0.
 *            isAsciiDigit(0x05) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 3
 */
/**
 * 题意：return 1 if 0x30 <= x <= 0x39 
 * 注意：!x == 1 iff x == 0, e.g.!(-1) = 0;
 * 利用a + ~b+1 实现a-b
 * 利用最高位（31位）是否为1来判断x是否为负数：当a<0时，a>>31==-1；当a>=0时，a>>31==0
 * 故当a<0时!(a>>31)==0；当a>=0时!(a>>31)==1
 */
int isAsciiDigit(int x) {
  return !((~0x30 + x + 1)>>31) & !((~x + 0x39 + 1)>>31);
}
/* 
 * conditional - same as x ? y : z 
 *   Example: conditional(2,4,5) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
/**
 * 题意：x ? y : z 
 * 利用掩码选择y与z
 * 利用x设置掩码mask为0xFFFFFFFF或者0x0
 */
int conditional(int x, int y, int z) {
  int tmp = !x;
  int mask = ~tmp + 1;
  return (~mask & y) + (mask & z);
}
/* 
 * isLessOrEqual - if x <= y  then return 1, else return 0 
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
/**
 * 题意：if x <= y  then return 1, else return 0
 * 简单地利用!((y+~x+1)>>31)存在溢出问题，使判断式出错。需要由符号是否相同分类。
 * 利用difSign作为掩码，当正负性相同时，difSign为0，否则为-1
 * 这样综合利用conditional和isAsciiDigit中的方法即可解决
 */
int isLessOrEqual(int x, int y) {
  int difSign = (x^y)>>31;
  return (~difSign&!((y+~x+1)>>31))+(difSign&(!(y>>31)));
}
//4
/* 
 * logicalNeg - implement the ! operator, using all of 
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
/**
 * 题意：实现！
 * 利用 只有0和它的相反数同时满足最高位为0 的特点实现
 */ 
int logicalNeg(int x) {
  return ~((x|(~x+1))>>31)&1;
}
/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */
/**
 * 题意：求表示x的二进制补码的最小位数
 * 我这么理解的：二分查找最高位1的位置，最后加上符号位即可
 * 一开始把负数x取非，为了把最前面无效的1给去掉
 */
int howManyBits(int x) {
  int b16,b8,b4,b2,b1,b0;
  int sign=x>>31;
  x = (sign&~x)|(~sign&x);//如果x为正，则x不变；如果x为负，则x改为~x
  // 不断缩小范围
  b16 = !!(x>>16)<<4;//高十六位是否有1
  x = x>>b16;//如果有（至少需要16位），则将原数右移16位
  b8 = !!(x>>8)<<3;//剩余位高8位(i.e.8-15位）是否有1
  x = x>>b8;//如果有（至少需要16+8=24位），则右移8位
  b4 = !!(x>>4)<<2;//同理
  x = x>>b4;
  b2 = !!(x>>2)<<1;
  x = x>>b2;
  b1 = !!(x>>1);
  x = x>>b1;
  b0 = x;
  return b16+b8+b4+b2+b1+b0+1;//+1表示加上符号位
}
//float
/* 
 * floatScale2 - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
/**
 * 注释：float数都是把unsigned int当作float来使的
 */
/**
 * 题意：如果uf为特殊值（i.e.exp全为1）返回uf，否则返回2*uf
 */ 
unsigned floatScale2(unsigned uf) {
  unsigned exp = uf>>23;
  unsigned ans;
  //NaN and inf，i.e.exp全为1，直接返回uf
  if(!((exp&0xFF)^0xFF))
	  return uf;
  //non-normalized 直接左移
  /**
   * 这里有一个精妙的点，关于frac的最高位为1的情况
   * 此时把uf左移一位，frac最高位变成了exp的最低位，uf从非规格化数变成了规格化数
   * 由于规格化和非规格化E和M的精妙设置，uf左移一位就直接正确了，做到了很好的平稳过渡
   */ 
  if(!(exp&0xFF))
  {
	  unsigned sign = uf>>31<<31;
	  return (uf<<1)|sign;
  }
  //normalized 指数部分加一
  /**
   * 这里也有一个小细节，我们无法通过修改frac来使uf变成两倍，只能通过exp加一的方法
   * 于是，当exp加一后变成全是一后，就是inf情况，需要再把frac置零即可
   */
  ans = (1<<23)+uf;
  //inf
  if (!((exp&0xFF)^0xFF))
	  return ans>>23<<23;
  else
	  return ans;
}
/* 
 * floatFloat2Int - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
/**
 * 题意：实现float到int的强制类型转换
 * 特别的，Anything out of range (including NaN and infinity) should return 0x80000000u.
 */ 
int floatFloat2Int(unsigned uf) {
  int E = ((uf>>23)&0xFF)-127;
  int sign = uf>>31;
  unsigned M = (uf&0x7FFFFF)|0x800000; //加上1，非规格化数已经在E<0的时候被排除了。严格的说这里的M是M * 2^23
  
  if(E<0) return 0;	//由于M<2，所以E<0时必为纯小数
  if(E>=31) return 0x80000000u;	//超出范围。提示：int范围为-2^31~2^31-1，0x80000000u=-2^31，NaN和inf也涵盖
  
  //M是一个23位小数，又注意到这里的M其实是M * 2^23
  if(E>23) M <<= E-23;
  else M >>= 23-E;

  if(sign) return ~M+1;//负数
  else return M;//正数
}
/* 
 * floatPower2 - Return bit-level equivalent of the expression 2.0^x
 *   (2.0 raised to the power x) for any 32-bit integer x.
 *
 *   The unsigned value that is returned should have the identical bit
 *   representation as the single-precision floating-point number 2.0^x.
 *   If the result is too small to be represented as a denorm, return
 *   0. If too large, return +INF.
 * 
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. Also if, while 
 *   Max ops: 30 
 *   Rating: 4
 */
/**
 * 题意：返回2^x的浮点型
 * 其实不难，就分类讨论
 */ 
unsigned floatPower2(int x) {
    //float最大为(2-2^-23)·2^127，小于2^128
    if (x>127)
        return 0xFF<<23; //返回+inf
    else if (x<-149) return 0;//float最接近0的数为2^-149，[00..001]
    else if (x>=-126) //规格化数
    {
        int exp = x + 127;
        return exp << 23;
    }
    else //-126>x>=-149，非规格化
    {
        int t = 149 + x;
        return (1 << t);
    }
}
