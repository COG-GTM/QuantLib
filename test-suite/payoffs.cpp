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
#include <ql/instruments/payoffs.hpp>
#include <ql/patterns/visitor.hpp>

using namespace QuantLib;
using namespace boost::unit_test_framework;

BOOST_FIXTURE_TEST_SUITE(QuantLibTests, TopLevelFixture)

BOOST_AUTO_TEST_SUITE(PayoffTests)

BOOST_AUTO_TEST_CASE(testNullPayoff) {
    BOOST_TEST_MESSAGE("Testing NullPayoff...");

    NullPayoff payoff;
    BOOST_CHECK_EQUAL(payoff.name(), "Null");
    BOOST_CHECK_EQUAL(payoff.description(), "Null");
    BOOST_CHECK_THROW(payoff(100.0), Error);
}

BOOST_AUTO_TEST_CASE(testPlainVanillaPayoff) {
    BOOST_TEST_MESSAGE("Testing PlainVanillaPayoff...");

    Real strike = 100.0;

    PlainVanillaPayoff call(Option::Call, strike);
    BOOST_CHECK_CLOSE(call(120.0), 20.0, 1e-10);
    BOOST_CHECK_CLOSE(call(100.0), 0.0, 1e-10);
    BOOST_CHECK_CLOSE(call(80.0), 0.0, 1e-10);
    BOOST_CHECK_EQUAL(call.optionType(), Option::Call);
    BOOST_CHECK_CLOSE(call.strike(), strike, 1e-10);

    PlainVanillaPayoff put(Option::Put, strike);
    BOOST_CHECK_CLOSE(put(80.0), 20.0, 1e-10);
    BOOST_CHECK_CLOSE(put(100.0), 0.0, 1e-10);
    BOOST_CHECK_CLOSE(put(120.0), 0.0, 1e-10);

    // description
    std::string desc = call.description();
    BOOST_CHECK(desc.find("Call") != std::string::npos);
    BOOST_CHECK(desc.find("100") != std::string::npos);

    // boundary values
    PlainVanillaPayoff zeroStrike(Option::Call, 0.0);
    BOOST_CHECK_CLOSE(zeroStrike(50.0), 50.0, 1e-10);
}

