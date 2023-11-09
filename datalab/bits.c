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
  3. Has unpredictable behavior when shifting an integer by more
     than the word size.

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

For the problems that require you to implent floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants.

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
  2. Each function has a maximum number of operators (! ~ & ^ | + << >>)
     that you are allowed to use for your implementation of the function. 
     The max operator count is checked by dlc. Note that '=' is not 
     counted; you may use as many of these as you want without penalty.
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
/* Copyright (C) 1991-2022 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */
/* This header is separate from features.h so that the compiler can
   include it implicitly at the start of every compilation.  It must
   not itself include <features.h> or any other header that includes
   <features.h> because the implicit include comes before any feature
   test macros that may be defined in a source file before it first
   explicitly includes a system header.  GCC knows the name of this
   header in order to preinclude it.  */
/* glibc's intent is to support the IEC 559 math functionality, real
   and complex.  If the GCC (4.9 and later) predefined macros
   specifying compiler intent are available, use them to determine
   whether the overall intent is to support these features; otherwise,
   presume an older compiler has intent to support these features and
   define these macros by default.  */
/* wchar_t uses Unicode 10.0.0.  Version 10.0 of the Unicode Standard is
   synchronized with ISO/IEC 10646:2017, fifth edition, plus
   the following additions from Amendment 1 to the fifth edition:
   - 56 emoji characters
   - 285 hentaigana
   - 3 additional Zanabazar Square characters */
/* 
 * bitAnd - x&y using only ~ and | 
 *   Example: bitAnd(6, 5) = 4
 *   Legal ops: ~ |
 *   Max ops: 8
 *   Rating: 1
 */
int bitAnd(int x, int y) {
  int a = ~x;
  int b = ~y;
  return ~(a | b);
}
/* 
 * bitConditional - x ? y : z for each bit respectively
 *   Example: bitConditional(0b00110011, 0b01010101, 0b00001111) = 0b00011101
 *   Legal ops: & | ^ ~
 *   Max ops: 8
 *   Rating: 1
 */
int bitConditional(int x, int y, int z) {
  int a = ~x;
  int la = x & y;
  int ra = a & z;
  return la | ra;
}
/* 
 * byteSwap - swaps the nth byte and the mth byte
 *  Examples: byteSwap(0x12345678, 1, 3) = 0x56341278
 *            byteSwap(0xDEADBEEF, 0, 2) = 0xDEEFBEAD
 *  You may assume that 0 <= n <= 3, 0 <= m <= 3
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 25
 *  Rating: 2
 */
int byteSwap(int x, int n, int m) {
    int rn = n << 3;
    int rm = m << 3;
    int an = (x >> rn) & 255;
    int am = (x >> rm) & 255;
    int ms = an << rm;
    int ns = am << rn;
    x = x & (~(255 << rn));
    x = x & (~(255 << rm));
    x = x | ms;
    x = x | ns;
    return x;
}
/* 
 * logicalShift - shift x to the right by n, using a logical shift
 *   Can assume that 0 <= n <= 31
 *   Examples: logicalShift(0x87654321,4) = 0x08765432
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 3 
 */
int logicalShift(int x, int n) {
  int a = ~(1 << 31);
  int sig = !n;
  x = x >> n;
  a = a + (sig << 31);
  a = a >> n;
  a = a << 1;
  a = a + 1;  
  return x & a;
}
/* 
 * cleanConsecutive1 - change any consecutive 1 to zeros in the binary form of x.
 *   Consecutive 1 means a set of 1 that contains more than one 1.
 *   Examples cleanConsecutive1(0x10) = 0x10
 *            cleanConsecutive1(0xF0) = 0x0
 *            cleanConsecutive1(0xFFFF0001) = 0x1
 *            cleanConsecutive1(0x4F4F4F4F) = 0x40404040
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 25
 *   Rating: 4
 */
