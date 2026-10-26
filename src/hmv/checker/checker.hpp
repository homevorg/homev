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

#ifndef __HMV_CHECKER_CHECKER_HPP__
#define __HMV_CHECKER_CHECKER_HPP__

#include <filesystem>
#include <memory>
#include <string>

#include "component/logger.hpp"
#include "component/report.hpp"
#include "hmv/core/event.hpp"
#include "hmv/core/test.hpp"
#include "hmv/core/time_counter.hpp"
#include "hmv/core/types.hpp"
#include "hmv/utility/type.hpp"

namespace hmv::checker {
    enum class Type : uint8_t {
        HOCH = 0,
        EVCH = 1,
        MRSB = 2,
        SRSB = 3,
        LCHECK = 4,
    };

    class Checker {
     public:
        Checker(Type type,
                MemoryTraits memory_traits,
                ThreadCount thread_count,
                ISATraits traits,
                const std::filesystem::path& log_dir,
                bool enable_logger);
        void begin_test(const Test& test);
        void begin_instance();
        void begin_thread(ThreadID tid);
        bool end_thread(ThreadID tid);
        bool end_instance();
        bool end_test();
        template <typename E>
        bool push(const E& event);
        const Report& report() const;
        Time time() const;
        Type type() const;

     private:
        class Concept {
         public:
            virtual ~Concept() = default;
            virtual void begin_test(const Test& test) = 0;
            virtual void begin_instance() = 0;
            virtual void begin_thread(ThreadID tid) = 0;
            virtual bool end_thread(ThreadID tid) = 0;
            virtual bool end_instance() = 0;
            virtual bool end_test() = 0;
            virtual bool push(const WriteAt<AVAILABILITY>& event) = 0;
            virtual bool push(const ReadAt<LOCAL_COMMIT>& event) = 0;
            virtual bool push(const WriteAt<LOCAL_COMMIT>& event) = 0;
            virtual bool push(const FenceAt<LOCAL_COMMIT>& event) = 0;
            virtual bool push(const ReadAt<LOCAL_COMPLETION>& event) = 0;
            virtual bool push(const WriteAt<LOCAL_COMPLETION>& event) = 0;
            virtual bool push(const ReadAt<MEMORY>& event) = 0;
            virtual bool push(const WriteAt<MEMORY>& event) = 0;
            virtual bool push(const ReadAt<ENTER_TIME>& event) = 0;
            virtual bool push(const WriteAt<ENTER_TIME>& event) = 0;
            virtual bool push(const FenceAt<ENTER_TIME>& event) = 0;
            virtual bool push(const ReadAt<COMMIT_TIME>& event) = 0;
            virtual bool push(const WriteAt<COMMIT_TIME>& event) = 0;
            virtual bool push(const FenceAt<COMMIT_TIME>& event) = 0;
            virtual bool push(const WriteAt<REMOTE_COMMIT>& event) = 0;
            virtual bool push(const WriteAt<REMOTE_COMPLETION>& event) = 0;
            virtual bool push(const ReadAt<SQUASH>& event) = 0;
            virtual bool push(const WriteAt<SQUASH>& event) = 0;
            virtual const Report& report() const = 0;
            virtual Time time() const = 0;
        };

        template <typename C>
        class Model final : public Concept {
         public:
            Model(MemoryTraits memory_traits,
                  ThreadCount thread_count,
                  ISATraits traits,
                  const std::filesystem::path& log_filename,
                  bool enable_logger);
            void begin_test(const Test& test) override;
            void begin_instance() override;
            void begin_thread(ThreadID tid) override;
            bool end_thread(ThreadID tid) override;
            bool end_instance() override;
            bool end_test() override;
            bool push(const WriteAt<AVAILABILITY>& event) override;
            bool push(const ReadAt<LOCAL_COMMIT>& event) override;
            bool push(const WriteAt<LOCAL_COMMIT>& event) override;
            bool push(const FenceAt<LOCAL_COMMIT>& event) override;
            bool push(const ReadAt<LOCAL_COMPLETION>& event) override;
            bool push(const WriteAt<LOCAL_COMPLETION>& event) override;
            bool push(const ReadAt<MEMORY>& event) override;
            bool push(const WriteAt<MEMORY>& event) override;
            bool push(const ReadAt<ENTER_TIME>& event) override;
            bool push(const WriteAt<ENTER_TIME>& event) override;
            bool push(const FenceAt<ENTER_TIME>& event) override;
            bool push(const ReadAt<COMMIT_TIME>& event) override;
            bool push(const WriteAt<COMMIT_TIME>& event) override;
            bool push(const FenceAt<COMMIT_TIME>& event) override;
            bool push(const WriteAt<REMOTE_COMMIT>& event) override;
            bool push(const WriteAt<REMOTE_COMPLETION>& event) override;
            bool push(const ReadAt<SQUASH>& event) override;
            bool push(const WriteAt<SQUASH>& event) override;
            const Report& report() const override;
            Time time() const override;

