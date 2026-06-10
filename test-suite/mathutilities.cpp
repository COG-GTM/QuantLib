/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2024 StatPro Italia srl

 This file is part of QuantLib, a free-software/open-source library
 for financial quantitative analysts and developers - http://quantlib.org/

 QuantLib is free software: you can redistribute it and/or modify it
 under the terms of the QuantLib license.  You should have received a
 copy of the license along with this program; if not, please email
 <quantlib-dev@lists.sf.net>. The license is also available online at
 <https://www.quantlib.org/license.shtml>.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the license for more details.
*/

#include "toplevelfixture.hpp"
#include "utilities.hpp"
#include <ql/math/factorial.hpp>
#include <ql/math/primenumbers.hpp>
#include <ql/math/bernsteinpolynomial.hpp>
#include <ql/math/richardsonextrapolation.hpp>
#include <ql/math/comparison.hpp>
#include <ql/math/matrix.hpp>
#include <ql/math/matrixutilities/pseudosqrt.hpp>
#include <cmath>

using namespace QuantLib;
using namespace boost::unit_test_framework;

BOOST_FIXTURE_TEST_SUITE(QuantLibTests, TopLevelFixture)

BOOST_AUTO_TEST_SUITE(MathUtilitiesTests)

/* =========================================================
   Factorial
   ========================================================= */

BOOST_AUTO_TEST_CASE(testFactorialValues) {
    BOOST_TEST_MESSAGE("Testing factorial known values...");

    Real expected[] = {
        1.0,          // 0!
        1.0,          // 1!
        2.0,          // 2!
        6.0,          // 3!
        24.0,         // 4!
        120.0,        // 5!
        720.0,        // 6!
        5040.0,       // 7!
        40320.0,      // 8!
        362880.0,     // 9!
        3628800.0,    // 10!
        39916800.0,   // 11!
        479001600.0   // 12!
    };

    for (Natural n = 0; n <= 12; ++n) {
        Real calculated = Factorial::get(n);
        if (!close(calculated, expected[n]))
            BOOST_ERROR("Factorial(" << n << ") = " << calculated
                        << ", expected " << expected[n]);
    }
}

BOOST_AUTO_TEST_CASE(testFactorialRecurrence) {
    BOOST_TEST_MESSAGE("Testing factorial recurrence relation n! = n*(n-1)!...");

    for (Natural n = 1; n <= 20; ++n) {
        Real fn = Factorial::get(n);
        Real fn1 = Factorial::get(n - 1);
        Real ratio = fn / fn1;
        if (!close(ratio, Real(n)))
            BOOST_ERROR("Factorial(" << n << ")/Factorial(" << n-1
                        << ") = " << ratio << ", expected " << n);
    }
}

BOOST_AUTO_TEST_CASE(testFactorialLn) {
    BOOST_TEST_MESSAGE("Testing log-factorial vs log of factorial...");

    for (Natural n = 0; n <= 20; ++n) {
        Real lnCalc = Factorial::ln(n);
        Real lnExpected = std::log(Factorial::get(n));
        if (std::fabs(lnCalc - lnExpected) > 1.0e-10)
            BOOST_ERROR("ln(Factorial(" << n << ")) = " << lnCalc
                        << ", expected " << lnExpected);
    }
}

BOOST_AUTO_TEST_CASE(testFactorialLnLargeN) {
    BOOST_TEST_MESSAGE("Testing log-factorial for large n via Stirling approximation...");

    Natural n = 100;
    Real lnCalc = Factorial::ln(n);
    // Stirling: ln(n!) ~ n*ln(n) - n + 0.5*ln(2*pi*n)
    Real stirling = n * std::log(Real(n)) - n
                    + 0.5 * std::log(2.0 * M_PI * n);
    Real relError = std::fabs(lnCalc - stirling) / lnCalc;
    if (relError > 1.0e-3)
        BOOST_ERROR("ln(100!) relative error vs Stirling = " << relError
                    << ", expected < 0.001");
}

/* =========================================================
   PrimeNumbers
   ========================================================= */

BOOST_AUTO_TEST_CASE(testPrimeNumbersKnown) {
    BOOST_TEST_MESSAGE("Testing first known prime numbers...");

    BigNatural expected[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29,
                             31, 37, 41, 43, 47, 53, 59, 61, 67, 71};

    for (Size i = 0; i < 20; ++i) {
        BigNatural p = PrimeNumbers::get(i);
        if (p != expected[i])
            BOOST_ERROR("PrimeNumbers::get(" << i << ") = " << p
                        << ", expected " << expected[i]);
    }
}

BOOST_AUTO_TEST_CASE(testPrimeNumbersMonotonicity) {
    BOOST_TEST_MESSAGE("Testing prime numbers are strictly increasing...");

    BigNatural prev = PrimeNumbers::get(0);
    for (Size i = 1; i < 100; ++i) {
        BigNatural curr = PrimeNumbers::get(i);
        if (curr <= prev)
            BOOST_ERROR("PrimeNumbers::get(" << i << ") = " << curr
                        << " is not greater than get(" << i-1
                        << ") = " << prev);
        prev = curr;
    }
}

