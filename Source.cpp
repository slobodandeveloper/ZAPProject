#include "stdio.h"
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>

#pragma pack(1)

using namespace std;

int ProgBase = 0x10000;					//High memory base
int DataBase = 0x4000;						//Dynamic and static memory
int firstAssemble = 0;					//first pass or second pass assembly
////////////////////////////////////////////////////////////
// Declare structures
/////////////////////////////////////////////////////////////
typedef int(*ProcessFuncPtr)(string args, string type, int opcode, string flags, char*result, bool bBranchType);

typedef struct INSTRUCTION_INFO {
	string instname;
	string insttype;
	int opcode;
	string resulttype;
	ProcessFuncPtr func;
} STINSTRUCTION, *LPSTINSTRUCTION;

typedef struct ZHEADER {
	char ver;				//0
	short Flags;			//1
	char v2;				//3
	short highMemBase;		//4
	short entry;			//6
	short loc_dictionary;	//8

	short loc_objtable;		//A	
	short loc_global;		//C
	short staticMemBase;	//E
	char v10;				//10 Flag1
	char v11;
	char v12;
	char v13;

	char v14;
	char v15;
	char v16;
	char v17;
	short loc_abb;			//18
	short filelen;			//1A
	short checksum;			//1C

	char internum;			//1E
	char interver;			//1F
	char scrheight;			//20
	char scrwidth;			//21
	short scrhinunit;		//22
	short scrwinunits;		//24
	char fontwinunit;		//26
	char fonthinunit;		//27

	short routineoffset;	//28
	short staticstringoffset;	//2A
	char defbackcolor;			//2C
	char defforecolor;			//2D
	short addr_termchar_table;	//2E
	short totalwidth;			//30

	short std_rev_num;			//32
	short alptable;				//34
	short hexextaddr;			//36
	short v38;					//38
	short v3A;					//3A

	short v3C;
	short v3E;
} stZHEADER, *LPZHEADER;

typedef struct ZOBJECT{
	string objname;
	short flag1;		//Object flags for 48bit
	short flag2;
	short flag3;
	short LOC;			//Parent Position
	short NEXT;			//Siblings
	short FIRST;		//First sibling
	short proptable;
	int offset;
} stZObject;

struct TABLEITEM{
	string type;
	vector<short> data;
	int offset;
};
struct TABLE {
	string name;
	int maxsize;
	vector<TABLEITEM> items;
	int offset;
};
struct GLOBALS {
	string datatype;
	string name;
	vector<unsigned short> value;
	int type;
	int offset;
};
struct GLOBALVARIABLE{
	string name;
	vector<unsigned short> value;
	int offset;
};
struct LOCALVARIABLE {
	string name;
	char localnum;
};
#define OPCODE_COUNT	112
#define DELIMITER	" "
#define COMMENTCHAR	';'
#define ALIGNMENT	4
#define TYPE_CODE	1
#define TYPE_DATA	0
#define MAXCHARS	65536

vector<char> assembled;					//assembled code
vector<char> databytes;					//assembled data bytes
vector<string> gInstructions;			//string by lines;
vector<STINSTRUCTION> gOpcodes;			//opcode table
vector<GLOBALS> gGlobalLabels;
vector<TABLE> gTables;
vector<ZOBJECT> gObjects;
vector<GLOBALVARIABLE> gVariables;
vector<LOCALVARIABLE> gLocalVariables;
int zversion = 6;


STINSTRUCTION codeTable_YZIP[255] = {
	{ "catch", "0op", 185, "result" },
	{ "crlf", "0op", 187, "" },
	{ "noop", "0op", 180, "" },
	{ "original", "0op", 191, "branch" },
	{ "rstack", "0op", 184, "" },
	{ "rfalse", "0op", 177, "" },
	{ "rtrue", "0op", 176, "" },
	{ "printr", "0op", 179, "" },
	{ "quit", "0op", 186, "" },
	{ "restart", "0op", 183, "" },
	{ "usl", "0op", 188, "" },
	{ "verify", "0op", 189, "branch" },
	{ "add", "2op", 20, "result" },
	{ "band", "2op", 9, "result" },
	{ "ashift", "ext", 3, "result" },
	{ "bufout", "var", 242, "" },
	{ "icall1", "1op", 143, "" },
	{ "call1", "1op", 136, "result" },
	{ "icall2", "2op", 26, "" },
	{ "call2", "2op", 25, "result" },
	{ "icall", "var", 249, "" },
	{ "ixcall", "var", 250, "" },
	{ "call", "var", 224, "result" },
	{ "assigned?", "var", 255, "branch" },
	{ "check_unicode", "ext", 12, "result" },
	{ "fclear", "2op", 12, "" },
	{ "copyt", "var", 253, "" },
	{ "dec", "1op", 134, "" },
	{ "dless?", "2op", 4, "branch" },
	{ "div", "2op", 23, "result" },
	{ "display", "ext", 5, "" },
	{ "zwstr", "var", 252, "" },
	{ "erase", "var", 238, "" },
	{ "dclear", "ext", 7, "" },
	{ "clear", "var", 237, "" },
	{ "first?", "1op", 130, "result.branch" },
	{ "curget", "var", 240, "" },
	{ "nextp", "2op", 19, "result" },
	{ "loc", "1op", 131, "result" },
	{ "getp", "2op", 17, "result" },
	{ "getpt", "2op", 18, "result" },
	{ "ptsize", "1op", 132, "result" },
	{ "next?", "1op", 129, "result.branch" },
	{ "winget", "ext", 19, "result" },
	{ "inc", "1op", 133, "" },
	{ "igrtr?", "2op", 5, "branch" },
	{ "dirin", "var", 244, "" },
	{ "move", "2op", 14, "" },
	{ "equal?", "2op", 1, "branch" },
	{ "equal?", "ext", 193, "branch" },
	{ "grtr?", "2op", 3, "branch" },
	{ "in?", "2op", 6, "branch" },
	{ "less?", "2op", 2, "branch" },
	{ "zero?", "1op", 128, "branch" },
	{ "value", "1op", 142, "result" },
	{ "bcom", "1op", 248, "result" },
	{ "getb", "2op", 16, "result" },
	{ "get", "2op", 15, "result" },
	{ "shift", "ext", 2, "result" },
	{ "menu", "ext", 27, "branch" },
	{ "mod", "2op", 24, "result" },
	{ "mouse-limit", "ext", 23, "" },
	{ "winpos", "ext", 16, "" },
	{ "mul", "2op", 22, "result" },
	{ "not", "var", 248, "result" },
	{ "bor", "2op", 8, "result" },
	{ "dirout", "var", 243, "" },
	{ "picinf", "ext", 6, "branch" },
	{ "picture_table", "ext", 28, "" },
	{ "fstack", "ext", 21, "" },
	{ "printb", "1op", 135, "" },
	{ "printc", "var", 229, "" },
	{ "printf", "ext", 26, "" },
	{ "printn", "var", 230, "" },
	{ "printd", "1op", 138, "" },
	{ "print", "1op", 141, "" },
	{ "printt", "var", 254, "" },
	{ "print_unicode", "ext", 11, "" },
	{ "pop", "var", 233, "" },
	{ "push", "var", 232, "" },
	{ "xpush", "ext", 24, "branch" },
	{ "putp", "ext", 227, "" },
	{ "winput", "ext", 25, "" },
	{ "random", "var", 231, "result" },
	{ "read", "var", 228, "result" },
	{ "input", "var", 246, "result" },
	{ "mouse-info", "ext", 22, "" },
	{ "remove", "1op", 137, "" },
	{ "restore", "ext", 1, "result" },
	{ "irestore", "ext", 10, "result" },
	{ "return", "1op", 139, "" },
	{ "save", "ext", 0, "result" },
	{ "isave", "ext", 9, "result" },
	{ "intbl?", "var", 247, "result.branch" },
	{ "scroll", "ext", 20, "" },
	{ "fset", "2op", 11, "" },
	{ "color", "2op", 27, "" },
	{ "curset", "var", 239, "" },
	{ "font", "ext", 4, "result" },
	{ "margin", "ext", 8, "" },
	{ "hlight", "var", 241, "" },
	{ "window", "var", 235, "" },
	{ "sound", "var", 245, "" },
	{ "split", "var", 234, "" },
	{ "set", "2op", 13, "" },
	{ "putb", "var", 226, "" },
	{ "put", "var", 225, "" },
	{ "sub", "2op", 21, "result" },
	{ "btst", "2op", 7, "branch" },
	{ "fset?", "2op", 10, "branch" },
	{ "throw", "2op", 28, "" },
	{ "lex", "var", 251, "" },
	{ "winsize", "ext", 17, "" },
	{ "winattr", "ext", 18, "" }
};

