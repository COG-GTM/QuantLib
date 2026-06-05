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
#include <ql/cashflows/cashflows.hpp>
#include <ql/cashflows/simplecashflow.hpp>
#include <ql/cashflows/fixedratecoupon.hpp>
#include <ql/cashflows/couponpricer.hpp>
#include <ql/cashflows/duration.hpp>
#include <ql/termstructures/yield/flatforward.hpp>
#include <ql/time/calendars/target.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>
#include <ql/time/daycounters/thirty360.hpp>
#include <ql/time/schedule.hpp>
#include <ql/optional.hpp>
#include <ql/settings.hpp>

using namespace QuantLib;
using namespace boost::unit_test_framework;

BOOST_FIXTURE_TEST_SUITE(QuantLibTests, TopLevelFixture)

BOOST_AUTO_TEST_SUITE(CashFlowsMoreTests)

namespace cashflows_more_test {

    Leg makeFixedLeg(const Date& start, const Date& end,
                     Rate coupon, Real notional = 100.0) {
        Schedule schedule(start, end, Period(Semiannual),
                          TARGET(), ModifiedFollowing, ModifiedFollowing,
                          DateGeneration::Backward, false);
        return FixedRateLeg(schedule)
            .withNotionals(notional)
            .withCouponRates(coupon, Thirty360(Thirty360::BondBasis));
    }

}

BOOST_AUTO_TEST_CASE(testNpvWithInterestRate) {
    BOOST_TEST_MESSAGE("Testing CashFlows::npv with InterestRate...");

    Date today(15, March, 2024);
    Settings::instance().evaluationDate() = today;

    Leg leg = cashflows_more_test::makeFixedLeg(
        Date(15, March, 2024), Date(15, March, 2029), 0.05);

    InterestRate rate(0.04, Actual365Fixed(), Compounded, Semiannual);
    Real npv = CashFlows::npv(leg, rate, ext::nullopt, today);
    BOOST_CHECK(npv > 0.0);

    InterestRate rate2(0.06, Actual365Fixed(), Compounded, Semiannual);
    Real npv2 = CashFlows::npv(leg, rate2, ext::nullopt, today);
    BOOST_CHECK(npv2 < npv);
}

BOOST_AUTO_TEST_CASE(testBpsWithInterestRate) {
    BOOST_TEST_MESSAGE("Testing CashFlows::bps with InterestRate...");

    Date today(15, March, 2024);
    Settings::instance().evaluationDate() = today;

    Leg leg = cashflows_more_test::makeFixedLeg(
        Date(15, March, 2024), Date(15, March, 2029), 0.05);

    InterestRate rate(0.04, Actual365Fixed(), Compounded, Semiannual);
    Real bps = CashFlows::bps(leg, rate, ext::nullopt, today);
    BOOST_CHECK(bps > 0.0);
}

BOOST_AUTO_TEST_CASE(testDurationTypes) {
    BOOST_TEST_MESSAGE("Testing CashFlows::duration with all Duration types...");

    Date today(15, March, 2024);
    Settings::instance().evaluationDate() = today;

    Leg leg = cashflows_more_test::makeFixedLeg(
        Date(15, March, 2024), Date(15, March, 2034), 0.05);

    InterestRate rate(0.05, Actual365Fixed(), Compounded, Semiannual);

    Time simpleDur = CashFlows::duration(leg, rate, Duration::Simple, ext::nullopt, today);
    BOOST_CHECK(simpleDur > 0.0);

    Time macaulayDur = CashFlows::duration(leg, rate, Duration::Macaulay, ext::nullopt, today);
    BOOST_CHECK(macaulayDur > 0.0);

    Time modifiedDur = CashFlows::duration(leg, rate, Duration::Modified, ext::nullopt, today);
    BOOST_CHECK(modifiedDur > 0.0);
    BOOST_CHECK(modifiedDur < macaulayDur);
}

BOOST_AUTO_TEST_CASE(testConvexity) {
    BOOST_TEST_MESSAGE("Testing CashFlows::convexity...");

    Date today(15, March, 2024);
    Settings::instance().evaluationDate() = today;

    Leg leg = cashflows_more_test::makeFixedLeg(
        Date(15, March, 2024), Date(15, March, 2034), 0.05);

    InterestRate rate(0.05, Actual365Fixed(), Compounded, Semiannual);

    Real convex = CashFlows::convexity(leg, rate, ext::nullopt, today);
    BOOST_CHECK(convex > 0.0);
}

BOOST_AUTO_TEST_CASE(testBasisPointValue) {
    BOOST_TEST_MESSAGE("Testing CashFlows::basisPointValue and yieldValueBasisPoint...");

    Date today(15, March, 2024);
    Settings::instance().evaluationDate() = today;

    Leg leg = cashflows_more_test::makeFixedLeg(
        Date(15, March, 2024), Date(15, March, 2029), 0.05, 100.0);

    InterestRate rate(0.05, Actual365Fixed(), Compounded, Semiannual);

    Real bpv = CashFlows::basisPointValue(leg, rate, ext::nullopt, today);
    BOOST_CHECK(bpv != 0.0);

    Real yvbp = CashFlows::yieldValueBasisPoint(leg, rate, ext::nullopt, today);
    BOOST_CHECK(yvbp != 0.0);
}

