#ifndef _STRING_HANDLER_H_
#define _STRING_HANDLER_H_

#include <cstring>
#include <string>
#include <vector>
using namespace std;

inline string trimright(const string &sStr, const string &s, bool bChar)
{
    if (sStr.empty())
    {
        return sStr;
    }

    /**
     * 去掉sStr右边的字符串s
     */
    if (!bChar)
    {
        if (sStr.length() < s.length())
        {
            return sStr;
        }

        if (sStr.compare(sStr.length() - s.length(), s.length(), s) == 0)
        {
            return sStr.substr(0, sStr.length() - s.length());
        }

        return sStr;
    }

    /**
     * 去掉sStr右边的 字符串s中的字符
     */
    string::size_type pos = sStr.length();
    while (pos != 0)
    {
        if (s.find_first_of(sStr[pos - 1]) == string::npos)
        {
            break;
        }

        pos--;
    }

    if (pos == sStr.length())
        return sStr;

    return sStr.substr(0, pos);
}

inline string trimleft(const string &sStr, const string &s = " \r\n\t", bool bChar = true)
{
    if (sStr.empty())
    {
        return sStr;
    }

    /**
     * 去掉sStr左边的字符串s
     */
    if (!bChar)
    {
        if (sStr.length() < s.length())
        {
            return sStr;
        }

        if (sStr.compare(0, s.length(), s) == 0)
        {
            return sStr.substr(s.length());
        }

        return sStr;
    }

    /**
     * 去掉sStr左边的 字符串s中的字符
     */
    string::size_type pos = 0;
    while (pos < sStr.length())
    {
        if (s.find_first_of(sStr[pos]) == string::npos)
        {
            break;
        }

        pos++;
    }

    if (pos == 0)
        return sStr;

    return sStr.substr(pos);
}

/**
 * @brief  去掉头部以及尾部的字符或字符串.
 * @brief  Remove the head and the tail characters or strings
 *
 * @param sStr    输入字符串
 * @param s       需要去掉的字符
 * @param bChar   如果为true, 则去掉s中每个字符; 如果为false, 则去掉s字符串
 * @return string 返回去掉后的字符串
 */

inline string trim(const string &sStr, const string &s, bool bChar)
{
    if (sStr.empty())
    {
        return sStr;
    }

    /**
     * 将完全与s相同的字符串去掉
     */
    if (!bChar)
    {
        return trimright(trimleft(sStr, s, false), s, false);
    }

    return trimright(trimleft(sStr, s, true), s, true);
}

inline string replace(const string &sString, const string &sSrc, const string &sDest)
{
    if (sSrc.empty())
    {
        return sString;
    }

    string sBuf = sString;

    string::size_type pos = 0;

    while ((pos = sBuf.find(sSrc, pos)) != string::npos)
    {
        sBuf.replace(pos, sSrc.length(), sDest);
        pos += sDest.length();
    }

    return sBuf;
}

inline std::vector<std::string> split(const std::string &str, const std::string &delimiter = " ")
{
    char *save = nullptr;
    char *token = strtok_r(const_cast<char *>(str.c_str()), delimiter.c_str(), &save);
    std::vector<std::string> result;
    while (token != nullptr)
    {
        result.emplace_back(token);
        token = strtok_r(nullptr, delimiter.c_str(), &save);
    }
    return result;
}

#endif