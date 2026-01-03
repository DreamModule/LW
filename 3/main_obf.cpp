#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <cstdint>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <numeric>
#include <functional>
#include <memory>
#include <array>

template<typename T, size_t N>
struct _0x4f { std::array<T, N> _; T& operator[](size_t i) { return _[i]; } const T& operator[](size_t i) const { return _[i]; } };

template<typename T> struct _0x54 { using type = T; };
template<typename T> using _0x49 = typename _0x54<T>::type;

namespace _0x4b {
    constexpr uint8_t _0x38 = 0x4F, _0x36 = 0x54, _0x5f = 0x49, _0x6b = 0x4B;
    constexpr uint8_t _0x38_2 = 0x38, _0x36_2 = 0x36;
    constexpr uint16_t _0x76 = 0x01;
    constexpr size_t _0x62 = 65536;
    
    template<size_t N> struct _0x73 { 
        char _[N]; 
        constexpr _0x73(const char (&s)[N]) { for(size_t i=0;i<N;++i) _[i]=s[i]; }
        operator const char*() const { return _; }
    };
}

#pragma pack(push, 1)
struct _0x48 {
    uint8_t _0[6];
    uint16_t _1;
    uint64_t _2;
    
    template<typename... Args>
    _0x48(Args&&... args) : _1(_0x4b::_0x76), _2(std::forward<Args>(args)...) {
        _0[0] = _0x4b::_0x38; _0[1] = _0x4b::_0x36; _0[2] = _0x4b::_0x5f;
        _0[3] = _0x4b::_0x6b; _0[4] = _0x4b::_0x38_2; _0[5] = _0x4b::_0x36_2;
    }
};
#pragma pack(pop)

template<typename _T1, typename _T2>
class _0x50 {
    using _F = std::function<void(char*, size_t)>;
    _F _0x66;
    
public:
    explicit _0x50(_F f) : _0x66(std::move(f)) {}
    
    template<typename... Args>
    auto operator()(Args&&... args) const -> decltype(auto) {
        return _0x66(std::forward<Args>(args)...);
    }
};

class _0x41 {
    template<typename _S, typename _D>
    static auto _0x70(_S& s, _D& d, uint64_t n, bool m) -> void {
        std::vector<char> _b(_0x4b::_0x62);
        uint64_t _p = 0;
        
        auto _0x6c = [](char* data, size_t sz) {
            std::transform(data, data + sz, data, [](char c) { return ~c; });
        };
        
        _0x50<char*, size_t> _0x72(_0x6c);
        
        while (_p < n) {
            size_t _r = std::min(_0x4b::_0x62, static_cast<size_t>(n - _p));
            s.read(_b.data(), _r);
            auto _g = s.gcount();
            if (_g == 0) break;
            _0x72(_b.data(), _g);
            d.write(_b.data(), _g);
            _p += _g;
        }
        
        if (_p != n) {
            throw std::runtime_error(std::string("_0x65:") + std::to_string(n) + 
                                   std::string("!=") + std::to_string(_p));
        }
    }
    
    static auto _0x77(std::ofstream& o, uint64_t s) -> void {
        _0x48 _h(s);
        o.write(reinterpret_cast<const char*>(&_h), sizeof(_h));
        uint8_t _c[2] = {0, 0};
        o.write(reinterpret_cast<const char*>(_c), 2);
    }
    
    static auto _0x72(std::ifstream& i) -> uint64_t {
        _0x48 _h(0);
        i.read(reinterpret_cast<char*>(&_h), sizeof(_h));
        
        if (i.gcount() != sizeof(_h)) {
            throw std::runtime_error("_0x68");
        }
        
        std::array<uint8_t, 6> _s = {_0x4b::_0x38, _0x4b::_0x36, _0x4b::_0x5f, 
                                      _0x4b::_0x6b, _0x4b::_0x38_2, _0x4b::_0x36_2};
        if (!std::equal(_s.begin(), _s.end(), _h._0)) {
            throw std::runtime_error("_0x73");
        }
        
        if (_h._1 != _0x4b::_0x76) {
            throw std::runtime_error("_0x76:" + std::to_string(_h._1));
        }
        
        uint8_t _c[2];
        i.read(reinterpret_cast<char*>(_c), 2);
        
        if (_c[0] != 0 || _c[1] != 0) {
            throw std::runtime_error("_0x63");
        }
        
        return _h._2;
    }
    
public:
    static auto _0x65(const std::string& _i, const std::string& _o) -> void {
        std::ifstream _f(_i, std::ios::binary | std::ios::ate);
        if (!_f.is_open()) throw std::runtime_error("_0x66:" + _i);
        
        uint64_t _s = _f.tellg();
        _f.seekg(0, std::ios::beg);
        
        std::ofstream _d(_o, std::ios::binary);
        if (!_d.is_open()) throw std::runtime_error("_0x64:" + _o);
        
        _0x77(_d, _s);
        _0x70(_f, _d, _s, true);
        
        _f.close();
        _d.close();
    }
    
    static auto _0x64(const std::string& _a, const std::string& _o) -> void {
        std::ifstream _f(_a, std::ios::binary);
        if (!_f.is_open()) throw std::runtime_error("_0x61:" + _a);
        
        uint64_t _s = _0x72(_f);
        
        std::ofstream _d(_o, std::ios::binary);
        if (!_d.is_open()) throw std::runtime_error("_0x6f:" + _o);
        
        _0x70(_f, _d, _s, false);
        
        _f.close();
        _d.close();
    }
};

auto main(int _ac, char* _av[]) -> int {
    try {
        if (_ac != 4) {
            auto _u = [&](auto&&... args) {
                (std::cerr << ... << args) << std::endl;
            };
            _u("Usage:");
            _u("  Encode: ", _av[0], " -c <input_file> <archive_file>");
            _u("  Decode: ", _av[0], " -d <archive_file> <output_file>");
            return 1;
        }
        
        const std::string _m = _av[1], _i = _av[2], _o = _av[3];
        
        auto _exec = [&](auto&& op, auto&& msg) {
            op(_i, _o);
            std::cout << msg << _o << std::endl;
        };
        
        if (_m == "-c") {
            _exec(std::mem_fn(&_0x41::_0x65), "Encoded: ");
        } else if (_m == "-d") {
            _exec(std::mem_fn(&_0x41::_0x64), "Decoded: ");
        } else {
            throw std::runtime_error("Unknown: " + _m);
        }
        
        return 0;
        
    } catch (const std::exception& _e) {
        std::cerr << "Error: " << _e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Fatal error" << std::endl;
        return 1;
    }
}