STINSTRUCTION codeTable_XZIP[255] = {
	{ "catch", "0op", 185, "result" },
	{ "new_line", "0op", 187, "" },
	{ "nop", "0op", 180, "" },
	{ "piracy", "0op", 191, "branch" },
	{ "pop", "0op", 185, "" },
	{ "ret_popped", "0op", 184, "" },
	{ "rfalse", "0op", 177, "" },
	{ "rtrue", "0op", 176, "" },
	{ "print_ret", "0op", 179, "" },
	{ "quit", "0op", 186, "" },
	{ "restart", "0op", 183, "" },
	{ "show_status", "0op", 188, "" },
	{ "verify", "0op", 189, "branch" },
	{ "add", "2op", 20, "result" },
	{ "and", "2op", 9, "result" },
	{ "art_shift", "ext", 3, "result" },
	{ "buffer_mode", "var", 242, "" },
	{ "call_1n", "1op", 143, "" },
	{ "call_1s", "1op", 136, "result" },
	{ "call_2n", "2op", 26, "" },
	{ "call_2s", "2op", 25, "result" },
	{ "call_vn", "var", 249, "" },
	{ "call_vs", "var", 224, "result" },
	{ "check_arg_count", "var", 255, "branch" },
	{ "check_unicode", "ext", 12, "result" },
	{ "clear_attr", "2op", 12, "" },
	{ "copy_table", "var", 253, "" },
	{ "dec", "1op", 134, "" },
	{ "dec_chk", "2op", 4, "branch" },
	{ "div", "2op", 23, "result" },
	{ "draw_picture", "ext", 5, "" },
	{ "encode_text", "var", 252, "" },
	{ "erase_line", "var", 238, "" },
	{ "erase_picture", "ext", 7, "" },
	{ "erase_window", "var", 237, "" },
	{ "get_child", "1op", 130, "result.branch" },
	{ "get_cursor", "var", 240, "" },
	{ "get_next_prop", "2op", 19, "result" },
	{ "get_parent", "1op", 131, "result" },
	{ "get_prop", "2op", 17, "result" },
	{ "get_prop_addr", "2op", 18, "result" },
	{ "get_prop_len", "1op", 132, "result" },
	{ "get_sibling", "1op", 129, "result" },
	{ "get_wind_prop", "ext", 19, "result" },
	{ "inc", "1op", 133, "" },
	{ "inc_chk", "2op", 5, "branch" },
	{ "input_stream", "var", 244, "" },
	{ "insert_obj", "2op", 14, "" },
	{ "je", "2op", 1, "branch" },
	{ "equal?", "ext", 193, "branch" },
	{ "jg", "2op", 3, "branch" },
	{ "jin", "2op", 6, "branch" },
	{ "jl", "2op", 2, "branch" },
	{ "jz", "1op", 128, "branch" },
	{ "load", "1op", 142, "result" },
	{ "bcom", "1op", 248, "result" },
	{ "loadb", "2op", 16, "result" },
	{ "loadw", "2op", 15, "result" },
	{ "log_shift", "ext", 2, "result" },
	{ "make_menu", "ext", 27, "branch" },
	{ "mod", "2op", 24, "result" },
	{ "mouse_window", "ext", 23, "" },
	{ "move_window", "ext", 16, "" },
	{ "mul", "2op", 22, "result" },
	{ "not", "var", 248, "result" },
	{ "or", "2op", 8, "result" },
	{ "output_stream", "var", 243, "" },
	{ "picture_data", "ext", 6, "branch" },
	{ "picture_table", "ext", 28, "" },
	{ "pop_stack", "ext", 21, "" },
	{ "print_addr", "1op", 135, "" },
	{ "print_char", "var", 229, "" },
	{ "print_form", "ext", 26, "" },
	{ "print_num", "var", 230, "" },
	{ "print_obj", "1op", 138, "" },
	{ "print_paddr", "1op", 141, "" },
	{ "print_table", "var", 254, "" },
	{ "print_unicode", "ext", 11, "" },
	{ "pull", "var", 233, "" },
	{ "push", "var", 232, "" },
	{ "push_stack", "ext", 24, "branch" },
	{ "put_prop", "var", 227, "" },
	{ "put_wind_prop", "ext", 25, "" },
	{ "random", "var", 231, "result" },
	{ "read", "var", 228, "result" },
	{ "read_char", "var", 246, "result" },
	{ "read_mouse", "ext", 22, "" },
	{ "remove_obj", "1op", 137, "" },
	{ "restore", "ext", 1, "result" },
	{ "restore_undo", "ext", 10, "result" },
	{ "ret", "1op", 139, "" },
	{ "save", "ext", 0, "result" },
	{ "save_undo", "ext", 9, "result" },
	{ "scan_table", "var", 247, "result" },
	{ "scroll_window", "ext", 20, "" },
	{ "set_attr", "2op", 11, "" },
	{ "set_colour", "2op", 27, "" },
	{ "set_cursor", "var", 239, "" },
	{ "set_font", "ext", 4, "result" },
	{ "set_margins", "ext", 8, "" },
	{ "set_text_style", "var", 241, "" },
	{ "set_window", "var", 235, "" },
	{ "sound_effect", "var", 245, "" },
	{ "split_window", "var", 234, "" },
	{ "store", "2op", 13, "" },
	{ "storeb", "var", 226, "" },
	{ "storew", "var", 225, "" },
	{ "sub", "2op", 21, "result" },
	{ "test", "2op", 7, "branch" },
	{ "test_attr", "2op", 10, "branch" },
	{ "throw", "2op", 28, "" },
	{ "tokenise", "var", 251, "" },
	{ "window_size", "ext", 17, "" },
	{ "window_style", "ext", 18, "" }
};
///////////////////////
//Special purpose function declaration
////////////////////////
void jump_function(string name);
void print_function(string arg);
char GetOperandType(string arg);
bool CheckandUpdateOffset(string name, int offset);
bool CheckandUpdateGVariable(string name, int offset);
bool CheckandUpdateObj(string name, int offset);
bool CheckandUpdateTable(string name, int offset);
unsigned short GetOperandValues(string arg);
/////////////////////////////////////
// String functions
/////////////////////////////////////
string & ltrim(string & str);
string & rtrim(string & str);
void trim(string &s);
string & makebackslash(string &s);
void ZcharEncode(char chin, vector<char>*zcharstr);
int ZStringEncode(const char *pin, char*pout);
bool is_number(const std::string& s);
string strlower(string arg);
string strupper(string arg);
string str_replace(string arg1, char arg2, char arg3);
string str_replace(string arg1, string arg2, string arg3);
///////////////////////////////////////////////////////////////
// Main function declr
//////////////////////////////////////////////////////////////
void WriteToFile(string outfile);					//Write assembly to file
void ReadLinebyLine(ifstream *fp);					//Read file by line
void Assembly();									//Assembly file
void initOpFunctions();								//Init opcodes
void jump_to(string name, vector<char> *opcodes, bool mode);	//jump functions process
int lineProcess(string token, string line, int linenum);

//////////////////////////////////////////////////////////////
// Special function implementation
////////////////////////////////////////////////////////////
void jump_function(string name) {
	assembled.push_back(140);
	int pos = ProgBase + assembled.size();
	bool exist = 0;
	unsigned short temp;
	for (int i = 0; i < gGlobalLabels.size(); i++){
		GLOBALS st = gGlobalLabels.at(i);
		if (name == st.name) {
			temp = (st.offset - pos) & 0xFFFF;
			assembled.push_back(temp >> 8);
			assembled.push_back(temp & 0xFF);
			exist = 1;
			break;
		}
	}
	if (exist == 0) {
		if (firstAssemble == 1) {
			assembled.push_back(0);
			assembled.push_back(0);
		}
		else {
			cout << "Invalid jmp label:" << name << endl;
			exit(1);
		}
	}
	return;
}
void print_function(string arg) {
	if (arg.at(0) != '"') {
		cout << "Invalid print string" << arg << endl;
		return;
	}
	if (arg.back() != '"') {
		cout << "Invalid print string" << arg << endl;
		return;
	}
	arg.erase(0, 1);
	arg.erase(arg.length() - 1, 1);
	if (arg.find('"') != string::npos) {
		cout << "Invalid print string" << arg << endl;
		return;
	}
	char chbuff[MAXCHARS] = { 0 };
	int i, nsize;
	nsize = ZStringEncode(arg.c_str(), chbuff);
	assembled.push_back(178);
	for (i = 0; i < nsize; i++) {
		assembled.push_back(chbuff[i]);
	}
	return;
}
bool CheckandUpdateOffset(string name, int offset) {
	int i;
	bool bex = 0;
	for (i = 0; i < gGlobalLabels.size(); i++) {
		GLOBALS st = gGlobalLabels.at(i);
		if (strlower(st.name) == strlower(name)) {
			if (st.offset == 0)
				gGlobalLabels.at(i).offset = offset;
			bex = 1;
			break;
		}
	}
	return bex;
}
bool CheckandUpdateTable(string name, int offset) {
	int i;
	bool bex = 0;
	for (i = 0; i < gTables.size(); i++) {
		TABLE st = gTables.at(i);
		if (strlower(st.name) == strlower(name)) {
			if (st.offset == 0)
				gTables.at(i).offset = offset;
			bex = 1;
			break;
		}
	}
	return bex;
}
bool CheckandUpdateObj(string name, int offset) {
	int i;
	bool bex = 0;
	for (i = 0; i < gObjects.size(); i++) {
		ZOBJECT st = gObjects.at(i);
		if (strlower(st.objname) == strlower(name)) {
			if (st.offset == 0)
				gObjects.at(i).offset = offset;
			bex = 1;
			break;
		}
	}
	return bex;
}
bool CheckandUpdateGVariable(string name, int offset) {
	int i;
	bool bex = 0;
	for (i = 0; i < gVariables.size(); i++) {
		GLOBALVARIABLE st = gVariables.at(i);
		if (strlower(st.name) == strlower(name)) {
			if (st.offset == 0)
				gVariables.at(i).offset = offset;
			bex = 1;
			break;
		}
	}
	return bex;
}

