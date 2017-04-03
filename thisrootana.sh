# Source this script to set up the ROOTANA build that this script is part of.
# This script if for the bash like shells, see thisrootana.csh for csh like shells.
# Modified for ROOTANA: Konstantin Olchanski, Jan 2017

#echo BASH_ARGV=[$BASH_ARGV]

if [ ! -r $BASH_ARGV ]; then
    echo This script must be invoked by a full path name
fi

export ROOTANASYS=$(dirname $BASH_ARGV)
#echo ROOTANASYS=[$ROOTANASYS]

#ROOTANA_INCLUDES="-I$ROOTANASYS/include"; export ROOTANA_INCLUDES
#ROOTANA_LIBS="-L$ROOTANASYS/lib -lrootana.a"; export ROOTANA_LIBS

unset thisrootana

#end
