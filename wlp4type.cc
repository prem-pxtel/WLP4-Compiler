#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <map>

using namespace std;

const std::string WLP4_CFG = R"END(.CFG
start BOF procedures EOF
procedures procedure procedures
procedures main
procedure INT ID LPAREN params RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE
main INT WAIN LPAREN dcl COMMA dcl RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE
params .EMPTY
params paramlist
paramlist dcl
paramlist dcl COMMA paramlist
type INT
type INT STAR
dcls .EMPTY
dcls dcls dcl BECOMES NUM SEMI
dcls dcls dcl BECOMES NULL SEMI
dcl type ID
statements .EMPTY
statements statements statement
statement lvalue BECOMES expr SEMI
statement IF LPAREN test RPAREN LBRACE statements RBRACE ELSE LBRACE statements RBRACE
statement WHILE LPAREN test RPAREN LBRACE statements RBRACE
statement PRINTLN LPAREN expr RPAREN SEMI
statement DELETE LBRACK RBRACK expr SEMI
test expr EQ expr
test expr NE expr
test expr LT expr
test expr LE expr
test expr GE expr
test expr GT expr
expr term
expr expr PLUS term
expr expr MINUS term
term factor
term term STAR factor
term term SLASH factor
term term PCT factor
factor ID
factor NUM
factor NULL
factor LPAREN expr RPAREN
factor AMP lvalue
factor STAR factor
factor NEW INT LBRACK expr RBRACK
factor ID LPAREN RPAREN
factor ID LPAREN arglist RPAREN
arglist expr
arglist expr COMMA arglist
lvalue ID
lvalue STAR factor
lvalue LPAREN lvalue RPAREN
)END";

struct Production {
  string LHS;
  string RHS;
  Production(string l, string r) : LHS{l}, RHS{r} {}
};
vector<Production> prodRules;

struct SymTree {
  string symbol = "";
  vector<SymTree *> children;
  string rule;
  bool leaf;
  string lexeme;
  string prodRule;
  string type = "";
  // SymTree *parent;

  SymTree *getChild(string key, int n = 1) {
    // Return the n'th instance of key in children
    int count = 0;
    for (auto &subtree : children) {
      if (subtree->symbol == key) {
        ++count;
        if (count == n) return subtree;
      } 
    }
    return nullptr;
  }
};

class Signature {
  vector<string> argTypes;
 public:
  vector<string> getSig() {
    return argTypes;
  }
  void pushType(string type) {
    argTypes.push_back(type);
  }
};

class SymbolTable {
  map<string, string> locSymTable;
 public:
  string getVarType(string name) {
    if (locSymTable.find(name) == locSymTable.end()) return "";
    return locSymTable[name];
  }
  void pushType(string name, string type) {
    locSymTable[name] = type;
  }
};

class Err {
  string message;
  public:
    Err(string message) : message{message} {}
    // Returns the message associated with the exception.
    const string &msg() const { return message; }
};

void loadCFG() {
  string s, t;
  char c;
  istringstream CFG_stream{WLP4_CFG};
  getline(CFG_stream, s); // CFG section (skip header)
  while (CFG_stream >> s) {
    // load production rules into prodRules stack
    CFG_stream >> noskipws >> c >> skipws;
    getline(CFG_stream, t);
    Production newProd = Production(s, t);
    prodRules.push_back(newProd);
  }
}

istringstream loadInput() {
  string inputStr, s;
  while (getline(cin, s)) {
    // load input string
    inputStr += s;
    inputStr += '\n';
  }
  istringstream i{inputStr};
  return i;
}

bool isInCFG(string left, string right) {
  for (auto &production : prodRules) {
    if (production.LHS == left && production.RHS == right) return true;
  }
  return false;
}

int wcount(string s) {
  int count = 0;
  istringstream i{s};
  while (i >> s) ++count;
  return count; 
}