char GetOperandType(string arg) {
	if (arg.find('$') != string::npos)	//This is variable
		return 2;//10
	if (stoi(arg) < 256 && stoi(arg) >= 0)
		return 1; //01
	return 0;
}
unsigned short GetOperandValues(string arg) {
	char ch = GetOperandType(arg);
	unsigned short temp;
	if (ch == 2) {
		temp = stoi(arg.substr(1), 0, 16);
	}
	else if (ch == 1) {
		temp = stoi(arg);
	}
	else if (ch == 0) {
		if (arg.find(":") != string::npos) {
			arg = arg.substr(1);
			bool exist = 0;
			for (int i = 0; i < gGlobalLabels.size(); i++) {
				GLOBALS st = gGlobalLabels.at(i);
				if (st.name == arg) {
					temp = st.offset;
					exist = 1;
					break;
				}
			}
			if (exist == 0) {
				if (firstAssemble == 1)
					temp = 0;
				else
					cout << "Error found:" << endl;
			}
		}
	}
	return temp;
}
/////////////////////////////////////////////////////////
// string functions
////////////////////////////////////////////////////////
string & ltrim(string & str)
{
	auto it2 = find_if(str.begin(), str.end(), [](char ch){ return !isspace<char>(ch, locale::classic()); });
	str.erase(str.begin(), it2);
	return str;
}

