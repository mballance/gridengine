/*___INFO__MARK_BEGIN__*/
/*************************************************************************
 * 
 *  The Contents of this file are made available subject to the terms of
 *  the Sun Industry Standards Source License Version 1.2
 * 
 *  Sun Microsystems Inc., March, 2001
 * 
 * 
 *  Sun Industry Standards Source License Version 1.2
 *  =================================================
 *  The contents of this file are subject to the Sun Industry Standards
 *  Source License Version 1.2 (the "License"); You may not use this file
 *  except in compliance with the License. You may obtain a copy of the
 *  License at http://gridengine.sunsource.net/Gridengine_SISSL_license.html
 * 
 *  Software provided under this License is provided on an "AS IS" basis,
 *  WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING,
 *  WITHOUT LIMITATION, WARRANTIES THAT THE SOFTWARE IS FREE OF DEFECTS,
 *  MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE, OR NON-INFRINGING.
 *  See the License for the specific provisions governing your rights and
 *  obligations concerning the Software.
 * 
 *   The Initial Developer of the Original Code is: Sun Microsystems, Inc.
 * 
 *   Copyright: 2001 by Sun Microsystems, Inc.
 * 
 *   All Rights Reserved.
 * 
 ************************************************************************/
/*___INFO__MARK_END__*/

#include <stdio.h>
#include <string.h>

#include "sgermon.h"
#include "symbols.h"
#include "sge.h"
#include "sge_time.h"
#include "sge_log.h"
#include "cull.h"
#include "sge_var.h"
#include "sge_prog.h"
#include "sge_stdlib.h"
#include "sge_string.h"
#include "sge_centry.h"

/****** sgeobj/var/-VariableList **********************************************
*  NAME
*     VariableList - Object to store variable name/value pairs
*
*  FUNCTION
*     In several places within SGE/EE it is necessary to store
*     variables and their values (e.g. job environment variable,
*     job context, ...) of following form: 
*  
*        variable[=value][,variable[=value],...]
*
*     The VA_Type CULL element is used to hold such data.
*     Funktions in the SEE ALSO section might be used to
*     work with VA_Type lists and elements.
*
*  SEE ALSO
*     sgeobj/var/--VA_Type
*     clients/var/var_list_parse_from_string()
*     sgeobj/var/var_list_dump_to_file()
*     sgeobj/var/var_list_copy_complex_vars_and_value()
*     sgeobj/var/var_list_copy_prefix_vars()
*     sgeobj/var/var_list_get_string()
*     sgeobj/var/var_list_set_string()
*     sgeobj/var/var_list_set_int()
*     sgeobj/var/var_list_set_sge_u32()
*     sgeobj/var/var_list_set_sharedlib_path()
*     sgeobj/var/var_list_remove_prefix_vars()
******************************************************************************/

/****** sgeobj/var/var_get_sharedlib_path_name() ******************************
*  NAME
*     var_get_sharedlib_path_name -- name of sharedlib path variable
*
*  SYNOPSIS
*     static const char *var_get_sharedlib_path_name(void);
*
*  FUNCTION
*     Returns the operating dependent name of the shared library path
*     (e.g. LIBPATH, SHLIB_PATH, LD_LIBRARY_PATH).
*     If the name of the sharedlib path is not known for an operating
*     system (the port has not yet been done), a compile time error
*     is raised.
*
*  RESULT
*     Name of the shared lib path variable
*
*  NOTES
*     Raising a compile time error (instead of e.g. just returning NULL
*     or LD_LIBRARY_PATH as default) has the following reason:
*     Setting the shared lib path is a very sensible operation 
*     concerning security.
*     Example: If a shared linked rshd (called for qrsh execution) is
*     executed with a faked shared lib path, operations defined in
*     a non sge library libgdi.so might be executed as user root.
******************************************************************************/
const char *var_get_sharedlib_path_name(void)
{
#if defined(AIX)
   return "LIBPATH";
#elif defined(HPUX)
   return "SHLIB_PATH";
#elif defined(ALPHA) || defined(IRIX) || defined(LINUX) || defined(SOLARIS) || defined(DARWIN) || defined(FREEBSD) || defined(NETBSD) || defined(INTERIX)
   return "LD_LIBRARY_PATH";
#elif defined(DARWIN)
   return "DYLD_LIBRARY_PATH";
#else
#error "don't know how to set shared lib path on this architecture"
   return NULL; /* never reached */
#endif
}

