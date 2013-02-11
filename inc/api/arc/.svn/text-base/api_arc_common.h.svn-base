// =====================================================================
//
// SYNOPSYS CONFIDENTIAL - This is an unpublished, proprietary work of
// Synopsys, Inc., and is fully protected under copyright and trade
// secret laws. You may not view, use, disclose, copy, or distribute
// this file or any information contained herein except pursuant to a
// valid written license from Synopsys.
//
// =====================================================================

#ifndef INC_API_ARC_APIARCCOMMON_H_
#define INC_API_ARC_APIARCCOMMON_H_

#if __GNUC__ && \
  (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 3))

   // Stupid GCC versions prior to 4.3.x seem to choke on CHECK_OVERRIDES,
   // but 4.3.x and later seem to handle it ok though.
   // We'll just skip checking for overrides if we detect a GGC
   // version prior to 4.3.x.
#  define CHECK_ARC_OVERRIDES(x)

#else
#  define CHECK_ARC_OVERRIDES(x) CHECK_OVERRIDES(x)
#endif

/* NCE 103910: 
 *   The product name changed.
 *   To avoid a lot of work if it ever changes again in the future,
 *   define some macro's here to contain the product name and that
 *   can be used throughout the source base.
 */
#define SNPS_PRODUCT_NAME      nSIM
#define SNPS_PRODUCT_NAME_UPC  NSIM
#define SNPS_PRODUCT_NAME_LWC  nsim
 
#define SNPS_PASTER(x,y) x ## _ ## y
#define SNPS_EVALUATOR(x,y)  SNPS_PASTER(x,y)

#define SNPS_PROD_STR(x) SNPS_EVALUATOR(SNPS_PRODUCT_NAME, x)
#define SNPS_PROD_STR_UPC(x) SNPS_EVALUATOR(SNPS_PRODUCT_NAME_UPC, x)
#define SNPS_PROD_STR_LWC(x) SNPS_EVALUATOR(SNPS_PRODUCT_NAME_LWC, x)

#endif  // INC_API_ARC_APIARCCOMMON_H_

