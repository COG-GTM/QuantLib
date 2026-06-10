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
#include <ql/time/daycounters/actual360.hpp>
#include <ql/time/daycounters/actual364.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>
#include <ql/time/daycounters/actual36525.hpp>
#include <ql/time/daycounters/actual366.hpp>
#include <ql/time/daycounters/one.hpp>
#include <ql/time/daycounters/simpledaycounter.hpp>
#include <ql/time/daycounters/thirty360.hpp>
#include <ql/time/daycounters/thirty365.hpp>
#include <cmath>

using namespace QuantLib;
using namespace boost::unit_test_framework;

BOOST_FIXTURE_TEST_SUITE(QuantLibTests, TopLevelFixture)

BOOST_AUTO_TEST_SUITE(DayCounterExtraTests)

BOOST_AUTO_TEST_CASE(testSameDateYearFraction) {
    BOOST_TEST_MESSAGE("Testing that year fraction is zero for same dates...");

    Date d(15, March, 2020);
    DayCounter counters[] = {
        Actual360(), Actual365Fixed(), Actual366(),
        Thirty360(Thirty360::BondBasis),
        SimpleDayCounter()
    };

    for (const auto& dc : counters) {
        Time yf = dc.yearFraction(d, d);
        if (std::fabs(yf) > 1.0e-15)
            BOOST_ERROR(dc.name() << ": year fraction for same date = "
                        << yf << ", expected 0");
    }
}

BOOST_AUTO_TEST_CASE(testDayCountZeroForSameDate) {
    BOOST_TEST_MESSAGE("Testing that day count is zero for same dates...");

    Date d(15, June, 2021);
    DayCounter counters[] = {
        Actual360(), Actual365Fixed(),
        Thirty360(Thirty360::BondBasis)
    };

    for (const auto& dc : counters) {
        Integer days = dc.dayCount(d, d);
        if (days != 0)
            BOOST_ERROR(dc.name() << ": day count for same date = "
                        << days << ", expected 0");
    }
}

BOOST_AUTO_TEST_CASE(testActual360KnownValues) {
    BOOST_TEST_MESSAGE("Testing Actual/360 known year fractions...");

    Actual360 dc;

    // 30 actual days
    Date d1(1, January, 2020);
    Date d2(31, January, 2020);
    Time yf = dc.yearFraction(d1, d2);
    Time expected = 30.0 / 360.0;
    if (std::fabs(yf - expected) > 1.0e-15)
        BOOST_ERROR("Actual/360 Jan 1-31: " << yf
                    << ", expected " << expected);

    // Full year (2020 is leap: 366 days)
    Date d3(1, January, 2020);
    Date d4(1, January, 2021);
    yf = dc.yearFraction(d3, d4);
    expected = 366.0 / 360.0;
    if (std::fabs(yf - expected) > 1.0e-15)
        BOOST_ERROR("Actual/360 full leap year: " << yf
                    << ", expected " << expected);
}

BOOST_AUTO_TEST_CASE(testActual364KnownValues) {
    BOOST_TEST_MESSAGE("Testing Actual/364 known year fractions...");

    Actual364 dc;

    // Exactly 364 days should give year fraction 1.0
    Date d1(1, January, 2021);
    Date d2(31, December, 2021);
    Time yf = dc.yearFraction(d1, d2);
    Time expected = 364.0 / 364.0;
    if (std::fabs(yf - expected) > 1.0e-15)
        BOOST_ERROR("Actual/364 for 364 days: " << yf
                    << ", expected " << expected);
}

BOOST_AUTO_TEST_CASE(testActual365FixedKnownValues) {
    BOOST_TEST_MESSAGE("Testing Actual/365 (Fixed) known year fractions...");

    Actual365Fixed dc;

    // Full non-leap year: 365/365 = 1.0
    Date d1(1, January, 2019);
    Date d2(1, January, 2020);
    Time yf = dc.yearFraction(d1, d2);
    if (std::fabs(yf - 1.0) > 1.0e-15)
        BOOST_ERROR("Actual/365 full non-leap year: " << yf);

    // Full leap year: 366/365
    Date d3(1, January, 2020);
    Date d4(1, January, 2021);
    yf = dc.yearFraction(d3, d4);
    Time expected = 366.0 / 365.0;
    if (std::fabs(yf - expected) > 1.0e-15)
        BOOST_ERROR("Actual/365 full leap year: " << yf
                    << ", expected " << expected);
}

