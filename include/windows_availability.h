// -*- C++ -*- header.

// Copyright (c) 2022 Darryl Pogue
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#ifdef _WIN32

#include <cassert>
#include <cstdint>
#include <tuple>
#include <string_view>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>

namespace __builtin_availability {
    typedef std::tuple<DWORD, DWORD, DWORD> VersionTuple;

    /**
     * Everything in this `detail` namespace should hopefully be evaluated at
     * compile-time.
     */
    namespace detail {
        /**
         * Returns a version tuple from between 0 and 3 provided version numbers.
         *
         * Called with no arguments, returns a special "invalid" version.
         * Otherwise, it fills any missing numbers with 0.
         */
        static inline constexpr VersionTuple makeVersion(DWORD M = static_cast<DWORD>(-1), DWORD m = 0, DWORD b = 0) {
            return std::tie(M, m, b);
        }

        /**
         * Returns whether the specified string is trying to check for a
         * Windows version (vs some other platform).
         */
        static inline constexpr bool checkPlatform(std::string_view s) {
            using namespace std::literals::string_view_literals;

            return (s.compare(0, 8, "Windows "sv) == 0) || (s.compare(0, 8, "windows "sv) == 0);
        }

        /**
         * Parses an integer out of a string, compile-time.
         *
         * This takes a reference to a string, and modifies (by advancing
         * through the characters of) that reference.
         *
         * The parsed number is stored in the integer provided as the second
         * argument.
         *
         * Stops when it reaches a character that is not in the range of [0-9].
         * Returns a boolean whether a number was successfully parsed.
         */
        static inline constexpr bool extractVersionNumber(std::string_view& s, DWORD& v) {
            if (s.empty())
                return false;

            char c = s[0];
            s = s.substr(1);
            if (c < '0' || c > '9')
                return false;
            v = static_cast<DWORD>(c - '0');

            while (!s.empty()) {
                c = s[0];
                if (c < '0' || c > '9')
                    return true;
                s = s.substr(1);
                v = (v * 10) + static_cast<DWORD>(c - '0');
            }

            return true;

        }

        /**
         * Parses the Windows version number out of a string such as "Windows
         * 10 21H2" and returns a VersionTuple for comparison against the
         * system version.
         *
         * Most of the complexity here is around handling Microsoft's
         * non-sequential numbering and naming of versions.
         *
         * Currently this *only* supports common consumer versions of Windows
         * (not server versions). If you need to compare against a server
         * version, it will work if you provide the full build number directly
         * rather than a name (i.e., "Windows 6.0.6003" instead of "Windows
         * Server 2008 SP2").
         *
         * Returns an "invalid" VersionTuple (that won't match anything) if it
         * fails to parse the string.
         */
        static inline constexpr VersionTuple parseWindowsVersion(std::string_view s) {
            using namespace std::literals::string_view_literals;

            // The parsed major version, which rarely aligns with the actual OS
            // major version, so we need to keep track of this for further
            // handling of not-so-special cases
            DWORD reqMajor = 0;

            DWORD major = 0;
            DWORD minor = 0;
            DWORD build = 0;

            s = s.substr(8); // Strip off "Windows "

            if (s.compare(0, 5, "Vista"sv) == 0 || s.compare(0, 5, "vista"sv) == 0) {
                reqMajor = 6;
            } else if (s.compare(0, 2, "XP"sv) == 0 || s.compare(0, 2, "xp"sv) == 0) {
                reqMajor = 5;
                minor = 1;
            } else if (!extractVersionNumber(s, reqMajor)) {
                // Failed to parse
                return makeVersion();
            }

            major = reqMajor;

            // Windows 7
            if (reqMajor == 7) {
                major = 6;
                minor = 1;
            }

            // Windows 8
            if (reqMajor == 8) {
                major = 6;
                minor = 2;
            }

            // Windows 11
            if (reqMajor == 11) {
                major = 10;
                build = 22000;
            }

            if (s.empty() || (s[0] != '.' && s[0] != '_' && s[0] != ' '))
                return makeVersion(major, minor, build);

            s = s.substr(1);

            // Windows 10 & 11 named updates
            if (reqMajor == 11 && s.compare(0, 4, "22H2"sv) == 0) {
                build = 22621;
            } else if (reqMajor == 11 && s.compare(0, 4, "21H2"sv) == 0) {
                build = 22000;
            } else if (reqMajor == 10 && s.compare(0, 4, "22H2"sv) == 0) {
                build = 19045;
            } else if (reqMajor == 10 && s.compare(0, 4, "21H2"sv) == 0) {
                build = 19044;
            } else if (reqMajor == 10 && s.compare(0, 4, "21H1"sv) == 0) {
                build = 19043;
            } else if (reqMajor == 10 && s.compare(0, 4, "20H2"sv) == 0) {
                build = 19042;
            } else if (!extractVersionNumber(s, minor)) {
                return makeVersion(major, minor, build);
            }

            // Windows 8.1
            if (reqMajor == 8 && minor == 1) {
                minor = 3;
            }

            // Windows 10 & 11
            if (major == 10 && minor > 0) {
                // If we parsed a minor version, it's probably actually the build number
                // Again, need to deal with user-facing names of Windows 10 updates
                if (reqMajor == 10 && minor == 2004) {
                    build = 19041;
                } else if (reqMajor == 10 && minor == 1909) {
                    build = 18363;
                } else if (reqMajor == 10 && minor == 1903) {
                    build = 18362;
                } else if (reqMajor == 10 && minor == 1809) {
                    build = 17763;
                } else if (reqMajor == 10 && minor == 1803) {
                    build = 17134;
                } else if (reqMajor == 10 && minor == 1709) {
                    build = 16299;
                } else if (reqMajor == 10 && minor == 1703) {
                    build = 15063;
                } else if (reqMajor == 10 && minor == 1607) {
                    build = 14393;
                } else if (reqMajor == 10 && minor == 1511) {
                    build = 10586;
                } else if (reqMajor == 10 && minor == 1507) {
                    build = 10240;
                } else {
                    build = minor;
                }
                minor = 0;
            }

            if (s.empty() || (s[0] != '.' && s[0] != '_' && s[0] != ' '))
                return makeVersion(major, minor, build);

            s = s.substr(1);

            if (!extractVersionNumber(s, build))
                return makeVersion(major, minor, build);

            return makeVersion(major, minor, build);
        }
    }


