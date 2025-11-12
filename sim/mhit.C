/*
 * plutt, a scriptable monitor for experimental data.
 *
 * Copyright (C) 2025
 * Hans Toshihide Toernqvist <hans.tornqvist@chalmers.se>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

{
  TFile file("mhit.root", "RECREATE");

  auto tree = new TTree("T", "Multi-hit mock");

  unsigned n;
  unsigned x[10];
  unsigned y[10];
  tree->Branch("n", &n, "n/i");
  tree->Branch("x", x, "x[n]/i");
  tree->Branch("y", y, "y[n]/i");

  TRandom3 rnd;
  for (unsigned ev = 0; ev < 100000; ++ev) {
    n = 0;
    for (unsigned i = 0; i < 10; ++i) {
      if (rnd.Uniform() < 0.5) {
        continue;
      }
      x[n] = i;
      y[n] = 9 - i;
      ++n;
    }
    tree->Fill();
  }

  tree->Print();
  tree->Write();
  delete tree;
}
