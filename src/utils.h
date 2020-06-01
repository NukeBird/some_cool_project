#include <memory>
#include <functional>
#include <optional>

template<typename T>
using functional_unique_ptr = std::unique_ptr<T, std::function<void(T*)>>;


//idea from PVS-studio's presentation
template <typename Func>
class lazy
{
    using result_type = std::invoke_result_t<Func>;
    using optional_type = std::conditional_t<std::is_void_v<result_type>, bool, //if void
        std::optional<std::conditional_t<std::is_reference_v<result_type>, std::reference_wrapper<std::remove_reference_t<result_type>>, result_type>>
    >;

    Func initializer;
    optional_type value = {};

public:
    lazy(Func&& initializer) : initializer(std::forward<Func>(initializer)) {}

    lazy() = delete;
    lazy(const lazy& ) = delete;
    lazy(lazy&&) = delete;
    lazy& operator= (const lazy& ) = delete;
    lazy& operator= (lazy&&) = delete;
    ~lazy() = default;

    result_type operator ()()
    {
        if constexpr (std::is_void_v<result_type>)
        {
            if (!value)
            {
                initializer();
                value = true;
            }
        }
        else
        {
            if (!value)
                value = initializer();

            return *value;
        }
    }

    template <typename = std::enable_if_t<!std::is_void_v<result_type>>>
    operator result_type ()
    {
        return operator()();
    }

    void reset()
    {
        value = {};
    }

};