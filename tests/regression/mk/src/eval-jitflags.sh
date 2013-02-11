#!/bin/bash
#
#

SIMCMD=$1
BMARKDIR=tests/$2
RUNCMD=$3
OUTFILE=$4

HEADER="name;walltime;transins;intins;totalins;crc32arcsim;threads;mode;thresh;"

THREADS="1 3"
MODES="bb page"
THRESH="4 5"
#THREADS=1
#THRESH=1
#MODES=page

BMARKNAME=`basename ${BMARKDIR}`

# EEMBC benchmarks are just files
if [ ! -d $BMARKDIR ];
then
   BMARKDIR=`dirname $BMARKDIR`
fi

tmpfile=`mktemp /tmp/arcsim.out.XXXXXXXXX`
arcsimbin=`echo $SIMCMD | cut -d' ' -f1`
crc32arcsim=`crc32 $arcsimbin`

if [ ! -e $OUTFILE ];
then
   echo $HEADER > $OUTFILE
fi

echo "==== Running $BMARKNAME ===="

for thread in $THREADS;
do
   echo
   echo "Threads $thread"
   for mode in $MODES;
   do
      echo "Mode $mode"
      for thresh in $THRESH;
      do
         echo "Thresh $thresh"
         pushd $BMARKDIR >/dev/null

         RUNARGS=`$RUNCMD`
         minwalltime=99999999
         iter=0

         tmpdir=`mktemp -d /tmp/arcsim.dir.XXXXXXXXX`
         while [ $iter -lt 5 ];
         do
            /usr/bin/time -p $SIMCMD --fast --fast-trans-mode=$mode --fast-thresh=$thresh --fast-num-threads=$thread --fast-tmp-dir=$tmpdir -e $RUNARGS &> $tmpfile
            if [ ! "$?" = "0" ];
            then
               echo
               echo " !!! Non-zero return value"
               cat $tmpfile
               echo " !!! End of output."
            else
               walltime=`awk '/^real / {print $2}' $tmpfile`
               echo -n "$walltime "
               if test 1 = `echo "a=$walltime;b=$minwalltime;r=-1;if(a<b)r=1;r"|bc`; then minwalltime=$walltime; fi
               iter=$(($iter+1))
            fi
         done
         rm -rf $tmpdir
         popd >/dev/null

         echo
         echo " >> Collecting data..."
         walltime=$minwalltime
         transins=`awk '/^ Translated instructions:/ {print $3}' $tmpfile`
         intins=`awk '/^ Interpreted instructions:/ {print $3}' $tmpfile`
         totalins=`awk '/^ Total instructions:/ {print $3}' $tmpfile`
         echo " >> Wall time: $walltime - Instructions: $transins trans, $intins int, $totalins total."
         echo "$BMARKNAME;$walltime;$transins;$intins;$totalins;$crc32arcsim;$thread;$mode;$thresh;" >> $OUTFILE

         rm $tmpfile
      done
   done
done

echo "==== Finished $BMARKNAME ===="


