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
#include <ql/pricingengines/bond/bondfunctions.hpp>
#include <ql/pricingengines/bond/discountingbondengine.hpp>
#include <ql/instruments/bonds/fixedratebond.hpp>
#include <ql/instruments/bonds/zerocouponbond.hpp>
#include <ql/termstructures/yield/flatforward.hpp>
#include <ql/time/calendars/target.hpp>
#include <ql/time/calendars/unitedstates.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>
#include <ql/time/daycounters/actual360.hpp>
#include <ql/time/daycounters/thirty360.hpp>
#include <ql/time/schedule.hpp>
#include <ql/cashflows/duration.hpp>

using namespace QuantLib;
using namespace boost::unit_test_framework;

BOOST_FIXTURE_TEST_SUITE(QuantLibTests, TopLevelFixture)

BOOST_AUTO_TEST_SUITE(BondFunctionsTests)

namespace bondfunctions_test {

    struct CommonVars {
        Calendar calendar;
        Date today;
        Date issueDate;
        Date maturity;
        Natural settlementDays;
        Real faceAmount;
        Rate couponRate;
        ext::shared_ptr<FixedRateBond> bond;
        Handle<YieldTermStructure> discountCurve;

        CommonVars() {
            calendar = TARGET();
            today = Date(15, March, 2024);
            Settings::instance().evaluationDate() = today;

            settlementDays = 2;
            faceAmount = 100.0;
            couponRate = 0.05;
            issueDate = Date(15, March, 2020);
            maturity = Date(15, March, 2030);

            Schedule schedule(issueDate, maturity, Period(Annual),
                              calendar, Unadjusted, Unadjusted,
                              DateGeneration::Backward, false);

            bond = ext::make_shared<FixedRateBond>(
                settlementDays, faceAmount,
                schedule, std::vector<Rate>(1, couponRate),
                Thirty360(Thirty360::BondBasis));

            discountCurve = Handle<YieldTermStructure>(
                ext::make_shared<FlatForward>(today, 0.04,
                                              Actual365Fixed()));

            bond->setPricingEngine(
                ext::make_shared<DiscountingBondEngine>(discountCurve));
        }
    };

}

BOOST_AUTO_TEST_CASE(testDateInspectors) {
    BOOST_TEST_MESSAGE("Testing BondFunctions date inspectors...");

    bondfunctions_test::CommonVars vars;

    Date start = BondFunctions::startDate(*vars.bond);
    BOOST_CHECK(start == vars.issueDate);

    Date mat = BondFunctions::maturityDate(*vars.bond);
    BOOST_CHECK(mat == vars.maturity);

    BOOST_CHECK(BondFunctions::isTradable(*vars.bond));
}

BOOST_AUTO_TEST_CASE(testCashFlowInspectors) {
    BOOST_TEST_MESSAGE("Testing BondFunctions cash flow inspectors...");

    bondfunctions_test::CommonVars vars;

    Date settlement = vars.bond->settlementDate();

    Date nextCfDate = BondFunctions::nextCashFlowDate(*vars.bond);
    BOOST_CHECK(nextCfDate > settlement);

    Real nextCfAmount = BondFunctions::nextCashFlowAmount(*vars.bond);
    BOOST_CHECK(nextCfAmount > 0.0);

    Date prevCfDate = BondFunctions::previousCashFlowDate(*vars.bond);
    BOOST_CHECK(prevCfDate <= settlement);

    Real prevCfAmount = BondFunctions::previousCashFlowAmount(*vars.bond);
    BOOST_CHECK(prevCfAmount > 0.0);

    Rate prevCoupon = BondFunctions::previousCouponRate(*vars.bond);
    BOOST_CHECK_CLOSE(prevCoupon, vars.couponRate, 1e-10);

    Rate nextCoupon = BondFunctions::nextCouponRate(*vars.bond);
    BOOST_CHECK_CLOSE(nextCoupon, vars.couponRate, 1e-10);
}

