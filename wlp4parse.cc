#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <deque>
#include "wlp4data.h"

using namespace std;

const string CFG = ".CFG";
const string INPUT = ".INPUT";
const string TRANSITIONS = ".TRANSITIONS";
const string REDUCTIONS = ".REDUCTIONS";
const string END = ".END";
const string EMPTY = ".EMPTY";
const string ACCEPT = ".ACCEPT";

struct Production {
  string LHS;
  string RHS;
  Production(string l, string r) : LHS{l}, RHS{r} {}
};

struct State {
  string stateNum;
  bool reducible = false;
  bool accepting = false;
  int prodToReduceTo;
  map<string, string> stateLookup = {}; // nextTOKEN -> stateNum
  vector<string> followSet = {};
  friend std::ostream& operator<<(std::ostream& os, const State& state);
};

struct SymTree {
  string symbol = "";
  vector<SymTree *> children;

  bool leaf;
  string lexeme;
  string prodRule;
};

// to ensure follow sets were added correctly:
std::ostream& operator<<(std::ostream& os, const State& state) {
    unsigned fSetSize = state.followSet.size();
    os << "fSet ";
    if (state.followSet.size() == 0) {
      os <<  "is empty.";
    } else {
      os << "{";
      for (unsigned i = 0; i < fSetSize - 1; ++i) {
        cout << state.followSet[i] << ", ";
      }
      os << state.followSet[fSetSize - 1] << "}";
    }
    return os;
}

// determines whether the top state in stateStack is reducible
// if so, modifies n to store the production rule to reduce to
bool canReduce(vector<State> stateStack, string a, int &n) {
  bool containedInFset = false;
  State top = stateStack.back();
  if (stateStack.size() == 0) return false;
  if (stateStack.back().reducible == false) return false;
  for (unsigned i = 0; i < top.followSet.size(); ++i) {
    if (top.followSet.at(i) == a) {
      containedInFset = true;
    }
  }
  if (containedInFset == false && top.accepting == false) return false;
  n = top.prodToReduceTo;
  return true;
}

void printParse(vector<SymTree> symStack, istringstream &input, bool afterReduce, string curSym) {  
  string redSeq = "";
  for (unsigned i = 0; i < symStack.size(); ++i) {
    redSeq = redSeq + " " + symStack.at(i).symbol;
  }
  // to calculate length of remainIn so we can use if (len > 0) condition
  // note: we don't need to
  // std::streampos currentPosition = input.tellg();
  // // Set the read position to the end of the stream
  // input.seekg(0, std::ios::end);
  // // Calculate the length of the remaining string
  // std::streamsize remainingLength = input.tellg() - currentPosition;
  // // Reset the read position to the original position
  // input.seekg(currentPosition);
  string remainIn(input.str().substr(input.tellg()));
  string outputSeq;
  if (afterReduce == false) {
    // we shifted, so consumed symbol is in redSeq
    outputSeq = redSeq + " . " + remainIn;
  } else {
    // we reduced, so we need to add the consumed symbol to our remaining input
    outputSeq = redSeq + " . " + curSym + remainIn;
  }
  outputSeq.erase(0, 1); // remove extra space at the start
  outputSeq.pop_back(); // and the end
  cout << outputSeq << endl << endl;
}

void printParseTree(SymTree *t) {
  cout << t->symbol;
  if (t->leaf == true) {
    cout << " " << t->lexeme << endl;
  } else {
    cout << " " << t->prodRule << endl;
  }
  for (unsigned i = 0; i < t->children.size(); ++i) {
    printParseTree(t->children[i]);
  }
}

void deleteSymTrees(SymTree *root) {
  for (auto &subtree : root->children) {
    deleteSymTrees(subtree);
  }
  delete root;
}

SymTree *mergeIntoOne(vector<SymTree *> symStack) {
  SymTree *t = new SymTree;
  t->symbol = "start";
  t->children.push_back(symStack[0]);
  t->children.push_back(symStack[1]);
  t->children.push_back(symStack[2]);
  t->leaf = false;
  t->prodRule = "BOF procedures EOF";
  return t;
}

