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
#include "../AudioLevels.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

using mpc::core::BalanceToDirectShow;
using mpc::core::DirectShowSilence;
using mpc::core::VolumeToDirectShow;

namespace
{
    // The shell's expressions as they were before the move (CPlayerToolBar::GetVolume and
    // CMainFrame::SetBalance at a087ea8c5), kept here so the move can be checked value for value.
    int LegacyVolume(int volume, bool muted)
    {
        if (muted || volume <= 0) {
            volume = -10000;
        } else {
            volume = std::min((int)(4000 * log10(volume / 100.0f)), 0);
        }
        return volume;
    }

    int LegacyBalance(int balance)
    {
        int sign = balance > 0 ? -1 : 1;
        int balance_dB;
        if (balance > -100 && balance < 100) {
            balance_dB = sign * (int)(100 * 20 * log10(1 - abs(balance) / 100.0f));
        } else {
            balance_dB = sign * (-10000);
        }
        return balance_dB;
    }
}

TEST(AudioLevels, VolumeMatchesTheShellForEveryInput)
{
    // the slider gives 0..100; the API and command line can pass more, so go a little wider
    for (int percent = -10; percent <= 110; percent++) {
        CHECK_EQ(VolumeToDirectShow(percent, false), (long)LegacyVolume(percent, false));
        CHECK_EQ(VolumeToDirectShow(percent, true), (long)LegacyVolume(percent, true));
    }
}

TEST(AudioLevels, BalanceMatchesTheShellForEveryInput)
{
    for (int balance = -110; balance <= 110; balance++) {
        CHECK_EQ(BalanceToDirectShow(balance), (long)LegacyBalance(balance));
    }
}

TEST(AudioLevels, VolumeKnownValues)
{
    CHECK_EQ(VolumeToDirectShow(100, false), 0L);
    CHECK_EQ(VolumeToDirectShow(150, false), 0L);
    CHECK_EQ(VolumeToDirectShow(50, false), -1204L);
    CHECK_EQ(VolumeToDirectShow(0, false), DirectShowSilence);
    CHECK_EQ(VolumeToDirectShow(-1, false), DirectShowSilence);
    CHECK_EQ(VolumeToDirectShow(100, true), DirectShowSilence);
}

TEST(AudioLevels, BalanceKnownValues)
{
    CHECK_EQ(BalanceToDirectShow(0), 0L);
    CHECK_EQ(BalanceToDirectShow(50), 602L);
    CHECK_EQ(BalanceToDirectShow(-50), -602L);
    CHECK_EQ(BalanceToDirectShow(100), 10000L);
    CHECK_EQ(BalanceToDirectShow(-100), -10000L);
    CHECK_EQ(BalanceToDirectShow(250), 10000L);
    CHECK_EQ(BalanceToDirectShow(-250), -10000L);
}

TEST(AudioLevels, VolumeIsMonotonic)
{
    long previous = VolumeToDirectShow(0, false);
    for (int percent = 1; percent <= 100; percent++) {
        const long current = VolumeToDirectShow(percent, false);
        CHECK(current >= previous);
        previous = current;
    }
}
