-c /disk/home/pasta1/s0675281/SPEC2006/benchspec/CPU2006/482.sphinx3/run/run_base_test_arcsim.0000 -o an4.log.cmp specperl /disk/home/pasta1/s0675281/SPEC2006/bin/specdiff -m -l 10  --reltol 0.001  --floatcompare /disk/home/pasta1/s0675281/SPEC2006/benchspec/CPU2006/482.sphinx3/data/test/output/an4.log an4.log
-c /disk/home/pasta1/s0675281/SPEC2006/benchspec/CPU2006/482.sphinx3/run/run_base_test_arcsim.0000 -o considered.out.cmp specperl /disk/home/pasta1/s0675281/SPEC2006/bin/specdiff -m -l 10  --reltol 0.0004  --floatcompare /disk/home/pasta1/s0675281/SPEC2006/benchspec/CPU2006/482.sphinx3/data/test/output/considered.out considered.out
-c /disk/home/pasta1/s0675281/SPEC2006/benchspec/CPU2006/482.sphinx3/run/run_base_test_arcsim.0000 -o total_considered.out.cmp specperl /disk/home/pasta1/s0675281/SPEC2006/bin/specdiff -m -l 10  --reltol 1e-06  --floatcompare /disk/home/pasta1/s0675281/SPEC2006/benchspec/CPU2006/482.sphinx3/data/test/output/total_considered.out total_considered.out