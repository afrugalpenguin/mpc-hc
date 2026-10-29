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

#include "AudioLevels.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace mpc::core
{
    // Both expressions are moved verbatim from the shell (CPlayerToolBar::GetVolume and
    // CMainFrame::SetBalance / OpenSetupAudio), including the float arithmetic, so the values
    // handed to IBasicAudio do not change. src/core/tests/AudioLevelsTests.cpp pins that.

    long VolumeToDirectShow(int percent, bool muted)
    {
        if (muted || percent <= 0) {
            return DirectShowSilence;
        }
        return std::min((int)(4000 * log10(percent / 100.0f)), 0); // 4000=2.0*100*20, where 2.0 is a special factor
    }

    long BalanceToDirectShow(int balance)
    {
        int sign = balance > 0 ? -1 : 1; // -1: invert sign for more right channel
        if (balance > -100 && balance < 100) {
            return sign * (int)(100 * 20 * log10(1 - abs(balance) / 100.0f));
        }
        return sign * DirectShowSilence; // -10000: only left, 10000: only right
    }
}
