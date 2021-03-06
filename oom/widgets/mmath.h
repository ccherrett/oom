//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//    $Id: mmath.h,v 1.1.1.1 2003/10/27 18:54:47 wschweer Exp $
//
//  (C) Copyright 2000 Werner Schweer (ws@seh.de)
//=========================================================

#ifndef __MATH_H__
#define __MATH_H__

#define LOG_MIN 1.0e-100
#define LOG_MAX 1.0e100

double qwtCeil125(double x);
double qwtFloor125(double x);
void qwtTwistArray(double *array, int size);
int qwtChkMono(double *array, int size);
void qwtLinSpace(double *array, int size, double xmin, double xmax);
void qwtLogSpace(double *array, int size, double xmin, double xmax);

template <class T>
inline int qwtSign(const T& x)
{
    if (x > T(0))
        return 1;
    else if (x < T(0))
        return (-1);
    else
        return 0;
}

inline int qwtInt(double x)
{
    return int(rint(x));
}

template <class T>
inline T qwtAbs(const T& x)
{
    return ( x > T(0) ? x : -x);
}

template <class T>
inline const T& qwtMax(const T& x, const T& y)
{
    return ( x > y ? x : y);
}

template <class T>
inline const T& qwtMin(const T& x, const T& y)
{
    return ( x < y ? x : y);
}

template <class T>
T qwtLim(const T& x, const T& x1, const T& x2)
{
    T rv;
    T xmin, xmax;

    xmin = qwtMin(x1, x2);
    xmax = qwtMax(x1, x2);

    if (x < xmin)
        rv = xmin;
    else if (x > xmax)
        rv = xmax;
    else
        rv = x;

    return rv;
}

#endif

