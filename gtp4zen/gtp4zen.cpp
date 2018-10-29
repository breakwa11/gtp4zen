// gtp4zen.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include "compile_time.h"
#include "zen6gtp.h"
#include "zen7gtp.h"

using namespace boost::program_options;
static void _play_zen(CGtp *pgtp);

int g_zenver = 7;
int g_threads = 1;
int g_maxtime = 60;			// �룬Ĭ��60��
int g_strength = 10000;			// ������Ĭ��10000��
bool g_logfilenametime = false;
bool g_debug = false;
int g_think_interval = 200;
float g_think_level_1 = 1.0;
float g_think_level_2 = 1.0;
int g_think_level_0 = 1;
int g_resign = 10;

#ifdef _DEBUG
string g_logfile = "gtp4zen_log.txt";
#else
string g_logfile = "";
#endif

// ��ȡ��ǰ·��
inline std::string GetModuleFilePath()
{
	TCHAR buffer[MAX_PATH] = L"";
	GetModuleFileName(NULL, buffer, MAX_PATH);
	LPTSTR lpPos = _tcsrchr(buffer, _T('\\'));
	if (lpPos)
	{
		*(lpPos + 1) = _T('\0');
	}

	std::string result = CWtoA(buffer);
	return result;
	//return (LPSTR)CWtoA(buffer);
}

