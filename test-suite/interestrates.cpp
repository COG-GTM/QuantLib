/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2004 Ferdinando Ametrano

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
#include <ql/interestrate.hpp>
#include <ql/math/rounding.hpp>
#include <ql/math/comparison.hpp>
#include <ql/time/daycounters/actual360.hpp>
#include <ql/utilities/dataformatters.hpp>
#include <iomanip>

using namespace QuantLib;
using namespace boost::unit_test_framework;

BOOST_FIXTURE_TEST_SUITE(QuantLibTests, TopLevelFixture)

BOOST_AUTO_TEST_SUITE(InterestRateTests)

struct InterestRateData {
    Rate r;
    Compounding comp;
    Frequency freq;
    Time t;
    Compounding comp2;
    Frequency freq2;
    Rate expected;
    Size precision;
};

BOOST_AUTO_TEST_CASE(testConversions) {
    BOOST_TEST_MESSAGE("Testing interest-rate conversions...");

    InterestRateData cases[] = {
        // data from "Option Pricing Formulas", Haug, pag.181-182
        // Rate,Compounding,        Frequency,   Time, Compounding2,      Frequency2,  Rate2, precision
        {0.0800, Compounded,        Quarterly,   1.00, Continuous,            Annual, 0.0792, 4},
        {0.1200, Continuous,           Annual,   1.00, Compounded,            Annual, 0.1275, 4},
        {0.0800, Compounded,        Quarterly,   1.00, Compounded,            Annual, 0.0824, 4},
        {0.0700, Compounded,        Quarterly,   1.00, Compounded,        Semiannual, 0.0706, 4},
        // undocumented, but reasonable :)
        {0.0100, Compounded,           Annual,   1.00,     Simple,            Annual, 0.0100, 4},
        {0.0200,     Simple,           Annual,   1.00, Compounded,            Annual, 0.0200, 4},
        {0.0300, Compounded,       Semiannual,   0.50,     Simple,            Annual, 0.0300, 4},
        {0.0400,     Simple,           Annual,   0.50, Compounded,        Semiannual, 0.0400, 4},
        {0.0500, Compounded, EveryFourthMonth,  1.0/3,     Simple,            Annual, 0.0500, 4},
        {0.0600,     Simple,           Annual,  1.0/3, Compounded,  EveryFourthMonth, 0.0600, 4},
        {0.0500, Compounded,        Quarterly,   0.25,     Simple,            Annual, 0.0500, 4},
        {0.0600,     Simple,           Annual,   0.25, Compounded,         Quarterly, 0.0600, 4},
        {0.0700, Compounded,        Bimonthly,  1.0/6,     Simple,            Annual, 0.0700, 4},
        {0.0800,     Simple,           Annual,  1.0/6, Compounded,         Bimonthly, 0.0800, 4},
        {0.0900, Compounded,          Monthly, 1.0/12,     Simple,            Annual, 0.0900, 4},
        {0.1000,     Simple,           Annual, 1.0/12, Compounded,           Monthly, 0.1000, 4},

        {0.0300, SimpleThenCompounded,       Semiannual,   0.25,               Simple,            Annual, 0.0300, 4},
        {0.0300, SimpleThenCompounded,       Semiannual,   0.25,               Simple,        Semiannual, 0.0300, 4},
        {0.0300, SimpleThenCompounded,       Semiannual,   0.25,               Simple,         Quarterly, 0.0300, 4},
        {0.0300, SimpleThenCompounded,       Semiannual,   0.50,               Simple,            Annual, 0.0300, 4},
        {0.0300, SimpleThenCompounded,       Semiannual,   0.50,               Simple,        Semiannual, 0.0300, 4},
        {0.0300, SimpleThenCompounded,       Semiannual,   0.75,           Compounded,        Semiannual, 0.0300, 4},

        {0.0400,               Simple,       Semiannual,   0.25, SimpleThenCompounded,         Quarterly, 0.0400, 4},
        {0.0400,               Simple,       Semiannual,   0.25, SimpleThenCompounded,        Semiannual, 0.0400, 4},
        {0.0400,               Simple,       Semiannual,   0.25, SimpleThenCompounded,            Annual, 0.0400, 4},

        {0.0400,           Compounded,        Quarterly,   0.50, SimpleThenCompounded,         Quarterly, 0.0400, 4},
        {0.0400,               Simple,       Semiannual,   0.50, SimpleThenCompounded,        Semiannual, 0.0400, 4},
        {0.0400,               Simple,       Semiannual,   0.50, SimpleThenCompounded,            Annual, 0.0400, 4},

        {0.0400,           Compounded,        Quarterly,   0.75, SimpleThenCompounded,         Quarterly, 0.0400, 4},
        {0.0400,           Compounded,       Semiannual,   0.75, SimpleThenCompounded,        Semiannual, 0.0400, 4},
        {0.0400,               Simple,       Semiannual,   0.75, SimpleThenCompounded,            Annual, 0.0400, 4}
    };

    Rounding roundingPrecision;
    Rate r3, r2;
    Date d1 = Date::todaysDate(), d2;
    InterestRate ir, ir2, ir3, expectedIR;
    Real compoundf, error;
    DiscountFactor disc;


    for (auto& i : cases) {
        ir = InterestRate(i.r, Actual360(), i.comp, i.freq);
        d2 = d1 + timeToDays(i.t);
        roundingPrecision = Rounding(i.precision);

        // check that the compound factor is the inverse of the discount factor
        compoundf = ir.compoundFactor(d1, d2);
        disc = ir.discountFactor(d1, d2);
        error = std::fabs(disc-1.0/compoundf);
        if (error>1e-15)
            BOOST_FAIL("\n  " << ir
                       << std::setprecision(16)
                       << "\n  1.0/compound_factor: " << 1.0/compoundf
                       << "\n  discount_factor:     " << disc
                       << "\n  error:               " << error);

        // check that the equivalent InterestRate with *same* daycounter,
        // compounding, and frequency is the *same* InterestRate
        ir2 = ir.equivalentRate(ir.dayCounter(),
                                ir.compounding(),
                                ir.frequency(),
                                d1, d2);
        error = std::fabs(ir.rate()-ir2.rate());
        if (error>1e-15)
            BOOST_FAIL(std::setprecision(12)
                       << "\n    original interest rate: " << ir
                       << "\n  equivalent interest rate: " << ir2
                       << "\n                rate error: " << error);
        if (ir.dayCounter()!=ir2.dayCounter())
            BOOST_FAIL("\n day counter error"
                       << "\n original interest rate:   " << ir
                       << "\n equivalent interest rate: " << ir2);
        if (ir.compounding()!=ir2.compounding())
            BOOST_FAIL("\n compounding error"
                       << "\n original interest rate:   " << ir
                       << "\n equivalent interest rate: " << ir2);
        if (ir.frequency()!=ir2.frequency())
            BOOST_FAIL("\n frequency error"
                       << "\n    original interest rate: " << ir
                       << "\n  equivalent interest rate: " << ir2);

        // check that the equivalent rate with *same* daycounter,
        // compounding, and frequency is the *same* rate
        r2 = ir.equivalentRate(ir.dayCounter(),
                               ir.compounding(),
                               ir.frequency(),
                               d1, d2);
        error = std::fabs(ir.rate()-r2);
        if (error>1e-15)
            BOOST_FAIL(std::setprecision(12)
                       << "\n    original rate: " << ir
                       << "\n  equivalent rate: " << io::rate(r2)
                       << "\n            error: " << error);

        // check that the equivalent InterestRate with *different*
        // compounding, and frequency is the *expected* InterestRate
        ir3 = ir.equivalentRate(ir.dayCounter(), i.comp2, i.freq2, d1, d2);
        expectedIR = InterestRate(i.expected, ir.dayCounter(), i.comp2, i.freq2);
        r3 = roundingPrecision(ir3.rate());
        error = std::fabs(r3-expectedIR.rate());
        if (error>1.0e-17)
            BOOST_FAIL(std::setprecision(i.precision + 1)
                       << "\n               original interest rate: " << ir
                       << "\n  calculated equivalent interest rate: " << ir3
                       << "\n            truncated equivalent rate: " << io::rate(r3)
                       << "\n    expected equivalent interest rate: " << expectedIR
                       << "\n                           rate error: " << error);
        if (ir3.dayCounter()!=expectedIR.dayCounter())
            BOOST_FAIL("\n day counter error"
                       << "\n    original interest rate: " << ir3
                       << "\n  equivalent interest rate: " << expectedIR);
        if (ir3.compounding()!=expectedIR.compounding())
            BOOST_FAIL("\n compounding error"
                       << "\n    original interest rate: " << ir3
                       << "\n  equivalent interest rate: " << expectedIR);
        if (ir3.frequency()!=expectedIR.frequency())
            BOOST_FAIL("\n frequency error"
                       << "\n    original interest rate: " << ir3
                       << "\n  equivalent interest rate: " << expectedIR);

        // check that the equivalent rate with *different*
        // compounding, and frequency is the *expected* rate
        r3 = ir.equivalentRate(ir.dayCounter(), i.comp2, i.freq2, d1, d2);
        r3 = roundingPrecision(r3);
        error = std::fabs(r3 - i.expected);
        if (error>1.0e-17)
            BOOST_FAIL(std::setprecision(i.precision - 2)
                       << "\n  calculated equivalent rate: " << io::rate(r3)
                       << "\n    expected equivalent rate: " << io::rate(i.expected)
                       << "\n                       error: " << error);
    }
}

