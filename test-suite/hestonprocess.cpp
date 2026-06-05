/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2024 Cognition AI

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
#include <ql/processes/hestonprocess.hpp>
#include <ql/termstructures/yield/flatforward.hpp>
#include <ql/quotes/simplequote.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>

using namespace QuantLib;
using namespace boost::unit_test_framework;

BOOST_FIXTURE_TEST_SUITE(QuantLibTests, TopLevelFixture)

BOOST_AUTO_TEST_SUITE(HestonProcessTests)

namespace hestonprocess_test {

    struct CommonVars {
        Handle<YieldTermStructure> riskFreeRate;
        Handle<YieldTermStructure> dividendYield;
        Handle<Quote> s0;
        Real v0, kappa, theta, sigma, rho;

        CommonVars() {
            Date today(15, March, 2024);
            Settings::instance().evaluationDate() = today;

            riskFreeRate = Handle<YieldTermStructure>(
                ext::make_shared<FlatForward>(today, 0.05, Actual365Fixed()));
            dividendYield = Handle<YieldTermStructure>(
                ext::make_shared<FlatForward>(today, 0.02, Actual365Fixed()));
            s0 = Handle<Quote>(ext::make_shared<SimpleQuote>(100.0));

            v0 = 0.04;
            kappa = 2.0;
            theta = 0.04;
            sigma = 0.5;
            rho = -0.7;
        }

        ext::shared_ptr<HestonProcess> makeProcess(
            HestonProcess::Discretization d =
                HestonProcess::QuadraticExponentialMartingale) const {
            return ext::make_shared<HestonProcess>(
                riskFreeRate, dividendYield, s0,
                v0, kappa, theta, sigma, rho, d);
        }
    };

}

BOOST_AUTO_TEST_CASE(testAccessors) {
    BOOST_TEST_MESSAGE("Testing HestonProcess accessors...");

    hestonprocess_test::CommonVars vars;
    auto process = vars.makeProcess();

    BOOST_CHECK_CLOSE(process->v0(), vars.v0, 1e-10);
    BOOST_CHECK_CLOSE(process->kappa(), vars.kappa, 1e-10);
    BOOST_CHECK_CLOSE(process->theta(), vars.theta, 1e-10);
    BOOST_CHECK_CLOSE(process->sigma(), vars.sigma, 1e-10);
    BOOST_CHECK_CLOSE(process->rho(), vars.rho, 1e-10);
    BOOST_CHECK_CLOSE(process->s0()->value(), 100.0, 1e-10);
}

BOOST_AUTO_TEST_CASE(testSizeAndFactors) {
    BOOST_TEST_MESSAGE("Testing HestonProcess size and factors...");

    hestonprocess_test::CommonVars vars;

    auto qe = vars.makeProcess(HestonProcess::QuadraticExponentialMartingale);
    BOOST_CHECK_EQUAL(qe->size(), Size(2));
    BOOST_CHECK_EQUAL(qe->factors(), Size(2));

    auto bk = vars.makeProcess(HestonProcess::BroadieKayaExactSchemeLobatto);
    BOOST_CHECK_EQUAL(bk->size(), Size(2));
    BOOST_CHECK_EQUAL(bk->factors(), Size(3));
}

BOOST_AUTO_TEST_CASE(testInitialValues) {
    BOOST_TEST_MESSAGE("Testing HestonProcess initial values...");

    hestonprocess_test::CommonVars vars;
    auto process = vars.makeProcess();

    Array init = process->initialValues();
    BOOST_CHECK_EQUAL(init.size(), Size(2));
    BOOST_CHECK_CLOSE(init[0], 100.0, 1e-10);
    BOOST_CHECK_CLOSE(init[1], vars.v0, 1e-10);
}

BOOST_AUTO_TEST_CASE(testDrift) {
    BOOST_TEST_MESSAGE("Testing HestonProcess drift...");

    hestonprocess_test::CommonVars vars;
    auto process = vars.makeProcess(HestonProcess::PartialTruncation);

    Array x(2);
    x[0] = std::log(100.0);
    x[1] = 0.04;

    Array d = process->drift(0.0, x);
    BOOST_CHECK_EQUAL(d.size(), Size(2));

    // drift[0] = r - q - 0.5*v
    Real expectedDrift0 = 0.05 - 0.02 - 0.5 * 0.04;
    BOOST_CHECK_CLOSE(d[0], expectedDrift0, 0.1);

    // drift[1] = kappa*(theta - v) for PartialTruncation
    Real expectedDrift1 = vars.kappa * (vars.theta - x[1]);
    BOOST_CHECK_CLOSE(d[1], expectedDrift1, 0.1);
}

