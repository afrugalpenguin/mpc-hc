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

#include "Test.h"

#include <cstdio>

namespace mpc::core::test
{
    namespace
    {
        int g_failures = 0;
    }

    std::vector<Case>& Registry()
    {
        static std::vector<Case> cases;
        return cases;
    }

    void Fail(const char* file, int line, const std::string& what)
    {
        ++g_failures;
        std::printf("    %s(%d): %s\n", file, line, what.c_str());
    }
}

int main()
{
    using namespace mpc::core::test;

    int failedCases = 0;
    for (const auto& c : Registry()) {
        const int before = g_failures;
        c.fn();
        const bool ok = g_failures == before;
        failedCases += ok ? 0 : 1;
        std::printf("%s %s.%s\n", ok ? "[ OK ]" : "[FAIL]", c.suite, c.name);
    }

    std::printf("\n%zu tests, %d failed, %d failed checks\n", Registry().size(), failedCases, g_failures);
    return (failedCases == 0 && !Registry().empty()) ? 0 : 1;
}
