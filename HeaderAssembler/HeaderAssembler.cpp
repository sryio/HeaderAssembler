#define _CRT_SECURE_NO_WARNINGS

#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <regex>
#include <list>
#include <vector>
#include <set>
#include <stdexcept>
#include <algorithm>
#include <iomanip>
#include <ctime>
#include <locale>
#include <codecvt>
#include <iterator>

#ifdef _WIN32
#	include <filesystem>
	namespace fs = std::experimental::filesystem;
#else
#	include <boost/filesystem.hpp>
	namespace fs = boost::filesystem;
#endif

std::set<std::wstring>	g_std_headers;
std::set<std::wstring>	g_included_header;
std::list<fs::path>		g_search_pathes;


void InitSearchPathes(int argc, char* argv[])
{
	if(argc < 4)
		return;

	std::string arg = argv[3];
	std::regex rgx(R"(\s*;\s*)");

	for(auto it = std::sregex_token_iterator(arg.cbegin(), arg.cend(), rgx, -1); it != std::sregex_token_iterator(); ++it)
	{
		g_search_pathes.push_back(it->str());
	}

	std::cout << "初始化搜索路径：" << std::endl;
	for(auto& path : g_search_pathes)
	{
		std::cout << path << std::endl;
	}

	std::cout << std::endl;
}

fs::path Resolve(fs::path path)
{
	for(auto& p : g_search_pathes)
	{
		auto filepath = fs::absolute(path, p);

		if(fs::exists(filepath))
			return filepath;
	}
	
	throw std::runtime_error("无法定位文件：" + path.filename().string());
}


std::list<std::wstring> AssembleHeader(fs::path path)
{
	std::list<std::wstring> lines;
	auto filename = path.filename().wstring();

	if(g_included_header.cend() == g_included_header.find(filename))
	{
		path = Resolve(path);

		std::cout << "读取 " << path.string() << std::endl;

		std::wifstream fs(path.string().c_str());
		fs.imbue(std::locale(std::locale::classic(), new std::codecvt_utf8<wchar_t, 0x10FFFF, std::consume_header>));

		std::wstring line;

		std::wregex once_regex(LR"=(^\s*#\s*pragma\s*once\s*$)=");
		std::wregex std_header_regex(LR"=(^\s*#\s*include\s*<([.\w]+)>\s*$)=");
		std::wregex my_header_regex(LR"=(^\s*#\s*include\s*"([.\\/\w]+)"\s*$)=");

		while(std::getline(fs, line))
		{
			std::wsmatch stdmatches, mymatches;
			if(std::regex_match(line, once_regex))
			{
				g_included_header.insert(filename); //识别#pragma once
				continue;
			}
			else if(std::regex_match(line, stdmatches, std_header_regex))
			{
				g_std_headers.insert(stdmatches[1].str());
			}
			else if(std::regex_match(line, mymatches, my_header_regex))
			{
				g_search_pathes.push_front(path.parent_path());

				auto sublines = AssembleHeader(mymatches[1].str());
				g_search_pathes.pop_front();

				std::copy(sublines.cbegin(), sublines.cend(), std::back_inserter(lines));
			}
			else
			{
				lines.push_back(std::move(line));
			}
		}

	}

	return lines;
}

void SaveLines(std::list<std::wstring> lines, fs::path path)
{
	std::cout << "合并至：" << path.string() << std::endl;

	auto time = std::time(nullptr);
	std::wostringstream buffer;

	buffer << (wchar_t)0xFEFF;
	buffer << L"#pragma once\r\n\r\n";
	buffer << L"// 本文件由 Header Assembler 自动生成，请勿手动修改\r\n";
	buffer << L"// https://github.com/icexile/HeaderAssembler \r\n";
	buffer << L"// 生成时间：" << std::put_time(localtime(&time), L"%c") << L"\r\n\r\n\r\n";

	for(auto& header : g_std_headers)
	{
		buffer << L"#include <" << header << ">\r\n";
	}

	for(auto line : lines)
	{
		line.erase(std::remove_if(line.begin(), line.end(), [](wchar_t ch) { return ch == L'\r' || ch == L'\n'; }), line.end());
		buffer << line << L"\r\n";
	}

	std::ofstream nfs(path.string().c_str(), std::ios::binary);

	std::wstring_convert<std::codecvt_utf8<wchar_t>> wc;
	auto utf8 = wc.to_bytes(buffer.str());
	
	std::copy(utf8.cbegin(), utf8.cend(), std::ostreambuf_iterator<char>(nfs));
}


int main(int argc, char* argv[])
{
#ifdef _WIN32
	system("chcp 936");
#endif

    std::ios::sync_with_stdio(false);
	std::cout << "头文件自动合并开始：" << std::endl;

	try
	{
		if(argc < 3)
			throw std::invalid_argument("调用方式：assembler from.h to.h [searchpath1;searchpath2;...]");

		InitSearchPathes(argc, argv);

		auto srcpath = argv[1];
		auto lines = AssembleHeader(srcpath);

		auto destpath = fs::path(argv[2]);
		SaveLines(std::move(lines), destpath);

	}
	catch(std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return -1;
	}

	std::cout << "成功！" << std::endl;

#if (defined _WIN32) && (defined _DEBUG)
	system("pause");
#endif

	return 0;
}
