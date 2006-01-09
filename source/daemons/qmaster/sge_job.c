/*___INFO__MARK_BEGIN__*/
/*************************************************************************
 *
 *  The Contents of this file are made available subject to the terms of
 *  the Sun Industry Standards Source License Version 1.2
 *
 *  Sun Microsystems Inc., March, 2001    
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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <fnmatch.h>
#include <ctype.h>

#include "sgermon.h"
#include "def.h"
#include "sge_str_from_file.h"
#include "sge_time.h"
#include "sge_exit.h"
#include "sge_log.h"
#include "sge.h"
#include "symbols.h"
#include "sge_conf.h"
#include "sge_stringL.h"
#include "sge_sched.h"
#include "sge_feature.h"
#include "sec.h"
#include "mail.h"
#include "sge_jobL.h"
#include "sge_jataskL.h"
#include "sge_eventL.h"
#include "sge_queueL.h"
#include "sge_answerL.h"
#include "sge_requestL.h"
#include "sge_complexL.h"
#include "sge_ckptL.h"
#include "sge_peL.h"
#include "sge_userprjL.h"
#include "sge_job_refL.h"
#include "sge_host.h"
#include "sge_job.h"
#include "sge_give_jobs.h"
#include "job_log.h"
#include "sge_pe_qmaster.h"
#include "sge_qmod_qmaster.h"
#include "sge_queue_qmaster.h"
#include "sge_userset_qmaster.h"
#include "sge_ckpt_qmaster.h"
#include "job_report_qmaster.h"
#include "sge_parse_num_par.h"
#include "sge_complex.h"
#include "sge_m_event.h"
#include "sge_signal.h"
#include "subordinate_qmaster.h"
#include "sge_userset.h"
#include "sge_userprj_qmaster.h"
#include "read_write_job.h"
#include "sge_prognames.h"
#include "sge_me.h"
#include "cull_parse_util.h"
#include "schedd_monitor.h"
#include "sge_messageL.h"
#include "sge_rangeL.h"
#include "sge_identL.h"
#include "job.h"
#include "sge_copy_append.h"
#include "sge_arch.h"
#include "sge_afsutil.h"
#include "sge_ulongL.h"
#include "sge_switch_user.h"
#include "setup_path.h"
#include "sge_string.h"
#include "jb_now.h"
#include "sge_security.h"
#include "sge_range.h"
#include "sge_job_jatask.h"
#include "sge_job_jatask.h"
#include "qmaster.h"
#include "sge_suser.h"
#include "resolve_host.h"

#include "msg_schedd.h"
#include "msg_common.h"
#include "msg_utilib.h"
#include "msg_qmaster.h"
#include "msg_execd.h"
#include "msg_daemons_common.h"

extern lList *Master_Queue_List;
extern lList *Master_Exechost_List;
extern lList *Master_Complex_List;
extern lList *Master_Project_List;
extern lList *Master_Userset_List;
extern lList *Master_User_List;
extern lList *Master_Sharetree_List;
extern lList *Master_Job_List;
extern lList *Master_Job_Schedd_Info_List;
extern lList *Master_Zombie_List;
extern lList *Master_Pe_List;

extern int enable_forced_qdel;

static int mod_task_attributes(lListElem *job, lListElem *new_ja_task, lListElem *tep, lList **alpp, char *ruser, char *rhost, int *trigger, int is_array, int is_task_enrolled);
static int mod_job_attributes(lListElem *new_job, lListElem *jep, lList **alpp, char *ruser, char *rhost, int *trigger); 

static int compress_ressources(lList **alpp, lList *rl); 
static void set_context(lList *ctx, lListElem *job); 
static u_long32 guess_highest_job_number(void);
static int verify_suitable_queues(lList **alpp, lListElem *jep, int *trigger);

static int changes_consumables(lList **alpp, lList* new, lList* old);
static int deny_soft_consumables(lList **alpp, lList *srl);
static lCondition *job_list_filter(int user_list_flag, lList *user_list, int jid_flag, u_long32 jobid, int all_users_flag, int all_jobs_flag, char *ruser);
static int job_verify_predecessors(const lListElem *job, lList **alpp, lList *predecessors);
static int job_verify_name(const lListElem *job, lList **alpp, const char *job_descr);
static u_long32 is_referenced_by_jobname(lListElem *jep);
static int verify_job_list_filter(lList **alpp, int all_users_flag, int all_jobs_flag, int jid_flag, int user_list_flag, char *ruser);
static void empty_job_list_filter(lList **alpp, int was_modify, int user_list_flag, lList *user_list, int jid_flag, u_long32 jobid, int all_users_flag, int all_jobs_flag, char *ruser, int is_array, u_long32 start, u_long32 end, u_long32 step);   
static u_long32 sge_get_job_number(void);
static void get_rid_of_schedd_job_messages(u_long32 job_number);
static int job_check_qsh_display(lListElem *job, lList **answer_list);
int job_resolve_host_for_path_list(const lListElem *job, lList **answer_list, int name);
/*-------------------------------------------------------------------------*/
/* sge_gdi_add_job                                                       */
/*    called in sge_c_gdi_add                                              */
/*-------------------------------------------------------------------------*/
int sge_gdi_add_job(lListElem *jep, lList **alpp, lList **lpp, char *ruser,
                    char *rhost, sge_gdi_request *request) 
{
   int ckpt_err;
   lListElem *reqep, *qep;
   const char *pe_name, *project, *ckpt_name;
   u_long32 ckpt_attr, ckpt_inter;
   u_long32 job_number;
   lListElem *ckpt_ep;
   char str[1024 + 1]="";
   u_long32 start, end, step;
   uid_t uid;
   gid_t gid;
   char user[128];
   char group[128];
   lList *pe_range = NULL;
   unsigned long pe_range_max = 0;
   unsigned long pe_range_min = 0;

   DENTER(TOP_LAYER, "sge_gdi_add_job");

   if ( !jep || !ruser || !rhost ) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* we take these values from gdi request structure */
   if (sge_get_auth_info(request, &uid, user, &gid, group) == -1) {
      ERROR((SGE_EVENT, MSG_GDI_FAILEDTOEXTRACTAUTHINFO));
      sge_add_answer(alpp, SGE_EVENT, STATUS_ENOMGR, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* check conf.min_uid */
   if (uid < conf.min_uid) {
      ERROR((SGE_EVENT, MSG_JOB_UID2LOW_II, (int)uid, (int)conf.min_uid));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* check conf.min_gid */
   if (gid < conf.min_gid) {
      ERROR((SGE_EVENT, MSG_JOB_GID2LOW_II, (int)gid, (int)conf.min_gid));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* check for qsh without DISPLAY set */
   if(JB_NOW_IS_QSH(lGetUlong(jep, JB_now))) {
      int ret = job_check_qsh_display(jep, alpp);
      if(ret != STATUS_OK) {
         DEXIT;
         return ret;
      }
   }

   /* 
    * fill in user and group
    *
    * this is not done by the submitter because we want to implement an 
    * gdi submit request it would be bad if you could say 
    * job->uid = 0 before submitting
    */
   lSetString(jep, JB_owner, user);
   lSetUlong(jep, JB_uid, uid);
   lSetString(jep, JB_group, group);
   lSetUlong(jep, JB_gid, gid);
   
   /* check conf.max_u_user */
   if((lGetUlong(jep, JB_verify_suitable_queues) != JUST_VERIFY)) {
      if(suser_check_new_job(jep, conf.max_u_jobs) != 0) {
         INFO((SGE_EVENT, MSG_JOB_ALLOWEDJOBSPERUSER_UU, u32c(conf.max_u_jobs), 
                                                         u32c(suser_job_count(jep))));
         sge_add_answer(alpp, SGE_EVENT, STATUS_NOTOK_DOAGAIN, 0);
         DEXIT;
         return STATUS_NOTOK_DOAGAIN;
      }
   }

   if (!sge_has_access_(lGetString(jep, JB_owner), lGetString(jep, JB_group), 
         conf.user_lists, conf.xuser_lists, Master_Userset_List)) {
      ERROR((SGE_EVENT, MSG_JOB_NOPERMS_SS, ruser, rhost));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   {
      int field[] = { 
         JB_ja_structure,
         JB_ja_n_h_ids,             
         JB_ja_u_h_ids,             
         JB_ja_s_h_ids,             
         JB_ja_o_h_ids,             
         JB_ja_z_ids,
         -1
      };
      int i = -1;

      while (field[++i] != -1) {
         lList *range_list = lGetList(jep, field[i]);
         lListElem *range = NULL;

         for_each(range, range_list) { 
            range_correct_end(range);
            if (range_is_id_within(range, 0)) {
               ERROR((SGE_EVENT, MSG_JOB_NULLNOTALLOWEDT));
               sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
               DEXIT;
               return STATUS_EUNKNOWN;
            }
         }
      }
   }


   /*
    * resolve host names. If this is not possible and error is produced
    */
   {
      int status;
      if( (status=job_resolve_host_for_path_list(jep, alpp, JB_stdout_path_list)) != STATUS_OK){
         DEXIT;
         return status;
      }

      if( (status=job_resolve_host_for_path_list(jep, alpp, JB_stderr_path_list)) != STATUS_OK){
         DEXIT;
         return status;
      }

      if( (status = job_resolve_host_for_path_list(jep, alpp,JB_shell_list)) != STATUS_OK){
         DEXIT;
         return status;
      }
   }



   /*
    * Is the max. size of array jobs exceeded?
    */
   if (conf.max_aj_tasks > 0) {
      lList *range_list = lGetList(jep, JB_ja_structure);
      u_long32 submit_size = range_list_get_number_of_ids(range_list);
   
      if (submit_size > conf.max_aj_tasks) {
         ERROR((SGE_EVENT, MSG_JOB_MORETASKSTHAN_U, u32c(conf.max_aj_tasks)));
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         DEXIT;
         return STATUS_EUNKNOWN;
      } 
   }

   /* fill name and shortcut for all requests
    * fill numeric values for all bool, time, memory and int type requests
    * use the Master_Complex_List for all fills
    * JB_hard/soft_resource_list points to a RE_Type list
    * RE_entries points to a CE_Type list
    */
   for_each(reqep, lGetList(jep, JB_hard_resource_list) ) {
      if (sge_fill_requests(lGetList(reqep,RE_entries), Master_Complex_List, 0, 1, 0)) {
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         DEXIT;
         return STATUS_EUNKNOWN;
      }
   }
   if (compress_ressources(alpp, lGetList(jep, JB_hard_resource_list))) {
      DEXIT;
      return STATUS_EUNKNOWN;
   }
   
   for_each(reqep, lGetList(jep, JB_soft_resource_list) ) {
      if (sge_fill_requests(lGetList(reqep,RE_entries), Master_Complex_List, 0, 1, 0)) {
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         DEXIT;
         return STATUS_EUNKNOWN;
      }
   }
   if (compress_ressources(alpp, lGetList(jep, JB_soft_resource_list))) {
      DEXIT;
      return STATUS_EUNKNOWN;
   }
   if (deny_soft_consumables(alpp, lGetList(jep, JB_soft_resource_list))) {
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   {
      lList* temp = NULL;
      lXchgList(jep, JB_context, &temp); 
      set_context(temp, jep);
      lFreeList(temp);
   }

   /* attribute "qname" in queue complex must be requestable for -q */
   if (lGetList(jep, JB_hard_queue_list)) {
      const char *qname;
      lListElem *ep;

      if (!queues_are_requestable(Master_Complex_List)) {
         ERROR((SGE_EVENT, MSG_JOB_QNOTREQUESTABLE)); 
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         DEXIT;
         return STATUS_EUNKNOWN;
      }

      for_each (ep, lGetList(jep, JB_hard_queue_list)) {
         qname = lGetString(ep, QR_name);
         if (!sge_locate_queue(qname)) {
            ERROR((SGE_EVENT, MSG_JOB_QUNKNOWN_S, qname));
            sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
            DEXIT;
            return STATUS_EUNKNOWN;
         }
      }
   }

   /* attribute "qname" in queue complex must be requestable for -masterq */
   if (lGetList(jep, JB_master_hard_queue_list)) {
      const char *qname;
      lListElem *ep;

      if (!queues_are_requestable(Master_Complex_List)) {
         ERROR((SGE_EVENT, MSG_JOB_QNOTREQUESTABLE)); 
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         DEXIT;
         return STATUS_EUNKNOWN;
      }

      for_each (ep, lGetList(jep, JB_master_hard_queue_list)) {
         qname = lGetString(ep, QR_name);
         if (!sge_locate_queue(qname)) {
            ERROR((SGE_EVENT, MSG_JOB_QUNKNOWN_S, qname));
            sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
            DEXIT;
            return STATUS_EUNKNOWN;
         }
      }
   }

   /*
   ** if the job has no JB_script_file it is an interactive job
   ** if it is a batch job, it must have a script file
   */
   if (!lGetString(jep, JB_script_ptr)  && lGetString(jep, JB_script_file)) { 
      ERROR((SGE_EVENT, MSG_JOB_NOSCRIPT));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* 
      here we test (if requested) the 
      parallel environment exists;
      if not the job is refused
   */
   pe_name = lGetString(jep, JB_pe);
   if (pe_name) {
      lListElem *pep;
      pep = sge_match_pe(pe_name);
      if (!pep) {
         ERROR((SGE_EVENT, MSG_JOB_PEUNKNOWN_S, pe_name));
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         DEXIT;
         return STATUS_EUNKNOWN;
      }
      /* check pe_range */
      pe_range = lGetList(jep, JB_pe_range);
      if (pe_range) {
         lListElem *relem = NULL;
         for_each(relem,pe_range) {
            pe_range_min = lGetUlong(relem, RN_min);
            pe_range_max = lGetUlong(relem, RN_max);
            DPRINTF(("pe max = %ld, pe min = %ld\n", pe_range_max, pe_range_min));
            if ( pe_range_max == 0 || pe_range_min == 0  ) {
               ERROR((SGE_EVENT, MSG_JOB_PERANGEMUSTBEGRZERO ));
               sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
               DEXIT;
               return STATUS_EUNKNOWN;
            }
         }
      }
      /* reject tightly integrated parallel array jobs 
       * in case of wildcard pe specification, check all possible 
       * pe's and reject job if any of them implements tight integration.
       */
      if (is_array(jep)) {
         lListElem *pep;

         for_each (pep, Master_Pe_List) {
            if (fnmatch(pe_name, lGetString(pep, PE_name), 0) == 0) {
               if (lGetUlong(pep, PE_control_slaves) != 0) {
                  ERROR((SGE_EVENT, MSG_QMASTER_TIGHTLYINTEGRATEDARRAYJOB));
                  sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
                  DEXIT;
                  return STATUS_EUNKNOWN;
               }
            }
         }
      }
   }

   ckpt_err = 0;
   /* command line -c switch has higher precedence than ckpt "when" */
   ckpt_attr = lGetUlong(jep, JB_checkpoint_attr);
   ckpt_inter = lGetUlong(jep, JB_checkpoint_interval);

   /* request for non existing ckpt object will be refused */
   if ((ckpt_name = lGetString(jep, JB_checkpoint_object))) {
      if (!(ckpt_ep = sge_locate_ckpt(ckpt_name)))
         ckpt_err = 1;
      else if (!ckpt_attr) {
         ckpt_attr = sge_parse_checkpoint_attr(lGetString(ckpt_ep, CK_when));
         lSetUlong(jep, JB_checkpoint_attr, ckpt_attr);
      }   
   }

   if (!ckpt_err) {
      if ((ckpt_attr & NO_CHECKPOINT) && (ckpt_attr & ~NO_CHECKPOINT))
         ckpt_err = 2;
      else if (ckpt_name && (ckpt_attr & NO_CHECKPOINT))
         ckpt_err = 3;   
      else if ((!ckpt_name && (ckpt_attr & ~NO_CHECKPOINT)))
         ckpt_err = 4;
      else if (!ckpt_name && ckpt_inter) 
         ckpt_err = 5;
   }

   if (ckpt_err) {
      switch (ckpt_err) {
      case 1:
         sprintf(str, MSG_JOB_CKPTUNKNOWN_S, ckpt_name);
   	 break;
      case 2:
      case 3:
         sprintf(str, MSG_JOB_CKPTMINUSC);
         break;
      case 4:
      case 5:
         sprintf(str, MSG_JOB_NOCKPTREQ);
   	 break;
      default:
         sprintf(str, MSG_JOB_CKPTDENIED);
         break;
      }                 
      
      ERROR((SGE_EVENT, "%s", str));
      sge_add_answer(alpp, SGE_EVENT, STATUS_ESEMANTIC, 0);
      DEXIT;
      return STATUS_ESEMANTIC;
   }
   
   /* first check user permissions */
   { 
      int has_permissions = 0;
      for_each (qep, Master_Queue_List) {
         if (sge_has_access(lGetString(jep, JB_owner), lGetString(jep, JB_group), 
               qep, Master_Userset_List)) {
            DPRINTF(("job has access to queue "SFQ"\n", lGetString(qep, QU_qname)));      
            has_permissions = 1;
            break;
         }
      }
      if (!has_permissions) {
         SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_JOB_NOTINANYQ_S, ruser));
         sge_add_answer(alpp, SGE_EVENT, STATUS_ESEMANTIC, 0);
      }
   }

   /* check sge attributes */
   if (feature_is_enabled(FEATURE_SGEEE)) {

      /* ensure user exists if enforce_user flag is set */
      if (conf.enforce_user && !strcasecmp(conf.enforce_user, "true") && 
               !sge_locate_user_prj(ruser, Master_User_List)) {
         ERROR((SGE_EVENT, MSG_JOB_USRUNKNOWN_S, ruser));
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         DEXIT;
         return STATUS_EUNKNOWN;
      } 

      /* set default project */
      if (!lGetString(jep, JB_project) && ruser && Master_User_List) {
         lListElem *uep;
         if ((uep = sge_locate_user_prj(ruser, Master_User_List)))
            lSetString(jep, JB_project, lGetString(uep, UP_default_project));
      }

      /* project */
      if ((project=lGetString(jep, JB_project))) {
         lListElem *pep;
         if (!(pep = sge_locate_user_prj(project, Master_Project_List))) {
            ERROR((SGE_EVENT, MSG_JOB_PRJUNKNOWN_S, project));
            sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
            DEXIT;
            return STATUS_EUNKNOWN;
         }

         /* ensure user belongs to this project */
         if (!sge_has_access_(user, group, 
               lGetList(pep, UP_acl), 
               lGetList(pep, UP_xacl), Master_Userset_List)) {
            ERROR((SGE_EVENT, MSG_SGETEXT_NO_ACCESS2PRJ4USER_SS,
               project, user));
            sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
            DEXIT;
            return STATUS_EUNKNOWN;
         }

         /* verify project can submit jobs */
         if ((conf.xprojects &&
              lGetElemStr(conf.xprojects, UP_name, project)) ||
             (conf.projects &&
              !lGetElemStr(conf.projects, UP_name, project))) {
            ERROR((SGE_EVENT, MSG_JOB_PRJNOSUBMITPERMS_S, project));
            sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
            DEXIT;
            return STATUS_EUNKNOWN;
         }

      } else {

         if (lGetNumberOfElem(conf.projects)>0) {
            ERROR((SGE_EVENT, MSG_JOB_PRJREQUIRED)); 
            sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
            DEXIT;
            return STATUS_EUNKNOWN;
         }

         if (conf.enforce_project && !strcasecmp(conf.enforce_project, "true")) {
            ERROR((SGE_EVENT, MSG_SGETEXT_NO_PROJECT));
            sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
            DEXIT;
            return STATUS_EUNKNOWN;
         }
      }
   }

   if (feature_is_enabled(FEATURE_SGEEE)) {
      /* try to dispatch a department to the job */
      if (set_department(alpp, jep, Master_Userset_List)!=1) {
         /* alpp gets filled by set_department */
         DEXIT;
         return STATUS_EUNKNOWN;
      }

      /* 
         If it is a deadline job the user has to be a deadline user
      */
      if (lGetUlong(jep, JB_deadline)) {
         if (!is_deadline_user(ruser, Master_Userset_List)) {
            ERROR((SGE_EVENT, MSG_JOB_NODEADLINEUSER_S, ruser));
            sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
            DEXIT;
            return STATUS_EUNKNOWN;
         }
      }
   }
   /* 
      here we have to fill in user 
      and group into the job 
      values filled in by the 
      submitter may get overwritten 

   */
   lSetString(jep, JB_owner, ruser);

   /* verify schedulability */
   {
      int ret = verify_suitable_queues(alpp, jep, NULL); 
      if (lGetUlong(jep, JB_verify_suitable_queues)==JUST_VERIFY 
         || ret != 0) {
         DEXIT;
         return ret;
      }   
   }

   /* set automatic default values */
   job_number = sge_get_job_number();
   lSetUlong(jep, JB_job_number, job_number);

   /*
    * only operators and managers are allowed to submit
    * jobs with higher priority than 0 (=BASE_PRIORITY)
    * we silently lower it to 0 in case someone tries to cheat
    */
   if (lGetUlong(jep, JB_priority)>BASE_PRIORITY && sge_operator(ruser))
      lSetUlong(jep, JB_priority, BASE_PRIORITY);

   lSetUlong(jep, JB_submission_time, sge_get_gmt());

   lSetList(jep, JB_ja_tasks, NULL);
   lSetList(jep, JB_jid_sucessor_list, NULL);

   if (lGetList(jep, JB_ja_template) == NULL) {
      lAddSubUlong(jep, JAT_task_number, 0, JB_ja_template, JAT_Type);
   }

   sprintf(str, "%s/%d", JOB_DIR, (int)job_number);
   lSetString(jep, JB_job_file, str);

   /*
   ** with interactive jobs, JB_exec_file is not set
   */
   if (lGetString(jep, JB_script_file)) {
      sprintf(str, "%s/%d", EXEC_DIR, (int)job_number);
      lSetString(jep, JB_exec_file, str);
   }

   if (!lGetString(jep, JB_account)) {
      lSetString(jep, JB_account, DEFAULT_ACCOUNT);
   } else {
      if (!job_has_valid_account_string(jep, alpp)) {
         return STATUS_EUNKNOWN;
      }
   }


   if (!lGetString(jep, JB_job_name)) {        /* look for job name */
      ERROR((SGE_EVENT, MSG_JOB_NOJOBNAME_U, u32c(job_number)));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   if (job_verify_name(jep, alpp, "this job")) {
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* checks on -hold_jid */
   if (job_verify_predecessors(jep, alpp, 
                               lGetList(jep, JB_jid_predecessor_list))) {
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* write script to file */
   if (lGetString(jep, JB_script_file)) {
      if (str2file(lGetString(jep, JB_script_ptr), 
                   lGetUlong(jep, JB_script_size),
                   lGetString(jep, JB_exec_file))) {
         ERROR((SGE_EVENT, MSG_JOB_NOWRITE_US, u32c(job_number), strerror(errno)));
         sge_add_answer(alpp, SGE_EVENT, STATUS_EDISK, 0);
         DEXIT;
         return STATUS_EDISK;
      }
   }

   /* clean file out of memory */
   lSetString(jep, JB_script_ptr, NULL);
   lSetUlong(jep, JB_script_size, 0);


   /*
   ** security hook
   **
   ** Execute command to store the client's DCE or Kerberos credentials.
   ** This also creates a forwardable credential for the user.
   */
   if (do_credentials) {
      if (store_sec_cred(request, jep, do_authentication, alpp) != 0) {
         DEXIT;
         return STATUS_EUNKNOWN;
      }
   }

   job_suc_pre(jep);

   if (job_write_spool_file(jep, 0, SPOOL_DEFAULT)) {
      ERROR((SGE_EVENT, MSG_JOB_NOWRITE_U, u32c(job_number)));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EDISK, 0);
      DEXIT;
      return STATUS_EDISK;
   }

   if (!is_array(jep)) {
      DPRINTF(("Added Job "u32"\n", lGetUlong(jep, JB_job_number)));
   } else {
      job_get_ja_task_ids(jep, &start, &end, &step);
      DPRINTF(("Added JobArray "u32"."u32"-"u32":"u32"\n", 
                lGetUlong(jep, JB_job_number), start, end, step));
   }
   
   /* add into job list */
   if (job_list_add_job(&Master_Job_List, "Master_Job_List", lCopyElem(jep), 
                        1)) {
      sge_add_answer(alpp, SGE_EVENT, STATUS_EDISK, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }
   /** increase user counter */
   suser_register_new_job(jep, conf.max_u_jobs, 0);

   /* generate an sgeE_JOB_ADD event and queue it into the event list */
   sge_add_job_event(sgeE_JOB_ADD, jep, 0);

   /*
   ** immediate jobs trigger scheduling immediately
   */
   {
      lListElem *schedd = sge_locate_scheduler();
      if(schedd != NULL) {
         if (JB_NOW_IS_IMMEDIATE(lGetUlong(jep, JB_now))) {
            sge_flush_events(schedd, FLUSH_EVENTS_SET);
         } else {
            sge_flush_events(schedd, FLUSH_EVENTS_JOB_SUBMITTED);
         }
      }
   }

   *str = 0;
   if (lGetList(jep, JB_job_args)) {
      lList *sp = lGetList(jep, JB_job_args);
      lListElem *se;
      int str_size = sizeof(str)-1;

      for_each(se,sp) {
         int do_quote = 0;
         int i = 0;
         int n;
         const char *s = lGetString(se,STR);

         /* handle NULL as empty string */
         if (s == NULL) {
            s = "";
         }
         n = strlen(s);
 
         /* quote for empty strings */
         if (n == 0)
            do_quote++;

         /* quote when white space is in argument */         
         for( i=0 ; i < n; i++ ) {
            if (s[i] == ' ') {
                do_quote++;
                break;
            }
         }

         strncat(str, " ", str_size);
         if (--str_size <= 0)
            break;

         if (do_quote != 0) {
            strncat(str, "\"", str_size);
            if (--str_size <= 0)
               break;
         }

         strncat(str, s, str_size);
         str_size -= n;
         if (str_size <= 0)
            break;

         if (do_quote != 0) {
            strncat(str, "\"", str_size);
            if (--str_size <= 0)
               break;
         }
      }
   }

   if (!is_array(jep)) {
      sprintf(SGE_EVENT, MSG_JOB_SUBMITJOB_USS,  
            u32c(lGetUlong(jep, JB_job_number)), 
            lGetString(jep, JB_job_name), str);
   } else {
      sprintf(SGE_EVENT, MSG_JOB_SUBMITJOBARRAY_UUUUSS,
            u32c(lGetUlong(jep, JB_job_number)), u32c(start), 
            u32c(end), u32c(step), 
            lGetString(jep, JB_job_name), str);
   }
   sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);

   /*
   **  add element to return list if necessary
   */
   if (lpp) {
      if (!*lpp)
         *lpp = lCreateList("Job Return", JB_Type);
      lAppendElem(*lpp, lCopyElem(jep));
   }   

   job_log(lGetUlong(jep, JB_job_number), 0, MSG_LOG_NEWJOB);

   DEXIT;
   return STATUS_OK;
}




/*-------------------------------------------------------------------------*/
/* sge_gdi_delete_job                                                    */
/*    called in sge_c_gdi_del                                              */
/*-------------------------------------------------------------------------*/

int sge_gdi_del_job(
lListElem *idep,
lList **alpp,
char *ruser,
char *rhost,
int sub_command 
) {
   lListElem *nxt, *job = NULL, *rn;
   u_long32 enrolled_start = 0;
   u_long32 enrolled_end = 0;
   u_long32 unenrolled_start = 0;
   u_long32 unenrolled_end = 0;
   u_long32 r_start = 0;
   u_long32 r_end = 0;   
   u_long32 step = 0;
   u_long32 job_number = 0;
   int alltasks = 1;
   int showmessage = 0;
   int all_jobs_flag;
   int all_users_flag;
   int jid_flag;
   int user_list_flag;
   const char *jid_str;
   lCondition *where = NULL; 
   int ret, njobs = 0;
   u_long32 deleted_tasks = 0;
   u_long32 time;
   u_long32 start_time;

   DENTER(TOP_LAYER, "sge_gdi_del_job");

   if ( !idep || !ruser || !rhost ) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* sub-commands */
   all_jobs_flag = ((sub_command & SGE_GDI_ALL_JOBS) > 0);
   all_users_flag = ((sub_command & SGE_GDI_ALL_USERS) > 0);

   /* Did we get a user list? */
   if (lGetList(idep, ID_user_list) 
       && lGetNumberOfElem(lGetList(idep, ID_user_list)) > 0)
      user_list_flag = 1;
   else
      user_list_flag = 0;

   /* Did we get a valid jobid? */
   if ((jid_str = lGetString(idep, ID_str)) && (atol(jid_str) > 0)) {
      jid_flag = 1;
   } else {
      jid_flag = 0;    
   }

   if ((ret=verify_job_list_filter(alpp, all_users_flag, all_jobs_flag, 
         jid_flag, user_list_flag, ruser))) { 
      DEXIT;
      return ret;
   }

   where = job_list_filter(
      user_list_flag, lGetList(idep, ID_user_list), /* user list */
      jid_flag, jid_flag?atol(lGetString(idep, ID_str)):0,     /* job id */
      all_users_flag,       /* all jobs  */
      all_jobs_flag,        /* all jobs in user list */
      ruser);

   /* first lets make sure they have permission if a force is involved */
   if (enable_forced_qdel != 1) {/* Flag ENABLE_FORCED_QDEL in qmaster_params */
      if (lGetUlong(idep, ID_force)) {
         if (sge_manager(ruser)) {
            ERROR((SGE_EVENT, MSG_JOB_FORCEDDELETEPERMS_S, ruser));
            sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
            lFreeWhere(where);
            DEXIT;
            return STATUS_EUNKNOWN;  
         }
      }
   }

   start_time = sge_get_gmt();
   nxt = lFirst(Master_Job_List);
   while ((job=nxt)) {
      u_long32 task_number;
      u_long32 existing_tasks;
      int deleted_unenrolled_tasks;

      nxt = lNext(job);   

      if (!lCompare(job, where)) {
         continue;
      } 

      job_number = lGetUlong(job, JB_job_number);

      /*
       * Repeat until all requested taskid ranges are handled
       */
      rn = lFirst(lGetList(idep, ID_ja_structure));
      do {
         /*
          * delete tasks or the whole job?
          * if ID_ja_structure not empty delete specified tasks
          * otherwise delete whole job
          */
         unenrolled_start = job_get_smallest_unenrolled_task_id(job);
         unenrolled_end = job_get_biggest_unenrolled_task_id(job);
         enrolled_start = job_get_smallest_enrolled_task_id(job);
         enrolled_end = job_get_biggest_enrolled_task_id(job);
         rn = lFirst(lGetList(idep, ID_ja_structure));
         if (rn) {
            r_start = lGetUlong(rn, RN_min);
            r_end = lGetUlong(rn, RN_max);
            unenrolled_start = MAX(r_start, unenrolled_start);
            unenrolled_end = MIN(r_end, unenrolled_end);
            enrolled_start = MAX(r_start, enrolled_start);
            enrolled_end = MIN(r_end, enrolled_end);
            step = lGetUlong(rn, RN_step);
            if (!step) {
               step = 1;
            }
            alltasks = 0;
         } else {
            step = 1;
            alltasks = 1;
         }
         DPRINTF(("Request: alltasks = %d, start = %d, end = %d, step = %d\n",
                  alltasks, r_start, r_end, step));
         DPRINTF(("unenrolled ----> start = %d, end = %d, step = %d\n",
                  unenrolled_start, unenrolled_end, step));
         DPRINTF(("enrolled   ----> start = %d, end = %d, step = %d\n",
                  enrolled_start, enrolled_end, step));


         /* Does user have privileges to delete the job/task? */
         if (sge_job_owner(ruser, lGetUlong(job, JB_job_number)) && 
             sge_manager(ruser)) {
            ERROR((SGE_EVENT, MSG_JOB_DELETEPERMS_SU, ruser, 
               u32c(lGetUlong(job, JB_job_number))));
            sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
            njobs++;
            /* continue with next job */
            break;
         }

         showmessage = 0;

         /*
          * Delete all unenrolled pending tasks
          */
         deleted_unenrolled_tasks = 0;
         deleted_tasks = 0;
         existing_tasks = job_get_ja_tasks(job);
         for (task_number = unenrolled_start; 
              task_number <= unenrolled_end; 
              task_number += step) {
            int is_defined = job_is_ja_task_defined(job, task_number); 

            if (is_defined) {
               int is_enrolled = job_is_enrolled(job, task_number);

               if (!is_enrolled) {
                  lListElem *tmp_task = NULL;

                  tmp_task = job_get_ja_task_template_pending(job, task_number);
                  deleted_tasks++;
                  sge_commit_job(job, tmp_task, 3, COMMIT_NO_SPOOLING |
                     COMMIT_NO_EVENTS | COMMIT_UNENROLLED_TASK);
                  sge_add_event(NULL, sgeE_JATASK_DEL, job_number, task_number,
                                NULL, NULL);
                  deleted_unenrolled_tasks = 1;
                  showmessage = 1;
                  if (!alltasks) {
                     INFO((SGE_EVENT, MSG_JOB_DELETETASK_SUU,
                           ruser, u32c(job_number), u32c(task_number)));
                     sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
                  }         
               }
            }
            increment_heartbeat(sge_get_gmt());
         }
         if (deleted_unenrolled_tasks) {
            lListElem *schedd;

            if (conf.zombie_jobs > 0) {
               lListElem *zombie;

               zombie = lGetElemUlong(Master_Zombie_List, JB_job_number, 
                                      job_number);
               if (zombie) { 
                  job_write_spool_file(zombie, 0, SPOOL_HANDLE_AS_ZOMBIE);
               }
            }
            if (existing_tasks > deleted_tasks) {
               job_write_common_part(job, 0, SPOOL_DEFAULT);
            } else {
               sge_add_event(NULL, sgeE_JOB_DEL, job_number, 0,
                             NULL, NULL);
            }
            schedd = sge_locate_scheduler();
            if(schedd != NULL) {
               sge_flush_events(schedd, FLUSH_EVENTS_JOB_FINISHED);                         } 
         }

         /*
          * Delete enrolled ja tasks
          */
         if (existing_tasks > deleted_tasks) { 
            for (task_number = enrolled_start; 
                 task_number <= enrolled_end; 
                 task_number += step) {
               int spool_job = 1;
               int is_defined = job_is_ja_task_defined(job, task_number);
              
               if (is_defined) {
                  lListElem *tmp_task = NULL;

                  tmp_task = lGetElemUlong(lGetList(job, JB_ja_tasks), 
                                           JAT_task_number, task_number);
                  if (tmp_task == NULL) {
                     /* ja task does not exist anymore - ignore silently */
                     continue;
                  }

                  njobs++; 

                  if (lGetString(tmp_task, JAT_master_queue)) {
                     job_ja_task_send_abort_mail(job, tmp_task, ruser, 
                                                 rhost, NULL);
                     get_rid_of_job_due_to_qdel(job, tmp_task,
                                                alpp, ruser, 
                                                lGetUlong(idep, ID_force));
                  } else {
                     sge_commit_job(job, tmp_task, 3, spool_job);
                     showmessage = 1;
                     if (!alltasks) {
                        INFO((SGE_EVENT, MSG_JOB_DELETETASK_SUU,
                              ruser, u32c(job_number), u32c(task_number)));
                        sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
                     }
                  }
               } else {
                  ; /* Task did never exist! - Ignore silently */
               }
               increment_heartbeat(sge_get_gmt()); 
            }
         }
         if (alltasks && showmessage) {
            get_rid_of_schedd_job_messages(job_number);
            INFO((SGE_EVENT, MSG_JOB_DELETEJOB_SU, ruser, u32c(job_number)));
            sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
         }
      
         time = sge_get_gmt();
         if (time - start_time > 6) {
            INFO((SGE_EVENT, MSG_JOB_DISCONTINUEDTRANS_SU, ruser, 
                  u32c(job_number)));
            sge_add_answer(alpp, SGE_EVENT, STATUS_OK_DOAGAIN, NUM_AN_INFO); 
            lFreeWhere(where);
            return STATUS_OK;
         }

         rn = lNext(rn);                       
      } while (rn != NULL);
   }

   lFreeWhere(where);

   if (!njobs && !deleted_tasks) {
      empty_job_list_filter(alpp, 0, user_list_flag,
            lGetList(idep, ID_user_list), jid_flag,
            jid_flag?atol(lGetString(idep, ID_str)):0,
            all_users_flag, all_jobs_flag, ruser,
            alltasks == 0 ? 1 : 0, r_start, r_end, step);
      DEXIT;
      return STATUS_EEXIST;
   }    

   DEXIT;
   return STATUS_OK;
}

static void empty_job_list_filter(
lList **alpp,
int was_modify,
int user_list_flag,
lList *user_list,
int jid_flag,
u_long32 jobid,
int all_users_flag,
int all_jobs_flag,
char *ruser, 
int is_array,
u_long32 start, 
u_long32 end, 
u_long32 step
) {
   DENTER(TOP_LAYER, "empty_job_list_filter");

   if (all_users_flag) {
      ERROR((SGE_EVENT, MSG_SGETEXT_THEREARENOJOBS));
   } else if (user_list_flag) {
      lListElem *user;
      char user_list_string[2048] = "";
      int umax = 20;

      for_each (user, user_list) {
         if (user_list_string[0] != 0)
            strcat(user_list_string, ", ");
         if (--umax)
            strcat(user_list_string, lGetString(user, STR));
         else {
            /* prevent buffer overrun */
            strcat(user_list_string, "...");
            break;
         }
      }
      ERROR((SGE_EVENT, MSG_SGETEXT_THEREARENOJOBSFORUSERS_S, 
             user_list_string));
   } else if (all_jobs_flag) {
      ERROR((SGE_EVENT, MSG_SGETEXT_THEREARENOJOBSFORUSERS_S, ruser));
   } else if (jid_flag) {
      if(is_array) {
         if(start == end) {
            ERROR((SGE_EVENT, MSG_SGETEXT_DOESNOTEXISTTASK_UU, 
                   u32c(jobid), u32c(start)));
         } else {
            ERROR((SGE_EVENT, MSG_SGETEXT_DOESNOTEXISTTASKRANGE_UUUU, 
                   u32c(jobid), u32c(start), u32c(end), u32c(step)));
         }
      } else {
         ERROR((SGE_EVENT,MSG_SGETEXT_DOESNOTEXIST_SU, "job", u32c(jobid)));
      }      
   } else {
      /* Should not be possible */
      ERROR((SGE_EVENT,
             was_modify?MSG_SGETEXT_NOJOBSMODIFIED:MSG_SGETEXT_NOJOBSDELETED));
   }

   sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
   DEXIT;
   return;
}            

/* Build filter for the joblist */
static lCondition *job_list_filter(
int user_list_flag,
lList *user_list,
int jid_flag,
u_long32 jobid,
int all_users_flag,
int all_jobs_flag,
char *ruser 
) {
   lCondition *where = NULL, *new_where = NULL;

   DENTER(TOP_LAYER, "job_list_filter");

   if (user_list_flag) {
      lListElem *user;
      lCondition *or_where = NULL;

      DPRINTF(("Add all users given in userlist to filter\n"));
      for_each(user, user_list) {

         new_where = lWhere("%T(%I==%s)", JB_Type, JB_owner,
               lGetString(user, STR));
         if (!or_where)
            or_where = new_where;
         else
            or_where = lOrWhere(or_where, new_where);
      }
      if (!where)
         where = or_where;
      else
         where = lAndWhere(where, or_where);
   }

   if (all_jobs_flag) {
      DPRINTF(("Add current user to filter\n"));
      new_where = lWhere("%T(%I==%s)", JB_Type, JB_owner, ruser);
      if (!where)
         where = new_where;
      else
         where = lAndWhere(where, new_where);
   }

   if (jid_flag) {
      DPRINTF(("Add jid "u32" to filter\n", jobid));
      new_where = lWhere("%T(%I==%u)", JB_Type, JB_job_number, jobid);
      if (!where)
         where = new_where;
      else
         where = lAndWhere(where, new_where);
   }

   DEXIT;
   return where;
}

/*
   qalter -uall               => all_users_flag = true
   qalter ... <jid> ...       => jid_flag = true
   qalter -u <username> ...   => user_list_flag = true
   qalter ... all             => all_jobs_flag = true

   1) all_users_flag && all_jobs_flag     => all jobs of all users (requires
                                             manager pevileges)
   2) all_users_flag && jid_flag          => not valid
   3) all_users_flag                      => all jobs of all users (requires
                                             manager pevileges)
   4) user_list_flag && all_jobs_flag     => all jobs of all users given in 
                                             <user_list>
   5) user_list_flag && jid_flag          => not valid
   6) user_list_flag                      => all jobs of all users given in 
                                             <user_list>
   7) all_jobs_flag                       => all jobs of current user
   8) jid_flag                            => <jid>
   9) all_users_flag && user_list_flag    => not valid
*/                                             

static int verify_job_list_filter(
lList **alpp,
int all_users_flag,
int all_jobs_flag,
int jid_flag,
int user_list_flag,
char *ruser 
) {
   DENTER(TOP_LAYER, "verify_job_list_filter");

   /* Reject incorrect requests */
   if (!all_users_flag && !all_jobs_flag && !jid_flag && !user_list_flag) {
      ERROR((SGE_EVENT, MSG_SGETEXT_SPECIFYUSERORJID));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* case 9 */
   if (all_users_flag && user_list_flag) {
      ERROR((SGE_EVENT, MSG_SGETEXT_SPECIFYONEORALLUSER));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* case 2,5 */
   if ((all_users_flag || user_list_flag) && jid_flag) {
      ERROR((SGE_EVENT, MSG_SGETEXT_NOTALLOWEDTOSPECUSERANDJID));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }                            

   /* case 1,3: Only manager can modify all jobs of all users */
   if (all_users_flag && !jid_flag && !(sge_manager(ruser) == 0)) {
      ERROR((SGE_EVENT, MSG_SGETEXT_MUST_BE_MGR_TO_SS, ruser, 
             "modify all jobs"));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   DEXIT;
   return 0;
}

static void get_rid_of_schedd_job_messages(
u_long32 job_number 
) {
   lListElem *sme, *mes, *next;
   lList *mes_list;

   DENTER(TOP_LAYER, "get_rid_of_schedd_job_messages");
   if (Master_Job_Schedd_Info_List) {
      sme = lFirst(Master_Job_Schedd_Info_List);
      mes_list = lGetList(sme, SME_message_list);

      /*
      ** remove all messages for job_number
      */
      next = lFirst(mes_list);
      while((mes = next)) {
         lListElem *job_ulng, *nxt_job_ulng;
         next = lNext(mes);
         
         nxt_job_ulng = lFirst(lGetList(mes, MES_job_number_list));
         while ((job_ulng = nxt_job_ulng)) {
            nxt_job_ulng = lNext(nxt_job_ulng);    

            if (lGetUlong(job_ulng, ULNG) == job_number) {
               /* 
               ** more than one job in list for this message => remove job id
               ** else => remove whole message 
               */
               if (lGetNumberOfElem(lGetList(mes, MES_job_number_list)) > 1) {
                  lRemoveElem(lGetList(mes, MES_job_number_list), job_ulng);
                  DPRINTF(("Removed jobid "u32" from list of scheduler messages\n", job_number));
               } else {
                  lRemoveElem(mes_list, mes);
                  DPRINTF(("Removed message from list of scheduler messages "u32"\n", job_number));
               }
            }
         }
      }
   }
   DEXIT;
}

void job_ja_task_send_abort_mail(const lListElem *job, 
                                 const lListElem *ja_task,
                                 const char *ruser,
                                 const char *rhost,
                                 const char *err_str)
{
   char subject[1024];
   char body[4*1024];
   lList *users = NULL;
   u_long32 job_id;
   u_long32 ja_task_id;
   const char *job_name = NULL;
   int send_abort_mail = 0;

   ja_task_id = lGetUlong(ja_task, JAT_task_number);
   job_name = lGetString(job, JB_job_name);
   job_id = lGetUlong(job, JB_job_number);
   users = lGetList(job, JB_mail_list);
   send_abort_mail = VALID(MAIL_AT_ABORT, lGetUlong(job, JB_mail_options))
                     && !(lGetUlong(ja_task, JAT_state) & JDELETED);

   if (send_abort_mail) {
      if (is_array(job)) {
         /* CR: don't localize mail subject, until we send it in Mime format!
          *     The message definition is not l10n'ed (no _() macro used)!!!        
          */
         sprintf(subject, MSG_MAIL_TASKKILLEDSUBJ_UUS,
                 u32c(job_id), u32c(ja_task_id), job_name);
         sprintf(body, MSG_MAIL_TASKKILLEDBODY_UUSSS,
                 u32c(job_id), u32c(ja_task_id), job_name, ruser, rhost);
      } else {    
         /* CR: don't localize mail subject, until we send it in Mime format!
          *     The message definition is not l10n'ed (no _() macro used)!!!        
          */
         sprintf(subject, MSG_MAIL_JOBKILLEDSUBJ_US,
                 u32c(job_id), job_name);
         sprintf(body, MSG_MAIL_JOBKILLEDBODY_USSS,
                 u32c(job_id), job_name, ruser, rhost);
      }
      if (err_str != NULL) {
         strcat(body, MSG_MAIL_BECAUSE);
         strcat(body, err_str);
      }
      cull_mail(users, subject, body, MSG_MAIL_TYPE_ABORT);
   }  
}

void get_rid_of_job_due_to_qdel(lListElem *j,
                                lListElem *t,
                                lList **answer_list,
                                const char *ruser,
                                int force)
{
   u_long32 job_number, task_number;
   lListElem *qep = NULL;

   DENTER(TOP_LAYER, "get_rid_of_job_due_to_qdel");

   job_number = lGetUlong(j, JB_job_number);
   task_number = lGetUlong(t, JAT_task_number);
   qep = sge_locate_queue(lGetString(t, JAT_master_queue));
   if (!qep) {
      ERROR((SGE_EVENT, MSG_JOB_UNABLE2FINDQOFJOB_S, 
             lGetString(t, JAT_master_queue)));
      sge_add_answer(answer_list, SGE_EVENT, STATUS_EEXIST, 0); 
   }
   if (sge_signal_queue(SGE_SIGKILL, qep, j, t)) {
      if (force) {
         if (is_array(j)) {
            ERROR((SGE_EVENT, MSG_JOB_FORCEDDELTASK_SUU, 
                   ruser, u32c(job_number), u32c(task_number)));
         } else {
            ERROR((SGE_EVENT, MSG_JOB_FORCEDDELJOB_SU, 
                   ruser, u32c(job_number)));
         }
         sge_commit_job(j, t, 3, COMMIT_DEFAULT);
         cancel_job_resend(job_number, task_number);
         j = NULL;
         sge_add_answer(answer_list, SGE_EVENT, STATUS_OK, NUM_AN_INFO); 
      } else {
         ERROR((SGE_EVENT, MSG_COM_NOSYNCEXECD_SU, 
                ruser, u32c(job_number)));
         sge_add_answer(answer_list, SGE_EVENT, STATUS_EEXIST, 0); 
      }
   } else {
      if (force) {
         if (is_array(j)) {
            ERROR((SGE_EVENT, MSG_JOB_FORCEDDELTASK_SUU, 
                   ruser, u32c(job_number), u32c(task_number)));
         } else {
            ERROR((SGE_EVENT, MSG_JOB_FORCEDDELJOB_SU, 
                   ruser, u32c(job_number)));
         }
         sge_commit_job(j, t, 3, COMMIT_DEFAULT);
         cancel_job_resend(job_number, task_number);
         j = NULL;
      } else {
         /*
          * the job gets registered for deletion:
          * 0. send signal to execd
          * 1. JB_pending_signal = SGE_SIGKILL
          * 2. ACK from execd resets JB_pending_signal to 0
          *    Here we need a state for the job displaying its
          *    pending deletion
          * 3. execd signals shepherd and reaps job after job exit
          * 4. execd informs master of job exits and job is
          *    deleted from master lists
          */

         if (is_array(j)) {
            INFO((SGE_EVENT, MSG_JOB_REGDELTASK_SUU, 
                  ruser, u32c(job_number), u32c(task_number)));
         } else {
            INFO((SGE_EVENT, MSG_JOB_REGDELJOB_SU, 
                  ruser, u32c(job_number)));
         }
      }
      sge_add_answer(answer_list, SGE_EVENT, STATUS_OK, NUM_AN_INFO); 
   }
   job_mark_job_as_deleted(j, t);
}

void job_mark_job_as_deleted(lListElem *j,
                             lListElem *t)
{
   DENTER(TOP_LAYER, "job_mark_job_as_deleted");
   if (j && t) {
      u_long32 state = lGetUlong(t, JAT_state);

      SETBIT(JDELETED, state);
      lSetUlong(t, JAT_state, state);
      job_write_spool_file(j, lGetUlong(t, JAT_task_number), SPOOL_DEFAULT);
   } 
   DEXIT;
}

/*-------------------------------------------------------------------------*/
/* sge_gdi_modify_job                                                    */
/*    called in sge_c_gdi_mod                                              */
/*-------------------------------------------------------------------------*/

/* 
   this is our strategy:

   do common checks and search old job
   make a copy of the old job (this will be the new job)
   modify new job using reduced job as instruction
      on error: dispose new job
   store new job to disc
      on error: dispose new job
   create events
   replace old job by new job
*/

/* actions to be done after successful 
saving to disk of a modified job */
enum { 
   MOD_EVENT = 1, 
   PRIO_EVENT = 2, 
   RECHAIN_JID_HOLD = 4,
   VERIFY_EVENT = 8 
};

int sge_gdi_mod_job(
lListElem *jep, /* reduced JB_Type */
lList **alpp,
char *ruser,
char *rhost,
int sub_command 
) {
   lListElem *nxt, *jobep = NULL;   /* pointer to old job */
   int job_id_pos;
   int user_list_pos;
   lCondition *where = NULL;
   int user_list_flag;
   int njobs = 0, ret, jid_flag;
   int all_jobs_flag;
   int all_users_flag;
 
   DENTER(TOP_LAYER, "sge_gdi_mod_job");

   if ( !jep || !ruser || !rhost ) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* sub-commands */
   all_jobs_flag = ((sub_command & SGE_GDI_ALL_JOBS) > 0);
   all_users_flag = ((sub_command & SGE_GDI_ALL_USERS) > 0); 

   /* Did we get a user list? */
   if (((user_list_pos = lGetPosViaElem(jep, JB_user_list)) >= 0) 
       && lGetNumberOfElem(lGetPosList(jep, user_list_pos)) > 0)
      user_list_flag = 1;
   else
      user_list_flag = 0;

   /* Did we get a job - with a jobid? */
   if (((job_id_pos = lGetPosViaElem(jep, JB_job_number)) >= 0) && 
       lGetPosUlong(jep, job_id_pos) > 0) { 
      jid_flag = 1; 
   } else
      jid_flag = 0;

   if ((ret=verify_job_list_filter(alpp, all_users_flag, all_jobs_flag, 
         jid_flag, user_list_flag, ruser))) {
      DEXIT;
      return ret;
   }    

   where = job_list_filter(
      user_list_flag, user_list_flag?lGetPosList(jep, user_list_pos):NULL,  
      jid_flag, jid_flag?lGetPosUlong(jep, job_id_pos):0,/* job id    */
      all_users_flag,                           /* all jobs  */
      all_jobs_flag,                            /* all jobs in user list */         ruser); 
   
   nxt = lFirst(Master_Job_List);
   while ((jobep=nxt)) {  
      u_long32 jobid = 0;
      lListElem *new_job;        /* new job */
      lList *tmp_alp = NULL;
      lListElem *jatep;
      int trigger = 0;
      nxt = lNext(jobep);

      if (!lCompare(jobep, where)) {
         continue;
      }

      njobs++;
      jobid = lGetUlong(jobep, JB_job_number);

      /* general check whether ruser is allowed to modify this job */
      if (strcmp(ruser, lGetString(jobep, JB_owner)) && sge_operator(ruser) && sge_manager(ruser)) {
         ERROR((SGE_EVENT, MSG_SGETEXT_MUST_BE_JOB_OWN_TO_SUS, ruser, u32c(jobid), MSG_JOB_CHANGEATTR));  
         sge_add_answer(alpp, SGE_EVENT, STATUS_ENOTOWNER, 0);
         lFreeWhere(where);
         DEXIT;
         return STATUS_ENOTOWNER;   
      }
    
      /* operate on a cull copy of the job */ 
      new_job = lCopyElem(jobep);

      
      if (mod_job_attributes(new_job, jep, &tmp_alp, ruser, rhost, &trigger)) {
         /* failure: just append last elem in tmp_alp 
            elements before may contain invalid success messages */ 
         lListElem *failure;
         failure = lLast(tmp_alp);
         lDechainElem(tmp_alp, failure);
         if (!*alpp)
            *alpp = lCreateList("answer", AN_Type);
         lAppendElem(*alpp, failure);
         lFreeList(tmp_alp);
         lFreeElem(new_job);

         DPRINTF(("---------- removed messages\n"));
         lFreeWhere(where);
         DEXIT;
         return STATUS_EUNKNOWN;
      }

      if (!(trigger & VERIFY_EVENT)) {

         if (trigger & MOD_EVENT)
            lSetUlong(new_job, JB_version, lGetUlong(new_job, JB_version)+1);

         /* all job modifications to be saved on disk must be made in new_job */
         if (job_write_spool_file(new_job, 0, SPOOL_DEFAULT)) {
            ERROR((SGE_EVENT, MSG_JOB_NOALTERNOWRITE_U, u32c(jobid)));
            sge_add_answer(alpp, SGE_EVENT, STATUS_EDISK, 0);
            lFreeList(tmp_alp);
            lFreeElem(new_job);
            lFreeWhere(where); 
            DEXIT;
            return STATUS_EDISK;
         }

         /* all elems in tmp_alp need to be appended to alpp */
         if (!*alpp)
            *alpp = lCreateList("answer", AN_Type);
         lAddList(*alpp, tmp_alp);

         if (trigger & MOD_EVENT) {
            sge_add_job_event(sgeE_JOB_MOD, new_job, 0);
            for_each(jatep, lGetList(new_job, JB_ja_tasks)) {
               sge_add_jatask_event(sgeE_JATASK_MOD, new_job, jatep);
            }
         }
         if (trigger & PRIO_EVENT)
            sge_add_event(NULL, sgeE_JOB_MOD_SCHED_PRIORITY, jobid, 0, NULL, jep);

         /* remove all existing trigger links - 
            this has to be done using the old 
            jid_predecessor_list */

         if (trigger & RECHAIN_JID_HOLD) {
            lListElem *suc_jobep, *jid;
            for_each(jid, lGetList(jobep, JB_jid_predecessor_list)) {
               const char *pre_ident = lGetString(jid, JRE_job_name);

               DPRINTF((" JOB #"u32": P: %s\n", jobid, pre_ident)); 

               if ((suc_jobep = locate_job_by_identifier(pre_ident, lGetString(jobep, JB_owner)))) {
                  DPRINTF(("  JOB "u32" removed from trigger "
                     "list of job %s\n", jobid, lGetString(jid, JRE_job_name)));
                  lRemoveElem(lGetList(suc_jobep, JB_jid_sucessor_list), 
                     lGetElemUlong(lGetList(suc_jobep, JB_jid_sucessor_list), JRE_job_number, jobid));
               } 
            }
         }

         /* write data back into job list  */
         {
            lListElem *prev = lPrev(jobep);
            lRemoveElem(Master_Job_List, jobep);
            lInsertElem(Master_Job_List, prev, new_job);
         }   
         /* no need to spool these mods */
         if (trigger & RECHAIN_JID_HOLD) 
            job_suc_pre(new_job);

         INFO((SGE_EVENT, MSG_SGETEXT_MODIFIEDINLIST_SSUS, ruser, 
               rhost, u32c(jobid), MSG_JOB_JOB));
      }
   }
   lFreeWhere(where);

   if (!njobs) {
      empty_job_list_filter(alpp, 1, user_list_flag,
            user_list_flag?lGetPosList(jep, user_list_pos):NULL,
            jid_flag, jid_flag?lGetPosUlong(jep, job_id_pos):0,
            all_users_flag, all_jobs_flag, ruser, 0, 0, 0, 0);
      DEXIT;
      return STATUS_EEXIST;
   }   


   DEXIT;
   return STATUS_OK;
}

void sge_add_job_event(
u_long32 type,
lListElem *jep,
lListElem *jatask 
) {
   DENTER(TOP_LAYER, "sge_add_job_event");
   sge_add_event(NULL, type, lGetUlong(jep, JB_job_number), 
                jatask?lGetUlong(jatask, JAT_task_number):0, 
                NULL, jep);
   DEXIT;
   return;
}

void sge_add_jatask_event(
u_long32 type,
lListElem *jep,
lListElem *jatask 
) {           
   DENTER(TOP_LAYER, "sge_add_jatask_event");
   sge_add_event(NULL, type, lGetUlong(jep, JB_job_number), 
                  lGetUlong(jatask, JAT_task_number),
                  NULL, jatask);
   DEXIT;
   return;  
}        

lListElem *locate_job_by_identifier(const char *pre_ident, const char *owner)
{
   lListElem *ret = NULL;

   DENTER(TOP_LAYER, "locate_job_by_identifier");
   if (isdigit(pre_ident[0])) {
      ret = sge_locate_job(atoi(pre_ident));
   } else {
      lListElem *user_job = NULL;         /* JB_Type */
      lListElem *next_user_job = NULL;    /* JB_Type */
      const void *user_iterator = NULL;
 
      next_user_job = lGetElemStrFirst(Master_Job_List, JB_owner, 
                                       owner, &user_iterator);
DTRACE;
      while ((user_job = next_user_job)) {
         const char *job_name = lGetString(user_job, JB_job_name);
DTRACE;

         next_user_job = lGetElemStrNext(Master_Job_List, JB_owner, 
                                         owner, &user_iterator);     
         if (!strcmp(job_name, pre_ident)) {
            ret = user_job;
            break;
         }
      }
   }
   DEXIT;
   return ret;
}

/* 
   build up jid hold links for a job 
   no need to spool them or to send
   events to update schedd data 
*/
void job_suc_pre(
lListElem *jep 
) {
   lListElem *parent_jep, *prep, *task;

   DENTER(TOP_LAYER, "job_suc_pre");

   /* 
      here we check whether every job 
      in the predecessor list has exited
   */
   prep = lFirst(lGetList(jep, JB_jid_predecessor_list));
   while (prep) {
      const char *pre_ident = lGetString(prep, JRE_job_name);

      parent_jep = locate_job_by_identifier(pre_ident, lGetString(jep, JB_owner));

      if (parent_jep) {
         int Exited = 1;
         lListElem *ja_task;

         if (lGetList(parent_jep, JB_ja_n_h_ids) != NULL ||
             lGetList(parent_jep, JB_ja_u_h_ids) != NULL ||
             lGetList(parent_jep, JB_ja_o_h_ids) != NULL ||
             lGetList(parent_jep, JB_ja_s_h_ids) != NULL) {
            Exited = 0;
         }
         if (Exited) {
            for_each(ja_task, lGetList(parent_jep, JB_ja_tasks)) {
               if (lGetUlong(ja_task, JAT_status) != JFINISHED) {
                  Exited = 0;
                  break;
               }
               for_each(task, lGetList(ja_task, JAT_task_list)) {
                  if (lGetUlong(lFirst(lGetList(task, JB_ja_tasks)), JAT_status)
                        !=JFINISHED) {
                     /* at least one task exists */
                     Exited = 0;
                     break;
                  }
               }
               if (!Exited)
                  break;
            }
         }
         if (!Exited) {
            DPRINTF(("adding jid "u32" into sucessor list of job %s\n",
               lGetUlong(jep, JB_job_number), pre_ident));

            /* add jid to sucessor_list of parent job */
            lAddSubUlong(parent_jep, JRE_job_number, lGetUlong(jep, JB_job_number), 
               JB_jid_sucessor_list, JRE_Type);
            
            prep = lNext(prep);
            
         } else {
            DPRINTF(("job %s from predecessor list already exited - ignoring it\n", 
                  pre_ident));

            prep = lNext(prep);      
            lDelSubStr(jep, JRE_job_name, pre_ident, JB_jid_predecessor_list);
         }
      } else {
         DPRINTF(("predecessor job %s does not exist\n", pre_ident));
         prep = lNext(prep);
         lDelSubStr(jep, JRE_job_name, pre_ident, JB_jid_predecessor_list);
      }
   }
   DEXIT;
   return;
}

/* handle all per task attributes which are changeable 
   from outside using gdi requests 

   job - the job
   new_ja_task - new task structure DST; may be NULL for not enrolled tasks
                 (not dispatched)
   tep  - reduced task element SRC
*/
static int mod_task_attributes(
lListElem *job,         
lListElem *new_ja_task,
lListElem *tep,      
lList **alpp,
char *ruser, 
char *rhost,
int *trigger, 
int is_array,
int is_task_enrolled 
) {
   u_long32 jobid = lGetUlong(job, JB_job_number);
   u_long32 jataskid = lGetUlong(new_ja_task, JAT_task_number);
   int pos;
   
   DENTER(TOP_LAYER, "mod_task_attributes");

   if (is_task_enrolled) {

      /* --- JAT_fshare */
      if ((pos=lGetPosViaElem(tep, JAT_fshare))>=0) {
         u_long32 uval; 

         /* need to be operator */
         if (sge_operator(ruser)) {
            ERROR((SGE_EVENT, MSG_SGETEXT_MUST_BE_OPR_TO_SS, ruser, 
                   MSG_JOB_CHANGESHAREFUNC));
            sge_add_answer(alpp, SGE_EVENT, STATUS_ENOOPR, 0);
            DEXIT;
            return STATUS_ENOOPR;   
         }
         uval = lGetPosUlong(tep, pos);
         if (uval != lGetUlong(new_ja_task, JAT_fshare)) {
            lSetUlong(new_ja_task, JAT_fshare, uval);
            DPRINTF(("JAT_fshare = "u32"\n", uval));
            *trigger |= MOD_EVENT;
         }

         sprintf(SGE_EVENT, MSG_JOB_SETSHAREFUNC_SSUUU,
                 ruser, rhost, u32c(jobid), u32c(jataskid), u32c(uval));
         sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
      }

   }

   /* --- JAT_hold */
   if ((pos=lGetPosViaElem(tep, JAT_hold))>=0) {
      u_long32 op_code_and_hold = lGetPosUlong(tep, pos); 
      u_long32 op_code = op_code_and_hold & ~MINUS_H_TGT_ALL;
      u_long32 target = op_code_and_hold & MINUS_H_TGT_ALL;
      int is_sub_op_code = (op_code == MINUS_H_CMD_SUB); 
      u_long32 old_hold = job_get_hold_state(job, jataskid);
      u_long32 new_hold; 
    
      if (!is_task_enrolled) { 
         new_ja_task = job_get_ja_task_template_pending(job, jataskid);
      }

      if ((target & MINUS_H_TGT_SYSTEM) != (old_hold & MINUS_H_TGT_SYSTEM)) {
         if (sge_manager(ruser)) {
            ERROR((SGE_EVENT, MSG_SGETEXT_MUST_BE_MGR_TO_SS, ruser, 
                  is_sub_op_code ? MSG_JOB_RMHOLDMNG : MSG_JOB_SETHOLDMNG)); 
            sge_add_answer(alpp, SGE_EVENT, STATUS_ENOOPR, 0);
            DEXIT;
            return STATUS_ENOOPR;   
         }
      }
      if ((target & MINUS_H_TGT_OPERATOR) !=
            (old_hold & MINUS_H_TGT_OPERATOR)) {
         if (sge_operator(ruser)) {
            ERROR((SGE_EVENT, MSG_SGETEXT_MUST_BE_OPR_TO_SS, ruser, 
                   is_sub_op_code ? MSG_JOB_RMHOLDOP : MSG_JOB_SETHOLDOP));  
            sge_add_answer(alpp, SGE_EVENT, STATUS_ENOOPR, 0);
            DEXIT;
            return STATUS_ENOOPR;   
         }
      }
      if ((target & MINUS_H_TGT_USER) != (old_hold & MINUS_H_TGT_USER)) {
         if (strcmp(ruser, lGetString(job, JB_owner)) && 
             sge_operator(ruser)) {
            ERROR((SGE_EVENT, MSG_SGETEXT_MUST_BE_JOB_OWN_TO_SUS, ruser, 
                   u32c(jobid), is_sub_op_code ? 
                   MSG_JOB_RMHOLDUSER : MSG_JOB_SETHOLDUSER));  
            sge_add_answer(alpp, SGE_EVENT, STATUS_ENOOPR, 0);
            DEXIT;
            return STATUS_ENOOPR;   
         } 
      }

      switch (op_code) {
         case MINUS_H_CMD_SUB:
            new_hold = old_hold & ~target;
            DPRINTF(("MINUS_H_CMD_SUB = "u32"\n", new_hold));
            break;
         case MINUS_H_CMD_ADD:
            new_hold = old_hold | target;
            DPRINTF(("MINUS_H_CMD_ADD = "u32"\n", new_hold));
            break;
         case MINUS_H_CMD_SET:
            new_hold = target;
            DPRINTF(("MINUS_H_CMD_SET = "u32"\n", new_hold));
            break;
         default:
            new_hold = old_hold;
            DPRINTF(("MINUS_H_CMD_[default] = "u32"\n", new_hold));
            break;
      }

      job_set_hold_state(job, NULL, jataskid, new_hold); 
      *trigger |= MOD_EVENT;
   
      if (new_hold != old_hold) { 
         if (is_array) { 
            sprintf(SGE_EVENT, MSG_SGETEXT_MOD_JATASK_SUU, MSG_JOB_HOLD, 
                    u32c(jobid), u32c(jataskid));
         } else {
            sprintf(SGE_EVENT, MSG_SGETEXT_MOD_JOBS_SU, MSG_JOB_HOLD, 
                    u32c(jobid));
         }
         sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
      }
   }

   DEXIT;
   return 0;
}

/****** sge_job/changes_consumables() ******************************************
*  NAME
*     changes_consumables() -- detect changes with consumable resource request
*
*  SYNOPSIS
*     static int changes_consumables(lList* new, lList* old) 
*
*  INPUTS
*     lList** alpp - answer list pointer pointer
*     lList* new - jobs new JB_hard_resource_list
*     lList* old - jobs old JB_hard_resource_list
*
*  RESULT
*     static int - 0     nothing changed 
*                  other it changed 
*******************************************************************************/
static int changes_consumables(lList **alpp, lList* new, lList* old)
{
   lListElem *old_reqep, *dcep, *new_reqep, *new_entry;
   lListElem *old_entry = NULL;
   const char *name;
   int found_it;

   DENTER(TOP_LAYER, "changes_consumables");

   /* ensure all old resource requests implying consumables 
      debitation are still contained in new resource request list */
   for_each(old_reqep, old) {
      for_each(old_entry, lGetList(old_reqep, RE_entries)) { 
         name = lGetString(old_entry, CE_name);

         if (!(dcep = sge_locate_complex_attr(name, Master_Complex_List))) {
            /* complex attribute definition has been removed though
               job still requests resource */ 
            ERROR((SGE_EVENT, MSG_ATTRIB_MISSINGATTRIBUTEXINCOMPLEXES_S , name));
            sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
            DEXIT;
            return -1; 
         }

         /* ignore non-consumables */
         if (!lGetUlong(dcep, CE_consumable))
            continue;
 
         /* search it in new hard resource list */
         found_it = 0;
         for_each (new_reqep, new) {
            if (lGetSubStr(new_reqep, CE_name, name, RE_entries)) {
               found_it = 1;
               break;
            }
         }

         if (!found_it) {
            ERROR((SGE_EVENT, MSG_JOB_MOD_MISSINGRUNNINGJOBCONSUMABLE_S, name));
            sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
            DEXIT;
            return -1;
         }
      }
   }
   
   /* ensure all new resource requests implying consumable 
      debitation were also contained in old resource request list
      AND have not changed the requested amount */ 
   for_each(new_reqep, new) {
      for_each(new_entry, lGetList(new_reqep, RE_entries)) { 
         name = lGetString(new_entry, CE_name);

         if (!(dcep = sge_locate_complex_attr(name, Master_Complex_List))) {
            /* refers to a not existing complex attribute definition */ 
            ERROR((SGE_EVENT, MSG_ATTRIB_MISSINGATTRIBUTEXINCOMPLEXES_S , name));
            sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
            DEXIT;
            return -1; 
         }

         /* ignore non-consumables */
         if (!lGetUlong(dcep, CE_consumable))
            continue;

         /* search it in old hard resource list */
         found_it = 0;
         for_each (old_reqep, old) {
            if ((old_entry=lGetSubStr(old_reqep, CE_name, name, RE_entries))) {
               found_it = 1;
               break;
            }
         }

         if (!found_it) {
            ERROR((SGE_EVENT, MSG_JOB_MOD_ADDEDRUNNINGJOBCONSUMABLE_S, name));
            sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
            DEXIT;
            return -1;
         }

         /* compare request in old_entry with new_entry */
         DPRINTF(("request: \"%s\" old: %f new: %f\n", name, 
            lGetDouble(old_entry, CE_doubleval), 
            lGetDouble(new_entry, CE_doubleval)));
         if (lGetDouble(old_entry, CE_doubleval) != 
            lGetDouble(new_entry, CE_doubleval)) {
            ERROR((SGE_EVENT, MSG_JOB_MOD_CHANGEDRUNNINGJOBCONSUMABLE_S, name));
            sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
            DEXIT;
            return -1;
         }
      }
   }

   DEXIT;
   return 0;
}

/****** sge_job/deny_soft_consumables() ****************************************
*  NAME
*     deny_soft_consumables() -- Deny soft consumables
*
*  SYNOPSIS
*     static int deny_soft_consumables(lList **alpp, lList *srl) 
*
*  FUNCTION
*     Find out if consumables are requested and deny them.
*
*  INPUTS
*     lList** alpp - answer list pointer pointer
*     lList *srl   - jobs JB_soft_resource_list
*
*  RESULT
*     static int - 0 request can pass
*                !=0 consumables requested soft
*
*******************************************************************************/
static int deny_soft_consumables(lList **alpp, lList *srl)
{
   lListElem *reqep, *entry, *dcep;
   const char *name;

   DENTER(TOP_LAYER, "deny_soft_consumables");

   /* ensure no consumables are requested in JB_soft_resource_list */
   for_each(reqep, srl) {
      for_each(entry, lGetList(reqep, RE_entries)) { 
         name = lGetString(entry, CE_name);

         if (!(dcep = sge_locate_complex_attr(name, Master_Complex_List))) {
            ERROR((SGE_EVENT, MSG_ATTRIB_MISSINGATTRIBUTEXINCOMPLEXES_S , name));
            sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
            DEXIT;
            return -1; 
         }

         /* ignore non-consumables */
         if (lGetUlong(dcep, CE_consumable)) {
            ERROR((SGE_EVENT, MSG_JOB_MOD_SOFTREQCONSUMABLE_S, name));
            sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
            DEXIT;
            return -1; 
         }
      }
   }

   DEXIT;
   return 0;
}



static int mod_job_attributes(
lListElem *new_job,            /* new job */
lListElem *jep,                /* reduced job element */
lList **alpp,
char *ruser, 
char *rhost,
int *trigger  
) {
   int pos;
   int is_running = 0, may_not_be_running = 0; 
   u_long32 uval;
   u_long32 jobid = lGetUlong(new_job, JB_job_number);

   DENTER(TOP_LAYER, "mod_job_attributes");

  
   /* is job running ? */
   {
      lListElem *ja_task;
      for_each(ja_task, lGetList(new_job, JB_ja_tasks)) {
         if (lGetUlong(ja_task, JAT_status) & JTRANSITING || 
             lGetUlong(ja_task, JAT_status) & JRUNNING) {
            is_running = 1;
         }
      }
   }

   /* 
    * ---- JB_ja_tasks
    *      Do we have per task change request? 
    */
   if ((pos=lGetPosViaElem(jep, JB_ja_tasks))>=0) {
      lList *ja_task_list = lGetPosList(jep, pos);
      lListElem *ja_task = lFirst(ja_task_list);
      int new_job_is_array = is_array(new_job);
      u_long32 jep_ja_task_number = lGetNumberOfElem(ja_task_list);
   
      /* 
       * Is it a valid per task request:
       *    - at least one task element 
       *    - task id field 
       *    - multi tasks requests are only valid for array jobs 
       */
      if (!ja_task) {
         ERROR((SGE_EVENT, MSG_SGETEXT_NEEDONEELEMENT_SS,
               lNm2Str(JB_ja_tasks), SGE_FUNC));
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         DEXIT;
         return STATUS_EUNKNOWN;
      }
      if ((pos = lGetPosViaElem(ja_task, JAT_task_number)) < 0) {
         ERROR((SGE_EVENT, MSG_SGETEXT_MISSINGCULLFIELD_SS,
               lNm2Str(JAT_task_number), SGE_FUNC));
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         DEXIT;
         return STATUS_EUNKNOWN;
      }
      if (!new_job_is_array && jep_ja_task_number > 1) { 
         ERROR((SGE_EVENT, MSG_JOB_NOJOBARRAY_U, u32c(jobid)));
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         DEXIT;
         return STATUS_EUNKNOWN;
      }    

      /* 
       * Visit tasks
       */
      if (ja_task_list != NULL) {
         lListElem *first = lFirst(ja_task_list);
         u_long32 handle_all_tasks = !lGetUlong(first, JAT_task_number);

         if (handle_all_tasks) {
            int list_id[] = {JB_ja_n_h_ids, JB_ja_u_h_ids, JB_ja_o_h_ids,
                             JB_ja_s_h_ids, -1};
            lListElem *dst_ja_task = NULL;
            int i = -1;

            /*
             * Visit all unenrolled tasks
             */
            while(list_id[++i] != -1) {
               lList *range_list = 
                              lCopyList("", lGetList(new_job, list_id[i]));
               lListElem *range = NULL;
               u_long32 id;
 
               for_each(range, range_list) {
                  lWriteElemTo(range, stderr);
                  for(id = lGetUlong(range, RN_min); 
                      id <= lGetUlong(range, RN_max); 
                      id += lGetUlong(range, RN_step)) {

                     dst_ja_task = 
                            job_get_ja_task_template_pending(new_job, id);

                     mod_task_attributes(new_job, dst_ja_task, ja_task, 
                                         alpp, ruser, rhost, trigger, 
                                         is_array(new_job), 0);
                  }
               }
               range_list = lFreeList(range_list);
            }
            /*
             * Visit enrolled tasks
             */
            for_each (dst_ja_task, lGetList(new_job, JB_ja_tasks)) {
               mod_task_attributes(new_job, dst_ja_task, ja_task, alpp,
                                   ruser, rhost, trigger, 
                                   is_array(new_job), 1);
            }
         } else {
            for_each (ja_task, ja_task_list) {
               u_long32 ja_task_id = lGetUlong(ja_task, JAT_task_number);
               int is_defined = job_is_ja_task_defined(new_job, ja_task_id);

               if (is_defined) {
                  lListElem *dst_ja_task = NULL;
                  int is_enrolled = 1;

                  dst_ja_task = job_search_task(new_job, NULL, ja_task_id, 0);
                  if (dst_ja_task == NULL) {
                     is_enrolled = 0;
                     dst_ja_task = 
                         job_get_ja_task_template_pending(new_job, 
                                                          ja_task_id); 
                  }
                  mod_task_attributes(new_job, dst_ja_task, ja_task, alpp,
                                      ruser, rhost, trigger, 
                                      is_array(new_job), is_enrolled);
               } else {
                  ; /* Ignore silently */
               }
            }
         }
      }
   }

   /* ---- JB_override_tickets 
           A attribute that must be allowed to 
           be changed when job is running
   */
   if ((pos=lGetPosViaElem(jep, JB_override_tickets))>=0) {
      uval=lGetPosUlong(jep, pos);

      /* need to be operator */
      if (sge_operator(ruser)) {
         ERROR((SGE_EVENT, MSG_SGETEXT_MUST_BE_OPR_TO_SS, ruser, 
               MSG_JOB_CHANGEOVERRIDETICKS));  
         sge_add_answer(alpp, SGE_EVENT, STATUS_ENOOPR, 0);
         DEXIT;
         return STATUS_ENOOPR;   
      }

      /* ok, do it */
      if (uval!=lGetUlong(new_job, JB_override_tickets)) {
         lSetUlong(new_job, JB_override_tickets, uval);
         *trigger |= MOD_EVENT;
      }

      sprintf(SGE_EVENT, MSG_JOB_SETOVERRIDETICKS_SSUU,
               ruser, rhost, u32c(jobid), u32c(uval));
      sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
   }

   /* ---- JB_priority */
   if ((pos=lGetPosViaElem(jep, JB_priority))>=0) {
      u_long32 old_priority;
      uval=lGetPosUlong(jep, pos);
      if (uval > (old_priority=lGetUlong(new_job, JB_priority))) { 
         /* need to be at least operator */
         if (sge_operator(ruser)) {
            ERROR((SGE_EVENT, MSG_SGETEXT_MUST_BE_OPR_TO_SS, ruser, MSG_JOB_PRIOINC));
            sge_add_answer(alpp, SGE_EVENT, STATUS_ENOOPR, 0);
            DEXIT;
            return STATUS_ENOOPR;   
         }
      }
      /* ok, do it */
      if (uval!=old_priority) 
        *trigger |= PRIO_EVENT;

      lSetUlong(new_job, JB_priority, uval);

      sprintf(SGE_EVENT, MSG_JOB_PRIOSET_SSUI,
               ruser, rhost, u32c(jobid), (int)(uval-BASE_PRIORITY));
      sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);

   }

   /* ---- JB_deadline */
   /* If it is a deadline job the user has to be a deadline user */
   if ((pos=lGetPosViaElem(jep, JB_deadline))>=0) {
      if (!is_deadline_user(ruser, Master_Userset_List)) {
         ERROR((SGE_EVENT, MSG_JOB_NODEADLINEUSER_S, ruser));
         sge_add_answer(alpp, SGE_EVENT, STATUS_ENOOPR, 0);
         DEXIT;
         return STATUS_ENOOPR;
      } else {
         lSetUlong(new_job, JB_deadline, lGetUlong(jep, JB_deadline));
         *trigger |= MOD_EVENT;
         sprintf(SGE_EVENT, MSG_SGETEXT_MOD_JOBS_SU, MSG_JOB_DEADLINETIME, u32c(jobid)); 
         sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
      }
   }

   /* ---- JB_execution_time */
   if ((pos=lGetPosViaElem(jep, JB_execution_time))>=0) {
      DPRINTF(("got new JB_execution_time\n")); 
      lSetUlong(new_job, JB_execution_time, lGetUlong(jep, JB_execution_time));
      *trigger |= MOD_EVENT;
      sprintf(SGE_EVENT, MSG_SGETEXT_MOD_JOBS_SU, MSG_JOB_STARTTIME, u32c(jobid));
      sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
   }

   /* ---- JB_account */
   if ((pos=lGetPosViaElem(jep, JB_account))>=0) {
      DPRINTF(("got new JB_account\n")); 
      if (!job_has_valid_account_string(jep, alpp)) {
         return STATUS_EUNKNOWN;
      }
      lSetString(new_job, JB_account, lGetString(jep, JB_account));
      sprintf(SGE_EVENT, MSG_SGETEXT_MOD_JOBS_SU, MSG_JOB_ACCOUNT, u32c(jobid));
      sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
   }

   /* ---- JB_cwd */
   if ((pos=lGetPosViaElem(jep, JB_cwd))>=0) {
      DPRINTF(("got new JB_cwd\n")); 
      lSetString(new_job, JB_cwd, lGetString(jep, JB_cwd));
      sprintf(SGE_EVENT, MSG_SGETEXT_MOD_JOBS_SU, MSG_JOB_WD, u32c(jobid));
      sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
   }

   /* ---- JB_checkpoint_object */
   if ((pos=lGetPosViaElem(jep, JB_checkpoint_object))>=0) {
      const char *ckpt_name;

      DPRINTF(("got new JB_checkpoint_object\n")); 
      ckpt_name = lGetString(jep, JB_checkpoint_object);
      if (ckpt_name && !sge_locate_ckpt(ckpt_name)) {
         ERROR((SGE_EVENT, MSG_SGETEXT_DOESNOTEXIST_SS, 
            MSG_OBJ_CKPT, ckpt_name));
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         DEXIT;
         return STATUS_EUNKNOWN;
      }
      lSetString(new_job, JB_checkpoint_object, ckpt_name);
      *trigger |= MOD_EVENT;
      sprintf(SGE_EVENT, MSG_SGETEXT_MOD_JOBS_SU, MSG_OBJ_CKPT, u32c(jobid));
      sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
   }

   /* ---- JB_stderr_path_list */
   if ((pos=lGetPosViaElem(jep, JB_stderr_path_list))>=0) {
      int status;
      DPRINTF(("got new JB_stderr_path_list\n"));


      if( (status=job_resolve_host_for_path_list(jep, alpp, JB_stderr_path_list)) != STATUS_OK){
         DEXIT;
         return status;
      } 

 
      lSetList(new_job, JB_stderr_path_list, 
            lCopyList("", lGetList(jep, JB_stderr_path_list)));
      sprintf(SGE_EVENT, MSG_SGETEXT_MOD_JOBS_SU, MSG_JOB_STDERRPATHLIST, u32c(jobid));
      sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
   }

   /* ---- JB_merge_stderr */
   if ((pos=lGetPosViaElem(jep, JB_merge_stderr))>=0) {
      DPRINTF(("got new JB_merge_stderr\n"));
      lSetUlong(new_job, JB_merge_stderr, lGetUlong(jep, JB_merge_stderr));
      sprintf(SGE_EVENT, MSG_SGETEXT_MOD_JOBS_SU, MSG_JOB_MERGEOUTPUT, u32c(jobid));
      sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
   }

   /* ---- JB_hard_resource_list */
   if ((pos=lGetPosViaElem(jep, JB_hard_resource_list))>=0) {
      lListElem *reqep;
      DPRINTF(("got new JB_hard_resource_list\n")); 
      for_each(reqep, lGetList(jep, JB_hard_resource_list) ) {
         if (sge_fill_requests(lGetList(reqep, RE_entries), Master_Complex_List, 0, 1, 0)) {
            sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
            DEXIT;
            return STATUS_EUNKNOWN;
         }
      }
      if (compress_ressources(alpp, lGetList(jep,JB_hard_resource_list))) {
         DEXIT;
         return STATUS_EUNKNOWN;
      }

      /* to prevent inconsistent consumable mgmnt:
         - deny resource requests changes on consumables for running jobs (IZ #251)
         - a better solution is to store for each running job the amount of resources */
      if (is_running && changes_consumables(alpp, lGetList(jep, JB_hard_resource_list), 
            lGetList(new_job, JB_hard_resource_list))) {
         DEXIT;
         return STATUS_EUNKNOWN;   
      }

      lSetList(new_job, JB_hard_resource_list, 
            lCopyList("", lGetList(jep, JB_hard_resource_list)));
      *trigger |= MOD_EVENT;
      sprintf(SGE_EVENT, MSG_SGETEXT_MOD_JOBS_SU, MSG_JOB_HARDRESOURCELIST, u32c(jobid));
      sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
   }

   /* ---- JB_soft_resource_list */
   if ((pos=lGetPosViaElem(jep, JB_soft_resource_list))>=0) {
      lListElem *reqep;
      DPRINTF(("got new JB_soft_resource_list\n")); 
      for_each(reqep, lGetList(jep, JB_soft_resource_list) ) {
         if (sge_fill_requests(lGetList(reqep, RE_entries), Master_Complex_List, 0, 1, 0)) {
            sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
            DEXIT;
            return STATUS_EUNKNOWN;
         }
      }
      if (compress_ressources(alpp, lGetList(jep, JB_soft_resource_list))) {
         DEXIT;
         return STATUS_EUNKNOWN;
      }
      if (deny_soft_consumables(alpp, lGetList(jep, JB_soft_resource_list))) {
         DEXIT;
         return STATUS_EUNKNOWN;
      }
      lSetList(new_job, JB_soft_resource_list, 
               lCopyList("", lGetList(jep, JB_soft_resource_list)));
      *trigger |= MOD_EVENT;
      sprintf(SGE_EVENT, MSG_SGETEXT_MOD_JOBS_SU, MSG_JOB_SOFTRESOURCELIST, u32c(jobid));
      sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
   }

   /* ---- JB_mail_options */
   if ((pos=lGetPosViaElem(jep, JB_mail_options))>=0) {
      DPRINTF(("got new JB_mail_options\n")); 
      lSetUlong(new_job, JB_mail_options, lGetUlong(jep, JB_mail_options));
      sprintf(SGE_EVENT, MSG_SGETEXT_MOD_JOBS_SU, MSG_JOB_MAILOPTIONS, u32c(jobid));
      sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
   }

   /* ---- JB_mail_list */
   if ((pos=lGetPosViaElem(jep, JB_mail_list))>=0) {
      DPRINTF(("got new JB_mail_list\n")); 
      lSetList(new_job, JB_mail_list, 
               lCopyList("", lGetList(jep, JB_mail_list)));
      sprintf(SGE_EVENT, MSG_SGETEXT_MOD_JOBS_SU, MSG_JOB_MAILLIST, u32c(jobid));
      sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
   }

   /* ---- JB_job_name */
   if ((pos=lGetPosViaElem(jep, JB_job_name))>=0 && lGetString(jep, JB_job_name)) {
      u_long32 succ_jid;
      const char *new_name = lGetString(jep, JB_job_name);

      DPRINTF(("got new JB_job_name\n")); 

      /* preform checks only if job name _really_ changes */
      if (strcmp(new_name, lGetString(new_job, JB_job_name))) {
         char job_descr[100];
         const char *job_name;

         /* reject changing job name if at least one other job points to this job
            in it's -hold_jid list using the job name */
         if ((succ_jid = is_referenced_by_jobname(new_job))) {
            ERROR((SGE_EVENT, MSG_JOB_MOD_CHGJOBNAMEDESTROYSREF_UU,
                      u32c(jobid), u32c(succ_jid)));
            sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
            DEXIT;
            return STATUS_EUNKNOWN;
         }

         sprintf(job_descr, "job "u32, jobid);
         job_name = lGetString(new_job, JB_job_name);
         lSetString(new_job, JB_job_name, new_name);
         if (job_verify_name(new_job, alpp, job_descr)) {
            lSetString(new_job, JB_job_name, job_name);
            DEXIT;
            return STATUS_EUNKNOWN;
         }
      }
     
      *trigger |= MOD_EVENT;
      sprintf(SGE_EVENT, MSG_SGETEXT_MOD_JOBS_SU, MSG_JOB_JOBNAME, u32c(jobid));
      sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
   }

   /* ---- JB_jid_predecessor_list */
   if ((pos=lGetPosViaElem(jep, JB_jid_predecessor_list))>=0) {
      lList *new_pre_list, *exited_pre_list = NULL;
      lListElem *pre, *exited, *nxt, *job;

      DPRINTF(("got new JB_jid_predecessor_list\n"));

      if (job_verify_predecessors(new_job, alpp, 
                                  lGetList(jep, JB_jid_predecessor_list))) {
         DEXIT;
         return STATUS_EUNKNOWN;
      }

      new_pre_list = lCopyList("predecessor list", lGetList(jep, JB_jid_predecessor_list));

      /* remove jobid's of all no longer existing jobs from this
         new job - this must be done before event is sent to schedd */
      nxt = lFirst(new_pre_list);
      while ((pre=nxt)) {
         int move_to_exited = 0;
         const char *pre_ident = lGetString(pre, JRE_job_name);

         nxt = lNext(pre);
         DPRINTF(("jid: %s\n", pre_ident));

         job = locate_job_by_identifier(pre_ident, lGetString(new_job, JB_owner));

         /* in SGE jobs are exited when they dont exist */ 
         if (!job) {
            if (feature_is_enabled(FEATURE_SGEEE))
              move_to_exited = 1;
            else {
               /* in SGEEE the job may be still known by qmaster
                  and in state JFINISHED but it is exited */ 
               lListElem *ja_task; 

               if (job) {   /* yes, we have the job */
                  if (lGetList(job, JB_ja_tasks) == NULL ) {
                     move_to_exited = 1;
                     for_each(ja_task, lGetList(job, JB_ja_tasks)) {
                        if (lGetUlong(ja_task, JAT_status)!=JFINISHED) {
                           move_to_exited = 0;
                        }
                     } 
                  }
               } else {     /* job not found */
                 move_to_exited = 1; 
               }
            }
         }
         
         if (move_to_exited) {
            if (!exited_pre_list)
               exited_pre_list = lCreateList("exited list", JRE_Type);
            exited = lDechainElem(new_pre_list, pre);
            lAppendElem(exited_pre_list, exited);
         }
      }

      if (!lGetNumberOfElem(new_pre_list))
         new_pre_list = lFreeList(new_pre_list);
      
      *trigger |= (RECHAIN_JID_HOLD|MOD_EVENT);

      /* added primarily for own debugging purposes - andreas */
      {
         char str_predec[256], str_exited[256]; 
         const char *delis[] = {NULL, ",", ""};

         intprt_type fields[] = { JRE_job_name, 0 };
         uni_print_list(NULL, str_predec, sizeof(str_predec)-1, new_pre_list, fields, delis, 0);
         uni_print_list(NULL, str_exited, sizeof(str_exited)-1, exited_pre_list, fields, delis, 0);
         sprintf(SGE_EVENT, MSG_JOB_HOLDLISTMOD_USS, 
                    u32c(jobid), str_predec, str_exited);
         sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
      }  
      lSetList(new_job, JB_jid_predecessor_list, new_pre_list); 
      lFreeList(exited_pre_list);
   }

   /* ---- JB_notify */
   if ((pos=lGetPosViaElem(jep, JB_notify))>=0) {
      DPRINTF(("got new JB_notify\n")); 
      lSetUlong(new_job, JB_notify, lGetUlong(jep, JB_notify));
      *trigger |= MOD_EVENT;
      sprintf(SGE_EVENT, MSG_SGETEXT_MOD_JOBS_SU, MSG_JOB_NOTIFYBEHAVIOR, u32c(jobid));
      sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
   }

   /* ---- JB_stdout_path_list */
   if ((pos=lGetPosViaElem(jep, JB_stdout_path_list))>=0) {
   int status;
   DPRINTF(("got new JB_stdout_path_list\n"));


      if( (status=job_resolve_host_for_path_list(jep, alpp, JB_stdout_path_list)) != STATUS_OK){
         DEXIT;
         return status;
      }

 
      lSetList(new_job, JB_stdout_path_list, 
               lCopyList("", lGetList(jep, JB_stdout_path_list)));
      sprintf(SGE_EVENT, MSG_SGETEXT_MOD_JOBS_SU, MSG_JOB_STDOUTPATHLIST, u32c(jobid));
      sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
   }

   /* ---- JB_project */
   if ((pos=lGetPosViaElem(jep, JB_project))>=0) {
      const char *project;

      DPRINTF(("got new JB_project\n")); 
      
      project = lGetString(jep, JB_project);
      if (project && !sge_locate_user_prj(project, Master_Project_List)) {
         ERROR((SGE_EVENT, MSG_SGETEXT_DOESNOTEXIST_SS, MSG_JOB_PROJECT, project));
         sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
         DEXIT;
         return STATUS_EUNKNOWN;
      }
      if (!project && conf.enforce_project &&
          !strcasecmp(conf.enforce_project, "true")) {
         ERROR((SGE_EVENT, MSG_SGETEXT_NO_PROJECT));
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         DEXIT;
         return STATUS_EUNKNOWN;
      }
      lSetString(new_job, JB_project, project);
      may_not_be_running = 1;
      *trigger |= MOD_EVENT;
      sprintf(SGE_EVENT, MSG_SGETEXT_MOD_JOBS_SU, MSG_JOB_PROJECT, u32c(jobid));
      sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
   }

   /* ---- JB_pe */
   if ((pos=lGetPosViaElem(jep, JB_pe))>=0) {
      const char *pe_name;

      DPRINTF(("got new JB_pe\n")); 
      pe_name = lGetString(jep, JB_pe);
      if (pe_name && !sge_match_pe(pe_name)) {
         ERROR((SGE_EVENT, MSG_SGETEXT_DOESNOTEXIST_SS, 
               MSG_OBJ_PE, pe_name));
         sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
         DEXIT;
         return STATUS_EUNKNOWN;
      }
      lSetString(new_job, JB_pe, pe_name);
      *trigger |= MOD_EVENT;
      sprintf(SGE_EVENT, MSG_SGETEXT_MOD_JOBS_SU, MSG_OBJ_PE, u32c(jobid));
      sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
   }

   /* ---- JB_pe_range */
   if ((pos=lGetPosViaElem(jep, JB_pe_range))>=0) {
      DPRINTF(("got new JB_pe_range\n")); 
      lSetList(new_job, JB_pe_range, 
               lCopyList("", lGetList(jep, JB_pe_range)));
      *trigger |= MOD_EVENT;
      sprintf(SGE_EVENT, MSG_SGETEXT_MOD_JOBS_SU, MSG_JOB_SLOTRANGE, u32c(jobid));
      sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
   }

   /* ---- JB_hard_queue_list */
   if ((pos=lGetPosViaElem(jep, JB_hard_queue_list))>=0) {
      DPRINTF(("got new JB_hard_queue_list\n")); 
      
      /* attribute "qname" in queue complex must be requestable for -q */
      if (lGetList(jep, JB_hard_queue_list) && !queues_are_requestable(Master_Complex_List)) {
         ERROR((SGE_EVENT, MSG_JOB_QNOTREQUESTABLE2));
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         DEXIT;
         return STATUS_EUNKNOWN;
      }

      lSetList(new_job, JB_hard_queue_list, 
               lCopyList("", lGetList(jep, JB_hard_queue_list)));
      *trigger |= MOD_EVENT;
      sprintf(SGE_EVENT, MSG_SGETEXT_MOD_JOBS_SU, MSG_JOB_HARDQLIST, u32c(jobid));
      sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
   }

   /* ---- JB_soft_queue_list */
   if ((pos=lGetPosViaElem(jep, JB_soft_queue_list))>=0) {
      DPRINTF(("got new JB_soft_queue_list\n")); 
      lSetList(new_job, JB_soft_queue_list, 
               lCopyList("", lGetList(jep, JB_soft_queue_list)));
      *trigger |= MOD_EVENT;
      sprintf(SGE_EVENT, MSG_SGETEXT_MOD_JOBS_SU, MSG_JOB_SOFTQLIST, u32c(jobid));
      sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
   }

   /* ---- JB_master_hard_queue_list */
   if ((pos=lGetPosViaElem(jep, JB_master_hard_queue_list))>=0) {
      DPRINTF(("got new JB_master_hard_queue_list\n")); 
      
      /* attribute "qname" in queue complex must be requestable for -q */
      if (lGetList(jep, JB_master_hard_queue_list) && !queues_are_requestable(Master_Complex_List)) {
         ERROR((SGE_EVENT, MSG_JOB_QNOTREQUESTABLE2));
         sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
         DEXIT;
         return STATUS_EUNKNOWN;
      }

      lSetList(new_job, JB_master_hard_queue_list, 
               lCopyList("", lGetList(jep, JB_master_hard_queue_list)));
      *trigger |= MOD_EVENT;
      sprintf(SGE_EVENT, MSG_SGETEXT_MOD_JOBS_SU, MSG_JOB_MASTERHARDQLIST, u32c(jobid));
      sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
   }

   /* ---- JB_restart */
   if ((pos=lGetPosViaElem(jep, JB_restart))>=0) {
      DPRINTF(("got new JB_restart\n")); 
      lSetUlong(new_job, JB_restart, lGetUlong(jep, JB_restart));
      *trigger |= MOD_EVENT;
      sprintf(SGE_EVENT, MSG_SGETEXT_MOD_JOBS_SU, MSG_JOB_RESTARTBEHAVIOR, u32c(jobid));
      sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
   }

   /* ---- JB_shell_list */
   if ((pos=lGetPosViaElem(jep, JB_shell_list))>=0) {

      int status;
      DPRINTF(("got new JB_shell_list\n")); 
      if( (status = job_resolve_host_for_path_list(jep, alpp,JB_shell_list)) != STATUS_OK){
         DEXIT;
         return status;
      }


      lSetList(new_job, JB_shell_list, 
               lCopyList("", lGetList(jep, JB_shell_list)));
      sprintf(SGE_EVENT, MSG_SGETEXT_MOD_JOBS_SU, MSG_JOB_SHELLLIST, u32c(jobid));
      sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
   }

   /* ---- JB_env_list */
   if ((pos=lGetPosViaElem(jep, JB_env_list))>=0) {
      DPRINTF(("got new JB_env_list\n")); 

      /* check for qsh without DISPLAY set */
      if(JB_NOW_IS_QSH(lGetUlong(new_job, JB_now))) {
         int ret = job_check_qsh_display(jep, alpp);
         if(ret != STATUS_OK) {
            DEXIT;
            return ret;
         }
      }

      lSetList(new_job, JB_env_list, 
               lCopyList("", lGetList(jep, JB_env_list)));
      sprintf(SGE_EVENT, MSG_SGETEXT_MOD_JOBS_SU, MSG_JOB_ENVLIST, u32c(jobid));
      sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
   }

   /* ---- JB_qs_args */
   if ((pos=lGetPosViaElem(jep, JB_qs_args))>=0) {
      DPRINTF(("got new JB_qs_args\n")); 
      lSetList(new_job, JB_qs_args, 
               lCopyList("", lGetList(jep, JB_qs_args)));
      sprintf(SGE_EVENT, MSG_SGETEXT_MOD_JOBS_SU, MSG_JOB_QSARGS, u32c(jobid));
      sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
   }


   /* ---- JB_job_args */
   if ((pos=lGetPosViaElem(jep, JB_job_args))>=0) {
      DPRINTF(("got new JB_job_args\n")); 
      lSetList(new_job, JB_job_args, 
               lCopyList("", lGetList(jep, JB_job_args)));
      sprintf(SGE_EVENT, MSG_SGETEXT_MOD_JOBS_SU, MSG_JOB_SCRIPTARGS, u32c(jobid));
      sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
   }

   /* ---- JB_verify_suitable_queues */
   if ((pos=lGetPosViaElem(jep, JB_verify_suitable_queues))>=0) {
      int ret;
      lSetUlong(new_job, JB_verify_suitable_queues, 
            lGetUlong(jep, JB_verify_suitable_queues));
      ret = verify_suitable_queues(alpp, new_job, trigger); 
      if (lGetUlong(new_job, JB_verify_suitable_queues)==JUST_VERIFY 
         || ret != 0) {
         DEXIT;
         return ret;
      }   
   }

   /* ---- JB_context */
   if ((pos=lGetPosViaElem(jep, JB_context))>=0) {
      DPRINTF(("got new JB_context\n")); 
      set_context(lGetList(jep, JB_context), new_job);
      sprintf(SGE_EVENT, MSG_SGETEXT_MOD_JOBS_SU, MSG_JOB_CONTEXT, u32c(jobid));
      sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
   }

   /* deny certain modifications of running jobs */
   if (may_not_be_running && is_running) {
      ERROR((SGE_EVENT, MSG_SGETEXT_CANT_MOD_RUNNING_JOBS_U, u32c(jobid)));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
      DEXIT;
      return STATUS_EEXIST;
   }

   DEXIT;
   return 0;
}

/****** sge_job/verify_jobname() ***********************************************
*  NAME
*     job_verify_name() - verifies job name
*
*  SYNOPSIS
*     static int job_verify_name(const lListElem *job, lList **alpp, job_descr) 
*
*  FUNCTION
*     These checks are done for the attribute JB_job_name of 'job':
*     #1 reject job name if it starts with a digit
*     #2 reject job name if it contains colon(s) which confuse accounting
*     #3 reject job name if the same name is already used by another job 
*        of the same user and if this is the referenced as predecessor 
*        in a -hold_jid list.
*     A detailed problem description is added to the answer list.
*
*  INPUTS
*     const lListElem *job  - JB_Type elemente
*     lList **alpp          - the answer list
*     char *job_descr       - used for the text in the answer list 
*
*  RESULT
*     int - returns != 0 if there is a problem with the job name
*******************************************************************************/
static int job_verify_name(const lListElem *job, lList **alpp, 
                           const char *job_descr)
{
   const char *job_name = lGetString(job, JB_job_name);
   lListElem *jep;
   int ret = 0;

   DENTER(TOP_LAYER, "job_verify_name");

   if (isdigit(job_name[0])) {
      ERROR((SGE_EVENT, MSG_JOB_MOD_NOJOBNAME_SS, job_name, job_descr));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      ret = STATUS_EUNKNOWN;
   } else if (strchr(job_name, ':')) {
      ERROR((SGE_EVENT, MSG_GDI_KEYSTR_MIDCHAR_SC, MSG_GDI_KEYSTR_COLON, ':'));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      ret = STATUS_EUNKNOWN;
   } else {
      const char *job_owner = lGetString(job, JB_owner);

      for_each (jep, Master_Job_List) {
         const char *jep_name = lGetString(jep, JB_job_name);

         if (strcmp(job_name, jep_name) == 0) {
            const char *jep_owner = lGetString(jep, JB_owner);

            if(strcmp(job_owner, jep_owner) == 0) {
               u_long32 succ_jid = is_referenced_by_jobname(jep);

               if (succ_jid) {
                  ERROR((SGE_EVENT, MSG_JOB_MOD_JOBNAMEVIOLATESJOBNET_SSUU, 
                         job_name, job_descr, u32c(lGetUlong(jep, JB_job_number)), 
                         u32c(succ_jid)));
                  sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
                  ret = STATUS_EUNKNOWN;
                  break;
               }
            }
         }
      }
   }

   DEXIT;
   return ret;
}


/****** sge_job/is_referenced_by_jobname() *************************************
*  NAME
*     is_referenced_by_jobname()
*
*  SYNOPSIS
*     static u_long32 is_referenced_by_jobname(lListElem *jep) 
*
*  FUNCTION
*     Check whether a certain job is (still) referenced by a second
*     job in it's -hold_jid list.
*
*  INPUTS
*     lListElem *jep - the job
*
*  RESULT
*     static u_long32 - job ID of the job referencing 'jep' or 0 if no such
*******************************************************************************/
static u_long32 is_referenced_by_jobname(lListElem *jep)
{
   lList *succ_lp;

   DENTER(TOP_LAYER, "is_referenced_by_jobname");

   if ((succ_lp=lGetList(jep, JB_jid_sucessor_list))) {
      lListElem *succ_ep, *succ_jep;
      const char *job_name = lGetString(jep, JB_job_name);

      for_each (succ_ep, succ_lp) { 
         u_long32 succ_jid;
         succ_jid = lGetUlong(succ_ep, JRE_job_number);
         if ((succ_jep = sge_locate_job(succ_jid)) &&
            lGetSubStr(succ_jep, JRE_job_name, job_name, JB_jid_predecessor_list)) {
            DEXIT;
            return succ_jid;
         }
      }
   }

   DEXIT;
   return 0;
}


/****** sge_job/job_verify_predecessors() *************************************
*  NAME
*     job_verify_predecessors() -- verify -hold_jid list of a job
*
*  SYNOPSIS
*     static int job_verify_predecessors(const lListElem *job,
*                                        lList **alpp,  
*                                        lList *predecessors) 
*
*  FUNCTION
*     These checks are done:
*       #1 Ensure the job will not become it's own predecessor
*       #2 Reject ambiguous job name references
*     A detailed problem description is added to the answer list.
*
*  INPUTS
*     const lListElem *job - JB_Type element (JB_job_number may be 0 if
*                            not yet know (at submit time)
*     lList **alpp         - the answer list
*     lList *predecessors  - the list of predecessors to be verified
*
*  RESULT
*     int - returns != 0 if there is a problem with predecessors
******************************************************************************/
static int job_verify_predecessors(const lListElem *job, lList **alpp, 
                                   lList *predecessors)
{
   u_long32 jobid = lGetUlong(job, JB_job_number);
   const char *job_name = lGetString(job, JB_job_name);
   lListElem *pre;

   DENTER(TOP_LAYER, "job_verify_predecessors");

   for_each(pre, predecessors) {
      const char *pre_ident = lGetString(pre, JRE_job_name);

      if (isdigit(pre_ident[0])) {
         if (strchr(pre_ident, '.')) {
            ERROR((SGE_EVENT, MSG_JOB_MOD_UNKOWNJOBTOWAITFOR));
            sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
            DEXIT;
            return STATUS_EUNKNOWN;
         }
         if (jobid && atoi(pre_ident) == jobid) {
            DPRINTF(("got my own jobid in JRE_job_name\n"));
            ERROR((SGE_EVENT, MSG_JOB_MOD_GOTOWNJOBIDINHOLDJIDOPTION_U, 
                   u32c(jobid)));
            sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
            DEXIT;
            return STATUS_EUNKNOWN;
         } 
      } else {
         lListElem *jep;
         u_long32 jid_ref = 0;

         /* prevent self-references */   
         if (job_name && !strcmp(pre_ident, job_name)) {
            ERROR((SGE_EVENT, MSG_JOB_MOD_GOTOWNJOBIDINHOLDJIDOPTION_S, 
                   pre_ident));
            sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
            DEXIT;
            return STATUS_EUNKNOWN;
         }

         /* reject ambiguous job name references */
         for_each (jep, Master_Job_List) {
            const char *job_owner = lGetString(job, JB_owner);
            const char *jep_owner = lGetString(jep, JB_owner);

            if (!strcmp(lGetString(jep, JB_job_name), pre_ident) &&
                !strcmp(jep_owner, job_owner)) {
               if (jid_ref == 0) {
                  jid_ref = lGetUlong(jep, JB_job_number);
               } else {
                  ERROR((SGE_EVENT, MSG_JOB_MOD_JOBNETPREDECESSAMBIGUOUS_SUU, 
                         pre_ident, u32c(jid_ref), 
                         u32c(lGetUlong(jep, JB_job_number))));
                  sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
                  DEXIT;
                  return STATUS_EUNKNOWN;
               }
            }
         }

         /* if no matching job has been found we have to assume 
            the job finished already */
      }
   }

   DEXIT;
   return 0;
}

/* remove multiple requests for one resource */
/* this can't be done fully in clients without having complex definitions */ 
/* -l arch=linux -l a=sun4 */
static int compress_ressources(
lList **alpp, /* AN_Type */
lList *rl     /* RE_Type */
) {
   lListElem *ep, *prev, *rm_ep, *meta_ep;
   lList *lp;
   const char *attr_name;

   DENTER(TOP_LAYER, "compress_ressources");

   for_each (meta_ep, rl) {
      if ((lp = lGetList(meta_ep, RE_entries))) {
         for_each_rev (ep, lp) { 
            attr_name = lGetString(ep, CE_name);

            /* ensure 'slots' is not requested explicitly */
            if (!strcmp(attr_name, "slots")) {
               ERROR((SGE_EVENT, MSG_JOB_NODIRECTSLOTS)); 
               sge_add_answer(alpp, SGE_EVENT, STATUS_EEXIST, 0);
               DEXIT;
               return -1;
            }

            /* remove all previous requests with the same name */
            prev =  lPrev(ep);
            while ((rm_ep=prev)) {
               prev = lPrev(rm_ep);
               if (!strcmp(lGetString(rm_ep, CE_name), attr_name)) {
                  DPRINTF(("resource request -l "SFN"="SFN" overrides previous -l "SFN"="SFN"\n",
                     attr_name, lGetString(ep, CE_stringval), 
                     attr_name, lGetString(rm_ep, CE_stringval)));
                  lRemoveElem(lp, rm_ep);
               }
            }
         }
      }
   }

   DEXIT;
   return 0;
}


/* The context comes as a VA_Type list with certain groups of
** elements: A group starts with either:
** (+, ): All following elements are appended to the job's
**        current context values, or replaces the current value
** (-, ): The following context values are removed from the
**        job's current list of values
** (=, ): The following elements replace the job's current
**        context values.
** Any combination of groups is possible.
** To ensure portablity with common sge_gdi, (=, ) is the default
** when no group tag is given at the beginning of the incoming list
*/
static void set_context(
lList *ctx, /* VA_Type */
lListElem *job  /* JB_Type */
) {
   lList* newctx = NULL;
   lListElem* ctxep;
   lListElem* temp;
   char   mode = '+';
   
   newctx = lGetList(job, JB_context);

   /* if the incoming list is empty, then simply clear the context */
   if(!ctx || !lGetNumberOfElem(ctx)) {
      lSetList(job, JB_context, NULL);
      newctx = NULL;
   }
   else {
      /* if first element contains no tag => assume (=, ) */
      switch(*lGetString(lFirst(ctx), VA_variable)) {
         case '+':
         case '-':
         case '=':
            break;
         default:
            lSetList(job, JB_context, NULL);
            newctx = NULL;
            break;
      }
   }

   for_each(ctxep, ctx) {
      switch(*(lGetString(ctxep, VA_variable))) {
         case '+':
            mode = '+';
            break;
         case '-':
            mode = '-';
            break;
         case '=':
            lSetList(job, JB_context, NULL);
            newctx = NULL;
            mode = '+';
            break;
         default:
            switch(mode) {
               case '+':
                  if(!newctx)
                     lSetList(job, JB_context, newctx = lCreateList("context list", VA_Type));
                  if((temp = lGetElemStr(newctx, VA_variable, lGetString(ctxep, VA_variable))))
                     lSetString(temp, VA_value, lGetString(ctxep, VA_value));
                  else
                     lAppendElem(newctx, lCopyElem(ctxep));
                  break;
               case '-':

                  lDelSubStr(job, VA_variable, lGetString(ctxep, VA_variable), JB_context); 
                  /* WARNING: newctx is not guilty when complete list was deleted */
                  break;
            }
            break;
      }
   }
}

/************************************************************************/
static u_long32 sge_get_job_number()
{
   FILE *fp = NULL;
   static u_long32 job_number=0;

   DENTER(TOP_LAYER, "sge_get_job_number");

   /* we are called first time after startup, read the job number from file
    * or guess job_number  
    */
   if (!job_number) {
      if ((fp = fopen(SEQ_NUM_FILE, "r"))) {
         if (fscanf(fp, u32, &job_number) != 1) {
            ERROR((SGE_EVENT, MSG_JOB_NOSEQNRREAD_SS, SEQ_NUM_FILE, strerror(errno)));
            job_number = guess_highest_job_number();
         }
         else if (job_number < 1 || job_number >= MAX_SEQNUM)
            job_number = guess_highest_job_number();
         fclose(fp);
         fp = NULL;
         
      }
      else {
         WARNING((SGE_EVENT, MSG_JOB_NOSEQFILEOPEN_SS, SEQ_NUM_FILE, strerror(errno)));
         job_number = guess_highest_job_number();
      }   
   }

   job_number++;
   if (job_number > MAX_SEQNUM) {
      DPRINTF(("highest job number MAX_SEQNUM %d exceeded, starting over with 1\n", MAX_SEQNUM));
      job_number = 1;
   }
   
   if (!(fp = fopen(SEQ_NUM_FILE, "w"))) {
      ERROR((SGE_EVENT, MSG_JOB_NOSEQFILECREATE_SS, SEQ_NUM_FILE, strerror(errno)));
   }         
   else {
      fprintf(fp, u32"\n", job_number);
      fclose(fp);
   }   

   DEXIT;
   return job_number;
}



static u_long32 guess_highest_job_number()
{
   lListElem *jep;
   u_long32 maxid = 0;
   int pos;
  
   jep = lFirst(Master_Job_List);

   if (jep) 
      pos = lGetPosViaElem(jep, JB_job_number); 
   else
      return 0;
      
   for_each(jep, Master_Job_List) 
      maxid = MAX(maxid, lGetPosUlong(jep, pos));

   return maxid;
}   
      
      
/*******************************************************/
lListElem *sge_locate_job(
u_long32 jobid 
) {
   lListElem *jep = NULL;

   DENTER(BASIS_LAYER, "sge_locate_job");
   
   jep = lGetElemUlong(Master_Job_List, JB_job_number, jobid);

   DEXIT;
   return jep;
}


/* all modifications are done now verify schedulability */
static int verify_suitable_queues(
lList **alpp,
lListElem *jep,
int *trigger 
) {
   int verify_mode = lGetUlong(jep, JB_verify_suitable_queues);

   DENTER(TOP_LAYER, "verify_suitable_queues");
   
   switch (verify_mode) {
   case SKIP_VERIFY:   
      DPRINTF(("skip expensive verification of schedulability\n"));
      break;
   case ERROR_VERIFY:
   case WARINING_VERIFY:
   case JUST_VERIFY:
   default:
      {
         lListElem *pep = NULL, *ckpt_ep = NULL;
         lList *granted;
         lList *talp = NULL;
         int ngranted = 0;
         int try_it = 1;
         const char *pe_name, *ckpt_name;

         DPRINTF(("verify schedulability = %c\n", OPTION_VERIFY_STR[verify_mode]));

         /* parallel */
         if ((pe_name=lGetString(jep, JB_pe))) {
            if (!sge_is_pattern(pe_name))
               pep = sge_locate_pe(pe_name);
            else {
               /* use the first matching pe if we got a wildcard -pe requests */
               for_each (pep, Master_Pe_List) {
   
                  if (!fnmatch(pe_name, lGetString(pep, PE_name), 0)) {
                     DPRINTF(("use %s as first matching pe for %s to verify schedulability\n", 
                              lGetString(pep, PE_name), pe_name));
                     break;
                  }
               }
            }
            if (!pep)
               try_it = 0;
         }

         /* checkpointing */
         if (try_it && (ckpt_name=lGetString(jep, JB_checkpoint_object)))
            if (!(ckpt_ep = sge_locate_ckpt(ckpt_name)))
               try_it = 0;

         if (try_it) {
            int prev_dipatch_type = DISPATCH_TYPE_NONE;

            /* imagine qs is empty */
            set_qs_state(QS_STATE_EMPTY);

            /* redirect scheduler monitoring into answer list */
            if (verify_mode==JUST_VERIFY)
               set_monitor_alpp(&talp);

            granted = sge_replicate_queues_suitable4job(Master_Queue_List, 
                  jep, NULL, pep, ckpt_ep, scheddconf.queue_sort_method,
                  Master_Complex_List, Master_Exechost_List, 
                  Master_Userset_List, NULL, 0, &prev_dipatch_type, 0);
            ngranted = nslots_granted(granted, NULL);
            granted = lFreeList(granted);

            /* stop redirection of scheduler monitoring messages */
            if (verify_mode==JUST_VERIFY)
               set_monitor_alpp(NULL);

            /* stop dreaming */
            set_qs_state(QS_STATE_FULL);
         }

         /* consequences */
         if (!ngranted || !try_it) {
            /* copy error msgs from talp into alpp */
            if (verify_mode==JUST_VERIFY) {
               if (!*alpp)
                  *alpp = lCreateList("answer", AN_Type);
               lAddList(*alpp, talp);
            } else {
               lFreeList(talp);
            }

            SGE_ADD_MSG_ID(sprintf(SGE_EVENT, MSG_JOB_NOSUITABLEQ_S,
               (verify_mode==JUST_VERIFY ? MSG_JOB_VERIFYVERIFY: 
                  (verify_mode==ERROR_VERIFY)?MSG_JOB_VERIFYERROR:MSG_JOB_VERIFYWARN)));
            sge_add_answer(alpp, SGE_EVENT, STATUS_ESEMANTIC, 0);

            if (verify_mode != WARINING_VERIFY) {
               DEXIT;
               return (verify_mode==JUST_VERIFY)?0:STATUS_ESEMANTIC;
            }
         }

         if (verify_mode==JUST_VERIFY) {
            if (trigger)
               *trigger |= VERIFY_EVENT;
            if (!pep)
               sprintf(SGE_EVENT, MSG_JOB_VERIFYFOUNDQ); 
            else 
               sprintf(SGE_EVENT, MSG_JOB_VERIFYFOUNDSLOTS_I, ngranted);
            sge_add_answer(alpp, SGE_EVENT, STATUS_OK, NUM_AN_INFO);
            DEXIT;
            return 0;
         }
      }
      break;
   }

   DEXIT;
   return 0;
}

int sge_gdi_copy_job(
lListElem *jep,
lList **alpp,
lList **lpp,
char *ruser,
char *rhost,
sge_gdi_request *request 
) {  
   u_long32 seek_jid;
   int ret;
   lListElem *old_jep, *new_jep;
   int dummy_trigger;

   DENTER(TOP_LAYER, "sge_gdi_copy_job");

   if ( !jep || !ruser || !rhost ) {
      CRITICAL((SGE_EVENT, MSG_SGETEXT_NULLPTRPASSED_S, SGE_FUNC));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* seek job */
   seek_jid = lGetUlong(jep, JB_job_number);
   DPRINTF(("SEEK jobid "u32" for COPY operation\n", seek_jid));
   if (!(old_jep = sge_locate_job(seek_jid))) {
      ERROR((SGE_EVENT, MSG_SGETEXT_DOESNOTEXIST_SU, "job", u32c(seek_jid)));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   } 

   /* ensure copy is allowed */
   if (strcmp(ruser, lGetString(old_jep, JB_owner)) && sge_manager(ruser)) {
      ERROR((SGE_EVENT, MSG_JOB_NORESUBPERMS_SSS, ruser, rhost, lGetString(old_jep, JB_owner)));
      sge_add_answer(alpp, SGE_EVENT, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   new_jep = lCopyElem(old_jep);

   /* read script from old job and reuse it */
   if ( lGetString(old_jep, JB_exec_file)) {
      char *str;
      int len;
      if ((str = str_from_file(lGetString(old_jep, JB_exec_file), &len))) {
         lSetString(new_jep, JB_script_ptr, str);
         FREE(str);
         lSetUlong(new_jep, JB_script_size, len);
      }
   }

#if 1 /* EB: TODO */
   {
      lList *n_h_list, *u_h_list, *o_h_list, *s_h_list;

      n_h_list = lCopyList("range list", lGetList(old_jep, JB_ja_structure));
      u_h_list = lCreateList("user hold list", RN_Type);
      o_h_list = lCreateList("operator hold list", RN_Type);
      s_h_list = lCreateList("system hold list", RN_Type); 
      lSetList(new_jep, JB_ja_n_h_ids, n_h_list);
      lSetList(new_jep, JB_ja_u_h_ids, NULL);
      lSetList(new_jep, JB_ja_o_h_ids, NULL);
      lSetList(new_jep, JB_ja_s_h_ids, NULL); 
   }
#endif

#if 0
   /* reinit state of all job array tasks - same is done by qsub */
   {
      u_long32 start, end, step;
      lList *jat_list = NULL;

      job_get_ja_task_ids(new_jep, &start, &end, &step);
      for (; start<=end; start+=step) {
         lAddElemUlong(&jat_list, JAT_task_number, start, JAT_Type);
      }
      lSetList(new_jep, JB_ja_tasks, jat_list);
   }
#endif

   /* override settings of old job with new settings of jep */
   if (mod_job_attributes(new_jep, jep, alpp, ruser, rhost, &dummy_trigger)) {
      DEXIT;
      return STATUS_EUNKNOWN;
   }

   /* call add() method */
   ret = sge_gdi_add_job(new_jep, alpp, lpp, ruser, rhost, request);

   lFreeElem(new_jep);

   DEXIT;
   return ret;
}

static int job_check_qsh_display(lListElem *job, lList **answer_list)
{
   const lListElem *display_ep;

   DENTER(TOP_LAYER, "job_check_qsh_display");

   /* check for existence of DISPLAY and its contents */
   display_ep = lGetElemStr(lGetList(job, JB_env_list), VA_variable, "DISPLAY");
   if(display_ep == NULL) {
      sge_add_answer(answer_list, MSG_EXECD_NODISPLAY, STATUS_EUNKNOWN, 0);
      DEXIT;
      return STATUS_EUNKNOWN;
   } else {
      /* check value of display variable, if it has the form :<id>
       * or is an empty string,
       * it is useless in a grid environment and will be deleted.
       * Better would be to output an appropriate error message,
       * but I cannot introduce a new error message for the next 
       * patch release due to L10N restrictions.
       */
      const char *display = lGetString(display_ep, VA_value);
      if(display == NULL || strlen(display) == 0 || *display == ':') {
         sge_add_answer(answer_list, MSG_EXECD_EMPTYDISPLAY, STATUS_EUNKNOWN, 0);
         DEXIT;
         return STATUS_EUNKNOWN;
      }
   }

   DEXIT;
   return STATUS_OK;
}

lList *Master_Finished_Pe_Task_List = NULL;

/****** qmaster/job/ftref_add() ****************************************************
*  NAME
*     ftref_add() -- add reference for finished pe task
*
*  SYNOPSIS
*     int 
*     ftref_add(u_long32 job_id, u_long32 ja_task_id, const char *pe_task_id) 
*
*  FUNCTION
*     Will add a reference to a finished pe task to qmasters list (tree structure)
*     of finished pe tasks.
*
*     The structure is built as follows:
*      
*     job1  -> ja_task1 -> pe_task1
*                       -> pe_task2
*                            ...
*                       -> pe_taskn
*
*           -> ja_task2 -> pe_task1
*                       -> pe_task2
*                            ...
*                       -> pe_taskn
*     job2  -> ...
*     ...
*
*     A job has a list of ja_tasks. A ja_task has a list of pe_tasks.
*     The data structure used just contains identifiers and sublists.
*
*  INPUTS
*     u_long32 job_id        - job id
*     u_long32 ja_task_id    - array task id
*     const char *pe_task_id - parallel task id
*
*  RESULT
*     int - TRUE, if the pe task did not yet exist and was added
*           FALSE, if the pe task already existed in the list
*
*  NOTES
*     For future releases the ja_task object type (JAT_Type) should contain
*     a sublist containing all finished pe task id's.
*
*  SEE ALSO
*     qmaster/job/ftref_del_job()
*     qmaster/job/ftref_del_ja_task()
*******************************************************************************/
int 
ftref_add(u_long32 job_id, u_long32 ja_task_id, const char *pe_task_id)
{
   lListElem *job;
   lListElem *ja_task;
   lListElem *pe_task;

   DENTER(TOP_LAYER, "ftref_add");

   job = lGetElemUlong(Master_Finished_Pe_Task_List, FJR_job_number, job_id);
   if (job == NULL) {
      job = lAddElemUlong(&Master_Finished_Pe_Task_List, FJR_job_number, job_id, FJR_Type);
   }

   ja_task = lGetElemUlong(lGetList(job, FJR_ja_tasks), FTR_task_number, ja_task_id);
   if (ja_task == NULL) {
      ja_task = lAddSubUlong(job, FTR_task_number, ja_task_id, FJR_ja_tasks, FTR_Type);
   }

   pe_task = lGetElemStr(lGetList(ja_task, FTR_pe_tasks), FPR_id, pe_task_id);
   if (pe_task != NULL) {
      DPRINTF(("already handled exit of pe task "U32CFormat"."U32CFormat" "SFN"\n",
               job_id, ja_task_id, pe_task_id));
      DEXIT;
      return FALSE;
   }

   pe_task = lAddSubStr(ja_task, FPR_id, pe_task_id, FTR_pe_tasks, FPR_Type);

   DEXIT;
   return TRUE;
}

/****** qmaster/job/ftref_del_job() ****************************************************
*  NAME
*     ftref_del_job() -- delete complete finished pe task subtree for a job
*
*  SYNOPSIS
*     int 
*     ftref_del_job(u_long32 job_id)
*
*  FUNCTION
*     Deletes the whole subtree for a certain job_id in the qmaster's finished pe task 
*     tree.
*
*  INPUTS
*     u_long32 job_id        - job id
*
*  RESULT
*     int - TRUE, if the job existed, else FALSE
*
*  SEE ALSO
*     qmaster/job/ftref_add()
*     qmaster/job/ftref_del_ja_task()
*******************************************************************************/
int 
ftref_del_job(u_long32 job_id)
{
   lListElem *job;

   DENTER(TOP_LAYER, "ftref_del_job");

   job = lGetElemUlong(Master_Finished_Pe_Task_List, FJR_job_number, job_id);
   if (job == NULL) {
      DEXIT;
      return FALSE;
   }

   lRemoveElem(Master_Finished_Pe_Task_List, job);
   
   DEXIT;
   return TRUE;
}

/****** qmaster/job/ftref_del_ja_task() ****************************************************
*  NAME
*     ftref_del_ja_task() -- delete complete finished pe task subtree for a ja_task
*
*  SYNOPSIS
*     int 
*     ftref_del_ja_task(u_long32 job_id, u_long32 ja_task_id)
*
*  FUNCTION
*     Deletes the whole subtree for a certain ja_task referenced by job_id and ja_task_id
*     in the qmaster's finished pe task tree.
*
*  INPUTS
*     u_long32 job_id        - job id
*     u_long32 ja_task_id    - array task id
*
*  RESULT
*     int - TRUE, if the job and ja_task existed, else FALSE
*
*  SEE ALSO
*     qmaster/job/ftref_add()
*     qmaster/job/ftref_del_job()
*******************************************************************************/
int 
ftref_del_ja_task(u_long32 job_id, u_long32 ja_task_id)
{
   lListElem *job;
   lListElem *ja_task;

   DENTER(TOP_LAYER, "ftref_del_ja_task");

   job = lGetElemUlong(Master_Finished_Pe_Task_List, FJR_job_number, job_id);
   if (job == NULL) {
      DEXIT;
      return FALSE;
   }

   ja_task = lGetElemUlong(lGetList(job, FJR_ja_tasks), FTR_task_number, ja_task_id);
   if (ja_task == NULL) {
      DEXIT;
      return FALSE;
   }

   lRemoveElem(lGetList(job, FJR_ja_tasks), ja_task);
  
   /* if last ja_task is removed, remove job */

   if (lGetNumberOfElem(lGetList(job, FJR_ja_tasks)) == 0) {
      ftref_del_job(job_id);
   }
  
   DEXIT;
   return TRUE;
}
/****** sge_job/job_resolve_host_for_path_list() *******************************
*  NAME
*     job_resolve_host_for_path_list() -- is a hostname valid 
*
*  SYNOPSIS
*     int job_resolve_host_for_path_list(const lListElem *job, lList 
*     **answer_list, int name) 
*
*  FUNCTION
*     ??? 
*
*  INPUTS
*     const lListElem *job - the submited cull list 
*     lList **answer_list  - AN_Type element
*     int name             - a JB_Type ( JB_stderr_path_list or JB_stout_path_list)
*
*  RESULT
*     int - error code ( STATUS_OK, or ...) 
*
*******************************************************************************/

int job_resolve_host_for_path_list(const lListElem *job, lList **answer_list, int name)
{
   lListElem *ep;

   DENTER(TOP_LAYER, "job_resolve_host_for_path_list");

   for_each( ep, lGetList(job, name) ){
      int res=sge_resolve_host(ep, PN_host);
printf("host ret: %d\n", res);
      if( (res!=0) && (res!=-1) && (res!=1)){ /* 0 = everything is fine, 1 = no host specified*/
         const char *hostname = lGetHost(ep, PN_host);

         ERROR((SGE_EVENT, MSG_SGETEXT_CANTRESOLVEHOST_S, hostname));
         sge_add_answer(answer_list, SGE_EVENT, STATUS_EUNKNOWN, 0);
         DEXIT;
         return STATUS_EUNKNOWN;
      }  
      else if(res==-1){/*something in the data-structure is wrong */
         ERROR((SGE_EVENT, MSG_PARSE_NULLPOINTERRECEIVED));
         sge_add_answer(answer_list, SGE_EVENT, STATUS_EUNKNOWN, 0);
         DEXIT;
         return STATUS_EUNKNOWN;
      }
      /* ensure, that each hostname is only specified once */
      {
         const char *hostname = lGetHost(ep, PN_host);       
         lListElem *temp;         
         for(temp= lPrev(ep); temp; temp = lPrev(temp)){
            const char *temp_hostname = lGetHost(temp, PN_host);
            if(hostname == NULL){
               if(temp_hostname == NULL){
                  ERROR((SGE_EVENT, MSG_PARSE_DUPLICATEHOSTINFILESPEC));
                  sge_add_answer(answer_list, SGE_EVENT, STATUS_EUNKNOWN, 0);
            
                  DEXIT;
                  return STATUS_EUNKNOWN;
               }
            } 
            else{
                if(strcmp(hostname, temp_hostname)==0){
               ERROR((SGE_EVENT, MSG_PARSE_DUPLICATEHOSTINFILESPEC));
               sge_add_answer(answer_list, SGE_EVENT, STATUS_EUNKNOWN, 0);

               DEXIT;
               return STATUS_EUNKNOWN;
            }
            }
         } 
      }
   }
   DEXIT;
   return STATUS_OK;
}