SymTree *buildFirstTree(istringstream &raw) {
  string s, left, right;
  char c;
  raw >> left;
  raw >> noskipws >> c >> skipws;
  getline(raw, right);

  SymTree *theTree = new SymTree;
  theTree->symbol = left;
  // theTree->parent = nullptr;
  if (isInCFG(left, right)) {
    theTree->leaf = false;
    theTree->prodRule = right;
    theTree->rule = theTree->symbol + " " + theTree->prodRule;
    int RHS_word_count = wcount(right);
    if (right == ".EMPTY") --RHS_word_count;
    for (int i = 0; i < RHS_word_count; ++i) {
      theTree->children.push_back(buildFirstTree(raw));
      // theTree->children[i]->parent = theTree;
    }
  } else {
    theTree->leaf = true;
    theTree->lexeme = right;
  }
  return theTree;
}

void printSymTree(SymTree *root) {
  cout << root->symbol << " ";
  if (root->leaf == true) {
    cout << root->lexeme;
  } else {
    cout << root->prodRule;
  }
  if (root->type != "") {
    cout << " : " << root->type;
  }
  cout << endl;
  for (auto &subtree : root->children) {
    // cout << subtree.children.size() << "   ";
    printSymTree(subtree);
  }
}

void deleteSymTrees(SymTree *root) {
  for (auto &subtree : root->children) {
    deleteSymTrees(subtree);
  }
  delete root;
}

map<string, pair<Signature, SymbolTable>> globalSymTable;

void pushParams(SymTree *root, Signature &sig) {
  string type;
  if (root->symbol == "dcl") {
    if (root->getChild("type")->prodRule == "INT") {
      type = "int";
    } else {
      type = "int*";
    }
    sig.pushType(type);
  }
  // recurse on subtrees
  for (auto &subtree : root->children) {
    pushParams(subtree, sig);
  }
}

bool checkArgs(SymTree *root, Signature &s, unsigned count = 1) {
  // root is an arglist node
  // can have children "expr" or "expr COMMA arglist"
  vector<string> sig = s.getSig();
  if (count > sig.size()) return false; // too many args
  if (root->getChild("expr")->type != sig.at(count - 1)) return false;
  if (root->prodRule == "expr") {
    if (count == sig.size()) return true;
  }
  else {
    // arglist child is present
    return checkArgs(root->getChild("arglist"), s, count + 1);
  }
  return false;
}

void buildSymTable(SymTree *root, string curProcName = "wain") {
  Signature curSig = Signature();
  // add entry in global symbol table for WAIN
  if (root->symbol == "main") {
    SymbolTable loc = SymbolTable();
    pushParams(root->getChild("dcl"), curSig);
    pushParams(root->getChild("dcl", 2), curSig);
    globalSymTable[curProcName] = make_pair(curSig, loc);
  }
  if (root->symbol == "procedure") {
    curProcName = root->getChild("ID")->lexeme;
    if (globalSymTable.find(curProcName) != globalSymTable.end()) {
      throw Err("ERROR1");
    }
    SymbolTable loc = SymbolTable();
    pushParams(root->getChild("params"), curSig);
    globalSymTable[curProcName] = make_pair(curSig, loc);
  }
  string varName, type;
  if (root->symbol == "dcl") {
    if (root->getChild("type")->prodRule == "INT") {
      type = "int";
    } else {
      type = "int*";
    }
    varName = root->getChild("ID")->lexeme;
    if (globalSymTable[curProcName].second.getVarType(varName) != "") {
      throw Err("ERROR2");
    }
    globalSymTable[curProcName].second.pushType(varName, type);
  }
  if (root->rule == "factor ID" ||
      root->rule == "lvalue ID") {
    varName = root->getChild("ID")->lexeme;
    if (globalSymTable[curProcName].second.getVarType(varName) == "") {
      throw Err("ERROR3");
    }
  }
  if (root->rule == "factor ID LPAREN RPAREN" ||
      root->rule == "factor ID LPAREN arglist RPAREN") {
    string procCall = root->getChild("ID")->lexeme;
    if (globalSymTable.find(procCall) == globalSymTable.end()) {
      throw Err("ERROR4");
    }
  }
  // recurse on subtrees
  for (auto &subtree : root->children) {
    buildSymTable(subtree, curProcName);
  }
}