BOOST_AUTO_TEST_CASE(testCashOrNothingPayoff) {
    BOOST_TEST_MESSAGE("Testing CashOrNothingPayoff...");

    Real strike = 100.0;
    Real cash = 10.0;

    CashOrNothingPayoff call(Option::Call, strike, cash);
    BOOST_CHECK_CLOSE(call(120.0), cash, 1e-10);
    BOOST_CHECK_CLOSE(call(80.0), 0.0, 1e-10);
    BOOST_CHECK_CLOSE(call(100.0), 0.0, 1e-10); // at the money
    BOOST_CHECK_CLOSE(call.cashPayoff(), cash, 1e-10);

    CashOrNothingPayoff put(Option::Put, strike, cash);
    BOOST_CHECK_CLOSE(put(80.0), cash, 1e-10);
    BOOST_CHECK_CLOSE(put(120.0), 0.0, 1e-10);

    std::string desc = call.description();
    BOOST_CHECK(desc.find("cash payoff") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(testAssetOrNothingPayoff) {
    BOOST_TEST_MESSAGE("Testing AssetOrNothingPayoff...");

    Real strike = 100.0;

    AssetOrNothingPayoff call(Option::Call, strike);
    BOOST_CHECK_CLOSE(call(120.0), 120.0, 1e-10);
    BOOST_CHECK_CLOSE(call(80.0), 0.0, 1e-10);
    BOOST_CHECK_CLOSE(call(100.0), 0.0, 1e-10);

    AssetOrNothingPayoff put(Option::Put, strike);
    BOOST_CHECK_CLOSE(put(80.0), 80.0, 1e-10);
    BOOST_CHECK_CLOSE(put(120.0), 0.0, 1e-10);
}

BOOST_AUTO_TEST_CASE(testGapPayoff) {
    BOOST_TEST_MESSAGE("Testing GapPayoff...");

    Real strike = 100.0;
    Real secondStrike = 95.0;

    GapPayoff call(Option::Call, strike, secondStrike);
    BOOST_CHECK_CLOSE(call(120.0), 120.0 - 95.0, 1e-10);
    BOOST_CHECK_CLOSE(call(100.0), 100.0 - 95.0, 1e-10); // >= trigger
    BOOST_CHECK_CLOSE(call(80.0), 0.0, 1e-10);
    BOOST_CHECK_CLOSE(call.secondStrike(), secondStrike, 1e-10);

    GapPayoff put(Option::Put, strike, secondStrike);
    BOOST_CHECK_CLOSE(put(80.0), 95.0 - 80.0, 1e-10);
    BOOST_CHECK_CLOSE(put(100.0), 95.0 - 100.0, 1e-10); // = trigger
    BOOST_CHECK_CLOSE(put(120.0), 0.0, 1e-10);

    std::string desc = call.description();
    BOOST_CHECK(desc.find("strike payoff") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(testSuperFundPayoff) {
    BOOST_TEST_MESSAGE("Testing SuperFundPayoff...");

    Real strike = 100.0;
    Real secondStrike = 150.0;

    SuperFundPayoff payoff(strike, secondStrike);
    BOOST_CHECK_CLOSE(payoff(120.0), 120.0 / 100.0, 1e-10);
    BOOST_CHECK_CLOSE(payoff(100.0), 100.0 / 100.0, 1e-10);
    BOOST_CHECK_CLOSE(payoff(80.0), 0.0, 1e-10);
    BOOST_CHECK_CLOSE(payoff(150.0), 0.0, 1e-10);
    BOOST_CHECK_CLOSE(payoff(149.99), 149.99 / 100.0, 1e-10);
}

BOOST_AUTO_TEST_CASE(testSuperSharePayoff) {
    BOOST_TEST_MESSAGE("Testing SuperSharePayoff...");

    Real strike = 100.0;
    Real secondStrike = 150.0;
    Real cashPayoff = 1.0;

    SuperSharePayoff payoff(strike, secondStrike, cashPayoff);
    BOOST_CHECK_CLOSE(payoff(120.0), cashPayoff, 1e-10);
    BOOST_CHECK_CLOSE(payoff(100.0), cashPayoff, 1e-10);
    BOOST_CHECK_CLOSE(payoff(80.0), 0.0, 1e-10);
    BOOST_CHECK_CLOSE(payoff(150.0), 0.0, 1e-10);
    BOOST_CHECK_CLOSE(payoff.cashPayoff(), cashPayoff, 1e-10);
    BOOST_CHECK_CLOSE(payoff.secondStrike(), secondStrike, 1e-10);

    std::string desc = payoff.description();
    BOOST_CHECK(desc.find("second strike") != std::string::npos);
    BOOST_CHECK(desc.find("amount") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(testPercentageStrikePayoff) {
    BOOST_TEST_MESSAGE("Testing PercentageStrikePayoff...");

    Real moneyness = 1.1;

    PercentageStrikePayoff call(Option::Call, moneyness);
    BOOST_CHECK_CLOSE(call(100.0), 0.0, 1e-10);

    PercentageStrikePayoff put(Option::Put, moneyness);
    BOOST_CHECK_CLOSE(put(100.0), 100.0 * (1.1 - 1.0), 1e-10);

    PercentageStrikePayoff call2(Option::Call, 0.9);
    BOOST_CHECK_CLOSE(call2(100.0), 100.0 * 0.1, 1e-10);
}

BOOST_AUTO_TEST_CASE(testFloatingTypePayoff) {
    BOOST_TEST_MESSAGE("Testing FloatingTypePayoff...");

    FloatingTypePayoff call(Option::Call);
    BOOST_CHECK_CLOSE(call(120.0, 100.0), 20.0, 1e-10);
    BOOST_CHECK_CLOSE(call(80.0, 100.0), 0.0, 1e-10);
    BOOST_CHECK_THROW(call(100.0), Error);

    FloatingTypePayoff put(Option::Put);
    BOOST_CHECK_CLOSE(put(80.0, 100.0), 20.0, 1e-10);
    BOOST_CHECK_CLOSE(put(120.0, 100.0), 0.0, 1e-10);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
