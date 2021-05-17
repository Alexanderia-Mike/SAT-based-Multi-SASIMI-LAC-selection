#ifndef _CREATEPLA_H_
#define _CREATEPLA_H_
#include <iostream>
#include <fstream>
#include <cmath>
#include <string>
#include<assert.h>
#include <vector>
using namespace std;

extern "C" {
string IntToBitString(int num,int precision);
void createPLA(string FilePath, int nInput, int nOutput,vector<int> Onset);
}

#endif