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

#ifndef __HMV_COVERAGE_METRIC_HPP__
#define __HMV_COVERAGE_METRIC_HPP__

#include <cassert>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#include "hmv/core/types.hpp"
#include "hmv/coverage/fsm_per_instance.hpp"
#include "hmv/coverage/fsm_structural.hpp"

namespace hmv::coverage {
    enum class Type : uint8_t {
        FSM_STRUCTURAL = 0,
        FSM_PER_INSTANCE = 1,
    };

    class Metric {
     public:
        Metric(Type type, size_t max = 1);
        Metric(const Metric& other);
        ~Metric() = default;
        Metric(Metric&& other) noexcept = default;
        Metric& operator=(const Metric& other);
        Metric& operator=(Metric&& other) noexcept = default;
        Metric clone() const;
        void push(const std::string& fsm,
                  const std::string& state,
                  const std::string& event,
                  hmv::Address address,
                  unsigned instance_id);
        bool affects_value(const Metric& other) const;
        void merge(const Metric& other);
        void remove(const Metric& other);
        void reset();
        bool full() const;
        size_t max() const;
        double percentage() const;
        size_t value() const;
        Type type() const;
        void serialize(std::ofstream& file) const;
        void write(const std::filesystem::path& filename) const;
        void write_csv(const std::filesystem::path& filename) const;
        static Metric deserialize(std::ifstream& file);
        static Metric read(const std::filesystem::path& filename);

     private:
        class Concept {
         public:
            virtual ~Concept() = default;
            virtual std::unique_ptr<Concept> clone() const = 0;
            virtual void push(const std::string& fsm,
                              const std::string& state,
                              const std::string& event,
                              hmv::Address address,
                              unsigned instance_id) = 0;
            virtual bool affects_value(const Metric& other) const = 0;
            virtual bool affects_value(const FSMStructural& other) const = 0;
            virtual bool affects_value(const FSMPerInstance& other) const = 0;
            virtual void merge(const Metric& other) = 0;
            virtual void merge(FSMStructural& other) const = 0;
            virtual void merge(FSMPerInstance& other) const = 0;
            virtual void remove(const Metric& other) = 0;
            virtual void remove(FSMStructural& other) const = 0;
            virtual void remove(FSMPerInstance& other) const = 0;
            virtual void reset() = 0;
            virtual size_t value() const = 0;
            virtual void
            write_csv(const std::filesystem::path& filename) const = 0;
            virtual void serialize(std::ofstream& file) const = 0;
        };

        template <typename M>
        class Model final : public Concept {
         public:
            Model(const M& metric);
            Model(std::ifstream& file);
            std::unique_ptr<Concept> clone() const override;
            void push(const std::string& fsm,
                      const std::string& state,
                      const std::string& event,
                      hmv::Address address,
                      unsigned instance_id) override;
            bool affects_value(const Metric& other) const override;
            bool affects_value(const FSMStructural& other) const override;
            bool affects_value(const FSMPerInstance& other) const override;
            void merge(const Metric& other) override;
            void merge(FSMStructural& other) const override;
            void merge(FSMPerInstance& other) const override;
            void remove(const Metric& other) override;
            void remove(FSMStructural& other) const override;
            void remove(FSMPerInstance& other) const override;
            void reset() override;
            size_t value() const override;
            void
            write_csv(const std::filesystem::path& filename) const override;
            void serialize(std::ofstream& file) const override;

         private:
            M metric;
        };

        Type tag;
        size_t max_value;
        std::unique_ptr<Concept> model;

        Metric(Type type, size_t max, std::ifstream& file);
        template <typename M>
        bool _affects_value(const M& other) const;
        template <typename M>
        void _merge(M& other) const;
        template <typename M>
        void _remove(M& other) const;
    };

    inline Type Metric::type() const {
        return tag;
    }

    template <typename M>
    inline bool Metric::_affects_value(const M& other) const {
        return model->affects_value(other);
    }

    template <typename M>
    inline void Metric::_merge(M& other) const {
        model->merge(other);
    }

    template <typename M>
    inline void Metric::_remove(M& other) const {
        model->remove(other);
    }

    template <typename M>
    inline Metric::Model<M>::Model(const M& metric) : metric{metric} {}

    template <typename M>
    inline Metric::Model<M>::Model(std::ifstream& file)
        : metric{M::deserialize(file)} {}

    template <typename M>
    std::unique_ptr<Metric::Concept> Metric::Model<M>::clone() const {
        return std::make_unique<Model<M>>(metric);
    }

    template <typename M>
    inline void Metric::Model<M>::push(const std::string& fsm,
                                       const std::string& state,
                                       const std::string& event,
                                       hmv::Address address,
                                       unsigned instance_id) {
        metric.push(fsm, state, event, address, instance_id);
    }

    template <typename M>
    inline bool Metric::Model<M>::affects_value(const Metric& other) const {
        return other._affects_value(metric);
    }

    template <typename M>
    inline bool
    Metric::Model<M>::affects_value(const FSMStructural& other) const {
        if constexpr (std::is_same<M, FSMStructural>::value) {
            return other.affects_value(metric);
        }
        assert(false);
        return false;
    }

    template <typename M>
    inline bool
    Metric::Model<M>::affects_value(const FSMPerInstance& other) const {
        if constexpr (std::is_same<M, FSMPerInstance>::value) {
            return other.affects_value(metric);
        }
        assert(false);
        return false;
    }

    template <typename M>
    void Metric::Model<M>::merge(const Metric& other) {
        other._merge(metric);
    }

    template <typename M>
    void Metric::Model<M>::merge(FSMStructural& other) const {
        if constexpr (std::is_same<M, FSMStructural>::value) {
            other.merge(metric);
        } else {
            assert(false);
        }
    }

    template <typename M>
    void Metric::Model<M>::merge(FSMPerInstance& other) const {
        if constexpr (std::is_same<M, FSMPerInstance>::value) {
            other.merge(metric);
        } else {
            assert(false);
        }
    }

    template <typename M>
    void Metric::Model<M>::remove(const Metric& other) {
        other._remove(metric);
    }

    template <typename M>
    void Metric::Model<M>::remove(FSMStructural& other) const {
        if constexpr (std::is_same<M, FSMStructural>::value) {
            other.remove(metric);
        } else {
            assert(false);
        }
    }

    template <typename M>
    void Metric::Model<M>::remove(FSMPerInstance& other) const {
        if constexpr (std::is_same<M, FSMPerInstance>::value) {
            other.remove(metric);
        } else {
            assert(false);
        }
    }

    template <typename M>
    inline void Metric::Model<M>::reset() {
        metric.reset();
    }

    template <typename M>
    size_t Metric::Model<M>::value() const {
        return metric.value();
    }

    template <typename M>
    inline void
    Metric::Model<M>::write_csv(const std::filesystem::path& filename) const {
        return metric.write_csv(filename);
    }

    template <typename M>
    inline void Metric::Model<M>::serialize(std::ofstream& file) const {
        return metric.serialize(file);
    }
} // namespace hmv::coverage

#endif /* __HMV_COVERAGE_METRIC_HPP__ */
