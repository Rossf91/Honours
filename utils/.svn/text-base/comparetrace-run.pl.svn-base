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
# Run two commands (i.e. two arcsims with --trace[-fast])
# and compare their output, showing a diff if there are any
# differences. This does some filtering, so fast mode and
# interpreted traces can be compared. The output will always
# be unfiltered.
#
# =====================================================================
#

if ($#ARGV < 1) {
   print "Usage:\n";
   print "  comparetrace-run.pl \"command 1\" \"command 2\" [context_lines]\n";
   print "\n";
   exit 1;
}

$pipe1 = $ARGV[0];
$pipe2 = $ARGV[1];
$context = $ARGV[2];

if (!$context) {
   $context = 20;
}

print "Launching commands ($context lines of context will be shown):\n";
print " CMD1: $pipe1\n";
print " CMD2: $pipe2\n";
print "**** Running...\n";

open(PIPE1, $pipe1." 2>&1 |") || die ("Cannot open $pipe1: $!\n");
open(PIPE2, $pipe2." 2>&1 |") || die ("Cannot open $pipe2: $!\n");

sub filter_line ($) {
   $_[0] =~ s/^\[[0-9a-f]+\]//;
   return $_[0];
}

my $linecount = 1;
my $in_diff_mode = 0;
my (@history1, @history2);

while (1) {
   $l1 = <PIPE1>;
   $l2 = <PIPE2>;

   $l1orig = $l1; $l2orig = $l2;

   if (eof(PIPE1)) { 
      if ($in_diff_mode) {
        print "**** EOF on command 1, collecting no more context.\n";
        last;
      } else {
        die "**** No difference detected before EOF on command 1.\n"; 
      }
   }
   if (eof(PIPE2)) {
      if ($in_diff_mode) {
        print "**** EOF on command 2, collecting no more context.\n";
        last;
      } else {
        die "**** No difference detected before EOF on command 2.\n"; 
      }
   }

   $l1 = filter_line($l1);
   $l2 = filter_line($l2);

   if (!$in_diff_mode && $l1 ne $l2) {
      $in_diff_mode = 1;

      print "**** Difference detected. Collecting more context...\n";

      $l1orig = $linecount.">>> ".$l1orig;
      $l2orig = $linecount."<<< ".$l2orig;
   }
   
   $len = push(@history1, $l1orig);
   push(@history2, $l2orig);

   if (!$in_diff_mode && $len > $context) {
         shift @history1;
         shift @history2;
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
    print $l1orig;
    print "<<<<<<<<<<<<<\n";
    foreach $line (@history2) {
      print $line;
    }
    print $l2orig;
    print ">>>>>>>>>>>>>\n";
} else {
    print "**** No difference found.\n";
}
   
close PIPE1;
close PIPE2;


