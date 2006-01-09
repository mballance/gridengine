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
#ifndef RMONLIGHT


#define DEBUG

#include "rmon_h.h"
#include "rmon_def.h"
#include "rmon_io.h"

#include "rmon_rmon.h"
#include "rmon_piped_message.h"

int rmon_get_piped_message(
int sfd,
piped_message_type *piped_message 
) {
   int i;

#undef  FUNC
#define FUNC "rmon_get_piped_message"
   DENTER;

   i = rmon_readnbytes(sfd, (char *) piped_message, sizeof(piped_message_type));

   if (i) {
      DPRINTF(("   returning %d\n", i));
      DEXIT;
      return 0;
   }

   DEXIT;
   return 1;
}


#endif /* RMONLIGHT */