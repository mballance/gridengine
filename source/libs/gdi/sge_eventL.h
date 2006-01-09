#ifndef __SGE_EVENTL_H
#define __SGE_EVENTL_H

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

#include "sge_boundaries.h"
#include "cull.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* *INDENT-OFF* */

/* valid values for EV_id */
enum {
   EV_ID_ANY = 0,            /* qmaster will give the ev a unique id */
   EV_ID_SCHEDD = 1,         /* schedd registers at qmaster */
   EV_ID_FIRST_DYNAMIC = 11  /* first id given by qmaster for EV_ID_ANY registration */ 
};

enum {
   /* identification */
   EV_id = EV_LOWERBOUND,    /* unique id requested by client or given by qmaster */
   EV_name,                  /* name of event client (non unique) */

   /* addressing informations */
   EV_host,                  /* host to deliver the events */
   EV_commproc,              /* used to deliver events */
   EV_commid,                /* unique id delivered by the sge_commd */

   /* authentication informations */
   EV_uid,

   /* event request description */
   EV_d_time,                /* delivery interval for events */
   EV_last_heard_from,         /* used to trash unheard event clients */
   EV_last_send_time,        /* time when last event list has been sent */
   EV_next_send_time,        /* time when next list has to be sent */
   EV_next_number,           /* the number the next event will get */
   EV_subscription,          /* subscription information */
   EV_events                 /* used to hold the events that */
                             /* are not acknowledged */
};

LISTDEF(EV_Type)
   SGE_ULONG(EV_id)
   SGE_STRING(EV_name)
   SGE_HOST(EV_host)
   SGE_STRING(EV_commproc)
   SGE_ULONG(EV_commid)
   SGE_ULONG(EV_uid)
   SGE_ULONG(EV_d_time)
   SGE_ULONG(EV_last_heard_from)
   SGE_ULONG(EV_last_send_time)
   SGE_ULONG(EV_next_send_time)
   SGE_ULONG(EV_next_number)
   SGE_STRING(EV_subscription)
   SGE_LIST(EV_events)
LISTEND 

NAMEDEF(EVN)
   NAME("EV_id")
   NAME("EV_name")
   NAME("EV_host")
   NAME("EV_commproc")
   NAME("EV_commid")
   NAME("EV_uid")
   NAME("EV_d_time")
   NAME("EV_last_heard_from")
   NAME("EV_last_send_time")
   NAME("EV_next_send_time")
   NAME("EV_next_number")
   NAME("EV_subscription")
   NAME("EV_events")
NAMEEND

#define EVS sizeof(EVN)/sizeof(char*)

/* valid values for ET_type */
enum {
   sgeE_ALL_EVENTS,                 /* + = impl. and tested, - = not available */

   sgeE_ADMINHOST_LIST,             /* + send admin host list at registration */
   sgeE_ADMINHOST_ADD,              /* + event add admin host */
   sgeE_ADMINHOST_DEL,              /* + event delete admin host */
   sgeE_ADMINHOST_MOD,              /* - event modify admin host */

   sgeE_CALENDAR_LIST,              /* + send calendar list at registration */
   sgeE_CALENDAR_ADD,               /* + event add calendar */
   sgeE_CALENDAR_DEL,               /* + event delete calendar */
   sgeE_CALENDAR_MOD,               /* + event modify calendar */

   sgeE_CKPT_LIST,                  /* + send ckpt list at registration */
   sgeE_CKPT_ADD,                   /* + event add ckpt */
   sgeE_CKPT_DEL                    /* + event delete ckpt */,
   sgeE_CKPT_MOD                    /* + event modify ckpt */,

   sgeE_COMPLEX_LIST,               /* + send complex list at registration */
   sgeE_COMPLEX_ADD,                /* + event add complex */
   sgeE_COMPLEX_DEL,                /* + event delete complex */
   sgeE_COMPLEX_MOD,                /* + event modify complex */

   sgeE_CONFIG_LIST,                /* + send config list at registration */
   sgeE_CONFIG_ADD,                 /* + event add config */
   sgeE_CONFIG_DEL,                 /* + event delete config */
   sgeE_CONFIG_MOD,                 /* + event modify config */

   sgeE_EXECHOST_LIST,              /* + send exec host list at registration */
   sgeE_EXECHOST_ADD,               /* + event add exec host */
   sgeE_EXECHOST_DEL,               /* + event delete exec host */
   sgeE_EXECHOST_MOD,               /* + event modify exec host */

   sgeE_GLOBAL_CONFIG,              /* + global config changed, replace by sgeE_CONFIG_MOD */

#ifndef __SGE_NO_USERMAPPING__
   sgeE_HOST_GROUP_LIST,
   sgeE_HOST_GROUP_ADD,
   sgeE_HOST_GROUP_DEL,
   sgeE_HOST_GROUP_MOD,
#endif

   sgeE_JATASK_DEL,                 /* + event delete array job task */
   sgeE_JATASK_MOD,                 /* + event modify array job task */

