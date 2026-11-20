#include "pathnorm.hpp"

namespace {

int hex_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// "/a//b///c" -> "/a/b/c". Clients and proxies are sloppy about slashes.
void collapse_slashes(std::string& path) {
    std::size_t pos;
    while ((pos = path.find("//")) != std::string::npos) path.erase(pos, 1);
}

}  // namespace

std::string percent_decode(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] != '%') {
            out += text[i];
            continue;
        }
        int hi = i + 1 < text.size() ? hex_value(text[i + 1]) : -1;
        int lo = i + 2 < text.size() ? hex_value(text[i + 2]) : -1;
        if (hi < 0 || lo < 0) throw PathError("bad escape at offset " + std::to_string(i));
        out += static_cast<char>(hi * 16 + lo);
        i += 2;
    }
    return out;
}

std::vector<std::string> normalize_path(std::string_view target) {
    // Decode first, so that "/%7Ebob" and "/~bob" end up as the same segment.
    std::string path = percent_decode(target);
    if (auto cut = path.find_first_of("?#"); cut != std::string::npos) path.resize(cut);
    collapse_slashes(path);

    std::vector<std::string> segments;
    std::size_t start = 0;
    while (start <= path.size()) {
        std::size_t slash = path.find('/', start);
        if (slash == std::string::npos) slash = path.size();
        std::string piece = path.substr(start, slash - start);
        start = slash + 1;
        if (piece.empty() || piece == ".") continue;
        if (piece == "..") {
            if (!segments.empty()) segments.pop_back();
            continue;
        }
        segments.push_back(std::move(piece));
    }
    return segments;
}

std::string format_path(const std::vector<std::string>& segments) {
    static const char* kHex = "0123456789ABCDEF";
    if (segments.empty()) return "/";
    std::string out;
    for (const std::string& s : segments) {
        out += '/';
        for (unsigned char c : s) {
            if (c <= ' ' || c >= 0x7F || c == '%' || c == '/' || c == '?' || c == '#') {
                out += '%';
                out += kHex[c >> 4];
                out += kHex[c & 15];
            } else {
                out += static_cast<char>(c);
            }
        }
    }
    return out;
}
