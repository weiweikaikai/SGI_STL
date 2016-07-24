#pragma once

//迭代器的型别

//只读迭代器
struct InputIteratorTag{};
//只写迭代器
struct OutputIteratorTag {};
//前向迭代器
struct ForwardIteratorTag : public InputIteratorTag {};
//双向迭代器
struct BidirectionalIteratorTag : public ForwardIteratorTag {};
//随机访问迭代器
struct RandomAccessIteratorTag : public BidirectionalIteratorTag {};


//迭代器内嵌包含五种相应的型别
template<class Category,class T,class Distance=int,class Pointer=T*,class Reference=T&>
struct Iterator
{
	typedef Category  IteratorCategory;		// 迭代器类型
	typedef T         ValueType;			// 迭代器所指对象类型
	typedef Distance  DifferenceType;		// 两个迭代器之间的距离
	typedef Pointer   Pointer;				// 迭代器所指对象类型的指针
	typedef Reference Reference;			// 迭代器所指对象类型的引用
};

//
// Traits就像一台“特性萃取机”，榨取各个迭代器的特性（对应的型别）
template <class Iterator> //迭代器的类型萃取
struct IteratorTraits
{
	typedef typename Iterator::IteratorCategory IteratorCategory;
	typedef typename Iterator::ValueType        ValueType;
	typedef typename Iterator::DifferenceType   DifferenceType;
	typedef typename Iterator::Pointer           Pointer;
	typedef typename Iterator::Reference         Reference;
};

// 偏特化原生指针类型
template <class T>//随机迭代器的
struct IteratorTraits<T*>
{
	typedef RandomAccessIteratorTag IteratorCategory;
	typedef T                          ValueType;
	typedef ptrdiff_t                  DifferenceType;
	typedef T*                         Pointer;
	typedef T&                         Reference;
};

// 偏特化const原生指针类型
template <class T>
struct IteratorTraits<const T*>
{
	typedef RandomAccessIteratorTag IteratorCategory;
	typedef T                          ValueType;
	typedef ptrdiff_t                  DifferenceType;
	typedef const T*                   Pointer;
	typedef const T&                   Reference;
};

template <class InputIterator>
inline typename IteratorTraits<InputIterator>::DifferenceType
__Distance(InputIterator first, InputIterator last, InputIteratorTag)
{
	IteratorTraits<InputIterator>::DifferenceType n = 0;
	while (first != last) {
		++first; ++n;
	}
	return n;
}

template <class RandomAccessIterator>
inline typename IteratorTraits<RandomAccessIterator>::DifferenceType
__Distance(RandomAccessIterator first, RandomAccessIterator last,
		   RandomAccessIteratorTag)
{
	return last - first;
}

template <class InputIterator>
inline typename IteratorTraits<InputIterator>::DifferenceType
Distance(InputIterator first, InputIterator last)
{
	return __Distance(first, last, IteratorTraits<InputIterator>::IteratorCategory());
}

template <class Iterator>
inline typename IteratorTraits<Iterator>::ValueType*
ValueType(const Iterator&)
{
	return static_cast<typename IteratorTraits<Iterator>::ValueType*>(0);
}