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

#include <memory>

namespace mpc::core
{
    // The playback engine being extracted from CMainFrame (docs/architecture-decision.md,
    // phase 1). Responsibilities move behind this interface one at a time, following the seam
    // map in docs/Engine-Seam.md; the shell keeps working unchanged after each move. A flat C
    // ABI is added over this interface later (phase 1 step 4), so keep it free of MFC types.
    class IEngine
    {
    public:
        virtual ~IEngine() = default;
    };

    std::unique_ptr<IEngine> CreateEngine();
}
