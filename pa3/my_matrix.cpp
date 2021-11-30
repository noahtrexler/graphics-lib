#include "include/GPoint.h"
#include "include/GMatrix.h"

#include "my_utils.h"

#include <stdio.h>
#include <math.h>


/** Set the elements of the matrix.
*
*  [ a  b  c ]        [ 0  1  2 ]
*  [ d  e  f ] ~~~~~> [ 3  4  5 ]
*  [ 0  0  1 ]        [ x  x  x ]
*/

// initialize to identity
GMatrix::GMatrix() : GMatrix(1,0,0,0,1,0) {}

GMatrix GMatrix::Translate(float tx, float ty) {
    return GMatrix(1,0,tx,0,1,ty);
}

GMatrix GMatrix::Scale(float sx, float sy) {
    return GMatrix(sx,0,0,0,sy,0);
}

GMatrix GMatrix::Rotate(float radians) {
    return GMatrix(cos(radians), -sin(radians), 0, sin(radians), cos(radians), 0);
}

// Return the product of two matrices: a * b
GMatrix GMatrix::Concat(const GMatrix& A, const GMatrix& B) {
    // check order of A and B
    return GMatrix(
        (A[0] * B[0] + A[1] * B[3]), // a1*a2 + b1*d2 + c1*0
        (A[0] * B[1] + A[1] * B[4]), // a1*b2 + b1*e2 + c1*0
        (A[0] * B[2] + A[1] * B[5] + A[2]), // a1*c2 + b1*f2 + c1*1
        (A[3] * B[0] + A[4] * B[3]), // d1*a2 + e1*d2 + f1*0
        (A[3] * B[1] + A[4] * B[4]), // d1*b2 + e1*e2 + f1*0
        (A[3] * B[2] + A[4] * B[5] + A[5])  // d1*c2 + e1*f2 + f1*1
    );
}

/*
*  Compute the inverse of fMat matrix, and store it in the "inverse" parameter, being
*  careful to handle the case where 'inverse' might alias fMat matrix.
*
*  If fMat matrix is invertible, return true. If not, return false, and ignore the
*  'inverse' parameter.
*/
bool GMatrix::invert(GMatrix* inverse) const { 
    float determinant;
    float detinv;
    float inv[6];

    determinant = fMat[1] * fMat[3] - fMat[0] * fMat[4]; // |A| = bd - ea
    if (determinant == 0) return false;

    detinv = 1/determinant;

    inv[0] = (-1 * fMat[4]) * detinv; //  -e
    inv[1] = (fMat[1]) * detinv; // b
    inv[2] = (-1 * fMat[1] * fMat[5] + fMat[4] * fMat[2]) * detinv; //  -bf + ec
    
    inv[3] = (fMat[3])  * detinv; // d
    inv[4] = (-1 * fMat[0]) * detinv; //  -a
    inv[5] = (-1 * fMat[2] * fMat[3] + fMat[0] * fMat[5]) * detinv; // -cd + af

    inverse->fMat[0] = inv[0];
    inverse->fMat[1] = inv[1];
    inverse->fMat[2] = inv[2];
    inverse->fMat[3] = inv[3];
    inverse->fMat[4] = inv[4];
    inverse->fMat[5] = inv[5];

    return true;
}

/**
 *  Transform the set of points in src, storing the resulting points in dst, by applying fMat
 *  matrix. It is the caller's responsibility to allocate dst to be at least as large as src.
 *
 *  [ a  b  c ] [ x ]     x' = ax + by + c
 *  [ d  e  f ] [ y ]     y' = dx + ey + f
 *  [ 0  0  1 ] [ 1 ]
 *
 *  Note: It is legal for src and dst to point to the same memory (however, they may not
 *  partially overlap). Thus the following is supported.
 *
 *  GPoint pts[] = { ... };
 *  matrix.mapPoints(pts, pts, count);
 */
void GMatrix::mapPoints(GPoint dst[], const GPoint src[], int count) const {
    for (int i = 0; i < count; i++) {
        float x_ = (fMat[0] * src[i].fX) + (fMat[1] * src[i].fY) + fMat[2];
        float y_ = (fMat[3] * src[i].fX) + (fMat[4] * src[i].fY) + fMat[5];
        dst[i] = GPoint::Make(x_, y_);
    }
}