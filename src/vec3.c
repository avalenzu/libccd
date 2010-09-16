/***
 * libgjk
 * ---------------------------------
 * Copyright (c)2010 Daniel Fiser <danfis@danfis.cz>
 *
 *
 *  This file is part of libgjk.
 *
 *  libgjk is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 3 of
 *  the License, or (at your option) any later version.
 *
 *  libgjk is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include "vec3.h"
#include "dbg.h"

static GJK_VEC3(__gjk_vec3_origin, GJK_ZERO, GJK_ZERO, GJK_ZERO);
gjk_vec3_t *gjk_vec3_origin = &__gjk_vec3_origin;


_gjk_inline gjk_real_t __gjkVec3PointSegmentDist2(const gjk_vec3_t *P,
                                                  const gjk_vec3_t *x0,
                                                  const gjk_vec3_t *b,
                                                  gjk_vec3_t *witness)
{
    // The computation comes from solving equation of segment:
    //      S(t) = x0 + t.d
    //          where - x0 is initial point of segment
    //                - d is direction of segment from x0 (|d| > 0)
    //                - t belongs to <0, 1> interval
    // 
    // Than, distance from a segment to some point P can be expressed:
    //      D(t) = |x0 + t.d - P|^2
    //          which is distance from any point on segment. Minimization
    //          of this function brings distance from P to segment.
    // Minimization of D(t) leads to simple quadratic equation that's
    // solving is straightforward.
    //
    // Bonus of this method is witness point for free.

    gjk_real_t dist, t;
    gjk_vec3_t d, a;

    // direction of segment
    gjkVec3Sub2(&d, b, x0);

    // precompute vector from P to x0
    gjkVec3Sub2(&a, x0, P);

    t  = -GJK_REAL(1.) * gjkVec3Dot(&a, &d);
    t /= gjkVec3Len2(&d);

    if (t < GJK_ZERO || gjkIsZero(t)){
        dist = gjkVec3Dist2(x0, P);
        if (witness)
            gjkVec3Copy(witness, x0);
    }else if (t > GJK_ONE || gjkEq(t, GJK_ONE)){
        dist = gjkVec3Dist2(b, P);
        if (witness)
            gjkVec3Copy(witness, b);
    }else{
        if (witness){
            gjkVec3Copy(witness, &d);
            gjkVec3Scale(witness, t);
            gjkVec3Add(witness, x0);
            dist = gjkVec3Dist2(witness, P);
        }else{
            // recycling variables
            gjkVec3Scale(&d, t);
            gjkVec3Add(&d, &a);
            dist = gjkVec3Len2(&d);
        }
    }

    return dist;
}

gjk_real_t gjkVec3PointSegmentDist2(const gjk_vec3_t *P,
                                    const gjk_vec3_t *x0, const gjk_vec3_t *b,
                                    gjk_vec3_t *witness)
{
    return __gjkVec3PointSegmentDist2(P, x0, b, witness);
}

gjk_real_t gjkVec3PointTriDist2(const gjk_vec3_t *P,
                                const gjk_vec3_t *x0, const gjk_vec3_t *B,
                                const gjk_vec3_t *C,
                                gjk_vec3_t *witness)
{
    // Computation comes from analytic expression for triangle (x0, B, C)
    //      T(s, t) = x0 + s.d1 + t.d2, where d1 = B - x0 and d2 = C - x0 and
    // Then equation for distance is:
    //      D(s, t) = | T(s, t) - P |^2
    // This leads to minimization of quadratic function of two variables.
    // The solution from is taken only if s is between 0 and 1, t is
    // between 0 and 1 and t + s < 1, otherwise distance from segment is
    // computed.

    gjk_vec3_t d1, d2, a;
    gjk_real_t u, v, w, p, q, r;
    gjk_real_t s, t, dist, dist2;
    gjk_vec3_t witness2;

    gjkVec3Sub2(&d1, B, x0);
    gjkVec3Sub2(&d2, C, x0);
    gjkVec3Sub2(&a, x0, P);

    u = gjkVec3Dot(&a, &a);
    v = gjkVec3Dot(&d1, &d1);
    w = gjkVec3Dot(&d2, &d2);
    p = gjkVec3Dot(&a, &d1);
    q = gjkVec3Dot(&a, &d2);
    r = gjkVec3Dot(&d1, &d2);

    s = (q * r - w * p) / (w * v - r * r);
    t = (-s * r - q) / w;

    if ((gjkIsZero(s) || s > GJK_ZERO)
            && (gjkEq(s, GJK_ONE) || s < GJK_ONE)
            && (gjkIsZero(t) || t > GJK_ZERO)
            && (gjkEq(t, GJK_ONE) || t < GJK_ONE)
            && (gjkEq(t + s, GJK_ONE) || t + s < GJK_ONE)){

        if (witness){
            gjkVec3Scale(&d1, s);
            gjkVec3Scale(&d2, t);
            gjkVec3Copy(witness, x0);
            gjkVec3Add(witness, &d1);
            gjkVec3Add(witness, &d2);

            dist = gjkVec3Dist2(witness, P);
        }else{
            dist  = s * s * v;
            dist += t * t * w;
            dist += GJK_REAL(2.) * s * t * r;
            dist += GJK_REAL(2.) * s * p;
            dist += GJK_REAL(2.) * t * q;
            dist += u;
        }
    }else{
        dist = __gjkVec3PointSegmentDist2(P, x0, B, witness);

        dist2 = __gjkVec3PointSegmentDist2(P, x0, C, &witness2);
        if (dist2 < dist){
            dist = dist2;
            if (witness)
                gjkVec3Copy(witness, &witness2);
        }

        dist2 = __gjkVec3PointSegmentDist2(P, B, C, &witness2);
        if (dist2 < dist){
            dist = dist2;
            if (witness)
                gjkVec3Copy(witness, &witness2);
        }
    }

    return dist;
}