void annotateTypes(SymTree *root, string curProcName = "wain") {
  if (root->symbol == "procedure") {
    curProcName = root->getChild("ID")->lexeme;
  }
  // recurse on subtrees
  for (auto &subtree : root->children) {
    annotateTypes(subtree, curProcName);
  }
  // base cases
  if (root->symbol == "NUM") root->type = "int";
  if (root->symbol == "NULL") root->type = "int*";
  if (root->rule == "factor ID" || root->rule == "lvalue ID") {
    root->getChild("ID")->type = globalSymTable[curProcName].second.getVarType(root->getChild("ID")->lexeme);
    root->type = root->getChild("ID")->type;
  }
  if (root->rule == "dcl type ID") {
    root->getChild("ID")->type = globalSymTable[curProcName].second.getVarType(root->getChild("ID")->lexeme);
  }
  if (root->rule == "expr term") root->type = root->getChild("term")->type;
  if (root->rule == "term factor") root->type = root->getChild("factor")->type;
  if (root->rule == "factor NUM") root->type = root->getChild("NUM")->type;
  if (root->rule == "factor NULL") root->type = root->getChild("NULL")->type;
  if (root->rule == "factor LPAREN expr RPAREN") root->type = root->getChild("expr")->type;
  if (root->rule == "expr expr PLUS term") {
    if (root->getChild("expr")->type == "int" &&
        root->getChild("term")->type == "int") {
      root->type = "int";
    } else if (root->getChild("expr")->type == "int*" &&
               root->getChild("term")->type == "int") {
      root->type = "int*";
    } else if (root->getChild("expr")->type == "int" &&
               root->getChild("term")->type == "int*") {
      root->type = "int*";
    } else {
      throw Err("ERROR5");
    }
  }
  if (root->rule == "expr expr MINUS term") {
    if (root->getChild("expr")->type == "int" &&
        root->getChild("term")->type == "int") {
      root->type = "int";
    } else if (root->getChild("expr")->type == "int*" &&
               root->getChild("term")->type == "int") {
      root->type = "int*";
    } else if (root->getChild("expr")->type == "int*" &&
               root->getChild("term")->type == "int*") {
      root->type = "int";
    } else {
      throw Err("ERROR6");
    }
  }
  if (root->rule == "term term STAR factor") {
    if (root->getChild("term")->type != "int" ||
        root->getChild("factor")->type != "int") throw Err("ERROR7");
    root->type = "int";
  }
  if (root->rule == "term term SLASH factor") {
    if (root->getChild("term")->type != "int" ||
        root->getChild("factor")->type != "int") throw Err("ERROR8");
    root->type = "int";
  }
  if (root->rule == "term term PCT factor") {
    if (root->getChild("term")->type != "int" ||
        root->getChild("factor")->type != "int") throw Err("ERROR9");
    root->type = "int";
  }
  if (root->rule == "factor AMP lvalue") {
    if (root->getChild("lvalue")->type != "int") throw Err("ERROR10");
    root->type = "int*";
  }
  if (root->rule == "factor STAR factor") {
    if (root->getChild("factor")->type != "int*") throw Err("ERROR11");
    root->type = "int";
  }
  if (root->rule == "factor NEW INT LBRACK expr RBRACK") {
    if (root->getChild("expr")->type != "int") throw Err("ERROR12");
    root->type = "int*";
  }
  if (root->rule == "lvalue STAR factor") {
    if (root->getChild("factor")->type != "int*") throw Err("ERROR13");
    root->type = "int";
  }
  if (root->rule == "lvalue LPAREN lvalue RPAREN") {
    root->type = root->getChild("lvalue")->type;
  }
  if (root->rule == "factor ID LPAREN RPAREN" || 
      root->rule == "factor ID LPAREN arglist RPAREN") {
    root->type = "int";
  }
}