/****** sgeobj/var/var_list_set_string() **************************************
*  NAME
*     var_list_set_string -- add/change an variable
*
*  SYNOPSIS
*     void var_list_set_string(lList **varl, 
*                              const char *name, 
*                              const char *value);
*
*  FUNCTION
*     If the variable <name> does not already exist in <varl>, 
*     it is created and initialized with <value>.
*     Otherwise, its value is overwritten with <value>
*
*  INPUTS
*     lList **varl      - VA_Type list
*     const char *name  - the name of the variable
*     const char *value - the (new) value of the variable
*
*  SEE ALSO
*     sgeobj/var/var_list_set_int()
*     sgeobj/var/var_list_set_sge_u32() 
*     sgeobj/var/var_list_set_sharedlib_path()
******************************************************************************/
void var_list_set_string(lList **varl, const char *name, const char *value) 
{
   lListElem *elem;

   DENTER(TOP_LAYER, "var_list_set_string");
   if (varl == NULL || name == NULL || value == NULL) {
      DEXIT;
      return;
   }
   elem = lGetElemStr(*varl, VA_variable, name);
   if (elem == NULL) {
      elem = lAddElemStr(varl, VA_variable, name, VA_Type);
   }
   lSetString(elem, VA_value, value);
   DEXIT;
}

/****** sgeobj/var/var_list_set_int() *****************************************
*  NAME
*     var_list_set_int -- add/change an variable
*
*  SYNOPSIS
*     void var_list_set_int(lList *varl, 
*                           const char *name, 
*                           int value);
*
*  FUNCTION
*     If the variable <name> does not already exist in <varl>, 
*     it is created and initialized with <value>.
*     Otherwise, its value is overwritten with <value>
*
*  INPUTS
*     varl  - VA_Type list
*     name  - the name of the variable
*     value - the (new) value of the variable
*
*  SEE ALSO
*     sgeobj/var/var_list_set_string()
*     sgeobj/var/var_list_set_sge_u32() 
*     sgeobj/var/var_list_set_sharedlib_path()
******************************************************************************/
void var_list_set_int(lList **varl, const char *name, int value)
{
   char buffer[2048];

   DENTER(TOP_LAYER, "var_list_set_int");
   sprintf(buffer, "%d", value);
   var_list_set_string(varl, name, buffer);
   DEXIT;
}

/****** sgeobj/var/var_list_set_sge_u32() *****************************************
*  NAME
*     var_list_set_sge_u32 -- add/change a variable
*
*  SYNOPSIS
*     void var_list_set_sge_u32(lList **varl, 
*                           const char *name, 
*                           u_long32 value);
*
*  FUNCTION
*     If the variable <name> does not already exist in <varl>, 
*     it is created and initialized with <value>.
*     Otherwise, its value is overwritten with <value>
*
*  INPUTS
*     lList **varl      - VA_Type list
*     const char *name  - the name of the variable
*     u_long32 value    - the (new) value of the variable
*
*  SEE ALSO
*     sgeobj/var/var_list_set_string()
*     sgeobj/var/var_list_set_int()
*     sgeobj/var/var_list_set_sharedlib_path()
******************************************************************************/
void var_list_set_sge_u32(lList **varl, const char *name, u_long32 value)
{
   char buffer[2048];

   DENTER(TOP_LAYER, "var_list_set_sge_u32");
   sprintf(buffer, sge_u32, value);
   var_list_set_string(varl, name, buffer);
   DEXIT;
}

