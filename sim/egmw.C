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
#define N 100
  Int_t caen_n;
  Int_t caen_id[N];
  Int_t caen_adc_a[N];
  Int_t caen_adc_b[N];
  tree->Branch("caen_n", &caen_n, "caen_n/I");
  tree->Branch("caen_id", caen_id, "caen_id[caen_n]/I");
  tree->Branch("caen_adc_a", caen_adc_a, "caen_adc_a[caen_n]/I");
  tree->Branch("caen_adc_b", caen_adc_b, "caen_adc_b[caen_n]/I");

  TRandom rnd;
  for (unsigned ev = 0; ev < 100000; ++ev) {
    caen_n = N * rnd.Rndm();
    for (unsigned i = 0; i < N; ++i) {
      caen_id[i] = 8 + 8 * rnd.Rndm();
      float x, y;
      rnd.Rannor(x, y);
      caen_adc_a[i] = MAX(1000 + 20 * x, 1);
      caen_adc_b[i] = MAX(1000 + 20 * y, 1);
    }
    tree->Fill();
  }

  tree->Print();
  tree->Write();
  delete tree;
}