bool wellTyped(SymTree *root) {
  // if given a "test" node, check if types match
  if (root->symbol == "test") {
    if (root->getChild("expr")->type == root->getChild("expr",2)->type) 
    return true;
    else return false;
  }

  // given a "statements" node
  if (root->prodRule == ".EMPTY") {
    return true;
  } else {
    // have statements -> statements statement
    // check if statement is well-typed and recurse on statements
    SymTree *stment = root->getChild("statement");
    if (stment->children[0]->symbol == "lvalue") {
      if (stment->getChild("lvalue")->type != stment->getChild("expr")->type) return false;
    }
    if (stment->children[0]->symbol == "IF") {
      if (!(wellTyped(stment->getChild("test")) &&
            wellTyped(stment->getChild("statements")) && 
            wellTyped(stment->getChild("statements",2)))) return false;
    }
    if (stment->children[0]->symbol == "WHILE") {
      if (!(wellTyped(stment->getChild("test")) &&
            wellTyped(stment->getChild("statements")))) return false;
    }
    if (stment->children[0]->symbol == "PRINTLN") {
      if (stment->getChild("expr")->type != "int") return false;
    }
    if (stment->children[0]->symbol == "DELETE") {
      if (stment->getChild("expr")->type != "int*") return false;
    }
    return wellTyped(root->getChild("statements"));
  }
}

bool isCorrect(SymTree *root, string curProcName = "wain") {
  if (root->symbol == "procedure") {
    curProcName = root->getChild("ID")->lexeme;
    if (root->getChild("expr")->type != "int") return false;
  }
  if (root->symbol == "main") {
    if (root->getChild("dcl")->getChild("ID")->lexeme == root->getChild("dcl",2)->getChild("ID")->lexeme) return false;
    if (root->getChild("dcl",2)->getChild("type")->prodRule != "INT") return false;
    if (root->getChild("expr")->type != "int") return false;
  }
  if (root->rule == "factor ID" || root->prodRule == "lvalue ID") {
    if (globalSymTable[curProcName].second.getVarType(root->getChild("ID")->lexeme) == "") return false;
  }
  if (root->symbol == "dcls" && root->children.size() != 0) {
    if (root->children[3]->symbol == "NUM") {
      if (root->getChild("dcl")->getChild("type")->prodRule != "INT") return false;
    } else {
      if (root->getChild("dcl")->getChild("type")->prodRule != "INT STAR") return false;
    }
  }
  // function calls
  if (root->rule == "factor ID LPAREN RPAREN") {
    // check that ID is in global symbol table
    string funcName = root->getChild("ID")->lexeme;
    if (globalSymTable.find(funcName) == globalSymTable.end()) return false;
    // if signature requires args, return false
    if (globalSymTable[funcName].first.getSig().size() != 0) return false;
  }
  if (root->rule == "factor ID LPAREN arglist RPAREN") {
    // same as before, but also check that arglist matches signature
    string funcName = root->getChild("ID")->lexeme;
    if (globalSymTable.find(funcName) == globalSymTable.end()) return false;
    if (checkArgs(root->getChild("arglist"), 
        globalSymTable[funcName].first) == false) return false;
  }
  // statements
  if (root->symbol == "statements") {
    if (!wellTyped(root)) return false;
  }
  // recurse on subtrees
  for (auto &subtree : root->children) {
    if (isCorrect(subtree, curProcName) == false) return false;
  }
  return true;
}

int main() {
  loadCFG();
  istringstream input = loadInput();
  SymTree *parseTree = buildFirstTree(input);

  try {
    buildSymTable(parseTree);
    annotateTypes(parseTree);
    if (isCorrect(parseTree)) {
      printSymTree(parseTree);
    } else {
      throw Err("ERROR14");
    }
  } catch (Err &e) {
    // cerr << e.msg() << endl;
    cerr << "ERROR" << endl;
  }
  deleteSymTrees(parseTree);
}