# Source this script to set up the ROOTANA build that this script is part of.
#
# This script if for the bash like shells, see thisrootana.csh for csh like shells.
# Author: Fons Rademakers, 18/8/2006
# Modified for ROOTANA: Konstantin Olchanski, Jan 2017

drop_from_path()
{
   # Assert that we got enough arguments
   if test $# -ne 2 ; then
      echo "drop_from_path: needs 2 arguments"
      return 1
   fi

   p=$1
   drop=$2

   newpath=`echo $p | sed -e "s;:${drop}:;:;g" \
                          -e "s;:${drop};;g"   \
                          -e "s;${drop}:;;g"   \
                          -e "s;${drop};;g"`
}

if [ -n "${ROOTANASYS}" ] ; then
   old_rootanasys=${ROOTANASYS}
fi

SOURCE=${BASH_ARGV[0]}
if [ "x$SOURCE" = "x" ]; then
    SOURCE=${(%):-%N} # for zsh
fi

if [ "x${SOURCE}" = "x" ]; then
    if [ -f bin/thisrootana.sh ]; then
        ROOTANASYS="$PWD"; export ROOTANASYS
    elif [ -f ./thisrootana.sh ]; then
        #ROOTANASYS=$(cd ..  > /dev/null; pwd); export ROOTANASYS
        ROOTANASYS=$(pwd); export ROOTANASYS
    else
        echo ERROR: must "cd where/rootana/is" before calling ". bin/thisrootana.sh" for this version of bash!
        ROOTANASYS=; export ROOTANASYS
        return 1
    fi
else
    # get param to "."
    thisrootana=$(dirname ${SOURCE})
    #ROOTANASYS=$(cd ${thisrootana}/.. > /dev/null;pwd); export ROOTANASYS
    ROOTANASYS=$(pwd); export ROOTANASYS
fi

if [ -n "${old_rootanasys}" ] ; then
   if [ -n "${PATH}" ]; then
      drop_from_path "$PATH" ${old_rootanasys}/bin
      PATH=$newpath
   fi
fi

if [ -z "${PATH}" ]; then
   PATH=$ROOTANASYS/bin; export PATH
else
   PATH=$ROOTANASYS/bin:$PATH; export PATH
fi

#ROOTANA_INCLUDES="-I$ROOTANASYS/include"; export ROOTANA_INCLUDES
#ROOTANA_LIBS="-L$ROOTANASYS/lib -lrootana.a"; export ROOTANA_LIBS

unset old_rootanasys
unset thisrootana

#end
