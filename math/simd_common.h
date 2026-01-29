// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "math.h"

namespace prt {

// Default SIMD size
#if defined(__AVX512F__)
#define PRT_SIMD_SIZE 16
const int simdSize = 16;
#else
#define PRT_SIMD_SIZE 8
const int simdSize = 8;
#endif

// Varying template
// ----------------

template <class T, int N = simdSize>
struct var;

// Functions
// ---------

template <int N = simdSize, class T>
var<T,N> load(const T* ptr);

template <int N = simdSize, class T>
var<T,N> load(const var<bool,N>& mask, const T* ptr);

template <int N = simdSize, class T>
var<T,N> load4(const T* ptr);

template <int N = simdSize, class T>
var<T,N> loadu(const T* ptr);

template <int N = simdSize, class T>
var<T,N> loadu(const var<bool,N>& mask, const T* ptr);

// Typedefs
// --------

typedef var<float,simdSize> vfloat;
typedef var<float,4>        vfloat4;
typedef var<float,8>        vfloat8;
typedef var<float,16>       vfloat16;

typedef var<int,simdSize> vint;
typedef var<int,4>        vint4;
typedef var<int,8>        vint8;
typedef var<int,16>       vint16;

typedef var<uint,simdSize> vuint;
typedef var<uint,4>        vuint4;
typedef var<uint,8>        vuint8;
typedef var<uint,16>       vuint16;

typedef var<bool,simdSize> vbool;
typedef var<bool,4>        vbool4;
typedef var<bool,8>        vbool8;
typedef var<bool,16>       vbool16;

// Traits
// ------

template <class T, int N>
struct ToFloat<var<T,N>> { typedef var<float,N> Type; };

template <class T, int N>
struct ToInt<var<T,N>> { typedef var<int,N> Type; };

template <class T, int N>
struct ToUInt<var<T,N>> { typedef var<uint,N> Type; };

template <class T, int N>
struct ToBool<var<T,N>> { typedef var<bool,N> Type; };

// Internals
// ---------

extern const uint32_t simdMaskTable4[16][4];
extern const uint32_t simdCompressTable8[256][8];
extern const uint32_t simdExpandTable8[256][8];

} // namespace prt
