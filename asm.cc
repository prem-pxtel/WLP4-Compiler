#include <iostream>
#include <string>
#include <vector>
#include <map>
#include "scanner.h"

struct Instruction {
  std::string name;
  std::string labelName = "";
  int curPC;
  int64_t opcode;
  int64_t operand1;
  int64_t operand2;
  int64_t operand3;
  int64_t instr;

  Instruction(int curPC) {
    this->curPC = curPC;
  }
};

bool isValidRange(int64_t num, Token::Kind kind) {
  // .word range checking
  if (kind == Token::Kind::WORD) {
    if (num < -2147483648) return false;
    if (num > 4294967295) return false;
  }
  // register range checking
  if (kind == Token::Kind::REG) {
    if (num < 0) return false;
    if (num > 31) return false;
  }
  // immediate range checking; labels fall into INT range check
  if (kind == Token::Kind::INT) {
    if (num < -32768) return false;
    if (num > 32767) return false;
  }
  if (kind == Token::Kind::HEXINT) {
    if (num < 0) return false;
    if (num > 65535) return false;
  }
  return true;
}

int main() {
  std::vector<Instruction> instructions;
  std::map<std::string, int> symbolTable = {};
  int PC = 0;
  std::string line;

  try {
    while (getline(std::cin, line)) {
      std::vector<Token> tokenLine = scan(line);
      // first pass: check syntax + range, create instuctions,
      // and generate symbolTable
      for (unsigned i = 0; i < tokenLine.size(); ++i) {
        Token tok = tokenLine[i];
        Token::Kind tokKind = tok.getKind();
        std::string tokLexeme = tok.getLexeme();

        if (tokKind == Token::Kind::LABEL) {
          std::string labelName = tokLexeme.substr(0, tokLexeme.size()-1);
          if (symbolTable.find(labelName) != symbolTable.end()) {
            throw ScanningFailure("ERROR");
          }
          symbolTable[labelName] = PC;
          continue;
        } else if (tokKind == Token::Kind::WORD) {
          if (tokenLine[i+1].getKind() == Token::Kind::INT ||
              tokenLine[i+1].getKind() == Token::Kind::HEXINT ||
              tokenLine[i+1].getKind() == Token::Kind::ID) {
            Instruction in = Instruction(PC);
            in.name = "word";

            if (tokenLine[i+1].getKind() == Token::Kind::ID) {
              in.labelName = tokenLine[i+1].getLexeme();
            } else {
              if (isValidRange(tokenLine[i+1].toNumber(), 
                               Token::Kind::WORD) == false) {
                throw ScanningFailure("ERROR");
              }
              in.instr = tokenLine[i+1].toNumber();
            }

            instructions.push_back(in);
            ++i;
            PC += 4;
            continue;
          }
          throw ScanningFailure("ERROR");
        } else if (tokKind == Token::Kind::ID) {
          if (tokLexeme == "add" || tokLexeme == "sub" ||
              tokLexeme == "slt" || tokLexeme == "sltu") {
            if (tokenLine[i+1].getKind() == Token::Kind::REG &&
                tokenLine[i+2].getKind() == Token::Kind::COMMA &&
                tokenLine[i+3].getKind() == Token::Kind::REG &&
                tokenLine[i+4].getKind() == Token::Kind::COMMA &&
                tokenLine[i+5].getKind() == Token::Kind::REG) {
              if (isValidRange(tokenLine[i+1].toNumber(), 
                    tokenLine[i+1].getKind()) == false) {
                throw ScanningFailure("ERROR");
              }
              if (isValidRange(tokenLine[i+3].toNumber(), 
                    tokenLine[i+3].getKind()) == false) {
                throw ScanningFailure("ERROR");
              }
              if (isValidRange(tokenLine[i+5].toNumber(), 
                    tokenLine[i+5].getKind()) == false) {
                throw ScanningFailure("ERROR");
              }
              Instruction in = Instruction(PC);
              in.name = tokLexeme;
              in.operand1 = tokenLine[i+1].toNumber();
              in.operand2 = tokenLine[i+3].toNumber();
              in.operand3 = tokenLine[i+5].toNumber();
              instructions.push_back(in);
              i += 5;
              PC += 4;
              continue;
            }
            throw ScanningFailure("ERROR");
          } else if (tokLexeme == "beq" || tokLexeme == "bne") {
            if ((tokenLine[i+1].getKind() == Token::Kind::REG) &&
                (tokenLine[i+2].getKind() == Token::Kind::COMMA) &&
                (tokenLine[i+3].getKind() == Token::Kind::REG) &&
                (tokenLine[i+4].getKind() == Token::Kind::COMMA) &&
                ((tokenLine[i+5].getKind() == Token::Kind::INT) ||
                (tokenLine[i+5].getKind() == Token::Kind::HEXINT))) {
              if (isValidRange(tokenLine[i+1].toNumber(), 
                    tokenLine[i+1].getKind()) == false) {
                throw ScanningFailure("ERROR");
              }
              if (isValidRange(tokenLine[i+3].toNumber(), 
                    tokenLine[i+3].getKind()) == false) {
                throw ScanningFailure("ERROR");
              }
              if (isValidRange(tokenLine[i+5].toNumber(), 
                    tokenLine[i+5].getKind()) == false) {
                throw ScanningFailure("ERROR");
              }
              Instruction in = Instruction(PC);
              in.name = tokLexeme;
              in.operand1 = tokenLine[i+1].toNumber();
              in.operand2 = tokenLine[i+3].toNumber();
              in.operand3 = tokenLine[i+5].toNumber();
              instructions.push_back(in);
              i += 5;
              PC += 4;
              continue;
            } else if (tokenLine[i+1].getKind() == Token::Kind::REG &&
                tokenLine[i+2].getKind() == Token::Kind::COMMA &&
                tokenLine[i+3].getKind() == Token::Kind::REG &&
                tokenLine[i+4].getKind() == Token::Kind::COMMA &&
                tokenLine[i+5].getKind() == Token::Kind::ID) {
              // last argument is label
              if (isValidRange(tokenLine[i+1].toNumber(), 
                  tokenLine[i+1].getKind()) == false) {
                throw ScanningFailure("ERROR");
              }
              if (isValidRange(tokenLine[i+3].toNumber(), 
                    tokenLine[i+3].getKind()) == false) {
                throw ScanningFailure("ERROR");
              }
              Instruction in = Instruction(PC);
              in.name = tokLexeme;
              in.operand1 = tokenLine[i+1].toNumber();
              in.operand2 = tokenLine[i+3].toNumber();
              in.labelName = tokenLine[i+5].getLexeme();
              instructions.push_back(in);
              i += 5;
              PC += 4;
              continue;
            }
            throw ScanningFailure("ERROR");
          } else if (tokLexeme == "mult" || tokLexeme == "multu" ||
                     tokLexeme == "div" || tokLexeme == "divu") {
            if (tokenLine[i+1].getKind() == Token::Kind::REG &&
              tokenLine[i+2].getKind() == Token::Kind::COMMA &&
              tokenLine[i+3].getKind() == Token::Kind::REG) {
              if (isValidRange(tokenLine[i+1].toNumber(), 
                  tokenLine[i+1].getKind()) == false) {
                throw ScanningFailure("ERROR");
              }
              if (isValidRange(tokenLine[i+3].toNumber(), 
                    tokenLine[i+3].getKind()) == false) {
                throw ScanningFailure("ERROR");
              }
              Instruction in = Instruction(PC);
              in.name = tokLexeme;
              in.operand1 = tokenLine[i+1].toNumber();
              in.operand2 = tokenLine[i+3].toNumber();
              instructions.push_back(in);
              i += 3;
              PC += 4;
              continue;
            }
            throw ScanningFailure("ERROR");
          } else if (tokLexeme == "mfhi" || tokLexeme == "mflo" ||
                     tokLexeme == "jr" || tokLexeme == "jalr" ||
                     tokLexeme == "lis") {
            if (tokenLine[i+1].getKind() == Token::Kind::REG) {
              if (isValidRange(tokenLine[i+1].toNumber(), 
                  tokenLine[i+1].getKind()) == false) {
                throw ScanningFailure("ERROR");
              }
              Instruction in = Instruction(PC);
              in.name = tokLexeme;
              in.operand1 = tokenLine[i+1].toNumber();
              instructions.push_back(in);
              i += 1;
              PC += 4;
              continue;
            }
            throw ScanningFailure("ERROR");
          } else if (tokLexeme == "lw" || tokLexeme == "sw") {
            if (tokenLine[i+1].getKind() == Token::Kind::REG &&
                tokenLine[i+2].getKind() == Token::Kind::COMMA &&
                (tokenLine[i+3].getKind() == Token::Kind::INT ||
                 tokenLine[i+3].getKind() == Token::Kind::HEXINT) &&
                tokenLine[i+4].getKind() == Token::Kind::LPAREN &&
                tokenLine[i+5].getKind() == Token::Kind::REG &&
                tokenLine[i+6].getKind() == Token::Kind::RPAREN) {
              if (isValidRange(tokenLine[i+1].toNumber(), 
                    tokenLine[i+1].getKind()) == false) {
                throw ScanningFailure("ERROR");
              }
              if (isValidRange(tokenLine[i+3].toNumber(), 
                    tokenLine[i+3].getKind()) == false) {
                throw ScanningFailure("ERROR");
              }
              if (isValidRange(tokenLine[i+5].toNumber(), 
                    tokenLine[i+5].getKind()) == false) {
                throw ScanningFailure("ERROR");
              }
              Instruction in = Instruction(PC);
              in.name = tokLexeme;
              in.operand1 = tokenLine[i+1].toNumber();
              in.operand2 = tokenLine[i+3].toNumber();
              in.operand3 = tokenLine[i+5].toNumber();
              instructions.push_back(in);
              i += 6;
              PC += 4;
              continue;
            }
            throw ScanningFailure("ERROR");
          } else {
            throw ScanningFailure("ERROR"); 
          }
        } else {
          throw ScanningFailure("ERROR");
        }
      }
    }
  } catch (ScanningFailure &f) {
    std::cerr << f.what() << std::endl;
    return 1;
  }

  // second pass: create binary instructions, making use of symbolTable
  // also checks range for offset, where applicable
  try {
    for (unsigned i = 0; i < instructions.size(); ++i) {
      if (instructions[i].name == "word" && instructions[i].labelName != "") {
        // e.g. .word label
        if (symbolTable.find(instructions[i].labelName) == symbolTable.end()) {
          throw ScanningFailure("ERROR"); 
        }
        if (isValidRange(symbolTable[instructions[i].labelName],
                         Token::Kind::WORD) == false) {
          throw ScanningFailure("ERROR"); 
        }
        instructions[i].instr = symbolTable[instructions[i].labelName];
      }
      if (instructions[i].name == "add" || instructions[i].name == "sub" ||
          instructions[i].name == "slt" || instructions[i].name == "sltu") {
        if (instructions[i].name == "add") instructions[i].opcode = 32;
        if (instructions[i].name == "sub") instructions[i].opcode = 34;
        if (instructions[i].name == "slt") instructions[i].opcode = 42;
        if (instructions[i].name == "sltu") instructions[i].opcode = 43;
        instructions[i].instr = (0 << 26) |
                                (instructions[i].operand2 << 21) |
                                (instructions[i].operand3 << 16) |
                                (instructions[i].operand1 << 11) |
                                instructions[i].opcode;                         
      }
      if (instructions[i].name == "beq" || instructions[i].name == "bne") {
        if (instructions[i].labelName != "") {
          // calculate difference betweem PC and label address for offset
          if (symbolTable.find(instructions[i].labelName) == symbolTable.end()) {
            throw ScanningFailure("ERROR"); 
          }
          int labelAddress = symbolTable[instructions[i].labelName];
          instructions[i].operand3 = (labelAddress-instructions[i].curPC-4) / 4;
          if (isValidRange(instructions[i].operand3, Token::Kind::INT) == false) {
            throw ScanningFailure("ERROR"); 
          }
        }
        if (instructions[i].name == "beq") instructions[i].opcode = 4;
        if (instructions[i].name == "bne") instructions[i].opcode = 5;
        // apply mask to offset; gets rid of leading 1s
        int64_t offset = (instructions[i].operand3) & 0xffff;
        instructions[i].instr = (instructions[i].opcode << 26) |
                                (instructions[i].operand1 << 21) |
                                (instructions[i].operand2 << 16) |
                                offset;
      }
      if (instructions[i].name == "mfhi" || instructions[i].name == "mflo" ||
          instructions[i].name == "lis") {
        if (instructions[i].name == "mfhi") instructions[i].opcode = 16;
        if (instructions[i].name == "mflo") instructions[i].opcode = 18;
        if (instructions[i].name == "lis") instructions[i].opcode = 20;
        instructions[i].instr = (0 << 26) |
                                (instructions[i].operand1 << 11) |
                                instructions[i].opcode;
      }
      if (instructions[i].name == "jr" || instructions[i].name == "jalr") {
        if (instructions[i].name == "jr") instructions[i].opcode = 8;
        if (instructions[i].name == "jalr") instructions[i].opcode = 9;
        instructions[i].instr = (0 << 26) |
                                (instructions[i].operand1 << 21) |
                                instructions[i].opcode;  
      }
      if (instructions[i].name == "lw" || instructions[i].name == "sw") {
        if (instructions[i].name == "lw") instructions[i].opcode = 35;
        if (instructions[i].name == "sw") instructions[i].opcode = 43;
        // apply mask to offset; gets rid of leading 1s
        int64_t offset = (instructions[i].operand2) & 0xffff;
        instructions[i].instr = (instructions[i].opcode << 26) |
                                (instructions[i].operand3 << 21) |
                                (instructions[i].operand1 << 16) |
                                offset;
      }
      if (instructions[i].name == "mult" || instructions[i].name == "multu" ||
          instructions[i].name == "div" || instructions[i].name == "divu") {
        if (instructions[i].name == "mult") instructions[i].opcode = 24;
        if (instructions[i].name == "multu") instructions[i].opcode = 25;
        if (instructions[i].name == "div") instructions[i].opcode = 26;
        if (instructions[i].name == "divu") instructions[i].opcode = 27;
        instructions[i].instr = (0 << 26) |
                                (instructions[i].operand1 << 21) |
                                (instructions[i].operand2 << 16) |
                                instructions[i].opcode;                         
      }
    }
  } catch (ScanningFailure &f) {
    std::cerr << f.what() << std::endl;
    return 1;
  }

  // print bytes to stdout, one byte at a time (using char)
  for (unsigned i = 0; i < instructions.size(); ++i) {
    int64_t curIn = instructions[i].instr;
    unsigned char c = curIn >> 24; std::cout << c;
    c = curIn >> 16; std::cout << c;
    c = curIn >> 8; std::cout << c;
    c = curIn >> 0; std::cout << c;
  }

  return 0;
}
