# Source this script to set up the ROOTANA build that this script is part of.
#
# This script if for the csh like shells, see thisrootana.sh for bash like shells.
#
# Author: Fons Rademakers, 18/8/2006
# Modified for ROOTANA: Konstantin Olchanski, Jan 2017

if ($?ROOTANASYS) then
   if ($ROOTANASYS != "") then
     set old_rootanasys="$ROOTANASYS"
   endif
endif

# $_ should be source .../thisrootana.csh
set ARGS=($_)

set LSOF=`env PATH=/usr/sbin:${PATH} which lsof`
set thisfile="`${LSOF} -w +p $$ | grep -oE '/.*thisrootana.csh'  `"
if ( "$thisfile" == "" ) then
#   set thisfile=/does/not/exist
endif
if ( "$thisfile" != "" && -e ${thisfile} ) then
   # We found it, didn't we.
   set thisrootana="`dirname ${thisfile}`"
else if ("$ARGS" != "") then
   set thisrootana="`dirname ${ARGS[2]}`"
else
   # But $_ might not be set if the script is source non-interactively.
   # In [t]csh the sourced file is inserted 'in place' inside the
   # outer script, so we need an external source of information
   # either via the current directory or an extra parameter.
   if ( -e thisrootana.csh ) then
      set thisrootana=${PWD}
   else if ( -e bin/thisrootana.csh ) then
      set thisrootana=${PWD}/bin
   else if ( "$1" != "" ) then
      if ( -e ${1}/bin/thisrootana.csh ) then
         set thisrootana=${1}/bin
      else if ( -e ${1}/thisrootana.csh ) then
         set thisrootana=${1}
      else
         echo "thisrootana.csh: ${1} does not contain a ROOTANA installation"
      endif
   else
      echo 'Error: The call to "source where_rootana_is/bin/thisrootana.csh" can not determine the location of the ROOTANA installation'
      echo "because it was embedded another script (this is an issue specific to csh)."
      echo "Use either:"
      echo "   cd where_rootana_is; source bin/thisrootana.csh"
      echo "or"
      echo "   source where_rootana_is/bin/thisrootana.csh where_rootana_is"
   endif
endif

if ($?thisrootana) then

#setenv ROOTANASYS "`(cd ${thisrootana}/..;pwd)`"
setenv ROOTANASYS "`(cd ${thisrootana};pwd)`"

if ($?old_rootanasys) then
   setenv PATH `echo $PATH | sed -e "s;:$old_rootanasys/bin:;:;g" \
                                 -e "s;:$old_rootanasys/bin;;g"   \
                                 -e "s;$old_rootanasys/bin:;;g"   \
                                 -e "s;$old_rootanasys/bin;;g"`
   #if ($?LD_LIBRARY_PATH) then
   #   setenv LD_LIBRARY_PATH `echo $LD_LIBRARY_PATH | \
   #                          sed -e "s;:$old_rootanasys/lib:;:;g" \
   #                              -e "s;:$old_rootanasys/lib;;g"   \
   #                              -e "s;$old_rootanasys/lib:;;g"   \
   #                              -e "s;$old_rootanasys/lib;;g"`
   #endif
endif

set path = ($ROOTANASYS/bin $path)

#if ($?LD_LIBRARY_PATH) then
#   setenv LD_LIBRARY_PATH $ROOTANASYS/lib:$LD_LIBRARY_PATH
#else
#   setenv LD_LIBRARY_PATH $ROOTANASYS/lib
#endif

endif # if ("$thisrootana" != "")

set thisrootana=
set old_rootanasys=