int cleanConsecutive1(int x){
    int y = ~(x << 1);
    int ans1 = x & y;
    int z = x >> 1;
    int s = ~(1 << 31);
    int z2 = ~(z & s);
    int ans2 = x & z2;
    return ans1 & ans2;

}
/* 
 * countTrailingZero - return the number of consecutive 0 from the lowest bit of 
 *   the binary form of x.
 *   YOU MAY USE BIG CONST IN THIS PROBLEM, LIKE 0xFFFF0000
 *   YOU MAY USE BIG CONST IN THIS PROBLEM, LIKE 0xFFFF0000
 *   YOU MAY USE BIG CONST IN THIS PROBLEM, LIKE 0xFFFF0000
 *   Examples countTrailingZero(0x0) = 32, countTrailingZero(0x1) = 0,
 *            countTrailingZero(0xFFFF0000) = 16,
 *            countTrailingZero(0xFFFFFFF0) = 8,
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 40
 *   Rating: 4
 */
int countTrailingZero(int x){
    int op = (~x) + 1;
    x = ~(x | op);
    x = (x & 0x55555555) + ((x & 0xAAAAAAAA) >> 1);
    x = (x & 0x33333333) + ((x & 0xCCCCCCCC) >> 2);
    x = (x & 0x0F0F0F0F) + ((x & 0xF0F0F0F0) >> 4);
    x = (x & 0x00FF00FF) + ((x & 0xFF00FF00) >> 8);
    x = (x & 0x0000FFFF) + ((x & 0xFFFF0000) >> 16);
    return x;

}
/* 
 * divpwr2 - Compute x/(2^n), for 0 <= n <= 30
 *  Round toward zero
 *   Examples: divpwr2(15,1) = 7, divpwr2(-33,4) = -2
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 2
 */
int divpwr2(int x, int n) {
    int sig = (x >> 31) & 1;
    int rem = ((1 << n) + (~0)) & x;
    int isrem = !!rem;
    int tail = isrem & sig;
    return (x >> n) + tail;
}
/* 
 * oneMoreThan - return 1 if y is one more than x, and 0 otherwise
 *   Examples oneMoreThan(0, 1) = 1, oneMoreThan(-1, 1) = 0
 *   Legal ops: ~ & ! ^ | + << >>
 *   Max ops: 15
 *   Rating: 2
 */
int oneMoreThan(int x, int y) {
  int sigx = (x >> 31) & 1;
  int a = x + 1;
  int siga = (a >> 31) & 1;
  int eno = (siga ^ 1) | (siga & sigx);
  int equ = !(a ^ y);
  return eno & equ;
}
/*
 * satMul3 - multiplies by 3, saturating to Tmin or Tmax if overflow
 *  Examples: satMul3(0x10000000) = 0x30000000
 *            satMul3(0x30000000) = 0x7FFFFFFF (Saturate to TMax)
 *            satMul3(0x70000000) = 0x7FFFFFFF (Saturate to TMax)
 *            satMul3(0xD0000000) = 0x80000000 (Saturate to TMin)
 *            satMul3(0xA0000000) = 0x80000000 (Saturate to TMin)
 *  Legal ops`: ! ~ & ^ | + << >>
 *  Max ops: 25
 *  Rating: 3
 */
int satMul3(int x) {
    int sig = (x >> 31) & 1;
    int lim = ~(~sig + 1);
    int lim2 = lim ^ (1 << 31);
    int mul2 = x << 1;
    int mul3 = mul2 + x;
    int sig2 = (mul2 >> 31) & 1;
    int sig3 = (mul3 >> 31) & 1;
    int if1 = sig ^ sig2; // 溢出可能1：第一次翻倍，变号
    int if2 = sig2 ^ sig3; // 溢出可能2：加第三个x，变号
    int ifn = if1 | if2;// ifn=1溢出
    int s1 = ~ifn + 1; // s1=-1溢出,0正常
    int ans = (mul3 & (~s1)) | (s1 & lim2);
    return ans; 
}
/* 
 * subOK - Determine if can compute x-y without overflow
 *   Example: subOK(0x80000000,0x80000000) = 1,
 *            subOK(0x80000000,0x70000000) = 0, 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 3
 */
