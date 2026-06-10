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
#include <ql/instruments/europeanoption.hpp>
#include <ql/instruments/vanillaoption.hpp>
#include <ql/pricingengines/vanilla/analyticeuropeanengine.hpp>
#include <ql/termstructures/yield/flatforward.hpp>
#include <ql/termstructures/volatility/equityfx/blackconstantvol.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>
#include <ql/quotes/simplequote.hpp>
#include <ql/time/calendars/nullcalendar.hpp>
#include <cmath>

using namespace QuantLib;
using namespace boost::unit_test_framework;

BOOST_FIXTURE_TEST_SUITE(QuantLibTests, TopLevelFixture)

BOOST_AUTO_TEST_SUITE(PricingEngineEdgeCaseTests)

namespace {

    ext::shared_ptr<BlackScholesMertonProcess>
    makeBSMProcess(Real spot, Rate r, Rate q, Volatility vol) {
        DayCounter dc = Actual365Fixed();
        Date today = Settings::instance().evaluationDate();

        auto spotQuote = ext::make_shared<SimpleQuote>(spot);
        auto rTS = ext::make_shared<FlatForward>(today, r, dc);
        auto qTS = ext::make_shared<FlatForward>(today, q, dc);
        auto volTS = ext::make_shared<BlackConstantVol>(today, NullCalendar(), vol, dc);

        return ext::make_shared<BlackScholesMertonProcess>(
            Handle<Quote>(spotQuote),
            Handle<YieldTermStructure>(qTS),
            Handle<YieldTermStructure>(rTS),
            Handle<BlackVolTermStructure>(volTS));
    }
}

BOOST_AUTO_TEST_CASE(testPutCallParity) {
    BOOST_TEST_MESSAGE("Testing put-call parity for European options...");

    Settings::instance().evaluationDate() = Date(15, May, 2020);
    Date today = Settings::instance().evaluationDate();

    Real spot = 100.0;
    Rate r = 0.05;
    Rate q = 0.02;
    Volatility vol = 0.20;
    Real strike = 100.0;
    Date maturity = today + 365;

    auto process = makeBSMProcess(spot, r, q, vol);
    auto engine = ext::make_shared<AnalyticEuropeanEngine>(process);

    auto callPayoff = ext::make_shared<PlainVanillaPayoff>(Option::Call, strike);
    auto putPayoff = ext::make_shared<PlainVanillaPayoff>(Option::Put, strike);
    auto exercise = ext::make_shared<EuropeanExercise>(maturity);

    EuropeanOption call(callPayoff, exercise);
    call.setPricingEngine(engine);
    Real callNPV = call.NPV();

    EuropeanOption put(putPayoff, exercise);
    put.setPricingEngine(engine);
    Real putNPV = put.NPV();

    DayCounter dc = Actual365Fixed();
    Time T = dc.yearFraction(today, maturity);
    // C - P = S*exp(-q*T) - K*exp(-r*T)
    Real lhs = callNPV - putNPV;
    Real rhs = spot * std::exp(-q * T) - strike * std::exp(-r * T);
    Real error = std::fabs(lhs - rhs);

    if (error > 1.0e-8)
        BOOST_ERROR("Put-call parity violated: C-P = " << lhs
                    << ", S*exp(-qT)-K*exp(-rT) = " << rhs
                    << ", error = " << error);
}

BOOST_AUTO_TEST_CASE(testDeepInTheMoneyCall) {
    BOOST_TEST_MESSAGE("Testing deep in-the-money European call...");

    Settings::instance().evaluationDate() = Date(15, May, 2020);
    Date today = Settings::instance().evaluationDate();

    Real spot = 200.0;
    Rate r = 0.05;
    Rate q = 0.0;
    Volatility vol = 0.20;
    Real strike = 50.0;
    Date maturity = today + 365;

    auto process = makeBSMProcess(spot, r, q, vol);
    auto engine = ext::make_shared<AnalyticEuropeanEngine>(process);

    auto payoff = ext::make_shared<PlainVanillaPayoff>(Option::Call, strike);
    auto exercise = ext::make_shared<EuropeanExercise>(maturity);

    EuropeanOption option(payoff, exercise);
    option.setPricingEngine(engine);
    Real npv = option.NPV();

    DayCounter dc = Actual365Fixed();
    Time T = dc.yearFraction(today, maturity);
    // Deep ITM call: NPV >= S - K*exp(-rT) (intrinsic, discounted)
    Real intrinsic = spot - strike * std::exp(-r * T);
    if (npv < intrinsic - 1.0e-8)
        BOOST_ERROR("Deep ITM call NPV " << npv
                    << " less than discounted intrinsic " << intrinsic);

    // Delta should be close to 1
    Real delta = option.delta();
    if (delta < 0.95)
        BOOST_ERROR("Deep ITM call delta " << delta
                    << " is too far from 1.0");
}

