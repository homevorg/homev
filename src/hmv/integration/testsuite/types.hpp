// Copyright (C) 2025-2026 Marleson Graf

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#ifndef __HTS_TYPES_HPP__
#define __HTS_TYPES_HPP__

#include <cstdint>

#ifndef HTS_THREAD_COUNT
#define HTS_THREAD_COUNT 8
#endif
#ifndef HTS_MAX_ADDRESS_COUNT
#define HTS_MAX_ADDRESS_COUNT 1024
#endif
#ifndef HTS_MACHINE_CODE
#define HTS_MACHINE_CODE unsigned
#endif
#ifdef HTS_USE_QUAD_WORD_VALUE
#define HTS_WORD_SHIFT 3
#else
#define HTS_WORD_SHIFT 2
#endif

namespace hts {
    using Address = uint64_t;
#ifdef HTS_USE_QUAD_WORD_VALUE
    using WordType = uint64_t;
#else
    using WordType = uint32_t;
#endif
    using ThreadFunction = void (*)(volatile WordType*);
} // namespace hts

#endif /* __HTS_TYPES_HPP__ */