int subOK(int x, int y) {
  int sigx = (x >> 31) & 1;
  int sigy = (y >> 31) & 1;
  int minus_y = ~y + 1;
  int val = x + minus_y;
  int sigv = (val >> 31) & 1;
  int if1 = sigx ^ sigy; // x和y符号不同
  int if2 = sigx ^ sigv; // x和x-y符号不同
  int if3 = if1 & if2; // 溢出的情况
  return !if3;
}
/* 
 * isLessOrEqual - if x <= y  then return 1, else return 0 
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y) {
  int sigx = (x >> 31) & 1;
  int sigy = (y >> 31) & 1;
  int if1 = sigx & (!sigy); // x符号位为1y符号位0，x<y
  int if2 = !(sigx ^ sigy); // x符号位和y相同
  int minus_x = ~x + 1;
  int xtm = sigx & !(x ^ minus_x); // x是tmin
  int y_minus_x = y + minus_x;
  int siga = (y_minus_x >> 31) & 1; //siga=0,y-x>=0,x<=y
  return (if1 | xtm) | (if2 & (!siga)); 
}
/*
 * trueThreeFourths - multiplies by 3/4 rounding toward 0,
 *   avoiding errors due to overflow
 *   Examples: trueThreeFourths(11) = 8
 *             trueThreeFourths(-9) = -6
 *             trueThreeFourths(1073741824) = 805306368 (no overflow)
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 4
 */
int trueThreeFourths(int x)
{
  int tail = 3 & x;
  int sig = (x >> 31) & 1;
  int big = (x >> 2);
  int cnt1 = (big << 1) + big; // 能整除4的部分
  int remain = (tail << 1) + tail; // 剩余部分
  int tail2 = !!(3 & remain); // 有余数
  int cnt2 = (remain >> 2) + (sig & tail2);
  return cnt1 + cnt2;
}
/* 
 * float_twice - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_twice(unsigned uf) {
  unsigned s = 0x80000000 & uf;
  unsigned exp = 0x7F800000 & uf;
  unsigned frac = 0x007FFFFF & uf;
  if(exp == 0x7F800000){ // NaN & Inf
	  return uf;
  }
  if((exp == 0)){
	  return (s | exp) | (frac << 1); // 非规格化
  }
  uf += 0x00800000;
  if(exp == 0x7F000000){
	  return 0x7F800000 | s; // 溢出
  }
  return uf; // 规格化
}
/* 
 * float_i2f - Return bit-level equivalent of expression (float) x
 *   Result is returned as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point values.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_i2f(int x) {
  int s1 = 0; // 符号位数值+位置
  int abs_x = x; // x的绝对值(负数化为正数处理）
  int k = 0; // 最高位1移动k位到31
  int tmp = 0;
  int exp = 0;
  int frac = 0;
  int bu = 0; // 是否补1
  if(x == 0x80000000) return 0xCF000000; // tmin
  if(x == 0) return 0; // 0
  if(x < 0){
	s1 = 0x80000000;
	abs_x = -x;
  }
  tmp = abs_x;
  while((0x80000000 & tmp) == 0){
  	tmp = tmp << 1;
	k += 1;
  }
  if((0x000000FF & tmp) > 0x00000080){
  	bu = 1; // 舍去位大于0.11...1
  }
  else if((0x00000180 & tmp) == 0x00000180){
  	bu = 1; // 舍去位等于0.1且最低有效位1
  }
  exp = (158 - k) << 23;
  frac = (0x7FFFFFFF & tmp) >> 8;
  return s1 + exp + frac + bu;
}
/* 
 * float_f2i - Return bit-level equivalent of expression (int) f
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
int float_f2i(unsigned uf) {
  int s = 0x80000000 & uf;
  int exp = 0x7F800000 & uf;
  int frac = 0x007FFFFF & uf;
  int tmp;
  int v_exp = exp >> 23;
  int ans;
  if(uf == 0xCF000000){
  	return 0x80000000; // tmin
  }
  if((v_exp < 127) || (uf == 0x80000000)){
  	return 0; // 0
  }
  else if((exp + frac) >= 0x4F000000){ //溢出，inf，nan
  	return 0x80000000u;
  }
  tmp = frac + (1 << 23);
  tmp = tmp << 7;
  tmp = tmp >> (157 - v_exp);
  ans = tmp;
  if(s != 0){
  	ans = -ans;
  }
  return ans;
}
/* 
 * float_pwr2 - Return bit-level equivalent of the expression 2.0^x
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
unsigned float_pwr2(int x) {
    int exp = 127;
    int pos;
    if(x >= 0){
    	if(x <= 127) return (exp + x) << 23;
	return 0x7F800000;
    }
    if(x < -149) return 0;
    if(x >= -126) return (exp + x) << 23;
    pos = 149 + x;
    return (1 << pos);
}