BOOST_AUTO_TEST_CASE(testPrimeNumbersPrimality) {
    BOOST_TEST_MESSAGE("Testing that returned primes are indeed prime...");

    for (Size i = 0; i < 50; ++i) {
        BigNatural p = PrimeNumbers::get(i);
        if (p < 2)
            BOOST_ERROR("PrimeNumbers::get(" << i << ") = " << p
                        << " is less than 2");
        bool isPrime = true;
        for (BigNatural d = 2; d * d <= p; ++d) {
            if (p % d == 0) {
                isPrime = false;
                break;
            }
        }
        if (!isPrime)
            BOOST_ERROR("PrimeNumbers::get(" << i << ") = " << p
                        << " is not prime");
    }
}

/* =========================================================
   BernsteinPolynomial
   ========================================================= */

BOOST_AUTO_TEST_CASE(testBernsteinPartitionOfUnity) {
    BOOST_TEST_MESSAGE("Testing Bernstein polynomials partition of unity...");

    Natural n = 5;
    Real testPoints[] = {0.0, 0.1, 0.25, 0.5, 0.75, 0.9, 1.0};

    for (Real x : testPoints) {
        Real sum = 0.0;
        for (Natural i = 0; i <= n; ++i) {
            sum += BernsteinPolynomial::get(i, n, x);
        }
        if (std::fabs(sum - 1.0) > 1.0e-12)
            BOOST_ERROR("Bernstein partition of unity at x=" << x
                        << ": sum = " << sum << ", expected 1.0");
    }
}

BOOST_AUTO_TEST_CASE(testBernsteinEndpoints) {
    BOOST_TEST_MESSAGE("Testing Bernstein polynomials at endpoints...");

    Natural n = 4;

    // B_{0,n}(0) = 1, B_{i,n}(0) = 0 for i>0
    for (Natural i = 0; i <= n; ++i) {
        Real val = BernsteinPolynomial::get(i, n, 0.0);
        Real expected = (i == 0) ? 1.0 : 0.0;
        if (std::fabs(val - expected) > 1.0e-12)
            BOOST_ERROR("B_{" << i << "," << n << "}(0) = " << val
                        << ", expected " << expected);
    }

    // B_{n,n}(1) = 1, B_{i,n}(1) = 0 for i<n
    for (Natural i = 0; i <= n; ++i) {
        Real val = BernsteinPolynomial::get(i, n, 1.0);
        Real expected = (i == n) ? 1.0 : 0.0;
        if (std::fabs(val - expected) > 1.0e-12)
            BOOST_ERROR("B_{" << i << "," << n << "}(1) = " << val
                        << ", expected " << expected);
    }
}

BOOST_AUTO_TEST_CASE(testBernsteinSymmetry) {
    BOOST_TEST_MESSAGE("Testing Bernstein polynomial symmetry B_{i,n}(x) = B_{n-i,n}(1-x)...");

    Natural n = 5;
    Real testPoints[] = {0.0, 0.1, 0.3, 0.5, 0.7, 0.9, 1.0};

    for (Real x : testPoints) {
        for (Natural i = 0; i <= n; ++i) {
            Real left = BernsteinPolynomial::get(i, n, x);
            Real right = BernsteinPolynomial::get(n - i, n, 1.0 - x);
            if (std::fabs(left - right) > 1.0e-12)
                BOOST_ERROR("B_{" << i << "," << n << "}(" << x
                            << ") = " << left << " != B_{" << n-i
                            << "," << n << "}(" << 1.0-x
                            << ") = " << right);
        }
    }
}

/* =========================================================
   RichardsonExtrapolation
   ========================================================= */

BOOST_AUTO_TEST_CASE(testRichardsonExtrapolationExact) {
    BOOST_TEST_MESSAGE("Testing Richardson extrapolation on a known convergent function...");

    // f(h) = sin(1.0) + h^2  (known order n=2)
    // Extrapolation should recover sin(1.0)
    auto f = [](Real h) { return std::sin(1.0) + h * h; };

    Real delta_h = 0.1;
    RichardsonExtrapolation re(f, delta_h, 2.0);
    Real calculated = re(2.0);
    Real expected = std::sin(1.0);

    if (std::fabs(calculated - expected) > 1.0e-10)
        BOOST_ERROR("Richardson extrapolation = " << calculated
                    << ", expected " << expected);
}

