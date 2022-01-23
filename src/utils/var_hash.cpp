#include <cstring>

template <typename ResultT, ResultT OffsetBasis, ResultT Prime>
class basic_fnv1a final
{

    static_assert(std::is_unsigned<ResultT>::value, "need unsigned integer");

    public:

        using result_type = ResultT;

    private:

        result_type state_ {};

    public:

    constexpr
    basic_fnv1a() noexcept : state_ {OffsetBasis}
    {}

    result_type
    hash(const char *const data) noexcept
        __attribute__((always_inline))
        __attribute__((flatten))
        __attribute__((hot))
        __attribute__((optimize("-Ofast")))
    {
        result_type acc = this->state_;
        for (auto i = std::size_t {}; i < strlen(data); ++i)
        {
            const auto next = std::size_t {data[i]};
            acc = (acc ^ next) * Prime;
        }
        return acc;
    }

};