int _tmain(int argc, _TCHAR* argv[])
{
	SYSTEM_INFO  sysInfo;
	GetSystemInfo(&sysInfo);
	g_threads = sysInfo.dwNumberOfProcessors;

#if 1
	// �����н���(ʹ��boost)
	options_description opts("Options");
	opts.add_options()
		("help,h", "Show all allowed options.")
		("zenverion,z",value<int>(),    "Version of zen.dll, must be 6 or 7. (default 7)")
		("threads,t",  value<int>(),    "Set the number of threads to use. (default CPU_CORES)")
		("maxtime,T",  value<int>(),    "Set the max time for one move. (default 10 seconds)")
		("strength,s", value<int>(),    "Set the playing strength. (default 10000)")
		("ithink,i", value<int>(),      "thinking interval, only set 100 when play cgos. (default 100)")
		("ilevel0,n", value<int>(),     "factor0. (default 1)")
		("ilevel1,o", value<float>(),   "factor1. (default 1)")
		("ilevel2,p", value<float>(),   "factor2. (default 1)")
		("resign,r", value<int>(),      "resign. (default 10)")
		("logfile,l", value<string>(),  "Enable logging and set the log file. (default none)")
		("logfilenametime,L",           "Add timestamp after log filename. (default off)")
		("debug,d",                     "Enable debug output to gtp shell. (default off)")
		;
	/*
	ѡ�� -t -T -M -m -l -k -w -s -f
	-t �߳��� (Ĭ��1)
	-T ÿ��mc����ʱ�� (������ѡ��ʱ��, ����������, ��Ϊ�ᵼ���������ȶ�)
	-M ÿ����λģ�������� (�����ϸ��, ������������)
	-m ÿ����ѡ��ģ�������� (ʵ�ʰ�M,m����)
	-l ��¼�ļ� (Ĭ��Ϊ ����ʱ��-��λ�����.log)
	-k -w -s -f Ϊ������������ (Ĭ�� -k0 -w0.5 -s0.25,0.25 -f1.1)
	�����������Գ����޸ģ�������� -k-6 ���Ժã���������ȷ���ۡ�
	*/
	try {
		bool error_options = false;;
		variables_map vm;
		store(parse_command_line(argc, argv, opts), vm);
		if (vm.count("help")) {
			cout << opts << endl;
			return 0;
		}
		if (vm.count("logfilenametime")) {
			g_logfilenametime = true;
		}
		if (vm.count("debug")) {
			g_debug = true;
		}
		if (vm.count("zenverion")) {
			// zen.dll�汾
			g_zenver = vm["zenverion"].as<int>();
			if (6 != g_zenver && 7 != g_zenver) {
				error_options = true;
			}
		}
		if (vm.count("threads")) {
			g_threads = vm["threads"].as<int>();
			if (g_threads <= 0) {
				error_options = true;
			}
		}
		if (vm.count("maxtime")) {
			g_maxtime = vm["maxtime"].as<int>();
			if (g_maxtime <= 0) {
				error_options = true;
			}
		}
		if (vm.count("strength")) {
			g_strength = vm["strength"].as<int>();
			if (g_strength <= 0) {
				error_options = true;
			}
		}
		if (vm.count("ithink")) {
			g_think_interval = vm["ithink"].as<int>();
			if (g_think_interval <= 0) {
				g_think_interval = 200;
			}
		}
		if (vm.count("ilevel0")) {
			g_think_level_0 = vm["ilevel0"].as<int>();
		}
		if (vm.count("ilevel1")) {
			g_think_level_1 = vm["ilevel1"].as<float>();
		}
		if (vm.count("ilevel2")) {
			g_think_level_2 = vm["ilevel2"].as<float>();
		}
		if (vm.count("resign")) {
			g_resign = vm["resign"].as<int>();
		}
		if (vm.count("logfile")) {
			g_logfile = vm["logfile"].as<string>();
		}

		if (error_options) {
			cerr << "ERROR: unknown, check by gtp4zen --help" << endl;
			return 0;
		}
	} catch (exception &e) {
		cerr << "ERROR: " << e.what() << endl;
		return 0;
	} catch (...) {
		cerr << "ERROR: unknown, check by gtp4zen --help" << endl;
		return 0;
	}
#endif

	// �ļ����Ƿ��ʱ��
	string filename = g_logfile;
	if (g_logfilenametime) {
		const char *p1 = strrchr(filename.c_str(), '/');
		const char *p2 = strrchr(filename.c_str(), '\\');
		const char *p3 = strrchr(filename.c_str(), '.');

		SYSTEMTIME sys;
		GetLocalTime(&sys);
		char timestr[64] = "";
		sprintf_s(timestr
			, 64
			, "_%d-%02d-%02d_%02d-%02d-%02d"
			, sys.wYear, sys.wMonth, sys.wDay
			, sys.wHour, sys.wMinute, sys.wSecond
			);

		bool add_tail = false;
		if (p1 && p3) {
			if (p3 > p1)
				add_tail = false;
			else
				add_tail = true;
		} else if (p2 && p3) {
			if (p3 > p2)
				add_tail = false;
			else
				add_tail = true;
		} else {
			if (p3)
				add_tail = false;
			else
				add_tail = true;
		}

		if (add_tail) {
			filename += timestr;
		} else {
			boost::replace_last(filename, ".", timestr + std::string("."));
		}
		g_logfile = filename;
	}

	logprintf(L"%s", L"========����gtp4zen����========");
	logprintf(L"CPU������%d", sysInfo.dwNumberOfProcessors);
	logprintf(L"ZEN�汾��%d", g_zenver);
	logprintf(L"�߳�������%d", g_threads);
	logprintf(L"ÿ�����˼��ʱ�䣨�룩��%d", g_maxtime);
	logprintf(L"ÿ�����ģ�ⲽ����������%d", g_strength);

	std::string zen_dll_path = GetModuleFilePath() + "zen.dll";
	std::string lua_engine_path = GetModuleFilePath() + "gtp4zen.lua";
	if (!boost::filesystem::is_regular_file(zen_dll_path.c_str())) {
		fprintf(stderr, "ERROR: zen.dll not exist?\n");
		logprintf(L"%s", L"ERROR: zen.dll not exist?");
		//system("pause");
		return 0;
	}

	if (6 == g_zenver) {
		CZen6Gtp gtp6;
		CGtp *pgtp = &gtp6;
		if (pgtp->load(zen_dll_path.c_str(), lua_engine_path.c_str()))
			_play_zen(pgtp);
		pgtp->unload();
	} else if (7 == g_zenver) {
		CZen7Gtp gtp7;
		CGtp *pgtp = &gtp7;
		if (pgtp->load(zen_dll_path.c_str(), lua_engine_path.c_str()))
			_play_zen(pgtp);
		pgtp->unload();
	}

	return 0;
}

