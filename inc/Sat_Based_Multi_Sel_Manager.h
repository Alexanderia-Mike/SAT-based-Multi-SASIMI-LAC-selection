//
// Created by 娄辰飞 on 2021/5/17.
//

#include "sasimi.h"
#include "cnf2Depqbf.h"
#include "sortingNetwork.h"
#include <fstream>
#include <sstream>

extern int truncateSize;
extern int lacApplyMode;

class SBMSM_t
{
  private:
    // file names
    char cnfFileName[200];
    char SATResultFileName[200];
    char SATProblemFileName[200];
    char aigFileName[200];
    char naiveMiterName[200];
    char verifyMiterName[200];
    char aigFileNameDup[200];
    char cnfFileNameDup[200];
    char cnfFileNameVerifyMiter[200];
    // networks
    Abc_Ntk_t * pOriNtk;
    Abc_Ntk_t * pAppNtk;
    Abc_Ntk_t * pMiter;
    Abc_Ntk_t * pAppNtkDup;
    // network parameters
    unsigned int OriPINum;
    unsigned int MUXPINum;
    // list to store cnf parameters
    int * OriPIIDs;
    int * MUXPIIDs;
    int nCnfVars;
    // list for LACs
    std::vector <LAC_t> candLACs;        // all the candidates (more concise)
    std::vector <LAC_t> candLACsDup;
    // threshold, dynamically allocated
    int * threshold;
    int threshold_value;
    // sasimi manager
    SASIMI_Manager_t sasimiMng;
    void print_candLAC( const int maximum_range );
    ////////////////////////////////////////////////////////////
    ///                  helping functions                   ///
    ////////////////////////////////////////////////////////////
  public:
    void Abc_Ntk_Dup_Level();
    void Abc_Ntk_Assign_Level();
    SBMSM_t( IN Abc_Ntk_t * _pOriNtk, int _threshold[], SASIMI_Manager_t & _sasimiMng );
    ~SBMSM_t();
    void SAT_Based_Multi_Selection();
    void collect_LACs();
    bool candLAC_check_size();
    void candLAC_truncate_size( unsigned int size );
    bool create_final_CNF ();
    void create_naive_miter();
    void create_sorted_miter( std::vector<Abc_Obj_t *> & sortedSelectionSignals, int selectionIndex );
    void create_sorted_miter_dup( std::vector<Abc_Obj_t *> & sortedSelectionSignalsDup, int pointer, Abc_Ntk_t * pMiterDup );
    bool create_miter_finalize();
    void add_muxes();
    void abc_obj_transfer_fanouts_except_mux ( IN Abc_Obj_t * pNodeFrom, IN Abc_Obj_t * pNodeTo, IN Abc_Obj_t * pMux );
    void abc_node_collect_fanouts_except_mux ( IN Abc_Obj_t * pNode, IN Vec_Ptr_t * vNodes, IN Abc_Obj_t * pMux );
    void create_subtractor_comparator_miter ();
    Abc_Obj_t ** X_subtract_Y_absolute(Abc_Ntk_t * pNtk, Abc_Obj_t * X[], Abc_Obj_t * Y[], int n);
    Abc_Obj_t * X_less_than_Y(Abc_Ntk_t * pNtk, Abc_Obj_t * X[], Abc_Obj_t * Y[], int n);
    Abc_Obj_t * const_node ( Abc_Ntk_t * pNtk, int n );
    Abc_Obj_t ** const_nodes ( Abc_Ntk_t * pNtk, int threshold[], int size );
    void miter_check ();
    void miter_add_sorting_network_area_encoded ( std::vector<Abc_Obj_t *> &sortedSelectionSignals, const bool restrict_range, const int maximum_range );
    void range_map ( std::vector<int> & range_mapped_list, const std::vector<int> & original_list, const int maximum_range );
    double calculate_total_reduced_area ();
    double calculate_total_reduced_area_s_a_a_d ();
    void apply_lac ( LAC_t & lac );
    void apply_lac_s_a_a_d ( LAC_t & lac );
    void batch_deletion_s_a_a_d ();
    void alexanderia_replace_obj( Abc_Obj_t * pNodeOld, Abc_Obj_t * pNodeNew );
    void alexanderia_replace_obj_s_a_a_d( Abc_Obj_t * pNodeOld, Abc_Obj_t * pNodeNew );
    void alexanderia_delete_obj_rec( Abc_Obj_t * pObj );
    void assign_PIIDs( Cnf_Dat_t * miterCnfData, bool print );
    void miter_add_sorting_network ( std::vector<Abc_Obj_t *> &sortedSelectionSignals );
    int sorted_selection_find_least ( std::vector<Abc_Obj_t *> &sortedSelectionSignals );
    void transform_miter_to_cnf ();
    void transform_miter_to_cnf_dup ( Abc_Ntk_t * pMiterDup );
    // for verify miter
    void build_verify_miter();
    void solve_verify_miter_cnf();
    void create_naive_miter_from_networks( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2 );
    void transform_verify_miter_to_cnf();
    void create_subtractor_comparator_miter_from_networks( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2 );
    void assign_PIIDs_for_verify_miter( Cnf_Dat_t * miterCnfData, bool print );
};