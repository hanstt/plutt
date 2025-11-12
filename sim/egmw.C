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
  TFile file("egmw.root", "RECREATE");

  auto tree = new TTree("T", "egmwsort mock");

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define N 8
  Int_t caen_n;
  Int_t caen_id[N];
  Int_t caen_adc_a[N];
  Int_t caen_adc_b[N];
  tree->Branch("caen_n", &caen_n, "caen_n/I");
  tree->Branch("caen_id", caen_id, "caen_id[caen_n]/I");
  tree->Branch("caen_adc_a", caen_adc_a, "caen_adc_a[caen_n]/I");
  tree->Branch("caen_adc_b", caen_adc_b, "caen_adc_b[caen_n]/I");

  TRandom3 rnd;
  for (unsigned ev = 0; ev < 100000; ++ev) {
    caen_n = 0;
    for (unsigned i = 0; i < N; ++i) {
      if (rnd.Uniform() < 0.5) {
        continue;
      }
      caen_id[caen_n] = i;
      float xy[2];
      for (unsigned j = 0; j < 2; ++j) {
        for (;;) {
          auto x = rnd.Uniform(-4, 4);
          auto y = rnd.Uniform(1.2);
          auto p = exp(-x*x) + (0 == j ? exp(-0.3*(x+6)) : -0.01*x+0.1);
          if (y < p) {
            xy[j] = x;
            break;
          }
        }
      }
      caen_adc_a[caen_n] = MAX(1000 + 20 * xy[0], 1);
      caen_adc_b[caen_n] = MAX(1000 + 20 * xy[1], 1);
      ++caen_n;
    }
    tree->Fill();
  }

  tree->Print();
  tree->Write();
  delete tree;
}
