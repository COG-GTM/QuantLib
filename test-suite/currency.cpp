/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 Copyright (C) 2021 Marcin Rybacki

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
#include <ql/currency.hpp>
#include <ql/currencies/america.hpp>
#include <ql/currencies/europe.hpp>
#include <ql/currencies/asia.hpp>
#include <sstream>

using namespace QuantLib;
using namespace boost::unit_test_framework;

BOOST_FIXTURE_TEST_SUITE(QuantLibTests, TopLevelFixture)

BOOST_AUTO_TEST_SUITE(CurrencyTests)

BOOST_AUTO_TEST_CASE(testBespokeConstructor) {
    BOOST_TEST_MESSAGE("Testing bespoke currency constructor...");

    std::string name("Some Currency");
    std::string code("CCY");
    std::string symbol("#");

    Currency customCcy(name, code, 100, symbol, "", 100, Rounding());

    if (customCcy.empty())
        BOOST_ERROR("Failed to create bespoke currency.");

    if (customCcy.name() != name)
        BOOST_ERROR("incorrect currency name\n"
                    << "    actual:    " << customCcy.name() << "\n"
                    << "    expected:    " << name << "\n");

    if (customCcy.code() != code)
        BOOST_ERROR("incorrect currency code\n"
                    << "    actual:    " << customCcy.code() << "\n"
                    << "    expected:    " << code << "\n");

    if (customCcy.symbol() != symbol)
        BOOST_ERROR("incorrect currency symbol\n"
                    << "    actual:    " << customCcy.symbol() << "\n"
                    << "    expected:    " << symbol << "\n");
}

BOOST_AUTO_TEST_CASE(testDefaultConstructor) {
    BOOST_TEST_MESSAGE("Testing default-constructed currency is empty...");

    Currency c;
    if (!c.empty())
        BOOST_ERROR("Default-constructed currency should be empty");
}

BOOST_AUTO_TEST_CASE(testEquality) {
    BOOST_TEST_MESSAGE("Testing currency equality and inequality...");

    USDCurrency usd1;
    USDCurrency usd2;
    EURCurrency eur;

    if (!(usd1 == usd2))
        BOOST_ERROR("Two USD instances should be equal");

    if (usd1 != usd2)
        BOOST_ERROR("Two USD instances should not be unequal");

    if (usd1 == eur)
        BOOST_ERROR("USD and EUR should not be equal");

    if (!(usd1 != eur))
        BOOST_ERROR("USD and EUR should be unequal");
}

BOOST_AUTO_TEST_CASE(testKnownCurrencies) {
    BOOST_TEST_MESSAGE("Testing known currency properties...");

    USDCurrency usd;
    if (usd.name() != "U.S. dollar")
        BOOST_ERROR("USD name incorrect: " << usd.name());
    if (usd.code() != "USD")
        BOOST_ERROR("USD code incorrect: " << usd.code());
    if (usd.numericCode() != 840)
        BOOST_ERROR("USD numeric code incorrect: " << usd.numericCode());
    if (usd.symbol() != "$")
        BOOST_ERROR("USD symbol incorrect: " << usd.symbol());
    if (usd.fractionsPerUnit() != 100)
        BOOST_ERROR("USD fractions per unit incorrect: "
                    << usd.fractionsPerUnit());

    EURCurrency eur;
    if (eur.code() != "EUR")
        BOOST_ERROR("EUR code incorrect: " << eur.code());
    if (eur.numericCode() != 978)
        BOOST_ERROR("EUR numeric code incorrect: " << eur.numericCode());

    GBPCurrency gbp;
    if (gbp.code() != "GBP")
        BOOST_ERROR("GBP code incorrect: " << gbp.code());
    if (gbp.symbol() != "\xA3")
        BOOST_ERROR("GBP symbol incorrect");

    JPYCurrency jpy;
    if (jpy.code() != "JPY")
        BOOST_ERROR("JPY code incorrect: " << jpy.code());
    if (jpy.numericCode() != 392)
        BOOST_ERROR("JPY numeric code incorrect: " << jpy.numericCode());
}

BOOST_AUTO_TEST_CASE(testCurrencyOutputStream) {
    BOOST_TEST_MESSAGE("Testing currency output stream operator...");

    USDCurrency usd;
    std::ostringstream oss;
    oss << usd;
    if (oss.str() != "USD")
        BOOST_ERROR("USD stream output incorrect: \"" << oss.str()
                    << "\", expected \"USD\"");
}

BOOST_AUTO_TEST_CASE(testTriangulationCurrency) {
    BOOST_TEST_MESSAGE("Testing triangulation currency is empty by default...");

    USDCurrency usd;
    if (!usd.triangulationCurrency().empty())
        BOOST_ERROR("USD should not have a triangulation currency");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