         private:
            C checker;
            Logger logger;
            TimeCounter time_counter;

            template <typename E>
            bool try_push(const E& event);
        };

        Type tag;
        std::unique_ptr<Concept> model;

        template <typename C>
        using BeginTestDefinition =
            std::is_same<decltype(std::declval<C>().begin_test(
                             std::declval<Test>())),
                         void>;
        template <typename C>
        using BeginInstanceDefinition =
            std::is_same<decltype(std::declval<C>().begin_instance()), void>;
        template <typename C>
        using BeginThreadDefinition =
            std::is_same<decltype(std::declval<C>().begin_thread(
                             std::declval<ThreadID>())),
                         void>;
        template <typename C>
        using EndThreadDefinition =
            std::is_same<decltype(std::declval<C>().end_thread(
                             std::declval<ThreadID>())),
                         bool>;
        template <typename C>
        using EndInstanceDefinition =
            std::is_same<decltype(std::declval<C>().end_instance()), bool>;
        template <typename C>
        using EndTestDefinition =
            std::is_same<decltype(std::declval<C>().end_test()), bool>;
        template <typename C>
        using ReportDefinition =
            std::is_same<decltype(std::declval<C>().report()), const Report&>;
        template <typename C>
        using StatsDefinition =
            std::is_same<decltype(std::declval<C>().stats()), std::string>;
        template <typename C>
        using WriteDefinition =
            std::is_same<decltype(std::declval<C>().write(
                             std::declval<std::filesystem::path>())),
                         void>;

     public:
        template <typename C>
        using HasBeginTest = HasMethod<C, BeginTestDefinition>;
        template <typename C>
        using HasBeginInstance = HasMethod<C, BeginInstanceDefinition>;
        template <typename C>
        using HasBeginThread = HasMethod<C, BeginThreadDefinition>;
        template <typename C>
        using HasEndThread = HasMethod<C, EndThreadDefinition>;
        template <typename C>
        using HasEndInstance = HasMethod<C, EndInstanceDefinition>;
        template <typename C>
        using HasEndTest = HasMethod<C, EndTestDefinition>;
        template <typename C>
        using HasWrite = HasMethod<C, WriteDefinition>;

        template <typename C, typename E>
        struct HasPush {
            template <typename T>
            using Push = std::is_same<decltype(std::declval<T>().push(
                                          std::declval<E>())),
                                      bool>;

            static constexpr auto value = HasMethod<C, Push>::value;
        };
    };

    std::string to_string(Type type);

    template <typename E>
    inline bool Checker::push(const E& event) {
        return model->push(event);
    }

    inline Type Checker::type() const {
        return tag;
    }

    template <typename C>
    inline Checker::Model<C>::Model(MemoryTraits memory_traits,
                                    ThreadCount thread_count,
                                    ISATraits traits,
                                    const std::filesystem::path& log_filename,
                                    bool enable_logger)
        : checker{thread_count, traits},
          logger{memory_traits, log_filename, C::Error::NONE, enable_logger} {}

    template <typename C>
    inline void Checker::Model<C>::begin_test(const Test& test) {
        time_counter.reset();
        if constexpr (HasBeginTest<C>::value) {
            time_counter.start();
            checker.begin_test(test);
            time_counter.stop();
        }
    }

    template <typename C>
    inline void Checker::Model<C>::begin_instance() {
        logger.emplace_instance();
        if constexpr (HasBeginInstance<C>::value) {
            time_counter.start();
            checker.begin_instance();
            time_counter.stop();
        }
    }

    template <typename C>
    inline void Checker::Model<C>::begin_thread(ThreadID tid) {
        if constexpr (HasBeginThread<C>::value) {
            time_counter.start();
            checker.begin_thread(tid);
            time_counter.stop();
        }
    }