BOOST_AUTO_TEST_CASE(testCompoundFactorContinuous) {
    BOOST_TEST_MESSAGE("Testing continuous compound factor...");

    DayCounter dc = Actual360();
    Rate r = 0.05;
    InterestRate ir(r, dc, Continuous, Annual);

    // Continuous: CF(t) = exp(r*t)
    Time t = 2.0;
    Real calculated = ir.compoundFactor(t);
    Real expected = std::exp(r * t);
    Real error = std::fabs(calculated - expected);
    if (error > 1.0e-15)
        BOOST_ERROR("Continuous compound factor: " << calculated
                    << " vs exp(r*t) = " << expected
                    << ", error = " << error);
}

BOOST_AUTO_TEST_CASE(testDiscountFactorInverse) {
    BOOST_TEST_MESSAGE("Testing discount factor is inverse of compound factor...");

    DayCounter dc = Actual360();
    Rate rates[] = {0.01, 0.05, 0.10, 0.15};
    Time times[] = {0.25, 0.5, 1.0, 2.0, 5.0, 10.0};

    for (Rate r : rates) {
        for (Time t : times) {
            InterestRate ir(r, dc, Compounded, Semiannual);
            Real cf = ir.compoundFactor(t);
            DiscountFactor df = ir.discountFactor(t);
            Real error = std::fabs(df * cf - 1.0);
            if (error > 1.0e-14)
                BOOST_ERROR("df * cf = " << df * cf
                            << " for rate " << r << " at t=" << t
                            << ", error = " << error);
        }
    }
}