int main() {
  istringstream in{WLP4_COMBINED};
  string s, t;
  char c;
  vector<Production> prodRules;
  string inputStr = "";
  string augStartSym;
  bool firstRead = true;

  getline(in, s); // CFG section (skip header)
  while (in >> s) {
    // load production rules into prodRules stack
    if (s == TRANSITIONS) break;
    if (firstRead) {
      augStartSym = s;
      firstRead = false;
    }
    in >> noskipws >> c >> skipws;
    getline(in, t);
    Production newProd = Production(s, t);
    prodRules.push_back(newProd);
  }

  getline(in, s); // TRANSITIONS section (skip header)
  map<string, State> states; // stateNum -> State
  while (getline(in, s)) {
    // create State if new; else update stateLookup
    if (s == REDUCTIONS) break;
    istringstream trans{s};
    string oldNum, tok, newNum;
    trans >> oldNum >> tok >> newNum;
    if (states.find(oldNum) == states.end()) {
      State newState = State();
      newState.stateNum = oldNum;
      newState.stateLookup[tok] = newNum;
      states[oldNum] = newState;
    } else {
      states[oldNum].stateLookup[tok] = newNum;
    }
  }

  // REDUCTIONS section (already skipped header)
  while (getline(in, s)) {
    // create new states when encountering them, and store reduction information
    if (s == END) break;
    istringstream reducs{s};
    string stateNum, ruleNum, tag;
    reducs >> stateNum >> ruleNum >> tag;
    if (states.find(stateNum) == states.end()) {
      State newState = State();
      newState.stateNum = stateNum;
      states[stateNum] = newState;
    } 
    states[stateNum].reducible = true;
    states[stateNum].prodToReduceTo = stoi(ruleNum);
    // store lookahead set in corresponding state, or flag as accepting state
    if (tag == ACCEPT) {
      states[stateNum].accepting = true;
    } else { 
      states[stateNum].followSet.push_back(tag);
    }
  }

  // print out all states that were read in
  // for (const auto& pair : states) {
  //     std::cout << "Key: " << pair.first << ", Value: " << pair.second << 
  //     std::endl;
  // }

  deque<string> lexemeStack = {};
  lexemeStack.push_back("BOF");
  while (getline(cin, s)) {
    // load input string
    istringstream line{s};
    line >> s;
    inputStr += s;
    inputStr.push_back(' ');
    line >> s;
    lexemeStack.push_back(s);
  }
  lexemeStack.push_back("EOF");
  inputStr = "BOF " + inputStr + "EOF ";

  // SLR(1) Algorithm Implementation
  istringstream input{inputStr};
  vector<SymTree *> symStack = {};
  vector<State> stateStack = {};
  int n;
  int numShifts = 0;
  // cout << "Input: " << inputStr << endl;

  stateStack.push_back(states["0"]);
  // printParse(symStack, input, false, "");  
  while (input >> s) {
    // Note: we consumed the input symbol, which is important for printParse
    // in the case that we are printing after a reduction
    while (canReduce(stateStack, s, n)) {
      // reduce as before, but pop and push to stateStack as well
      // cout << "can reduce using production " << n << endl;
      string desiredRHS = prodRules.at(n).RHS;
      string reduction = prodRules.at(n).LHS;
      string partialReduc = "";
      SymTree *partialSymTree = new SymTree;
      while (true) {
        if (partialReduc == desiredRHS ||
          (partialReduc == "" && desiredRHS == EMPTY)) {
          // desiredRHS is already popped from symStack
          // push reduction onto symStack and push new state
          SymTree *newSymTree = new SymTree;
          newSymTree->symbol = reduction;
          newSymTree->children = partialSymTree->children;
          delete partialSymTree;
          newSymTree->leaf = false;
          newSymTree->prodRule = desiredRHS;
          symStack.push_back(newSymTree);
          State newState = states.at(stateStack.back().stateLookup.at(reduction));
          stateStack.push_back(newState);
          //cout << "     reduce:" << endl;
          // printParse(symStack, input, true, s);
          break;
        }
        // pop top of symStack and add to LHS of partialReduc
        // also pop from stateStack
        string topSymStack = symStack.back()->symbol;
        SymTree *newChild = symStack.back();
        partialSymTree->children.push_back(newChild);
        // push to FRONT of the stack
        for (int i = partialSymTree->children.size() - 1; i > 0; --i) {
          partialSymTree->children[i] = partialSymTree->children[i-1];
        }
        partialSymTree->children[0] = newChild;
        symStack.pop_back();
        if (partialReduc == "") {
          partialReduc = topSymStack;
        } else {
          partialReduc = topSymStack + " " + partialReduc;
        }
        stateStack.pop_back();
      }
    }
    // symStack.push a (shift)
    input >> noskipws >> c >> skipws;
    SymTree *shiftedSymTree = new SymTree;
    shiftedSymTree->symbol = s;
    shiftedSymTree->leaf = true;
    shiftedSymTree->lexeme = lexemeStack.front();
    // pop from FRONT of lexeme stack
    lexemeStack.pop_front();
    symStack.push_back(shiftedSymTree);
    // cout << "     shift:"<< endl;
    // printParse(symStack, input, false, "");
    ++numShifts;
    // reject if there is no next state in DFA
    State top = stateStack.back();
    if (top.stateLookup.find(s) == top.stateLookup.end()) {
      cerr << "ERROR at " << numShifts - 1 << endl;
      for (auto &stackElem : symStack) {
        deleteSymTrees(stackElem);
      }
      return 1;
    }
    // stateStack.push next state
    State newState = states.at(top.stateLookup.at(s));
    stateStack.push_back(newState);
  }
  // accept
  SymTree *theTree = mergeIntoOne(symStack);
  // Print Parse Tree
  printParseTree(theTree);
  // Free memory
  deleteSymTrees(theTree);
  return 0;
}