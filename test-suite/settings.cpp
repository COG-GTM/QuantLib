/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2021 StatPro Italia srl

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
#include <ql/settings.hpp>

using namespace QuantLib;
using namespace boost::unit_test_framework;

BOOST_FIXTURE_TEST_SUITE(QuantLibTests, TopLevelFixture)

BOOST_AUTO_TEST_SUITE(SettingsTests)

BOOST_AUTO_TEST_CASE(testNotificationsOnDateChange) {
    BOOST_TEST_MESSAGE("Testing notifications on evaluation-date change...");

#ifdef QL_HIGH_RESOLUTION_DATE

    Date d1(11, February, 2021, 9, 17, 0);
    Date d2(11, February, 2021, 10, 21, 0);

#else

    Date d1(11, February, 2021);
    Date d2(12, February, 2021);

#endif

    Settings::instance().evaluationDate() = d1;

    Flag flag;
    flag.registerWith(Settings::instance().evaluationDate());

    // Set to same date, no notification
    Settings::instance().evaluationDate() = d1;

    if (flag.isUp())
        BOOST_ERROR("unexpected notification");

    // Set to different date, notification expected
    Settings::instance().evaluationDate() = d2;

    if (!flag.isUp())
        BOOST_ERROR("missing notification");
}

BOOST_AUTO_TEST_CASE(testSavedSettings) {
    BOOST_TEST_MESSAGE("Testing that SavedSettings restores state...");

    Date original = Settings::instance().evaluationDate();
    bool origRefDateEvents = Settings::instance().includeReferenceDateEvents();
    bool origEnforceFixings = Settings::instance().enforcesTodaysHistoricFixings();

    {
        SavedSettings saved;
        Settings::instance().evaluationDate() = Date(1, January, 2000);
        Settings::instance().includeReferenceDateEvents() = !origRefDateEvents;
        Settings::instance().enforcesTodaysHistoricFixings() = !origEnforceFixings;
    }

    Date restored = Settings::instance().evaluationDate();
    if (restored != original)
        BOOST_ERROR("SavedSettings did not restore evaluation date: "
                    << restored << " vs " << original);

    if (Settings::instance().includeReferenceDateEvents() != origRefDateEvents)
        BOOST_ERROR("SavedSettings did not restore includeReferenceDateEvents");

    if (Settings::instance().enforcesTodaysHistoricFixings() != origEnforceFixings)
        BOOST_ERROR("SavedSettings did not restore enforcesTodaysHistoricFixings");
}

BOOST_AUTO_TEST_CASE(testIncludeReferenceDateEvents) {
    BOOST_TEST_MESSAGE("Testing includeReferenceDateEvents flag...");

    Settings::instance().includeReferenceDateEvents() = false;
    if (Settings::instance().includeReferenceDateEvents())
        BOOST_ERROR("includeReferenceDateEvents should be false");

    Settings::instance().includeReferenceDateEvents() = true;
    if (!Settings::instance().includeReferenceDateEvents())
        BOOST_ERROR("includeReferenceDateEvents should be true");
}

BOOST_AUTO_TEST_CASE(testIncludeTodaysCashFlows) {
    BOOST_TEST_MESSAGE("Testing includeTodaysCashFlows optional flag...");

    Settings::instance().includeTodaysCashFlows() = ext::nullopt;
    if (Settings::instance().includeTodaysCashFlows())
        BOOST_ERROR("includeTodaysCashFlows should be nullopt");

    Settings::instance().includeTodaysCashFlows() = true;
    if (!Settings::instance().includeTodaysCashFlows() ||
        !(*Settings::instance().includeTodaysCashFlows()))
        BOOST_ERROR("includeTodaysCashFlows should be true");

    Settings::instance().includeTodaysCashFlows() = false;
    if (!Settings::instance().includeTodaysCashFlows() ||
        *Settings::instance().includeTodaysCashFlows())
        BOOST_ERROR("includeTodaysCashFlows should be false");
}

BOOST_AUTO_TEST_CASE(testAnchorAndResetEvaluationDate) {
    BOOST_TEST_MESSAGE("Testing anchor and reset of evaluation date...");

    Date d(15, March, 2020);
    Settings::instance().evaluationDate() = d;

    Settings::instance().anchorEvaluationDate();
    Date anchored = Settings::instance().evaluationDate();
    if (anchored != d)
        BOOST_ERROR("Anchored date " << anchored << " != " << d);

    Settings::instance().resetEvaluationDate();
    Date reset = Settings::instance().evaluationDate();
    if (reset != Date::todaysDate())
        BOOST_ERROR("Reset date " << reset << " != today "
                    << Date::todaysDate());
}

BOOST_AUTO_TEST_CASE(testEnforcesTodaysHistoricFixings) {
    BOOST_TEST_MESSAGE("Testing enforcesTodaysHistoricFixings flag...");

    Settings::instance().enforcesTodaysHistoricFixings() = false;
    if (Settings::instance().enforcesTodaysHistoricFixings())
        BOOST_ERROR("enforcesTodaysHistoricFixings should be false");

    Settings::instance().enforcesTodaysHistoricFixings() = true;
    if (!Settings::instance().enforcesTodaysHistoricFixings())
        BOOST_ERROR("enforcesTodaysHistoricFixings should be true");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
