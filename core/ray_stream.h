// Copyright 2015 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sys/memory.h"
#include "ray_simd.h"

namespace prt {

constexpr int rayStreamPfL2Dist = 2*cacheLineSize; // prefetch into L2 distance

template <class T, int size>
class RayStreamChannel;

struct RayStreamId
{
  vint cur;

  prt_inline RayStreamId(vint cur) : cur(cur) {}

  template <int size>
  static prt_inline RayStreamId get(const RayStreamChannel<int,size>& ids, int i)
  {
    return RayStreamId(ids.get(i));
  }

  template <int size>
  static prt_inline RayStreamId getA(const RayStreamChannel<int,size>& ids, int i)
  {
    return RayStreamId(ids.getA(i));
  }
};

template <class T, int size>
class RayStreamChannel
{
private:
  prt_align_cache T v[size + simdSize]; // padded

public:
  prt_inline void setA(int i, const var<T>& a)
  {
    prefetchL2Ex(&v[i], rayStreamPfL2Dist);
    store(&v[i], a);
  }

  prt_inline void set(int i, const var<T>& a)
  {
    prefetchL2Ex(&v[i], rayStreamPfL2Dist);
    storeu(&v[i], a);
  }

  prt_inline void packSet(vbool m, int i, const var<T>& a)
  {
    prefetchL2Ex(&v[i], rayStreamPfL2Dist);
    compress(m, &v[i], a);
  }

  prt_inline var<T> getA(int i) const
  {
    prefetchL2(&v[i], rayStreamPfL2Dist);
    return load(&v[i]);
  }

  prt_inline var<T> get(int i) const
  {
    prefetchL2(&v[i], rayStreamPfL2Dist);
    return loadu(&v[i]);
  }

  prt_inline var<T> get(vbool m, vint i) const
  {
    return gather(m, v, i);
  }

  prt_inline var<T> get(vbool m, const RayStreamId& i) const
  {
    return get(m, i.cur);
  }

  prt_inline var<T> getUnpack(vbool m, int i) const
  {
    prefetchL2(&v[i], rayStreamPfL2Dist);
    return expand(m, &v[i]);
  }

  prt_inline const T& operator [](size_t i) const
  {
    return v[i];
  }

  prt_inline T& operator [](size_t i)
  {
    return v[i];
  }

  prt_inline const T* get() const { return v; }
  prt_inline T* get() { return v; }
};

template <class T, int size>
class RayStreamChannel2
{
private:
  typedef T V[size + simdSize]; // padded

public:
  prt_align_cache V x;
  prt_align_cache V y;

  prt_inline void setA(int i, const Vec2v<T>& a)
  {
    prefetchL2Ex(&x[i], rayStreamPfL2Dist);
    prefetchL2Ex(&y[i], rayStreamPfL2Dist);

    store(&x[i], a[0]);
    store(&y[i], a[1]);
  }

  prt_inline void set(int i, const Vec2v<T>& a)
  {
    prefetchL2Ex(&x[i], rayStreamPfL2Dist);
    prefetchL2Ex(&y[i], rayStreamPfL2Dist);

    storeu(&x[i], a[0]);
    storeu(&y[i], a[1]);
  }

  prt_inline void packSet(vbool m, int i, const Vec2v<T>& a)
  {
    prefetchL2Ex(&x[i], rayStreamPfL2Dist);
    prefetchL2Ex(&y[i], rayStreamPfL2Dist);

    compress(m, &x[i], a[0]);
    compress(m, &y[i], a[1]);
  }

  prt_inline Vec2v<T> getA(int i) const
  {
    prefetchL2(&x[i], rayStreamPfL2Dist);
    prefetchL2(&y[i], rayStreamPfL2Dist);

    return Vec2v<T>(load(&x[i]),
                    load(&y[i]));
  }

  prt_inline Vec2v<T> get(int i) const
  {
    prefetchL2(&x[i], rayStreamPfL2Dist);
    prefetchL2(&y[i], rayStreamPfL2Dist);

    return Vec3v<T>(loadu(&x[i]),
                    loadu(&y[i]));
  }

  prt_inline Vec2v<T> get(vbool m, vint i) const
  {
    return Vec2v<T>(gather(m, x, i),
                    gather(m, y, i));
  }

  prt_inline Vec2v<T> get(vbool m, const RayStreamId& i) const
  {
    return get(m, i.cur);
  }

  prt_inline Vec2v<T> getUnpack(vbool m, int i) const
  {
    prefetchL2(&x[i], rayStreamPfL2Dist);
    prefetchL2(&y[i], rayStreamPfL2Dist);

    return Vec2v<T>(expand(m, &x[i]),
                    expand(m, &y[i]));
  }

  prt_inline const T* getX() const { return x; }
  prt_inline const T* getY() const { return y; }

  prt_inline T* getX() { return x; }
  prt_inline T* getY() { return y; }
};

template <class T, int size>
class RayStreamChannel3
{
private:
  typedef T V[size + simdSize]; // padded

public:
  prt_align_cache V x;
  prt_align_cache V y;
  prt_align_cache V z;

  prt_inline void setA(int i, const Vec3v<T>& a)
  {
    prefetchL2Ex(&x[i], rayStreamPfL2Dist);
    prefetchL2Ex(&y[i], rayStreamPfL2Dist);
    prefetchL2Ex(&z[i], rayStreamPfL2Dist);

    store(&x[i], a[0]);
    store(&y[i], a[1]);
    store(&z[i], a[2]);
  }

  prt_inline void set(int i, const Vec3v<T>& a)
  {
    prefetchL2Ex(&x[i], rayStreamPfL2Dist);
    prefetchL2Ex(&y[i], rayStreamPfL2Dist);
    prefetchL2Ex(&z[i], rayStreamPfL2Dist);

    storeu(&x[i], a[0]);
    storeu(&y[i], a[1]);
    storeu(&z[i], a[2]);
  }

