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

int ProgBase = 0x1000;					//High memory base
int DataBase = 0x40;					//Dynamic and static memory
int firstAssemble = 0;					//first pass or second pass assembly
////////////////////////////////////////////////////////////
// Declare structures
/////////////////////////////////////////////////////////////
typedef int(*ProcessFuncPtr)(string args, string type, int opcode, string flags, int linenum, bool bBranchType, bool xcall);

typedef struct INSTRUCTION_INFO {
	string instname;				//Instruction name
	string insttype;				//Instruction type
	int opcode;						//opcode
	string resulttype;				//result type
	ProcessFuncPtr func;			//process function
} STINSTRUCTION, *LPSTINSTRUCTION;

typedef struct ZHEADER {
	char ver;				//0
	short Flags;			//1
	char releasenumber;				//3
	short highMemBase;		//4
	short entry;			//6
	short loc_dictionary;	//8
	short loc_objtable;		//A	
	short loc_global;		//C
	short staticMemBase;	//E
	char v10;				//10 Flag1
	char v11;
	char serial[6];
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
	short FIRST;		//First sibling
	short NEXT;			//Siblings	
	short proptable;	//prop table address
	int offset;
	int objnum;			//object number 1~63
} stZObject;

struct TABLEITEM{		//Table's item
	string type;
	vector<short> data;
	int offset;
};
struct TABLE {
	string name;				//Tables
	int maxsize;
	vector<TABLEITEM> items;
	int offset;
};
struct GLOBALS {				//Global label's
	string datatype;
	string name;
	vector<unsigned short> value;
	int type;
	int offset;
};
struct GLOBALVARIABLE{				//Global variables
	string name;
	vector<unsigned short> value;
	char num;
	int offset;
};
struct LOCALVARIABLE {			//Local variables
	string name;
	char localnum;
};
struct LOCALLABEL {				//
	string name;
	string functionname;
	int offset;
};
#define OPCODE_COUNT	113
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
vector<GLOBALS> gGlobalLabels;			//global label
vector<TABLE> gTables;					//tables
vector<ZOBJECT> gObjects;				//objects
vector<GLOBALVARIABLE> gVariables;		//global variables
vector<LOCALVARIABLE> gLocalVariables;	//local variables
vector<LOCALLABEL> gLocalLabels;		//local labels
int zversion = 6;					//default version
string serialnum = "";				//serial number
unsigned short releasenum = 1;		//program's release number
int global_num = 16;				//Global variable number
int object_num = 1;					//Global object number;
string gfuncname;					//Current function name

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
	{ "ashift", "ext", 259, "result" },
	{ "bufout", "ext", 242, "" },
	{ "icall1", "1op", 143, "" },
	{ "call1", "1op", 136, "result" },
	{ "icall2", "2op", 26, "" },
	{ "call2", "2op", 25, "result" },
	{ "xcall", "ext", 236, "result" },
	{ "icall", "ext", 249, "" },
	{ "ixcall", "ext", 250, "" },
	{ "call", "ext", 224, "result" },
	{ "assigned?", "ext", 255, "branch" },
	{ "fclear", "2op", 12, "" },
	{ "copyt", "ext", 253, "" },
	{ "dec", "1op", 134, "" },
	{ "dless?", "2op", 4, "branch" },
	{ "div", "2op", 23, "result" },
	{ "display", "ext", 261, "" },
	{ "zwstr", "ext", 252, "" },
	{ "clear", "ext", 237, "" },
	{ "erase", "ext", 238, "" },
	{ "dclear", "ext", 263, "" },
	{ "first?", "1op", 130, "result.branch" },
	{ "curget", "ext", 240, "" },
	{ "nextp", "2op", 19, "result" },
	{ "loc", "1op", 131, "result" },
	{ "getp", "2op", 17, "result" },
	{ "getpt", "2op", 18, "result" },
	{ "ptsize", "1op", 132, "result" },
	{ "next?", "1op", 129, "result.branch" },
	{ "winget", "ext", 275, "result" },
	{ "inc", "1op", 133, "" },
	{ "igrtr?", "2op", 5, "branch" },
	{ "dirin", "ext", 244, "" },
	{ "move", "2op", 14, "" },
	{ "equal?", "ext", 193, "branch" },
	{ "grtr?", "2op", 3, "branch" },
	{ "in?", "2op", 6, "branch" },
	{ "less?", "2op", 2, "branch" },
	{ "zero?", "1op", 128, "branch" },
	{ "value", "1op", 142, "result" },
	{ "bcom", "1op", 248, "result" },
	{ "getb", "2op", 16, "result" },
	{ "get", "2op", 15, "result" },
	{ "shift", "ext", 258, "result" },
	{ "menu", "ext", 283, "branch" },
	{ "mod", "2op", 24, "result" },
	{ "mouse-limit", "ext", 279, "" },
	{ "winpos", "ext", 272, "" },
	{ "mul", "2op", 22, "result" },
	{ "not", "ext", 248, "result" },
	{ "bor", "2op", 8, "result" },
	{ "dirout", "ext", 243, "" },
	{ "picinf", "ext", 262, "branch" },
	{ "check_unicode", "ext", 268, "" },
	{ "picture_table", "ext", 284, "" },
	{ "fstack", "ext", 277, "" },
	{ "printb", "1op", 135, "" },
	{ "printc", "ext", 229, "" },
	{ "printf", "ext", 282, "" },
	{ "printn", "ext", 230, "" },
	{ "print", "1op", 141, "" },
	{ "printt", "ext", 254, "" },
	{ "print_unicode", "ext", 267, "" },
	{ "pop", "ext", 233, "" },
	{ "push", "ext", 232, "" },
	{ "xpush", "ext", 280, "branch" },
	{ "putp", "ext", 227, "" },
	{ "winput", "ext", 281, "" },
	{ "random", "ext", 231, "result" },
	{ "read", "ext", 228, "result" },
	{ "input", "ext", 246, "result" },
	{ "mouse-info", "ext", 278, "" },
	{ "remove", "1op", 137, "" },
	{ "restore", "ext", 257, "result" },
	{ "irestore", "ext", 266, "result" },
	{ "return", "1op", 139, "" },
	{ "save", "ext", 256, "result" },
	{ "isave", "ext", 265, "result" },
	{ "intbl?", "ext", 247, "result.branch" },
	{ "scroll", "ext", 276, "" },
	{ "fset", "2op", 11, "" },
	{ "color", "2op", 27, "" },
	{ "curset", "ext", 239, "" },
	{ "font", "ext", 260, "result" },
	{ "margin", "ext", 264, "" },
	{ "hlight", "ext", 241, "" },
	{ "window", "ext", 235, "" },
	{ "sound", "ext", 245, "" },
	{ "split", "ext", 234, "" },
	{ "set", "2op", 13, "" },
	{ "putb", "ext", 226, "" },
	{ "put", "ext", 225, "" },
	{ "sub", "2op", 21, "result" },
	{ "btst", "2op", 7, "branch" },
	{ "fset?", "2op", 10, "branch" },
	{ "throw", "2op", 28, "" },
	{ "lex", "ext", 251, "" },
	{ "winsize", "ext", 273, "" },
	{ "winattr", "ext", 274, "" }
};

