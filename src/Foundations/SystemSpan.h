#pragma once

#include <initializer_list>

/**
 * Lightweight non-owning mutable view over a contiguous sequence of elements.
 *
 * Span is intentionally passed and copied by value. Copying a Span only copies its pointer and
 * length; it does not copy, allocate, own, or track the lifetime of the referenced elements.
 *
 * Element access and slicing are intentionally unchecked. The caller is responsible for keeping
 * indexes and slices inside the referenced range and for ensuring that the underlying storage
 * remains valid for the lifetime of the Span.
 *
 * A Span<char> or Span<wchar_t> is still only a pointer-and-length view. Some Foundations string
 * helpers allocate an additional null character immediately after Length, but null termination is
 * a property of those produced buffers, not of Span itself. In particular, a slice is not
 * necessarily null-terminated.
 *
 * @tparam T Element type referenced by the span.
 */
template<typename T>
struct Span
{
    /**
     * Constructs an empty Span.
     */
    Span()
    {
        Pointer = nullptr;
        Length = 0;
    }

    /**
     * Constructs a Span over an existing contiguous range.
     *
     * No ownership or lifetime tracking is added.
     *
     * @param pointer Pointer to the first element.
     * @param length Number of elements in the range.
     */
    Span(T* pointer, size_t length)
    {
        Pointer = pointer;
        Length = length;
    }

    T* Pointer;    ///< Pointer to the first element, or nullptr for an empty span.
    size_t Length; ///< Number of elements in the view.

    /**
     * Returns a mutable reference to an element.
     *
     * No bounds check is performed.
     *
     * @param index Element index inside the span.
     * @return Mutable reference to the selected element.
     */
    T& operator[](size_t index) const
    {
        return Pointer[index];
    }

    /**
     * Returns the suffix beginning at start.
     *
     * No bounds check is performed. For character spans, slicing does not guarantee that the
     * returned view is null-terminated at its new Length.
     *
     * @param start Index of the first element in the returned span.
     * @return Span covering [start, Length).
     */
    Span<T> Slice(size_t start) const
    {
        return Span<T>(Pointer + start, Length - start);
    }

    /**
     * Returns a sub-range of this span.
     *
     * No bounds check is performed. For character spans, slicing does not guarantee that the
     * returned view is null-terminated at its new Length.
     *
     * @param start Index of the first element in the returned span.
     * @param length Number of elements in the returned span.
     * @return Span covering [start, start + length).
     */
    Span<T> Slice(size_t start, size_t length) const
    {
        return Span<T>(Pointer + start, length);
    }
};

/**
 * Lightweight non-owning read-only view over a contiguous sequence of elements.
 *
 * ReadOnlySpan is intentionally passed and copied by value. Copying it only copies its pointer and
 * length; it does not copy, allocate, own, or track the lifetime of the referenced elements.
 *
 * Element access and slicing are intentionally unchecked. The caller is responsible for the
 * lifetime of the referenced storage and for keeping indexes and slices inside the referenced range.
 *
 * For ReadOnlySpan<char> and ReadOnlySpan<wchar_t>, construction from a null-terminated string scans
 * up to the terminator and stores the logical character count in Length. The terminator is therefore
 * not part of the span. Other constructors and Slice() do not imply null termination.
 *
 * @tparam T Element type referenced by the span.
 */
template<typename T>
struct ReadOnlySpan
{
    /**
     * Constructs an empty ReadOnlySpan.
     */
    ReadOnlySpan()
    {
        Pointer = nullptr;
        Length = 0;
    }

    /**
     * Constructs a ReadOnlySpan over an existing contiguous range.
     *
     * The source may be const. No ownership or lifetime tracking is added.
     *
     * @param pointer Pointer to the first element.
     * @param length Number of elements in the range.
     */
    ReadOnlySpan(const T* pointer, size_t length)
    {
        Pointer = pointer;
        Length = length;
    }

    /**
     * Constructs a ReadOnlySpan from an std::initializer_list.
     *
     * This is the deliberate STL convenience exception used to keep small call-site lists concise.
     * The elements are not copied. The caller must not retain the resulting ReadOnlySpan beyond the
     * lifetime of the initializer-list backing storage; this constructor is primarily intended for
     * immediate function-call arguments.
     *
     * @param initList Initializer list whose elements are referenced by the span.
     */
    ReadOnlySpan(std::initializer_list<T> initList)
    {
        Pointer = initList.begin();
        Length = initList.size();
    }

    /**
     * Constructs a ReadOnlySpan<char> from a null-terminated character string.
     *
     * Length contains the number of characters before the null terminator. The terminator is not
     * included in Length.
     *
     * @param stringValue Null-terminated character string.
     */
    ReadOnlySpan(const char* stringValue)
    {
        Pointer = stringValue;
        Length = 0;

        while (stringValue[Length] != '\0')
        {
            Length++;
        }
    }

    /**
     * Constructs a ReadOnlySpan<wchar_t> from a null-terminated wide-character string.
     *
     * Length contains the number of characters before the null terminator. The terminator is not
     * included in Length.
     *
     * @param stringValue Null-terminated wide-character string.
     */
    ReadOnlySpan(const wchar_t* stringValue)
    {
        Pointer = stringValue;
        Length = 0;

        while (stringValue[Length] != L'\0')
        {
            Length++;
        }
    }

    /**
     * Constructs a read-only view over a mutable Span.
     *
     * @param spanValue Mutable span whose range will be referenced.
     */
    ReadOnlySpan(Span<T> spanValue)
    {
        Pointer = spanValue.Pointer;
        Length = spanValue.Length;
    }

    const T* Pointer; ///< Pointer to the first element, or nullptr for an empty span.
    size_t Length;    ///< Number of elements in the view.

    /**
     * Returns a read-only reference to an element.
     *
     * No bounds check is performed.
     *
     * @param index Element index inside the span.
     * @return Read-only reference to the selected element.
     */
    const T& operator[](size_t index) const
    {
        return Pointer[index];
    }

    /**
     * Returns the suffix beginning at start.
     *
     * No bounds check is performed. For character spans, slicing does not guarantee that the
     * returned view is null-terminated at its new Length.
     *
     * @param start Index of the first element in the returned span.
     * @return ReadOnlySpan covering [start, Length).
     */
    ReadOnlySpan<T> Slice(size_t start) const
    {
        return ReadOnlySpan<T>(Pointer + start, Length - start);
    }

    /**
     * Returns a sub-range of this span.
     *
     * No bounds check is performed. For character spans, slicing does not guarantee that the
     * returned view is null-terminated at its new Length.
     *
     * @param start Index of the first element in the returned span.
     * @param length Number of elements in the returned span.
     * @return ReadOnlySpan covering [start, start + length).
     */
    ReadOnlySpan<T> Slice(size_t start, size_t length) const
    {
        return ReadOnlySpan<T>(Pointer + start, length);
    }
};