/****** sgeobj/var/var_list_set_sharedlib_path() ******************************
*  NAME
*     var_list_set_sharedlib_path -- set shared lib path
*
*  SYNOPSIS
*     void var_list_set_sharedlib_path(lList **varl);
*
*  FUNCTION
*     Sets or replaces the shared lib path in the list of variables.
*     The SGE shared lib path is always set to the beginning of the
*     resulting shared lib path 
*     (security, see var_get_sharedlib_path_name())
*
*  INPUTS
*     lList **varl - list of nment variables
*
*  SEE ALSO
*     sgeobj/var/var_get_sharedlib_path_name()
*     sgeobj/var/var_list_set_string()
*     sgeobj/var/var_list_set_int()
*     sgeobj/var/var_list_set_sge_u32() 
******************************************************************************/
void var_list_set_sharedlib_path(lList **varl)
{
   char *sharedlib_path;
   char *sge_sharedlib_path;
   const char *sge_root = sge_get_root_dir(0, NULL, 0, 1);
   const char *sharedlib_path_name = var_get_sharedlib_path_name();
   lListElem *sharedlib_elem = NULL;

   DENTER(TOP_LAYER, "set_sharedlib_path");

   /* this is the SGE sharedlib path */
   sge_sharedlib_path = sge_malloc(strlen(sge_root) + 
                        strlen("/lib/") + strlen(sge_get_arch()) + 1);
   sprintf(sge_sharedlib_path, "%s/lib/%s", sge_root, sge_get_arch());

   /* if already in environment: extend by SGE sharedlib path, else set */
   sharedlib_elem = lGetElemStr(*varl, VA_variable, sharedlib_path_name);
   if(sharedlib_elem != NULL) {
      const char *old_value = lGetString(sharedlib_elem, VA_value);

      if(old_value && strlen(old_value) > 0) {
         DPRINTF(("sharedlib path %s already set:\n", sharedlib_path_name));
         
         sharedlib_path = sge_malloc(strlen(old_value) + 1 + 
                          strlen(sge_sharedlib_path) + 1);
         strcpy(sharedlib_path, sge_sharedlib_path);
         strcat(sharedlib_path, ":");
         strcat(sharedlib_path, old_value);
         lSetString(sharedlib_elem, VA_value, sharedlib_path);
         sharedlib_path = sge_free(sharedlib_path);
      } else {
         DPRINTF(("overwriting empty sharedlib path %s\n", 
                  sharedlib_path_name));
         lSetString(sharedlib_elem, VA_value, sge_sharedlib_path);
      }
   } else {
      DPRINTF(("creating new sharedlib path %s\n", sharedlib_path_name));
      sharedlib_elem = lAddElemStr(varl, VA_variable, 
                                   sharedlib_path_name, VA_Type);
      lSetString(sharedlib_elem, VA_value, sge_sharedlib_path);
   }

   sge_sharedlib_path = sge_free(sge_sharedlib_path);
   DEXIT;
}

/****** sgeobj/var/var_list_dump_to_file() ************************************
*  NAME
*     var_list_dump_to_file -- dump variables from list to file
*
*  SYNOPSIS
*     void var_list_dump_to_file(const lList *varl, FILE *file);
*
*  FUNCTION
*     Parses the list of type VA_Type <varl> containing the
*     description of variables.
*     Dumps all list elements to <file> in the format:
*     <name>=<value>
*     <name> is read from VA_variable, <value> from VA_value.
*
*  INPUTS
*     varl - list of variables
*     file - filehandle of a previously opened (for writing) file
******************************************************************************/
void var_list_dump_to_file(const lList *varl, FILE *file)
{
   lListElem *elem;

   if(varl == NULL || file == NULL) {
      return;
   }

   for_each(elem, varl) {
      fprintf(file, "%s=%s\n", lGetString(elem, VA_variable), 
              lGetString(elem, VA_value));
   }
}

/****** sgeobj/var/var_list_get_string() **************************************
*  NAME
*     var_list_get_string() -- get value of certain variable 
*
*  SYNOPSIS
*     const char* var_list_get_string(lList *varl, 
*                                     const char *variable) 
*
*  FUNCTION
*     Return the string value of a variable
*     with the name "variable" which is stored in "varl". 
*
*  INPUTS
*     lList *varl          - VA_Type list 
*     const char *variable - variable name 
*
*  RESULT
*     const char* - value or NULL 
*
*  SEE ALSO
*     sgeobj/var/var_list_set_string()
*     sgeobj/var/var_list_set_int()
*     sgeobj/var/var_list_set_sge_u32() 
*     sgeobj/var/var_list_set_sharedlib_path()
******************************************************************************/
const char* var_list_get_string(lList *varl, const char *variable)
{
   lListElem *var = NULL;     /* VA_Type */
   const char *ret = NULL;

   var = lGetElemStr(varl, VA_variable, variable);
   if (var != NULL) {
      ret = lGetString(var, VA_value);
   }
   return ret;
}