///////////////////////
//Special purpose function declaration
////////////////////////
void jump_function(string name);		//jump opcodes functions
void print_function(string arg);		//printi, printf functions
char GetOperandType(string arg);		//Get Operand type.
char get2opArgment(string arg);			//Get opcode 2 argument
char Get2_ExtArgType(string arg);		//opcode2_ext function's argument type
bool CheckandUpdateOffset(GLOBALS *g);	//update offset(if is exist only update offset)
bool CheckandUpdateGVariable(string name, int offset);
bool CheckandUpdateObj(string name, int offset);
bool CheckandUpdateTable(string name, int offset);
unsigned short GetOperandValues(string arg, unsigned char *valsize);	//Get Operand value
/////////////////////////////////////
// String functions
/////////////////////////////////////
string & ltrim(string & str);
string & rtrim(string & str);
string trim(string &s);
string & makebackslash(string &s);
void ZcharEncode(char chin, vector<char>*zcharstr);
int ZStringEncode(const char *pin, char*pout);
bool is_number(string s);
string strlower(string arg);
string strupper(string arg);
string str_replace(string arg1, char arg2, char arg3);
string str_replace(string arg1, string arg2, string arg3);
int Split(string arg, string delimiter, string *res);
///////////////////////////////////////////////////////////////
// Main function declr
//////////////////////////////////////////////////////////////
void WriteToFile(string outfile);					//Write assembly to file
void ReadLinebyLine(ifstream *fp);					//Read file by line
void Assembly();									//Assembly file
void initOpFunctions();								//Init opcodes
void jump_to(string name, vector<char> *opcodes, bool mode);	//jump functions process
int lineProcess(string token, string line, int linenum);
int GetVariableNumber(string arg);
unsigned short getOffset(string arg, unsigned char*p);
string getFileName(string org);
void makeSerial(char*p);
//////////////////////////////////////////////////////////////
// Special function implementation
////////////////////////////////////////////////////////////
/*
parameter : name
This is jump label, for example jump /jumppos
jump /jumppos
...
...
jumppos:
PRINTI "This is jump position"
This function generate opcode for jump.
*/
void jump_function(string name) {
	assembled.push_back(140);				//jump opcode is 140
	int pos = ProgBase + assembled.size();  //current cmd position
	unsigned short temp;					//temperate varialbe
	unsigned char ch;
	temp = getOffset(name, &ch);				//get jump offset
	if (temp) {
		temp = (temp - pos) & 0xFFFF;		//convert new position to byte array and add it.
		assembled.push_back(temp >> 8);		//top byte
		assembled.push_back(temp & 0xFF);	//low byte
	}
	if (temp == 0) {
		if (firstAssemble == 1) {			//if first pass assemble, offset is 0
			assembled.push_back(0);
			assembled.push_back(0);
		}
		else {
			cout << "Invalid jmp label:" << name << endl;	//if second pass, and not exist, it will be error;
			exit(1);
		}
	}
	return;
}
/*
parameter : string arg
This is string to display;

PRINTI "This is a string."

Strins have to start " and end ".

*/
void print_function(string arg) {
	if (arg.at(0) != '"') {
		cout << "Invalid print string" << arg << endl;	//Check string
		return;
	}
	if (arg.back() != '"') {
		cout << "Invalid print string" << arg << endl;	//Check string.
		return;
	}
	arg.erase(0, 1);					//Erase first and last character "
	arg.erase(arg.length() - 1, 1);
	arg = str_replace(arg, """", "\""); // "" is replaced \"
	
	char chbuff[MAXCHARS] = { 0 };
	int i, nsize;
	nsize = ZStringEncode(arg.c_str(), chbuff);	//encode string to zscii
	assembled.push_back(178);					//printi opcode;
	for (i = 0; i < nsize; i++) {
		assembled.push_back(chbuff[i]);			//push opcodes
	}
	return;
}
/*
parameter : string name
int offset
return value : bool
whether name exist or not.

when first assemble, all offset is zero
and second assemble, we have to update only offset.
*/
bool CheckandUpdateOffset(GLOBALS *g) {
	size_t i, j;
	bool bex = 0;
	for (i = 0; i < gGlobalLabels.size(); i++) {	//Get Labels size and loop
		GLOBALS st = gGlobalLabels.at(i);
		if (strlower(st.name) == strlower(g->name)) {		//Check label name
			//if (st.offset == 0)							//if offset is 0
			gGlobalLabels.at(i).offset = g->offset;	//update offset
			gGlobalLabels.at(i).datatype = g->datatype;	//update offset
			gGlobalLabels.at(i).value.clear();
			for (j = 0; j < g->value.size(); j++)
				gGlobalLabels.at(i).value.push_back(g->value.at(j));

			bex = 1;									//update flag
			break;
		}
	}
	return bex;
}
/*
parameter : string name			: local label name
string funcname		: function's name
int offset			: label offset
return value : bool
whether name exist or not.

when first assemble, all offset is zero
and second assemble, we have to update only offset.
*/
bool CheckandUpdateLocal(string name, string funcname, int offset) {
	size_t i;
	bool bex = 0;
	for (i = 0; i < gLocalLabels.size(); i++) {		//check label loop
		LOCALLABEL st = gLocalLabels.at(i);
		if (strlower(st.name) == strlower(name) && strlower(st.functionname) == strlower(funcname)) {
			if (st.offset == 0)
				gLocalLabels.at(i).offset = offset;	//update offset
			bex = 1;
			break;
		}
	}
	return bex;
}
/*
parameter :			string name			: table name
int offset			: label offset
return value : bool
whether name exist or not.

when first assemble, all offset is zero
and second assemble, we have to update only offset.
*/
bool CheckandUpdateTable(string name, int offset) {
	size_t i;
	bool bex = 0;
	for (i = 0; i < gTables.size(); i++) {
		TABLE st = gTables.at(i);
		if (strlower(st.name) == strlower(name)) {		//table name compare
			if (st.offset == 0)
				gTables.at(i).offset = offset;			//update offset
			bex = 1;
			break;
		}
	}
	return bex;
}
/*
parameter :				string name			: object name
int offset			: label offset
return value : bool
whether name exist or not.

when first assemble, all offset is zero
and second assemble, we have to update only offset.
*/
bool CheckandUpdateObj(string name, int offset) {
	size_t i;
	bool bex = 0;
	for (i = 0; i < gObjects.size(); i++) {
		ZOBJECT st = gObjects.at(i);
		if (strlower(st.objname) == strlower(name)) {	//check name
			if (st.offset == 0)
				gObjects.at(i).offset = offset;	//update offset
			bex = 1;
			break;
		}
	}
	return bex;
}
/*
parameter :			string name			: global variable name
int offset			: label offset
return value : bool
whether name exist or not.

when first assemble, all offset is zero
and second assemble, we have to update only offset.
*/
bool CheckandUpdateGVariable(string name, int offset) {
	size_t i;
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
/*
parameter :			string arg			: string name of variable

return value : int
if assigned variable return 1~255
else return 0
First remove prefixs like ' and >
return get variable number
*/
int GetVariableNumber(string arg) {
	arg = str_replace(arg, "'", "");			//remove '
	arg = str_replace(arg, ">", "");			//remove >
	int i, pos = gLocalVariables.size();				//check local variable first
	for (i = 0; i < pos; i++)
	{
		LOCALVARIABLE lv = gLocalVariables.at(i);
		if (trim(lv.name) == trim(arg))			//compare name
			return lv.localnum;					//return variable number
	}
	pos = gVariables.size();					//global variables list.
	for (i = 0; i < pos; i++)
	{
		GLOBALVARIABLE gv = gVariables.at(i);	//check with global variable name
		if (trim(gv.name) == trim(arg))
			return gv.num;						//return variable number
	}
	return 0;								//can not found, then return 0
}
/*
parameter :			string arg			: string name of variable

return value : char 0~3
00 : long imm or offset
01 : imm
10 : variable
11 : undefined

Check arg is variable first. then return 2 : 10
and check ', If ' then that is imm value.
if arg is number and >=0 and < 256 then return 01;
if arg is object name then return imm
and return 0 : long imm
*/
char GetOperandType(string arg) {
	size_t i;
	arg = trim(arg);
	int gv = GetVariableNumber(arg);	//Is variable
	if (arg.find("'") != string::npos)	//if set 'x, 10 then x is not variable x is imm type
		return 1;					//01 is imm type
	if (gv != 0)					//This is variable
		return 2;					//10 is variable
	if (is_number(arg))				//is imm?
	{
		if (stoi(arg) < 256 && stoi(arg) >= 0)	//if value >=
			return 1; //01
	}
	for (i = 0; i < gObjects.size(); i++) {			//check arg is object name
		ZOBJECT st = gObjects.at(i);
		if (strlower(st.objname) == strlower(arg))
			return 1;					//return 01
	}
	return 0;// This operand is long imm or offset varialbe
}
/*
parameter :			string arg			: string name of variable

return value :	char 0~3
00 : long imm
01 : imm
10 : variable
11 : no more operand

This function check arg2 and ext argument count and type.
if arg is emptry string then no more operand(3) returns.

*/
char Get2_ExtArgType(string arg) {
	if (arg == "")						//if string is empty
		return 3;						//no more operand

	char ch = GetOperandType(arg);		//get operand type
	if (ch == 1)						//1 means immediate
		return 1;						//return 01 : immediate
	if (ch == 2)						//2 means variable
		return 2;						//return 2;
	if (is_number(arg)) {				//if arg is number and bigger than 256
		if (stoi(arg) >= 256)			//then that is long imm
			return 0;
	}
	unsigned char chtype;
	unsigned short temp = getOffset(arg, &chtype);	//get offset type
	if (chtype == 3)	//chtype 3 means imm
		return 1;			//return 1	//imm
	return 0;				//return 0 : long imm
}
/*
parameter :			string arg			: string name of variable
unsigned char *ptye : offset type
return value :	unsigned short(offset)

This function check local label's list first with name and function name
if exist then offset return;
if not exist then check global label with name
if global data's type is imm and
if not exist then check oject list.

*/
unsigned short getOffset(string arg, unsigned char*ptype) {
	size_t i;
	unsigned short temp = 0;
	unsigned char exist = 0;
	arg = trim(arg);
	for (i = 0; i < gLocalLabels.size(); i++) {
		LOCALLABEL st = gLocalLabels.at(i);
		if (strlower(st.name) == strlower(arg) && strlower(gfuncname) == strlower(st.functionname)) {
			temp = st.offset;			//check local labels list
			exist = 1;
			break;
		}
	}
	if (exist == 0)
	{
		for (i = 0; i < gGlobalLabels.size(); i++) {
			GLOBALS st = gGlobalLabels.at(i);
			if (strlower(st.name) == strlower(arg)) {
				if (strlower(st.datatype) == "imm") {		//check global labels
					temp = st.value.at(0);
					exist = 3;
				}
				else if (strlower(st.datatype) == "immfixed") {
					temp = (st.value.at(0) << 8);
					exist = 2;
				}
				else if (strlower(st.datatype) == "word") {
					temp = (st.value.at(0) << 8) | st.value.at(1);
					exist = 2;
				}
				else {
					temp = st.offset;
					exist = 2;
				}
				break;
			}
		}
	}
	if (exist == 0)
	{
		for (i = 0; i < gObjects.size(); i++) {
			ZOBJECT st = gObjects.at(i);
			if (strlower(st.objname) == strlower(arg)) {
				temp = st.objnum;		//check object list
				exist = 3;
				break;
			}
		}
	}
	*ptype = exist;
	return temp;
}
/*
parameter :			string arg			: string name of variable
unsigned char *valsize : databytes count
return value :	unsigned short(operand value)

This function get operand value.
remove ' and > first in argument
get operand type and value;
*/

unsigned short GetOperandValues(string arg, unsigned char *valsize) {
	arg = str_replace(arg, "'", "");		//remove '
	arg = str_replace(arg, ">", "");		//remove >
	arg = trim(arg);						//remove spaces
	char ch = GetOperandType(arg);			//get operand type
	unsigned short temp;
	if (ch == 2) {							//argument is varialbe
		short gn = GetVariableNumber(arg);		//get variable number
		temp = gn;
		valsize[0] = 1;							//data size is 1
	}
	else if (ch == 1 && is_number(arg)) {		//imm and arg is number then
		temp = stoi(arg);						//data size  = 1
		valsize[0] = 1;
	}
	else {
		if (is_number(arg)) {		//Long imm
			temp = stoi(arg);		//data size = 2
			valsize[0] = 2;
		}
		else {
			unsigned char ptype;					//offset
			
			temp = getOffset(arg, &ptype);				//get offset
			valsize[0] = (ptype == 3) ? 1 : 2;	//if ptype ==3(imm) or long imm
			if (temp == 0) {
				if (firstAssemble == 0) {					//check offset
					cout << "Error found:" << arg << endl;	//not exist in second assemble then error.
					exit(1);
				}
			}
		}
	}
	return temp;
}
/*
parameter :			string arg			: string name of variable

return value :	char
00 : immediate, immediate
01 : immediate, variable
10 : variable, immediate
11 : variable, variable
so it returns 0 or 1
0 : immediate
1 : variable
This function check arg is integer and >=0 and < 256 then return 0
check arg is variable then return 1
error value is -1(long imm or offset)
*/

char get2opArgment(string arg) {
	if (is_number(arg)) {			//check integer and 
		if (stoi(arg) >= 0 && stoi(arg) < 256)		//range
			return 0;				//return 0
	}
	int val = GetVariableNumber(arg);		//is variable
	if (val != 0)
		return 1;					//return 1;
	return -1;			//error returm
}
/////////////////////////////////////////////////////////
// string functions
////////////////////////////////////////////////////////
/*
parameter :			string &str

return value :	string &
removes left space " abcd"=>"abcd"

*/

string & ltrim(string & str)
{
	auto it2 = find_if(str.begin(), str.end(), [](char ch){ return !isspace<char>(ch, locale::classic()); });
	str.erase(str.begin(), it2);
	return str;
}
/*
parameter :			string &str

return value :	string &
removes right space "abcd "=>"abcd"

*/

string & rtrim(string & str)
{
	auto it1 = find_if(str.rbegin(), str.rend(), [](char ch){ return !isspace<char>(ch, locale::classic()); });
	str.erase(it1.base(), str.end());
	return str;
}
/*
parameter :			string &str

return value :	string &
removes left and right space " abcd		"=>"abcd"
*/

string trim(string &s) {
	ltrim(s);
	rtrim(s);
	return s;
}
/*
parameter :			string &s

return value :	string &
removes \\ to \
*/
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
	s = str_replace(s, "\"\"", "*?\\//*");
	s = str_replace(s, "\"", "");
	s = str_replace(s, "*?\\//*", "\"");
	return s;
}
/*
parameter :			char chin : input ascii char
vector<char> *zcharstr : zscii char array

return value :	void
convert character chin to zscii char
if chin is space then zscii code is 0
check a0, a1, a2 charset and find zscii value
*/
void ZcharEncode(char chin, vector<char>*zcharstr) {
	char a0[] = "abcdefghijklmnopqrstuvwxyz";		//a0 charset
	char a1[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";		//a1 charset
	char a2[] = "\n0123456789.,!?_#'\"/\\-:()";		//a2 charset
	if (chin == ' ') {					//if chin is space
		zcharstr->push_back(0);			//zscii code is 0
		return;
	}
	int i;
	for (i = 0; i < 26; i++) {			//check a0 charset
		if (a0[i] == chin) {			//if found
			zcharstr->push_back(i + 6);	//zscii code is 6 + i
			return;
		}
	}
	for (i = 0; i < 26; i++) {			//check a1 charset
		if (a1[i] == chin) {
			zcharstr->push_back(4);		//4 means a1
			zcharstr->push_back(i + 6);	//return zscii code
			return;
		}
	}
	for (i = 0; i < 26; i++) {			//check a2 charset
		if (a2[i] == chin) {
			zcharstr->push_back(5);		//5 means a2
			zcharstr->push_back(i + 7);	//return zscii
			return;
		}
	}
	zcharstr->push_back(5);				//not found then default codes
	zcharstr->push_back(6);
	zcharstr->push_back(chin >> 5);
	zcharstr->push_back(chin & 0x1F);
}
/*
parameter :			char *pin  (string to encode "This is... "
char *pout (outputbuffer, zscii code bytes 0x94, 0xA5 ,...)

return value :	int
Byte length of zscii codes

check input string is empty then return 0x94A5( and return 2(byte length) this is null zscii string)
convert every ascii character to zscii.
padding zscii code with 5 until mod 3=0
convert zscii code to byte array.
every zscii code is 5 bytes and 3 zscii codes are make 2 byte.
last 3 zscii code make also 2 byte but most significant byte is 1

*/
int ZStringEncode(const char *pin, char*pout) {
	size_t i, j = 0;
	string temp = pin;					//input string
	temp = makebackslash(temp);			//remove \\ to
	if (temp == "") {					//if input string is empty output buffer is 0x94a5
		pout[0] = 0x94;
		pout[1] = 0xA5;
		return 2;						//byte length is 2
	}
	vector<char> zcharstr;
	for (i = 0; i < temp.length(); i++) {	//calculate every ascii characters
		ZcharEncode(temp.at(i), &zcharstr);
	}
	while (zcharstr.size() < 0 || zcharstr.size() % 3 != 0)	//if zchar length mod 3 is not 0 then padding with 5
		zcharstr.push_back(5);
	unsigned short v;
	unsigned short ch1;
	unsigned short ch2;
	unsigned short ch3;
	size_t nsize = zcharstr.size();
	for (i = 0; i < nsize - 3; i += 3) {		//every 3 characters
		ch1 = zcharstr.at(i);					//character1
		ch2 = zcharstr.at(i + 1);				//character2
		ch3 = zcharstr.at(i + 2);				//character3
		v = (ch1 << 10) | (ch2 << 5) | ch3;		//make 2byte code
		pout[j] = v >> 8;						//fill output buffer
		pout[j + 1] = v & 0xff;
		j += 2;
	}
	ch1 = zcharstr.at(nsize - 3);				//last three zchar1
	ch2 = zcharstr.at(nsize - 2);				//char2
	ch3 = zcharstr.at(nsize - 1);				//char3
	v = 0x8000 | (ch1 << 10) | (ch2 << 5) | ch3;	//make 2 byte, 0x8000 means this is last byte
	pout[j] = v >> 8;						//fill output buffer
	pout[j + 1] = v & 0xff;
	return j + 2;
}
/*
parameter :			string s

return value :	bool
check input string is number or not.
check - sign first. -1 is also number
*/
bool is_number(string s)
{
	if (s == "")				//if empty then not number
		return false;
	if (s.at(0) == '-') {
		s = s.replace(0, 1, "");//remove - sign
	}
	std::string::const_iterator it = s.begin();
	while (it != s.end() && std::isdigit(*it)) ++it;
	return !s.empty() && it == s.end();
}
/*
parameter :			string arg

return value :	string
make string lower "AbdF"=>"abdf"
*/
string strlower(string arg)
{
	transform(arg.begin(), arg.end(), arg.begin(), ::tolower);
	return arg;
}
/*
parameter :			string arg

return value :	string
make string upper "AbdF"=>"ABDF"
*/
string strupper(string arg)
{
	transform(arg.begin(), arg.end(), arg.begin(), ::toupper);
	return arg;
}
/*
parameter :			string arg1,
char arg2
char arg3

return value :	string
replace arg2 in arg1 with arg3
example : str_replace("ABCD",'B','C')=>"ACCD"
*/
string str_replace(string arg1, char arg2, char arg3) {
	size_t i;
	for (i = 0; i < arg1.length(); i++)
	{
		if (arg1.at(i) == arg2) {
			arg1.replace(i, 1, 1, arg3);
		}
	}
	return arg1;
}
/*
parameter :			string arg1,
string arg2
string arg3

return value :	string
replace arg2 in arg1 with arg3
example : str_replace("ABCD","B","C")=>"ACCD"
*/
string str_replace(string arg1, string arg2, string arg3){
	int pos;
	int n = arg2.length();
	if (n == 0)
		return arg1;
	while ((pos = arg1.find(arg2)) != string::npos) {
		arg1.replace(pos, n, arg3);
	}
	return arg1;
}

/////////////////////////////////////////////////////////
// main functions
/////////////////////////////////////////////////////////
/*
parameter :			string arg (ARGUMENT STRING)
string type (0OP or 1op or 2op or ext)
int opcode (binary value of opcode)
string flags (result, branch, or result.branch)
int linenum (line number)
bool branchtype (branchtype : true or false, used very rarely)
bool xcall(this is only for xcall and ixcall)
return value : int
generated opcode's length

This function generate opcode from inputed string.
arg is only argument string, opcode string changed opcode integer.

*/
int MakeByteFromOpcode(string args, string type, int opcode, string flags, int linenum, bool bBranchType = false, bool xcall = false) {
	size_t pos, tokencount = 0, i;	//token count is argument count
	unsigned short temp;
	string token, arg = args;	//token means separate argument
	vector<string> optokens;	//optokens are argument list
	vector<char> opcodes;		//opcode is generated bytes
	string resultop;			//resultop is result argument
	string jump_name;			//jump_name is branch argument
	string buff[100];			//buff means argument array
	unsigned char valsize;
	int ccnt = Split(args, DELIMITER, buff);	//split args using space " "
	if (flags == "branch")		//if branch instrunction final argument is branch
	{
		jump_name = buff[ccnt];	//get branch arguemnt
	}
	if (flags == "result")		//if result instrunction final is result argument
	{
		resultop = buff[ccnt];	//get result argument
	}
	if (flags == "result.branch") {	//if result and branch instruction
		if (ccnt < 1) {
			cout << "line number : " << linenum << " argument count error." << endl;
			exit(1);
		}
		jump_name = buff[ccnt];		//get jump_name argument
		resultop = buff[ccnt - 1];	//get result argument name
	}
	arg = str_replace(args, jump_name, "");		//remove jump names from arg
	arg = str_replace(arg, resultop, "");		//remove result op from arg
	arg = trim(arg);							//remove spaces from arg
	jump_name = str_replace(jump_name, "/", "");		//jump name can start / or \\. we have to remove it
	jump_name = str_replace(jump_name, "\\", "");
	while ((pos = arg.find(",")) != string::npos) {	//split args into arguments list
		token = arg.substr(0, pos);
		token = trim(token);
		optokens.push_back(token);
		arg.erase(0, pos + 1);
	}
	if (arg != "")
		optokens.push_back(arg);					//get last arguments
	tokencount = optokens.size();					//argument length;

	if (type == "0op") {
		if (tokencount != 0) {
			cout << "Too many argument error:" << linenum << ":" << arg << endl;
			exit(1);
		}
		opcodes.push_back(0xB0 | opcode);		//0 opcode
	}
	else if (type == "1op") {
		if (tokencount != 1) {
			cout << "Too many argument error:" << linenum << ":" << arg << endl;
			exit(1);
		}

		string val = optokens.at(0);			//1 op argument count is 1
		val = trim(val);
		unsigned char oval = GetOperandType(val);//argcode 
		temp = 0x80 | (oval << 4) | opcode;		//get opcode
		opcodes.push_back(temp & 0xff);

		temp = GetOperandValues(val, &valsize);	//get operand value
		if (valsize == 2)						//value size is 2?
		{
			opcodes.push_back(temp >> 8);		//push 2 bytes
			opcodes.push_back(temp & 0xff);
		}
		else{
			opcodes.push_back(temp & 0xff);		//push 1byte
		}
	}
	else if (type == "2op") {
		if (tokencount != 2) {
			cout << "Too many argument error:" << linenum << ":" << arg << endl;	//
			exit(1);
		}
		string arg1 = optokens.at(0);			//first argument
		string arg2 = optokens.at(1);			//next argument
		arg1 = trim(arg1);
		arg2 = trim(arg2);
		if (get2opArgment(arg1) != -1 && get2opArgment(arg2) != -1) {		//if arguments are imm or variable
			temp = get2opArgment(arg1) << 6 | get2opArgment(arg2) << 5 | opcode;	//opcode
			opcodes.push_back(temp & 0xff);				//push opcode

			temp = GetOperandValues(arg1, &valsize);		//push argument value
			opcodes.push_back(temp & 0xff);
			temp = GetOperandValues(arg2, &valsize);		//push argument value
			opcodes.push_back(temp & 0xff);
		}
		else {											//if parameter has long imm or offest?
			temp = 0xc0 | opcode;						//we can make is using ext
			opcodes.push_back(temp & 0xff);				//opcode
			temp = Get2_ExtArgType(arg1) << 6 | Get2_ExtArgType(arg2) << 4 | 0xF; //opcodes are only 2 so we have to fill 0xF
			opcodes.push_back(temp & 0xff);				//push operand type
			temp = GetOperandValues(arg1, &valsize);	//get operand value and save
			if (valsize == 2)
			{
				opcodes.push_back(temp >> 8);			//push first byte
				opcodes.push_back(temp & 0xff);			//push next byte
			}
			else{
				opcodes.push_back(temp & 0xff);			//push databyte
			}
			temp = GetOperandValues(arg2, &valsize);	//get second operand bytes
			if (valsize == 2)
			{
				opcodes.push_back(temp >> 8);			//push first byte			
				opcodes.push_back(temp & 0xff);			//push next byte
			}
			else{
				opcodes.push_back(temp & 0xff);			//push byte
			}
		}
	}
	else if (type == "ext") {						//opcode type is ext
		if ((xcall == false && tokencount > 4) || (xcall == true && tokencount > 8)) {
			cout << "Too many argument error:" << linenum << ":" << arg << endl;
			exit(1);
		}
		string arg1 = optokens.size() > 0 ? optokens.at(0) : "";	//get first argument
		string arg2 = optokens.size() > 1 ? optokens.at(1) : "";	//get second argument
		string arg3 = optokens.size() > 2 ? optokens.at(2) : "";//get third argument
		string arg4 = optokens.size() > 3 ? optokens.at(3) : "";//get fourth argument
		string arg5, arg6, arg7, arg8;
		if (xcall == true) {						//if xcall, xcall can have 8 operand.
			arg5 = optokens.size() > 4 ? optokens.at(4) : ""; //get fifth argument
			arg6 = optokens.size() > 5 ? optokens.at(5) : "";//get sixth argument
			arg7 = optokens.size() > 6 ? optokens.at(6) : "";//get seventh argument
			arg8 = optokens.size() > 7 ? optokens.at(7) : "";//get eighth argument
			//remove spaces
			arg5 = trim(arg5);
			arg6 = trim(arg6);
			arg7 = trim(arg7);
			arg8 = trim(arg8);
		}
		//remove spaces
		arg1 = trim(arg1);
		arg2 = trim(arg2);
		arg3 = trim(arg3);
		arg4 = trim(arg4);

		if (opcode < 256) {				//if opcode < 256
			temp = 0xc0 | opcode;		//we have to or with 0xc0
			opcodes.push_back(temp);
		}
		else {
			opcodes.push_back(0xBE);	//if less then we have to push 190(0xbe) first and push next operand
			opcodes.push_back(opcode - 256);
		}
		unsigned char t1 = Get2_ExtArgType(arg1) << 6 | Get2_ExtArgType(arg2) << 4 | Get2_ExtArgType(arg3) << 2 | Get2_ExtArgType(arg4);
		unsigned char t2 = Get2_ExtArgType(arg5) << 6 | Get2_ExtArgType(arg6) << 4 | Get2_ExtArgType(arg7) << 2 | Get2_ExtArgType(arg8);
		//Get first and second operand type
		opcodes.push_back(t1);	//push first operand
		if (xcall == true)			//xcall have second operand type
			opcodes.push_back(t2);
		for (i = 0; i < optokens.size(); i++) {
			arg1 = optokens.at(i);
			temp = GetOperandValues(arg1, &valsize);	//get operand values
			if (valsize == 2)
			{
				opcodes.push_back(temp >> 8);
				opcodes.push_back(temp & 0xff);
			}
			else{
				opcodes.push_back(temp & 0xff);
			}
		}
	}
	if (flags == "result.branch")						//if result.branch
	{
		if (GetOperandType(resultop) != 2) {				//get operand type and value of result op
			cout << "Result argument error." << endl;
			exit(1);
		}
		temp = GetOperandValues(resultop, &valsize);	//add result op
		opcodes.push_back(temp & 0xff);

		jump_to(jump_name, &opcodes, bBranchType);		//add jump opcodes to
	}
	if (flags == "result") {							//if opcode type is result
		if (GetOperandType(resultop) != 2) {
			cout << "Result argument error." << endl;
			exit(1);
		}
		temp = GetOperandValues(resultop, &valsize);		//result ops' val
		opcodes.push_back(temp & 0xff);

	}
	if (flags == "branch") {							//if flag == "branch"
		jump_to(jump_name, &opcodes, bBranchType);		//make jump_name
	}
	for (pos = 0; pos < opcodes.size(); pos++) {		//push all generated opcodes to assembled array
		char ch = opcodes.at(pos);
		assembled.push_back(ch);
	}
	return opcodes.size();								//return opcode size
}
/*
parameter :			string name (ARGUMENT STRING)
vector<char*> opcodes type (0OP or 1op or 2op or ext)
bool modes (binary value of opcode)
return value : void
generated jump byte array and push it to opcodes

if jump name is rfalse or rtrue then push 0x8000, or 0x8001
check local and global labels to jump.

*/
void jump_to(string name, vector<char> *opcodes, bool mode = 0){
	size_t i;
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
	int pos = ProgBase + assembled.size() + opcodes->size();	//current position
	unsigned short temp;
	bool exist = 0;
	for (i = 0; i < gLocalLabels.size(); i++){			//check local labels';
		LOCALLABEL st = gLocalLabels.at(i);
		if (strlower(name) == strlower(st.name) && strlower(gfuncname) == strlower(st.functionname)) {
			if (mode == 0) {							//if branchmode == false
				temp = 0x8000 | (st.offset - pos) & 0x3FFF;	//temp calc
			}
			else {										//branch mode true
				temp = (st.offset - pos) & 0x3FFF;
			}
			opcodes->push_back(temp >> 8);				//push jump offset
			opcodes->push_back(temp & 0xFF);
			exist = 1;
			break;
		}
	}
	if (exist == 0) {									//can not found in local labels then
		for (i = 0; i < gGlobalLabels.size(); i++){		//check global labels.
			GLOBALS st = gGlobalLabels.at(i);
			if (strlower(name) == strlower(st.name)) {		//
				if (mode == 0) {
					temp = 0x8000 | (st.offset - pos) & 0x3FFF;
				}
				else {
					temp = (st.offset - pos) & 0x3FFF;
				}
				opcodes->push_back(temp >> 8);				//push jump offset
				opcodes->push_back(temp & 0xFF);
				exist = 1;
				break;
			}
		}
	}
	if (exist == 0) {					//if jump offset not ound
		if (firstAssemble == 1) {		//if first assemble it will be ok
			opcodes->push_back(0);		//push 0, 0
			opcodes->push_back(0);
		}
		else {
			cout << "Invalid jmp label:" << name << endl;
			exit(1);					//else invalid jump label
		}
	}
}
/*
parameter :
return value : void

generated opcode vector from codeTable
branch ! means branchtype = true
*/
void initOpFunctions() {
	for (int i = 0; i < OPCODE_COUNT; i++) {
		if (codeTable_YZIP[i].resulttype == "branch") {
			STINSTRUCTION stIns;
			stIns.instname = codeTable_YZIP[i].instname + "!";		//opecode name
			stIns.insttype = codeTable_YZIP[i].insttype;			//opcode type
			stIns.resulttype = codeTable_YZIP[i].resulttype;		//result type
			stIns.opcode = codeTable_YZIP[i].opcode;				//opcode
			stIns.func = MakeByteFromOpcode;					//process function
			gOpcodes.push_back(stIns);				//push array
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
/*
parameter :
return value : void

make assemble.

*/
void Assembly() {
	int i, pos;
	string token;
	int nsize = gInstructions.size();			//instruction array size
	for (i = 0; i < nsize; i++) {
		string line = gInstructions.at(i);		//instrunction one line

		pos = line.find(DELIMITER);				//find delimiter
		if (pos == -1) {
			token = line;						//if not found then all is opcode
			line = "";
		}
		else {
			token = line.substr(0, pos);		//divide into opcode and argument
			line.erase(0, pos + 1);
			trim(line);
		}
		token = strlower(token);				//if meet .end
		if (token == ".end")
			break;								//finish
		trim(token);
		pos = lineProcess(token, line, i);		//process one line
		i = (pos != 0) ? pos : i;				//
	}
	while (assembled.size() % ALIGNMENT)		//make code alignment
		assembled.push_back(0);
	while (databytes.size() % ALIGNMENT)
		databytes.push_back(0);
}
/*
parameter :
return value : boolean

This function check current position is code segment or not.
if we met endlod before then this is code section.
*/
bool IsCodeSegment() {
	size_t i;
	for (i = 0; i < gGlobalLabels.size(); i++) {
		GLOBALS g = gGlobalLabels.at(i);			//if endlod already parsed then after that is code segment's label.
		if (g.name == "endlod")
			return 1;
	}
	return 0;
}
/*
parameter :
return value : void

Erase endlod
This is for second pass
*/
void RemoveSegmentFlag() {
	size_t i;
	for (i = 0; i < gGlobalLabels.size(); i++) {
		GLOBALS g = gGlobalLabels.at(i);
		if (g.name == "endlod")
		{
			gGlobalLabels.erase(gGlobalLabels.begin() + i);
			break;
		}
	}
}
/*
parameter : string valname
return value : int

Get Current label's offset.
if label is object name then return object number;
*/
int getGlobalPointer(string valname, unsigned char &opsize) {
	size_t i;
	for (i = 0; i < gGlobalLabels.size(); i++) {
		GLOBALS g = gGlobalLabels.at(i);			//check labels
		if (strlower(g.name) == strlower(valname)) {
			if (g.datatype == "imm") {
				opsize = 1;
				return g.value.at(0);
			}
			else {
				opsize = 2;
				return g.offset;
			}
		}
	}
	for (i = 0; i < gObjects.size(); i++) {
		ZOBJECT g = gObjects.at(i);			//check objects
		if (strlower(g.objname) == strlower(valname)) {
			opsize = 1;
			return g.objnum;
		}
	}
	opsize = 2;
	return 0;
}

/*
parameter : string arg			(string to divide)
string delimiter
string *Res			result string array
return value : int (count)

Get Current label's offset.
if label is object name then return object number;
*/
int Split(string arg, string delimiter, string *res) {
	int pos, decnt = delimiter.length();		//delimiter; 
	string token;
	int i = 0;
	while ((pos = arg.find(delimiter)) != string::npos) {
		token = arg.substr(0, pos);			//get one argument
		res[i] = token;						//push it to array
		arg.erase(0, pos + decnt);
		i++;
	}
	if (arg != "") {
		res[i] = arg;
	}
	return i;							//return count
}
/*
parameter : string token (opcode)
string line	(argument)
int linenum (line number)
return value : int (generated opcode)

This function is main process function.
This function received opcode and argument string and it's linenumber and generate byte result.

All lines divided into 11 part
1 : Global data processing ::
2 : local labels processing:
3 : Global data processing include =
4 : equal
5 : seq
6 : fstr and gstr
7 : funct
8 : printi and printr
9 : jump
10 : usl(undefined)
11 : other opcodes
*/

int lineProcess(string token, string line, int linenum) {
P1:
	int pos = token.length() - 1, nRet = 0, nl;
	int i, j;
	unsigned short val;
	unsigned char opsize;
	bool isCode = IsCodeSegment();		//check current position is code segment
	if (token == ".new")				//.new means
		zversion = is_number(line) ? stoi(line) : 6;			//new zversion
	else if (token == "%serial::")
		serialnum = line;
	else if (token == "%zorkid::")
		releasenum = is_number(line) ? stoi(line) : 1;
	else if (token.find("::") != string::npos) {	//find ::
		if (token.substr(pos - 1) != "::") {			//if token not start with ::
			string retstr[10] = { 0 };
			if (Split(token, "::", retstr) > 2) {		//then it is error
				cout << "token error." << endl;
				exit(1);
			}
			trim(retstr[1]);
			line = retstr[1] + line;
		}
		if (line == "") {
			GLOBALS g;
			g.datatype = "statement";		// if argument area is empty then it is simliar as 
			g.name = str_replace(token, "::", "");	//only statement
			g.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
			g.type = isCode ? TYPE_CODE : TYPE_DATA;
			token = str_replace(token, "::", "");
			if (CheckandUpdateOffset(&g) == 0)
				gGlobalLabels.push_back(g);
		}
		else if ((pos = line.find(DELIMITER)) != string::npos) {
			string arg1 = line.substr(0, pos);
			string arg2 = line.erase(0, pos + 1);
			arg1 = strlower(arg1);
			GLOBALS g;
			if (arg1 == ".byte") {
				g.datatype = "imm";
				g.name = str_replace(token, "::", "");
				g.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
				g.type = isCode ? TYPE_CODE : TYPE_DATA;

				string buff[100];
				pos = Split(arg2, ",", buff);
				for (i = 0; i <= pos; i++) {
					if (is_number(buff[i]) == false) {
						cout << ".byte data have to be number";
						exit(1);
					}
					val = stoi(buff[i]);
					isCode ? assembled.push_back(val & 0xff) : databytes.push_back(val & 0xff);
					g.value.push_back(val);
					databytes.push_back(val & 0xFF);
				}
				if (CheckandUpdateOffset(&g) == 0)
					gGlobalLabels.push_back(g);
			}
			else if (arg1 == ".word") {
				g.datatype = "word";
				g.name = str_replace(token, "::", "");
				g.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
				g.type = isCode ? TYPE_CODE : TYPE_DATA;
				string buff[100];
				pos = Split(arg2, ",", buff);
				for (i = 0; i <= pos; i++) {
					if (is_number(buff[i]) == false) {
						cout << ".byte data have to be number";
						exit(1);
					}
					val = stoi(buff[i]);
					g.value.push_back(val);
					isCode ? assembled.push_back(val >> 8) : databytes.push_back(val >> 8);
					isCode ? assembled.push_back(val & 0xff) : databytes.push_back(val & 0xff);
				}
				if (CheckandUpdateOffset(&g) == 0)
					gGlobalLabels.push_back(g);
			}
			else if (arg1 == ".table") {
				TABLE t;
				t.name = str_replace(token, "::", "");
				t.maxsize = 0;
				t.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
				token = str_replace(token, "::", "");

				g.datatype = "table";
				g.name = str_replace(token, "::", "");
				g.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
				g.type = isCode ? TYPE_CODE : TYPE_DATA;

				if (CheckandUpdateOffset(&g) == 0)
					gGlobalLabels.push_back(g);

				nl = linenum + 1;
				while (1) {
					if (gInstructions.size() <= (size_t)nl)
						break;
					string newline = gInstructions.at(nl);
					trim(newline);
					newline = strupper(newline);
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
							val = arg2.length() == 0 ? 1 : arg2.length();
							item.data.push_back(arg2.length());
							item.type = "byte";
							item.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
							isCode ? assembled.push_back(val & 0xff) : databytes.push_back(val & 0xff);

							TABLEITEM item1;
							item1.type = "string";
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
							item1.type = "string";
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
							zobj.objnum = object_num;

							zobj.objname = buff[0];
							zobj.flag1 = stoi(buff[1]);
							zobj.flag2 = stoi(buff[2]);
							zobj.flag3 = stoi(buff[3]);
							zobj.LOC = is_number(buff[4]) ? stoi(buff[4]) : getGlobalPointer(buff[4], opsize);
							zobj.FIRST = is_number(buff[5]) ? stoi(buff[5]) : getGlobalPointer(buff[5], opsize);
							zobj.NEXT = is_number(buff[6]) ? stoi(buff[6]) : getGlobalPointer(buff[6], opsize);
							zobj.proptable = is_number(buff[7]) ? stoi(buff[7]) : getGlobalPointer(buff[7], opsize);
							zobj.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
							if (CheckandUpdateObj(zobj.objname, zobj.offset) == 0)
								gObjects.push_back(zobj);

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


							object_num++;
						}
						else if (arg1 == ".gvar") {
							string buff[2];
							Split(arg2, ",", buff);
							unsigned short val;
						p3:
							if (is_number(buff[1])) {
								val = stoi(buff[1]);
							}
							else {
								if ((pos = buff[1].find(",")) == -1)
									val = getGlobalPointer(buff[1], opsize);
								else {
									buff[1] = buff[1].substr(0, pos);
									goto p3;
								}
							}
							GLOBALVARIABLE gv;
							gv.name = buff[0];
							gv.value.push_back(val >> 8);
							gv.value.push_back(val & 0xFF);
							gv.num = global_num;
							gv.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
							if (CheckandUpdateGVariable(gv.name, gv.offset) == 0)
								gVariables.push_back(gv);

							global_num++;
							databytes.push_back(val >> 8);
							databytes.push_back(val & 0xFF);

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
								v3 = getGlobalPointer(arg4, opsize);
							}
							if (v2 == 1) {
								v1 = v3;			//1byte header 00
								v3 = 0;
							}
							else if (v2 == 2){
								v1 = (1 << 6) | v3;
								v3 = 0;
							}
							else {
								v1 = 2 << 6 | v3;
								v3 = 2 << 6 | v2;
							}
							item.type = "prop";
							item.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
							item.data.push_back(v1);
							t.items.push_back(item);
							isCode ? assembled.push_back(v1 & 0xff) : databytes.push_back(v1 & 0xff);
							if (v3 != 0) {
								item.data.push_back(v3);
								t.items.push_back(item);
								isCode ? assembled.push_back(v3 & 0xff) : databytes.push_back(v3 & 0xff);
							}
							nl = linenum + 1;
							while (1) {
								if (gInstructions.size() <= (size_t)nl)
									break;
								string newline = gInstructions.at(nl);
								trim(newline);
								if (newline == ".PROP") {
									nRet = nl - 1;
									break;
								}
								if (newline == ".ENDT") {
									nRet = nl;
									break;
								}
								if (is_number(newline)) {
									val = stoi(newline);
									isCode ? assembled.push_back(val >> 8) : databytes.push_back(val >> 8);
									isCode ? assembled.push_back(val & 0xff) : databytes.push_back(val & 0xff);
								}
								else if ((pos = newline.find(DELIMITER)) != string::npos) {
									string arg1 = newline.substr(0, pos);
									string arg2 = newline.erase(0, pos + 1);
									arg1 = strlower(arg1);
									if (arg1 == ".byte") {
										val = stoi(arg2);
										isCode ? assembled.push_back(val) : databytes.push_back(val);
									}
									else if (arg1 == ".word") {
										val = stoi(arg2);
										isCode ? assembled.push_back(val >> 8) : databytes.push_back(val >> 8);
										isCode ? assembled.push_back(val & 0xff) : databytes.push_back(val & 0xff);
									}
								}
								else {
									unsigned short v1 = getGlobalPointer(newline, opsize);
									isCode ? assembled.push_back(v1 >> 8) : databytes.push_back(v1 >> 8);
									isCode ? assembled.push_back(v1 & 0xff) : databytes.push_back(v1 & 0xff);
								}
								nl++;
							}
						}
					}
					else {
						unsigned short v1 = getGlobalPointer(newline, opsize);
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
				g.datatype = "imm";
				g.name = str_replace(token, "::", "");
				token = str_replace(token, "::", "");
				g.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
				g.type = isCode ? TYPE_CODE : TYPE_DATA;

				val = arg2.length() == 0 ? 1 : arg2.length();
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
				if (CheckandUpdateOffset(&g) == 0)
					gGlobalLabels.push_back(g);
				if (CheckandUpdateOffset(&g1) == 0)
					gGlobalLabels.push_back(g1);
			}
			else if (arg1 == ".str") {
				GLOBALS g1;
				g1.datatype = "str";
				arg1 = str_replace(token, "::", "");
				token = str_replace(token, "::", "");
				g1.name = trim(arg1);

				g1.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
				g1.type = isCode ? TYPE_CODE : TYPE_DATA;

				char outbuff[MAXCHARS] = { 0 };
				int nsize = ZStringEncode(arg2.c_str(), outbuff);
				for (i = 0; i < nsize; i += 2) {
					val = *(unsigned short*)(outbuff + i);
					g1.value.push_back(val);
					isCode ? assembled.push_back(outbuff[i]) : databytes.push_back(outbuff[i]);
					isCode ? assembled.push_back(outbuff[i + 1]) : databytes.push_back(outbuff[i + 1]);
				}
				if (CheckandUpdateOffset(&g1) == 0)
					gGlobalLabels.push_back(g1);
			}
			else if (arg1 == ".len") {
				GLOBALS g;
				g.datatype = "imm";
				g.name = str_replace(token, "::", "");
				g.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
				g.type = isCode ? TYPE_CODE : TYPE_DATA;

				val = arg2.length();
				g.value.push_back(val);
				isCode ? assembled.push_back(val & 0xff) : databytes.push_back(val & 0xff);
				token = str_replace(token, "::", "");
				if (CheckandUpdateOffset(&g) == 0)
					gGlobalLabels.push_back(g);
			}
			else if (arg1 == ".zword") {
				GLOBALS g;
				g.datatype = "str";
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
				token = str_replace(token, "::", "");
				if (CheckandUpdateOffset(&g) == 0)
					gGlobalLabels.push_back(g);
			}
			else if (arg1 == ".true") {
				GLOBALS g;
				g.datatype = "imm";
				g.name = str_replace(token, "::", "");
				g.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
				g.type = isCode ? TYPE_CODE : TYPE_DATA;
				g.value.push_back(1);
				isCode ? assembled.push_back(1) : databytes.push_back(1);
				token = str_replace(token, "::", "");
				if (CheckandUpdateOffset(&g) == 0)
					gGlobalLabels.push_back(g);
			}
			else if (arg1 == ".false") {
				GLOBALS g;
				g.datatype = "imm";
				g.name = str_replace(token, "::", "");
				g.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
				g.type = isCode ? TYPE_CODE : TYPE_DATA;
				g.value.push_back(0);
				isCode ? assembled.push_back(0) : databytes.push_back(0);
				token = str_replace(token, "::", "");
				if (CheckandUpdateOffset(&g) == 0)
					gGlobalLabels.push_back(g);
			}
			else if (arg1 == ".object") {
				string buff[8];
				Split(arg2, ",", buff);
				ZOBJECT zobj;
				zobj.objnum = object_num;

				zobj.objname = buff[0];
				zobj.flag1 = stoi(buff[1]);
				zobj.flag2 = stoi(buff[2]);
				zobj.flag3 = stoi(buff[3]);
				zobj.LOC = is_number(buff[4]) ? stoi(buff[4]) : getGlobalPointer(buff[4], opsize);
				zobj.FIRST = is_number(buff[5]) ? stoi(buff[5]) : getGlobalPointer(buff[5], opsize);
				zobj.NEXT = is_number(buff[6]) ? stoi(buff[6]) : getGlobalPointer(buff[6], opsize);
				zobj.proptable = is_number(buff[7]) ? stoi(buff[7]) : getGlobalPointer(buff[7], opsize);
				zobj.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
				if (CheckandUpdateObj(zobj.objname, zobj.offset) == 0)
					gObjects.push_back(zobj);

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


				object_num++;
			}
			else if (arg1 == ".gvar") {
				string buff[2];
				Split(arg2, ",", buff);
				unsigned short val;
			p4:
				if (is_number(buff[1])) {
					val = stoi(buff[1]);
				}
				else {
					if ((pos = buff[1].find(",")) == -1)
						val = getGlobalPointer(buff[1], opsize);
					else {
						buff[1] = buff[1].substr(0, pos);
						goto p4;
					}
				}
				GLOBALVARIABLE gv;
				gv.name = buff[0];
				gv.value.push_back(val);
				gv.num = global_num;
				gv.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
				if (CheckandUpdateGVariable(gv.name, g.offset) == 0)
					gVariables.push_back(gv);

				global_num++;
				isCode ? assembled.push_back(val >> 8) : databytes.push_back(val >> 8);
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
					v3 = getGlobalPointer(arg4, opsize);
				}
				v1 = (v2 - 1) << 5 | v3;
				g.value.push_back(v1);
				isCode ? assembled.push_back(v1 & 0xff) : databytes.push_back(v1 & 0xff);
				token = str_replace(token, "::", "");
				if (CheckandUpdateOffset(&g) == 0)
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
					if (CheckandUpdateOffset(&g) == 0)
						gGlobalLabels.push_back(g);
				}
				else if (strlower(line) == ".table") {
					TABLE t;
					t.name = str_replace(token, "::", "");
					t.maxsize = 0;
					t.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();

					g.datatype = "table";
					g.name = str_replace(token, "::", "");
					g.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
					g.type = isCode ? TYPE_CODE : TYPE_DATA;

					if (CheckandUpdateOffset(&g) == 0)
						gGlobalLabels.push_back(g);

					nl = linenum + 1;
					while (1) {
						if (gInstructions.size() <= (size_t)nl)
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
								val = arg2.length() == 0 ? 1 : arg2.length();
								item.data.push_back(arg2.length());
								item.type = "byte";
								item.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
								isCode ? assembled.push_back(val & 0xff) : databytes.push_back(val & 0xff);

								TABLEITEM item1;
								item1.type = "string";
								item1.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
								char outbuff[MAXCHARS] = { 0 };
								int nsize = ZStringEncode(arg2.c_str(), outbuff);
								for (i = 0; i < nsize; i += 2) {
									val = *(unsigned short*)(outbuff + i);
									item1.data.push_back(val);
									isCode ? assembled.push_back(outbuff[i]) : databytes.push_back(outbuff[i]);
									isCode ? assembled.push_back(outbuff[i + 1]) : databytes.push_back(outbuff[i + 1]);
								}
								t.items.push_back(item);
								t.items.push_back(item1);
							}
							else if (arg1 == ".str") {
								TABLEITEM item1;
								arg2 = str_replace(arg2, "\"", "");
								item1.type = "string";
								char outbuff[MAXCHARS] = { 0 };
								int nsize = ZStringEncode(arg2.c_str(), outbuff);
								item1.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
								for (i = 0; i < nsize; i += 2) {
									val = *(unsigned short*)(outbuff + i);
									item1.data.push_back(val);
									isCode ? assembled.push_back(outbuff[i]) : databytes.push_back(outbuff[i]);
									isCode ? assembled.push_back(outbuff[i + 1]) : databytes.push_back(outbuff[i + 1]);
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
								zobj.objnum = object_num;

								zobj.objname = buff[0];
								zobj.flag1 = stoi(buff[1]);
								zobj.flag2 = stoi(buff[2]);
								zobj.flag3 = stoi(buff[3]);
								zobj.LOC = is_number(buff[4]) ? stoi(buff[4]) : getGlobalPointer(buff[4], opsize);
								zobj.FIRST = is_number(buff[5]) ? stoi(buff[5]) : getGlobalPointer(buff[5], opsize);
								zobj.NEXT = is_number(buff[6]) ? stoi(buff[6]) : getGlobalPointer(buff[6], opsize);
								zobj.proptable = is_number(buff[7]) ? stoi(buff[7]) : getGlobalPointer(buff[7], opsize);
								zobj.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
								if (CheckandUpdateObj(zobj.objname, zobj.offset) == 0)
									gObjects.push_back(zobj);

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


								object_num++;
							}
							else if (arg1 == ".gvar") {
								string buff[2];
								Split(arg2, "=", buff);
								unsigned short val;
								unsigned char opsize = 1;
							p5:
								if (is_number(buff[1])) {
									val = stoi(buff[1]);
								}
								else {
									if ((pos = buff[1].find(",")) == -1)
										val = getGlobalPointer(buff[1], opsize);
									else {
										buff[1] = buff[1].substr(0, pos);
										goto p5;
									}
								}
								GLOBALVARIABLE gv;
								gv.name = buff[0];
								if (opsize == 2) {
									gv.value.push_back(val >> 8);
									gv.value.push_back(val & 0xff);
								}
								else {
									gv.value.push_back(val);
								}
								gv.num = global_num;
								gv.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
								if (CheckandUpdateGVariable(gv.name, gv.offset) == 0)
									gVariables.push_back(gv);

								global_num++;
								isCode ? assembled.push_back(val >> 8) : databytes.push_back(val >> 8);
								isCode ? assembled.push_back(val & 0xff) : databytes.push_back(val & 0xff);

							}
							else if (arg1 == ".prop") {
								TABLEITEM item;
								pos = arg2.find(",");
								string arg3 = arg2.substr(0, pos);
								string arg4 = arg2.erase(0, pos + 1);
								unsigned short v1, v2, v3;
								unsigned char opsize;
								if (is_number(arg4)) {
									v2 = stoi(arg3);
									v3 = stoi(arg4);
								}
								else {
									v2 = stoi(arg3);
									v3 = getGlobalPointer(arg4, opsize);
								}
								if (v2 == 1) {
									v1 = v3;			//1byte header 00
									v3 = 0;
								}
								else if (v2 == 2){
									v1 = (1 << 6) | v3;
									v3 = 0;
								}
								else {
									v1 = 2 << 6 | v3;
									v3 = 2 << 6 | v2;
								}
								item.type = "prop";
								item.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
								item.data.push_back(v1);
								t.items.push_back(item);
								isCode ? assembled.push_back(v1 & 0xff) : databytes.push_back(v1 & 0xff);
								if (v3 != 0) {
									item.data.push_back(v3);
									t.items.push_back(item);
									isCode ? assembled.push_back(v3 & 0xff) : databytes.push_back(v3 & 0xff);
								}
								int nl1 = nl + 1;
								while (1) {
									if (gInstructions.size() <= (size_t)nl1)
										break;
									string newline = gInstructions.at(nl1);
									trim(newline);
									if (newline.find(".PROP") != string::npos) {
										nl = nl1 - 1;
										break;
									}
									if (newline == ".ENDT") {
										nl = nl1 - 1;
										break;
									}
									if (is_number(newline)) {
										val = stoi(newline);
										isCode ? assembled.push_back(val >> 8) : databytes.push_back(val >> 8);
										isCode ? assembled.push_back(val & 0xff) : databytes.push_back(val & 0xff);
									}
									else if ((pos = newline.find(DELIMITER)) != string::npos) {
										string arg1 = newline.substr(0, pos);
										string arg2 = newline.erase(0, pos + 1);
										arg1 = strlower(arg1);

										if (arg1 == ".byte") {
											val = stoi(arg2);
											isCode ? assembled.push_back(val) : databytes.push_back(val);
										}
										else if (arg1 == ".word") {
											val = stoi(arg2);
											isCode ? assembled.push_back(val >> 8) : databytes.push_back(val >> 8);
											isCode ? assembled.push_back(val & 0xff) : databytes.push_back(val & 0xff);
										}
									}
									else {
										unsigned char opsize;
										unsigned short v1 = getGlobalPointer(newline, opsize);
										isCode ? assembled.push_back(v1 >> 8) : databytes.push_back(v1 >> 8);
										isCode ? assembled.push_back(v1 & 0xff) : databytes.push_back(v1 & 0xff);
									}
									nl1++;
								}
							}
						}
						else {
							unsigned char opsize;
							unsigned short v1 = getGlobalPointer(newline, opsize);
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

					if (CheckandUpdateOffset(&g) == 0)
						gGlobalLabels.push_back(g);
				}
			}
			else {
				GLOBALS g;
				g.datatype = "imm";
				g.name = str_replace(token, "::", "");
				g.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
				g.type = isCode ? TYPE_CODE : TYPE_DATA;
				val = stoi(line);
				g.value.push_back(val);

				if (CheckandUpdateOffset(&g) == 0)
					gGlobalLabels.push_back(g);
				isCode ? assembled.push_back(val >> 8) : databytes.push_back(val >> 8);
				isCode ? assembled.push_back(val & 0xff) : databytes.push_back(val & 0xff);
			}
		}
	}
	else if (token.at(pos) == ':') {
		//local simbol
		LOCALLABEL g;
		g.functionname = gfuncname;
		g.name = str_replace(token, ":", "");
		g.offset = ProgBase + assembled.size();
		if (CheckandUpdateLocal(g.name, gfuncname, g.offset) == 0)
			gLocalLabels.push_back(g);
		if (line != "") {
			pos = line.find(DELIMITER);
			token = strlower(line.substr(0, pos));
			line = line.erase(0, pos + 1);
			goto P1;
		}
	}
	else if ((pos = token.find('=')) != -1) {
		//This is local label;
		string arg1 = token.substr(0, pos);
		string arg2 = token.erase(0, pos + 1);
		string arg3 = "";
	p2:
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
			g.datatype = "imm";
			val = stoi(arg2);
			g.value.push_back(val);
			isCode ? assembled.push_back(val >> 8) : databytes.push_back(val >> 8);
			isCode ? assembled.push_back(val & 0xff) : databytes.push_back(val & 0xff);
		}
		else if ((pos = arg2.find(",")) != -1){
			arg2 = token.substr(0, pos);
			arg3 = token.erase(0, pos + 1);
			arg2 = trim(arg2);
			arg3 = trim(arg3);
			goto p2;
		}
		else {
			g.datatype = (arg3 == "") ? (line != "" ? line : "word") : arg3;
			unsigned char opsize;
			val = getGlobalPointer(arg2, opsize);
			if (opsize == 1) {
				g.datatype = "immfixed";
				g.value.push_back(val & 0xff);
				isCode ? assembled.push_back(val & 0xff) : databytes.push_back(val & 0xff);
			}
			else {
				g.value.push_back(val >> 8);
				g.value.push_back(val & 0xff);
				isCode ? assembled.push_back(val >> 8) : databytes.push_back(val >> 8);
				isCode ? assembled.push_back(val & 0xff) : databytes.push_back(val & 0xff);
			}
		}
		if (CheckandUpdateOffset(&g) == 0)
			gGlobalLabels.push_back(g);
	}
	else if (token == ".equal") {
		string buff[2];
		Split(line, ",", buff);
		GLOBALS g;
		g.datatype = "imm";
		g.name = buff[1];
		val = stoi(buff[2]);
		g.value.push_back(val);
		g.type = isCode ? TYPE_CODE : TYPE_DATA;
		g.offset = isCode ? ProgBase + assembled.size() : DataBase + databytes.size();
		if (CheckandUpdateOffset(&g) == 0)
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
			if (CheckandUpdateOffset(&g) == 0)
				gGlobalLabels.push_back(g);
			g.value.push_back(i);
			isCode ? assembled.push_back(i) : databytes.push_back(i);
		}
	}
	else if (token == ".fstr" || token == ".gstr") {
		pos = line.find(",");
		string name = line.substr(0, pos);
		line.erase(0, pos + 1);
		GLOBALS g;
		g.datatype = token;
		g.name = name;
		g.type = isCode ? TYPE_CODE : TYPE_DATA;
		while (databytes.size() % ALIGNMENT)
			databytes.push_back(0);
		g.offset = DataBase + databytes.size();
		g.offset /= ALIGNMENT;
		char outbuff[MAXCHARS] = { 0 };
		int nsize = ZStringEncode(line.c_str(), outbuff);
		for (i = 0; i < nsize; i += 2) {
			val = *(unsigned short*)(outbuff + i);
			g.value.push_back(val);
			isCode ? assembled.push_back(outbuff[i]) : databytes.push_back(outbuff[i]);
			isCode ? assembled.push_back(outbuff[i + 1]) : databytes.push_back(outbuff[i + 1]);
		}
		if (CheckandUpdateOffset(&g) == 0)
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
		while (assembled.size() % ALIGNMENT)
			assembled.push_back(0);
		g.offset = ProgBase + assembled.size();
		g.offset = g.offset / 4;
		g.type = TYPE_CODE;
		gfuncname = buff1[0];
		if (CheckandUpdateOffset(&g) == 0)
			gGlobalLabels.push_back(g);
		gLocalVariables.clear();
		if (nsize != 0) {
			for (i = 1; i <= nsize; i++) {
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
	else if (token == "usl") {
		cout << "Obsolete opcode : usl" << endl;
		exit(1);
	}
	else{
		string name;
		string type;
		string rtype;
		int opcode, cmdsize;
		bool xcall = 0, branchType = 0;
		for (j = 0; j < (int)(gOpcodes.size()); j++) {
			name = gOpcodes.at(j).instname;
			if (name != token)
				continue;
			xcall = false;
			if (name == "xcall" || name == "ixcall")
				xcall = true;

			type = gOpcodes.at(j).insttype;
			rtype = gOpcodes.at(j).resulttype;
			opcode = gOpcodes.at(j).opcode;
			ProcessFuncPtr pFunc = gOpcodes.at(j).func;
			branchType = (name.find("!") == string::npos) ? false : true;
			cmdsize = pFunc(line, type, opcode, rtype, linenum, branchType, xcall);	//#first call assembly, Here we can get correct function and label address
			break;
		}
	}
	return nRet;
}
/*
parameter : ifstream *fp (opcode)
return value : void

This function read src file line by line and store line to vector list.
*/
void ReadLinebyLine(ifstream *fp) {
	string line;
	int pos;
	string token, token1;
	while (!fp->eof())  // EOF is false here
	{
		getline(*fp, line);			//read line
		trim(line);					//remove space
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
			ifstream infile(line);		//read insert file
			if (infile.is_open() == false) {
				cout << "File not found: " << line << endl;
				exit(0);
			}
			ReadLinebyLine(&infile);
		}
		else
		{
			pos = line.find(COMMENTCHAR);		//remove text after comment char
			if (pos != string::npos)
				line.erase(pos, line.length() - pos);
			gInstructions.push_back(line);
		}
	}

}
/*
parameter :
return value : int

This function find object global position.
*/
int checkObjectLabel() {
	size_t i;
	for (i = 0; i < gGlobalLabels.size(); i++) {
		GLOBALS st = gGlobalLabels.at(i);
		if (st.name == "object") {
			return st.offset;
		}
	}
	return 0;
}
/*
parameter :
return value : int

This function impure tag position(impure means static base)
*/
int checkStaticBaseLabel() {
	size_t i;
	for (i = 0; i < gGlobalLabels.size(); i++) {
		GLOBALS st = gGlobalLabels.at(i);
		if (st.name == "impure") {
			return st.offset;
		}
	}
	return 0;
}
/*
parameter :
return value : int

This function find global variable position.
*/
int checkGlobalLabel() {
	size_t i;
	for (i = 0; i < gGlobalLabels.size(); i++) {
		GLOBALS st = gGlobalLabels.at(i);
		if (st.name == "global") {
			return st.offset;
		}
	}
	return 0;
}
/*
parameter :
return value : int

This function find start label position,
*/
int checkStartLabel() {
	size_t i, j;
	for (i = 0; i < gGlobalLabels.size(); i++){
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
/*
parameter : unsigned short val
return value : unsigned short

This function convert short to big endian.
0x1234 => buff[0] = 0x12, buff[1]=0x34
*/
unsigned short ToBigEndian(unsigned short val) {
	unsigned short byte1 = val >> 8;
	unsigned short byte2 = val & 0xFF;
	unsigned short v = byte2 << 8 | byte1;
	return v;
}
/*
parameter : string outfile (output file name)
return value : void

Write assembled data and code to file.
*/
void WriteToFile(string outfile) {
	size_t i;
	firstAssemble = 0;
	object_num = 1;
	global_num = 16;
	databytes.clear();
	assembled.clear();
	RemoveSegmentFlag();
	Assembly();
	int startPosition = checkStartLabel();
	if (startPosition == 0) {
		cout << "No start label" << endl;
		exit(1);
	}
	unsigned short globaladdr = checkGlobalLabel();			//Find global varaible postion
	unsigned short objectaddr = checkObjectLabel();			//find object table position
	unsigned short staticbase = checkStaticBaseLabel();		//find static base position
	startPosition = startPosition / ALIGNMENT;				//start position /= 4;
	int ns = sizeof(ZHEADER);
	int divider = (zversion > 5) ? 8 : 4;					//Divider

	while (assembled.size() % divider)
		assembled.push_back(0);

	ZHEADER zh = { 0 };
	FILE *fp = fopen(outfile.c_str(), "wb");
	zh.ver = zversion;
	zh.highMemBase = ToBigEndian(ProgBase);
	zh.entry = (zversion > 5) ? ToBigEndian(ProgBase / 4) : ToBigEndian(ProgBase + 1);
	zh.staticMemBase = ToBigEndian(staticbase);
	zh.filelen = ToBigEndian((ProgBase + assembled.size()) / divider);
	zh.routineoffset = 0;							//routine offset may be consider
	zh.staticstringoffset = 0;						//string offset maybe consider 
	zh.loc_global = ToBigEndian(globaladdr);		//Global table position
	zh.loc_objtable = ToBigEndian(objectaddr);		//Object table
	zh.releasenumber = releasenum;					//release number

	memset(zh.serial, 0x20, 6);
	i = (serialnum.length() > 6) ? 6 : serialnum.length();
	memcpy(zh.serial, serialnum.c_str(), i);
	fwrite(&zh, sizeof(ZHEADER), 1, fp);				//Write Z header 64 byte to file.


	int heapval = ProgBase - DataBase - databytes.size();
	char zchar = 0;
	for (i = sizeof(ZHEADER); i < (size_t)DataBase; i++){		//Write Zero bytes to data's base
		fwrite(&zchar, 1, 1, fp);						//database is 0x40 now, so no written operate
	}
	for (i = 0; i < databytes.size(); i++){				//Write databytes to files
		char ch = databytes.at(i);						//Write file one by one
		fwrite(&ch, 1, 1, fp);
	}
	for (i = 0; i < (size_t)heapval; i++){						//Write Zero to high membase
		fwrite(&zchar, 1, 1, fp);
	}
	for (i = 0; i < assembled.size(); i++){				//Write assembled files/
		zchar = assembled.at(i);
		fwrite(&zchar, 1, 1, fp);
	}
	fclose(fp);
}

/*
parameter : string org
return value : string

This function make file name from zversion,
*/
string getFileName(string org) {
	string s = org;
	size_t pos = 0;
	string deli = ".";
	size_t i;

	for (i = 0; i < gGlobalLabels.size(); i++) {
		GLOBALS gb = gGlobalLabels.at(i);
		if (gb.name == "%zversion") {
			zversion = gb.value.at(0);
			break;
		}
	}
	string ext = to_string(zversion);
	ext = ".z" + ext;

	std::string token;
	string path = "";
	pos = s.rfind(deli);
	if (pos == string::npos)
		return s + ext;
	s.erase(pos);
	return s + ext;
}
/*
parameter :
return value : int

main entry point
*/
int main(int argc, char** argv) {
	printf("My Zasm version 0.1\n");

	if (argc != 2) {
		printf("Usage: zasm <file.z>\n");
		exit(0);
	}
	string inname = argv[1];
	ifstream infile(inname);
	if (infile.is_open() == false) {
		printf("File not exist.\n");
		exit(0);
	}

	initOpFunctions();						//Init Opcodes
	ReadLinebyLine(&infile);				//Read file 
	firstAssemble = 1;
	Assembly();								//Get Assembly
	inname = getFileName(inname);
	WriteToFile(inname);			//Write assembly to file
	infile.close();							//close input file.
	return 0;
}