    template <typename C>
    inline bool Checker::Model<C>::end_thread(ThreadID tid) {
        auto result = false;
        if constexpr (HasEndThread<C>::value) {
            time_counter.start();
            result = checker.end_thread(tid);
            time_counter.stop();
        }
        return result;
    }

    template <typename C>
    inline bool Checker::Model<C>::end_instance() {
        auto result = false;
        if constexpr (HasEndInstance<C>::value) {
            time_counter.start();
            result = checker.end_instance();
            time_counter.stop();
        }
        return result;
    }

    template <typename C>
    inline bool Checker::Model<C>::end_test() {
        auto result = false;
        if constexpr (HasEndTest<C>::value) {
            time_counter.start();
            result = checker.end_test();
            time_counter.stop();
        }
        if (logger.write(checker.report())) {
            if constexpr (HasWrite<C>::value) {
                checker.write(logger.dir());
            }
        }
        return result;
    }

    template <typename C>
    inline bool Checker::Model<C>::push(const ReadAt<LOCAL_COMMIT>& event) {
        return try_push(event);
    }

    template <typename C>
    inline bool Checker::Model<C>::push(const WriteAt<LOCAL_COMMIT>& event) {
        return try_push(event);
    }

    template <typename C>
    inline bool Checker::Model<C>::push(const FenceAt<LOCAL_COMMIT>& event) {
        return try_push(event);
    }

    template <typename C>
    inline bool Checker::Model<C>::push(const WriteAt<REMOTE_COMMIT>& event) {
        return try_push(event);
    }

    template <typename C>
    inline bool Checker::Model<C>::push(const ReadAt<LOCAL_COMPLETION>& event) {
        return try_push(event);
    }

    template <typename C>
    inline bool
    Checker::Model<C>::push(const WriteAt<LOCAL_COMPLETION>& event) {
        return try_push(event);
    }

    template <typename C>
    inline bool
    Checker::Model<C>::push(const WriteAt<REMOTE_COMPLETION>& event) {
        return try_push(event);
    }

    template <typename C>
    inline bool Checker::Model<C>::push(const ReadAt<SQUASH>& event) {
        return try_push(event);
    }

    template <typename C>
    inline bool Checker::Model<C>::push(const WriteAt<SQUASH>& event) {
        return try_push(event);
    }

    template <typename C>
    inline bool Checker::Model<C>::push(const WriteAt<AVAILABILITY>& event) {
        return try_push(event);
    }

    template <typename C>
    inline bool Checker::Model<C>::push(const ReadAt<MEMORY>& event) {
        return try_push(event);
    }

    template <typename C>
    inline bool Checker::Model<C>::push(const WriteAt<MEMORY>& event) {
        return try_push(event);
    }

    template <typename C>
    inline bool Checker::Model<C>::push(const ReadAt<ENTER_TIME>& event) {
        return try_push(event);
    }

    template <typename C>
    inline bool Checker::Model<C>::push(const WriteAt<ENTER_TIME>& event) {
        return try_push(event);
    }

    template <typename C>
    inline bool Checker::Model<C>::push(const FenceAt<ENTER_TIME>& event) {
        return try_push(event);
    }

    template <typename C>
    inline bool Checker::Model<C>::push(const ReadAt<COMMIT_TIME>& event) {
        return try_push(event);
    }

    template <typename C>
    inline bool Checker::Model<C>::push(const WriteAt<COMMIT_TIME>& event) {
        return try_push(event);
    }

    template <typename C>
    inline bool Checker::Model<C>::push(const FenceAt<COMMIT_TIME>& event) {
        return try_push(event);
    }

    template <typename C>
    inline const Report& Checker::Model<C>::report() const {
        return checker.report();
    }

    template <typename C>
    inline Time Checker::Model<C>::time() const {
        return time_counter.nanoseconds();
    }

    template <typename C>
    template <typename E>
    inline bool Checker::Model<C>::try_push(const E& event) {
        auto result = false;
        if constexpr (HasPush<C, E>::value) {
            logger.add(event);
            time_counter.start();
            result = checker.push(event);
            time_counter.stop();
        }
        return result;
    }
} // namespace hmv::checker

#endif /* __HMV_CHECKER_CHECKER_HPP__ */
