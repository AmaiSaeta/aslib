#ifndef ASLIB_INCLUDED_DEEPCOPYPTR_HPP_
#define ASLIB_INCLUDED_DEEPCOPYPTR_HPP_

#include <cassert>

namespace aslib {

/**
 * 深いコピーをサポートするスマートポインタ.
 * 格納するオブジェクトはCopy-Constructiveで、且つ、同じ型のオブジェクトからの代入を行うoperator=()が定義されていなければならない.
 * 各publicメンバの名称は、標準ライブラリのスマートポインタと名称を揃えている.
 */
template<typename Type>
class DeepCopyPtr {
	typedef DeepCopyPtr<Type> ThisType;

public:
	/// 指し示すオブジェクトの型を表す.
	typedef Type element_type;

	/**
	 * コンストラクタ.
	 * 初期化しなかった場合はNULLを指す.
	 * @param[in]	p	格納するオブジェクトを指すポインタ.
	 */
	DeepCopyPtr()
		: ptr_(0), deleter_(0)
	{ }
	explicit
	DeepCopyPtr(element_type *p)
		: ptr_(p), deleter_(p ? new Deleter<element_type>() : 0)
	{ }
	template<typename DerivedType>
//	explicit
	DeepCopyPtr(DerivedType *p)	// class T; class U:T; Ptr<T> p = new U();
		: ptr_(p), deleter_(p ? new Deleter<DerivedType>() : 0)
	{ }

	/**
	 * コピーコンストラクタ.
	 * コピー元がDeepCopyPtrの時はコピーとして扱う.
	 * @param[in]	src	コピー元オブジェクト.
	 */
	explicit
	DeepCopyPtr(const ThisType &src)
		: ptr_(src.get() ? new element_type(*(src.get())) : 0), deleter_(src.get() ? new Deleter<element_type>() : 0)
	{ }
	template<typename SomeType>
	explicit
	DeepCopyPtr(const DeepCopyPtr<SomeType> &src)
		: ptr_(src.get() ? new SomeType(*(src.get())) : 0), deleter_(src.get() ? new Deleter<SomeType>() : 0)
	{ }

	/**
	 * コピーコンストラクタ.
	 * コピー元が const element_type * ならば、named valueに決まっているのでコピーとして扱う.
	 * @param[in]	src	コピー元を指すポインタ.
	 */
	DeepCopyPtr(const element_type *src)
		: ptr_(src ? new element_type(*src) : 0), deleter_(src ? new Deleter<element_type>() : 0)
	{ }

	~DeepCopyPtr() {
		doDelete();
	}

	/**
	 * @return	保持する生ポインタ.
	 */
	element_type *get() { return ptr_; }
	const element_type *get() const { return ptr_; }

	/**
	 * 保持するポインタのswap.
	 */
	void swap(ThisType &other) {
		std::swap(ptr_, other.ptr_);
		std::swap(deleter_, other.deleter_);
	}

	/**
	 * ポインタの値を変更する.
	 * @param[in,out]	変更先ポインタ. 呼出時点で指し示していたオブジェクトはdeleteされる. 省略時はNULL.
	 */
	void reset(element_type *p = 0) {	// 0->0,0->1,1->0,1->1の4パターン?
		if(p == ptr_) return;

		ThisType(p).swap(*this);
	}
	template<typename Other>
	void reset(Other *p) {
		ThisType(p).swap(*this);
	}

	/// 通常のポインタと同等に使えるよう、->演算子の定義.
	element_type *operator->() const { assert(ptr_); return ptr_; }
	/// 通常のポインタと同等に使えるよう、*演算子の定義.
	element_type &operator*() const { assert(ptr_); return *ptr_;}

	/// @return	同じオブジェクトを指し示しているならばtrue.
	bool operator==(const ThisType &rhs) const {
		return (ptr_ == rhs.ptr_);
	}

	/// @return	違うオブジェクトを指し示しているならばtrue.
	bool operator!=(const ThisType &rhs) const {
		return !this->operator==(rhs);
	}

	/**
	 * 深いコピーを行う.
	 * 右辺のポインタが指し示すオブジェクトをコピーし、そのコピーしたオブジェクトのポインタを取得する.
	 */
	ThisType &operator=(const ThisType &rhs) {	// DeepCopyPtr p1, p2; p1 = p2;
		if(this == &rhs) return *this;

		doDelete();

		if(rhs) {
			ptr_ = new element_type(*rhs.ptr_);
			deleter_ = new Deleter<element_type>();
		}

		return *this;
	}
	template<typename DerivedType>
	ThisType &operator=(const DeepCopyPtr<DerivedType> &rhs) {	// DeepCopyPtr<T> tp; DeepCopyPtr<U> up; tp = up;
		doDelete();

		if(rhs) {
			ptr_ = new element_type(*rhs.get());
			deleter_ = new Deleter<DerivedType>();
		}

		return *this;
	}
	ThisType &operator=(const element_type *const rhs) {
		if(get() == rhs) return *this;

		doDelete();
		ptr_ = rhs ? new element_type(*rhs) : 0;
		deleter_ = new Deleter<element_type>();

		return *this;
	}
	template<typename SomeType>
	ThisType &operator=(const SomeType *const rhs) {
		doDelete();
		ptr_ = rhs ? new SomeType(*rhs) : 0;
		deleter_ = new Deleter<SomeType>();

		return *this;
	}

	template<typename SomeTypePtr>
	bool operator==(const SomeTypePtr rhs) const {
		return get() == static_cast<element_type *>(rhs);
	}

	template<typename SomeTypePtr>
	bool operator!=(const SomeTypePtr rhs) const {
		return !operator==(rhs);
	}

	/// @return	オブジェクトを指し示している(=NULLポインタではない)ならばtrue.
	operator bool() const { return get() != 0; }

private:
	/// 動的削除子のベースとなる型.
	struct DeleterBase {
		virtual ~DeleterBase() {};
		virtual void operator()(void *p) const = 0;
	};
	/// 指し示すオブジェクトの削除を担当する動的削除子.
	template<typename TargetType>
	struct Deleter : public DeleterBase {
		virtual ~Deleter() {}
		virtual void operator()(void *p) const {
			delete static_cast<TargetType *>(p);
		}
	};

	/**
	 * 指しているオブジェクト(及び、動的削除子)をdeleteする.
	 */
	void doDelete() {
		if(!deleter_) return;
		(*deleter_)(ptr_);
		delete deleter_;
		ptr_ = 0;
		deleter_ = 0;
	}

	/// 指し示しているオブジェクト.
	Type *ptr_;

	/// ptr_が指しているオブジェクトを削除する為の動的削除子.
	DeleterBase *deleter_;
};

}	// namespace aslib

#endif