/****** sgeobj/var/var_list_copy_prefix_vars() ********************************
*  NAME
*     var_list_copy_prefix_vars() -- copy vars with certain prefix 
*
*  SYNOPSIS
*     void var_list_copy_prefix_vars(lList **varl, 
*                                    const lList *src_varl,
*                                    const char *prefix, 
*                                    const char *new_prefix) 
*
*  FUNCTION
*     Make a copy of all entries in "src_varl" 
*     beginning with "prefix". "prefix" is replaced by "new_prefix"
*     for all created elements. The new elements will be added to 
*     "varl".
*
*  INPUTS
*     lList **varl           - VA_Type list 
*     const char *prefix     - prefix string (e.g. VAR_PREFIX) 
*     const char *new_prefix - new prefix string (e.g. "SGE_") 
*
*  EXAMPLE
*     "__SGE_PREFIX__O_HOME" ===> "SGE_O_HOME 
*
*  SEE ALSO
*     sgeobj/var/var_list_remove_prefix_vars()
******************************************************************************/
void var_list_copy_prefix_vars(lList **varl, 
                               const lList *src_varl,
                               const char *prefix, 
                               const char *new_prefix)
{
   int prefix_len = strlen(prefix);
   lListElem *var_elem = NULL;
   lList *var_list2 = NULL;

   DENTER(TOP_LAYER, "var_list_copy_prefix_vars");
   for_each(var_elem, src_varl) {
      const char *prefix_name = lGetString(var_elem, VA_variable);
      const char *value = lGetString(var_elem, VA_value);
      char *name_without_prefix = (char*)(prefix_name + prefix_len);

      if (!strncmp(prefix_name, prefix, prefix_len)) {
         char name[MAX_STRING_SIZE];

         prefix_name += prefix_len;
         sprintf(name, "%s%s", new_prefix, name_without_prefix);
         var_list_set_string(&var_list2, name, value);
      }
   }
   if (*varl == NULL) {
      *varl = lCreateList("", VA_Type);
   }
   var_list_add_as_set (*varl, var_list2); 
   DEXIT;
}

/****** sgeobj/var/var_list_copy_prefix_vars_undef() **************************
*  NAME
*     var_list_copy_prefix_vars_undef() -- copy vars with certain prefix 
*
*  SYNOPSIS
*     void 
*     var_list_copy_prefix_vars_undef(lList **varl, 
*                                     const lList *src_varl,
*                                     const char *prefix, 
*                                     const char *new_prefix) 
*
*  FUNCTION
*     Make a copy of all entries in "src_varl" 
*     beginning with "prefix". "prefix" is replaced by "new_prefix"
*     for all created elements. The new elements will be added to 
*     "varl" if it is undefined in "varl".
*
*  INPUTS
*     lList **varl           - VA_Type list 
*     const char *prefix     - prefix string (e.g. VAR_PREFIX_NR) 
*     const char *new_prefix - new prefix string (e.g. "SGE_") 
*
*  EXAMPLE
*     "__SGE_PREFIX2__TASK_ID" ===> "SGE_TASK_ID 
*
*  SEE ALSO
*     sgeobj/var/var_list_remove_prefix_vars()
******************************************************************************/
void var_list_copy_prefix_vars_undef(lList **varl, 
                                     const lList *src_varl,
                                     const char *prefix, 
                                     const char *new_prefix)
{
   int prefix_len = strlen(prefix);
   lListElem *var_elem = NULL;
   lList *var_list2 = NULL;

   DENTER(TOP_LAYER, "var_list_copy_prefix_vars");
   for_each(var_elem, src_varl) {
      const char *prefix_name = lGetString(var_elem, VA_variable);
      const char *value = lGetString(var_elem, VA_value);
      char *name_without_prefix = (char*)(prefix_name + prefix_len);

      if (!strncmp(prefix_name, prefix, prefix_len)) {
         char name[MAX_STRING_SIZE];
         lListElem *existing_variable;

         prefix_name += prefix_len;
         sprintf(name, "%s%s", new_prefix, name_without_prefix);
         existing_variable = lGetElemStr(*varl, VA_variable, name);
         if (existing_variable == NULL) {
            var_list_set_string(&var_list2, name, value);
         }
      }
   }
   if (*varl == NULL) {
      *varl = lCreateList("", VA_Type);
   }
   lAddList(*varl, &var_list2);
   DEXIT;
}