BOOST_AUTO_TEST_CASE(testDriftWithNegativeVariance) {
    BOOST_TEST_MESSAGE("Testing HestonProcess drift with negative variance...");

    hestonprocess_test::CommonVars vars;

    Array x(2);
    x[0] = std::log(100.0);
    x[1] = -0.01;

    // PartialTruncation: uses v directly in kappa*(theta-v), vol=0
    auto pt = vars.makeProcess(HestonProcess::PartialTruncation);
    Array dpt = pt->drift(0.0, x);
    Real expectedDrift1 = vars.kappa * (vars.theta - x[1]);
    BOOST_CHECK_CLOSE(dpt[1], expectedDrift1, 0.1);

    // FullTruncation: vol=0, so drift1 = kappa*(theta - 0)
    auto ft = vars.makeProcess(HestonProcess::FullTruncation);
    Array dft = ft->drift(0.0, x);
    Real expectedFtDrift1 = vars.kappa * vars.theta;
    BOOST_CHECK_CLOSE(dft[1], expectedFtDrift1, 0.1);

    // Reflection: vol = -sqrt(-v)
    auto ref = vars.makeProcess(HestonProcess::Reflection);
    Array dref = ref->drift(0.0, x);
    Real vol = -std::sqrt(0.01);
    Real expectedRefDrift1 = vars.kappa * (vars.theta - vol*vol);
    BOOST_CHECK_CLOSE(dref[1], expectedRefDrift1, 0.1);
}

BOOST_AUTO_TEST_CASE(testDiffusion) {
    BOOST_TEST_MESSAGE("Testing HestonProcess diffusion matrix...");

    hestonprocess_test::CommonVars vars;
    auto process = vars.makeProcess();

    Array x(2);
    x[0] = std::log(100.0);
    x[1] = 0.04;

    Matrix diff = process->diffusion(0.0, x);
    BOOST_CHECK_EQUAL(diff.rows(), Size(2));
    BOOST_CHECK_EQUAL(diff.columns(), Size(2));

    Real vol = std::sqrt(0.04);
    BOOST_CHECK_CLOSE(diff[0][0], vol, 1e-10);
    BOOST_CHECK_CLOSE(diff[0][1], 0.0, 1e-10);
    BOOST_CHECK_CLOSE(diff[1][0], vars.sigma * vol * vars.rho, 1e-10);
    Real sqrtRhoTerm = vars.sigma * vol * std::sqrt(1.0 - vars.rho * vars.rho);
    BOOST_CHECK_CLOSE(diff[1][1], sqrtRhoTerm, 1e-10);
}

BOOST_AUTO_TEST_CASE(testApply) {
    BOOST_TEST_MESSAGE("Testing HestonProcess apply...");

    hestonprocess_test::CommonVars vars;
    auto process = vars.makeProcess();

    Array x0(2), dx(2);
    x0[0] = std::log(100.0);
    x0[1] = 0.04;
    dx[0] = 0.01;
    dx[1] = 0.001;

    Array result = process->apply(x0, dx);
    BOOST_CHECK_EQUAL(result.size(), Size(2));
    // apply: spot component is x0[0]*exp(dx[0]), variance is x0[1]+dx[1]
    BOOST_CHECK_CLOSE(result[0], x0[0] * std::exp(dx[0]), 1e-10);
    BOOST_CHECK_CLOSE(result[1], x0[1] + dx[1], 1e-10);
}

BOOST_AUTO_TEST_CASE(testEvolveQuadraticExponential) {
    BOOST_TEST_MESSAGE("Testing HestonProcess evolve with QE scheme...");

    hestonprocess_test::CommonVars vars;
    auto process = vars.makeProcess(HestonProcess::QuadraticExponential);

    Array x0(2);
    x0[0] = 100.0;
    x0[1] = vars.v0;

    Array dw(2);
    dw[0] = 0.1;
    dw[1] = 0.2;

    Array result = process->evolve(0.0, x0, 0.01, dw);
    BOOST_CHECK_EQUAL(result.size(), Size(2));
    BOOST_CHECK(result[0] > 0.0);
    BOOST_CHECK(result[1] >= 0.0);
}

BOOST_AUTO_TEST_CASE(testEvolvePartialTruncation) {
    BOOST_TEST_MESSAGE("Testing HestonProcess evolve with PartialTruncation...");

    hestonprocess_test::CommonVars vars;
    auto process = vars.makeProcess(HestonProcess::PartialTruncation);

    Array x0(2);
    x0[0] = 100.0;
    x0[1] = vars.v0;

    Array dw(2);
    dw[0] = 0.0;
    dw[1] = 0.0;

    Array result = process->evolve(0.0, x0, 0.01, dw);
    BOOST_CHECK(result[0] > 0.0);
}

BOOST_AUTO_TEST_CASE(testDiscretizationSchemes) {
    BOOST_TEST_MESSAGE("Testing HestonProcess various discretization schemes...");

    hestonprocess_test::CommonVars vars;

    std::vector<HestonProcess::Discretization> schemes = {
        HestonProcess::PartialTruncation,
        HestonProcess::FullTruncation,
        HestonProcess::Reflection,
        HestonProcess::QuadraticExponential,
        HestonProcess::QuadraticExponentialMartingale
    };

    Array x0(2);
    x0[0] = 100.0;
    x0[1] = vars.v0;

    Array dw(2);
    dw[0] = 0.1;
    dw[1] = -0.05;

    for (auto scheme : schemes) {
        auto process = vars.makeProcess(scheme);
        Array result = process->evolve(0.0, x0, 0.01, dw);
        BOOST_CHECK(result[0] > 0.0);
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
