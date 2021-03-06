#ifndef SASIMI_H
#define SASIMI_H

#if defined(__BORLANDC__)
    typedef unsigned char uint8_t;
    typedef __int64 int64_t;
    typedef unsigned long uintptr_t;
#elif defined(_MSC_VER)
    typedef unsigned char uint8_t;
    typedef __int64 int64_t;
#else
    #include <stdint.h>
#endif


#include "simulator.h"


class LAC_t;

extern LACSortMethod_t sortingMethod;
extern AreaEncodeMode_t areaEncodeMode;
extern float errorScale;


class SASIMI_Manager_t{
private:
    int nFrame;
    int maxLevel;
    int cntRound;
    Metric_t metricType;
    double errorBound;

    SASIMI_Manager_t & operator = (const SASIMI_Manager_t &);
    friend class SBMSM_t;

public:
    SASIMI_Manager_t(const SASIMI_Manager_t & sasimiMng);
    explicit SASIMI_Manager_t(Abc_Ntk_t * pNtk, int nFrame, int maxLevel, Metric_t metricType, double errorBound);
    ~SASIMI_Manager_t();
    // to apply SASIMI local approximate change until <errorBound> is reached,
    // and store the result in the file "<outPrefix>_sth.blif"
    void GreedySelection(Abc_Ntk_t * pOriNtk, std::string outPrefix);
    // to add 0s and 1s to candidate list of pNtk
    void PatchConst(Abc_Ntk_t * pNtk);
    // find the Maximum-Fanout-Free-Cones and store them in <vMffcs>
    void CollectMFFC(IN Simulator_t & appSmlt, OUT std::vector <Vec_Ptr_t *> & vMffcs);
    // free the memory in <vMffcs>
    void FreeMFFC(std::vector <Vec_Ptr_t *> & vMffcs);
    // get the matrix CPM and update <bds> (boolean difference)
    void GetCPM(IN Simulator_t & oriSmlt, IN Simulator_t & appSmlt, OUT std::vector < std::vector <tVec> > & bds);
    // a more advanced method ot calculate CPM
    void GetCPMOneCut(IN Simulator_t & oriSmlt, IN Simulator_t & appSmlt, OUT std::vector < std::vector <tVec> > & bds);
    // find out all the local approximate changes that does not violate error rate, and then store them in <nodeLACs>
    void CollectAllLACsUnderER(IN Simulator_t & oriSmlt, IN Simulator_t & appSmlt, IN std::vector < std::vector <tVec> > & bds, IN std::vector <Vec_Ptr_t * > & vMffcs, OUT std::vector <LAC_t> & nodeLACs);

    void Alexanderia_CollectAllLACsUnderER( IN Simulator_t & oriSmlt, IN Simulator_t & appSmlt, IN std::vector < std::vector <tVec> > & bds, IN std::vector <Vec_Ptr_t *> & vMffcs, OUT std::vector <LAC_t> & nodeLACs, OUT std::vector <LAC_t> & nodeLACsDup, IN Abc_Ntk_t * pAppNtkDup );
    // find out all the local approximate changes that does not violate mean error distance, and then store them in <nodeLACs>
    void CollectAllLACsUnderNMED(IN Simulator_t & oriSmlt, IN Simulator_t & appSmlt, IN std::vector < std::vector <tVec> > & bds, IN std::vector <Vec_Ptr_t * > & vMffcs, OUT std::vector <LAC_t> & nodeLACs);

    void Alexanderia_CollectAllLACsUnderNMED( IN Simulator_t & oriSmlt, IN Simulator_t & appSmlt, IN std::vector < std::vector <tVec> > & bds, IN std::vector <Vec_Ptr_t * > & vMffcs, OUT std::vector <LAC_t> & nodeLACs, OUT std::vector <LAC_t> & nodeLACsDup, IN Abc_Ntk_t * pAppNtkDup );
    // called by CollectAllLACsUnderER
    void CollectNodeLACUnderER(IN Abc_Obj_t * pTS, IN Simulator_t & oriSmlt, IN Simulator_t & appSmlt, IN std::vector <tVec> & isERInc, IN std::vector <tVec> & isERDec, IN std::vector <tVec> & sources, IN std::vector <Vec_Ptr_t * > & vMffcs, IN int baseER, OUT LAC_t & nodeLAC);

