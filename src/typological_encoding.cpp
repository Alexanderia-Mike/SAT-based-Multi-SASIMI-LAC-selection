#include "Typological_Structure_Encoded_Sat_Based_Multi_Sel_Manager.h"
#include <algorithm>

using namespace std;

// create a adjacent matrix where 
void 
SBMSM_t::get_transitive_fanin_cone_using_matrix()
{
    if ( pAppNtk == nullptr )
    {
        cout << "get_transitive_fanin_cone_using_matrix: no network specified!" << endl;
        exit( 0 );
    }
    int max_id = get_max_id();
    transitive_fanin_cone_matrix.resize( max_id + 1 );
}

int 
SBMSM_t::get_max_id()
{
    Abc_Obj_t * pObj; 
    int i = 0; 
    int maxId = -1; 
    Abc_NtkForEachObj(pAppNtk, pObj, i) 
        maxId = max(maxId, pObj->Id);
    return maxId;
}