  prt_inline void packSet(vbool m, int i, const Vec3v<T>& a)
  {
    prefetchL2Ex(&x[i], rayStreamPfL2Dist);
    prefetchL2Ex(&y[i], rayStreamPfL2Dist);
    prefetchL2Ex(&z[i], rayStreamPfL2Dist);

    compress(m, &x[i], a[0]);
    compress(m, &y[i], a[1]);
    compress(m, &z[i], a[2]);
  }

  prt_inline Vec3v<T> getA(int i) const
  {
    prefetchL2(&x[i], rayStreamPfL2Dist);
    prefetchL2(&y[i], rayStreamPfL2Dist);
    prefetchL2(&z[i], rayStreamPfL2Dist);

    return Vec3v<T>(load(&x[i]),
            load(&y[i]),
            load(&z[i]));
  }

  prt_inline Vec3v<T> get(int i) const
  {
    prefetchL2(&x[i], rayStreamPfL2Dist);
    prefetchL2(&y[i], rayStreamPfL2Dist);
    prefetchL2(&z[i], rayStreamPfL2Dist);

    return Vec3v<T>(loadu(&x[i]),
                    loadu(&y[i]),
                    loadu(&z[i]));
  }

  prt_inline Vec3v<T> get(vbool m, vint i) const
  {
    return Vec3v<T>(gather(m, x, i),
                    gather(m, y, i),
                    gather(m, z, i));
  }

  prt_inline Vec3v<T> get(vbool m, const RayStreamId& i) const
  {
    return get(m, i.cur);
  }

  prt_inline Vec3v<T> getUnpack(vbool m, int i) const
  {
    prefetchL2(&x[i], rayStreamPfL2Dist);
    prefetchL2(&y[i], rayStreamPfL2Dist);
    prefetchL2(&z[i], rayStreamPfL2Dist);

    return Vec3v<T>(expand(m, &x[i]),
                    expand(m, &y[i]),
                    expand(m, &z[i]));
  }

  prt_inline const T* getX() const { return x; }
  prt_inline const T* getY() const { return y; }
  prt_inline const T* getZ() const { return z; }

  prt_inline T* getX() { return x; }
  prt_inline T* getY() { return y; }
  prt_inline T* getZ() { return z; }
};

template <int size>
struct RayStream
{
  RayStreamChannel3<float, size> org;
  RayStreamChannel3<float, size> dir;
  RayStreamChannel<float, size> near;
  RayStreamChannel<float, size> far;

  prt_inline float* getOrgX() { return org.getX(); }
  prt_inline float* getOrgY() { return org.getY(); }
  prt_inline float* getOrgZ() { return org.getZ(); }
  prt_inline float* getDirX() { return dir.getX(); }
  prt_inline float* getDirY() { return dir.getY(); }
  prt_inline float* getDirZ() { return dir.getZ(); }
  prt_inline float* getNear() { return near.get(); }
  prt_inline float* getFar()  { return far.get(); }

  prt_inline void setA(int i, const RaySimd& ray)
  {
    org.setA(i, ray.org);
    dir.setA(i, ray.dir);
    near.setA(i, ray.near);
    far.setA(i, ray.far);
  }

  prt_inline void set(int i, const RaySimd& ray)
  {
    org.set(i, ray.org);
    dir.set(i, ray.dir);
    near.set(i, ray.near);
    far.set(i, ray.far);
  }

  prt_inline void packSet(vbool m, int i, const RaySimd& ray)
  {
    org.packSet(m, i, ray.org);
    dir.packSet(m, i, ray.dir);
    near.packSet(m, i, ray.near);
    far.packSet(m, i, ray.far);
  }

  prt_inline void getA(int i, RaySimd& ray) const
  {
    ray.org = org.getA(i);
    ray.dir = dir.getA(i);
    ray.near = near.getA(i);
    ray.far = far.getA(i);
  }

  template <class RayStreamIdT>
  prt_inline void get(vbool m, const RayStreamIdT& i, RaySimd& ray) const
  {
    ray.org = org.get(m, i);
    ray.dir = dir.get(m, i);
    ray.near = near.get(m, i);
    ray.far = far.get(m, i);
  }

  prt_inline bool isHit(int i) const
  {
    return far[i] < float(posMax);
  }

  prt_inline vbool isHitA(int i) const
  {
    return far.getA(i) < float(posMax);
  }
};

template <int size>
struct HitStream
{
  RayStreamChannel<int, size> primId;
  RayStreamChannel<int, size> geomId;
  RayStreamChannel<int, size> instId;
  RayStreamChannel<float, size> u;
  RayStreamChannel<float, size> v;

  prt_inline int*   getPrimId() { return primId.get(); }
  prt_inline int*   getGeomId() { return geomId.get(); }
  prt_inline int*   getInstId() { return instId.get(); }
  prt_inline float* getU()      { return u.get(); }
  prt_inline float* getV()      { return v.get(); }

  prt_inline void getA(int i, HitSimd& hit) const
  {
    hit.primId = primId.getA(i);
    hit.geomId = geomId.getA(i);
    hit.instId = instId.getA(i);
    hit.u = u.getA(i);
    hit.v = v.getA(i);
  }

  template <class RayStreamIdT>
  prt_inline void get(vbool m, const RayStreamIdT& i, HitSimd& hit) const
  {
    hit.primId = primId.get(m, i);
    hit.geomId = geomId.get(m, i);
    hit.instId = instId.get(m, i);
    hit.u = u.get(m, i);
    hit.v = v.get(m, i);
  }
};

} // namespace prt
