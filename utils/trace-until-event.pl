#!/usr/bin/perl -w
# =====================================================================
#
#                      Confidential Information
#           Limited Distribution to Authorized Persons Only
#         Copyright (C) 2011 The University of Edinburgh
#                        All Rights Reserved
#
# =====================================================================
#
# Run command (i.e. arcsim with --trace[-fast] on) and match for certain
# strings/events in the trace output. This is usefull when some event
# occurs after millions of instructions and redirecting the trace
# to a file exceeds the 2GB file limit.
#
# =====================================================================
#

if ($#ARGV < 1) {
   print "Usage:\n";
   print "  trace-until-event.pl \"event\" \"command 1\" [context_lines]\n";
   print "\n";
   exit 1;
}

$event = $ARGV[0];
$pipe1 = $ARGV[1];
$context = $ARGV[2];

if (!$context) {
   $context = 20;
}

print "Launching command ($context lines of context will be shown):\n";
print " CMD: $pipe1\n";
print "**** Running...\n";

open(PIPE1, $pipe1." 2>&1 |") || die ("Cannot open $pipe1: $!\n");

my $linecount = 1;
my $in_diff_mode = 0;
my (@history1);

while (1) {
   $l1 = <PIPE1>;

   if (eof(PIPE1)) { 
      if ($in_diff_mode) {
        print "**** EOF on command, collecting no more context.\n";
        last;
      } else {
        die "**** Event not detected before EOF on command 1.\n"; 
      }
   }

   if (!$in_diff_mode && $l1 =~ /$event/) {
     print "**** Event detected. Collecting more context...\n";
      $in_diff_mode = 1;
      $l1 = $linecount.">>> ".$l1;
   }
   
   $len = push(@history1, $l1);

   if (!$in_diff_mode && $len > $context) {
         shift @history1;
   } elsif ($in_diff_mode && $len > 2*$context) {
         last;
   }

   ++$linecount;
}

if ($in_diff_mode) {
    print ">>>>>>>>>>>>>\n";
    foreach $line (@history1) {
      print $line;
    }
    print $l1;
    print "<<<<<<<<<<<<<\n";
} else {
    print "**** Event not found.\n";
}
   
close PIPE1;


