/*
 * Copyright Notice for 
 * Proactive Data Containers (PDC) Software Library and Utilities
 * -----------------------------------------------------------------------------

 *** Copyright Notice ***
 
 * Proactive Data Containers (PDC) Copyright (c) 2017, The Regents of the
 * University of California, through Lawrence Berkeley National Laboratory,
 * UChicago Argonne, LLC, operator of Argonne National Laboratory, and The HDF
 * Group (subject to receipt of any required approvals from the U.S. Dept. of
 * Energy).  All rights reserved.
 
 * If you have questions about your rights to use or distribute this software,
 * please contact Berkeley Lab's Innovation & Partnerships Office at  IPO@lbl.gov.
 
 * NOTICE.  This Software was developed under funding from the U.S. Department of
 * Energy and the U.S. Government consequently retains certain rights. As such, the
 * U.S. Government has been granted for itself and others acting on its behalf a
 * paid-up, nonexclusive, irrevocable, worldwide license in the Software to
 * reproduce, distribute copies to the public, prepare derivative works, and
 * perform publicly and display publicly, and to permit other to do so.
 */

#ifndef _pdc_H
#define _pdc_H

#include <stdint.h>
#include <stdbool.h>
#include "pdc_public.h"
#include "pdc_error.h"
#include "pdc_malloc.h"
#include "pdc_pkg.h"
#include "pdc_prop.h"
#include "pdc_prop_pkg.h"
#include "pdc_cont.h"
#include "pdc_cont_pkg.h"

#ifdef ENABLE_MPI
#include "pdc_mpi.h"
#endif

#include "pdc_obj.h"
#include "pdc_obj_pkg.h"
#include "pdc_linkedlist.h"
#include "pdc_client_public.h"
#include "pdc_analysis_support.h"
#include "pdc_transform_support.h"
#include "pdc_query.h"

/**
 * Initialize the PDC layer
 *
 * \param pdc_name [IN]         Name of the PDC
 *
 * \return PDC id on success/Zero on failure
 */
pdcid_t PDC_init(const char *pdc_name);

/**
 * Close the PDC layer
 *
 * \param pdc_id [IN]          ID of the PDC
 *
 * \return Non-negative on success/Negative on failure
 */
perr_t PDC_close(pdcid_t pdcid);

#endif 
