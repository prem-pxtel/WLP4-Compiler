#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <map>

using namespace std;

// WLP4 Context-Free Grammar
const string WLP4_CFG = R"END(.CFG
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
  string s, left, right, type;
  char c;
  raw >> left;
  raw >> noskipws >> c >> skipws;

  while (raw >> s) {
    if (s == ":") break;
    right += s + " ";
    
    raw >> noskipws >> c >> skipws;
    if (c == '\n') break;
  }
  right.pop_back();
  if (s == ":") raw >> type;

  SymTree *theTree = new SymTree;
  theTree->symbol = left;
  theTree->type = type;
  if (isInCFG(left, right)) {
    theTree->leaf = false;
    theTree->prodRule = right;
    theTree->rule = theTree->symbol + " " + theTree->prodRule;
    int RHS_word_count = wcount(right);
    if (right == ".EMPTY") --RHS_word_count;
    for (int i = 0; i < RHS_word_count; ++i) {
      theTree->children.push_back(buildFirstTree(raw));
    }
  } else {
    theTree->leaf = true;
    theTree->lexeme = right;
  }
  return theTree;
}

void printSymTree(SymTree *root) {
  cout << root->symbol + " ";
  if (root->leaf == true) {
    cout << root->lexeme;
  } else {
    cout << root->prodRule;
  }
  if (root->type != "") {
    cout << " : " + root->type;
  }
  cout << endl;
  for (auto &subtree : root->children) {
    printSymTree(subtree);
  }
}

void deleteSymTrees(SymTree *root) {
  for (auto &subtree : root->children) {
    deleteSymTrees(subtree);
  }
  delete root;
}

map<string, int> offsetTable = {};
int varCount = 0; // this is for non-parameter variables (wain and other procs)
// parameters are dealt with in the general cases for "main" and "procedure"
int ifCount = 0;
int whileCount = 0;
int delCount = 0;

void push(string reg) {
  cout << "sw $" + reg + ", -4($30)" << endl;
  cout << "sub $30, $30, $4" << endl; 
}
void pop(string reg) {
  cout << "add $30, $30, $4" << endl;
  cout << "lw $" + reg + ", -4($30)" << endl;
}
void lis(string reg, string word) {
  // lis $reg
  // .word ___
  cout << "lis $" + reg << endl;
  cout << ".word " + word << endl;
}
void loadVar(string reg, string offset) {
  // lw $reg, offset($29)
  cout << "lw $" + reg + ", " + offset + "($29)" << endl;
}
void jalr(string reg) {
  cout << "jalr $" + reg << endl;
}
void init(SymTree *root) {
  cout << ".import init" << endl;
  cout << ".import new" << endl;
  cout << ".import delete" << endl;
  cout << ".import print" << endl;
  lis("4", "4");
  lis("11", "1");
  push("2");
  push("31");
  // if 1st param in wain is int, set $2 = 0
  SymTree *procs = root->getChild("procedures");
  while (procs->prodRule == "procedure procedures") {
    procs = procs->getChild("procedures");
  }
  SymTree *wain = procs->getChild("main");
  if (wain->getChild("dcl")->getChild("type")->prodRule == "INT") {
    lis("2", "0");
  }
  lis("5", "init");
  jalr("5");
  pop("31");
  pop("2");
  cout << "sub $29, $30, $4" << endl;
}

void processParams(SymTree *params) { // doesnt gen code but updates offset table
  // calculate number of parameters
  int pCount = 0;
  vector<string> pStack;
  if (params->prodRule == "paramlist") {
    SymTree *paramlist = params->getChild("paramlist");
    while (paramlist->prodRule == "dcl COMMA paramlist") {
      ++pCount;
      pStack.push_back(paramlist->getChild("dcl")->getChild("ID")->lexeme);
      paramlist = paramlist->getChild("paramlist");
    }
    // have paramlist -> dcl
    ++pCount;
    pStack.push_back(paramlist->getChild("dcl")->getChild("ID")->lexeme);
  }
  // then update offset table
  for (int i = 1; i <= pCount; ++i) {
    offsetTable[pStack.at(i-1)] = 4 * (pCount - i + 1);
  }
}

