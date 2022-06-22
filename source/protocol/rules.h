#pragma once
#include <string>
#include <vector>

const std::vector<std::string> SubtitleExtensions{
    ".utf", ".utf8", ".utf-8", ".idx", ".sub", ".srt", ".rt", ".ssa",
    ".ass", ".mks", ".vtt", ".sup", ".scc", ".smi", ".lrc", ".pgs", ".txt" 
};

const std::vector<std::string> VideoExtensions{
    ".3g2", ".3gp", ".amv", ".asf", ".avi", ".divx", ".drc",".dv",
    ".evo", ".f4v", ".flv", ".gxf", ".iso", ".m1v", ".m2p", ".m2t",
    ".m2ts", ".m2v", ".m4v", ".mkv", ".mov", ".mp2", ".mp4", ".mpe",
    ".mpeg", ".mpg", ".mts", ".mtv", ".mxf", ".nsv", ".nut", ".nuv",
    ".ogm", ".ogv", ".qt", ".rec", ".rm", ".rmvb", ".swf",
    ".tp", ".ts", ".vob", ".vp6", ".webm", ".wmv", ".wtv"
};

const std::vector<std::string> filters_field_skybox{ "H264", "H265", "x264", "x265", "H.264",
                                                 "H.265", "x264", "x265", "MPEG-4", "HEVC",
                                                 "480P", "720p", "1080p", "WEB-DL", "WEBRip",
                                                 "BluRay", "HDTV", "HDrip", "BRrip", "BDrip",
                                                 "DVDrip", "DVDscr", "XviD", "DivX", "R5",
                                                 "AC3", "DD5.1", "DD+5.1", "DDP5.1", "AVS",
                                                 "DTS", "ViSUM", "AMZN", "SPARKS", "YTS-AG",
                                                 "RARbg", "iTunes", "HQ.Hive-CM8", "FGT",
                                                 "¡¾", "¡¿", "£¨", "£©" 
};

bool IsVideo(const std::string& path);
bool IsSubtitle(const std::string& path);
bool IsPicture(const std::string& path);
bool is_video_name_match(std::string haystack, std::string needle);