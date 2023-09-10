#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <cctype>
#include <vector>

using namespace std;

class State {
  public:
    string name;
    bool accept;
    // map<input char or word, stateName>
    map<char, string> stateLookup = {};
};

// Returns the starting state of the DFA
State start(map<string, State> &states) { 
	return states.at("start"); 
}

void createState(string stateName, map<string, State> &states, bool acc = true) {
	State newState = State();
	newState.accept = acc;
	newState.name = stateName;
	states[stateName] = newState;
}

/* Returns the state corresponding to following a transition
	* from the given starting state on the given character,
	* or a special fail state if the transition does not exist.
*/
State &transition(State &curState, char next, map<string, State> &states) {
	// first check if its one of the keywords
	return states.at(curState.stateLookup[next]);
}

// Register a transition on all chars in chars
void registerTransition(State &oldState, char chars, string newStateName) {
	oldState.stateLookup[chars] = newStateName;
}

// Register a transition on all chars matching isType
// For some reason the cctype functions all use ints, hence the function
// argument type.
void registerTransition(State &oldState, int (*isType)(int), string newStateName) {
	for (int c = 0; c < 128; ++c) {
		if (isType(c)) {
			oldState.stateLookup[c] = newStateName;
		}
	}
}

bool notWS(string stateName) {
	if (stateName == "COMMENT" ||
		stateName == "SPACE" ||
		stateName == "TAB" ||
		stateName == "NEWLINE") {
			return false;
	}
	return true;
}

void outputTok(string type, string lex) {
	if (type == "ID") {
		if (lex == "int") {
			type = "INT";
		} else if (lex == "return") {
			type = "RETURN";
		} else if (lex == "if") {
			type = "IF";
		} else if (lex == "else") {
			type = "ELSE";
		} else if (lex == "while") {
			type = "WHILE";
		} else if (lex == "println") {
			type = "PRINTLN";
		} else if (lex == "wain") {
			type = "WAIN";
		} else if (lex == "new") {
			type = "NEW";
		} else if (lex == "delete") {
			type = "DELETE";
		} else if (lex == "NULL") {
			type = "NULL";
		}
	}
	if (type == "NUM") {
		try {
			stoi(lex);
		} catch (const out_of_range& ex) {
			// Handle the case when the value exceeds INT_MAX
			cerr << "ERROR" << endl;
			return;
		}
	}
	if (notWS(type)) {
		cout << type << " " << lex << endl;
	}
}

int main() {
	map<string, State> states;

	// Define States
	vector<string> acptStates = {"ID", "NUM", "LPAREN", "RPAREN", "LBRACE", "RBRACE", "BECOMES", "EQ", "NE", "LT", "GT", "LE", "GE", "PLUS", "MINUS", "STAR", "SLASH", "PCT", "COMMA", "SEMI", "LBRACK", "RBRACK", "AMP", "SPACE", "TAB", "NEWLINE", "COMMENT"};

	vector<string> nonAcptStates = {"start", "exclam"};

	for (vector<string>::iterator it = acptStates.begin(); it != acptStates.end(); ++it) {
		createState(*it, states);
	}
	for (vector<string>::iterator it = nonAcptStates.begin(); it != nonAcptStates.end(); ++it) {
		createState(*it, states, false);
	}

	// Define Transitions
	registerTransition(states["start"], isalpha, "ID");
	registerTransition(states["ID"], isalnum, "ID");
	registerTransition(states["start"], isdigit, "NUM");
	registerTransition(states["NUM"], isdigit, "NUM");
	registerTransition(states["start"], '(', "LPAREN");
	registerTransition(states["start"], ')', "RPAREN");
	registerTransition(states["start"], '{', "LBRACE");
	registerTransition(states["start"], '}', "RBRACE");
	registerTransition(states["start"], '=', "BECOMES");
	registerTransition(states["BECOMES"], '=', "EQ");
	registerTransition(states["start"], '!', "exclam");
	registerTransition(states["exclam"], '=', "NE");
	registerTransition(states["start"], '<', "LT");
	registerTransition(states["start"], '>', "GT");
	registerTransition(states["LT"], '=', "LE");
	registerTransition(states["GT"], '=', "GE");
	registerTransition(states["start"], '+', "PLUS");
	registerTransition(states["start"], '-', "MINUS");
	registerTransition(states["start"], '*', "STAR");
	registerTransition(states["start"], '/', "SLASH");
	registerTransition(states["SLASH"], '/', "COMMENT");
	registerTransition(states["start"], '%', "PCT");
	registerTransition(states["start"], ',', "COMMA");
	registerTransition(states["start"], ';', "SEMI");
	registerTransition(states["start"], '[', "LBRACK");
	registerTransition(states["start"], ']', "RBRACK");
	registerTransition(states["start"], '&', "AMP");
	registerTransition(states["start"], isspace, "SPACE");
	registerTransition(states["start"], [](int c) -> int { return c == 9; }, "TAB");
	registerTransition(states["start"], [](int c) -> int { return c == 10; }, "NEWLINE");
	registerTransition(states["COMMENT"], [](int c) -> int { return c != '\n'; }, "COMMENT");
	registerTransition(states["COMMENT"], [](int c) -> int { return c == '\n'; }, "start");

	// Input Section
	istringstream sock;
	char c;
	cin >> noskipws;
	while (cin >> c) {
		string charToStr = string(1, c);
		sock.str(sock.str() + charToStr);
	}

	if (sock.str() == "") return 0;

	State curState = start(states);
	char curChar;
	string curToken;
	int EMPTY = -1;
	sock >> noskipws;

	while (sock.peek() != EMPTY) {
		if (curState.stateLookup.find(sock.peek()) == curState.stateLookup.end()) {
			if (curState.accept == true) {
				// output token
				outputTok(curState.name, curToken);
				// go back to initial state
				curState = start(states);
				curToken = "";
			} else {
				// reject
				break;
			}
		} else {
			sock >> curChar;
			curToken.push_back(curChar);
			curState = transition(curState, curChar, states);
		}
	}
	if (curState.accept == true) {
		// output token and accept
		outputTok(curState.name, curToken);
	} else {
		// reject
		cerr << "ERROR" << endl;
	}
}