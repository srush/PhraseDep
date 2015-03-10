// Based on Percy Liang's LISP tree code
// https://github.com/redpony/brown-cluster/blob/master/basic/lisp.cc

// Permission is granted for anyone to copy, use, or modify these programs and
// accompanying documents for purposes of research or education, provided this
// copyright notice is retained, and note is made of any changes that have been
// made.

#ifndef __LISP_H__
#define __LISP_H__

#include <vector>
#include <string>

using namespace std;

////////////////////////////////////////////////////////////

struct LispNode {
  void destroy();
  void print(int ind) const;
  bool is_leaf() const { return children.empty(); }
  bool is_preterminal() const {
    if(children.empty()){
      return false;
    }else{
      if(children.size() == 1 && children[0]->children.empty()){
        return true;
      }
      else{
        return false;
      }
    }
  }
  vector<LispNode*> to_node_list();
  string to_string() const;
  string value;
  vector<LispNode *> children;
};

////////////////////////////////////////////////////////////

struct LispTree {
  LispTree() : root(NULL) { }
  ~LispTree();

  bool read_token(istream &in, string &s);
  LispNode *read_node(const vector<string> &tokens, int &i);
  void read_from_string(string &s);
  void read(const char *file);
  void print() const;
  string to_string() const;
  vector<LispNode*> to_node_list();

  LispNode *root;
};

#endif