#include "rules.h"

bool IsVideo(const std::string& path)
{
    size_t index = path.find_last_of('.');
    if (index == std::string::npos)
        return false;

    const std::string suffix = path.substr(index);
    for (const std::string& ext : VideoExtensions)
    {
        if (std::equal(suffix.begin(), suffix.end(), ext.begin(), ext.end(),
            [](char x, char y) {
                return std::tolower(x) == y;
            }))
            return true;
    }
    return false;
}

bool IsPicture(const std::string& path)
{
    size_t index = path.find_last_of('.');
    if (index == std::string::npos)
        return false;

    const std::string suffix = path.substr(index);
    for (const std::string& ext : { ".png",".jpg" })
    {
        if (std::equal(suffix.begin(), suffix.end(), ext.begin(), ext.end(),
            [](char x, char y) {
                return std::tolower(x) == y;
            }))
            return true;
    }
    return false;
}

bool IsSubtitle(const std::string& path)
{
    size_t index = path.find_last_of('.');
    if (index == std::string::npos)
        return false;

    const std::string suffix = path.substr(index);
    for (const std::string& ext : SubtitleExtensions)
    {
        if (std::equal(suffix.begin(), suffix.end(), ext.begin(), ext.end(),
            [](char x, char y) {
                return std::tolower(x) == y;
            }))
            return true;
    }
    return false;
}

bool checkSeparator(const char chr)
{
    if (chr == '.' || chr == '_' || chr == '\\' ||
        chr == '/' || chr == '(' || chr == ')' ||
        chr == '[' || chr == ']' || chr == '{' ||
        chr == '}' || chr == ' ')
        return true;
    return false;
}

bool checkStr(char* p)
{
    char c = *p;
    if (c >= '0' && c <= '9') return false;
    if (c >= 'A' && c <= 'Z') return false;
    if (c >= 'a' && c <= 'z') {
        *p = (int)(*p) - 32;
        return false;
    }

    return true;
}

std::string strstrEx(const std::string& haystack, const std::string& needle)
{
    std::string res;
    for (size_t i = 0; i < haystack.size(); ++i)
    {
        res = haystack.substr(i);
        if (std::equal(res.begin(), res.end(), needle.begin(), needle.end(),
            [](char x, char y) {
                return std::tolower(x) == y;
            }))
            return res;
    }
    return std::string();
}

bool is_video_name_match(std::string haystack, std::string needle)
{
    std::string hayStr;
    std::string needleStr;
    //1.过滤
    for (auto& it : filters_field_skybox)
    {
        hayStr = strstrEx(haystack, it);
        if (!hayStr.empty())
        {
            for (size_t j = 0; j < it.size(); ++j)
                hayStr[j] = '_';
        }
        needleStr = strstrEx(needle, it);
        if (!needleStr.empty())
        {
            for (size_t j = 0; j < it.size(); ++j)
                needleStr[j] = '_';
        }
    }

    //2.比较
    //2.1 检查视频标题是否存在字母和数字
    int i = 0, j = 0, k = 0;
    for (; k < needle.length(); ++k)
    {
        if (checkStr(&needle[k]) == false) {
            break;
        }
    }

    if (k == needle.length())
    {
        //视频标题中不含数字和字母
        //匹配策略：相似度60%以上
        i = 0, j = 0;
        int points = 0;
        int sum = 0;
        while (i < haystack.length() && j < needle.length())
        {
            if (checkSeparator(haystack[i]))
            {
                ++i;
                continue;
            }
            if (checkSeparator(needle[j]))
            {
                ++j;
                continue;
            }

            if (haystack[i++] == needle[j++])
                ++points;
            ++sum;
        }

        if (sum == 0 || ((double)points / (double)sum > 0.6)) {
            return true;
        }
    }
    else
    {
        //视频标题中含有数字和字母
        //匹配策略：字母和数字是否一致
        while (i < haystack.length() && j < needle.length())
        {
            if (checkStr(&haystack[i]))
            {
                ++i;
                continue;
            }
            if (checkStr(&needle[j]))
            {
                ++j;
                continue;
            }

            if (haystack[i++] != needle[j++])
                return false;
        }

        if (j == needle.length())
            return true;
    }

    return false;
}