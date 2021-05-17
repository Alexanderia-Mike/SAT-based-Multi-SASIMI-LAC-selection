# include "abcApi.h" // the directory to abc api header files

extern Abc_Obj_t ** X_subtract_Y_abs(Abc_Ntk_t * pNtk, Abc_Obj_t * X[], Abc_Obj_t * Y[], int n);
extern Abc_Ntk_t * CreateMiterXorMulti ( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2);