BOOST_AUTO_TEST_CASE(testDeepOutOfTheMoneyPut) {
    BOOST_TEST_MESSAGE("Testing deep out-of-the-money European put...");

    Settings::instance().evaluationDate() = Date(15, May, 2020);
    Date today = Settings::instance().evaluationDate();

    Real spot = 200.0;
    Rate r = 0.05;
    Rate q = 0.0;
    Volatility vol = 0.20;
    Real strike = 50.0;
    Date maturity = today + 365;

    auto process = makeBSMProcess(spot, r, q, vol);
    auto engine = ext::make_shared<AnalyticEuropeanEngine>(process);

    auto payoff = ext::make_shared<PlainVanillaPayoff>(Option::Put, strike);
    auto exercise = ext::make_shared<EuropeanExercise>(maturity);

    EuropeanOption option(payoff, exercise);
    option.setPricingEngine(engine);
    Real npv = option.NPV();

    // Deep OTM put should be close to 0
    if (npv > 0.5)
        BOOST_ERROR("Deep OTM put NPV " << npv
                    << " is unexpectedly large");

    if (npv < 0.0)
        BOOST_ERROR("Put NPV " << npv << " is negative");
}

BOOST_AUTO_TEST_CASE(testExpiredOption) {
    BOOST_TEST_MESSAGE("Testing expired European option returns zero NPV...");

    Settings::instance().evaluationDate() = Date(15, May, 2020);
    Date today = Settings::instance().evaluationDate();

    Real spot = 100.0;
    Rate r = 0.05;
    Rate q = 0.0;
    Volatility vol = 0.20;
    Real strike = 100.0;
    Date maturity = today - 30;

    auto process = makeBSMProcess(spot, r, q, vol);
    auto engine = ext::make_shared<AnalyticEuropeanEngine>(process);

    auto payoff = ext::make_shared<PlainVanillaPayoff>(Option::Call, strike);
    auto exercise = ext::make_shared<EuropeanExercise>(maturity);

    EuropeanOption option(payoff, exercise);
    option.setPricingEngine(engine);

    if (!option.isExpired())
        BOOST_ERROR("Option with past maturity should be expired");

    Real npv = option.NPV();
    if (npv != 0.0)
        BOOST_ERROR("Expired option NPV should be 0, got " << npv);
}

BOOST_AUTO_TEST_CASE(testZeroVolatility) {
    BOOST_TEST_MESSAGE("Testing European option with zero volatility...");

    Settings::instance().evaluationDate() = Date(15, May, 2020);
    Date today = Settings::instance().evaluationDate();

    Real spot = 110.0;
    Rate r = 0.05;
    Rate q = 0.0;
    Volatility vol = 1.0e-10;  // near-zero vol
    Real strike = 100.0;
    Date maturity = today + 365;

    auto process = makeBSMProcess(spot, r, q, vol);
    auto engine = ext::make_shared<AnalyticEuropeanEngine>(process);

    auto payoff = ext::make_shared<PlainVanillaPayoff>(Option::Call, strike);
    auto exercise = ext::make_shared<EuropeanExercise>(maturity);

    EuropeanOption option(payoff, exercise);
    option.setPricingEngine(engine);
    Real npv = option.NPV();

    DayCounter dc = Actual365Fixed();
    Time T = dc.yearFraction(today, maturity);
    // With zero vol, ITM call = S - K*exp(-rT)
    Real expected = spot - strike * std::exp(-r * T);
    Real error = std::fabs(npv - expected);
    if (error > 0.1)
        BOOST_ERROR("Near-zero vol ITM call NPV " << npv
                    << " differs from intrinsic " << expected
                    << ", error = " << error);
}

BOOST_AUTO_TEST_CASE(testHighVolatility) {
    BOOST_TEST_MESSAGE("Testing European option greeks with high volatility...");

    Settings::instance().evaluationDate() = Date(15, May, 2020);
    Date today = Settings::instance().evaluationDate();

    Real spot = 100.0;
    Rate r = 0.05;
    Rate q = 0.0;
    Volatility vol = 2.0;  // 200% vol
    Real strike = 100.0;
    Date maturity = today + 365;

    auto process = makeBSMProcess(spot, r, q, vol);
    auto engine = ext::make_shared<AnalyticEuropeanEngine>(process);

    auto payoff = ext::make_shared<PlainVanillaPayoff>(Option::Call, strike);
    auto exercise = ext::make_shared<EuropeanExercise>(maturity);

    EuropeanOption option(payoff, exercise);
    option.setPricingEngine(engine);

    Real npv = option.NPV();
    Real delta = option.delta();
    Real gamma = option.gamma();
    Real vega = option.vega();

    // Sanity: NPV positive, delta in [0,1], gamma >=0, vega >= 0
    if (npv <= 0.0)
        BOOST_ERROR("High vol call NPV should be positive, got " << npv);
    if (delta < 0.0 || delta > 1.0)
        BOOST_ERROR("High vol call delta " << delta << " out of [0,1]");
    if (gamma < 0.0)
        BOOST_ERROR("High vol call gamma " << gamma << " is negative");
    if (vega < 0.0)
        BOOST_ERROR("High vol call vega " << vega << " is negative");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
