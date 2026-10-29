/*
 * (C) 2026 see Authors.txt
 *
 * This file is part of MPC-HC.
 *
 * MPC-HC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-HC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

// Minimal test runner for mpc-core: no framework, no dependencies.
//
//   TEST(VolumeConversion, MuteIsSilent) {
//       CHECK(VolumeToDirectShow(50, true) == -10000);
//       CHECK_EQ(VolumeToDirectShow(100, false), 0);
//   }
//
// A failed check reports file:line and the expression, and the test carries on; the process
// exits non-zero if any check failed, which is what CI looks at.

#include <sstream>
#include <string>
#include <vector>

namespace mpc::core::test
{
    struct Case {
        const char* suite;
        const char* name;
        void (*fn)();
    };

    std::vector<Case>& Registry();
    void Fail(const char* file, int line, const std::string& what);

    struct Registrar {
        Registrar(const char* suite, const char* name, void (*fn)()) {
            Registry().push_back({ suite, name, fn });
        }
    };
}

#define MPC_TEST_CONCAT2(a, b) a##b
#define MPC_TEST_CONCAT(a, b) MPC_TEST_CONCAT2(a, b)

#define TEST(suite, name)                                                                    \
    static void MPC_TEST_CONCAT(test_, MPC_TEST_CONCAT(suite, MPC_TEST_CONCAT(_, name)))();  \
    static const mpc::core::test::Registrar MPC_TEST_CONCAT(reg_, __LINE__)(                 \
        #suite, #name, &MPC_TEST_CONCAT(test_, MPC_TEST_CONCAT(suite, MPC_TEST_CONCAT(_, name)))); \
    static void MPC_TEST_CONCAT(test_, MPC_TEST_CONCAT(suite, MPC_TEST_CONCAT(_, name)))()

#define CHECK(expr)                                                                          \
    do {                                                                                     \
        if (!(expr)) {                                                                       \
            mpc::core::test::Fail(__FILE__, __LINE__, "CHECK(" #expr ")");                  \
        }                                                                                    \
    } while (false)

#define CHECK_EQ(actual, expected)                                                           \
    do {                                                                                     \
        const auto& a_ = (actual);                                                           \
        const auto& e_ = (expected);                                                         \
        if (!(a_ == e_)) {                                                                   \
            std::ostringstream os_;                                                          \
            os_ << "CHECK_EQ(" #actual ", " #expected "): got " << a_ << ", expected " << e_; \
            mpc::core::test::Fail(__FILE__, __LINE__, os_.str());                            \
        }                                                                                    \
    } while (false)
