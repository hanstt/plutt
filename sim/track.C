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
	TFile file("track.root", "RECREATE");

	auto tree = new TTree("T", "Track mock");

#define N 50

	int adc, adcI[N], adcv[N];
	tree->Branch("adc", &adc, "adc/I");
	tree->Branch("adcI", &adcI, "adcI[adc]/I");
	tree->Branch("adcv", &adcv, "adcv[adc]/I");

	TRandom rnd;
	for (unsigned ev = 0; ev < 1000000; ++ev) {
		adc = N;
		for (int i = 0; i < N; ++i) {
			int j = N/2 + i * 0.5 * (rnd.Rndm() - 0.5);
			adcI[i] = j + i * N;
			adcv[i] = 1;
		}
		tree->Fill();
	}

	tree->Print();
	tree->Write();
	delete tree;
}
