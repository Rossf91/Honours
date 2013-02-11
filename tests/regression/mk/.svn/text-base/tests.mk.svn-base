#################################################################################
# mk/tests.mk
#
# Test binary definitions
################################################################################

################################################################################
# 'Benchmark':'# of executed instructions':'Fail or Warn upon consistency check (F|W)'
#
EEMBC_DEFAULT_ITER=                          \
	a2time01-default-iter:488927341:F          \
	aifftr01-default-iter:126654467:F          \
	aifirf01-default-iter:19488282:F           \
	aiifft01-default-iter:122143760:F          \
	autcor00-default-iter:389802673:F          \
	basefp01-default-iter:389397481:F          \
	bezier01-default-iter:574815994:F          \
	bitmnp01-default-iter:86004464:F           \
	cacheb01-default-iter:13458448:F           \
	canrdr01-default-iter:26309364:F           \
	cjpeg-default-iter:15379979678:F           \
	conven00-default-iter:289588760:F          \
	dither01-default-iter:1737243150:F         \
	djpeg-default-iter:13340652937:F           \
	fbital00-default-iter:929808652:F          \
	fft00-default-iter:54650671:F              \
	idctrn01-default-iter:29951770:F           \
	iirflt01-default-iter:41889934:F           \
	matrix01-default-iter:1861408323:F         \
	ospf-default-iter:4937091:F                \
	pktflow-default-iter:4395822:F             \
	pntrch01-default-iter:52958916:F           \
	puwmod01-default-iter:17108240:F           \
	rgbcmy01-default-iter:2073605735:F         \
	rgbhpg01-default-iter:318655386:F          \
	rgbyiq01-default-iter:399361740:F          \
	rotate01-default-iter:453987119:F          \
	routelookup-default-iter:20005912:F        \
	rspeed01-default-iter:20171779:F           \
	tblook01-default-iter:261754809:F          \
	text01-default-iter:811209623:F            \
	ttsprk01-default-iter:45070732:F           \
	viterb00-default-iter:524503493:F          \
	coremark1000-default-iter:349006189:F

EEMBC_LARGE_ITER=                         \
	a2time01-120000-iter:1182378613:F         \
	aifftr01-2000-iter:1097009731:F           \
	aifirf01-600000-iter:1168832029:F         \
	aiifft01-2000-iter:1051923424:F           \
	autcor00-15000-iter:1169362673:F          \
	basefp01-30000-iter:1168152409:F          \
	bezier01-2000-iter:1149063994:F           \
	bitmnp01-40000-iter:1146599000:F          \
	cacheb01-10000000-iter:1342104542:F       \
	canrdr01-10000000-iter:1315041407:F       \
	cjpeg-1000-iter:15379979678:F             \
	conven00-20000-iter:1158328760:F          \
	dither01-1000-iter:1737243150:F           \
	djpeg-1000-iter:13340652937:F             \
	fbital00-6000-iter:1115769652:F           \
	fft00-20000-iter:1044759671:F             \
	idctrn01-60000-iter:1098794030:F          \
	iirflt01-250000-iter:1047094989:F         \
	matrix01-101-iter:1675270000:F            \
	ospf-26000-iter:1271291691:F              \
	pktflow-25000-iter:1076477772:F           \
	pntrch01-50000-iter:1058835946:F          \
	puwmod01-6000000-iter:1025626849:F        \
	rgbcmy01-1000-iter:2073605735:F           \
	rgbhpg01-400-iter:1274562786:F            \
	rgbyiq01-400-iter:1597444740:F            \
	rotate01-2400-iter:1088933319:F           \
	routelookup-5500-iter:1095448312:F        \
	rspeed01-5000000-iter:1008410197:F        \
	tblook01-50000-iter:1308893896:F          \
	text01-1400-iter:1135346823:F             \
	ttsprk01-250000-iter:1126582732:F         \
	viterb00-6000-iter:1100533535:F           \
	coremark-3000-iter:1197045183:F

