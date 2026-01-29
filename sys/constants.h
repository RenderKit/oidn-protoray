// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <limits>
#include "sys/common.h"

namespace prt {

static struct Zero
{
  prt_inline operator int()      const { return 0; }
  prt_inline operator uint32_t() const { return 0; }
  prt_inline operator int8_t()   const { return 0; }
  prt_inline operator uint8_t()  const { return 0; }
  prt_inline operator int16_t()  const { return 0; }
  prt_inline operator uint16_t() const { return 0; }
  prt_inline operator int64_t()  const { return 0; }
  prt_inline operator uint64_t() const { return 0; }
  prt_inline operator float()    const { return 0; }
  prt_inline operator double()   const { return 0; }
} zero;

static struct One
{
  prt_inline operator int()      const { return 1; }
  prt_inline operator uint32_t() const { return 1; }
  prt_inline operator int8_t()   const { return 1; }
  prt_inline operator uint8_t()  const { return 1; }
  prt_inline operator int16_t()  const { return 1; }
  prt_inline operator uint16_t() const { return 1; }
  prt_inline operator int64_t()  const { return 1; }
  prt_inline operator uint64_t() const { return 1; }
  prt_inline operator float()    const { return 1; }
  prt_inline operator double()   const { return 1; }
} one;

static struct PosMax
{
  prt_inline operator float() const { return std::numeric_limits<float>::max(); }
  prt_inline operator double() const { return std::numeric_limits<double>::max(); }
} posMax;

static struct NegMax
{
  prt_inline operator float() const { return -std::numeric_limits<float>::max(); }
  prt_inline operator double() const { return -std::numeric_limits<double>::max(); }
} negMax;

static struct PosInf
{
  prt_inline operator float() const { return std::numeric_limits<float>::infinity(); }
  prt_inline operator double() const { return std::numeric_limits<double>::infinity(); }
} posInf;

static struct NegInf
{
  prt_inline operator float() const { return -std::numeric_limits<float>::infinity(); }
  prt_inline operator double() const { return -std::numeric_limits<double>::infinity(); }
} negInf;

static struct Qnan
{
  prt_inline operator float() const { return std::numeric_limits<float>::quiet_NaN(); }
  prt_inline operator double() const { return std::numeric_limits<double>::quiet_NaN(); }
} qnan;

static struct Ulp
{
  prt_inline operator float() const { return std::numeric_limits<float>::epsilon(); }
  prt_inline operator double() const { return std::numeric_limits<double>::epsilon(); }
} ulp;

static struct Step {} step;
static struct Empty {} empty;

static struct Pi
{
  prt_inline operator float() const { return 3.14159265358979323846f; }
  prt_inline operator double() const { return 3.14159265358979323846; }
} pi;

constexpr float minVectorLength    = 1e-10f;
constexpr float minVectorLengthSqr = minVectorLength * minVectorLength;

} // namespace prt
