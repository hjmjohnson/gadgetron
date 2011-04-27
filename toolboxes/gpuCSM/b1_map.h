#pragma once

#include "vector_td.h"
#include "cuNDArray.h"
#include <memory>

//
// Estimate b1 map
//

template<class REAL, unsigned int D> std::auto_ptr< cuNDArray<typename complext<REAL>::Type> >
estimate_b1_map( cuNDArray<typename complext<REAL>::Type> *data );
