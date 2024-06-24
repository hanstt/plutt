/*
 * plutt, a scriptable monitor for experimental data.
 *
 * Copyright (C) 2023  Hans Toshihide Toernqvist <hans.tornqvist@chalmers.se>
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

#if PLUTT_ROOT

#include <iostream>
#include <list>

#include <TFile.h>
#include <TTree.h>

#include <trig_map.hpp>
#include <config.hpp>
#include <root.hpp>
#include <test/test_root.hpp>
#include <test/test.hpp>

namespace {

class MyTest: public Test {
  void Run();
};
MyTest g_test_root_;

void MyTest::Run()
{
  // Write a file with a bunch of branches.
  // NOTE: This code assumes that config sorts aliases in alphabetic order.
#define FILENAME "plutt_test_.root"
  auto file = new TFile(FILENAME, "recreate");
  auto tree = new TTree("tree", "Type test tree");
#define BRANCH(Type, name) \
    Type name; \
    tree->Branch(#name, &name)
  BRANCH(Double_t, d);
  BRANCH(Float_t, f);
  auto cls = new MyClass;
  tree->Branch("cls", &cls, sizeof *cls, 2);
  BRANCH(UChar_t, uc);
  BRANCH(UInt_t, ui);
  BRANCH(ULong_t, ul);
  BRANCH(UShort_t, us);
  for (unsigned char i = 0; i < 10; ++i) {
    d = i;
    f = i;
    cls->d = i;
    cls->f = i;
    cls->uc = i;
    cls->ui = i;
    cls->ul = i;
    cls->us = i;
    uc = i;
    ui = i;
    ul = i;
    us = i;
    tree->Fill();
  }
  file->Write();
  delete tree;
  delete file;
  delete cls;

  auto config = new Config("test/test_root.plutt");
  char *argv[2];
  argv[0] = strdup("tree");
  argv[1] = strdup(FILENAME);
  auto root = new Root(true, config, 2, argv);
  free(argv[0]);
  free(argv[1]);

  for (unsigned char i = 0; i < 10; ++i) {
    root->Fetch();
    root->Buffer();

    auto data_cls_d = root->GetData(0);
    auto data_cls_f = root->GetData(1);
    auto data_cls_uc = root->GetData(2);
    auto data_cls_ui = root->GetData(3);
    auto data_cls_ul = root->GetData(4);
    auto data_cls_us = root->GetData(5);
    auto data_d = root->GetData(6);
    auto data_f = root->GetData(7);
    auto data_uc = root->GetData(8);
    auto data_ui = root->GetData(9);
    auto data_ul = root->GetData(10);
    auto data_us = root->GetData(11);

    TEST_CMP(std::abs(data_cls_d.first->dbl - i), <, 1e-9);
    TEST_CMP(std::abs(data_cls_f.first->dbl - i), <, 1e-9);
    TEST_CMP(data_cls_uc.first->u64, ==, i);
    TEST_CMP(data_cls_ui.first->u64, ==, i);
    TEST_CMP(data_cls_ul.first->u64, ==, i);
    TEST_CMP(data_cls_us.first->u64, ==, i);
    TEST_CMP(std::abs(data_d.first->dbl - i), <, 1e-9);
    TEST_CMP(std::abs(data_f.first->dbl - i), <, 1e-9);
    TEST_CMP(data_uc.first->u64, ==, i);
    TEST_CMP(data_ui.first->u64, ==, i);
    TEST_CMP(data_ul.first->u64, ==, i);
    TEST_CMP(data_us.first->u64, ==, i);
  }

  delete root;
  delete config;

  remove(FILENAME);
}

}

#endif