BOOST_AUTO_TEST_CASE(testRichardsonExtrapolationConvergence) {
    BOOST_TEST_MESSAGE("Testing Richardson extrapolation improves convergence...");

    // f(h) = cos(2.0) + 3.0*h^2 + 0.5*h^4
    auto f = [](Real h) {
        return std::cos(2.0) + 3.0 * h * h + 0.5 * h * h * h * h;
    };

    Real delta_h = 0.5;
    Real expected = std::cos(2.0);

    // Without extrapolation
    Real raw = f(delta_h);
    Real rawError = std::fabs(raw - expected);

    // With extrapolation (known order 2)
    RichardsonExtrapolation re(f, delta_h, 2.0);
    Real extrapolated = re(2.0);
    Real extrapolatedError = std::fabs(extrapolated - expected);

    if (extrapolatedError >= rawError)
        BOOST_ERROR("Richardson extrapolation did not improve: "
                    << "raw error = " << rawError
                    << ", extrapolated error = " << extrapolatedError);
}

/* =========================================================
   Matrix basic operations
   ========================================================= */

BOOST_AUTO_TEST_CASE(testMatrixConstruction) {
    BOOST_TEST_MESSAGE("Testing matrix construction and element access...");

    Matrix m1;
    if (m1.rows() != 0 || m1.columns() != 0)
        BOOST_ERROR("Default matrix is not empty");

    Matrix m2(3, 4, 1.5);
    if (m2.rows() != 3 || m2.columns() != 4)
        BOOST_ERROR("Matrix dimensions incorrect");

    for (Size i = 0; i < m2.rows(); ++i)
        for (Size j = 0; j < m2.columns(); ++j)
            if (m2[i][j] != 1.5)
                BOOST_ERROR("Matrix element [" << i << "][" << j
                            << "] = " << m2[i][j] << ", expected 1.5");
}

BOOST_AUTO_TEST_CASE(testMatrixArithmetic) {
    BOOST_TEST_MESSAGE("Testing matrix arithmetic operations...");

    Matrix a(2, 2, 0.0);
    a[0][0] = 1.0; a[0][1] = 2.0;
    a[1][0] = 3.0; a[1][1] = 4.0;

    Matrix b(2, 2, 0.0);
    b[0][0] = 5.0; b[0][1] = 6.0;
    b[1][0] = 7.0; b[1][1] = 8.0;

    // Addition
    Matrix c = a + b;
    if (!close(c[0][0], 6.0) || !close(c[0][1], 8.0) ||
        !close(c[1][0], 10.0) || !close(c[1][1], 12.0))
        BOOST_ERROR("Matrix addition failed");

    // Subtraction
    Matrix d = b - a;
    if (!close(d[0][0], 4.0) || !close(d[0][1], 4.0) ||
        !close(d[1][0], 4.0) || !close(d[1][1], 4.0))
        BOOST_ERROR("Matrix subtraction failed");

    // Scalar multiplication
    Matrix e = a * 2.0;
    if (!close(e[0][0], 2.0) || !close(e[0][1], 4.0) ||
        !close(e[1][0], 6.0) || !close(e[1][1], 8.0))
        BOOST_ERROR("Scalar multiplication failed");

    // Matrix multiplication
    Matrix f = a * b;
    // [1,2;3,4] * [5,6;7,8] = [19,22;43,50]
    if (!close(f[0][0], 19.0) || !close(f[0][1], 22.0) ||
        !close(f[1][0], 43.0) || !close(f[1][1], 50.0))
        BOOST_ERROR("Matrix multiplication failed: got ["
                    << f[0][0] << "," << f[0][1] << ";"
                    << f[1][0] << "," << f[1][1] << "]");
}

BOOST_AUTO_TEST_CASE(testMatrixTranspose) {
    BOOST_TEST_MESSAGE("Testing matrix transpose...");

    Matrix a(2, 3, 0.0);
    a[0][0] = 1.0; a[0][1] = 2.0; a[0][2] = 3.0;
    a[1][0] = 4.0; a[1][1] = 5.0; a[1][2] = 6.0;

    Matrix t = transpose(a);

    if (t.rows() != 3 || t.columns() != 2)
        BOOST_ERROR("Transpose dimensions incorrect: "
                    << t.rows() << "x" << t.columns());

    for (Size i = 0; i < a.rows(); ++i)
        for (Size j = 0; j < a.columns(); ++j)
            if (!close(a[i][j], t[j][i]))
                BOOST_ERROR("Transpose element mismatch at [" << i << "]["
                            << j << "]");
}

BOOST_AUTO_TEST_CASE(testMatrixIdentityMultiplication) {
    BOOST_TEST_MESSAGE("Testing multiplication by identity matrix...");

    Matrix a(3, 3, 0.0);
    a[0][0] = 1.0; a[0][1] = 2.0; a[0][2] = 3.0;
    a[1][0] = 4.0; a[1][1] = 5.0; a[1][2] = 6.0;
    a[2][0] = 7.0; a[2][1] = 8.0; a[2][2] = 9.0;

    Matrix id(3, 3, 0.0);
    id[0][0] = 1.0; id[1][1] = 1.0; id[2][2] = 1.0;

    Matrix result = a * id;
    if (result != a)
        BOOST_ERROR("A * I != A");

    result = id * a;
    if (result != a)
        BOOST_ERROR("I * A != A");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
