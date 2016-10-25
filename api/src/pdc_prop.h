#ifndef _pdc_prop_H
#define _pdc_prop_H

#include "pdc_error.h"
#include "pdc_interface.h"
#include "pdc_prop_pkg.h"


/* PDC container and object property initialization
 * Return: Non-negative on success/Negative on failure
 */
perr_t PDCprop_init(PDC_CLASS_t *pc);

/* Create PDC property 
 * Param type [IN]: PDC property creation type (enum type), PDC_CONT_CREATE or PDC_OBJ_CREATE
 * Return: PDC property id, 0 for container and 1 for object
 */
pdcid_t PDCprop_create(PDC_prop_type type, pdcid_t pdc);

/* Close PDC property
 * Param prop_id [IN]: Id of the PDC property
 * Return: Non-negative on success/Negative on failure
 */
perr_t PDCprop_close(pdcid_t id, pdcid_t pdc);

/* PDC container and object property finalize
 * Return: Non-negative on success/Negative on failure
 */
perr_t PDCprop_end();

/* private functions */

/* Check if object property list is empty
 * Return: SUCCEED if empty, or FAIL if not empty
 */
perr_t PDC_prop_obj_list_null();

/* Check if container property list is empty
 * Return: SUCCEED if empty, or FAIL if not empty
 */
perr_t PDC_prop_cont_list_null();

#endif