/****** sgeobj/var/var_list_copy_complex_vars_and_value() *********************
*  NAME
*     var_list_copy_complex_vars_and_value() -- copy certain vars 
*
*  SYNOPSIS
*     void 
*     var_list_copy_complex_vars_and_value(lList **varl, 
*                                          const lList *src_varl, 
*                                          const lList *centry_list) 
*
*  FUNCTION
*     Copy all variables from "src_varl" into
*     "varl" whose names begin with the define VAR_COMPLEX_PREFIX
*     If the tail of the name of a variable is equivalent
*     with a complex entry name then replace the value of the
*     corresponding variable with the complex value. 
*
*     SGE_COMPLEX_hostname="" ==> SGE_COMPLEX_hostname="fangorn.sun.com" 
*
*  INPUTS
*     lList **varl             - VA_Type list 
*     const lList *src_varl    - source VA_Type list 
*     const lList *centry_list - complex entry list 
******************************************************************************/
void var_list_copy_complex_vars_and_value(lList **varl,
                                          const lList* src_varl,
                                          const lList* centry_list)
{
   lListElem *var = NULL;
   int n = strlen(VAR_COMPLEX_PREFIX);
   DENTER(TOP_LAYER, "var_list_copy_complex_vars_and_value");

   for_each(var, src_varl) {
      const char *name = lGetString(var, VA_variable);

      if (!strncmp(name, VAR_COMPLEX_PREFIX, n)) {
         const lListElem* attr = lGetElemStr(centry_list, CE_name, &name[n]);

         if (attr != NULL) {
            const char *value = lGetString(attr, CE_stringval);
   
            if (value != NULL) {
               var_list_set_string(varl, name, value);
            } else {
               var_list_set_string(varl, name, "");
            }
         } else {
            var_list_set_string(varl, name, "");
         }
      }
   }
   DEXIT;
}

/****** sgeobj/var/var_list_copy_env_vars_and_value() *************************
*  NAME
*     var_list_copy_env_vars_and_value() -- Copy env. vars 
*
*  SYNOPSIS
*     void 
*     var_list_copy_env_vars_and_value(lList **varl, 
*                                      const lList *src_varl, 
*                                      const char *ignore_prefix) 
*
*  FUNCTION
*     Copy all variables from "src_varl" into "varl". Ignore
*     all variables beginning with "ignore_prefix".
*
*  INPUTS
*     lList **varl              - VA_Type list 
*     const lList *src_varl     - source VA_Type list 
*     const char *ignore_prefix - prefix 
*
*  RESULT
*     void - none
******************************************************************************/
void var_list_copy_env_vars_and_value(lList **varl,
                                      const lList* src_varl,
                                      const char *ignore_prefix) 
{
   lListElem *env;
   int n = strlen(ignore_prefix);

   for_each(env, src_varl) {
      const char *s, *name;

      /*
       * var_list_copy_complex_vars_and_value() might be used to handle 
       * variables which will skip now 
       */
      name = lGetString(env, VA_variable);
      if (n > 0 && !strncmp(name, ignore_prefix, n)) {
         continue; 
      }

      s = lGetString(env, VA_value);
      var_list_set_string(varl, name, s ? s : "");
   }
}

/****** sgeobj/var/var_list_remove_prefix_vars() ******************************
*  NAME
*     var_list_remove_prefix_vars() -- remove vars with certain prefix 
*
*  SYNOPSIS
*     void var_list_remove_prefix_vars(lList **varl, 
*                                      const char *prefix) 
*
*  FUNCTION
*     Remove all entries from "varl" where the name
*     beginns with "prefix" 
*
*  INPUTS
*     lList **varl       - VA_Type list 
*     const char *prefix - prefix string (e.g. VAR_PREFIX) 
*
*  SEE ALSO
*     sgeobj/var/var_list_remove_prefix_vars()
******************************************************************************/
void var_list_remove_prefix_vars(lList **varl, const char *prefix)
{
   int prefix_len = strlen(prefix);
   lListElem *var_elem = NULL;
   lListElem *next_var_elem = NULL;

   DENTER(TOP_LAYER, "var_list_remove_prefix_vars");
   next_var_elem = lFirst(*varl);
   while((var_elem = next_var_elem)) {
      const char *prefix_name = lGetString(var_elem, VA_variable);
      next_var_elem = lNext(var_elem);

      if (!strncmp(prefix_name, prefix, prefix_len)) {
         lRemoveElem(*varl, &var_elem);
      } 
   }
   DEXIT;
   return;
}

