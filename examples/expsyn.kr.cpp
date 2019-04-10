

/** all global variables */
struct ExpSyn_Store {
    int point_type;
    double g0;
    int reset;
    int mech_type;
    int* slist1;
    int* dlist1;
    ThreadDatum* __restrict__ ext_call_thread;
};

/** holds object of global variable */
ExpSyn_Store ExpSyn_global;


/** all mechanism instance variables */
const double __restrict__ tau[N];
const double __restrict__ e[N];
double __restrict__ i[N];
double __restrict__ g[N];
double __restrict__ Dg[N];
double __restrict__ v_unused[N];
double __restrict__ g_unused[N];
double __restrict__ tsave[N];



static inline double nrn_current(int id, int pnodecount, ExpSyn_Instance* inst, double* data, const Datum* indexes, ThreadDatum* thread, NrnThread* nt, double v) {
    double current = 0.0;
    i[id] = g[id] * (v - e[id]);
    current += i[id];
    return current;
}


/** update current */
void nrn_cur_ExpSyn(NrnThread* nt, Memb_list* ml, int type) {
    int start = 0;
    int end = nodecount;
    int id
    double v
    #pragma ivdep
    for (id = start; id < end; id++)  {
        int node_id = node_index[id];
        double v = voltage[node_id];
        double g = nrn_current(id, pnodecount, inst, data, indexes, thread, nt, v+0.001);
        double rhs = nrn_current(id, pnodecount, inst, data, indexes, thread, nt, v);
        g = (g-rhs)/0.001;
        double mfactor = 1.e2/inst->node_area[indexes[0*pnodecount+id]];
        g = g*mfactor;
        rhs = rhs*mfactor;
        shadow_rhs[id] = rhs;
        shadow_d[id] = g;
    }
    for (int id = start; id < end; id++)  {
        int node_id = node_index[id];
        vec_rhs[node_id] -= shadow_rhs[id];
        vec_d[node_id] += shadow_d[id];
    }
}


/** update state */
int start = 0;
int end = nodecount;
int id
double v
#pragma ivdep
for (id = start; id < end; id++)  {
    /*[INFO] elision of 1 indirect access: */
    /*[INFO] node_id = node_index[id]; */
    /*[INFO] v = voltage[node_id]; */
    v = voltage[id];