BIOPERF_DEFAULT_ITER=                      \
	clustalw:833343182:F                     \
	fasta-ssearch:18595299674:W              \
	phylip-promlk:46020931162:W              \
	grappa:493553641:F                       \
	hmmer-hmmsearch:118797407:F              \
	hmmer-hmmpfam:40880624840:W              \
	tcoffee:319332249:W                      \
	blast-blastp:1369014891:W                \
	blast-blastn:60660885:W                  \
	glimmer:2447437589:F                     \
	ce:33897449616:F
# Disable extremely long running BIOPERF tests
#
#	phylip-dnapenny:0:W
#	predator:0:W

SPEC_DEFAULT_ITER = \
   400.perlbench:542360736:W \
   401.bzip2:20161793814:F \
   403.gcc:5320884790:F \
   429.mcf:4771964279:F \
   433.milc:303526077274:W \
   445.gobmk:45647616189:W \
   453.povray:20797275551:W \
   456.hmmer:51953814038:W \
   458.sjeng:0:W \
   462.libquantum:0:W \
   464.h264ref:0:W \
   470.lbm:0:W \
   471.omnetpp:0:W \
   473.astar:0:W \
   482.sphinx3:0:W \
   483.xalancbmk:0:W
# Disable extremely long running SPEC tests
#
#   444.namd:1179951939691:W
#   447.dealII:0:W
#   450.soplex:0:W


################################################################################
# 'Benchmark':'# of executed instructions':'Fail or Warn upon consistency check (F|W)'
#
EEMBC_REGRESSION=                            \
	a2time01-default-iter:488927341:F          \
	aifftr01-default-iter:126654467:F          \
	aifirf01-default-iter:19488282:F           \
	aiifft01-default-iter:122143760:F          \
	autcor00-default-iter:389802673:F          \
	basefp01-default-iter:389397481:F          \
	bezier01-default-iter:574815994:F          \
	bitmnp01-default-iter:86004464:F           \
	cacheb01-default-iter:13458448:F           \
	canrdr01-default-iter:26309364:F           \
	cjpeg-default-iter:15379979678:F           \
	conven00-default-iter:289588760:F          \
	dither01-default-iter:1737243150:F         \
	djpeg-default-iter:13340652937:F           \
	fbital00-default-iter:929808652:F          \
	fft00-default-iter:54650671:F              \
	idctrn01-default-iter:29951770:F           \
	iirflt01-default-iter:41889934:F           \
	matrix01-default-iter:1861408323:F         \
	ospf-default-iter:4937091:F                \
	pktflow-default-iter:4395822:F             \
	pntrch01-default-iter:52958155:F           \
	puwmod01-default-iter:17108240:F           \
	rgbcmy01-default-iter:2073605735:F         \
	rgbhpg01-default-iter:318655386:F          \
	rgbyiq01-default-iter:399361740:F          \
	rotate01-default-iter:453987119:F          \
	routelookup-default-iter:20005912:F        \
	rspeed01-default-iter:20171779:F           \
	tblook01-default-iter:261754809:F          \
	text01-default-iter:811209623:F            \
	ttsprk01-default-iter:45070732:F           \
	viterb00-default-iter:524503493:F          \
	coremark1000-default-iter:349006189:F

BIOPERF_REGRESSION=                        \
	clustalw:833343182:F                     \
	grappa:493553641:F                       \
	hmmer-hmmsearch:118797407:F              \
	tcoffee:319332249:W                      \
	blast-blastp:1369014891:W                \
	blast-blastn:60660885:W                  \
	glimmer:2447437589:W

SPEC_REGRESSION=                           \
	400.perlbench:542360736:W                \
	403.gcc:5320884790:F                     \
	483.xalancbmk:542015344:W

