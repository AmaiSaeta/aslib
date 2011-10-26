/**
 * @file
 */
#ifndef ASLIB_INCLUDED_RESULT_HPP_
#define ASLIB_INCLUDED_RESULT_HPP_

#include <stdexcept>
#include <type_traits>

namespace aslib {

/**
 * 正常値とエラー値の両方を表現する型. 関数の戻り値として用いる.
 * 通常、関数は決まった型の値しか返せず、関数の処理結果の値と、呼出元へのエラー発生通知を同時に行う為には、どちらかを(ポインタや参照型で)引数に持たなければならない。
 * このクラスを用いる事により、処理結果とエラー通知のいずれかを関数の戻り値のみで表現出来る。
 * @code
 *	aslib::Result<Succeeded, Failed> function() {	// 正常に処理出来た場合は結果をSucceeded型で、エラーが発生した時はその内容をFailed型で返す関数
 *		if(isSucceess) return Succceeded();
 *		else if(isFailure) return Failed();
 * 	}
 *
 *	aslib::Result<Succeeded, Failed> result = function();
 *
 *	if(result) {}	// 正常値かエラー値かはboolへの変換で判断出来る. 正常値であればtrue.
 *
 *	// dereference、或いは->演算子で正常値にアクセス出来る. エラー値であった場合は、aslib::Result::CantDereference例外が発生する.
 *  Succeeded s = *result;
 *  result->member();
 *
 *	// エラー値の値は、failedメンバ関数でアクセス出来る.
 *	result = Failed();
 *  if(!result) { result.failed().memberOfFailed(); }
 * @endcode
 * @param[in]	S	正常値の型.
 * @param[in]	F	エラー値の型.
 * @warning	現在のこのクラスでは、テンプレートパラメータとして参照を用いる事は出来ない.
 * @sa	aslib::Result::CantDereference
 */
template<typename S, typename F>
class Result {
public:	// public types {{{
	/// 正常値の型.
	typedef S SucceededType;
	/// エラー値の型.
	typedef F FailedType;

	/// エラー値を保持したaslib::Resultオブジェクトをdereference、または->演算子を利用した場合に発生する例外.
	struct CantDereference : std::runtime_error {
		CantDereference() : runtime_error("Failed result value.") {}
	};
	/// エラー値を保持していない(正常値或いは未初期化である)事を表す例外.
	struct NotHaveFailedObject : std::runtime_error {
		NotHaveFailedObject() : runtime_error("Not have the Failed object.") {}
	};
// }}}

private: // private types {{{
	typedef Result<SucceededType, FailedType> My;

	/// 内部保持している値の種類.
	enum State {
		uninit,	///< 未初期化. 内部に保持している値は無い.
		succeeded,	///< 正常値.
		failed	///< エラー値.
	};
// }}}

public:	// constructors / destructor {{{
	Result() : state_(uninit) {}
	/// @param[in]	src	正常値.
	Result(const SucceededType &src) { allocate(&src,succeeded); }
	/// @param[in]	src	正常値.
	Result(SucceededType &&src) { allocate_move(&src, succeeded); }
	/// @param[in]	src	エラー値.
	Result(const FailedType &src) { allocate(&src, failed); }
	/// @param[in]	src	エラー値.
	Result(FailedType &&src) { allocate_move(&src, failed); }

	/// @param[in]	src	コピー元.
	Result(const Result &src) {
		if(src.state_ == uninit) return;

		allocate(src.storage_, src.state_);
	}
	/// @param[in]	src	コピー元.
	Result(Result &&src) : state_(uninit) {
		if(src.state_ == uninit) return;

		allocate_move(&src.storage_, src.state_);
	}

	~Result() { destroy(); }
// }}}

public:	// functions / operators
	/// @return	保持している値が正常値かエラー値か. trueならば正常値.
	operator bool() const { return state_ == succeeded; }

	/**
	 * @return	正常値を保持している場合、その値への参照を返す.
	 * @exception	aslib::Result::CantDereference	エラー値を保持している場合に発生する.
	 */
	SucceededType &operator*() { return const_cast<SucceededType &>(const_cast<const My *>(this)->operator*()); }
	/// @overload
	const SucceededType &operator*() const {
		if(!operator bool()) { throw Result::CantDereference(); }
		return *reinterpret_cast<const SucceededType *>(storage_);
	}

	/**
	 * @return	正常値を保持している場合、その値へのポインタを返す.
	 * @exception	aslib::Result::CantDereference	エラー値を保持している場合に発生する.
	 */
	SucceededType *operator->() { return const_cast<SucceededType *>(const_cast<const My *>(this)->operator->()); }
	/// @overload
	const SucceededType *operator->() const {
		if(!operator bool()) { throw Result::CantDereference(); }
		return reinterpret_cast<const SucceededType *>(storage_);
	}

	/**
	 * @return	エラー値を保持している場合に、その値への参照を返す.
	 */
	FailedType &fail() { return const_cast<FailedType &>(const_cast<const My *>(this)->fail()); }
	// @overload
	const FailedType &fail() const {
		if(state_ == failed) return *reinterpret_cast<const FailedType *>(storage_);
		throw NotHaveFailedObject();
	}

