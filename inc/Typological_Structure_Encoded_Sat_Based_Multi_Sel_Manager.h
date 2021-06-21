//
// Created by 娄辰飞 on 2021/5/17.
//

#include "sasimi.h"
#include "typological_structure_encoded_cnf2Depqbf.h"
#include "typological_encoding.h"
#include "sortingNetwork.h"
#include <fstream>
#include <sstream>

class SBMSM_t
{
  private:
    // file names
    char cnfFileName[100];
    char SATResultFileName[100];
    char aigFileName[100];
    char naiveMiterName[100];
    char verifyMiterName[100];
    char aigFileNameDup[100];
    char cnfFileNameDup[100];
    char cnfFileNameVerifyMiter[100];
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
    int * structure_code_ids;
    int nCnfVars;
    // list for LACs
    std::vector <LAC_t> candLACs;        // all the candidates (more concise)
    std::vector <LAC_t> candLACsDup;
    // threshold, dynamically allocated
    int * threshold;
    int threshold_value;
    // sasimi manager
    SASIMI_Manager_t sasimiMng;

    // for structural encoding
    std::vector<std::vector<uint64_t> > transitive_fanin_cone_matrix;   
    // Ckt_GetBit( transitive_fanin_cone_matrix[x->Id][z->Id >> 6], z->Id ) = 1 means node x have node z as its fanin
    std::vector<std::vector<uint64_t> > LAC_priority_matrix;
    ////////////////////////////////////////////////////////////
    ///                  helping functions                   ///
    ////////////////////////////////////////////////////////////
    void print_candLAC( const int maximum_range );
    int get_max_id();
    bool check_a_in_b_mffc( Abc_Obj_t * a, Abc_Obj_t * b );
    void reassign_pclauses( Cnf_Dat_t * miter_cnf_data );
  public:
    void Abc_Ntk_Dup_Level();
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
    void apply_lac ( LAC_t & lac );
    void alexanderia_replace_obj( Abc_Obj_t * pNodeOld, Abc_Obj_t * pNodeNew );
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

    // for typological_encoding
    void get_transitive_fanin_cone_using_matrix();
    void print_transitive_fanin_cone_matrix();
    void get_LAC_priority_factor();
    void print_LAC_priority_factor();
    void miter_cnf_add_structural_clauses( Cnf_Dat_t * miter_cnf_data );
};