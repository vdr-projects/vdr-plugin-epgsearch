#!/bin/bash
#------------------------------------------------------------------------------
# this script allows searching for a repeat of recording using epgsearch (>=0.9.3)
# add the following lines to your reccmds.conf
#
# Search for repeat : /path_to_this_script/recrep.sh 0
# Search for repeat with subtitle (same episode): /path_to_this_script/recrep.sh 1
#
# Author: Christian Wieninger (cwieninger@gmx.de)
#------------------------------------------------------------------------------

# adjust the following lines to your config
# your video dir
VIDEODIR=/video
# your plugins config dir
PLUGINCONFDIR=/video/plugins/epgsearch
# path to svdrpsend.pl
SVDRPSEND=/usr/local/src/VDR/svdrpsend.pl
# if you are using special subfolders for some recordings, please add them here
FOLDERS="docu,series,magazines"
# the key used to call epgsearch
EPGSEARCHKEY=green

# do not edit below this line

cat << EOM >/tmp/cmd.sh
SEARCHTERM='$2'
#cut a trailing /
SEARCHTERM=\${SEARCHTERM%*/}
#cut leading video dir
SEARCHTERM=\${SEARCHTERM#$VIDEODIR}
#cut leading /
SEARCHTERM=\${SEARCHTERM#/}
#cut leading special folders
i=0;
FOLDERS=$FOLDERS;
while [ "\$LASTWORD" != "\$FOLDERS" ];
do
    LASTWORD=\${FOLDERS%%,*};
    SEARCHTERM=\${SEARCHTERM#*\$LASTWORD/};
    i=\$i+1;
    FOLDERS=\${FOLDERS#*,};
done
#cut last subdir
SEARCHTERM=\${SEARCHTERM%/*.rec}
#cut trailing dummy subtitle created by epgsearch
SEARCHTERM=\${SEARCHTERM%/???_??.??.????-??:??}
if [ "$1" -eq "0" ]; then
    SEARCHTERM=\${SEARCHTERM%/*};
fi
#replace / with ~ as word delimiter
SEARCHTERM=\${SEARCHTERM//\//~}
#replace _ with blank
SEARCHTERM=\${SEARCHTERM//_/ }

RCFILE=$PLUGINCONFDIR/.epgsearchrc
echo Search=\$SEARCHTERM > \$RCFILE
#search for this term as phrase
echo SearchMode=0 >> \$RCFILE
if [ "$1" -eq "0" ]; then
    echo UseSubtitle=0 >> \$RCFILE;
fi
echo UseDescr=0 >> \$RCFILE
$SVDRPSEND HITK $EPGSEARCHKEY
EOM

echo ". /tmp/cmd.sh; rm /tmp/cmd.sh" | at now
#/tmp/cmd.sh;
#cat /video/plugins/.epgsearchrc


