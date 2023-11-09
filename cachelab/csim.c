// 姓名：孙少凡
// 学号：2100013085
#include "cachelab.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>

int S, B, s, E, b; // cache相关参数
int nHit, nMiss, nEvic = 0; // 统计变量
char t[100]; // 存文件名

// cache line 结构定义
typedef struct {
    char valid_bit; // 有效位
    unsigned long tag; // 标志位
    int past_time; // 最后一次使用时间距今操作数
} cache_line;
typedef cache_line* cache_set; // 组
typedef cache_line** cache; // 整个缓存

cache myCache = NULL;

void init_cache(){ // 初始化cache
    S = 1 << s;
    B = 1 << b;
    myCache = (cache)malloc(S * sizeof(cache_set)); // 为myCache分配S组的空间
    for(int i = 0; i < S; i++){
        myCache[i] = (cache_set)malloc(E * sizeof(cache_line)); // 为每个组分配E行的空间
        for(int j = 0; j < E; j++){ // 初始化每行
            myCache[i][j].valid_bit = 0;
            myCache[i][j].tag = 0;
            myCache[i][j].past_time = 0;
        }
    }
}

void free_cache(){ // 释放内存
    for(int i = 0; i < S; i++){
        free(myCache[i]);
    }
    free(myCache);
}

void update_time(){ // 更新每个有效行的距今时间
    for(int i = 0; i < S; i++){
        for(int j = 0; j < E; j++){
            if(myCache[i][j].valid_bit == 1){
                myCache[i][j].past_time++;
            }
        }
    }
}

int cal_set(unsigned long addr){ // 计算组索引
    return (addr >> b) & (((unsigned long)-1) >> (64 - s));
}

unsigned long cal_tag(unsigned long addr){ // 计算tag
    return addr >> (b + s);
}

void deal(unsigned long addr){ // 处理一个地址
    int sindex = cal_set(addr); // 组索引
    unsigned long tag = cal_tag(addr); // tag
    // hit
    for(int j = 0; j < E; j++){
        if(myCache[sindex][j].valid_bit == 1 && myCache[sindex][j].tag == tag){ // hit
            nHit++; // 命中次数+1
            myCache[sindex][j].past_time = 0; // 重置时间
            return;
        }
    }
    // miss
    for(int j = 0; j < E; j++){
        if(myCache[sindex][j].valid_bit == 0){ // 空行
            nMiss++; // 不命中次数+1
            myCache[sindex][j].valid_bit = 1;
            myCache[sindex][j].tag = tag;
            myCache[sindex][j].past_time = 0; // 替换空行
            return;
        }
    }
    // 如果不命中且无空行，使用LRU策略驱逐一个
    nMiss++;
    nEvic++;
    int maxtime = 0;
    int id = -1;
    for(int j = 0; j < E; j++){ // 寻找最后一次操作距今时间最长的行
        if(myCache[sindex][j].past_time > maxtime){
            maxtime = myCache[sindex][j].past_time;
            id = j;
        }
    }
    myCache[sindex][id].valid_bit = 1;
    myCache[sindex][id].tag = tag;
    myCache[sindex][id].past_time = 0;
}

void perform(){ // 读取并执行操作
    init_cache(); // 初始化cache
    FILE * fp = fopen(t, "r"); // 读取文件
    char op;
    unsigned long addr;
    int size;
    while(fscanf(fp, " %c %lx,%d\n", &op, &addr, &size) > 0){
        switch(op){
            case 'L':
                deal(addr);
                break;
            case 'M': // modify操作需要两步，故不break
                deal(addr);
            case 'S':
                deal(addr);
        }
        update_time();
    }
    fclose(fp);
    free_cache();
}

int main(int argc, char *argv[]){
    int opt; // 接受getopt返回值
    while((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1){
        switch(opt){
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
        }
    }
    perform();
    printSummary(nHit, nMiss, nEvic);
    return 0;
}