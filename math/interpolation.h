// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"

namespace prt {

// Catmull-Rom cubic interpolation between a1 and a2
template <class T, class S>
prt_inline T catmullRom(const T& a0, const T& a1, const T& a2, const T& a3, const S& s)
{
  S w0 = s*(-0.5f + s*(1 - 0.5f*s));
  S w1 = 1.f + s*s*(-2.5f + 1.5f*s);
  S w2 = s*(0.5f + s*(2.f - 1.5f*s));
  S w3 = s*s*(-0.5f + 0.5f*s);

  return a0*w0 + a1*w1 + a2*w2 + a3*w3;
}

// x should be between [0..size-1]
template <class T>
prt_inline T interp1DLinear(float x, const T* f, int size)
{
  float xc = clamp(x, 0.f, toFloat(size-1));
  float s = xc - floor(xc);

  int x0 = min(toInt(xc), size-1);
  int x1 = min(x0+1,      size-1);

  return lerp(f[x0], f[x1], s);
}

prt_inline vfloat interp1DLinear(vbool m, vfloat x, const float* f, int size)
{
  vfloat xc = clamp(x, vfloat(0.f), toFloat(vint(size-1)));
  vfloat s = xc - floor(xc);

  vint x0 = min(toInt(xc), size-1);
  vint x1 = min(x0+1,      size-1);

  return lerp(gather(m,f,x0), gather(m,f,x1), s);
}

template <class T>
prt_inline T interp1DCatmullRom(float x, const T* f, int size)
{
  float xc = clamp(x, 0.f, toFloat(size-1));
  float s = xc - floor(xc);

  int x1 = min(toInt(xc), size-1);
  int x0 = max(x1-1,      0);
  int x2 = min(x1+1,      size-1);
  int x3 = min(x1+2,      size-1);

  return catmullRom(f[x0], f[x1], f[x2], f[x3], s);
}

prt_inline vfloat interp1DCatmullRom(vbool m, vfloat x, const float* f, int size)
{
  vfloat xc = clamp(x, vfloat(0.f), toFloat(vint(size-1)));
  vfloat s = xc - floor(xc);

  vint x1 = min(toInt(xc), size-1);
  vint x0 = max(x1-1,      0);
  vint x2 = min(x1+1,      size-1);
  vint x3 = min(x1+2,      size-1);

  return catmullRom(gather(m,f,x0), gather(m,f,x1), gather(m,f,x2), gather(m,f,x3), s);
}

// p should be between [0..size-1]
template <class T>
prt_inline T interp2DLinear(const Vec2f& p, const T* f, const Vec2i& size)
{
  float xc = clamp(p.x, 0.f, toFloat(size.x-1));
  float yc = clamp(p.y, 0.f, toFloat(size.y-1));

  float sx = xc - floor(xc);
  float sy = yc - floor(yc);

  int x0 = min(toInt(xc), size.x-1);
  int x1 = min(x0+1,      size.x-1);

  int y0 = min(toInt(yc), size.y-1);
  int y1 = min(y0+1,      size.y-1);

  int ny = size.x;

  T f0 = lerp(f[x0+y0*ny], f[x1+y0*ny], sx);
  T f1 = lerp(f[x0+y1*ny], f[x1+y1*ny], sx);

  return lerp(f0, f1, sy);
}

prt_inline vfloat interp2DLinear(vbool m, const Vec2vf& p, const float* f, const Vec2i& size)
{
  vfloat xc = clamp(p.x, vfloat(0.f), toFloat(vint(size.x-1)));
  vfloat yc = clamp(p.y, vfloat(0.f), toFloat(vint(size.y-1)));

  vfloat sx = xc - floor(xc);
  vfloat sy = yc - floor(yc);

  vint x0 = min(toInt(xc), size.x-1);
  vint x1 = min(x0+1,      size.x-1);

  vint y0 = min(toInt(yc), size.y-1);
  vint y1 = min(y0+1,      size.y-1);

  vint ny = size.x;

  vfloat f0 = lerp(gather(m,f,x0+y0*ny), gather(m,f,x1+y0*ny), sx);
  vfloat f1 = lerp(gather(m,f,x0+y1*ny), gather(m,f,x1+y1*ny), sx);

  return lerp(f0, f1, sy);
}

template <class T>
prt_inline T interp2DCatmullRom(const Vec2f& p, const T* f, const Vec2i& size)
{
  float xc = clamp(p.x, 0.f, toFloat(size.x-1));
  float yc = clamp(p.y, 0.f, toFloat(size.y-1));

  float sx = xc - floor(xc);
  float sy = yc - floor(yc);

  int x1 = min(toInt(xc), size.x-1);
  int x0 = max(x1-1,      0);
  int x2 = min(x1+1,      size.x-1);
  int x3 = min(x1+2,      size.x-1);

  int y1 = min(toInt(yc), size.y-1);
  int y0 = max(y1-1,      0);
  int y2 = min(y1+1,      size.y-1);
  int y3 = min(y1+2,      size.y-1);

  int ny = size.x;

  T f0 = catmullRom(f[x0+y0*ny], f[x1+y0*ny], f[x2+y0*ny], f[x3+y0*ny], sx);
  T f1 = catmullRom(f[x0+y1*ny], f[x1+y1*ny], f[x2+y1*ny], f[x3+y1*ny], sx);
  T f2 = catmullRom(f[x0+y2*ny], f[x1+y2*ny], f[x2+y2*ny], f[x3+y2*ny], sx);
  T f3 = catmullRom(f[x0+y3*ny], f[x1+y3*ny], f[x2+y3*ny], f[x3+y3*ny], sx);

  return catmullRom(f0, f1, f2, f3, sy);
}

prt_inline vfloat interp2DCatmullRom(vbool m, const Vec2vf& p, const float* f, const Vec2i& size)
{
  vfloat xc = clamp(p.x, vfloat(0.f), toFloat(vint(size.x-1)));
  vfloat yc = clamp(p.y, vfloat(0.f), toFloat(vint(size.y-1)));

  vfloat sx = xc - floor(xc);
  vfloat sy = yc - floor(yc);

  vint x1 = min(toInt(xc), size.x-1);
  vint x0 = max(x1-1,      0);
  vint x2 = min(x1+1,      size.x-1);
  vint x3 = min(x1+2,      size.x-1);

  vint y1 = min(toInt(yc), size.y-1);
  vint y0 = max(y1-1,      0);
  vint y2 = min(y1+1,      size.y-1);
  vint y3 = min(y1+2,      size.y-1);

  vint ny = size.x;

  vfloat f0 = catmullRom(gather(m,f,x0+y0*ny), gather(m,f,x1+y0*ny), gather(m,f,x2+y0*ny), gather(m,f,x3+y0*ny), sx);
  vfloat f1 = catmullRom(gather(m,f,x0+y1*ny), gather(m,f,x1+y1*ny), gather(m,f,x2+y1*ny), gather(m,f,x3+y1*ny), sx);
  vfloat f2 = catmullRom(gather(m,f,x0+y2*ny), gather(m,f,x1+y2*ny), gather(m,f,x2+y2*ny), gather(m,f,x3+y2*ny), sx);
  vfloat f3 = catmullRom(gather(m,f,x0+y3*ny), gather(m,f,x1+y3*ny), gather(m,f,x2+y3*ny), gather(m,f,x3+y3*ny), sx);

  return catmullRom(f0, f1, f2, f3, sy);
}

// p should be between [0..size-1]
prt_inline float interp3DLinear(const Vec3f& p, const float* f, const Vec3i& size)
{
  float xc = clamp(p.x, 0.f, toFloat(size.x-1));
  float yc = clamp(p.y, 0.f, toFloat(size.y-1));
  float zc = clamp(p.z, 0.f, toFloat(size.z-1));

  float sx = xc - floor(xc);
  float sy = yc - floor(yc);
  float sz = zc - floor(zc);

  int x0 = min(toInt(xc), size.x-1);
  int x1 = min(x0+1,      size.x-1);

  int y0 = min(toInt(yc), size.y-1);
  int y1 = min(y0+1,      size.y-1);

  int z0 = min(toInt(zc), size.z-1);
  int z1 = min(z0+1,      size.z-1);

  int ny = size.x;
  int nz = size.x * size.y;

  float f00 = lerp(f[x0+y0*ny+z0*nz], f[x1+y0*ny+z0*nz], sx);
  float f01 = lerp(f[x0+y1*ny+z0*nz], f[x1+y1*ny+z0*nz], sx);

  float f10 = lerp(f[x0+y0*ny+z1*nz], f[x1+y0*ny+z1*nz], sx);
  float f11 = lerp(f[x0+y1*ny+z1*nz], f[x1+y1*ny+z1*nz], sx);

  float f0 = lerp(f00, f01, sy);
  float f1 = lerp(f10, f11, sy);

  return lerp(f0, f1, sz);
}

prt_inline vfloat interp3DLinear(vbool m, const Vec3vf& p, const float* f, const Vec3i& size)
{
  vfloat xc = clamp(p.x, vfloat(0.f), toFloat(vint(size.x-1)));
  vfloat yc = clamp(p.y, vfloat(0.f), toFloat(vint(size.y-1)));
  vfloat zc = clamp(p.z, vfloat(0.f), toFloat(vint(size.z-1)));

  vfloat sx = xc - floor(xc);
  vfloat sy = yc - floor(yc);
  vfloat sz = zc - floor(zc);

  vint x0 = min(toInt(xc), size.x-1);
  vint x1 = min(x0+1,      size.x-1);

  vint y0 = min(toInt(yc), size.y-1);
  vint y1 = min(y0+1,      size.y-1);

  vint z0 = min(toInt(zc), size.z-1);
  vint z1 = min(z0+1,      size.z-1);

  vint ny = size.x;
  vint nz = size.x * size.y;

  vfloat f00 = lerp(gather(m,f,x0+y0*ny+z0*nz), gather(m,f,x1+y0*ny+z0*nz), sx);
  vfloat f01 = lerp(gather(m,f,x0+y1*ny+z0*nz), gather(m,f,x1+y1*ny+z0*nz), sx);

  vfloat f10 = lerp(gather(m,f,x0+y0*ny+z1*nz), gather(m,f,x1+y0*ny+z1*nz), sx);
  vfloat f11 = lerp(gather(m,f,x0+y1*ny+z1*nz), gather(m,f,x1+y1*ny+z1*nz), sx);

  vfloat f0 = lerp(f00, f01, sy);
  vfloat f1 = lerp(f10, f11, sy);

  return lerp(f0, f1, sz);
}

prt_inline float interp4DLinear(const Vec4f& p, const float* f, const Vec4i& size)
{
  float xc = clamp(p.x, 0.f, toFloat(size.x-1));
  float yc = clamp(p.y, 0.f, toFloat(size.y-1));
  float zc = clamp(p.z, 0.f, toFloat(size.z-1));
  float wc = clamp(p.w, 0.f, toFloat(size.w-1));

  float sx = xc - floor(xc);
  float sy = yc - floor(yc);
  float sz = zc - floor(zc);
  float sw = wc - floor(wc);

  int x0 = min(toInt(xc), size.x-1);
  int x1 = min(x0+1,      size.x-1);

  int y0 = min(toInt(yc), size.y-1);
  int y1 = min(y0+1,      size.y-1);

  int z0 = min(toInt(zc), size.z-1);
  int z1 = min(z0+1,      size.z-1);

  int w0 = min(toInt(wc), size.w-1);
  int w1 = min(w0+1,      size.w-1);

  int ny = size.x;
  int nz = size.x * size.y;
  int nw = size.x * size.y * size.z;

  float f000 = lerp(f[x0+y0*ny+z0*nz+w0*nw], f[x1+y0*ny+z0*nz+w0*nw], sx);
  float f001 = lerp(f[x0+y1*ny+z0*nz+w0*nw], f[x1+y1*ny+z0*nz+w0*nw], sx);
  float f010 = lerp(f[x0+y0*ny+z1*nz+w0*nw], f[x1+y0*ny+z1*nz+w0*nw], sx);
  float f011 = lerp(f[x0+y1*ny+z1*nz+w0*nw], f[x1+y1*ny+z1*nz+w0*nw], sx);

  float f100 = lerp(f[x0+y0*ny+z0*nz+w1*nw], f[x1+y0*ny+z0*nz+w1*nw], sx);
  float f101 = lerp(f[x0+y1*ny+z0*nz+w1*nw], f[x1+y1*ny+z0*nz+w1*nw], sx);
  float f110 = lerp(f[x0+y0*ny+z1*nz+w1*nw], f[x1+y0*ny+z1*nz+w1*nw], sx);
  float f111 = lerp(f[x0+y1*ny+z1*nz+w1*nw], f[x1+y1*ny+z1*nz+w1*nw], sx);

  float f00 = lerp(f000, f001, sy);
  float f01 = lerp(f010, f011, sy);
  float f10 = lerp(f100, f101, sy);
  float f11 = lerp(f110, f111, sy);

  float f0 = lerp(f00, f01, sz);
  float f1 = lerp(f10, f11, sz);

  return lerp(f0, f1, sw);
}

prt_inline vfloat interp4DLinear(vbool m, const Vec4vf& p, const float* f, const Vec4i& size)
{
  vfloat xc = clamp(p.x, vfloat(0.f), toFloat(vint(size.x-1)));
  vfloat yc = clamp(p.y, vfloat(0.f), toFloat(vint(size.y-1)));
  vfloat zc = clamp(p.z, vfloat(0.f), toFloat(vint(size.z-1)));
  vfloat wc = clamp(p.w, vfloat(0.f), toFloat(vint(size.w-1)));

  vfloat sx = xc - floor(xc);
  vfloat sy = yc - floor(yc);
  vfloat sz = zc - floor(zc);
  vfloat sw = wc - floor(wc);

  vint x0 = min(toInt(xc), size.x-1);
  vint x1 = min(x0+1,      size.x-1);

  vint y0 = min(toInt(yc), size.y-1);
  vint y1 = min(y0+1,      size.y-1);

  vint z0 = min(toInt(zc), size.z-1);
  vint z1 = min(z0+1,      size.z-1);

  vint w0 = min(toInt(wc), size.w-1);
  vint w1 = min(w0+1,      size.w-1);

  vint ny = size.x;
  vint nz = size.x * size.y;
  vint nw = size.x * size.y * size.z;

  vfloat f000 = lerp(gather(m,f,x0+y0*ny+z0*nz+w0*nw), gather(m,f,x1+y0*ny+z0*nz+w0*nw), sx);
  vfloat f001 = lerp(gather(m,f,x0+y1*ny+z0*nz+w0*nw), gather(m,f,x1+y1*ny+z0*nz+w0*nw), sx);
  vfloat f010 = lerp(gather(m,f,x0+y0*ny+z1*nz+w0*nw), gather(m,f,x1+y0*ny+z1*nz+w0*nw), sx);
  vfloat f011 = lerp(gather(m,f,x0+y1*ny+z1*nz+w0*nw), gather(m,f,x1+y1*ny+z1*nz+w0*nw), sx);

  vfloat f100 = lerp(gather(m,f,x0+y0*ny+z0*nz+w1*nw), gather(m,f,x1+y0*ny+z0*nz+w1*nw), sx);
  vfloat f101 = lerp(gather(m,f,x0+y1*ny+z0*nz+w1*nw), gather(m,f,x1+y1*ny+z0*nz+w1*nw), sx);
  vfloat f110 = lerp(gather(m,f,x0+y0*ny+z1*nz+w1*nw), gather(m,f,x1+y0*ny+z1*nz+w1*nw), sx);
  vfloat f111 = lerp(gather(m,f,x0+y1*ny+z1*nz+w1*nw), gather(m,f,x1+y1*ny+z1*nz+w1*nw), sx);

  vfloat f00 = lerp(f000, f001, sy);
  vfloat f01 = lerp(f010, f011, sy);
  vfloat f10 = lerp(f100, f101, sy);
  vfloat f11 = lerp(f110, f111, sy);

  vfloat f0 = lerp(f00, f01, sz);
  vfloat f1 = lerp(f10, f11, sz);

  return lerp(f0, f1, sw);
}

} // namespace prt