################################################################################
# 'Benchmark':'# of cycles':'Fail or Warn upon consistency check (F|W)'
#
EEMBC_REGRESSION_CYCLES_SMALL=                            \
	a2time01-default-iter:608215768:F          \
	aifftr01-default-iter:198020534:F          \
	aifirf01-default-iter:40783723:F           \
	aiifft01-default-iter:189100101:F          \
	autcor00-default-iter:1087532977:F          \
	basefp01-default-iter:449234375:F          \
	bezier01-default-iter:920878646:F          \
	bitmnp01-default-iter:115079855:F           \
	cacheb01-default-iter:24372147:F           \
	canrdr01-default-iter:30383416:F           \
	conven00-default-iter:310107323:F          \
	fbital00-default-iter:1100499892:F          \
	fft00-default-iter:64635951:F              \
	idctrn01-default-iter:43529301:F           \
	iirflt01-default-iter:58682231:F           \
	ospf-default-iter:6658891:F               \
	pktflow-default-iter:10250304:F             \
	pntrch01-default-iter:55285040:F           \
	puwmod01-default-iter:21270016:F           \
	rgbhpg01-default-iter:511340788:F          \
	rgbyiq01-default-iter:764117292:F          \
	rotate01-default-iter:606137744:F          \
	routelookup-default-iter:21795280:F        \
	rspeed01-default-iter:25581676:F           \
	tblook01-default-iter:305879409:F          \
	text01-default-iter:953290841:F            \
	ttsprk01-default-iter:55719603:F           \
	viterb00-default-iter:583596028:F          \
	coremark1000-default-iter:488041950:F

BIOPERF_REGRESSION_CYCLES_SMALL=                 \
	clustalw:1036330531:F                    \
	grappa:566248680:F                       \
	hmmer-hmmsearch:184792682:F              \
	tcoffee:852938209:W                      \
	blast-blastn:143261851:W                  

################################################################################
# Regression tests for new SkipJack options ArcSim a.k.a. a6k:
# "'Benchmark':'Arguments':"
#
EEMBC_A6K_REGRESSION=                                  \
	"cjpeg:output:"                                      \
	"djpeg:output:"                                      \
	"filter01::"                                         \
	"rgbcmy01::"                                         \
	"rgbyiq01::"                                         \
	"ospf:output:"                                       \
	"pktflow:output:"                                    \
	"routelookup:output:"                                \
	"bezier01:output:"                                   \
	"dither01:output:"                                   \
	"rotate01:output:"                                   \
	"text01:output:"                                     \
	"autcor00:output 1:"                                 \
	"biquad00:output 1:"                                 \
	"conven00:output 1:"                                 \
	"fbital00:output 1:"                                 \
	"fft00:output Forward ' ':"                          \
	"viterb00:output 1:"


################################################################################
# Other test binaries testing things like self modifying code, timers, etc.
# "name:benchmark description:path:arguments:instruction count:(F|W)"
# 
# DISABLED: "mul64.x:A600 MUL64 Test Code:tests/tests/a600-mul64:--fast --fast-num-threads=3 --options=a600=1,mpy_option=mul64 --emt:1000000222:F"
# DISABLED: "a600-ccm-test.x:A600 CCM Test Code:tests/tests/ccm-tests:--arch=ccm.arc --fast --fast-num-threads=3 --options=a600=1 --emt:6980026577:F"
MISC_TEST_CASES=                                       \
	"self-modifying-heap.x:Self Modifying Code HEAP:tests/tests/self-modifying-code:--fast --fast-num-threads=3 --emt:180000531:F"\
	"self-modifying-text-section.x:Self Modifying Code .text:tests/tests/self-modifying-code:--fast --fast-num-threads=3 --emt:180000266:F"\
	"MyEiaSampleProg.x:EIA-Interface test:../../examples/eia/tests/src/MyEiaSampleProg:--fast --fast-num-threads=3 --eia-lib=../../../MyEiaSample/MyEiaSample.so,../../../WhizzBangExtension/EiaWhizzBangExtension.so:134217763:F"