    void Alexanderia_CollectNodeLACUnderER(IN Abc_Obj_t * pTS, IN Simulator_t & oriSmlt, IN Simulator_t & appSmlt, IN std::vector <tVec> & isERInc, IN std::vector <tVec> & isERDec, IN std::vector <tVec> & sources, IN std::vector <Vec_Ptr_t * > & vMffcs, IN int baseER, OUT LAC_t & nodeLAC, OUT LAC_t & nodeLACDup, IN Abc_Obj_t * pTSDup, IN Abc_Ntk_t * pAppNtkDup);
    // called by CollectAllLACsUnderNMED
    void CollectNodeLACUnderNMED(IN Abc_Obj_t * pTS, IN Simulator_t & appSmlt, IN std::vector <int64_t> & oriOutputs, IN std::vector <int64_t> & appOutputs, IN tVec & bdNode, IN std::vector <tVec> & sources, IN std::vector <Vec_Ptr_t * > & vMffcs, IN int64_t baseNMED, OUT LAC_t & nodeLAC);

    void Alexanderia_CollectNodeLACUnderNMED(IN Abc_Obj_t * pTS, IN Simulator_t & appSmlt, IN std::vector <int64_t> & oriOutputs, IN std::vector <int64_t> & appOutputs, IN tVec & bdNode, IN std::vector <tVec> & sources, IN std::vector <Vec_Ptr_t * > & vMffcs, IN int64_t baseNMED, OUT LAC_t & nodeLAC, OUT LAC_t & nodeLACDup, IN Abc_Obj_t * pTSDup, IN Abc_Ntk_t * pAppNtkDup);
    // sort the LACs in <nodeLACs> according to their scores and then store the results in <candLACs>
    void SortCandLACs(IN std::vector <LAC_t> & nodeLACs, IN int nFrame, OUT std::vector <LAC_t> & candLACs);
    
    void Alexanderia_CollectAllLACsUnderMaxED( IN Simulator_t & oriSmlt, IN Simulator_t & appSmlt, IN std::vector < std::vector <tVec> > & bds, IN std::vector <Vec_Ptr_t * > & vMffcs, OUT std::vector <LAC_t> & nodeLACs, OUT std::vector <LAC_t> & nodeLACsDup, IN Abc_Ntk_t * pAppNtkDup, IN unsigned int threshold );
    
    void Alexanderia_CollectNodeLACUnderMaxED(IN Abc_Obj_t * pTS, IN Simulator_t & appSmlt, IN std::vector <int64_t> & oriOutputs, IN std::vector <int64_t> & appOutputs, IN tVec & bdNode, IN std::vector <tVec> & sources, IN std::vector <Vec_Ptr_t * > & vMffcs, IN int64_t baseNMED, OUT LAC_t & nodeLAC, OUT LAC_t & nodeLACDup, IN Abc_Obj_t * pTSDup, IN Abc_Ntk_t * pAppNtkDup, IN unsigned int threshold);

    void Alexanderia_CollectAllLACsUnderArea( IN Simulator_t & oriSmlt, IN Simulator_t & appSmlt, IN std::vector <Vec_Ptr_t * > & vMffcs, OUT std::vector <LAC_t> & nodeLACs, OUT std::vector <LAC_t> & nodeLACsDup, IN Abc_Ntk_t * pAppNtkDup );

    void Alexanderia_CollectNodeLACUnderArea(IN Abc_Obj_t * pTS, IN Simulator_t & appSmlt, IN std::vector <tVec> & sources, IN std::vector <Vec_Ptr_t * > & vMffcs, OUT LAC_t & nodeLAC, OUT LAC_t & nodeLACDup, IN Abc_Obj_t * pTSDup, IN Abc_Ntk_t * pAppNtkDup);
    
    // apply the best <topNum> candidates and then store the resulting circuit in "<outPrefix>_sth.blif"
    int ApplyBestLAC(Simulator_t & oriSmlt, Simulator_t & appSmlt, std::vector <LAC_t> & candLACs, int topNum, std::string outPrefix, unsigned seed);
    // get Delta Error Rate, with pTS the "target signal", and pSS the "substitute signal"
    void GetDER(IN Simulator_t & appSmlt, IN Abc_Obj_t * pTS, IN Abc_Obj_t * pSS, IN std::vector <tVec> & isERInc, IN std::vector <tVec> & isERDec, OUT std::pair <int, int> & errors);
    // get Delta Mean Error Distance
    void GetDNMED(IN Simulator_t & appSmlt, IN std::vector <int64_t> & oriOutputs, IN std::vector <int64_t> & appOutputs, IN std::vector <int64_t> & appOutputsNew, IN Abc_Obj_t * pTS, IN Abc_Obj_t * pSS, OUT std::pair <int64_t, int64_t> & dErrors);

