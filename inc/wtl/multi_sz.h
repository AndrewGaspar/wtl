#pragma once

#include <iterator>
#include <vector>
#include <exception>

namespace wtl
{
    template<typename CharTIt>
    using it_value_t = typename std::iterator_traits<CharTIt>::value_type;

    template<typename char_type>
    constexpr char_type null_char() { return static_cast<char_type>(0); }

    template<typename CharTIt, typename CharT = std::decay_t<it_value_t<CharTIt>>>
    constexpr size_t wstrlen(CharTIt str) { return str[0] == null_char<CharT>() ? 0 : wstrlen(str + 1) + 1; }

    template<typename CharTIt>
    constexpr CharTIt find_next(CharTIt str) { return str + wstrlen(str) + 1; }

    template<typename CharTIt, typename CharT = std::decay_t<it_value_t<CharTIt>>>
    constexpr CharTIt find_last(CharTIt str) { return str[0] == null_char<CharT>() ? str : find_last(find_next(str)); }

    template<typename CharTIt>
    bool is_valid_multi_string_buffer(CharTIt begin, CharTIt end)
    {
        const auto null = null_char<std::decay_t<it_value_t<CharTIt>>>();
        const auto size = end - begin;

        // (void buffer)
        if (size == 0)
        {
            return true;
        }

        // \0
        if (size == 1)
        {
            return begin[0] == null;
        }

        // verify two nulls at the end, and that 'last' string points to end - 1
        if (end[-1] == null && end[-2] == null)
        {
            return find_last(begin) == end - 1;
        }

        return false;
    }

    // When we get a better compiler, this whole thing could be constexpr
    template<typename CharTIt, typename CharT = std::decay_t<it_value_t<CharTIt>>>
    class multi_string_view_iterator : public std::iterator<std::bidirectional_iterator_tag, CharT const *, ptrdiff_t, CharT const * const *, CharT const *>
    {
        CharTIt start, sz;
        using char_type = CharT;
    public:

        using base_iterator = CharTIt;

        multi_string_view_iterator(base_iterator start, base_iterator sz) noexcept : start(start), sz(sz)
        {

        }

        explicit multi_string_view_iterator(base_iterator sz) noexcept : multi_string_view_iterator(sz, sz) { }

        bool operator==(multi_string_view_iterator const & other) const noexcept
        {
            return sz == other.sz;
        }

        bool operator!=(multi_string_view_iterator const & other) const noexcept
        {
            return sz != other.sz;
        }

        reference operator*() const noexcept
        {
            return &sz[0];
        }

        pointer operator->() const noexcept
        {
            return &sz;
        }

        multi_string_view_iterator & operator++() noexcept
        {
            while (*sz++ != null_char<char_type>());

            return *this;
        }

        multi_string_view_iterator operator++(int) noexcept
        {
            auto prev = *this;
            ++(*this);
            return prev;
        }

        multi_string_view_iterator & operator--() noexcept
        {
            do
            {
                sz--;
            } while (sz[-1] != null_char<char_type>() && sz != start);

            return *this;
        }

        multi_string_view_iterator operator--(int) noexcept
        {
            auto prev = *this;
            --(*this);
            return prev;
        }

        base_iterator const & base() const
        {
            return sz;
        }
    };

    template<typename char_type>
    class multi_string_view
    {
        using string_type = char_type const *;

        string_type start, stop = nullptr;

    public:
        using iterator = multi_string_view_iterator<string_type>;
        using const_iterator = iterator;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = reverse_iterator;

        explicit multi_string_view(string_type start) noexcept : start(start), stop(find_last(start)) { }

        const_iterator begin() const noexcept
        {
            return const_iterator(start);
        }

        const_reverse_iterator rbegin() const noexcept
        {
            return const_reverse_iterator(end());
        }

        const_iterator end() const noexcept
        {
            return const_iterator(start, stop);
        }

        const_reverse_iterator rend() const noexcept
        {
            return const_reverse_iterator(begin());
        }
    };

    class invalid_multi_string_error : public std::domain_error
    {
    public:
        invalid_multi_string_error() : std::domain_error("Attempted to create a wtl::multi_string with a buffer that is not a valid multi string")
        {

        }
    };

    template<typename CharT, typename Alloc = std::allocator<CharT>>
    class multi_string
    {
        using char_type = CharT;
        using vector_type = std::vector<char_type, Alloc>;

        vector_type buffer;

        bool initialize()
        {
            if (buffer.empty())
            {
                buffer.push_back(null_char<char_type>());
                return true;
            }

            return false;
        }

    public:
        using value_type = CharT const *;
        using allocator_type = Alloc;
        using reference = value_type const &;
        using const_reference = reference;

        using iterator = multi_string_view_iterator<typename vector_type::const_iterator>;
        using const_iterator = iterator;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = reverse_iterator;

        multi_string() { }

        template<typename It>
        multi_string(It begin, It end) : multi_string(vector_type(begin, end)) { }

        template<size_t N>
        multi_string(const char_type (&str)[N]) : multi_string(std::begin(str), std::end(str)) { }

        explicit multi_string(vector_type&& input)
        {
            if (!is_valid_multi_string_buffer(input.begin(), input.end()))
            {
                throw invalid_multi_string_error();
            }

            buffer = std::move(input);
        }

        const_iterator begin() const noexcept
        {
            return const_iterator(buffer.cbegin());
        }

        const_reverse_iterator rbegin() const noexcept
        {
            return const_reverse_iterator(end());
        }

        const_iterator end() const noexcept
        {
            if (buffer.empty())
            {
                return const_iterator(buffer.cbegin(), buffer.cend());
            }

            return const_iterator(buffer.cbegin(), buffer.cend() - 1);
        }

        const_reverse_iterator rend() const noexcept
        {
            return const_reverse_iterator(begin());
        }

        const_iterator insert(const_iterator position, value_type const & str)
        {
            if (initialize())
            {
                position = begin();
            }

            return const_iterator(buffer.cbegin(), buffer.insert(position.base(), str, find_next(str)));
        }

        template<typename InputIterator>
        const_iterator insert(const_iterator position, InputIterator first, InputIterator last)
        {
            if (initialize())
            {
                position = begin();
            }

            auto startIndex = position.base() - buffer.begin();

            auto it = position;

            for (auto in = first; in != last; in++)
            {
                it = insert(it, *in);
                it++;
            }

            return const_iterator(buffer.begin(), buffer.begin() + startIndex);
        }

        const_iterator insert(const_iterator position, std::initializer_list<value_type> il)
        {
            return insert(position, std::begin(il), std::end(il));
        }

        void push_back(value_type const & str)
        {
            initialize();

            insert(end(), str);
        }

        vector_type const & view_buffer() const
        {
            return buffer;
        }

        typename vector_type::size_type get_buffer_size() const
        {
            return buffer.size();
        }

        vector_type take_buffer()
        {
            return std::move(buffer);
        }
    };

    using multi_sz_view = multi_string_view<wchar_t>;
    using multi_sz = multi_string<wchar_t>;
}