BOOST_AUTO_TEST_CASE(testActual36525KnownValues) {
    BOOST_TEST_MESSAGE("Testing Actual/365.25 known year fractions...");

    Actual36525 dc;

    // 4 years should be close to 4.0 (1461 days / 365.25)
    Date d1(1, January, 2020);
    Date d2(1, January, 2024);
    Time yf = dc.yearFraction(d1, d2);
    // 2020: 366, 2021: 365, 2022: 365, 2023: 365 = 1461 days
    Time expected = 1461.0 / 365.25;
    if (std::fabs(yf - expected) > 1.0e-12)
        BOOST_ERROR("Actual/365.25 for 4 years: " << yf
                    << ", expected " << expected);
}

BOOST_AUTO_TEST_CASE(testActual366KnownValues) {
    BOOST_TEST_MESSAGE("Testing Actual/366 known year fractions...");

    Actual366 dc;

    Date d1(1, January, 2020);
    Date d2(1, February, 2020);
    Time yf = dc.yearFraction(d1, d2);
    Time expected = 31.0 / 366.0;
    if (std::fabs(yf - expected) > 1.0e-15)
        BOOST_ERROR("Actual/366 Jan 2020: " << yf
                    << ", expected " << expected);
}

BOOST_AUTO_TEST_CASE(testOneDayCounter) {
    BOOST_TEST_MESSAGE("Testing OneDayCounter year fraction...");

    OneDayCounter dc;

    // OneDayCounter always returns 1 (or -1), regardless of distance
    Date d1(1, January, 2020);
    Date d2(1, January, 2020);
    // Same date: dayCount returns 1, yearFraction returns 1.0
    if (std::fabs(dc.yearFraction(d1, d2) - 1.0) > 1.0e-15)
        BOOST_ERROR("OneDayCounter same date: " << dc.yearFraction(d1, d2)
                    << ", expected 1.0");

    Date d3(1, January, 2020);
    Date d4(2, January, 2020);
    Time yf1 = dc.yearFraction(d3, d4);
    if (std::fabs(yf1 - 1.0) > 1.0e-15)
        BOOST_ERROR("OneDayCounter 1-day: " << yf1 << ", expected 1.0");

    Date d5(1, January, 2020);
    Date d6(1, February, 2020);
    Time yf2 = dc.yearFraction(d5, d6);
    if (std::fabs(yf2 - 1.0) > 1.0e-15)
        BOOST_ERROR("OneDayCounter 1-month: " << yf2 << ", expected 1.0");
}

BOOST_AUTO_TEST_CASE(testThirty360BondBasisKnown) {
    BOOST_TEST_MESSAGE("Testing 30/360 Bond Basis known values...");

    Thirty360 dc(Thirty360::BondBasis);

    // Feb 1 to Mar 1 in non-leap year: 30 days → 30/360
    Date d1(1, February, 2019);
    Date d2(1, March, 2019);
    Integer days = dc.dayCount(d1, d2);
    if (days != 30)
        BOOST_ERROR("30/360 BB Feb-Mar 2019: " << days << " days, expected 30");

    // Jan 30 to Feb 28 in non-leap year
    Date d3(30, January, 2019);
    Date d4(28, February, 2019);
    days = dc.dayCount(d3, d4);
    if (days != 28)
        BOOST_ERROR("30/360 BB Jan30-Feb28 2019: " << days
                    << " days, expected 28");

    // Full year: always 360 days
    Date d5(1, January, 2020);
    Date d6(1, January, 2021);
    days = dc.dayCount(d5, d6);
    if (days != 360)
        BOOST_ERROR("30/360 BB full year: " << days
                    << " days, expected 360");
}

BOOST_AUTO_TEST_CASE(testDayCounterName) {
    BOOST_TEST_MESSAGE("Testing day counter name accessors...");

    if (Actual360().name() != "Actual/360")
        BOOST_ERROR("Actual/360 name: " << Actual360().name());
    if (Actual365Fixed().name() != "Actual/365 (Fixed)")
        BOOST_ERROR("Actual/365 name: " << Actual365Fixed().name());
    if (Actual364().name() != "Actual/364")
        BOOST_ERROR("Actual/364 name: " << Actual364().name());
    if (Actual366().name() != "Actual/366")
        BOOST_ERROR("Actual/366 name: " << Actual366().name());
    if (Thirty360(Thirty360::BondBasis).name() != "30/360 (Bond Basis)")
        BOOST_ERROR("30/360 BB name: "
                    << Thirty360(Thirty360::BondBasis).name());
}

BOOST_AUTO_TEST_CASE(testDayCounterEquality) {
    BOOST_TEST_MESSAGE("Testing day counter equality...");

    Actual360 dc1;
    Actual360 dc2;
    Actual365Fixed dc3;

    if (dc1 != dc2)
        BOOST_ERROR("Two Actual/360 instances should be equal");
    if (dc1 == dc3)
        BOOST_ERROR("Actual/360 and Actual/365 should not be equal");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