BOOST_AUTO_TEST_CASE(testAccrualInspectors) {
    BOOST_TEST_MESSAGE("Testing BondFunctions accrual inspectors...");

    bondfunctions_test::CommonVars vars;

    Date accrualStart = BondFunctions::accrualStartDate(*vars.bond);
    Date accrualEnd = BondFunctions::accrualEndDate(*vars.bond);
    BOOST_CHECK(accrualStart < accrualEnd);

    Date refStart = BondFunctions::referencePeriodStart(*vars.bond);
    Date refEnd = BondFunctions::referencePeriodEnd(*vars.bond);
    BOOST_CHECK(refStart <= refEnd);

    Time accrualPeriod = BondFunctions::accrualPeriod(*vars.bond);
    BOOST_CHECK(accrualPeriod > 0.0);
    BOOST_CHECK(accrualPeriod <= 1.0);

    Date::serial_type accrualDays = BondFunctions::accrualDays(*vars.bond);
    BOOST_CHECK(accrualDays > 0);

    Time accruedPeriod = BondFunctions::accruedPeriod(*vars.bond);
    BOOST_CHECK(accruedPeriod >= 0.0);
    BOOST_CHECK(accruedPeriod <= accrualPeriod);

    Date::serial_type accruedDays = BondFunctions::accruedDays(*vars.bond);
    BOOST_CHECK(accruedDays >= 0);
    BOOST_CHECK(accruedDays <= accrualDays);

    Real accruedAmount = BondFunctions::accruedAmount(*vars.bond);
    BOOST_CHECK(accruedAmount >= 0.0);
}

BOOST_AUTO_TEST_CASE(testDurationCalculations) {
    BOOST_TEST_MESSAGE("Testing BondFunctions duration calculations...");

    bondfunctions_test::CommonVars vars;

    Rate yield = 0.05;
    DayCounter dc = Actual365Fixed();

    Time simpleDur = BondFunctions::duration(
        *vars.bond, yield, dc, Compounded, Annual,
        Duration::Simple);
    BOOST_CHECK(simpleDur > 0.0);

    Time macaulayDur = BondFunctions::duration(
        *vars.bond, yield, dc, Compounded, Annual,
        Duration::Macaulay);
    BOOST_CHECK(macaulayDur > 0.0);
    BOOST_CHECK(macaulayDur <= 10.0);

    Time modifiedDur = BondFunctions::duration(
        *vars.bond, yield, dc, Compounded, Annual,
        Duration::Modified);
    BOOST_CHECK(modifiedDur > 0.0);
    BOOST_CHECK(modifiedDur < macaulayDur);

    // modified = macaulay / (1 + y/freq)
    Real expectedModified = macaulayDur / (1.0 + yield);
    BOOST_CHECK_CLOSE(modifiedDur, expectedModified, 0.5);

    // also test InterestRate overload
    InterestRate ir(yield, dc, Compounded, Annual);
    Time dur2 = BondFunctions::duration(*vars.bond, ir, Duration::Modified);
    BOOST_CHECK_CLOSE(dur2, modifiedDur, 1e-10);
}

BOOST_AUTO_TEST_CASE(testConvexityCalculation) {
    BOOST_TEST_MESSAGE("Testing BondFunctions convexity...");

    bondfunctions_test::CommonVars vars;

    Rate yield = 0.05;
    DayCounter dc = Actual365Fixed();

    Real convex = BondFunctions::convexity(
        *vars.bond, yield, dc, Compounded, Annual);
    BOOST_CHECK(convex > 0.0);

    InterestRate ir(yield, dc, Compounded, Annual);
    Real convex2 = BondFunctions::convexity(*vars.bond, ir);
    BOOST_CHECK_CLOSE(convex, convex2, 1e-10);
}

BOOST_AUTO_TEST_CASE(testBpsCalculation) {
    BOOST_TEST_MESSAGE("Testing BondFunctions bps...");

    bondfunctions_test::CommonVars vars;

    Rate yield = 0.05;
    DayCounter dc = Actual365Fixed();

    Real bps = BondFunctions::bps(*vars.bond, yield, dc, Compounded, Annual);
    BOOST_CHECK(bps > 0.0);

    InterestRate ir(yield, dc, Compounded, Annual);
    Real bps2 = BondFunctions::bps(*vars.bond, ir);
    BOOST_CHECK_CLOSE(bps, bps2, 1e-10);
}

BOOST_AUTO_TEST_CASE(testBasisPointValueAndYieldValue) {
    BOOST_TEST_MESSAGE("Testing BondFunctions basisPointValue and yieldValueBasisPoint...");

    bondfunctions_test::CommonVars vars;

    Rate yield = 0.05;
    DayCounter dc = Actual365Fixed();

    Real bpv = BondFunctions::basisPointValue(
        *vars.bond, yield, dc, Compounded, Annual);
    BOOST_CHECK(bpv != 0.0);

    Real yvbp = BondFunctions::yieldValueBasisPoint(
        *vars.bond, yield, dc, Compounded, Annual);
    BOOST_CHECK(yvbp != 0.0);

    // bpv * yvbp should approximately equal 0.01 (1bp * 1bp)
    // They are roughly inverses scaled by notional

    InterestRate ir(yield, dc, Compounded, Annual);
    Real bpv2 = BondFunctions::basisPointValue(*vars.bond, ir);
    BOOST_CHECK_CLOSE(bpv, bpv2, 1e-10);

    Real yvbp2 = BondFunctions::yieldValueBasisPoint(*vars.bond, ir);
    BOOST_CHECK_CLOSE(yvbp, yvbp2, 1e-10);
}

