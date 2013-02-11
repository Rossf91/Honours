//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// Type for dynamically-compiled block functions.
//
// =====================================================================


#ifndef INC_TRANSLATE_TRANSLATION_H_
#define INC_TRANSLATE_TRANSLATION_H_

// -----------------------------------------------------------------------------
// Forward declare cpuState structure
//
struct cpuState;

// -----------------------------------------------------------------------------
// Type for all dynamically-compiled block functions.
//
typedef void (*TranslationBlock)(cpuState * const);


#endif /* INC_TRANSLATE_TRANSLATION_H_ */
