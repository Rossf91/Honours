//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
//                                            
// =====================================================================
//
// Description:
// 
//  CCM - Closely Coupled Memory class.
//
// =====================================================================



#include "sys/mem/CcMemory.h"



CCM::CCM (uint32 base, uint32 size, bool init_with_0)
 : Ram (base, size, init_with_0) 
{ /* EMPTY */ }
