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
#include <filesystem>
#include <stdexcept>
#include <algorithm>
#include <iomanip>
#include <ctime>
namespace fs = std::experimental::filesystem;

std::set<std::wstring> g_std_headers;
std::set<std::wstring> g_included_header;

fs::path Resolve(fs::path path, fs::path relative)
{
	auto filepath = fs::absolute(path, relative);
	if(fs::exists(filepath))
		return filepath;

	filepath = fs::absolute(path);
	if(!fs::exists(filepath))
		throw std::runtime_error("无法定位文件：" + path.filename().string());

	return filepath;
}


std::list<std::wstring> AssembleHeader(fs::path path)
{
	std::list<std::wstring> lines;
	auto filename = path.filename().wstring();

	if(g_included_header.cend() == g_included_header.find(filename))
	{
		std::cout << "读取 " << path.string() << std::endl;

		std::shared_ptr<FILE> fp(_wfopen(path.wstring().c_str(), L"rt, ccs=UTF-8"), fclose);
		std::wifstream fs(fp.get());

		g_included_header.insert(filename);

		std::wstring line;

		std::wregex once_regex(LR"=(^\s*#\s*pragma\s*once\s*$)=");
		std::wregex std_header_regex(LR"=(^\s*#\s*include\s*<([.\w]+)>\s*$)=");
		std::wregex my_header_regex(LR"=(^\s*#\s*include\s*"([.\\/\w]+)"\s*$)=");

		while(std::getline(fs, line))
		{
			std::wsmatch stdmatches, mymatches;
			if(std::regex_match(line, once_regex))
			{
				continue;
			}
			else if(std::regex_match(line, stdmatches, std_header_regex))
			{
				g_std_headers.insert(stdmatches[1].str());
			}
			else if(std::regex_match(line, mymatches, my_header_regex))
			{
				auto filepath = Resolve(mymatches[1].str(), path.parent_path());
				auto sublines = AssembleHeader(filepath);
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
	std::shared_ptr<FILE> fp(_wfopen(path.wstring().c_str(), L"wt, ccs=UTF-8"), fclose);
	std::wofstream fs(fp.get());

	std::cout << "保存至：" << path.string() << std::endl;

	auto time = std::time(nullptr);
	std::tm tm = {0};
	localtime_s(&tm, &time);


	fs << L"#pragma once" << std::endl << std::endl;
	fs << L"/******本文件由 Header Assembler 自动生成，请勿手动修改******/" << std::endl;
	fs << L"/***** https://github.com/icexile/HeaderAssembler.git *****/" << std::endl;
	fs << L"/************** 生成时间：" << std::put_time(&tm, L"%c") << L" ****************/" << std::endl << std::endl << std::endl;
	
	for(auto& header : g_std_headers)
	{
		fs << L"#include <" << header << ">" << std::endl;
	}

	for(auto& line : lines)
	{
		fs << line << std::endl;
	}
}


int main(int argc, char* argv[])
{
	try
	{
		if(argc < 3)
			throw std::invalid_argument("调用方式：assembler from.h to.h");

		auto srcpath = fs::absolute(argv[1]);
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
	std::cin.get();
	return 0;
}