BOOST_AUTO_TEST_CASE(testZSpread) {
    BOOST_TEST_MESSAGE("Testing CashFlows::zSpread...");

    Date today(15, March, 2024);
    Settings::instance().evaluationDate() = today;

    Leg leg = cashflows_more_test::makeFixedLeg(
        Date(15, March, 2024), Date(15, March, 2029), 0.05, 100.0);

    auto curve = ext::make_shared<FlatForward>(today, 0.04, Actual365Fixed());

    Real npv = CashFlows::npv(leg, *curve, ext::nullopt, today);

    Spread spread = CashFlows::zSpread(
        leg, npv, curve, Compounded, Semiannual, ext::nullopt, today);

    BOOST_CHECK_SMALL(std::abs(spread), 0.0001);
}

BOOST_AUTO_TEST_CASE(testAtmRate) {
    BOOST_TEST_MESSAGE("Testing CashFlows::atmRate...");

    Date today(15, March, 2024);
    Settings::instance().evaluationDate() = today;

    Leg leg = cashflows_more_test::makeFixedLeg(
        Date(15, March, 2024), Date(15, March, 2029), 0.05, 100.0);

    auto curve = ext::make_shared<FlatForward>(today, 0.05, Actual365Fixed());

    Rate atm = CashFlows::atmRate(leg, *curve, ext::nullopt, today);
    BOOST_CHECK(atm > 0.0);
}

BOOST_AUTO_TEST_CASE(testYieldFromNpv) {
    BOOST_TEST_MESSAGE("Testing CashFlows yield from NPV round-trip...");

    Date today(15, March, 2024);
    Settings::instance().evaluationDate() = today;

    Leg leg = cashflows_more_test::makeFixedLeg(
        Date(15, March, 2024), Date(15, March, 2029), 0.06, 100.0);

    InterestRate targetRate(0.05, Actual365Fixed(), Compounded, Semiannual);
    Real npv = CashFlows::npv(leg, targetRate, ext::nullopt, today);

    Rate computedYield = CashFlows::yield(
        leg, npv, Actual365Fixed(), Compounded, Semiannual,
        ext::nullopt, today);

    BOOST_CHECK_CLOSE(computedYield, 0.05, 0.01);
}

BOOST_AUTO_TEST_CASE(testSimpleCashFlowLeg) {
    BOOST_TEST_MESSAGE("Testing CashFlows functions on SimpleCashFlow leg...");

    Date today(15, March, 2024);
    Settings::instance().evaluationDate() = today;

    Leg leg;
    leg.push_back(ext::make_shared<SimpleCashFlow>(50.0, today + 180));
    leg.push_back(ext::make_shared<SimpleCashFlow>(1050.0, today + 365));

    Date startDate = CashFlows::startDate(leg);
    Date matDate = CashFlows::maturityDate(leg);
    BOOST_CHECK(startDate < matDate);

    InterestRate rate(0.04, Actual365Fixed(), Compounded, Annual);
    Real npv = CashFlows::npv(leg, rate, ext::nullopt, today);
    BOOST_CHECK(npv > 0.0);
}

BOOST_AUTO_TEST_CASE(testPreviousAndNextCashFlow) {
    BOOST_TEST_MESSAGE("Testing CashFlows previous/next cash flow iteration...");

    Date today(15, March, 2024);
    Settings::instance().evaluationDate() = today;

    Date start = Date(15, March, 2023);
    Date end = Date(15, March, 2025);
    Leg leg = cashflows_more_test::makeFixedLeg(start, end, 0.05);

    auto next = CashFlows::nextCashFlow(leg, ext::nullopt, today);
    BOOST_CHECK(next != leg.end());
    BOOST_CHECK((*next)->date() > today);

    auto prev = CashFlows::previousCashFlow(leg, ext::nullopt, today);
    BOOST_CHECK(prev != leg.rend());

    Date nextDate = CashFlows::nextCashFlowDate(leg, ext::nullopt, today);
    BOOST_CHECK(nextDate > today);

    Date prevDate = CashFlows::previousCashFlowDate(leg, ext::nullopt, today);
    BOOST_CHECK(prevDate <= today);

    Real nextAmt = CashFlows::nextCashFlowAmount(leg, ext::nullopt, today);
    BOOST_CHECK(nextAmt > 0.0);

    Real prevAmt = CashFlows::previousCashFlowAmount(leg, ext::nullopt, today);
    BOOST_CHECK(prevAmt > 0.0);
}

BOOST_AUTO_TEST_CASE(testNpvWithYieldTermStructure) {
    BOOST_TEST_MESSAGE("Testing CashFlows::npv with YieldTermStructure...");

    Date today(15, March, 2024);
    Settings::instance().evaluationDate() = today;

    Leg leg = cashflows_more_test::makeFixedLeg(
        Date(15, March, 2024), Date(15, March, 2029), 0.05, 100.0);

    auto curve = ext::make_shared<FlatForward>(today, 0.04, Actual365Fixed());

    Real npv = CashFlows::npv(leg, *curve, ext::nullopt, today);
    BOOST_CHECK(npv > 0.0);

    // NPV with z-spread of 0 should equal base NPV
    Real npvZero = CashFlows::npv(leg, curve, 0.0,
                                  Compounded, Semiannual,
                                  ext::nullopt, today);
    BOOST_CHECK_CLOSE(npv, npvZero, 0.01);

    // positive z-spread should reduce NPV
    Real npvSpread = CashFlows::npv(leg, curve, 0.01,
                                    Compounded, Semiannual,
                                    ext::nullopt, today);
    BOOST_CHECK(npvSpread < npv);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