    void GetDMaxED(IN Simulator_t & appSmlt, IN std::vector <int64_t> & oriOutputs, IN std::vector <int64_t> & appOutputs, IN std::vector <int64_t> & appOutputsNew, IN Abc_Obj_t * pTS, IN Abc_Obj_t * pSS, OUT std::pair <int64_t, int64_t> & dErrors);
    // get Delta Area
    double GetDArea(Abc_Obj_t * pTS, Abc_Obj_t * pSS, std::vector <Vec_Ptr_t *> & vMffcs);
    // replace <pTS> with <pSS>, by first substituting the fanouts of <pTS> with <pSS>
    void ReplaceObj(Abc_Obj_t * pTS, Abc_Obj_t * pSS);
    // called by ReplaceObj
    void DeleteObj_rec(Abc_Obj_t * pObj);
    // reorganize the BD
    void ReorganizeBD(IN std::vector < std::vector <tVec> > & bds, IN Abc_Obj_t * pObj, OUT tVec & bdNode);

    /** SATBasedMultiSelection *******************************************************
     * 
     * written by Alexanderia
     * 
     * [description]
     *      collect the available SASIMI local approximate change of all the nodes in 
     *      the network; then feed the original node together with its LAC into a Mux, 
     *      whose output replaces the fanouts of the original node. Repeat this manipulation
     *      to all the nodes in the original circuit. Then, after adding the subtractor and
     *      the comparator with the <threshold>, transform the resulting
     *      network into a cnf expression and solve it by a SAT solver. If SAT, the
     *      resulting network is stored in <outPrefix>_sth.blif. Otherwise, 
     *      output the message that no LACs are available.
     *
     *  [require]
     *      the size of <threshold> should be the same as the number of POs in <pOriNtk>
     */
    void SATBasedMultiSelection( IN Abc_Ntk_t * pOriNtk, IN std::string outPrefix, int threshold[] );

    /** CreateMuxedCNF ***************************************************************
     * 
     * written by Alexanderia
     * 
     * [description]
     *      according to the original network <pOriNtk> and the list of candidates
     *      <nodeLACs>, create the CNF encoding of the Muxed network and store
     *      the result in the file <cnfPrefix>.cnf
     */
    bool CreateMuxedCNF( IN Abc_Ntk_t * pOriNtk, IN std::vector <LAC_t> & nodeLACs, IN char * cnfFileName, int threshold[], int ** OriPIIDs, int ** MUXPIIDs, int * pNCnfVars );

    /** SatSolveApply ****************************************************************
     * 
     * written by Alexanderia
     * 
     * [description]
     *      solve the cnf expression stored in the file <cnfPrefix>.cnf using a
     *      SAT solver provided by abc package, and then apply the LACs and return 1
     *      if the solver returns a SAT assignemnt. Otherwise return 0. The approximated
     *      circuit, if SAT, is stored in the file <outPrefix>.blif.
     * 
     * [see also]
     *      AddMuxes
     */
    int SatSolveApply(IN std::string cnfPrefix, IN std::string outPrefix);
};


class LAC_t {
private:
    Abc_Obj_t * pTS;
    Abc_Obj_t * pSS;
    bool isInv;
    double dError;
    double dArea;
    double FOM;

public:
    explicit LAC_t();
    ~LAC_t();
    void Init();
    void Update(Abc_Obj_t * pTS, Abc_Obj_t * pSS, bool isInv, double error, double dArea, double FOM);
    void Print() const;
    inline Abc_Obj_t * GetTS() const {return pTS;}
    inline Abc_Obj_t * GetSS() const {return pSS;}
    inline bool GetIsInv() const {return isInv;}
    inline double GetDError() const {return dError;}
    inline double GetDArea() const {return dArea;}
    inline void SetFOM(double FOM) {this->FOM = FOM;}
    inline double GetFOM() const {return FOM;}
    bool operator < (const LAC_t & other) const;
};


extern "C" {
    void Abc_NodeMffcConeSuppPrint(Abc_Obj_t * pNode);
}


#endif