   sgeE_JOB_LIST,                   /* + send job list at registration */
   sgeE_JOB_ADD,                    /* + event job add (new job) */
   sgeE_JOB_DEL,                    /* + event job delete */
   sgeE_JOB_MOD,                    /* + event job modify */
   sgeE_JOB_MOD_SCHED_PRIORITY,     /* + event job modify priority */
   sgeE_JOB_USAGE,                  /* + event job online usage */
   sgeE_JOB_FINAL_USAGE,            /* + event job final usage report after job end */

   sgeE_JOB_SCHEDD_INFO_LIST,       /* + send job schedd info list at registration */
   sgeE_JOB_SCHEDD_INFO_ADD,        /* - event jobs schedd info added */
   sgeE_JOB_SCHEDD_INFO_DEL,        /* - event jobs schedd info deleted */
   sgeE_JOB_SCHEDD_INFO_MOD,        /* + event jobs schedd info modified */

   sgeE_MANAGER_LIST,               /* + send manager list at registration */
   sgeE_MANAGER_ADD,                /* + event add manager */
   sgeE_MANAGER_DEL,                /* + event delete manager */
   sgeE_MANAGER_MOD,                /* - event modify manager */

   sgeE_OPERATOR_LIST,              /* + send operator list at registration */
   sgeE_OPERATOR_ADD,               /* + event add operator */
   sgeE_OPERATOR_DEL,               /* + event delete operator */
   sgeE_OPERATOR_MOD,               /* - event modify operator */

   sgeE_NEW_SHARETREE,              /* + replace possibly existing share tree */

   sgeE_PE_LIST,                    /* + send pe list at registration */
   sgeE_PE_ADD,                     /* + event pe add */
   sgeE_PE_DEL,                     /* + event pe delete */
   sgeE_PE_MOD,                     /* + event pe modify */

   sgeE_PROJECT_LIST,               /* + send project list at registration */
   sgeE_PROJECT_ADD,                /* + event project add */
   sgeE_PROJECT_DEL,                /* + event project delete */
   sgeE_PROJECT_MOD,                /* + event project modify */

   sgeE_QMASTER_GOES_DOWN,          /* + qmaster notifies all event clients, before
                                         it exits */

   sgeE_QUEUE_LIST,                 /* + send queue list at registration */
   sgeE_QUEUE_ADD,                  /* + event queue add */
   sgeE_QUEUE_DEL,                  /* + event queue delete */
   sgeE_QUEUE_MOD,                  /* + event queue modify */
   sgeE_QUEUE_SUSPEND_ON_SUB,       /* + queue is suspended by subordinate mechanism */
   sgeE_QUEUE_UNSUSPEND_ON_SUB,     /* + queue is unsuspended by subordinate mechanism */

   sgeE_SCHED_CONF,                 /* + replace existing (sge) scheduler configuration */

   sgeE_SCHEDDMONITOR,              /* + trigger scheduling run */

   sgeE_SHUTDOWN,                   /* + request shutdown of an event client */

   sgeE_SUBMITHOST_LIST,            /* + send submit host list at registration */
   sgeE_SUBMITHOST_ADD,             /* + event add submit host */
   sgeE_SUBMITHOST_DEL,             /* + event delete submit host */
   sgeE_SUBMITHOST_MOD,             /* - event modify submit host */

   sgeE_USER_LIST,                  /* + send user list at registration */
   sgeE_USER_ADD,                   /* + event user add */
   sgeE_USER_DEL,                   /* + event user delete */
   sgeE_USER_MOD,                   /* + event user modify */
   
#ifndef __SGE_NO_USERMAPPING__
   sgeE_USERMAPPING_ENTRY_LIST,
   sgeE_USERMAPPING_ENTRY_ADD,
   sgeE_USERMAPPING_ENTRY_DEL,
   sgeE_USERMAPPING_ENTRY_MOD,
#endif

   sgeE_USERSET_LIST,               /* + send userset list at registration */
   sgeE_USERSET_ADD,                /* + event userset add */
   sgeE_USERSET_DEL,                /* + event userset delete */
   sgeE_USERSET_MOD,                /* + event userset modify */

   sgeE_EVENTSIZE 
};

enum {
   ET_number = ET_LOWERBOUND,        /* number of the event */
   ET_type,                  /* type the event */
   ET_intkey,                /* a int key for use by a specific event type */
   ET_intkey2,               /* a int key for use by a specific event type */
   ET_strkey,                /* a str key for use by a specific event type */
   ET_new_version            /* new version of the changed object */
};

LISTDEF(ET_Type)
   SGE_ULONG(ET_number)
   SGE_ULONG(ET_type)
   SGE_ULONG(ET_intkey)
   SGE_ULONG(ET_intkey2)
   SGE_STRING(ET_strkey)
   SGE_LIST(ET_new_version)
LISTEND 

NAMEDEF(ETN)
   NAME("ET_number")
   NAME("ET_type")
   NAME("ET_intkey")
   NAME("ET_intkey2")
   NAME("ET_strkey")
   NAME("ET_new_version")
NAMEEND

/* *INDENT-ON* */ 

#define ETS sizeof(ETN)/sizeof(char*)
#ifdef  __cplusplus
}
#endif
#endif                          /* __SGE_EVENTL_H */