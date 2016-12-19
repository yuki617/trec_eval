/* 
   Written by Ian Soboroff, NIST, 2016.
   As a government work this file is in the public domain.
*/

#include "common.h"
#include "sysfunc.h"
#include "trec_eval.h"
#include "functions.h"
#include "trec_format.h"

static int
te_calc_rbp (const EPI *epi, const REL_INFO *rel_info,
	     const RESULTS *results, const TREC_MEAS *tm, TREC_EVAL *eval);
static PARAMS default_rbp_params = {NULL, 0, NULL};

/* See trec_eval.h for definition of TREC_MEAS */
TREC_MEAS te_meas_rbp =  {"rbp",
     "    Rank-Biased Precision\n\
    This description needs updating (it's the text for AP)\n\
    Precision measured after each relevant doc is retrieved, then averaged\n\
    for the topic, and then averaged over topics (if more than one).\n\
    This is the main single-valued number used to compare the entire rankings\n\
    of two or more retrieval methods.  It has proven in practice to be useful\n\
    and robust.\n\
    The name of the measure is unfortunately inaccurate since it is \n\
    calculated for a single topic (and thus don't want both 'mean' and\n\
    'average') but was dictated by common usage and the need to distiguish\n\
    map from Precision averaged over topics (I had to give up my attempts to\n\
    call it something else!)\n\
    History: Developed by Chris Buckley after TREC 1.\n\
    Cite: Moffat, Alistair and Justin Zobel, \"Rank-Biased Precision for \n\
    Measurement of Retrieval Effectiveness.\"  ACM Transactions on\n\
    Information Systems, vol. 27, no. 1, article 2, publication date\n\
    December, 2008.  http://doi.acm.org/10.1145/1416950.1416952\n",
     te_init_meas_s_float_p_pair,
     te_calc_rbp,
     te_acc_meas_s,
     te_calc_avg_meas_s,
     te_print_single_meas_s_float,
     te_print_final_meas_s_float_p,
     &default_rbp_params, -1};

static int 
te_calc_rbp (const EPI *epi, const REL_INFO *rel_info, const RESULTS *results,
	     const TREC_MEAS *tm, TREC_EVAL *eval)
{
    RES_RELS res_rels;
    double sum, min, max;
    double p = 0.9; /* default value, settable with param */
    double cur_p;
    double gain;
    long i;
    FLOAT_PARAM_PAIR *pairs;
    GAINS gains;
    int num_pairs;

    if (UNDEF == te_form_res_rels (epi, rel_info, results, &res_rels))
	return (UNDEF);

    if (tm->meas_params) {
	pairs = (FLOAT_PARAM_PAIR *) tm->meas_params->param_values;
	num_pairs = tm->meas_params->num_params;
	for (i = 0; i < num_pairs; i++) {
	    if (0 == strcmp(pairs[i].name, "p"))
		p = (double) pairs[i].value;
	}
    }

    if (UNDEF == setup_gains (tm, &res_rels, &gains))
	return UNDEF;

    /* normalize gains to [0,1], per email conversation with Alistair Moffat */
    /* Note we don't care whether there is a relevance value with gain 1, only
       that the gains all lie between 0 and 1 inclusive. */
    for (i = 0; i < gains.num_gains; i++) {
	if (gains.rel_gains[i].gain < min)
	    min = gains.rel_gains[i].gain;
	if (gains.rel_gains[i].gain > max)
	    max = gains.rel_gains[i].gain;
    }
    if (min < 0 || max > 1) {
	for (i = 0; i < gains.num_gains; i++) {
	    gains.rel_gains[i].gain = (gains.rel_gains[i].gain - min) / (max - min);
	}
    }
    
    sum = 0.0;
    cur_p = 1.0;  /* p^0 */
    for (i = 0; i < res_rels.num_ret; i++) {
	gain = get_gain(res_rels.results_rel_list[i], &gains);
	if (gain != 0) {
	    sum += (double) gain * cur_p;
	    if (epi->debug_level > 0) 
		printf("rbp:%ld %3.1f %6.4f\n", i, gain, sum);

	}
        cur_p = cur_p * p;
    }

    eval->values[tm->eval_index].value = (1.0 - p) * sum;
    return (1);
}
