/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2023 Klaus Spanderen

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

/*! \file optional.hpp
    \brief Deprecated compatibility header; use <optional> directly
    \deprecated Use std::optional and std::nullopt instead.
*/

#ifndef quantlib_optional_hpp
#define quantlib_optional_hpp

#include <ql/qldefines.hpp>
#include <optional>

namespace QuantLib::ext {

    /*! \deprecated Use std::optional instead. */
    using std::optional;                    // NOLINT(misc-unused-using-decls)
    /*! \deprecated Use std::nullopt instead. */
    inline constexpr const std::nullopt_t& nullopt = std::nullopt;

}

#endif