    static inline DWORD majorVersion = 0;
    static inline DWORD minorVersion = 0;
    static inline DWORD buildVersion = 0;

    /**
     * Should be called once, automatically, at runtime, to initialize the static version numbers with the current OS value.
     */
    static inline void _loadSystemVersion() {
        typedef void (WINAPI *RtlGetNtVersionNumbersPtrType)(LPDWORD, LPDWORD, LPDWORD);

        HINSTANCE inst = GetModuleHandle(_T("ntdll.dll"));
        RtlGetNtVersionNumbersPtrType RtlGetNtVersionNumbers = reinterpret_cast<RtlGetNtVersionNumbersPtrType>(GetProcAddress(inst, "RtlGetNtVersionNumbers"));

        assert(RtlGetNtVersionNumbers);

        RtlGetNtVersionNumbers(&majorVersion, &minorVersion, &buildVersion);
        buildVersion &= 0x0FFFFFFF;
    }

    /**
     * The version comparison function, checking if the requested version is supported by the current OS.
     *
     * Called at runtime whenever a `__builtin_available(Windows ...)` block is encountered.
     */
    static inline bool _isVersionAtLeast(const VersionTuple& version) {
        if (!majorVersion)
            _loadSystemVersion();

        return version <= std::tie(majorVersion, minorVersion, buildVersion);
    }
}

#define ___windows_available_check(x) ([]{ \
    if constexpr (__builtin_availability::detail::checkPlatform(x)) { \
        return __builtin_availability::_isVersionAtLeast(__builtin_availability::detail::parseWindowsVersion(x)); \
    } else { \
        return false; \
    } }())

#define ___wa_select(a1, a2, a3, a4, a5, a6, ...)   a6
#define ___wa_num(...)                          ___wa_select(__VA_ARGS__, 5, 4, 3, 2, 1, 0)
#define ___windows_available_helper_5(x, ...)   ___windows_available_check(#x) || ___windows_available_helper_4(__VA_ARGS__)
#define ___windows_available_helper_4(x, ...)   ___windows_available_check(#x) || ___windows_available_helper_3(__VA_ARGS__)
#define ___windows_available_helper_3(x, ...)   ___windows_available_check(#x) || ___windows_available_helper_2(__VA_ARGS__)
#define ___windows_available_helper_2(x, ...)   ___windows_available_check(#x) || ___windows_available_helper_1(__VA_ARGS__)
#define ___windows_available_helper_1(x)        ___windows_available_check(#x)
#define ___windows_available_macro2(c, ...)     ___windows_available_helper_##c(__VA_ARGS__)
#define ___windows_available_macro1(c, ...)     ___windows_available_macro2(c, __VA_ARGS__)
#define ___windows_available(...)               ___windows_available_macro1(___wa_num(__VA_ARGS__), __VA_ARGS__)

#define windows_version_available               ___windows_available
#define windows_version(...)                    __VA_ARGS__

#ifndef NO_BUILTIN_AVAILABLE_CLOBBER
#define __builtin_available                     ___windows_available
#endif

#else // _WIN32

#define windows_version_available(...)          false
#define windows_version(...)

#endif // _WIN32