BOOST_AUTO_TEST_CASE(testCleanAndDirtyPriceFromYield) {
    BOOST_TEST_MESSAGE("Testing BondFunctions clean/dirty price from yield...");

    bondfunctions_test::CommonVars vars;

    Rate yield = 0.05;
    DayCounter dc = Thirty360(Thirty360::BondBasis);

    Real cleanPrice = BondFunctions::cleanPrice(
        *vars.bond, yield, dc, Compounded, Annual);
    Real dirtyPrice = BondFunctions::dirtyPrice(
        *vars.bond, yield, dc, Compounded, Annual);

    BOOST_CHECK(cleanPrice > 0.0);
    BOOST_CHECK(dirtyPrice > 0.0);
    BOOST_CHECK(dirtyPrice >= cleanPrice);

    Real accrued = vars.bond->accruedAmount();
    BOOST_CHECK_CLOSE(dirtyPrice - cleanPrice, accrued, 0.01);
}

BOOST_AUTO_TEST_CASE(testYieldFromPrice) {
    BOOST_TEST_MESSAGE("Testing BondFunctions yield calculation...");

    bondfunctions_test::CommonVars vars;

    DayCounter dc = Thirty360(Thirty360::BondBasis);
    Rate targetYield = 0.05;

    Real price = BondFunctions::cleanPrice(
        *vars.bond, targetYield, dc, Compounded, Annual);

    Rate computedYield = BondFunctions::yield(
        *vars.bond, {price, Bond::Price::Clean},
        dc, Compounded, Annual);

    BOOST_CHECK_CLOSE(computedYield, targetYield, 0.001);
}

BOOST_AUTO_TEST_CASE(testAtmRate) {
    BOOST_TEST_MESSAGE("Testing BondFunctions ATM rate...");

    bondfunctions_test::CommonVars vars;

    Rate atmRate = BondFunctions::atmRate(
        *vars.bond, **vars.discountCurve);
    BOOST_CHECK(atmRate > 0.0);
}

BOOST_AUTO_TEST_CASE(testZSpread) {
    BOOST_TEST_MESSAGE("Testing BondFunctions z-spread...");

    bondfunctions_test::CommonVars vars;

    Real cleanPrice = vars.bond->cleanPrice();

    Spread zSpread = BondFunctions::zSpread(
        *vars.bond, {cleanPrice, Bond::Price::Clean},
        *vars.discountCurve,
        Compounded, Annual);

    BOOST_CHECK_SMALL(std::abs(zSpread), 0.01);

    // price at a z-spread
    Real priceAtSpread = BondFunctions::cleanPrice(
        *vars.bond, *vars.discountCurve,
        zSpread, Compounded, Annual);
    BOOST_CHECK_CLOSE(priceAtSpread, cleanPrice, 0.01);
}

BOOST_AUTO_TEST_CASE(testZeroCouponBondFunctions) {
    BOOST_TEST_MESSAGE("Testing BondFunctions with zero-coupon bond...");

    bondfunctions_test::CommonVars vars;

    Date zcMaturity = Date(15, March, 2034);
    ZeroCouponBond zcBond(vars.settlementDays, vars.calendar,
                          100.0, zcMaturity);
    zcBond.setPricingEngine(
        ext::make_shared<DiscountingBondEngine>(vars.discountCurve));

    BOOST_CHECK(BondFunctions::isTradable(zcBond));

    Date mat = BondFunctions::maturityDate(zcBond);
    BOOST_CHECK(mat == zcMaturity);

    Real cleanPrice = zcBond.cleanPrice();
    BOOST_CHECK(cleanPrice > 0.0);
    BOOST_CHECK(cleanPrice < 100.0);
}

BOOST_AUTO_TEST_CASE(testDurationTypeStreaming) {
    BOOST_TEST_MESSAGE("Testing Duration::Type streaming operator...");

    std::ostringstream oss;

    oss << Duration::Simple;
    BOOST_CHECK_EQUAL(oss.str(), "Simple");

    oss.str("");
    oss << Duration::Macaulay;
    BOOST_CHECK_EQUAL(oss.str(), "Macaulay");

    oss.str("");
    oss << Duration::Modified;
    BOOST_CHECK_EQUAL(oss.str(), "Modified");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