	Result &operator=(const SucceededType &rhs) { return substitute(&rhs, succeeded); }
	Result &operator=(SucceededType &&rhs) { return substitute_move(&rhs, succeeded); }
	Result &operator=(const FailedType &rhs) { return substitute(&rhs, failed); }
	Result &operator=(FailedType &&rhs) { return substitute_move(&rhs, failed); }
	Result &operator=(const Result &rhs) { return substitute(rhs.storage_, rhs.state_); }
	Result &operator=(Result &&rhs) { return substitute_move(rhs.storage_, rhs.state_); }
// }}}

private:	// inner functions. {{{
	/**
	 * 指定した型の値を構築する.
	 * 格納型のコピーコンストラクタを使用する.
	 * @param[in]	src	コピー元
	 * @param[in]	state	コピー元の値種別.
	 */
	void allocate(const void *src, State srcState) {
		switch(srcState) {
		case succeeded:
			new(storage_) SucceededType(*static_cast<const SucceededType *>(src));
			break;
		case failed:
			new(storage_) FailedType(*static_cast<const FailedType *>(src));
			break;
		}
		state_ = srcState;
	}
	/**
	 * @overload
	 * 指定した型の値を構築する.
	 * 格納型のデフォルトコンストラクタを使用する.
	 * @param[in]	state	
	 */
	void allocate(State state) {
		assert(state != uninit);

		switch(state) {
		case succeeded: new(storage_) SucceededType(); break;
		case failed: new(storage_) FailedType(); break;
		}
		state_ = state;
	}

	/**
	 * 指定した型の値を構築する.
	 * 格納型のムーブコンストラクタを使用する.
	 * @param[in]	src	コピー元
	 * @param[in]	state	コピー元の値種別.
	 */
	void allocate_move(void *src, State srcState) {
		switch(srcState) {
		case succeeded:
			new(storage_) SucceededType(std::move(*static_cast<SucceededType *>(src)));
			break;
		case failed:
			new(storage_) FailedType(std::move(*static_cast<FailedType *>(src)));
			break;
		}
		state_ = srcState;
	}

	/**
	 * 指定した型の値を代入する.
	 * 格納型に対応した代入演算子を使用する.
	 * @param[in]	src	代入元を指すポインタ.
	 * @param[in]	srcState	代入元の種類.
	 * @return	代入処理を行った自分自身(*this).
	 */
	Result &substitute(const void *src, State srcState) {
		if(state_ != uninit) destroy();
		if(srcState != uninit) allocate(srcState);
		switch(srcState) {
		case succeeded:
			*reinterpret_cast<SucceededType *>(storage_) = *static_cast<const SucceededType *>(src);
			break;
		case failed:
			*reinterpret_cast<FailedType *>(storage_) = *static_cast<const FailedType *>(src);
			break;
		}
		state_ = srcState;

		return *this;
	}
	/**
	 * 指定した型の値を代入する.
	 * 右辺値参照を右辺値とするoperator=を使用する.
	 * @param[in]	src	代入元を指すポインタ.
	 * @param[in]	srcState	代入元の種類.
	 * @return	代入処理を行った自分自身(*this).
	 */
	Result &substitute_move(void *src, State srcState) {
		if(state_ != uninit) destroy();
		if(srcState != uninit) allocate(srcState);
		switch(srcState) {
		case succeeded:
			*reinterpret_cast<SucceededType *>(storage_) = std::move(*static_cast<SucceededType *>(src));
			break;
		case failed:
			*reinterpret_cast<FailedType *>(storage_) = std::move(*static_cast<FailedType *>(src));
			break;
		}
		state_ = srcState;

		return *this;
	}

	/// 内部に保持している値を破棄する.
	void destroy() {
		switch(state_) {
		case succeeded: Destoroyer<SucceededType>()(storage_); break;
		case failed:    Destoroyer<FailedType   >()(storage_); break;
		}
		state_ = uninit;
	}
	/// 実際に値の破棄を行う.
	template<typename T>
	struct Destoroyer {
		void operator()(void *p) { destory<std::is_pod<T>::value>(p); }
	private:
		template<typename bool isPod /* == true */> void destory(void *p) { /* NOTHING */ }
		template<> void destory<false>(void *p) { static_cast<T *>(p)->~T(); }
	};

	/**
	 * 正常値、エラー値双方とも保持可能なサイズを算出するメタ関数.
	 */
	class CalcStorageSize {
		static const std::size_t sSize = sizeof(SucceededType);
		static const std::size_t fSize = sizeof(FailedType);
		static const std::size_t cSize = sizeof(char);
		static const std::size_t storageSize = (sSize > fSize) ? sSize : fSize;
	public:
		static const std::size_t value = storageSize / cSize + ((storageSize % cSize) > 0);
	};
// }}}

private:	// fields {{{

	char storage_[CalcStorageSize::value];
	State state_;
// }}}
};

}	// namespace aslib

#endif	// ASLIB_INCLUDED_RESULT_HPP_
