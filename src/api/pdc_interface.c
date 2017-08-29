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

#include <stdlib.h>
#include <assert.h>
#include "pdc_interface.h"
#include "pdc_malloc.h"

/* Combine a Type number and an atom index into an atom */
#define PDCID_MAKE(g,i)   ((((pdcid_t)(g) & TYPE_MASK) << ID_BITS) | ((pdcid_t)(i) & ID_MASK))

struct PDC_id_info *PDC_find_id(pdcid_t idid)
{
    PDC_type_t      type;               /*ID's type         */
    struct PDC_id_type   *type_ptr;          /*ptr to the type   */
    struct PDC_id_info   *ret_value = NULL;  /* Return value     */

    FUNC_ENTER(NULL);

    /* Check arguments */
    type = PDC_TYPE(idid);
    if(type <= PDC_BADID || type >= PDC_next_type)
        PGOTO_DONE(NULL);

    type_ptr = (pdc_id_list_g->PDC_id_type_list_g)[type];
    if(!type_ptr || type_ptr->init_count <= 0)
        PGOTO_DONE(NULL);
    
    /* Locate the ID node for the ID */
    PDC_LIST_SEARCH(ret_value, &type_ptr->ids, entry, id, idid);
    
done:
    FUNC_LEAVE(ret_value);
} /* end PDC_find_id() */


perr_t PDC_register_type(PDC_type_t type_id, PDC_free_t free_func)
{
    struct PDC_id_type *type_ptr = NULL;     /* Ptr to the atomic type */
    perr_t ret_value = SUCCEED;              /* Return value           */
    
    FUNC_ENTER(NULL);

    /* Sanity check */
    assert(type_id > 0 && type_id < PDC_MAX_NUM_TYPES);

    /* Initialize the type */
    if(NULL == (pdc_id_list_g->PDC_id_type_list_g)[type_id]) {
        /* Allocate the type information for new type */
        if(NULL == (type_ptr = (struct PDC_id_type *)PDC_CALLOC(struct PDC_id_type)))
            PGOTO_ERROR(FAIL, "ID type allocation failed");
        (pdc_id_list_g->PDC_id_type_list_g)[type_id] = type_ptr;
    }
    else {
        /* Get the pointer to the existing type */
        type_ptr = (pdc_id_list_g->PDC_id_type_list_g)[type_id];
    }
    /* Initialize the ID type structure for new types */
    if(type_ptr->init_count == 0) {
        type_ptr->type_id = type_id;
        type_ptr->free_func = free_func;
        type_ptr->id_count = 0;
        type_ptr->nextid = 0;
        PDC_LIST_INIT(&type_ptr->ids);
    }
    /* Increment the count of the times this type has been initialized */
    type_ptr->init_count++;

done:
    FUNC_LEAVE(ret_value);
} /* end PDC_register_type() */

/*
pdcid_t PDCid_register(PDC_type_t type, const void *object) {
    pdcid_t ret_value = PDC_INVALID_ID;  

    FUNC_ENTER(NULL);

    if(!PDCID_IS_LIB_TYPE(type))
        PGOTO_ERROR(FAIL, "cannot call public function on library type");
    ret_value = PDC_id_register(type, object);
done:
    FUNC_LEAVE(ret_value);
}*/ /* end PDCid_register() */


pdcid_t PDC_id_register(PDC_type_t type, const void *object)
{
    struct PDC_id_type   *type_ptr;           /* ptr to the type               */
    struct PDC_id_info   *id_ptr;             /* ptr to the new ID information */
    pdcid_t              new_id;              /* new ID                        */
    pdcid_t              ret_value = SUCCEED; /* return value                  */
    FUNC_ENTER(NULL);

    /* Check arguments */
    if(type <= PDC_BADID || type >= PDC_next_type)
        PGOTO_ERROR(FAIL, "invalid type number");
    type_ptr = (pdc_id_list_g->PDC_id_type_list_g)[type];
    if(NULL == type_ptr || type_ptr->init_count <= 0)
        PGOTO_ERROR(FAIL, "invalid type");
    if(NULL == (id_ptr = PDC_MALLOC(struct PDC_id_info)))
        PGOTO_ERROR(FAIL, "memory allocation failed");

    /* Create the struct & it's ID */
    PDC_MUTEX_LOCK(type_ptr->ids);
    new_id = PDCID_MAKE(type, type_ptr->nextid);
    id_ptr->id = new_id;
    id_ptr->count = ATOMIC_VAR_INIT(1);      /*initial reference count*/
    id_ptr->obj_ptr = object;

    /* Insert into the type */
    PDC_LIST_INSERT_HEAD(&type_ptr->ids, id_ptr, entry);   
    type_ptr->id_count++;
    type_ptr->nextid++;
    PDC_MUTEX_UNLOCK(type_ptr->ids);

    /* Sanity check for the 'nextid' getting too large and wrapping around. */
    assert(type_ptr->nextid <= ID_MASK);

    /* Set return value */
    ret_value = new_id;
    
done:
    FUNC_LEAVE(ret_value);
} /* end PDC_id_register() */