HEX_TEST_CASES=                                       \
	"SC_read_oper_ecr_check.hex:Stack Checking Read:tests/tests/em_1.1:-o stack_checking=1,a6kv21=1:184:F" \
	"SC_write_oper_ecr_check.hex:Stack Checking Write:tests/tests/em_1.1:-o stack_checking=1,a6kv21=1:185:F" \
	"SC_exchange_ecr_check.hex:Stack Checking EX:tests/tests/em_1.1:-o stack_checking=1,a6kv21=1:56:F"  \
	"mpu_instr_exec_test_0838289988.hex:MPU Random Test1:tests/tests/em_1.1: -Z 0 --arch mpu_instr_exec_test_0838289988.hex.arc -o disable_stack_setup=1 --memory -o ignore_brk_sleep=1 -o a6kv21=1 -o addr_size=32 -o aps_feature=0 -o atomic_option=0 -o bitscan_option=1 -o code_density_option=1 -o dc_uncached_region=0 -o dc_feature_level=2 -o div_rem_option=1 -o enable_timer_0=0 -o enable_timer_1=0 -o fmt14=1 -o has_dmp_peripheral=0 -o host_timer=0 -o ic_feature_level=2 -o lpc_size=32 -o num_actionpoints=0 -o number_of_interrupts=32 -o pc_size=32 -o rgf_num_regs=32 -o rgf_num_wr_ports=2 -o shift_option=3 -o timer_0_int_level=1 -o timer_1_int_level=2 -o smart_stack_entries=0 -o swap_option=1 -o mpy_option=wlh5 :36006:F" \
	"mpu_instr_exec_test_0337631769.hex:MPU Random Test2:tests/tests/em_1.1:-Z 0 --arch mpu_instr_exec_test_0337631769.hex.arc -o disable_stack_setup=1 --memory -o ignore_brk_sleep=1 -o a6kv21=1 -o addr_size=32 -o aps_feature=0 -o atomic_option=0 -o bitscan_option=1 -o code_density_option=1 -o dc_uncached_region=0 -o dc_feature_level=2 -o div_rem_option=1 -o enable_timer_0=0 -o enable_timer_1=0 -o fmt14=1 -o has_dmp_peripheral=0 -o host_timer=0 -o ic_feature_level=2 -o lpc_size=32 -o num_actionpoints=0 -o number_of_interrupts=32 -o pc_size=32 -o rgf_num_regs=32 -o rgf_num_wr_ports=2 -o shift_option=3 -o timer_0_int_level=1 -o timer_1_int_level=2 -o smart_stack_entries=0 -o swap_option=1 -o mpy_option=wlh5:26422:F" \
	"EM_stack_excep_enter_leave_test_1009200884.hex:Stack Checking test:tests/tests/em_1.1: -Z 0 --arch EM_stack_excep_enter_leave_test_1009200884.hex.arc -o disable_stack_setup=1 --memory -o ignore_brk_sleep=1 -o a6kv21=1 -o addr_size=32 -o aps_feature=0 -o atomic_option=0 -o bitscan_option=1 -o code_density_option=1 -o stack_checking=1 -o dc_uncached_region=0 -o dc_feature_level=2 -o div_rem_option=1 -o enable_timer_0=0 -o enable_timer_1=0 -o fmt14=1 -o has_dmp_peripheral=0 -o host_timer=0 -o ic_feature_level=2 -o lpc_size=32 -o num_actionpoints=0 -o number_of_interrupts=32 -o pc_size=32 -o rgf_num_regs=32 -o rgf_num_wr_ports=2 -o shift_option=3 -o timer_0_int_level=1 -o timer_1_int_level=2 -o smart_stack_entries=0 -o swap_option=1 -o mpy_option=wlh5:2219:F"
	


################################################################################
# Multi-core benchmarks
#
MC_SPLASH_BENCHMARKS=  \
	cholesky:242         \
	fft:20000            \
	lu:200               \
	radix:60             \
	barnes:400           \
	ocean:400            \
	raytrace:100         \
	radiosity:1600       \
	volrend:200          \
	water-nsquared:500   \
	water-spatial:500

MC_MULTIBENCH_BENCHMARKS = \
	md5-32M4worker.exe:120 \
	ippktcheck-4M.exe:330 \
	rgbcmyk-sea-acorn.exe:165 \
	rotate-4Ms64.exe:650 \
	ipres-small.exe:328 \
	mpeg2-railgrind.exe:140 \
	rgbyiq03-data3.exe:159 \
	rgbhpg03-data3.exe:101 \
	cjpeg-data4.exe:164 \
	djpeg-data5.exe:166 \
	tcp-4M.exe:120 \
	mp3player-jupiter.exe:546 \
	huffde-all.exe:30 \
	x264-4Mq.exe:2468