string & rtrim(string & str)
{
	auto it1 = find_if(str.rbegin(), str.rend(), [](char ch){ return !isspace<char>(ch, locale::classic()); });
	str.erase(it1.base(), str.end());
	return str;
}
void trim(string &s) {
	ltrim(s);
	rtrim(s);
}
string & makebackslash(string &s) {
	int pos = s.find("\\n");
	if (pos != string::npos) {
		string str1 = "\n";
		s.replace(pos, 3, str1);
	}
	pos = s.find("\\t");
	if (pos != string::npos) {
		string str2 = "\t";
		s.replace(pos, 3, str2);
	}
	pos = s.find("\\b");
	if (pos != string::npos) {
		string str3 = "\b";
		s.replace(pos, 3, str3);
	}
	s = str_replace(s, "\"", "");
	return s;
}
void ZcharEncode(char chin, vector<char>*zcharstr) {
	char a0[] = "abcdefghijklmnopqrstuvwxyz";
	char a1[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	char a2[] = "\n0123456789.,!?_#'\"/\\-:()";
	if (chin == ' ') {
		zcharstr->push_back(0);
		return;
	}
	int i;
	for (i = 0; i < 26; i++) {
		if (a0[i] == chin) {
			zcharstr->push_back(i + 6);
			return;
		}
	}
	for (i = 0; i < 26; i++) {
		if (a1[i] == chin) {
			zcharstr->push_back(4);
			zcharstr->push_back(i + 6);
			return;
		}
	}
	for (i = 0; i < 26; i++) {
		if (a2[i] == chin) {
			zcharstr->push_back(5);
			zcharstr->push_back(i + 7);
			return;
		}
	}
	zcharstr->push_back(5);
	zcharstr->push_back(6);
	zcharstr->push_back(chin >> 5);
	zcharstr->push_back(chin & 0x1F);
}
int ZStringEncode(const char *pin, char*pout) {
	int i, j = 0;
	string temp = pin;
	temp = makebackslash(temp);
	if (temp == "")
		return 0;
	vector<char> zcharstr;
	for (i = 0; i < temp.length(); i++) {
		ZcharEncode(temp.at(i), &zcharstr);
	}
	while (zcharstr.size() < 0 || zcharstr.size() % 3 != 0)
		zcharstr.push_back(5);
	unsigned short v;
	unsigned short ch1;
	unsigned short ch2;
	unsigned short ch3;
	int nsize = zcharstr.size();
	for (i = 0; i < nsize - 3; i += 3) {
		ch1 = zcharstr.at(i);
		ch2 = zcharstr.at(i + 1);
		ch3 = zcharstr.at(i + 2);
		v = (ch1 << 10) | (ch2 << 5) | ch3;
		pout[j] = v >> 8;
		pout[j + 1] = v & 0xff;
		j += 2;
	}
	ch1 = zcharstr.at(nsize - 3);
	ch2 = zcharstr.at(nsize - 2);
	ch3 = zcharstr.at(nsize - 1);
	v = 0x8000 | (ch1 << 10) | (ch2 << 5) | ch3;
	pout[j] = v >> 8;
	pout[j + 1] = v & 0xff;
	return j + 2;
}
bool is_number(const std::string& s)
{
	std::string::const_iterator it = s.begin();
	while (it != s.end() && std::isdigit(*it)) ++it;
	return !s.empty() && it == s.end();
}
string strlower(string arg)
{
	transform(arg.begin(), arg.end(), arg.begin(), ::tolower);
	return arg;
}
string strupper(string arg)
{
	transform(arg.begin(), arg.end(), arg.begin(), ::toupper);
	return arg;
}
string str_replace(string arg1, char arg2, char arg3) {
	size_t pos; int i;
	for (i = 0; i < arg1.length(); i++)
	{
		if (arg1.at(i) == arg2) {
			arg1.replace(i, 1, 1, arg3);
		}
	}
	return arg1;
}
string str_replace(string arg1, string arg2, string arg3){
	int pos;
	int n = arg2.length();
	while ((pos = arg1.find(arg2)) != string::npos) {
		arg1.replace(pos, n, arg3);
	}
	return arg1;
}

/////////////////////////////////////////////////////////
// main functions
/////////////////////////////////////////////////////////
int MakeByteFromOpcode(string args, string type, int opcode, string flags, char *result, bool bBranchType = false) {
	int pos, tokencount = 0;
	string token, arg = args;
	vector<string> optokens;
	vector<char> opcodes;
	string resultop;
	string jump_name;
	while ((pos = arg.find(DELIMITER)) != string::npos) {
		token = arg.substr(0, pos);
		optokens.push_back(token);
		arg.erase(0, pos + 1);
	}
	if (arg != "") 	optokens.push_back(arg);

	if (flags == "branch") {
		jump_name = optokens.back();
		optokens.pop_back();
	}
	if (flags == "result") {
		resultop = optokens.back();
		optokens.pop_back();
	}
	tokencount = optokens.size();

	if (type == "0op") {
		if (tokencount != 0) {
			cout << "Too many argument error:" << endl;
			exit(1);
		}
		opcodes.push_back(0xB0 | opcode);
	}
	else if (type == "1op") {
		if (tokencount != 1) {
			cout << "Too many argument error:" << endl;
			exit(1);
		}
		string val = optokens.at(0);
		unsigned char oval = GetOperandType(val);
		opcodes.push_back(0x80 | oval << 4 | opcode);
		opcodes.push_back(GetOperandValues(optokens.at(0)));
	}
	if (flags == "result") {
		if (GetOperandType(resultop) != 2) {
			cout << "Result argument error." << endl;
			exit(1);
		}
		opcodes.push_back(GetOperandValues(resultop));
	}
	if (flags == "branch") {
		jump_to(jump_name, &opcodes, bBranchType);
	}
	for (pos = 0; pos < opcodes.size(); pos++) {
		char ch = opcodes.at(pos);
		assembled.push_back(ch);
	}
	return opcodes.size();
}
void jump_to(string name, vector<char> *opcodes, bool mode = 0){
	if (name == "rfalse") {
		if (mode == 0)
			opcodes->push_back(0x80);
		opcodes->push_back(0);
		return;
	}
	if (name == "rtrue") {
		if (mode == 0)
			opcodes->push_back(0x80);
		opcodes->push_back(1);
		return;
	}
	int pos = ProgBase + assembled.size() + opcodes->size();
	unsigned short temp;
	bool exist = 0;
	for (int i = 0; i < gGlobalLabels.size(); i++){
		GLOBALS st = gGlobalLabels.at(i);

		if (strlower(name) == strlower(st.name)) {
			if (mode == 0) {
				temp = 0x8000 | (st.offset - pos) & 0x3FFF;
			}
			else {
				temp = (st.offset - pos) & 0x3FFF;
			}
			opcodes->push_back(temp >> 8);
			opcodes->push_back(temp & 0xFF);
			exist = 1;
			break;
		}
	}
	if (exist == 0) {
		if (firstAssemble == 1) {
			opcodes->push_back(0);
			opcodes->push_back(0);
		}
		else {
			cout << "Invalid jmp label:" << name << endl;
			exit(1);
		}
	}
}

void initOpFunctions() {
	for (int i = 0; i < OPCODE_COUNT; i++) {
		if (codeTable_YZIP[i].resulttype == "branch") {
			STINSTRUCTION stIns;
			stIns.instname = codeTable_YZIP[i].instname + "!";
			stIns.insttype = codeTable_YZIP[i].insttype;
			stIns.resulttype = codeTable_YZIP[i].resulttype;
			stIns.opcode = codeTable_YZIP[i].opcode;
			stIns.func = MakeByteFromOpcode;
			gOpcodes.push_back(stIns);
		}
		STINSTRUCTION stIns;
		stIns.instname = codeTable_YZIP[i].instname;
		stIns.insttype = codeTable_YZIP[i].insttype;
		stIns.resulttype = codeTable_YZIP[i].resulttype;
		stIns.opcode = codeTable_YZIP[i].opcode;
		stIns.func = MakeByteFromOpcode;
		gOpcodes.push_back(stIns);
	}
}
void Assembly() {
	int i, pos;
	string token;
	int nsize = gInstructions.size();
	for (i = 0; i < nsize; i++) {
		string line = gInstructions.at(i);

		pos = line.find(DELIMITER);
		if (pos == -1) {
			token = line;
			line = "";
		}
		else {
			token = line.substr(0, pos);
			line.erase(0, pos + 1);
			trim(line);
		}
		token = strlower(token);
		if (token == ".end")
			break;
		trim(token);
		pos = lineProcess(token, line, i);
		i = (pos != 0) ? pos : i;
	}
	while (assembled.size() % ALIGNMENT)
		assembled.push_back(0);
	while (databytes.size() % ALIGNMENT)
		databytes.push_back(0);
}
bool IsCodeSegment() {
	int i;
	for (i = 0; i < gGlobalLabels.size(); i++) {
		GLOBALS g = gGlobalLabels.at(i);			//if endlod already parsed then after that is code segment's label.
		if (g.name == "endlod")
			return 1;
	}
	return 0;
}
void RemoveSegmentFlag() {
	int i;
	for (i = 0; i < gGlobalLabels.size(); i++) {
		GLOBALS g = gGlobalLabels.at(i);			//if endlod already parsed then after that is code segment's label.
		if (g.name == "endlod")
		{
			gGlobalLabels.erase(gGlobalLabels.begin() + i);
			break;
		}
	}
}
int getGlobalPointer(string valname) {
	int i;
	for (i = 0; i < gGlobalLabels.size(); i++) {
		GLOBALS g = gGlobalLabels.at(i);			//if endlod already parsed then after that is code segment's label.
		if (strlower(g.name) == strlower(valname))
			return g.offset;
	}
	return 0;
}
int getGlobalValue(string valname) {
	int i;
	for (i = 0; i < gGlobalLabels.size(); i++) {
		GLOBALS g = gGlobalLabels.at(i);			//if endlod already parsed then after that is code segment's label.
		if (strlower(g.name) == strlower(valname))
			return g.value.at(0);
	}
	return 0;
}
int Split(string arg, string delimiter, string *res) {
	int pos, decnt = delimiter.length();
	string token;
	int i = 0;
	while ((pos = arg.find(delimiter)) != string::npos) {
		token = arg.substr(0, pos);
		res[i] = token;
		arg.erase(0, pos + decnt);
		i++;
	}
	if (arg != "") {
		res[i] = arg;
		return i + 1;
	}
	return i;
}
int lineProcess(string token, string line, int linenum) {
	int pos = token.length() - 1, nRet = 0, nl;
	int i, j;
	unsigned short val;
	bool isCode = IsCodeSegment();
	if (token == ".new")
		zversion = stoi(line);
	else if (token.find("::") != string::npos) {
		if (line == "") {
			GLOBALS g;
			g.datatype = "statement";
			g.name = str_replace(token, "::", "");
			g.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
			g.type = isCode ? TYPE_CODE : TYPE_DATA;
			if (CheckandUpdateOffset(token, g.offset) == 0)
				gGlobalLabels.push_back(g);
		}
		else if ((pos = line.find(DELIMITER)) != string::npos) {
			string arg1 = line.substr(0, pos);
			string arg2 = line.erase(0, pos + 1);
			arg1 = strlower(arg1);
			GLOBALS g;
			if (arg1 == ".byte") {
				g.datatype = "byte";
				g.name = str_replace(token, "::", "");
				g.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
				g.type = isCode ? TYPE_CODE : TYPE_DATA;
				if (is_number(arg2) == false) {
					cout << ".byte data have to be number";
					exit(1);
				}
				val = stoi(arg2);
				isCode ? assembled.push_back(val & 0xff) : databytes.push_back(val & 0xff);
				g.value.push_back(val);
				databytes.push_back(val & 0xFF);
				if (CheckandUpdateOffset(token, g.offset) == 0)
					gGlobalLabels.push_back(g);
			}
			else if (arg1 == ".word") {
				g.datatype = "word";
				g.name = str_replace(token, "::", "");
				g.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
				g.type = isCode ? TYPE_CODE : TYPE_DATA;
				if (is_number(arg2) == false) {
					cout << ".byte data have to be number";
					exit(1);
				}
				val = stoi(arg2);
				g.value.push_back(val);
				isCode ? assembled.push_back(val >> 8) : databytes.push_back(val >> 8);
				isCode ? assembled.push_back(val & 0xff) : databytes.push_back(val & 0xff);
				if (CheckandUpdateOffset(token, g.offset) == 0)
					gGlobalLabels.push_back(g);
			}
			else if (arg1 == ".table") {
				TABLE t;
				t.name = str_replace(token, "::", "");
				t.maxsize = 0;
				t.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
				nl = linenum + 1;
				while (1) {
					if (gInstructions.size() <= nl)
						break;
					string newline = gInstructions.at(nl);
					trim(newline);
					if (newline == ".ENDT") {
						nRet = nl;
						break;
					}

					if (is_number(newline)) {
						TABLEITEM item;
						val = stoi(newline);
						item.data.push_back(val);
						item.type = "word";
						item.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
						t.items.push_back(item);
						isCode ? assembled.push_back(val >> 8) : databytes.push_back(val >> 8);
						isCode ? assembled.push_back(val & 0xff) : databytes.push_back(val & 0xff);
					}
					else if ((pos = newline.find(DELIMITER)) != string::npos) {
						string arg1 = newline.substr(0, pos);
						string arg2 = newline.erase(0, pos + 1);
						arg1 = strlower(arg1);
						if (arg1 == ".strl") {
							TABLEITEM item;
							val = arg2.length();
							item.data.push_back(arg2.length());
							item.type = "byte";
							item.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
							isCode ? assembled.push_back(val & 0xff) : databytes.push_back(val & 0xff);

							TABLEITEM item1;
							item1.type = "word";
							char outbuff[MAXCHARS] = { 0 };
							item1.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
							int nsize = ZStringEncode(arg2.c_str(), outbuff);
							for (i = 0; i < nsize; i += 2) {
								val = *(unsigned short*)(outbuff + i);
								item1.data.push_back(val);
								isCode ? assembled.push_back(outbuff[0]) : databytes.push_back(outbuff[0]);
								isCode ? assembled.push_back(outbuff[1]) : databytes.push_back(outbuff[1]);
							}
							t.items.push_back(item);
							t.items.push_back(item1);
						}
						else if (arg1 == ".str") {
							TABLEITEM item1;
							item1.type = "word";
							item1.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
							char outbuff[MAXCHARS] = { 0 };
							int nsize = ZStringEncode(arg2.c_str(), outbuff);
							for (i = 0; i < nsize; i += 2) {
								val = *(unsigned short*)(outbuff + i);
								item1.data.push_back(val);
								isCode ? assembled.push_back(outbuff[0]) : databytes.push_back(outbuff[0]);
								isCode ? assembled.push_back(outbuff[1]) : databytes.push_back(outbuff[1]);
							}
							t.items.push_back(item1);
						}
						else if (arg1 == ".len") {
							TABLEITEM item;
							val = arg2.length();
							item.data.push_back(val);
							item.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
							item.type = "byte";
							t.items.push_back(item);
							isCode ? assembled.push_back(val >> 8) : databytes.push_back(val >> 8);
						}
						else if (arg1 == ".zword") {
							TABLEITEM item;
							item.type = "byte";
							item.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
							char outbuff[MAXCHARS] = { 0 };
							int nsize = ZStringEncode(arg2.c_str(), outbuff);
							for (i = 0; i < nsize; i++) {
								val = outbuff[i];
								item.data.push_back(val);
								isCode ? assembled.push_back(val) : databytes.push_back(val);
							}
							t.items.push_back(item);
						}
						else if (arg1 == ".byte") {
							TABLEITEM item;
							val = stoi(arg2);
							item.data.push_back(val);
							item.type = "byte";
							item.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
							isCode ? assembled.push_back(val) : databytes.push_back(val);
							t.items.push_back(item);
						}
						else if (arg1 == ".word") {
							TABLEITEM item;
							val = stoi(arg2);
							item.data.push_back(stoi(arg2));
							item.type = "word";
							item.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
							isCode ? assembled.push_back(val >> 8) : databytes.push_back(val >> 8);
							isCode ? assembled.push_back(val & 0xff) : databytes.push_back(val & 0xff);
							t.items.push_back(item);
						}
						else if (arg1 == ".true") {
							TABLEITEM item;
							item.data.push_back(1);
							item.type = "true";
							item.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
							t.items.push_back(item);
							isCode ? assembled.push_back(1) : databytes.push_back(1);
						}
						else if (arg1 == ".false") {
							TABLEITEM item;
							item.data.push_back(0);
							item.type = "false";
							item.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
							t.items.push_back(item);
							isCode ? assembled.push_back(0) : databytes.push_back(0);
						}
						else if (arg1 == ".object") {
							string buff[8];
							Split(arg2, ",", buff);
							ZOBJECT zobj;
							zobj.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
							zobj.objname = buff[0];
							zobj.flag1 = stoi(buff[1]);
							zobj.flag2 = stoi(buff[2]);
							zobj.flag3 = stoi(buff[3]);
							zobj.LOC = is_number(buff[4]) ? stoi(buff[4]) : getGlobalPointer(buff[4]);
							zobj.FIRST = is_number(buff[5]) ? stoi(buff[5]) : getGlobalPointer(buff[5]);
							zobj.NEXT = is_number(buff[6]) ? stoi(buff[6]) : getGlobalPointer(buff[6]);
							zobj.proptable = is_number(buff[7]) ? stoi(buff[7]) : getGlobalPointer(buff[7]);

							isCode ? assembled.push_back(gObjects.size() >> 8) : databytes.push_back(gObjects.size() >> 8);
							isCode ? assembled.push_back(gObjects.size() & 0xff) : databytes.push_back(gObjects.size() & 0xff);

							isCode ? assembled.push_back(zobj.flag1 >> 8) : databytes.push_back(zobj.flag1 >> 8);
							isCode ? assembled.push_back(zobj.flag1 & 0xff) : databytes.push_back(zobj.flag1 & 0xff);

							isCode ? assembled.push_back(zobj.flag2 >> 8) : databytes.push_back(zobj.flag2 >> 8);
							isCode ? assembled.push_back(zobj.flag2 & 0xff) : databytes.push_back(zobj.flag2 & 0xff);

							isCode ? assembled.push_back(zobj.flag3 >> 8) : databytes.push_back(zobj.flag3 >> 8);
							isCode ? assembled.push_back(zobj.flag3 & 0xff) : databytes.push_back(zobj.flag3 & 0xff);

							isCode ? assembled.push_back(zobj.LOC >> 8) : databytes.push_back(zobj.LOC >> 8);
							isCode ? assembled.push_back(zobj.LOC & 0xff) : databytes.push_back(zobj.LOC & 0xff);

							isCode ? assembled.push_back(zobj.FIRST >> 8) : databytes.push_back(zobj.FIRST >> 8);
							isCode ? assembled.push_back(zobj.FIRST & 0xff) : databytes.push_back(zobj.FIRST & 0xff);

							isCode ? assembled.push_back(zobj.NEXT >> 8) : databytes.push_back(zobj.NEXT >> 8);
							isCode ? assembled.push_back(zobj.NEXT & 0xff) : databytes.push_back(zobj.NEXT & 0xff);

							isCode ? assembled.push_back(zobj.proptable >> 8) : databytes.push_back(zobj.proptable >> 8);
							isCode ? assembled.push_back(zobj.proptable & 0xff) : databytes.push_back(zobj.proptable & 0xff);
							if (CheckandUpdateObj(zobj.objname, zobj.offset) == 0)
								gObjects.push_back(zobj);
						}
						else if (arg1 == ".gvar") {
							string buff[2];
							Split(arg2, ",", buff);
							unsigned short val;
							if (is_number(buff[1])) {
								val = stoi(buff[1]);
							}
							else {
								val = getGlobalValue(buff[1]);
							}
							GLOBALVARIABLE gv;
							gv.name = buff[0];
							gv.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
							gv.value.push_back(val);
							if (CheckandUpdateGVariable(gv.name, gv.offset) == 0)
								gVariables.push_back(gv);

							isCode ? assembled.push_back(val & 0xff) : databytes.push_back(val & 0xff);
						}
						else if (arg1 == ".prop") {
							TABLEITEM item;
							pos = arg2.find(",");
							string arg3 = arg2.substr(0, pos);
							string arg4 = arg2.erase(0, pos + 1);
							unsigned short v1, v2, v3;
							if (is_number(arg4)) {
								v2 = stoi(arg3);
								v3 = stoi(arg4);
							}
							else {
								v2 = stoi(arg3);
								v3 = getGlobalValue(arg4);
							}
							v1 = v2 << 5 | v3;
							item.type = "prop";
							item.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
							item.data.push_back(v1);
							t.items.push_back(item);
							isCode ? assembled.push_back(v1 & 0xff) : databytes.push_back(v1 & 0xff);
						}
					}
					else {
						unsigned short v1 = getGlobalValue(newline);
						TABLEITEM item;
						item.data.push_back(v1);
						item.type = "word";
						item.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
						t.items.push_back(item);
						isCode ? assembled.push_back(v1 >> 8) : databytes.push_back(v1 >> 8);
						isCode ? assembled.push_back(v1 & 0xff) : databytes.push_back(v1 & 0xff);
					}
					nl++;
				}
				if (CheckandUpdateTable(t.name, t.offset) == 0)
					gTables.push_back(t);
			}
			else if (arg1 == ".strl") {
				GLOBALS g;
				g.datatype = "byte";
				g.name = str_replace(token, "::", "");
				g.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
				g.type = isCode ? TYPE_CODE : TYPE_DATA;

				val = arg2.length();
				g.value.push_back(val);
				isCode ? assembled.push_back(val & 0xff) : databytes.push_back(val & 0xff);

				GLOBALS g1;
				g1.datatype = "strl";
				g1.name = str_replace(token, "::", "");
				g1.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
				g1.type = isCode ? TYPE_CODE : TYPE_DATA;

				char outbuff[MAXCHARS] = { 0 };
				int nsize = ZStringEncode(arg2.c_str(), outbuff);
				for (i = 0; i < nsize; i += 2) {
					val = *(unsigned short*)(outbuff + i);
					g1.value.push_back(val);
					isCode ? assembled.push_back(outbuff[0]) : databytes.push_back(outbuff[0]);
					isCode ? assembled.push_back(outbuff[1]) : databytes.push_back(outbuff[1]);
				}
				if (CheckandUpdateOffset(g.name, g.offset) == 0)
					gGlobalLabels.push_back(g);
				if (CheckandUpdateOffset(g1.name, g1.offset) == 0)
					gGlobalLabels.push_back(g1);
			}
			else if (arg1 == ".str") {
				GLOBALS g1;
				g1.datatype = "str";
				g1.name = str_replace(token, "::", "");
				g1.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
				g1.type = isCode ? TYPE_CODE : TYPE_DATA;

				char outbuff[MAXCHARS] = { 0 };
				int nsize = ZStringEncode(arg2.c_str(), outbuff);
				for (i = 0; i < nsize; i += 2) {
					val = *(unsigned short*)(outbuff + i);
					g1.value.push_back(val);
					isCode ? assembled.push_back(outbuff[0]) : databytes.push_back(outbuff[0]);
					isCode ? assembled.push_back(outbuff[1]) : databytes.push_back(outbuff[1]);
				}
				if (CheckandUpdateOffset(g1.name, g1.offset) == 0)
					gGlobalLabels.push_back(g1);
			}
			else if (arg1 == ".len") {
				GLOBALS g;
				g.datatype = "len";
				g.name = str_replace(token, "::", "");
				g.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
				g.type = isCode ? TYPE_CODE : TYPE_DATA;

				val = arg2.length();
				g.value.push_back(val);
				isCode ? assembled.push_back(val & 0xff) : databytes.push_back(val & 0xff);
				if (CheckandUpdateOffset(g.name, g.offset) == 0)
					gGlobalLabels.push_back(g);
			}
			else if (arg1 == ".zword") {
				GLOBALS g;
				g.datatype = "len";
				g.name = str_replace(token, "::", "");
				g.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
				g.type = isCode ? TYPE_CODE : TYPE_DATA;
				char outbuff[MAXCHARS] = { 0 };
				int nsize = ZStringEncode(arg2.c_str(), outbuff);
				for (i = 0; i < nsize; i++) {
					val = outbuff[i];
					g.value.push_back(val);
					isCode ? assembled.push_back(val) : databytes.push_back(val);
				}
				if (CheckandUpdateOffset(g.name, g.offset) == 0)
					gGlobalLabels.push_back(g);
			}
			else if (arg1 == ".true") {
				GLOBALS g;
				g.datatype = "true";
				g.name = str_replace(token, "::", "");
				g.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
				g.type = isCode ? TYPE_CODE : TYPE_DATA;
				g.value.push_back(1);
				isCode ? assembled.push_back(1) : databytes.push_back(1);
				if (CheckandUpdateOffset(g.name, g.offset) == 0)
					gGlobalLabels.push_back(g);
			}
			else if (arg1 == ".false") {
				GLOBALS g;
				g.datatype = "true";
				g.name = str_replace(token, "::", "");
				g.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
				g.type = isCode ? TYPE_CODE : TYPE_DATA;
				g.value.push_back(0);
				isCode ? assembled.push_back(0) : databytes.push_back(0);
				if (CheckandUpdateOffset(g.name, g.offset) == 0)
					gGlobalLabels.push_back(g);
			}
			else if (arg1 == ".object") {
				string buff[8];
				Split(arg2, ",", buff);
				ZOBJECT zobj;
				zobj.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
				zobj.objname = buff[0];
				zobj.flag1 = stoi(buff[1]);
				zobj.flag2 = stoi(buff[2]);
				zobj.flag3 = stoi(buff[3]);
				zobj.LOC = is_number(buff[4]) ? stoi(buff[4]) : getGlobalPointer(buff[4]);
				zobj.FIRST = is_number(buff[5]) ? stoi(buff[5]) : getGlobalPointer(buff[5]);
				zobj.NEXT = is_number(buff[6]) ? stoi(buff[6]) : getGlobalPointer(buff[6]);
				zobj.proptable = is_number(buff[7]) ? stoi(buff[7]) : getGlobalPointer(buff[7]);

				isCode ? assembled.push_back(gObjects.size() >> 8) : databytes.push_back(gObjects.size() >> 8);
				isCode ? assembled.push_back(gObjects.size() & 0xff) : databytes.push_back(gObjects.size() & 0xff);

				isCode ? assembled.push_back(zobj.flag1 >> 8) : databytes.push_back(zobj.flag1 >> 8);
				isCode ? assembled.push_back(zobj.flag1 & 0xff) : databytes.push_back(zobj.flag1 & 0xff);

				isCode ? assembled.push_back(zobj.flag2 >> 8) : databytes.push_back(zobj.flag2 >> 8);
				isCode ? assembled.push_back(zobj.flag2 & 0xff) : databytes.push_back(zobj.flag2 & 0xff);

				isCode ? assembled.push_back(zobj.flag3 >> 8) : databytes.push_back(zobj.flag3 >> 8);
				isCode ? assembled.push_back(zobj.flag3 & 0xff) : databytes.push_back(zobj.flag3 & 0xff);

				isCode ? assembled.push_back(zobj.LOC >> 8) : databytes.push_back(zobj.LOC >> 8);
				isCode ? assembled.push_back(zobj.LOC & 0xff) : databytes.push_back(zobj.LOC & 0xff);

				isCode ? assembled.push_back(zobj.FIRST >> 8) : databytes.push_back(zobj.FIRST >> 8);
				isCode ? assembled.push_back(zobj.FIRST & 0xff) : databytes.push_back(zobj.FIRST & 0xff);

				isCode ? assembled.push_back(zobj.NEXT >> 8) : databytes.push_back(zobj.NEXT >> 8);
				isCode ? assembled.push_back(zobj.NEXT & 0xff) : databytes.push_back(zobj.NEXT & 0xff);

				isCode ? assembled.push_back(zobj.proptable >> 8) : databytes.push_back(zobj.proptable >> 8);
				isCode ? assembled.push_back(zobj.proptable & 0xff) : databytes.push_back(zobj.proptable & 0xff);
				if (CheckandUpdateObj(zobj.objname, zobj.offset) == 0)
					gObjects.push_back(zobj);
			}
			else if (arg1 == ".gvar") {
				string buff[2];
				Split(arg2, ",", buff);
				unsigned short val;
				if (is_number(buff[1])) {
					val = stoi(buff[1]);
				}
				else {
					val = getGlobalValue(buff[1]);
				}
				GLOBALVARIABLE gv;
				gv.name = buff[0];
				gv.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
				gv.value.push_back(val);
				if (CheckandUpdateGVariable(gv.name, g.offset) == 0)
					gVariables.push_back(gv);

				isCode ? assembled.push_back(val & 0xff) : databytes.push_back(val & 0xff);
			}
			else if (arg1 == ".prop") {
				GLOBALS g;
				g.datatype = "prop";
				g.name = str_replace(token, "::", "");
				g.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
				g.type = isCode ? TYPE_CODE : TYPE_DATA;

				pos = arg2.find(",");
				string arg3 = arg2.substr(0, pos);
				string arg4 = arg2.erase(0, pos + 1);
				unsigned short v1, v2, v3;
				if (is_number(arg4)) {
					v2 = stoi(arg3);
					v3 = stoi(arg4);
				}
				else {
					v2 = stoi(arg3);
					v3 = getGlobalValue(arg4);
				}
				v1 = v2 << 5 | v3;
				g.value.push_back(v1);
				isCode ? assembled.push_back(v1 & 0xff) : databytes.push_back(v1 & 0xff);
				if (CheckandUpdateOffset(g.name, g.offset) == 0)
					gGlobalLabels.push_back(g);
			}

		}
		else {
			GLOBALS g;
			if (is_number(line) == false) {
				if (line.at(0) == '"') {
					//String;
					g.datatype = "string";
					g.name = str_replace(token, "::", "");
					g.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
					g.type = isCode ? TYPE_CODE : TYPE_DATA;
					char outbuff[MAXCHARS] = { 0 };
					int nsize = ZStringEncode(line.c_str(), outbuff);
					for (i = 0; i < nsize; i += 2) {
						val = *(unsigned short*)(outbuff + i);
						g.value.push_back(val);
						isCode ? assembled.push_back(outbuff[i]) : databytes.push_back(outbuff[i]);
						isCode ? assembled.push_back(outbuff[i + 1]) : databytes.push_back(outbuff[i + 1]);
					}
					if (CheckandUpdateOffset(g.name, g.offset) == 0)
						gGlobalLabels.push_back(g);
				}
				else if (strlower(line) == ".table") {
					TABLE t;
					t.name = str_replace(token, "::", "");
					t.maxsize = 0;
					t.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
					nl = linenum + 1;
					while (1) {
						if (gInstructions.size() <= nl)
							break;
						string newline = gInstructions.at(nl);
						trim(newline);
						if (newline == ".ENDT") {
							nRet = nl;
							break;
						}
						if (is_number(newline)) {
							TABLEITEM item;
							val = stoi(newline);
							item.data.push_back(val);
							item.type = "word";
							item.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
							t.items.push_back(item);
							isCode ? assembled.push_back(val >> 8) : databytes.push_back(val >> 8);
							isCode ? assembled.push_back(val & 0xff) : databytes.push_back(val & 0xff);
						}
						else if ((pos = newline.find(DELIMITER)) != string::npos) {
							string arg1 = newline.substr(0, pos);
							string arg2 = newline.erase(0, pos + 1);
							arg1 = strlower(arg1);
							if (arg1 == ".strl") {
								TABLEITEM item;
								arg2 = str_replace(arg2, "\"", "");
								val = arg2.length();
								item.data.push_back(arg2.length());
								item.type = "byte";
								item.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
								isCode ? assembled.push_back(val & 0xff) : databytes.push_back(val & 0xff);

								TABLEITEM item1;
								item1.type = "word";
								item1.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
								char outbuff[MAXCHARS] = { 0 };
								int nsize = ZStringEncode(arg2.c_str(), outbuff);
								for (i = 0; i < nsize; i += 2) {
									val = *(unsigned short*)(outbuff + i);
									item1.data.push_back(val);
									isCode ? assembled.push_back(outbuff[0]) : databytes.push_back(outbuff[0]);
									isCode ? assembled.push_back(outbuff[1]) : databytes.push_back(outbuff[1]);
								}
								t.items.push_back(item);
								t.items.push_back(item1);
							}
							else if (arg1 == ".str") {
								TABLEITEM item1;
								arg2 = str_replace(arg2, "\"", "");
								item1.type = "word";
								char outbuff[MAXCHARS] = { 0 };
								int nsize = ZStringEncode(arg2.c_str(), outbuff);
								item1.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
								for (i = 0; i < nsize; i += 2) {
									val = *(unsigned short*)(outbuff + i);
									item1.data.push_back(val);
									isCode ? assembled.push_back(outbuff[0]) : databytes.push_back(outbuff[0]);
									isCode ? assembled.push_back(outbuff[1]) : databytes.push_back(outbuff[1]);
								}
								t.items.push_back(item1);
							}
							else if (arg1 == ".len") {
								TABLEITEM item;
								arg2 = str_replace(arg2, "\"", "");
								val = arg2.length();
								item.data.push_back(val);
								item.type = "byte";
								item.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
								t.items.push_back(item);
								isCode ? assembled.push_back(val >> 8) : databytes.push_back(val >> 8);
							}
							else if (arg1 == ".zword") {
								TABLEITEM item;
								item.type = "byte";
								char outbuff[MAXCHARS] = { 0 };
								item.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
								int nsize = ZStringEncode(arg2.c_str(), outbuff);
								for (i = 0; i < nsize; i++) {
									val = outbuff[i];
									item.data.push_back(val);
									isCode ? assembled.push_back(val) : databytes.push_back(val);
								}
								t.items.push_back(item);
							}
							else if (arg1 == ".byte") {
								TABLEITEM item;
								val = stoi(arg2);
								item.data.push_back(val);
								item.type = "byte";
								item.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
								isCode ? assembled.push_back(val) : databytes.push_back(val);
								t.items.push_back(item);
							}
							else if (arg1 == ".word") {
								TABLEITEM item;
								val = stoi(arg2);
								item.data.push_back(stoi(arg2));
								item.type = "word";
								item.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
								isCode ? assembled.push_back(val >> 8) : databytes.push_back(val >> 8);
								isCode ? assembled.push_back(val & 0xff) : databytes.push_back(val & 0xff);
								t.items.push_back(item);
							}
							else if (arg1 == ".true") {
								TABLEITEM item;
								item.data.push_back(1);
								item.type = "true";
								item.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
								t.items.push_back(item);
								isCode ? assembled.push_back(1) : databytes.push_back(1);
							}
							else if (arg1 == ".false") {
								TABLEITEM item;
								item.data.push_back(0);
								item.type = "false";
								item.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
								t.items.push_back(item);
								isCode ? assembled.push_back(0) : databytes.push_back(0);
							}
							else if (arg1 == ".object") {
								string buff[8];
								Split(arg2, ",", buff);
								ZOBJECT zobj;
								zobj.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
								zobj.objname = buff[0];
								zobj.flag1 = stoi(buff[1]);
								zobj.flag2 = stoi(buff[2]);
								zobj.flag3 = stoi(buff[3]);
								zobj.LOC = is_number(buff[4]) ? stoi(buff[4]) : getGlobalPointer(buff[4]);
								zobj.FIRST = is_number(buff[5]) ? stoi(buff[5]) : getGlobalPointer(buff[5]);
								zobj.NEXT = is_number(buff[6]) ? stoi(buff[6]) : getGlobalPointer(buff[6]);
								zobj.proptable = is_number(buff[7]) ? stoi(buff[7]) : getGlobalPointer(buff[7]);

								isCode ? assembled.push_back(gObjects.size() >> 8) : databytes.push_back(gObjects.size() >> 8);
								isCode ? assembled.push_back(gObjects.size() & 0xff) : databytes.push_back(gObjects.size() & 0xff);

								isCode ? assembled.push_back(zobj.flag1 >> 8) : databytes.push_back(zobj.flag1 >> 8);
								isCode ? assembled.push_back(zobj.flag1 & 0xff) : databytes.push_back(zobj.flag1 & 0xff);

								isCode ? assembled.push_back(zobj.flag2 >> 8) : databytes.push_back(zobj.flag2 >> 8);
								isCode ? assembled.push_back(zobj.flag2 & 0xff) : databytes.push_back(zobj.flag2 & 0xff);

								isCode ? assembled.push_back(zobj.flag3 >> 8) : databytes.push_back(zobj.flag3 >> 8);
								isCode ? assembled.push_back(zobj.flag3 & 0xff) : databytes.push_back(zobj.flag3 & 0xff);

								isCode ? assembled.push_back(zobj.LOC >> 8) : databytes.push_back(zobj.LOC >> 8);
								isCode ? assembled.push_back(zobj.LOC & 0xff) : databytes.push_back(zobj.LOC & 0xff);

								isCode ? assembled.push_back(zobj.FIRST >> 8) : databytes.push_back(zobj.FIRST >> 8);
								isCode ? assembled.push_back(zobj.FIRST & 0xff) : databytes.push_back(zobj.FIRST & 0xff);

								isCode ? assembled.push_back(zobj.NEXT >> 8) : databytes.push_back(zobj.NEXT >> 8);
								isCode ? assembled.push_back(zobj.NEXT & 0xff) : databytes.push_back(zobj.NEXT & 0xff);

								isCode ? assembled.push_back(zobj.proptable >> 8) : databytes.push_back(zobj.proptable >> 8);
								isCode ? assembled.push_back(zobj.proptable & 0xff) : databytes.push_back(zobj.proptable & 0xff);
								if (CheckandUpdateObj(zobj.objname, zobj.offset) == 0)
									gObjects.push_back(zobj);
							}
							else if (arg1 == ".gvar") {
								string buff[2];
								Split(arg2, "=", buff);
								unsigned short val;
								if (is_number(buff[1])) {
									val = stoi(buff[1]);
								}
								else {
									val = getGlobalValue(buff[1]);
								}
								GLOBALVARIABLE gv;
								gv.name = buff[0];
								gv.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
								gv.value.push_back(val);
								if (CheckandUpdateGVariable(gv.name, gv.offset) == 0)
									gVariables.push_back(gv);

								isCode ? assembled.push_back(val & 0xff) : databytes.push_back(val & 0xff);
							}
							else if (arg1 == ".prop") {
								TABLEITEM item;
								pos = arg2.find(",");
								string arg3 = arg2.substr(0, pos);
								string arg4 = arg2.erase(0, pos + 1);
								unsigned short v1, v2, v3;
								if (is_number(arg4)) {
									v2 = stoi(arg3);
									v3 = stoi(arg4);
								}
								else {
									v2 = stoi(arg3);
									v3 = getGlobalValue(arg4);
								}
								v1 = v2 << 5 | v3;
								item.type = "prop";
								item.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
								item.data.push_back(v1);
								t.items.push_back(item);
								isCode ? assembled.push_back(v1 & 0xff) : databytes.push_back(v1 & 0xff);
							}
						}
						else {
							unsigned short v1 = getGlobalPointer(newline);
							TABLEITEM item;
							item.data.push_back(v1);
							item.type = "word";
							item.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
							t.items.push_back(item);
							isCode ? assembled.push_back(v1 >> 8) : databytes.push_back(v1 >> 8);
							isCode ? assembled.push_back(v1 & 0xff) : databytes.push_back(v1 & 0xff);
						}
						nl++;
					}
					if (CheckandUpdateTable(t.name, t.offset) == 0)
						gTables.push_back(t);

				}
				else {
					g.value.push_back(val);
					g.name = str_replace(token, "::", "");
					g.datatype = line;
					g.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
					if (CheckandUpdateOffset(g.name, g.offset) == 0)
						gGlobalLabels.push_back(g);
				}
			}
			else {
				GLOBALS g;
				g.datatype = "word";
				g.name = str_replace(token, "::", "");
				g.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
				g.type = isCode ? TYPE_CODE : TYPE_DATA;
				val = stoi(line);
				g.value.push_back(val);
				if (CheckandUpdateOffset(g.name, g.offset) == 0)
					gGlobalLabels.push_back(g);
				isCode ? assembled.push_back(val >> 8) : databytes.push_back(val >> 8);
				isCode ? assembled.push_back(val & 0xff) : databytes.push_back(val & 0xff);
			}
		}
	}
	else if (token.at(pos) == ':') {
		//local simbol
		if (line == "") {
			GLOBALS g;
			g.datatype = "label";
			g.name = str_replace(token, ":", "");

			g.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
			g.type = isCode ? TYPE_CODE : TYPE_DATA;
			if (CheckandUpdateOffset(g.name, g.offset) == 0)
				gGlobalLabels.push_back(g);
		}
		else {
			string buff[2];
			Split(line, DELIMITER, buff);
			lineProcess(buff[0], buff[1], linenum);
		}
	}
	else if ((pos = token.find('=')) != -1) {
		//This is local label;
		string arg1 = token.substr(0, pos);
		string arg2 = token.erase(0, pos + 1);
		GLOBALS g;
		g.name = arg1;
		g.type = isCode ? TYPE_CODE : TYPE_DATA;
		g.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();

		if (arg2.find('"') != string::npos) {
			g.datatype = "string";
			char outbuff[MAXCHARS] = { 0 };
			int nsize = ZStringEncode(arg2.c_str(), outbuff);
			for (i = 0; i < nsize; i += 2) {
				val = *(unsigned short*)(outbuff + i);
				g.value.push_back(val);
				isCode ? assembled.push_back(outbuff[i]) : databytes.push_back(outbuff[i]);
				isCode ? assembled.push_back(outbuff[i + 1]) : databytes.push_back(outbuff[i + 1]);
			}
		}
		else if (is_number(arg2)){
			g.datatype = "word";
			val = stoi(arg2);
			g.value.push_back(val);
			isCode ? assembled.push_back(val >> 8) : databytes.push_back(val >> 8);
			isCode ? assembled.push_back(val & 0xff) : databytes.push_back(val & 0xff);
		}
		else {
			g.datatype = "word";
			val = getGlobalValue(arg2);
			g.value.push_back(val);
			isCode ? assembled.push_back(val >> 8) : databytes.push_back(val >> 8);
			isCode ? assembled.push_back(val & 0xff) : databytes.push_back(val & 0xff);
		}
		if (CheckandUpdateOffset(g.name, g.offset) == 0)
			gGlobalLabels.push_back(g);
	}
	else if (token == ".equal") {
		string buff[2];
		Split(line, ",", buff);
		GLOBALS g;
		g.datatype = "word";
		g.name = buff[1];
		val = stoi(buff[2]);
		g.value.push_back(val);
		g.type = isCode ? TYPE_CODE : TYPE_DATA;
		g.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
		if (CheckandUpdateOffset(g.name, g.offset) == 0)
			gGlobalLabels.push_back(g);
		isCode ? assembled.push_back(val >> 8) : databytes.push_back(val >> 8);
		isCode ? assembled.push_back(val & 0xff) : databytes.push_back(val & 0xff);
	}
	else if (token == ".seq") {
		string buff[64];
		int nsize = Split(line, ",", buff);
		for (int i = 0; i < nsize; i++) {
			GLOBALS g;
			g.datatype = "word";
			g.name = buff[i];
			g.type = isCode ? TYPE_CODE : TYPE_DATA;
			g.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
			if (CheckandUpdateOffset(g.name, g.offset) == 0)
				gGlobalLabels.push_back(g);
			g.value.push_back(i);
			isCode ? assembled.push_back(i) : databytes.push_back(i);
		}
	}
	else if (token == ".fstr") {
		string buff[2];
		Split(line, ",", buff);
		GLOBALS g;
		g.datatype = "fstr";
		g.name = buff[0];
		g.type = isCode ? TYPE_CODE : TYPE_DATA;
		g.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
		char outbuff[MAXCHARS] = { 0 };
		int nsize = ZStringEncode(buff[1].c_str(), outbuff);
		for (i = 0; i < nsize; i += 2) {
			val = *(unsigned short*)(outbuff + i);
			g.value.push_back(val);
			isCode ? assembled.push_back(outbuff[i]) : databytes.push_back(outbuff[i]);
			isCode ? assembled.push_back(outbuff[i + 1]) : databytes.push_back(outbuff[i + 1]);
		}
		if (CheckandUpdateOffset(g.name, g.offset) == 0)
			gGlobalLabels.push_back(g);
	}
	else if (token == ".funct") {
		string buff[16];
		int i;
		int nsize = Split(line, ",", buff);
		string funcmain;
		if (nsize == 0)
			funcmain = line;
		else
			funcmain = buff[0];
		string buff1[4];
		Split(funcmain, ":", buff1);
		GLOBALS g;
		g.datatype = "function";
		g.name = buff1[0];
		g.offset = ProgBase + assembled.size();
		g.type = TYPE_CODE;

		if (CheckandUpdateOffset(g.name, g.offset) == 0)
			gGlobalLabels.push_back(g);
		gLocalVariables.clear();
		if (nsize != 0) {
			for (i = 1; i < nsize; i++) {
				LOCALVARIABLE lv;
				lv.name = buff[i];
				lv.localnum = i;
				gLocalVariables.push_back(lv);
			}
		}
		assembled.push_back(nsize);
	}
	else if (token == "printi" || token == "printr")
		print_function(line);
	else if (token == "jump")
		jump_function(line);
	else{
		string name;
		string type;
		string rtype;
		char buff[100] = { 0 };
		int opcode, branchType = 0, cmdsize;

		for (j = 0; j < gOpcodes.size(); j++) {
			name = gOpcodes.at(j).instname;
			if (name != token)
				continue;
			type = gOpcodes.at(j).insttype;
			rtype = gOpcodes.at(j).resulttype;
			opcode = gOpcodes.at(j).opcode;
			ProcessFuncPtr pFunc = gOpcodes.at(j).func;
			branchType = (name.find("!") == string::npos) ? false : true;
			cmdsize = pFunc(line, type, opcode, rtype, buff, branchType);	//#first call assembly, Here we can get correct function and label address
			break;
		}
	}
	return nRet;
}
void ReadLinebyLine(ifstream *fp) {
	string line;
	int pos;
	string token, token1;
	while (!fp->eof())  // EOF is false here
	{
		getline(*fp, line);
		trim(line);
		if (line.empty())
			continue;			//This is empty line.
		if (line.at(0) == COMMENTCHAR)
			continue;			//This is comment line
		line = str_replace(line, '\t', ' ');

		pos = line.find(DELIMITER);			//Find separator fist
		token = line.substr(0, pos);
		transform(token.begin(), token.end(), token.begin(), ::tolower);
		if (token == ".insert")	//if .insert comment 
		{
			line.erase(0, pos + 1);
			ifstream infile(line);
			if (infile.is_open() == false) {
				cout << "File not found: " << line << endl;
				exit(0);
			}
			ReadLinebyLine(&infile);
		}
		else
		{
			pos = line.find(COMMENTCHAR);
			if (pos != string::npos)
				line.erase(pos, line.length() - pos);
			gInstructions.push_back(line);
		}
	}

}
int checkStartLabel() {
	int j;
	for (int i = 0; i < gGlobalLabels.size(); i++){
		GLOBALS st = gGlobalLabels.at(i);
		if (st.name == "%start") {
			string funcname = st.datatype;
			for (j = 0; j < gGlobalLabels.size(); j++) {
				GLOBALS st1 = gGlobalLabels.at(j);
				if (st1.name == st.datatype) {
					return st1.offset;
				}
			}
		}
		if (st.name == "start")
			return st.offset;
	}
	return 0;
}
unsigned short ToBigEndian(unsigned short val) {
	unsigned short byte1 = val >> 8;
	unsigned short byte2 = val & 0xFF;
	unsigned short v = byte2 << 8 | byte1;
	return v;
}
void WriteToFile(string outfile) {
	firstAssemble = 0;
	databytes.clear();
	assembled.clear();
	RemoveSegmentFlag();
	Assembly();
	int startPosition = checkStartLabel();
	if (startPosition == 0) {
		cout << "No start label" << endl;
		exit(1);
	}
	startPosition = startPosition / ALIGNMENT;
	int ns = sizeof(ZHEADER);
	ZHEADER zh = { 0 };
	FILE *fp = fopen(outfile.c_str(), "wb");
	zh.ver = 6;
	zh.highMemBase = ToBigEndian(0xFFF0);
	zh.entry = ToBigEndian(0xfff0 / 4);
	zh.staticMemBase = ToBigEndian(DataBase);
	zh.filelen = ToBigEndian((0xfff0 + assembled.size()) / 8);
	zh.routineoffset = 0;
	zh.staticstringoffset = 0;

	fwrite(&zh, sizeof(ZHEADER), 1, fp);

	int i;
	int heapval = 0xbff0 - databytes.size();
	char zchar = 0;
	for (i = sizeof(ZHEADER); i < DataBase; i++){
		fwrite(&zchar, 1, 1, fp);
	}
	for (i = 0; i < databytes.size(); i++){
		char ch = databytes.at(i);
		fwrite(&ch, 1, 1, fp);
	}
	for (i = 0; i < heapval; i++){
		fwrite(&zchar, 1, 1, fp);
	}

	for (i = 0; i < assembled.size(); i++){
		zchar = assembled.at(i);
		fwrite(&zchar, 1, 1, fp);
	}
	fclose(fp);
}
int main(int argc, char** argv) {
	printf("My Zasm version 0.1\n");

	/*if (argc != 2) {
	printf("Usage: zasm <file.z>\n");
	exit(0);
	}*/
	string inname = "hello.zasm";//argv[1];
	ifstream infile(inname);
	if (infile.is_open() == false) {
		printf("File not exist.\n");
		exit(0);
	}

	initOpFunctions();						//Init Opcodes
	ReadLinebyLine(&infile);				//Read file 
	firstAssemble = 1;
	Assembly();								//Get Assembly
	WriteToFile(inname + ".z6");			//Write assembly to file
	infile.close();							//close input file.
	return 0;
}