static void _play_zen(CGtp *pgtp)
{
	std::string result;
	std::string info_result;
	while (true) {
		std::string line;
		std::getline(cin, line);

		boost::trim_if(line, boost::is_any_of("\r\n\t "));
		std::vector<std::string> list;
		boost::split(list, line, boost::is_any_of("\r\n\t "), boost::token_compress_on);
		if (line.size() == 0)
			continue;
		result = "";
		info_result = "";
		if (0 == list.size()) {
			continue;
		} else if ("list_commands" == list[0]) {
			result = pgtp->list_commands();
		} else if ("name" == list[0]) {
			result = pgtp->name();
		} else if ("version" == list[0]) {
			result = pgtp->version();
		} else if ("protocol_version" == list[0]) {
			result = pgtp->protocol_version();
		} else if ("boardsize" == list[0] && list.size() >= 2) {
			result = pgtp->boardsize(atoi(list[1].c_str()));
		} else if ("quit" == list[0]) {
			fprintf(stdout, "=\n\n");
			fflush(stdout);
			break;
		} else if ("clear_board" == list[0]) {
			result = pgtp->clear_board();
		} else if ("komi" == list[0] && list.size() >= 2) {
			result = pgtp->komi((float)std::atof(list[1].c_str()));
		} else if ("play" == list[0] && list.size() >= 3) {		// play w Q17
			result = pgtp->play(list[1].c_str(), list[2].c_str());
		} else if ("genmove" == list[0] && list.size() >= 2) {		// genmove b
			result = pgtp->genmove(list[1].c_str());
			float winrate = pgtp->get_winrate();
			if (winrate > -1 && winrate < 2 && result != "pass") {
				char buff[1024];
				if (list[1] == "W" || list[1] == "w")
					winrate = 1 - winrate;
				sprintf_s(buff, "Black winrate: %.2f", winrate * 100);
				info_result = buff;
			}
		} else if ("place_free_handicap" == list[0] && list.size() >= 2) {
			std::vector<std::string> posarray;
			std::copy(list.begin() + 1, list.end(), posarray.begin());
			result = pgtp->free_handicap(posarray);
		} else if ("set_free_handicap" == list[0] && list.size() >= 2) {
			std::vector<std::string> posarray;
			std::copy(list.begin() + 1, list.end(), posarray.begin());
			result = pgtp->free_handicap(posarray);
		} else if ("winrate" == list[0]) {
			result = pgtp->winrate();
		} else if ("undo" == list[0]) {
			result = pgtp->undo();
		} else if ("time_settings" == list[0] && list.size() >= 4) {
			// ������ʱ��
			result = pgtp->time_settings(atoi(list[1].c_str())
				, atoi(list[2].c_str())
				, atoi(list[3].c_str())
				);
		} else if ("time_left" == list[0] && list.size() >= 4) {
			// ÿ�ζԷ����Լ����Ӿ͸����Լ���ʱ��
			result = pgtp->time_left(list[1].c_str()
				, atoi(list[2].c_str())
				, atoi(list[3].c_str())
				);
		} else if ("final_score" == list[0]) {
			result = pgtp->score();
		} else {
			result = "? unknown command [" + line + "]\n";
		}
		fprintf(stdout, "%s\n", result.c_str());
		//logprintf(L"command: %s", line.c_str());
		//logprintf(L"result: %s", result.c_str());
		fflush(stdout);
		if (info_result.size() > 0)
		{
			int ret = fprintf(stderr, "%s\n", info_result.c_str());
			if (ret > 0)
				fflush(stderr);
		}
	}
}

int __ansi2num(char ch)
{
	const char *axes = "ABCDEFGHJKLMNOPQRSTUVWXYZ";
	const char *find = strchr(axes, ch);
	if (!find)
		return -1;
	else
		return find - axes;
}

std::string __num2ansi(int x, int y, int boardsize)
{
	if (x < 0 || y < 0 || boardsize <= 0 || boardsize > 25)
		return "";

	const char array1[] = "ABCDEFGHJKLMNOPQRSTUVWXYZ";
	char buffer[32] = "";
	sprintf_s(buffer, 32, "%c%d", array1[x], boardsize - y);
	return buffer;
}

void logprintf(const TCHAR *_Format, ...)
{
	// �ɱ����
	TCHAR pszBuffer[1024] = L"";
	va_list ap;
	va_start(ap, _Format);
	vswprintf_s(pszBuffer, 1024, _Format, ap);
	va_end(ap);

	// д�ļ�
	ofstream out;
	if (g_logfile.size() > 0) {
		out.open(g_logfile.c_str(), ios::app | ios::out | ios::binary);
		if (out.is_open()) {
			SYSTEMTIME sys;
			GetLocalTime(&sys);
			TCHAR timestr[128] = L"";
			wsprintf(timestr
				, L"%d-%02d-%02d %02d:%02d:%02d.%04d | "
				, sys.wYear, sys.wMonth, sys.wDay
				, sys.wHour, sys.wMinute, sys.wSecond
				, sys.wMilliseconds
				);
			out << CWtoA(timestr);
			out << CWtoA(pszBuffer);
			out << "\r\n";
			out.close();
		}
	}
}

