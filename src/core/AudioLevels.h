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

namespace mpc::core
{
    // IBasicAudio's scales are hundredths of a decibel: volume runs from -10000 (silent) to 0
    // (full), balance from -10000 (left only) to 10000 (right only).
    constexpr long DirectShowSilence = -10000;

    // The player's volume is a percentage. 0, anything below it, or muted is silence; above that
    // the curve is 40 * log10(percent / 100) dB, capped at 0 (so above 100 is full volume).
    long VolumeToDirectShow(int percent, bool muted);

    // The player's balance runs from -100 (left only) through 0 (centre) to 100 (right only).
    // The channel on the opposite side is attenuated by 20 * log10(1 - |balance| / 100) dB, and
    // the ends (and anything beyond them) silence it completely.
    long BalanceToDirectShow(int balance);
}