void code(SymTree *root) {
  if (root->symbol == "procedures") {
    vector<SymTree *> procStack;
    SymTree *procs = root;
    while (procs->prodRule != "main") {
      procStack.push_back(procs->getChild("procedure"));
      procs = procs->getChild("procedures");
    }
    // now we have procedures -> main
    // print main FIRST, then procedure in order of appearance
    code(procs->getChild("main"));
    for (unsigned i = 0; i < procStack.size(); ++i) {
      code(procStack.at(i));
    }
    return;
  }
  if (root->symbol == "main") {
    // note: we already init'd in main
    string varName = root->getChild("dcl")->getChild("ID")->lexeme;
    offsetTable[varName] = varCount * -4;
    ++varCount;
    push("1");

    varName = root->getChild("dcl",2)->getChild("ID")->lexeme;
    offsetTable[varName] = varCount * -4;
    ++varCount;
    push("2");

    code(root->getChild("dcls"));
    code(root->getChild("statements"));
    code(root->getChild("expr"));
    cout << "jr $31" << endl;
    return;
  }
  if (root->symbol == "procedure") {
    cout << "F" + root->getChild("ID")->lexeme + ":" << endl;
    cout << "sub $29, $30, $4" << endl; // set the frame pointer

    processParams(root->getChild("params"));
    // get number of non-parameter local vars
    SymTree* dcls = root->getChild("dcls");
    int locVarCount = 0;
    while (dcls->rule != "dcls .EMPTY") {
      ++locVarCount;
      dcls = dcls->getChild("dcls");
    }

    varCount = 0; // first non-parameter starts at offset 0!
    code(root->getChild("dcls")); // push non-parameter local variables (same as wain)
    code(root->getChild("statements")); 
    code(root->getChild("expr"));

    // Generate code that pops non-parameter local variables
    for (int i = 0; i < locVarCount; ++i) {
      cout << "add $30, $30, $4" << endl;
    }
    cout << "jr $31" << endl;
    return;
  }
  if (root->rule == "expr term") {
    code(root->getChild("term"));
    return;
  }
  if (root->rule == "term factor") {
    code(root->getChild("factor"));
    return;
  }  
  if (root->rule == "factor LPAREN expr RPAREN") {
    code(root->getChild("expr"));
    return;
  }
  if (root->rule == "factor ID") {
    string id = root->getChild("ID")->lexeme;
    int offset = offsetTable.at(id);
    loadVar("3", to_string(offset));
    return;
  }
  if (root->rule == "factor NUM") {
    string num = root->getChild("NUM")->lexeme;
    lis("3", num);
    return;
  }
  // Q1++ function calls
  if (root->rule == "factor ID LPAREN RPAREN") {
    push("29");
    push("31");
    lis("5", "F" + root->getChild("ID")->lexeme);
    jalr("5");
    pop("31");
    pop("29");
    return;
  }
  if (root->rule == "factor ID LPAREN arglist RPAREN") {
    push("29");
    push("31");
    int numArgs = 0;
    SymTree* arglist = root->getChild("arglist");
    while (true) {
      // arglist -> expr COMMA arglist OR arglist -> expr
      ++numArgs;
      code(arglist->getChild("expr"));
      push("3");
      if (arglist->rule == "arglist expr") break;
      arglist = arglist->getChild("arglist");
    }
    lis("5", "F" + root->getChild("ID")->lexeme);
    jalr("5");
    for (int i = 0; i < numArgs; ++i) {
      // pop and discard each argument from the stack
      cout << "add $30, $30, $4" << endl;
    }
    pop("31");
    pop("29");
    return;
  }

  // Q2
  if (root->rule == "dcls dcls dcl BECOMES NUM SEMI") {
    code(root->getChild("dcls"));
    string varName = root->getChild("dcl")->getChild("ID")->lexeme;
    string num = root->getChild("NUM")->lexeme;
    offsetTable[varName] = varCount * -4;
    ++varCount;
    // store initial value (use $3) instead of $1 and $2, and push
    lis("3", num);
    push("3");
    return;
  }
  if (root->rule == "statement lvalue BECOMES expr SEMI") {
    SymTree* expr = root->getChild("expr");
    SymTree* lvalue = root->getChild("lvalue");
    while (lvalue->rule == "lvalue LPAREN lvalue RPAREN") {
      lvalue = lvalue->getChild("lvalue");
    }
    if (lvalue->rule == "lvalue ID") {
      // code for assignment to a variable
      code(root->getChild("expr"));
      int offset = offsetTable.at(lvalue->getChild("ID")->lexeme);
      cout << "sw $3, " + to_string(offset) + "($29)" << endl;
    } else if (lvalue->rule == "lvalue STAR factor") {
      // code for assignment to a dereferenced pointer (Q2++)
      code(expr);
      push("3");
      code(lvalue->getChild("factor"));
      pop("5");
      cout << "sw $5, 0($3)" << endl;
    }
    return;
  }
  // Q2++
  if (root->rule == "dcls dcls dcl BECOMES NULL SEMI") {
    code(root->getChild("dcls"));
    string varName = root->getChild("dcl")->getChild("ID")->lexeme;
    offsetTable[varName] = varCount * -4;
    ++varCount;
    // store initial value (use $3) instead of $1 and $2, and push
    lis("3", "1"); // 1 is the NULL constant
    push("3");
    return;
  }
  if (root->rule == "factor NULL") {
    lis("3", "1");
    return;
  }
  if (root->rule == "factor STAR factor") {
    code(root->getChild("factor"));
    cout << "lw $3, 0($3)" << endl;
    return;
  }
  if (root->rule == "factor AMP lvalue") {
    SymTree *lvalue = root->getChild("lvalue");
    if (lvalue->rule == "lvalue ID") {
      int offset = offsetTable.at(lvalue->getChild("ID")->lexeme);
      lis("3", to_string(offset));
      cout << "add $3, $29, $3" << endl;
    } else if (lvalue->rule == "lvalue STAR factor") {
      code(root->getChild("factor"));
    }
    return;
  }

  // Q3 / Q3++
  if (root->rule == "expr expr PLUS term") {
    if (root->getChild("expr")->type == "int" && root->getChild("term")->type == "int") {
      // int + int
      code(root->getChild("expr"));
      push("3");
      code(root->getChild("term"));
      pop("5");
      cout << "add $3, $5, $3" << endl;
    } else {
      if (root->getChild("expr")->type == "int*") {
        // int* + int
        code(root->getChild("expr"));
        push("3");
        code(root->getChild("term"));
        cout << "mult $3, $4" << endl;
        cout << "mflo $3" << endl;
        pop("5");
        cout << "add $3, $5, $3" << endl;
      } else {
        // int + int*
        code(root->getChild("term"));
        push("3");
        code(root->getChild("expr"));
        cout << "mult $3, $4" << endl;
        cout << "mflo $3" << endl;
        pop("5");
        cout << "add $3, $5, $3" << endl;
      }
    }
    return;
  }
  if (root->rule == "expr expr MINUS term") {
    if (root->getChild("expr")->type == "int" && root->getChild("term")->type == "int") {
      // int - int
      code(root->getChild("expr"));
      push("3");
      code(root->getChild("term"));
      pop("5");
      cout << "sub $3, $5, $3" << endl;
    } else if (root->getChild("expr")->type == "int*" && root->getChild("term")->type == "int") {
      // int* - int
      code(root->getChild("expr"));
      push("3");
      code(root->getChild("term"));
      cout << "mult $3, $4" << endl;
      cout << "mflo $3" << endl;
      pop("5");
      cout << "sub $3, $5, $3" << endl;
    } else {
      // int* - int*
      code(root->getChild("expr"));
      push("3");
      code(root->getChild("term"));
      pop("5");
      cout << "sub $3, $5, $3" << endl;
      cout << "div $3, $4" << endl;
      cout << "mflo $3" << endl;
    }
    return;
  }
  if (root->rule == "term term STAR factor") {
    code(root->getChild("term"));
    push("3");
    code(root->getChild("factor"));
    pop("5");
    cout << "mult $5, $3" << endl;
    cout << "mflo $3" << endl;
    return;
  }
  if (root->rule == "term term SLASH factor") {
    code(root->getChild("term"));
    push("3");
    code(root->getChild("factor"));
    pop("5");
    cout << "div $5, $3" << endl;
    cout << "mflo $3" << endl;
    return;
  }
  if (root->rule == "term term PCT factor") {
    code(root->getChild("term"));
    push("3");
    code(root->getChild("factor"));
    pop("5");
    cout << "div $5, $3" << endl;
    cout << "mfhi $3" << endl;
    return;
  }

  // Q4 / Q4++
  if (root->symbol == "test" && root->children[1]->symbol == "NE") {
    code(root->getChild("expr"));
    push("3");
    code(root->getChild("expr",2));
    pop("5");
    if (root->getChild("expr")->type == "int") {
      cout << "slt $6, $3, $5" << endl;
      cout << "slt $7, $5, $3" << endl;
    } else {
      cout << "sltu $6, $3, $5" << endl;
      cout << "sltu $7, $5, $3" << endl;
    }
    cout << "add $3, $6, $7" << endl;
    return;
  }
  if (root->symbol == "test" && root->children[1]->symbol == "EQ") {
    code(root->getChild("expr"));
    push("3");
    code(root->getChild("expr",2));
    pop("5");
    if (root->getChild("expr")->type == "int") {
      cout << "slt $6, $3, $5" << endl;
      cout << "slt $7, $5, $3" << endl;
    } else {
      cout << "sltu $6, $3, $5" << endl;
      cout << "sltu $7, $5, $3" << endl;
    }
    cout << "add $3, $6, $7" << endl;
    // invert:
    cout << "sub $3, $11, $3" << endl;
    return;
  }
  if (root->symbol == "test" && root->children[1]->symbol == "LT") {
    code(root->getChild("expr"));
    push("3");
    code(root->getChild("expr",2));
    pop("5");
    if (root->getChild("expr")->type == "int") {
      cout << "slt $3, $5, $3" << endl;
    } else {
      cout << "sltu $3, $5, $3" << endl;
    }
    return;
  }
  if (root->symbol == "test" && root->children[1]->symbol == "GE") {
    code(root->getChild("expr"));
    push("3");
    code(root->getChild("expr",2));
    pop("5");
    if (root->getChild("expr")->type == "int") {
      cout << "slt $3, $5, $3" << endl;
    } else {
      cout << "sltu $3, $5, $3" << endl;
    }
    // invert:
    cout << "sub $3, $11, $3" << endl;
    return;
  }
  if (root->symbol == "test" && root->children[1]->symbol == "GT") {
    code(root->getChild("expr"));
    push("3");
    code(root->getChild("expr",2));
    pop("5");
    if (root->getChild("expr")->type == "int") {
      cout << "slt $3, $3, $5" << endl;
    } else {
      cout << "sltu $3, $3, $5" << endl;
    }
    return;
  }
  if (root->symbol == "test" && root->children[1]->symbol == "LE") {
    code(root->getChild("expr"));
    push("3");
    code(root->getChild("expr",2));
    pop("5");
    if (root->getChild("expr")->type == "int") {
      cout << "slt $3, $3, $5" << endl;
    } else {
      cout << "sltu $3, $3, $5" << endl;
    }
    // invert:
    cout << "sub $3, $11, $3" << endl;
    return;
  }
  if (root->symbol == "statement" && root->children[0]->symbol == "IF") {
    ++ifCount;
    string strCount = to_string(ifCount);

    code(root->getChild("test"));
    cout << "beq $3, $0, else" + strCount << endl;
    code(root->getChild("statements"));
    cout << "beq $0, $0, endif" + strCount << endl;
    cout << "else" + strCount + ":" << endl;
    code(root->getChild("statements",2));
    cout << "endif" + strCount + ":" << endl;
    return;
  }
  if (root->symbol == "statement" && root->children[0]->symbol == "WHILE") {
    ++whileCount;
    string strCount = to_string(whileCount);

    cout << "loop" + strCount + ":" << endl;
    code(root->getChild("test"));
    cout << "beq $3, $0, endWhile" + strCount << endl;
    code(root->getChild("statements"));
    cout << "beq $0, $0, loop" + strCount << endl;
    cout << "endWhile" + strCount + ":" << endl;
    return;
  }

  // Q5
  if (root->rule == "statement PRINTLN LPAREN expr RPAREN SEMI") {
    code(root->getChild("expr"));
    cout << "add $1, $3, $0" << endl;
    push("31");
    lis("5", "print");
    jalr("5");
    pop("31");
    return;
  }
  // Q5++
  if (root->rule == "factor NEW INT LBRACK expr RBRACK") {
    code(root->getChild("expr"));
    cout << "add $1, $3, $0" << endl; // place the size value in the parameter register
    push("31");
    lis("5", "new");
    jalr("5");
    pop("31");
    cout << "bne $3, $0, 1" << endl; // skip the next instruction if "new" was successful
    cout << "add $3, $11, $0" << endl; // set $3 = NULL if "new" failed, assuming $11 contains 1
    return;
  }
  if (root->rule == "statement DELETE LBRACK RBRACK expr SEMI") {
    ++delCount;
    string strCount = to_string(delCount);
    code(root->getChild("expr"));
    cout << "beq $3, $11, skipDelete" + strCount << endl; // skip if $3 = NULL
    cout << "add $1, $3, $0" << endl; // place the address to delete in the parameter register
    push("31");
    lis("5", "delete");
    jalr("5");
    pop("31");
    cout << "skipDelete" + strCount + ":" << endl;
    return;
  }

  // recurse on subtrees
  for (auto &subtree : root->children) {
    code(subtree);
  }
}

int main() {
  loadCFG();
  istringstream input = loadInput();
  SymTree *pt = buildFirstTree(input);
  init(pt);
  code(pt);  
  deleteSymTrees(pt);
}