/****** sgeobj/var/var_list_split_prefix_vars() *******************************
*  NAME
*     var_list_split_prefix_vars() -- split a list of variables 
*
*  SYNOPSIS
*     void var_list_split_prefix_vars(lList **varl, 
*                                     lList **pefix_vars, 
*                                     const char *prefix) 
*
*  FUNCTION
*     Move all variable elements form "varl" to "pefix_vars" which 
*     begin with "prefix". *pefix_vars will be created if is does not
*     exist.
*
*  INPUTS
*     lList **varl        - VA_Type list 
*     lList **pefix_vars - pointer to VA_Type list 
*     const char *prefix - string (e.g. VAR_PREFIX) 
*
*  RESULT
*     void - None
*
*  SEE ALSO
*     sgeobj/var/var_list_remove_prefix_vars()
******************************************************************************/
void var_list_split_prefix_vars(lList **varl, 
                                lList **pefix_vars, 
                                const char *prefix)
{
   int prefix_len = strlen(prefix);
   lListElem *var_elem = NULL;
   lListElem *next_var_elem = NULL;

   DENTER(TOP_LAYER, "var_list_remove_prefix_vars");
   next_var_elem = lFirst(*varl);
   while((var_elem = next_var_elem)) {
      const char *prefix_name = lGetString(var_elem, VA_variable);
      next_var_elem = lNext(var_elem);

      if (!strncmp(prefix_name, prefix, prefix_len)) {
         lListElem *dechained_elem = lDechainElem(*varl, var_elem);

         if (*pefix_vars == NULL) {
            *pefix_vars = lCreateList("", VA_Type);
         }

         lAppendElem(*pefix_vars, dechained_elem);
      }
   }
   DEXIT;
   return;
}

/****** sgeobj/var/var_list_add_as_set() ***************************************
*  NAME
*     var_list_add_as_set() -- Concatenate two lists as sets
*
*  SYNOPSIS
*     int var_list_add_as_set(lList *lp0, lList *lp1) 
*
*  FUNCTION
*     Concatenate two lists of equal type throwing away the second list.
*     Elements in the second list will replace elements with the same key in the
*     the first list.  If the first list contains duplicate element keys, only
*     the first element with a given key will be replaced by an element from the
*     second list with the same key.
*
*  INPUTS
*     lList *lp0 - first list 
*     lList *lp1 - second list 
*
*  RESULT
*     int - error state
*         0 - OK
*        -1 - Error
******************************************************************************/
int var_list_add_as_set(lList *lp0, lList *lp1) 
{
   lListElem *ep0, *ep1;
   const lDescr *dp0, *dp1;
   const char *name, *value;

   DENTER(CULL_LAYER, "var_list_add_as_set");

   if (!lp1 || !lp0) {
      DEXIT;
      return -1;
   }

   /* Check if the two lists are equal */
   dp0 = lGetListDescr(lp0);
   dp1 = lGetListDescr(lp1);
   if (lCompListDescr(dp0, dp1)) {
      DEXIT;
      return -1;
   }

   while (lp1->first) {
      /* Get the first element from the second list */
      if (!(ep1 = lDechainElem(lp1, lp1->first))) {
         DEXIT;
         return -1;
      }
   
      /* Get it's name, and use the name to look for a matching element in the
       * first list. */
      name = lGetString (ep1, VA_variable);
      ep0 = lGetElemStr (lp0, VA_variable, name);

      /* If there is a matching element in the first list, set it's value to the
       * value of the element from the second list. */
      if (ep0 != NULL) {
         value = lGetString (ep1, VA_value);         
         lSetString (ep0, VA_value, value);
      }
      /* If there is no matching element, add the element from the second list
       * to the first list. */
      else {
         if (lAppendElem(lp0, ep1) == -1) {
            DEXIT;
            return -1;
         }
      }
   }

   /* The second list is no longer needed. */
   lFreeList(&lp1);

   DEXIT;
   return 0;
}