// Based on Percy Liang's LISP tree code
// https://github.com/redpony/brown-cluster/blob/master/basic/lisp.cc

// Permission is granted for anyone to copy, use, or modify these programs and
// accompanying documents for purposes of research or education, provided this
// copyright notice is retained, and note is made of any changes that have been
// made.

#include <iostream>
#include "lisp.h"
#include <stdlib.h>       
#include <vector>
#include <string>
#include <assert.h>
#include <fstream>
#include <sstream>
#include "indent.hpp"
#include <boost/algorithm/string/join.hpp>

using namespace std;

#define len(vec) (int)(vec).size()
// # means to string and ## means concat two stings

void LispNode::destroy() {
  for(int i = 0; i < children.size(); i++){
    LispNode* node = children[i];
    node->destroy();
    delete node;
  }
}

void LispNode::print(int ind) const {
  // (is_preterminal() ? "[PRE_TERMINAL] " : "") << (is_leaf() ? "[LEAF] " : "")
  cout << Indent(ind) << (value.empty() ? "(empty)" : value) << endl;
  for(int i = 0; i < children.size(); i++){
    LispNode* subnode = children[i];
    subnode->print(ind+1);
  }
}

string LispNode::to_string() const {
  if(!is_leaf()){
    vector<string> list;
    // the current non-terminal
    list.push_back((value.empty() ? "(empty)" : value));
    // the children
    for(int i = 0; i < children.size(); i++){
      list.push_back(children[i]->to_string());
    }
    string joined = boost::algorithm::join(list, " ");
    return "(" + joined + ")"; 
  }else{
      return (value.empty() ? "(empty)" : value);    
  }
}

vector<LispNode*> LispNode::to_node_list() {
  vector<LispNode*> list;
  list.push_back(this);
  for(int i = 0; i < children.size(); i++){
      auto sub_list = children[i]->to_node_list();
      for(int j = 0; j < sub_list.size(); j++){
        list.push_back(sub_list[j]);
      }
  }
  return list;
}

////////////////////////////////////////////////////////////

LispTree::~LispTree() {
  root->destroy();
  delete root;
}

bool is_paren(char c) {
  return c == '(' || c == ')';
}
bool is_paren(string s) {
  return s == "(" || s == ")";
}
bool is_left_paren(string s) {
  return s == "(";
}
bool is_right_paren(string s) {
  return s == ")";
}
string matching_right_paren(char c) {
  if(c == '(') return ")";
  return "";
}

// Return first non-space character.
char skip_space(istream &in) {
  char c;
  while(true) {
    c = in.peek();
    if(!isspace(c)) break;
    in.get();
  }
  return c;
}

// There are trees in the PTB which has # inside and this will cause error.

// Comments start with # and end with the line.
// There must be a space before the #.
// char skip_comments(istream &in) {
//   while(true) {
//     char c = skip_space(in);
//     if(c == '#')
//       while((c = in.peek()) != '\n') in.get();
//     else
//       return c;
//   }
// }

bool LispTree::read_token(istream &in, string &s) {
  // char c = skip_comments(in);
  // Instead of skip comments (#), only skip spaces here.

  char c = skip_space(in);
  if(is_paren(c)) {
    s = in.get();
    return true;
  }

  s = "";
  while(true) {
    c = in.peek();
    if(c == EOF) return false;
    if(isspace(c) || is_paren(c)) break;
    s += in.get();
  }

  return true;
}

LispNode *LispTree::read_node(const vector<string> &tokens, int &i) {
  LispNode *node = new LispNode();
  assert(i < len(tokens));

  string s = tokens[i++];
  if(is_left_paren(s)) {
    char left_paren = s[0];

    if(left_paren == '(') {
      assert(i < len(tokens) && !is_paren(tokens[i]));
      node->value = tokens[i++];
    }

    while(i < len(tokens) && !is_right_paren(tokens[i])) {
      node->children.push_back(read_node(tokens, i));
    }

    assert(i < len(tokens));
    s = tokens[i++];
    assert(s == matching_right_paren(left_paren));
  }
  else if(is_right_paren(s))
    assert(false);
  else
    node->value = s;

  return node;
}

void LispTree::read(const char *file) {
  ifstream in(file);
  vector<string> tokens;
  string token;
  while(read_token(in, token)) {
    tokens.push_back(token);
  }
  int i = 0;
  root = read_node(tokens, i);
  assert(i == len(tokens));
}

void LispTree::read_from_string(string &s){
  istringstream in(s);
  vector<string> tokens;
  string token;
  while(read_token(in, token)) {
    tokens.push_back(token);
  }
  int i = 0;
  root = read_node(tokens, i);
  assert(i == len(tokens));
}

void LispTree::print() const {
  assert(root);
  root->print(0);
}

string LispTree::to_string() const {
  assert(root);
  return root->to_string();
}

vector<LispNode*> LispTree::to_node_list(){
  assert(root);
  return root->to_node_list();
}
