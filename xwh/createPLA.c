#include "createPLA.h"
using namespace std;

string IntToBitString(int num,int precision)
{
	//vector<int> bits;
	string s_bits;
	string bit;
	for (int i = 0; i < precision; i++)
	{
		bit = char(((num % 2) - 0) + '0');
		s_bits.insert(0,bit);
		num /= 2;
	}
	return s_bits;
}

void createPLA(string FilePath, int nInput, int nOutput, vector<int> Onset)
{
    ofstream f_pla;
    assert(Onset.size()<=pow(2,nInput));
    f_pla.open(FilePath.c_str());
	f_pla << ".i "<<nInput<<endl<<".o "<<nOutput<<endl;
	for (int i = 0; i < Onset.size(); i++)
	{
		f_pla << IntToBitString(Onset[i],nInput) + " 1" << endl;
	}
	f_pla << ".e";
	f_pla.close();
}



