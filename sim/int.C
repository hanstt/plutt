/*
 * plutt, a scriptable monitor for experimental data.
 *
 * Copyright (C) 2023-2025
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
  TFile file("int.root", "RECREATE");

  auto tree = new TTree("T", "Integer mock");

  Int_t ij[2];
  tree->Branch("i", &ij[0], "i/I");
  tree->Branch("j", &ij[1], "j/I");

  TRandom3 rnd;
  for (unsigned ev = 0; ev < 100000; ++ev) {
    for (unsigned i = 0; i < 2; ++i) {
      for (;;) {
        auto x = rnd.Uniform(-4, 4);
        auto y = rnd.Uniform(1.2);
        auto p = exp(-x*x) + (0 == i ? exp(-0.3*(x+6)) : -0.005*x+0.1);
        if (y < p) {
          ij[i] = floor(2 * x);
          break;
        }
      }
    }
    tree->Fill();
  }

  tree->Print();
  tree->Write();
  delete tree;
}