int PDC_dec_ref(pdcid_t id)
{
    int ret_value = 0;               /* Return value */
    struct PDC_id_info *id_ptr;      /* Pointer to the new ID */
    
    FUNC_ENTER(NULL);

    /* Sanity check */
    assert(id >= 0);

    /* General lookup of the ID */
    if(NULL == (id_ptr = PDC_find_id(id)))
        PGOTO_ERROR(FAIL, "can't locate ID");

//    (id_ptr->count)--;
//    if(id_ptr->count == 0) {
    ret_value = atomic_fetch_sub(&(id_ptr->count), 1) - 1;
    if(ret_value == 0) {
        struct PDC_id_type   *type_ptr;      /*ptr to the type   */
        
        /* Get the ID's type */
        type_ptr = (pdc_id_list_g->PDC_id_type_list_g)[PDC_TYPE(id)];
        if(!type_ptr->free_func || (type_ptr->free_func)((void *)id_ptr->obj_ptr) >= 0) {
            /* check if list is empty before remove */
            if(PDC_LIST_IS_EMPTY(&type_ptr->ids))
                PGOTO_ERROR(FAIL, "can't remove ID node");

            PDC_MUTEX_LOCK(type_ptr->ids);
            /* Remove the node from the type */
            PDC_LIST_REMOVE(id_ptr, entry);
	    id_ptr = PDC_FREE(struct PDC_id_info, id_ptr);
            /* Decrement the number of IDs in the type */
            (type_ptr->id_count)--;
            ret_value = 0;
            PDC_MUTEX_UNLOCK(type_ptr->ids);
        }
        else
            ret_value = FAIL;
    }
//    else
//        ret_value = (int)id_ptr->count;
done:
    FUNC_LEAVE(ret_value);
} /* end PDC_dec_ref() */

pdcid_t PDC_find_byname(PDC_type_t type, const char *byname)
{
    pdcid_t              ret_value = SUCCEED; /* return value          */
    struct PDC_id_info   *id_ptr = NULL;      /* Pointer to the ID     */
    struct PDC_id_type   *type_ptr;           /* Pointer to the type   */
    
    FUNC_ENTER(NULL);

    if(type <= PDC_BADID || type >= PDC_next_type)
        PGOTO_ERROR(FAIL, "invalid type number");

    type_ptr = (pdc_id_list_g->PDC_id_type_list_g)[type];

    /* Locate the ID node for the ID */
    PDC_LIST_SEARCH_CONT_NAME(id_ptr, &type_ptr->ids, entry, obj_ptr, name, byname);
    if(id_ptr == NULL)
        PGOTO_ERROR(FAIL, "cannot find the name");
    ret_value = id_ptr->id;
    
done:
    FUNC_LEAVE(ret_value);
} /* end of PDC__find_byname() */

int pdc_inc_ref(pdcid_t id)
{
    int ret_value = 0;               /* Return value */
    struct PDC_id_info *id_ptr;      /* Pointer to the ID */
    
    FUNC_ENTER(NULL);

    /* Sanity check */
    assert(id >= 0);

    /* General lookup of the ID */
    if(NULL == (id_ptr = PDC_find_id(id)))
        PGOTO_ERROR(FAIL, "can't locate ID");

    /* Set return value */
    ret_value = atomic_fetch_add(&(id_ptr->count), 1) + 1;
    
done:
    FUNC_LEAVE(ret_value);
} /* end of pdc_inc_ref() */

int PDC_id_list_null(PDC_type_t type)
{
    perr_t ret_value = 0;              /* Return value */
    struct PDC_id_type   *type_ptr;    /* Pointer to the type   */
    
    FUNC_ENTER(NULL);

    if(type <= PDC_BADID || type >= PDC_next_type)
        PGOTO_ERROR(FAIL, "invalid type number");

    type_ptr = (pdc_id_list_g->PDC_id_type_list_g)[type];
    if(type_ptr->id_count != 0)
        ret_value = type_ptr->id_count;
    
done:
    FUNC_LEAVE(ret_value);
}

perr_t PDC_id_list_clear(PDC_type_t type)
{
    perr_t ret_value = SUCCEED;        /* Return value */
    struct PDC_id_type   *type_ptr;    /* Pointer to the type   */
    
    FUNC_ENTER(NULL);

    type_ptr = (pdc_id_list_g->PDC_id_type_list_g)[type];

    while(!PDC_LIST_IS_EMPTY(&type_ptr->ids)) {
        struct PDC_id_info *id_ptr = (&type_ptr->ids)->head;
        if(!type_ptr->free_func || (type_ptr->free_func)((void *)id_ptr->obj_ptr) >= 0) {
            PDC_MUTEX_LOCK(type_ptr->ids);
            PDC_LIST_REMOVE(id_ptr, entry);
            id_ptr = PDC_FREE(struct PDC_id_info, id_ptr);
            (type_ptr->id_count)--;
            PDC_MUTEX_UNLOCK(type_ptr->ids);
        }
        else
            ret_value = FAIL;
    }

    FUNC_LEAVE(ret_value);
}


perr_t PDC_destroy_type(PDC_type_t type)
{
    perr_t ret_value = SUCCEED;              /* Return value           */
    struct PDC_id_type *type_ptr = NULL;     /* Ptr to the atomic type */
    
    FUNC_ENTER(NULL);

    type_ptr = (pdc_id_list_g->PDC_id_type_list_g)[type];
    if(type_ptr == NULL)
        PGOTO_ERROR(FAIL, "type was not initialized correctly");
    type_ptr = PDC_FREE(struct PDC_id_type, type_ptr);
    
done:
    FUNC_LEAVE(ret_value);
}