BOOST_AUTO_TEST_CASE(testImpliedRate) {
    BOOST_TEST_MESSAGE("Testing implied rate from compound factor...");

    DayCounter dc = Actual360();
    Compounding comp = Compounded;
    Frequency freq = Semiannual;

    Rate originalRate = 0.06;
    Time t = 1.5;

    InterestRate ir(originalRate, dc, comp, freq);
    Real cf = ir.compoundFactor(t);

    InterestRate implied = InterestRate::impliedRate(cf, dc, comp, freq, t);
    Real error = std::fabs(implied.rate() - originalRate);
    if (error > 1.0e-12)
        BOOST_ERROR("Implied rate " << implied.rate()
                    << " differs from original " << originalRate
                    << ", error = " << error);
}

BOOST_AUTO_TEST_CASE(testZeroRateCompoundFactor) {
    BOOST_TEST_MESSAGE("Testing zero rate compound factor...");

    DayCounter dc = Actual360();
    Rate r = 0.0;

    InterestRate irCont(r, dc, Continuous, Annual);
    InterestRate irComp(r, dc, Compounded, Semiannual);
    InterestRate irSimp(r, dc, Simple, Annual);

    Time t = 1.0;
    if (!close(irCont.compoundFactor(t), 1.0))
        BOOST_ERROR("Continuous CF at r=0 = " << irCont.compoundFactor(t));
    if (!close(irComp.compoundFactor(t), 1.0))
        BOOST_ERROR("Compounded CF at r=0 = " << irComp.compoundFactor(t));
    if (!close(irSimp.compoundFactor(t), 1.0))
        BOOST_ERROR("Simple CF at r=0 = " << irSimp.compoundFactor(t));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
