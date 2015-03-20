// Based on Percy Liang's LISP tree code
// https://github.com/redpony/brown-cluster/blob/master/basic/lisp.cc

// Permission is granted for anyone to copy, use, or modify these programs and
// accompanying documents for purposes of research or education, provided this
// copyright notice is retained, and note is made of any changes that have been
// made.

#ifndef __INDENT_H__
#define __INDENT_H__

#include <iostream>

using namespace std;

struct Indent {
  Indent(int level) : level(level) { }
  int level;
};

inline ostream &operator<<(ostream &out, const Indent &ind) {
  for(int i = 0; i < ind.level; i++) out << "  ";
  return out;
}

#endif